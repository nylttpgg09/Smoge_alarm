// FeeViewModel.kt
package com.example.myapplication.ui.fee

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import androidx.room.Room
import com.example.myapplication.billing.FeeCalculator
import com.example.myapplication.room.AppDatabase
import com.example.myapplication.room.DatabaseProvider
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch

class FeeViewModel(application: Application) : AndroidViewModel(application) {

    // 使用单例数据库
    private val database: AppDatabase by lazy {
        DatabaseProvider.getDatabase(application)
    }

    // 2. 延迟创建 FeeCalculator
    private val calculator by lazy {
        FeeCalculator(database.deviceDataDao())
    }

    // 3. Flow -> StateFlow
    val feeResult: StateFlow<FeeCalculator.FeeResult> = calculator.calculateFeesFlow()
        .stateIn(
            scope = viewModelScope,
            started = SharingStarted.WhileSubscribed(5000),
            initialValue = FeeCalculator.FeeResult(0.0, 0.0, false)
        )

    init {
        startCleanupJob()
    }

    private fun startCleanupJob() {
        viewModelScope.launch {
            // 这里可以调用数据库的清理操作
            // val cutoff = System.currentTimeMillis() - (30L * 24 * 60 * 60 * 1000)
            // database.deviceDataDao().cleanupOldFeeData(cutoff)
        }
    }
}
