package com.example.geckowatch.data

data class SmartWatchResult(
    val batteryVoltage: Float,
    val connectionState: ConnectionState,
    val message: String
)
