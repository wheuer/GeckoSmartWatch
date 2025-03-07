package com.example.geckowatch

import android.Manifest
import android.app.Activity
import android.bluetooth.BluetoothAdapter
import android.content.ComponentName
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Bundle
import android.provider.Settings
import android.text.TextUtils
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.core.content.PermissionChecker.PERMISSION_GRANTED
import com.example.geckowatch.data.SmartWatchReceiveManager
import com.example.geckowatch.presentation.Navigation
import com.example.geckowatch.presentation.SmartWatchViewModel
import com.example.geckowatch.ui.theme.GeckoWatchTheme

class MainActivity : ComponentActivity() {

    private lateinit var bluetoothAdapter: BluetoothAdapter
    private lateinit var smartWatchReceiveManager: SmartWatchReceiveManager

    private lateinit var appContext: BLEApplication

    private val watchViewModel by viewModels<SmartWatchViewModel>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        Log.i("MAIN_ACTIVITY", "onCreate")

        // Get reference to application context
        appContext = (applicationContext as BLEApplication)

        // Grab the bluetooth adapter
        bluetoothAdapter = appContext.getBluetoothAdapter()

        // Grab our BLE manager
        smartWatchReceiveManager = appContext.getGeckoBLEManager()

        // Force pass our BLE manger to our view model don't @ me dEpEnDeNcY iNjEcTiOn Is BeTtEr
        watchViewModel.setBLEManger(smartWatchReceiveManager)

        setContent {
            GeckoWatchTheme {
                Navigation(
                    // Pass view model with access to all relevant functions and state variables
                    getActivityContext = {
                        getMainActivityContext()
                    },
                    viewModel = watchViewModel
                )
            }
        }
    }

    override fun onStart() {
        super.onStart()
        Log.i("MAIN_ACTIVITY", "onStart")
        updateViewModelPermissions()
    }

    override fun onResume() {
        super.onResume()
        Log.i("MAIN_ACTIVITY", "onResume")
        // Activity will resume once user is back from the notification listener settings
        // We need to update the permissions in both the application and StartScreen
        updateViewModelPermissions()
    }

    private fun updateViewModelPermissions() {
        appContext.updatePermissionsState()
        watchViewModel.updatePermissionsState(appContext.getBluetoothPermissionsState(),
            appContext.getNotificationPermissionsState(),
            appContext.getBluetoothEnabledStatus())
    }

    fun navigateToNotificationPermissions() {
        val intent = Intent("android.settings.ACTION_NOTIFICATION_LISTENER_SETTINGS")
        startActivity(intent)
    }

    private fun getMainActivityContext(): MainActivity {
        return this
    }

    private var isBluetoothDialogAlreadyShown = false // Prevent multiple popups
    private fun showBluetoothDialog(){
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) == PERMISSION_GRANTED){
            if (!bluetoothAdapter.isEnabled){
                if (!isBluetoothDialogAlreadyShown) {
                    val enableBluetoothIntent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
                    startBluetoothIntentForResult.launch(enableBluetoothIntent)
                    isBluetoothDialogAlreadyShown = true
                }
            }
        }
    }

    // This function requests te basic bluetooth permissions
    private fun requestBluetoothPermissions(){
        when {
            ContextCompat.checkSelfPermission(
                this,
                Manifest.permission.BLUETOOTH_CONNECT
            ) == PackageManager.PERMISSION_GRANTED -> {
                // Permission is granted
            }
            ActivityCompat.shouldShowRequestPermissionRationale(
                this,
                Manifest.permission.BLUETOOTH_CONNECT
            ) -> {
                // Just request permission again
                requestPermissionLauncher.launch(Manifest.permission.BLUETOOTH_CONNECT)
            } else -> {
            // Permission has not been asked
            requestPermissionLauncher.launch(Manifest.permission.BLUETOOTH_CONNECT)
        }
        }
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

    private val startBluetoothIntentForResult =
        registerForActivityResult(ActivityResultContracts.StartActivityForResult()){ result ->
            isBluetoothDialogAlreadyShown = false
            if (result.resultCode != Activity.RESULT_OK){
                showBluetoothDialog()
            }
        }

    private val requestPermissionLauncher =
        registerForActivityResult(ActivityResultContracts.RequestPermission()){ isGranted: Boolean ->
            if (isGranted){
                Log.i("Permission: ", "Granted")
                // Make sure bluetooth is enabled, we can ask now since we have bluetooth permissions
                showBluetoothDialog()
            } else {
                Log.i("Permission: ", "Denied")
            }
        }

    fun onBluetoothChange() {
        updateViewModelPermissions()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
        deviceId: Int
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults, deviceId)
        updateViewModelPermissions()
    }
}