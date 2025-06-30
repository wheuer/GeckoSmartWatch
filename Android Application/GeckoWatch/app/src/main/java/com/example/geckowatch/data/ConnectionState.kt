package com.example.geckowatch.data

sealed interface ConnectionState {
    data object Connected: ConnectionState              // Valid gatt and connected
    data object Disconnected: ConnectionState           // Valid gatt but disconnected
    data object Uninitialized: ConnectionState          // No valid gatt
    data object CurrentlyInitializing: ConnectionState  // Attempting to connect
    data object Error: ConnectionState                  // Unknown state/error
}

// Depending on how we disconnect we want to either close the gatt connection or keep it open
sealed interface DisconnectRational {
    data object Disconnect: DisconnectRational  // Prompted disconnect but keep gatt open
    data object Close: DisconnectRational       // Prompted disconnect with intent to close
    data object None: DisconnectRational        // Unprompted disconnect
}