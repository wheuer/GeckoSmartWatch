package com.example.geckowatch.data

import com.example.geckowatch.util.Resource
import kotlinx.coroutines.flow.MutableSharedFlow

interface SmartWatchReceiveManager {

    val data: MutableSharedFlow<Resource<SmartWatchResult>>

    fun readBatteryVoltage()

    fun writeNotification(payload: ByteArray)

    fun updateWatchTime()

    fun reconnect()

    fun disconnect()

    fun startReceiving()

    fun closeConnection()

    fun getConnectionState(): ConnectionState?

}