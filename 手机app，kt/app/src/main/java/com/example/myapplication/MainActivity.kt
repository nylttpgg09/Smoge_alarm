package com.example.myapplication

import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.viewModels
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.*  // 注意：全部来自 Material 2
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Android
import androidx.compose.material.icons.outlined.Android
//import androidx.compose.material3.MaterialTheme
//import androidx.compose.material3.Scaffold
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.lifecycle.lifecycleScope
import com.example.myapplication.room.DatabaseProvider
import com.example.myapplication.room.DeviceData
import com.example.myapplication.ui.fee.FeeViewModel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext



class MainActivity : ComponentActivity() {

    // 设备属性：Map<属性名, 值>，用于实时存储
    private val attributeData = mutableStateOf<Map<String, String>>(emptyMap())

    // 费用相关的 ViewModel
    private val feeViewModel: FeeViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // 获取数据库单例
        val db = DatabaseProvider.getDatabase(applicationContext)

        setContent {
            // 只使用 Material 2 时，自己定义的主题可以在下面调用
            // 如果没有特意定义，可以使用默认:
            MaterialTheme {
                // 主界面
                MyAppContent(attributeData = attributeData.value, feeViewModel = feeViewModel)
            }
        }

        Toast.makeText(this, "请查看Android Studio 控制台输出数据", Toast.LENGTH_LONG).show()

        // 周期性获取设备属性并写入数据库
        lifecycleScope.launch(Dispatchers.IO) {
            val hwiot = huaweiIOT()  // 示例：请替换成您实际的获取方式
            val attributes = listOf("Fire", "Hum", "Smg", "Temp", "Fee")
            while (true) {
                val result = mutableMapOf<String, String>()
                for (attr in attributes) {
                    try {
                        val rawValue = hwiot.getAtt(attr, "shadow") ?: "N/A"
                        println("$attr 上报值: $rawValue")

                        db.deviceDataDao().insert(
                            DeviceData(
                                attribute = attr,
                                value = rawValue,
                                timestamp = System.currentTimeMillis()
                            )
                        )
                        result[attr] = rawValue
                    } catch (e: Exception) {
                        e.printStackTrace()
                        result[attr] = "error"
                    }
                }
                withContext(Dispatchers.Main) {
                    attributeData.value = result
                }
                delay(3000L)
            }
        }
    }
}

/**
 * 整个App的主内容，使用 Material 2 (Scaffold, TopAppBar)
 */
@Composable
fun MyAppContent(
    attributeData: Map<String, String>,
    feeViewModel: FeeViewModel
) {
    var showHistory by remember { mutableStateOf(false) }

    // Material 2 Scaffold
    Scaffold(
        topBar = {
            // Material 2 TopAppBar
            TopAppBar(
                title = {
                    // Material 2 里的 Typography 用法
                    Text("智慧设备监控", style = MaterialTheme.typography.h6)
                },
                backgroundColor = MaterialTheme.colors.primarySurface,  // primarySurface是Material 2可用
                contentColor = Color.White,  // 或 contentColorFor(backgroundColor)
                actions = {
                    // 在右侧放一个按钮，切换“历史记录”界面
                    Button(onClick = { showHistory = !showHistory }) {
                        Text(if (showHistory) "返回" else "历史记录")
                    }
                },
                elevation = 4.dp
            )
        }
    ) { innerPadding ->
        // 根据标记 showHistory 切换不同的界面
        if (showHistory) {
            // 历史记录界面
            HistoryScreen(viewModel = feeViewModel)
        } else {
            // 主界面：属性展示 & 费用展示
            Column(
                modifier = Modifier
                    .padding(innerPadding)
                    .fillMaxSize()
                    .padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(16.dp)
            ) {
                // Material 2 Card
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    elevation = 4.dp
                ) {
                    AttributeScreen(attributeData)
                }

                Card(
                    modifier = Modifier.fillMaxWidth(),
                    elevation = 4.dp
                ) {
                    FeeScreen(viewModel = feeViewModel)
                }
            }
        }
    }
}

/**
 * 显示设备属性
 */
@Composable
fun AttributeScreen(attributeData: Map<String, String>) {
    Column(modifier = Modifier.padding(16.dp)) {
        // Material 2 的 text style: subtitle1 / subtitle2 / body1 / body2 / h6 / etc.
        Text(
            text = "设备属性数据",
            style = MaterialTheme.typography.subtitle1,
            color = MaterialTheme.colors.primary
        )
        Spacer(modifier = Modifier.height(8.dp))

        if (attributeData.isEmpty()) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                CircularProgressIndicator(
                    modifier = Modifier
                        .size(24.dp)
                        .padding(end = 8.dp)
                )
                Text("正在加载数据...", style = MaterialTheme.typography.body1)
            }
        } else {
            LazyColumn(verticalArrangement = Arrangement.spacedBy(4.dp)) {
                items(attributeData.toList()) { (attr, value) ->
                    Text(
                        text = "$attr : $value",
                        style = MaterialTheme.typography.body1
                    )
                }
            }
        }
    }
}

/**
 * 显示费用信息，并在超过阈值时弹窗
 */
@Composable
fun FeeScreen(viewModel: FeeViewModel) {
    val feeState by viewModel.feeResult.collectAsState()

    // 简易阈值
    val threshold = 2000.0
    val showAlert = remember { mutableStateOf(false) }

    // 监听 totalFee，超过时弹窗
    LaunchedEffect(feeState.totalFee) {
        if (feeState.totalFee >= threshold) {
            showAlert.value = true
        }
    }

    if (showAlert.value) {
        // Material 2 AlertDialog
        AlertDialog(
            onDismissRequest = { showAlert.value = false },
            title = { Text("费用提醒", style = MaterialTheme.typography.h6) },
            text = { Text("当前总费用已达 ${feeState.totalFee} 元，超出阈值 ${threshold} 元！", style = MaterialTheme.typography.body1) },
            confirmButton = {
                Button(onClick = { showAlert.value = false }) {
                    Text("确定")
                }
            }
        )
    }

    // 展示计费信息
    Column(modifier = Modifier.padding(16.dp)) {
        Text(
            text = "总费用：¥${"%.2f".format(feeState.totalFee)}",
            style = MaterialTheme.typography.h6,
            color = MaterialTheme.colors.primary
        )
        Spacer(modifier = Modifier.height(16.dp))

        Row(verticalAlignment = Alignment.CenterVertically) {
            Icon(
                imageVector = if (feeState.isBilling) Icons.Filled.Android else Icons.Outlined.Android,
                contentDescription = null,
                tint = if (feeState.isBilling) Color.Green else Color.Gray,
                modifier = Modifier.size(24.dp)
            )
            Spacer(modifier = Modifier.width(8.dp))
            Text(
                text = if (feeState.isBilling)
                    "当前计费：¥${"%.2f".format(feeState.currentFee)}"
                else "未在计费中",
                style = MaterialTheme.typography.body1
            )
        }
    }
}

/**
 * 示例：查看历史记录
 * 假设在 FeeViewModel 里有 getRecentRecords(days: Int): Flow<List<DeviceData>>
 */
@Composable
fun HistoryScreen(viewModel: FeeViewModel) {
    val recentRecordsFlow = viewModel.getRecentRecords(days = 7)
    val recentRecords by recentRecordsFlow.collectAsState(initial = emptyList())

    Column(modifier = Modifier.padding(16.dp)) {
        Text(
            text = "最近 7 天历史记录",
            style = MaterialTheme.typography.subtitle1,
            color = MaterialTheme.colors.primary
        )
        Spacer(modifier = Modifier.height(8.dp))

        if (recentRecords.isEmpty()) {
            Text("没有历史数据或正在加载...", style = MaterialTheme.typography.body1)
        } else {
            LazyColumn(verticalArrangement = Arrangement.spacedBy(4.dp)) {
                items(recentRecords) { record ->
                    Text(
                        text = "时间：${record.timestamp} / ${record.attribute} = ${record.value}",
                        style = MaterialTheme.typography.body2
                    )
                }
            }
        }
    }
}
