package com.example.myapplication.room

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import kotlinx.coroutines.flow.Flow

@Dao
interface DeviceDataDao {
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insert(deviceData: DeviceData) // 正确：无返回类型或返回 Long

    @Query("SELECT * FROM device_data ORDER BY timestamp DESC")
    fun getAllData(): Flow<List<DeviceData>> // 正确：使用 Flow 观察数据

    // 新增以下两个查询方法
    @Query("SELECT * FROM device_data WHERE attribute = 'Fee' ORDER BY timestamp ASC")
    fun getFeeRecords(): Flow<List<DeviceData>>

    @Query("SELECT * FROM device_data WHERE attribute = 'Fee' AND timestamp >= :startTime ORDER BY timestamp ASC")
    fun getRecentFeeRecords(startTime: Long): Flow<List<DeviceData>>

    // 新增数据清理方法
    @Query("DELETE FROM device_data WHERE timestamp < :cutoff AND attribute = 'Fee'")
    suspend fun cleanupOldFeeData(cutoff: Long)
}