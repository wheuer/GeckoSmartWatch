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
import com.example.geckowatch.data.SmartWatchReceiveManager
import com.example.geckowatch.util.Resource
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
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
) : SmartWatchReceiveManager {

    // Smart watch specific connection information, used to confirm services
    // The watch only has one BLE service with 2 characteristics,
    //      battery level (read) and notifications (write)
    private var geckoDevice: BluetoothDevice ?= null
    private val DEVICE_NAME = "Gecko"
    private val SMART_WATCH_SERVICE_UUID = "1f96e240-7e6e-452c-ab50-3e0feb504976"
    private val BATTERY_LEVEL_CHARACTERISTIC_UUID = "1f96e242-7e6e-452c-ab50-3e0feb504976"
    private val NOTIFICATION_CHARACTERISTIC_UUID = "1f96e241-7e6e-452c-ab50-3e0feb504976"
    private val UPDATE_WATCH_TIME_CHARACTERISTIC_UUID = "1f96e243-7e6e-452c-ab50-3e0feb504976"

    // Shared data with anyone (just our view model) that wants to be updated on connection state
    override val data: MutableSharedFlow<Resource<SmartWatchResult>> = MutableSharedFlow()
    private var isScanning = false
    private var connectionStatus: ConnectionState ?= null
    private val coroutineScope = CoroutineScope(Dispatchers.Default) // Used to send the messages

    /*
       BLE Scanner variables
    */
    private var disconnectRational: DisconnectRational = DisconnectRational.None
    private var currentConnectionAttempt = 1
    private val MAXIMUM_CONNECTION_ATTEMPTS = 5
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
                    data.emit(Resource.Loading(message = "Connecting to device..."))
                }

                // Attempt to make the connection
                if (isScanning) {
                    // We use autoConnect false here to immediately connect but not issue reconnects
                    //      if we are disconnected we then re-issue the connect with auto connect
                    geckoDevice = result.device
                    result.device.connectGatt(context, false, gattCallback)
                    isScanning = false
                    bleScanner.stopScan(this)
                }
            }
        }
    }

    /*
       GATT: The gatt object is the final object we interact with once we are connected
    */
    private var gatt: BluetoothGatt? = null

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(incomingGatt: BluetoothGatt, status: Int, newState: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                // We are connected, need to setup the watches services
                if (newState == BluetoothProfile.STATE_CONNECTED) {
                    Log.i("BLE GATT STATE", "CONNECTED")
                    // Let anyone listening know we are continuing the connection process
                    connectionStatus = ConnectionState.Connected
                    currentConnectionAttempt = 1
                    coroutineScope.launch {
                        data.emit(Resource.Loading(message = "Discovering Services..."))
                    }
                    gatt = incomingGatt
                    incomingGatt.discoverServices()
                } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                    Log.i("BLE GATT STATE", "DISCONNECTED")
                    // We could have disconnected forcefully though disconnect(), closeConnection()
                    //      or we simply disconnected due to a timeout etc. We will know by the
                    //      current state of the connection status
                    when (disconnectRational) {
                        DisconnectRational.None -> {
                            // In theory this should never be hit as we are only here in the callback
                            //      if we were the ones that issued the disconnect?
                            // We were previously connected and want to stay connected, so start
                            //      a sticky reconnect and acknowledge the disconnection
                            connectionStatus = ConnectionState.Disconnected
                            issueStickyBluetoothConnection()
                            coroutineScope.launch {
                                data.emit(Resource.Success(data = SmartWatchResult(0f, ConnectionState.Disconnected)))
                            }
                        }
                        DisconnectRational.Close -> {
                            // We have intentionally disconnected and want to close the connection
                            incomingGatt.close()
                            connectionStatus = ConnectionState.Uninitialized
                            disconnectRational = DisconnectRational.None
                            gatt = null
                            coroutineScope.launch {
                                data.emit(Resource.Success(data = SmartWatchResult(0f, ConnectionState.Uninitialized)))
                            }
                        }
                        DisconnectRational.Disconnect -> {
                            // We have intentionally disconnected but want to keep the gatt open
                            connectionStatus = ConnectionState.Disconnected
                            disconnectRational = DisconnectRational.None
                            coroutineScope.launch {
                                data.emit(Resource.Success(data = SmartWatchResult(0f, ConnectionState.Disconnected)))
                            }
                        }
                    }
                }
            } else {
                Log.i("BLE GATT STATE", "ERROR")
                // Connection state changed/failed without us requesting it. Our action will depend
                //      on if we were currently connected or not
                if (connectionStatus == ConnectionState.Connected) {
                    // We were connected but somehow got disconnected, issue a sticky reconnect
                    connectionStatus = ConnectionState.Disconnected
                    issueStickyBluetoothConnection()
                    coroutineScope.launch {
                        data.emit(Resource.Success(data = SmartWatchResult(0f, ConnectionState.Disconnected)))
                    }
                } else {
                    // We were not connected os we either tried and failed to connect or we failed a reconnect
                    //      in either case just reset and if we have more attempts, start over
                    incomingGatt.close()
                    connectionStatus = ConnectionState.Uninitialized

                    // Decide whether we continue to try connecting
                    if (currentConnectionAttempt <= MAXIMUM_CONNECTION_ATTEMPTS){
                        // Let the system know we failed to connect but are still trying
                        currentConnectionAttempt += 1
                        coroutineScope.launch {
                            data.emit(
                                Resource.Loading(message = "Attempting to connect $currentConnectionAttempt/$MAXIMUM_CONNECTION_ATTEMPTS")
                            )
                        }
                        // Restart the connection process, indicate we are still scanning so our
                        //      connection attempts message doesn't get overridden
                        isScanning = true
                        startReceiving()
                    } else {
                        // We have exhausted our timeout, user will have to re-initiate
                        connectionStatus = ConnectionState.Uninitialized
                        isScanning = false
                        coroutineScope.launch {
                            data.emit(Resource.Error(errorMessage = "Could not connect to BLE device"))
                        }
                    }
                }
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            with (gatt) {
                // We have successfully read the available services, try to increase the MTU size
                //  We'd prefer a larger MTU size so we can send larger blocks of data at one time
                printGattTable()
                coroutineScope.launch {
                    data.emit(Resource.Loading(message = "Adjusting MTU space..."))
                }
                gatt.requestMtu(517)
            }
        }

        override fun onMtuChanged(gatt: BluetoothGatt, mtu: Int, status: Int) {
            // Once the MTU size is updated we are done with the connection process and can let
            //  the user know that we are officially connected
            Log.d("BluetoothGatt", "MTU was updated to $mtu")
            coroutineScope.launch {
                data.emit(Resource.Success(data = SmartWatchResult(0.0f, ConnectionState.Connected)))
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
                            // Log the battery voltage and let the user know
                            // Data comes in little endian
                            Log.i("BluetoothGattCallback", "Incoming battery voltage byte array: ${value.toHexString()}")
                            val buffer = ByteBuffer.wrap(value).order(ByteOrder.LITTLE_ENDIAN)
                            val batteryVoltage = buffer.float
                            coroutineScope.launch {
                                data.emit(Resource.Success(data = SmartWatchResult(batteryVoltage, ConnectionState.Connected)))
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
        geckoDevice?.connectGatt(context, true, gattCallback)
    }


    /*
       Public functions to interact with
    */
    // This is the entry point for starting the connection process, it is the only pre connected
    //      function that is called
    override fun startReceiving() {
        // Start scanning for BLE devices
        bleScanner.startScan(null, scanSettings, scanCallback)

        // Let anyone that is listening know that we are scanning
        // If we are already scanning and this is a re-attempt, don't override previous message
        connectionStatus = ConnectionState.CurrentlyInitializing
        if (!isScanning) {
            isScanning = true
            coroutineScope.launch {
                data.emit(Resource.Loading(message = "Scanning BLE devices..."))
            }
        }
    }

    override fun readBatteryVoltage(){
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

    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    override fun writeNotification(payload: ByteArray) {
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

    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    override fun updateWatchTime() {
        // This will only send the payload it will not formulate the notification
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

    // Quick getter to know if we are connected
    override fun getConnectionState(): ConnectionState? {
        return connectionStatus
    }

    // Attempt to reconnect to the watch after being disconnected
    override fun reconnect() {
        gatt?.connect()
    }

    // Force disconnect from the watch, this does not void the connection just disconnects
    override fun disconnect() {
        disconnectRational = DisconnectRational.Disconnect
        gatt?.disconnect()
    }

    // Force disconnect and completely close the connection, after calling you will need to
    //     start connection process over
    // This function is useful to stop the scanning process if the watch is not being found
    override fun closeConnection() {
        // Stop the scan if it was in progress
        bleScanner.stopScan(scanCallback)
        isScanning = false

        // If we are already disconnected, just de-init the gatt connection
        if (connectionStatus == ConnectionState.Disconnected) {
            connectionStatus = ConnectionState.Uninitialized
            disconnectRational = DisconnectRational.None
            gatt = null
            coroutineScope.launch {
                data.emit(Resource.Success(data = SmartWatchResult(0f, ConnectionState.Uninitialized)))
            }
        } else {
            // We do not call gatt.close() until the disconnect was successful
            disconnectRational = DisconnectRational.Close
            gatt?.disconnect()
        }
    }



}