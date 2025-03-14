package com.example.myapplication.billing
import com.example.myapplication.room.DeviceData
import com.example.myapplication.room.DeviceDataDao
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.channelFlow
class FeeCalculator(private val dao: DeviceDataDao) {
    companion object {
        private const val HOURLY_RATE = 10.0 // 每小时费率
        private const val THIRTY_DAYS_MILLIS = 30L * 24 * 60 * 60 * 1000
    }

    data class FeeResult(
        val totalFee: Double,
        val currentFee: Double,
        val isBilling: Boolean
    )

    // 带Flow的实时计算
    fun calculateFeesFlow(): Flow<FeeResult> = channelFlow {
        var lastResult: FeeResult? = null

        dao.getFeeRecords().collect { allRecords ->
            val recentRecords = allRecords.filter {
                it.timestamp >= System.currentTimeMillis() - THIRTY_DAYS_MILLIS
            }

            val result = calculate(recentRecords, allRecords)
            if (result != lastResult) {
                send(result)
                lastResult = result
            }
        }
    }

    private fun calculate(recentRecords: List<DeviceData>, allRecords: List<DeviceData>): FeeResult {
        var totalFee = 0.0
        var currentFee = 0.0
        var isBilling = false
        var pendingStart: Long? = null

        // 处理所有记录（用于检测当前状态）
        var lastStatus = 0
        for (record in allRecords) {
            val currentStatus = record.value.toIntOrNull() ?: continue
            if (currentStatus !in 0..1) continue

            when {
                // 有效状态切换：0 -> 1
                lastStatus == 0 && currentStatus == 1 -> {
                    pendingStart = record.timestamp
                }
                // 有效状态切换：1 -> 0
                lastStatus == 1 && currentStatus == 0 -> {
                    pendingStart?.let {
                        totalFee += calculateDuration(record.timestamp - it)
                        pendingStart = null
                    }
                }
            }
            lastStatus = currentStatus
        }

        // 处理当前状态
        if (pendingStart != null) {
            currentFee = calculateDuration(System.currentTimeMillis() - pendingStart!!)
            isBilling = true
        }

        // 计算30天内费用（需排除pendingStart在30天外的情况）
        totalFee = recentRecords
            .filter { it.value.toIntOrNull() in 0..1 }
            .fold(Pair(0.0, null as Long?)) { (sum, startTime), record ->
                when (record.value.toInt()) {
                    1 -> if (startTime == null) Pair(sum, record.timestamp) else Pair(sum, startTime)
                    0 -> startTime?.let {
                        val duration = maxOf(record.timestamp - it, 0)
                        Pair(sum + calculateDuration(duration), null)
                    } ?: Pair(sum, null)
                    else -> Pair(sum, startTime)
                }
            }.first

        return FeeResult(
            totalFee = totalFee.roundToTwoDecimal(),
            currentFee = currentFee.roundToTwoDecimal(),
            isBilling = isBilling
        )
    }

    private fun calculateDuration(millis: Long): Double {
        return (millis / (1000.0 * 60 * 60)) * HOURLY_RATE
    }

    private fun Double.roundToTwoDecimal() = "%.2f".format(this).toDouble()
}