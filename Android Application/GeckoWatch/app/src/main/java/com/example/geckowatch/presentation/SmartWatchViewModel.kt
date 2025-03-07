package com.example.geckowatch.presentation

import android.util.Log
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.example.geckowatch.data.ConnectionState
import com.example.geckowatch.data.SmartWatchReceiveManager
import com.example.geckowatch.util.Resource
import kotlinx.coroutines.launch

class SmartWatchViewModel() : ViewModel() {

    private var smartWatchReceiveManager: SmartWatchReceiveManager ?= null

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

    fun setBLEManger(bleManager: SmartWatchReceiveManager) {
        if (smartWatchReceiveManager == null) {
            smartWatchReceiveManager = bleManager
            connectionState = bleManager.getConnectionState() ?: ConnectionState.Uninitialized
        }
    }

    // Function to listen for changes in our BLE manager, done through MutableSharedFlow
    // This is how we are alerted of changes in connection status
    private fun subscribeToChanges(){
        viewModelScope.launch {
            smartWatchReceiveManager?.data?.collect{ result ->
                when (result){
                    is Resource.Success -> {
                        connectionState = result.data.connectionState
                        batteryVoltage = result.data.batteryVoltage
                    }

                    is Resource.Loading -> {
                        statusMessage = result.message
                        connectionState = ConnectionState.CurrentlyInitializing
                    }

                    is Resource.Error -> {
                        statusMessage = result.errorMessage
                        connectionState = ConnectionState.Error
                    }
                }
            }
        }
    }

    // Start the BLE scan/connection process through our BLE manager
    fun initializeConnection() {
        // Make sure we get updated to the change in BLE connection state
        subscribeToChanges()

        // Start the process of connecting to watch
        smartWatchReceiveManager?.startReceiving()
    }

    fun readVoltage(){
        smartWatchReceiveManager?.readBatteryVoltage()
    }

    fun disconnect(){
        smartWatchReceiveManager?.disconnect() // Just disconnect but remember connection info
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
