package com.example.geckowatch.data.ble

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothProfile
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.Build
import android.util.Log
import androidx.annotation.RequiresApi
import com.example.geckowatch.data.ConnectionState
import com.example.geckowatch.data.DisconnectRational
import com.example.geckowatch.data.SmartWatchResult
import com.example.geckowatch.util.Resource
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.launch
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.time.Instant
import java.util.TimeZone
import java.util.UUID


@SuppressLint("MissingPermission")
class SmartWatchBLEReceiveManager(
    private val bluetoothAdapter: BluetoothAdapter,
    private val context: Context
) {

    companion object {
        private const val DEVICE_NAME = "Gecko"
        private const val SMART_WATCH_SERVICE_UUID = "1f96e240-7e6e-452c-ab50-3e0feb504976"
        private const val BATTERY_LEVEL_CHARACTERISTIC_UUID = "1f96e242-7e6e-452c-ab50-3e0feb504976"
        private const val NOTIFICATION_CHARACTERISTIC_UUID = "1f96e241-7e6e-452c-ab50-3e0feb504976"
        private const val UPDATE_WATCH_TIME_CHARACTERISTIC_UUID = "1f96e243-7e6e-452c-ab50-3e0feb504976"

        private const val MAXIMUM_CONNECTION_ATTEMPTS = 5
    }

    // Smart watch specific connection information, used to confirm services and issue a direct connection
    // The watch only has one BLE service with 2 characteristics,
    //      battery level (read) and notifications (write)
    private var geckoDevice: BluetoothDevice ?= null

    // Shared data with anyone (just our view model) that wants to be updated on connection state
    val data: MutableSharedFlow<SmartWatchResult> = MutableSharedFlow()
    @Volatile private var isScanning = false
    @Volatile private var connectionStatus: ConnectionState = ConnectionState.Uninitialized
    private val coroutineScope = CoroutineScope(Dispatchers.Default) // Used to send the messages

    /*
       BLE Scanner variables
    */
    private var disconnectRational: DisconnectRational = DisconnectRational.None
    private var currentConnectionAttempt = 0
    private val bleScanner by lazy {
        bluetoothAdapter.bluetoothLeScanner
    }
    private val scanSettings = ScanSettings.Builder()
        .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
        .build()

    private val scanCallback = object : ScanCallback(){
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            // Check for Gecko, ignore all other devices
            if (result.device.name == DEVICE_NAME){
                // Let anyone listening that we found the watch
                coroutineScope.launch {
                    data.emit(SmartWatchResult(0.0f, connectionStatus, "Connecting to device..."))
                }

                // Attempt to make the connection
                // We use autoConnect false here to immediately connect but not issue reconnects
                //      for a faster connection process. If we are disconnected we then re-issue
                //      the connection with auto connect enabled.
                geckoDevice = result.device
                result.device.connectGatt(context, false, gattCallback)
                bleScanner.stopScan(this)
                isScanning = false
            }
        }
    }

    /*
       GATT instance representing our active connection
    */
    private var gatt: BluetoothGatt? = null

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(incomingGatt: BluetoothGatt, status: Int, newState: Int) {
            when (status) {
                BluetoothGatt.GATT_SUCCESS -> {
                    when (newState) {
                        BluetoothProfile.STATE_CONNECTED -> {
                            connectionStatus = ConnectionState.Connected
                            currentConnectionAttempt = 0
                            gatt = incomingGatt
                            incomingGatt.discoverServices()
                            coroutineScope.launch {
                                data.emit(SmartWatchResult(0.0f, connectionStatus, "Discovering Services..."))
                            }
                        }
                        BluetoothProfile.STATE_DISCONNECTED -> {
                            when (disconnectRational) {
                                DisconnectRational.None -> {
                                    // Unwanted disconnect, try a sticky reconnect
                                    connectionStatus = ConnectionState.Disconnected
                                    issueStickyBluetoothConnection()
                                    coroutineScope.launch {
                                        data.emit(SmartWatchResult(0.0f, connectionStatus, "Unwanted Disconnect"))
                                    }
                                }
                                DisconnectRational.Disconnect -> {
                                    connectionStatus = ConnectionState.Disconnected
                                    disconnectRational = DisconnectRational.None
                                    coroutineScope.launch {
                                        data.emit(SmartWatchResult(0.0f, connectionStatus, "Disconnect successful"))
                                    }
                                }
                                DisconnectRational.Close -> {
                                    incomingGatt.close()
                                    gatt = null
                                    connectionStatus = ConnectionState.Uninitialized
                                    disconnectRational = DisconnectRational.None
                                    coroutineScope.launch {
                                        data.emit(SmartWatchResult(0.0f, connectionStatus, "GATT closed"))
                                    }
                                }
                            }
                        }
                    }
                }
                else -> {
                    // Some sort of GATT connection error, could be a timeout or a generic disconnect
                    when (newState) {
                        BluetoothProfile.STATE_CONNECTED -> {
                            // This is a weird scenario, we have an error but we are still connected
                            connectionStatus = ConnectionState.Connected
                            currentConnectionAttempt = 0
                            gatt = incomingGatt
                            coroutineScope.launch {
                                data.emit(SmartWatchResult(0.0f, connectionStatus, "Error but connected"))
                            }
                        }
                        BluetoothProfile.STATE_DISCONNECTED -> {
                            if (connectionStatus == ConnectionState.Connected) {
                                // We were connected but an error caused a disconnect, try to reconnect
                                connectionStatus = ConnectionState.Disconnected
                                issueStickyBluetoothConnection()
                                coroutineScope.launch {
                                    data.emit(SmartWatchResult(0.0f, connectionStatus, "Unwarranted Disconnect. Trying to connect..."))
                                }
                            } else {
                                // We were not connected so we were trying to connect but failed
                                // Restart connection process if we haven't exhausted our retry attempts
                                incomingGatt.close()
                                connectionStatus = ConnectionState.Uninitialized
                                gatt = null
                                if (currentConnectionAttempt <= MAXIMUM_CONNECTION_ATTEMPTS) {
                                    currentConnectionAttempt += 1
                                    coroutineScope.launch {
                                        data.emit(SmartWatchResult(0.0f, connectionStatus, "Attempting to connect $currentConnectionAttempt/$MAXIMUM_CONNECTION_ATTEMPTS"))
                                    }
                                    startReceiving()
                                } else {
                                    // We have exhausted our timeout, user will have to re-initiate
                                    currentConnectionAttempt = 0
                                    coroutineScope.launch {
                                        data.emit(SmartWatchResult(0.0f, connectionStatus, "Could not connect to Gecko"))
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            with (gatt) {
                // We have successfully read the available services, try to increase the MTU size
                // We'd prefer a larger MTU size so we can send larger blocks of data at one time
                printGattTable()
                coroutineScope.launch {
                    data.emit(SmartWatchResult(0.0f, connectionStatus, "Adjusting MTU space..."))
                }
                gatt.requestMtu(517)
            }
        }

        override fun onMtuChanged(gatt: BluetoothGatt, mtu: Int, status: Int) {
            // Once the MTU size is updated we are done with the complete connection process
            Log.d("BluetoothGatt", "MTU was updated to $mtu")
            coroutineScope.launch {
                data.emit(SmartWatchResult(0.0f, connectionStatus, "MTU Updated to $mtu"))
            }
        }

        // This is a characteristic read from us (the phone) to the watch (the watch is responding)
        override fun onCharacteristicRead(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            value: ByteArray,
            status: Int
        ) {
            when (characteristic.uuid) {
                UUID.fromString(BATTERY_LEVEL_CHARACTERISTIC_UUID) -> {
                    when (status) {
                        BluetoothGatt.GATT_SUCCESS -> {
                            // Data comes in little endian
                            Log.i("BluetoothGattCallback", "Incoming battery voltage byte array: ${value.toHexString()}")
                            val buffer = ByteBuffer.wrap(value).order(ByteOrder.LITTLE_ENDIAN)
                            val batteryVoltage = buffer.float
                            coroutineScope.launch {
                                data.emit(SmartWatchResult(batteryVoltage, connectionStatus, "Voltage Updated"))
                            }
                        }
                        else -> {
                            // Unlikely to happen but may be from the characteristic not being set as readable
                            Log.e(
                                "BluetoothGattCallback",
                                "Characteristic read failed for BATTERY_LEVEL_CHARACTERISTIC_UUID, error: $status"
                            )
                        }
                    }
                }
                else -> {
                    // Unlikely to happen as we requested the read, but you never know
                    Log.e(
                        "BluetoothGattCallback",
                        "Read UUID not recognized"
                    )
                }
            }
        }
    }

    /*
       Helper functions
    */
    fun ByteArray.toHexString(): String =
        joinToString(separator = " ", prefix = "0x") { String.format("%02X", it) }

    private fun issueStickyBluetoothConnection() {
        currentConnectionAttempt = 0
        geckoDevice?.connectGatt(context, true, gattCallback)
    }

    /*
       Public functions to interact with
    */
    // This is the entry point for starting the connection process, it is the only pre connected
    //      function that is called
    fun startReceiving() {
        // Although the disconnect and close process is asynchronous, the scanning will issue
        //      connection retries until the close process is finished (if it doesn't finish immediately)
        if (connectionStatus != ConnectionState.Uninitialized) closeConnection()

        // Let anyone that is listening know that we are scanning
        // If we are already scanning and this is a re-attempt, don't override previous message
        if (!isScanning) {
            currentConnectionAttempt = 0
            bleScanner.startScan(null, scanSettings, scanCallback)
            isScanning = true
            connectionStatus = ConnectionState.CurrentlyInitializing
            coroutineScope.launch {
                data.emit(SmartWatchResult(0.0f, connectionStatus, "Scanning BLE devices..."))
            }
        }
    }

    fun readBatteryVoltage() {
        if (connectionStatus == ConnectionState.Connected) {
            // Get battery characteristic's handle
            val batteryLevelCharacteristic =
                gatt?.getService(UUID.fromString(SMART_WATCH_SERVICE_UUID))
                    ?.getCharacteristic(UUID.fromString(BATTERY_LEVEL_CHARACTERISTIC_UUID))

            // Initiate read of battery characteristic
            // The result is handled in the gatt callback object
            if (batteryLevelCharacteristic?.isReadable() == true) {
                gatt?.readCharacteristic(batteryLevelCharacteristic)
            }
        }
    }

    fun writeNotification(payload: ByteArray) {
        // This will only send the payload it will not formulate the notification
        if (connectionStatus == ConnectionState.Connected) {
            // Get notification characteristic's handle
            val notificationCharacteristic =
                gatt?.getService(UUID.fromString(SMART_WATCH_SERVICE_UUID))
                    ?.getCharacteristic(UUID.fromString(NOTIFICATION_CHARACTERISTIC_UUID))

            // Write characteristic needs to know if we expect a response, find out
            val writeType = when {
                notificationCharacteristic?.isWritable() == true -> BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
                notificationCharacteristic?.isWritableWithoutResponse() == true -> BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
                else -> Log.e(
                    "BluetoothWriteNotification",
                    "Notification characteristic cannot be written to"
                )
            }

            // Initiate write to notification characteristic data
            if (notificationCharacteristic != null) {
                gatt?.writeCharacteristic(notificationCharacteristic, payload, writeType)
            } else {
                Log.e("BluetoothWriteNotification", "FAILED")
            }
        }
    }

    fun updateWatchTime() {
        if (connectionStatus == ConnectionState.Connected) {
            // Get notification characteristic's handle
            val notificationCharacteristic =
                gatt?.getService(UUID.fromString(SMART_WATCH_SERVICE_UUID))
                    ?.getCharacteristic(UUID.fromString(UPDATE_WATCH_TIME_CHARACTERISTIC_UUID))

            // Write characteristic needs to know if we expect a response, find out
            val writeType = when {
                notificationCharacteristic?.isWritable() == true -> BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
                notificationCharacteristic?.isWritableWithoutResponse() == true -> BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
                else -> Log.e(
                    "BluetoothWriteNotification",
                    "Notification characteristic cannot be written to"
                )
            }

            // Create the payload, just the raw bytes of the Long holding the epoch time
            // Make sure to account for the device's time zone and daylight savings
            var currentEpoch = Instant.now().epochSecond
            currentEpoch += (TimeZone.getDefault().getOffset(currentEpoch * 1000) / 1000)
            Log.i("UpdatingTime", "Adjusted Timestamp: $currentEpoch")
            val result = ByteArray(8)
            for (i in 7 downTo 0) {
                result[i] = (currentEpoch and 0xFF).toByte()
                currentEpoch = currentEpoch shr 8
            }

            // Initiate write to notification characteristic data
            if (notificationCharacteristic != null) {
                gatt?.writeCharacteristic(notificationCharacteristic, result, writeType)
            } else {
                Log.e("BluetoothUpdateWatchTime", "FAILED")
            }
        }
    }

    fun getConnectionState(): ConnectionState {
        return connectionStatus
    }

    fun reconnect() {
        gatt?.connect()
    }

    // Force disconnect from the watch, this does not void the connection just disconnects
    fun disconnect() {
        if (gatt != null) {
            disconnectRational = DisconnectRational.Disconnect
            gatt?.disconnect()
        }
    }

    // Force disconnect and completely close the connection, after calling you will need to
    //     start connection process over
    // This function is useful to stop the scanning process if the watch is not being found
    fun closeConnection() {
        // Force stop any active scan in case it's results try to issue a connection
        bleScanner.stopScan(scanCallback)
        isScanning = false

        when (connectionStatus) {
            ConnectionState.Connected -> {
                // We do not call gatt.close() until we disconnect successfully
                // The closing process will finish in the gatt onConnectionChange callback
                disconnectRational = DisconnectRational.Close
                gatt?.disconnect()
            }
            ConnectionState.Disconnected -> {
                // If we are already disconnected we can immediately close the gatt connection
                connectionStatus = ConnectionState.Uninitialized
                disconnectRational = DisconnectRational.None
                gatt?.close()
                gatt = null
                coroutineScope.launch {
                    data.emit(SmartWatchResult(0.0f, connectionStatus, "Gatt Closed"))
                }
            }
            else -> {
                // If we aren't/weren't connected just reset entire connection state
                connectionStatus = ConnectionState.Uninitialized
                disconnectRational = DisconnectRational.None
                gatt = null
                coroutineScope.launch {
                    data.emit(SmartWatchResult(0.0f, connectionStatus, "Reset Connection State"))
                }
            }
        }
    }

}