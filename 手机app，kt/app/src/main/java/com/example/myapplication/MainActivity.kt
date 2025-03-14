package com.example.myapplication

import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.viewModels
import androidx.compose.foundation.Image
import androidx.lifecycle.lifecycleScope
import androidx.compose.foundation.layout.*
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.example.myapplication.ui.theme.MyApplicationTheme
import com.example.myapplication.huaweiIOT  //
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import androidx.room.Room
import com.example.myapplication.room.AppDatabase
import com.example.myapplication.room.DeviceData
import com.example.myapplication.ui.fee.FeeViewModel
import androidx.compose.ui.res.painterResource // 确保导入这个
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Android
import androidx.compose.material.icons.outlined.Android
import androidx.compose.material3.Icon


class MainActivity : ComponentActivity() {
    // 使用 mutableStateOf 保存设备属性数据，初始为空 Map
    private val attributeData = mutableStateOf<Map<String, String>>(emptyMap())

    // 使用单例数据库
    private lateinit var db: com.example.myapplication.room.AppDatabase

    // FeeViewModel 注入
    val feeViewModel: FeeViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        // 使用单例 DatabaseProvider 获取数据库实例
        db = com.example.myapplication.room.DatabaseProvider.getDatabase(applicationContext)

        setContent {
            MyApplicationTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    Column(modifier = Modifier.padding(innerPadding)) {
                        AttributeScreen(attributeData = attributeData.value)
                        Spacer(modifier = Modifier.height(16.dp))
                        FeeScreen(viewModel = feeViewModel)
                    }
                }
            }
        }


        // 显示 Toast 提示，提醒用户查看控制台输出数据
        Toast.makeText(this, "请查看Android Studio 控制台输出数据", Toast.LENGTH_LONG).show()

        // 利用 lifecycleScope 周期性获取数据
        lifecycleScope.launch(Dispatchers.IO) {
            val hwiot = huaweiIOT()  // HuaweiIOT 类必须在项目中已正确实现
            // 定义需要获取的属性列表（注意确保名称与返回 JSON 字段一致）
            val attributes = listOf("Light", "Hum", "CO", "Smg", "Temp","Fee")
            while (true) {
                val result = mutableMapOf<String, String>()
                for (attribute in attributes) {
                    try {
                        // 判空，避免空指针
                        val rawValue = hwiot.getAtt(attribute, "shadow") ?: "N/A"
                        println("$attribute 上报值: $rawValue")

                        // 将成功获取的数据保存到 result 中
                        result[attribute] = rawValue

                        // 写入数据库，显式传入 timestamp
                        db.deviceDataDao().insert(
                            DeviceData(
                                attribute = attribute,
                                value = rawValue,
                                timestamp = System.currentTimeMillis()
                            )
                        )

                    } catch (e: Exception) {
                        e.printStackTrace()
                        result[attribute] = "error"
                    }
                }
                // 切换到主线程更新状态，进而刷新 UI
                withContext(Dispatchers.Main) {
                    attributeData.value = result
                }
                // 每隔 3 秒更新一次数据
                delay(3000L)
            }
        }
    }
}

@Composable
fun AttributeScreen(attributeData: Map<String, String>, modifier: Modifier = Modifier) {
    Column(modifier = modifier.padding(16.dp)) {
        Text("设备属性数据", style = androidx.compose.material3.MaterialTheme.typography.titleLarge)
        Spacer(modifier = Modifier.height(8.dp))
        if (attributeData.isEmpty()) {
            Text("正在加载数据...")
        } else {
            // 逐行显示各属性及其上报值
            attributeData.forEach { (attribute, value) ->
                Text("$attribute : $value")
                Spacer(modifier = Modifier.height(4.dp))
            }
        }
    }
}


@Preview(showBackground = true)
@Composable
fun AttributeScreenPreview() {
    MyApplicationTheme {
        // 预览示例数据
        AttributeScreen(
            attributeData = mapOf(
                "Light" to "N/A",
                "Hum" to "N/A",
                "CO" to "N/A",
                "Smg" to "N/A",
                "Temp" to "N/A"
            )
        )
    }
}
@Composable
fun FeeScreen(viewModel: FeeViewModel) {
    val feeState by viewModel.feeResult.collectAsState()

    Column(modifier = Modifier.padding(16.dp)) {
        Text(
            text = "总费用：¥${"%.2f".format(feeState.totalFee)}",
            style = MaterialTheme.typography.headlineMedium,
            color = MaterialTheme.colorScheme.primary
        )

        Spacer(modifier = Modifier.height(16.dp))

        Row(verticalAlignment = Alignment.CenterVertically) {
            Icon(
                imageVector = if (feeState.isBilling) Icons.Filled.Android
                else Icons.Outlined.Android,
                contentDescription = "状态指示",
                tint = if (feeState.isBilling) Color.Green else Color.Gray,
                modifier = Modifier.size(24.dp)
            )

            Spacer(modifier = Modifier.width(8.dp))

            Text(
                text = if (feeState.isBilling) "当前计费：¥${"%.2f".format(feeState.currentFee)}"
                else "未在计费中",
                style = MaterialTheme.typography.bodyLarge
            )
        }
    }
}