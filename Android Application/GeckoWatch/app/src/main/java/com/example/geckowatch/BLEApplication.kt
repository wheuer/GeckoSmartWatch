package com.example.geckowatch

import android.Manifest
import android.app.Application
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.content.ComponentName
import android.content.Context
import android.os.CountDownTimer
import android.provider.Settings
import android.text.TextUtils
import android.util.Log
import androidx.core.content.ContextCompat
import androidx.core.content.PermissionChecker.PERMISSION_GRANTED
import com.example.geckowatch.data.SmartWatchReceiveManager
import com.example.geckowatch.data.ble.SmartWatchBLEReceiveManager
import com.example.geckowatch.presentation.permissions.PermissionUtils

class BLEApplication : Application() {

    private lateinit var bluetoothManager: BluetoothManager
    private lateinit var bluetoothAdapter: BluetoothAdapter
    private lateinit var smartWatchReceiveManager: SmartWatchReceiveManager

    private var bluetoothPermissionsState: Boolean = false
    private var notificationListenerPermissionState: Boolean = false
    private var bluetoothEnabledState: Boolean = false

    override fun onCreate() {
        super.onCreate()

        Log.i("APPLICATION", "onCreate")

        // Access the bluetooth adapter to eventually pass to our BLE manager
        // Bluetooth does not have to be enabled for us to access
        bluetoothManager = applicationContext.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothAdapter = bluetoothManager.adapter

        // Create our BLE manager
        smartWatchReceiveManager = SmartWatchBLEReceiveManager(bluetoothAdapter, applicationContext)

        // Initialize the state of our required permissions
        updatePermissionsState()
    }

    fun updatePermissionsState() {
        // Check for required bluetooth permissions
        if ((ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) == PERMISSION_GRANTED) &&
            (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) == PERMISSION_GRANTED)) {
            bluetoothPermissionsState = true
            Log.i("BLE PERM CHECK", "TRUE")
        } else {
            bluetoothPermissionsState = false
            Log.i("BLE PERM CHECK", "FALSE")
        }

        // Check for required notification listener permissions
        if (isNotificationServiceEnabled()){
            notificationListenerPermissionState = true
            Log.i("NOTIFICATION PERM CHECK", "TRUE")
        } else {
            notificationListenerPermissionState = false
            Log.i("NOTIFICATION PERM CHECK", "FALSE")
        }

        // Check if bluetooth is enabled
        bluetoothEnabledState = bluetoothAdapter.isEnabled
    }

    private fun isNotificationServiceEnabled() : Boolean {
        val packageName = packageName
        val flat = Settings.Secure.getString(contentResolver, "enabled_notification_listeners")
        if (!TextUtils.isEmpty(flat)){
            val names = flat.split(":".toRegex()).dropLastWhile { it.isEmpty() }.toTypedArray()
            for (i in names.indices) {
                val cn = ComponentName.unflattenFromString(names[i])
                if (cn != null) {
                    if (TextUtils.equals(packageName, cn.packageName)) {
                        return true
                    }
                }
            }
        }
        return false
    }

    fun getBluetoothAdapter(): BluetoothAdapter {
        return this.bluetoothAdapter
    }

    fun getGeckoBLEManager(): SmartWatchReceiveManager {
        return this.smartWatchReceiveManager
    }

    fun getBluetoothPermissionsState(): Boolean {
        return this.bluetoothPermissionsState
    }

    fun getNotificationPermissionsState(): Boolean {
        return this.notificationListenerPermissionState
    }

    fun getBluetoothEnabledStatus(): Boolean {
        return this.bluetoothEnabledState
    }

}
