package com.example.geckowatch.presentation

import android.bluetooth.BluetoothAdapter
import android.graphics.Paint.Align
import android.util.Log
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.navigation.NavController
import com.example.geckowatch.BLEApplication
import com.example.geckowatch.MainActivity
import com.example.geckowatch.presentation.permissions.PermissionUtils
import com.example.geckowatch.presentation.permissions.SystemBroadcastReceiver
import com.google.accompanist.permissions.ExperimentalPermissionsApi
import com.google.accompanist.permissions.rememberMultiplePermissionsState

@OptIn(ExperimentalPermissionsApi::class)
@Composable
fun StartScreen(
    getActivityContext: ()->MainActivity,
    navController: NavController,
    viewModel: SmartWatchViewModel
) {

    // Grouping of required permissions to request at once
    val permissionState = rememberMultiplePermissionsState(permissions = PermissionUtils.bluetoothPermissions)

    // Setup a broadcast listener to notify upon changes on bluetooth state
    SystemBroadcastReceiver(systemAction = BluetoothAdapter.ACTION_STATE_CHANGED){bluetoothState ->
        val action = bluetoothState?.action ?: return@SystemBroadcastReceiver
        if (action == BluetoothAdapter.ACTION_STATE_CHANGED){
            getActivityContext().onBluetoothChange()
        }
    }

    Column(
        Modifier
            .fillMaxWidth()
            .fillMaxHeight()
            .padding(20.dp),
        verticalArrangement = Arrangement.Center,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text("Bluetooth permissions?: ${viewModel.bluetoothPermissionsState}")
        Text("Notification permissions?: ${viewModel.notificationListenerPermissionState}")
        Text("Bluetooth enabled?: ${viewModel.bluetoothEnabledState}")

        // If we have all the required permission, navigate to the smart watch screen
        if (viewModel.bluetoothPermissionsState && viewModel.notificationListenerPermissionState && viewModel.bluetoothEnabledState) {
            Button(
                onClick = {
                    navController.navigate(Screen.SmartWatchScreen) {
                        popUpTo(Screen.StartScreen) {
                            inclusive = true
                        }
                    }
                },
                shape = RoundedCornerShape(20.dp),
                modifier = Modifier.padding(20.dp)
            ) {
                Text(text = "Interact with Watch")
            }
        } else {
            Text("Cannot attempt watch connection, not all permissions enabled.")
        }

        // Button to enable bluetooth permissions
        Button(
            onClick = {
                permissionState.launchMultiplePermissionRequest()
            },
            shape = RoundedCornerShape(20.dp),
            modifier = Modifier.padding(20.dp)
        ) {
            Text("Enable bluetooth permissions")
        }

        // Button to navigate to notification permission screen, user will have to enable them manually
        Button(
            onClick = {
                getActivityContext().navigateToNotificationPermissions()
            },
            shape = RoundedCornerShape(20.dp),
            modifier = Modifier.padding(20.dp)
        ) {
            Text("Enable notification listener permissions")
        }
    }
}

