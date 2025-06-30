package com.example.geckowatch.presentation

import android.os.Build
import android.util.Log
import androidx.annotation.RequiresApi
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.geckowatch.data.ConnectionState
import com.example.geckowatch.data.ble.SmartWatchBLEReceiveManager
import kotlinx.coroutines.launch

class SmartWatchViewModel() : ViewModel() {

    private var smartWatchReceiveManager: SmartWatchBLEReceiveManager ?= null

    // Permission state variables
    var bluetoothPermissionsState by mutableStateOf<Boolean>(false)
        private set

    var notificationListenerPermissionState by mutableStateOf<Boolean>(false)
        private set

    var bluetoothEnabledState by mutableStateOf<Boolean>(false)
        private set

    // Bluetooth connection state variables
    var statusMessage by mutableStateOf<String?>(null)
        private set

    var batteryVoltage by mutableFloatStateOf(0f)
        private set

    var connectionState by mutableStateOf<ConnectionState>(ConnectionState.Uninitialized)

    fun updatePermissionsState(bluetoothPermission: Boolean, notificationPermission: Boolean, bluetoothEnabled: Boolean){
        bluetoothPermissionsState = bluetoothPermission
        notificationListenerPermissionState = notificationPermission
        bluetoothEnabledState = bluetoothEnabled
        Log.i("VIEW MODEL", "Permissions Updated")
    }

    fun setBLEManger(bleManager: SmartWatchBLEReceiveManager) {
        smartWatchReceiveManager = bleManager

        // Update state and subscribe to new changes
        connectionState = bleManager.getConnectionState()
        subscribeToChanges()
    }

    // Function to listen for changes in our BLE manager, done through MutableSharedFlow
    // This is how we are alerted of changes in connection status
    private fun subscribeToChanges(){
        viewModelScope.launch {
            smartWatchReceiveManager?.data?.collect{ result ->
                connectionState = result.connectionState
                statusMessage = result.message
                batteryVoltage = result.batteryVoltage
            }
        }
    }

    // Start the BLE scan/connection process through our BLE manager
    fun initializeConnection() {
        smartWatchReceiveManager?.startReceiving()
    }

    fun readVoltage(){
        smartWatchReceiveManager?.readBatteryVoltage()
    }

    fun disconnect(){
        smartWatchReceiveManager?.disconnect()
    }

    fun reconnect(){
        smartWatchReceiveManager?.reconnect()
    }

    fun updateWatchTime() {
        smartWatchReceiveManager?.updateWatchTime()
    }

    fun closeConnection(){
        smartWatchReceiveManager?.closeConnection()
    }

}
