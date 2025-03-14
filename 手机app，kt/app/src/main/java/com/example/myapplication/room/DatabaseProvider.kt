package com.example.myapplication.room


import android.content.Context
import androidx.room.Room

object DatabaseProvider {
    @Volatile
    private var INSTANCE: AppDatabase? = null

    fun getDatabase(context: Context): AppDatabase {
        return INSTANCE ?: synchronized(this) {
            val instance = Room.databaseBuilder(
                context.applicationContext,
                AppDatabase::class.java,
                "device_data_database"
            )
                .fallbackToDestructiveMigration() // 当 schema 变更时，自动销毁重建数据库
                .build()
            INSTANCE = instance
            instance
        }
    }
}
