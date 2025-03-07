package com.example.geckowatch.presentation

import androidx.compose.runtime.Composable
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.example.geckowatch.BLEApplication
import com.example.geckowatch.MainActivity

@Composable
fun Navigation(
    getActivityContext: ()->MainActivity,
    viewModel: SmartWatchViewModel
) {

    val navController = rememberNavController()

    NavHost(navController = navController, startDestination = Screen.StartScreen){
        composable(Screen.StartScreen){
            StartScreen(
                getActivityContext = getActivityContext,
                navController = navController,
                viewModel = viewModel
            )
        }
        composable(Screen.SmartWatchScreen){
            SmartWatchScreen(
                getActivityContext = getActivityContext,
                navController = navController,
                viewModel = viewModel
            )
        }
    }
}

object Screen {
    var StartScreen = "start_screen"
    var SmartWatchScreen = "smart_watch_screen"
}

//sealed class Screen(val route:String){
//    data object StartScreen:Screen("start_screen")
//    data object SmartWatchScreen:Screen("smart_watch_screen")
//}
