package com.example.myapplication.room

import androidx.room.Database
import androidx.room.RoomDatabase

@Database(entities = [DeviceData::class], version = 1, exportSchema = false)
abstract class AppDatabase : RoomDatabase() {
    abstract fun deviceDataDao(): DeviceDataDao
}
