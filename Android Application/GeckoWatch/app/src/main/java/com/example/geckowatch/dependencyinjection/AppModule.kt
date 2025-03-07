package com.example.geckowatch.dependencyinjection

import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.content.Context
import com.example.geckowatch.data.SmartWatchReceiveManager
import com.example.geckowatch.data.ble.SmartWatchBLEReceiveManager
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.android.qualifiers.ApplicationContext
import dagger.hilt.components.SingletonComponent
import javax.inject.Singleton

//@Module
//@InstallIn(SingletonComponent::class)
//object AppModule {
//
//    @Provides
//    @Singleton
//    fun provideBluetoothAdapter(@ApplicationContext context: Context):BluetoothAdapter{
//        val manager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
//        return manager.adapter
//    }
//
//    @Provides
//    @Singleton
//    fun provideSmartWatchReceiveManager(
//        @ApplicationContext context: Context,
//        bluetoothAdapter: BluetoothAdapter // This will be auto initialized for us as seen above
//    ):SmartWatchReceiveManager{
//        return SmartWatchBLEReceiveManager(bluetoothAdapter, context)
//    }
//
//}