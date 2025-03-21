package com.example.myapplication.billing

import com.example.myapplication.room.DeviceData
import com.example.myapplication.room.DeviceDataDao
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.channelFlow
import java.util.Calendar

class FeeCalculator(private val dao: DeviceDataDao) {
    companion object {
        // 默认的基准费率，可在 calculateDuration 中根据时间段切换
        private const val PEAK_HOURLY_RATE = 12.0   // 高峰期每小时12元
        private const val OFF_PEAK_HOURLY_RATE = 6.0 // 低峰期每小时6元
        private const val THIRTY_DAYS_MILLIS = 30L * 24 * 60 * 60 * 1000
    }

    data class FeeResult(
        val totalFee: Double,
        val currentFee: Double,
        val isBilling: Boolean
    )

    // 通过 Flow 实时计算
    fun calculateFeesFlow(): Flow<FeeResult> = channelFlow {
        var lastResult: FeeResult? = null
        dao.getFeeRecords().collect { allRecords ->
            val now = System.currentTimeMillis()
            val recentRecords = allRecords.filter {
                it.timestamp >= now - THIRTY_DAYS_MILLIS
            }
            val result = calculate(recentRecords, allRecords)
            if (result != lastResult) {
                send(result)
                lastResult = result
            }
        }
    }

    private fun calculate(
        recentRecords: List<DeviceData>,
        allRecords: List<DeviceData>
    ): FeeResult {
        var totalFee = 0.0
        var currentFee = 0.0
        var isBilling = false
        var pendingStart: Long? = null

        var lastStatus = 0
        for (record in allRecords) {
            val currentStatus = record.value.toIntOrNull() ?: continue
            if (currentStatus !in 0..1) continue

            when {
                // 从0->1：开始计费
                lastStatus == 0 && currentStatus == 1 -> {
                    pendingStart = record.timestamp
                }
                // 从1->0：停止计费
                lastStatus == 1 && currentStatus == 0 -> {
                    pendingStart?.let {
                        totalFee += calculateDuration(it, record.timestamp)
                        pendingStart = null
                    }
                }
            }
            lastStatus = currentStatus
        }

        // 如果最后仍在计费状态
        if (pendingStart != null) {
            currentFee = calculateDuration(pendingStart!!, System.currentTimeMillis())
            isBilling = true
        }

        // totalFee 的值有时可以直接复用，或者你也可像原逻辑一样单独去计算「30天内总费用」
        // 这里我们简单使用上面迭代过程中的 totalFee，再结合 30 天内的限制：
        val finalTotal = recentRecords
            .filter { it.value.toIntOrNull() in 0..1 }
            .fold(
                Accumulator(
                    sum = 0.0,
                    startTime = null,
                    lastStatus = 0
                )
            ) { acc, record ->
                val state = record.value.toInt()
                when {
                    acc.lastStatus == 0 && state == 1 -> {
                        // 0->1
                        acc.copy(startTime = record.timestamp, lastStatus = 1)
                    }
                    acc.lastStatus == 1 && state == 0 -> {
                        // 1->0
                        val newSum = acc.startTime?.let { st ->
                            acc.sum + calculateDuration(st, record.timestamp)
                        } ?: acc.sum
                        acc.copy(sum = newSum, startTime = null, lastStatus = 0)
                    }
                    else -> acc.copy(lastStatus = state)
                }
            }.sum

        return FeeResult(
            totalFee = finalTotal.roundToTwoDecimal(),
            currentFee = currentFee.roundToTwoDecimal(),
            isBilling = isBilling
        )
    }

    /**
     * 简易的「分时费率」计算示例：
     * 按小时拆分，从 start 到 end，每个小时判断是高峰或低峰。
     */
    private fun calculateDuration(start: Long, end: Long): Double {
        if (end <= start) return 0.0
        var totalFee = 0.0
        var currentTime = start

        while (currentTime < end) {
            val nextHour = getNextHourTime(currentTime)
            val sliceEnd = minOf(nextHour, end)
            val durationMillis = sliceEnd - currentTime

            // 判断 currentTime 所在的时段是高峰还是低峰
            val rate = if (isPeakHour(currentTime)) PEAK_HOURLY_RATE else OFF_PEAK_HOURLY_RATE

            // 叠加该时间片的费用
            totalFee += (durationMillis / (1000.0 * 60 * 60)) * rate

            currentTime = sliceEnd
        }
        return totalFee
    }

    // 判断某个时间是否为高峰期（示例：7:00~22:00 为高峰）
    private fun isPeakHour(timeMillis: Long): Boolean {
        val calendar = Calendar.getInstance().apply { timeInMillis = timeMillis }
        val hour = calendar.get(Calendar.HOUR_OF_DAY)
        return hour in 7..21  // 7:00 <= hour <= 21:59
    }

    // 取当前时间所在小时的下一个整点，用于把区间按小时切分
    private fun getNextHourTime(timeMillis: Long): Long {
        val cal = Calendar.getInstance().apply { timeInMillis = timeMillis }
        cal.set(Calendar.MINUTE, 0)
        cal.set(Calendar.SECOND, 0)
        cal.set(Calendar.MILLISECOND, 0)
        cal.add(Calendar.HOUR_OF_DAY, 1)
        return cal.timeInMillis
    }

    data class Accumulator(
        val sum: Double,
        val startTime: Long?,
        val lastStatus: Int
    )

    private fun Double.roundToTwoDecimal() = "%.2f".format(this).toDouble()
}
