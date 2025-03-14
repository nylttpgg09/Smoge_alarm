package com.example.myapplication.room

import androidx.room.Entity
import androidx.room.Index
import androidx.room.PrimaryKey

@Entity(
    tableName = "device_data",
    indices = [
        Index(value = ["attribute", "timestamp"], name = "idx_attr_time")
    ]
)
data class DeviceData(
    @PrimaryKey(autoGenerate = true) val id: Int = 0,
    val attribute: String,
    val value: String,
    val timestamp: Long = System.currentTimeMillis()
)