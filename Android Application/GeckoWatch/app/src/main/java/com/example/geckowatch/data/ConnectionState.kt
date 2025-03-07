package com.example.geckowatch.data

sealed interface ConnectionState {
    data object Connected: ConnectionState
    data object Disconnected: ConnectionState
    data object Uninitialized: ConnectionState
    data object CurrentlyInitializing: ConnectionState
    data object Error: ConnectionState
}

sealed interface DisconnectRational {
    data object Disconnect: DisconnectRational
    data object Close: DisconnectRational
    data object None: DisconnectRational
}