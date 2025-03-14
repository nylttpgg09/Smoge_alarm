package com.example.myapplication.ui.fee

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.compose.ui.platform.ComposeView
import androidx.fragment.app.Fragment
import androidx.fragment.app.viewModels
import com.example.myapplication.FeeScreen
import com.example.myapplication.ui.theme.MyApplicationTheme
import androidx.compose.ui.res.painterResource // 确保导入这个
class FeeFragment : Fragment() {
    private val viewModel: FeeViewModel by viewModels()

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View = ComposeView(requireContext()).apply {
        setContent {
            MyApplicationTheme {
                FeeScreen(viewModel = viewModel)
            }
        }
    }
}