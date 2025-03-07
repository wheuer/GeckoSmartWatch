package com.example.geckowatch.presentation

import android.bluetooth.BluetoothAdapter
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.navigation.NavController
import com.example.geckowatch.MainActivity
import com.example.geckowatch.data.ConnectionState
import com.example.geckowatch.presentation.permissions.SystemBroadcastReceiver

@Composable
fun SmartWatchScreen(
    getActivityContext: ()-> MainActivity,
    navController: NavController,
    viewModel: SmartWatchViewModel
) {

    // Setup a broadcast listener to notify upon changes on bluetooth state
    SystemBroadcastReceiver(systemAction = BluetoothAdapter.ACTION_STATE_CHANGED){bluetoothState ->
        val action = bluetoothState?.action ?: return@SystemBroadcastReceiver
        if (action == BluetoothAdapter.ACTION_STATE_CHANGED){
            getActivityContext().onBluetoothChange()
        }
    }

    // If at any time we do not have the proper bluetooth and notification listener permissions,
    //      or bluetooth is disabled, go back to the start screen
    LaunchedEffect (key1 = viewModel.bluetoothEnabledState,
        key2 = viewModel.bluetoothPermissionsState,
        key3 = viewModel.notificationListenerPermissionState) {
        if (!(viewModel.bluetoothPermissionsState && viewModel.notificationListenerPermissionState && viewModel.bluetoothEnabledState)) {
            navController.navigate(Screen.StartScreen) {
                popUpTo(Screen.SmartWatchScreen) {
                    inclusive = true
                }
            }
        }
    }

    Column(
        modifier = Modifier
            .fillMaxWidth()
            .fillMaxHeight()
            .padding(20.dp),
        verticalArrangement = Arrangement.Center,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth(1f)
                .aspectRatio(1f)
                .border(
                    BorderStroke(
                        5.dp, Color.Blue
                    ),
                    RoundedCornerShape(10.dp)
                ),
            verticalArrangement = Arrangement.Center,
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            when (viewModel.connectionState) {
                ConnectionState.Uninitialized -> {
                    // Initial state, no connection attempts have been made
                    // We just want to offer the option to start the connection process
                    Text("Ready to attempt connection.")
                    Button(
                        onClick = {
                            viewModel.initializeConnection()
                        },
                        shape = RoundedCornerShape(20.dp)
                    ) {
                        Text(text = "Attempt Connection")
                    }
                }

                ConnectionState.CurrentlyInitializing -> {
                    // In the process of trying to connect to watch
                    // We are attempting to connect but have not succeeded, just display that info
                    Text("Attempting Connection...")
                    Text("Status: ${viewModel.statusMessage}")
                }

                ConnectionState.Connected -> {
                    // We are connected to the watch and ready to talk to it
                    // Display that we are connected and offer the ability to disconnect or read battery voltage
                    Text("Connected.")
                    Text("Battery Voltage: ${viewModel.batteryVoltage}")
                    Button(
                        onClick = {
                            viewModel.readVoltage()
                        },
                        shape = RoundedCornerShape(20.dp)
                    ) {
                        Text(text = "Read battery voltage")
                    }
                    Button(
                        onClick = {
                            viewModel.updateWatchTime()
                        },
                        shape = RoundedCornerShape(20.dp)
                    ) {
                        Text(text = "Update Watch Time")
                    }
                    Button(
                        onClick = {
                            viewModel.disconnect()
                        },
                        shape = RoundedCornerShape(20.dp)
                    ) {
                        Text(text = "Disconnect")
                    }
                }

                ConnectionState.Disconnected -> {
                    // We were previously connected and have been disconnected
                    // Display that we have been disconnected and offer to try and reconnect
                    Text("Disconnected.")
                    Button(
                        onClick = {
                            viewModel.reconnect()
                        },
                        shape = RoundedCornerShape(20.dp)
                    ) {
                        Text(text = "Attempt Reconnect")
                    }
                    Button(
                        onClick = {
                            viewModel.closeConnection()

                        },
                        shape = RoundedCornerShape(20.dp)
                    ) {
                        Text(text = "Close Connection")
                    }
                }

                ConnectionState.Error -> {
                    // We got an error while trying to connect or some other error
                    // Display the error message but don't offer anything just yet
                    // TODO: Depending on the error states we implement maybe handle differently
                    Text("Error: ${viewModel.statusMessage}")
                }
            }
        }
    }
}