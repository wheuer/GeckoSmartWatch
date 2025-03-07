package com.example.geckowatch

import android.content.Intent
import android.os.IBinder
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.util.Log
import android.widget.Toast
import com.example.geckowatch.data.ConnectionState
import com.example.geckowatch.data.SmartWatchReceiveManager
import dagger.hilt.android.AndroidEntryPoint
import java.util.TimeZone
import javax.inject.Inject

class WatchNotificationListenerService : NotificationListenerService() {

    private lateinit var smartWatchReceiveManager: SmartWatchReceiveManager

    override fun onNotificationPosted(sbn: StatusBarNotification?) {
        if (sbn != null) {
            Log.i("WatchNotificationListener",
                "New Notification, App Name: " +
                        "${packageManager.getApplicationLabel(packageManager.getApplicationInfo(sbn.packageName, 0))}")
        }

        if (smartWatchReceiveManager.getConnectionState() == ConnectionState.Connected) {
            // The watch expects the notification in the format,
            //      appName:title:bodyText:timestamp
            // Because it is using the ':' as a delimiter we have to scan through and replace them
            //      probably replace them with a ; for now, we could have a customer code to look
            //      for on the watch, but the difference between a ':' and a ';' on the small screen
            //      is pretty small
            if (sbn != null) {
                val notificationStringBuilder = StringBuilder()

                // App name
                notificationStringBuilder.append(
                    packageManager.getApplicationLabel(packageManager.getApplicationInfo(sbn.packageName, 0))
                        .toString().replace(":", ";"))
                notificationStringBuilder.append(":")

                // Notification Title
                notificationStringBuilder.append(
                    sbn.notification.extras.getString("android.title")
                        ?.replace(":", ";") ?: "[No Title]"
                )
                notificationStringBuilder.append(":")

                // Notification Text
                notificationStringBuilder.append(
                    sbn.notification.extras.getString("android.text")
                        ?.replace(":", ";") ?: "[No Text]"
                )
                notificationStringBuilder.append(":")

                // Notification Timestamp, accounting for time zone and daylight savings
                val timestamp = (sbn.postTime + TimeZone.getDefault().getOffset(sbn.postTime)) / 1000
                Log.i("NotificationSending", "Timestamp $timestamp")
                notificationStringBuilder.append(
                    timestamp.toString().replace(":", ";")
                )

                Log.i("WatchNotificationListener", "Sending Notification: $notificationStringBuilder")
                smartWatchReceiveManager.writeNotification(notificationStringBuilder.toString().toByteArray(Charsets.UTF_8))
            }
        }
    }

    override fun onNotificationRemoved(sbn: StatusBarNotification?) {
        Log.i("WatchNotificationListener", "Notification removed: ${sbn.toString()}")
    }

    override fun onListenerConnected() {
        super.onListenerConnected()
        Log.i("WatchNotificationListener", "Notification service onListenerConnected")
    }

    override fun onListenerDisconnected() {
        super.onListenerDisconnected()
        Log.i("WatchNotificationListener", "Notification service onListenerDisconnected")
    }

    override fun onBind(intent: Intent?): IBinder? {
        Log.i("WatchNotificationListener", "Notification service onBind")
        smartWatchReceiveManager = (applicationContext as BLEApplication).getGeckoBLEManager()
        return super.onBind(intent)
    }
}


