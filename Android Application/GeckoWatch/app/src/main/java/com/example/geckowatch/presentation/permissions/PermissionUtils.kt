package com.example.geckowatch.presentation.permissions

import android.Manifest
import android.os.Build

object PermissionUtils {
    val bluetoothPermissions =
        listOf(
            Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT
        )
}

