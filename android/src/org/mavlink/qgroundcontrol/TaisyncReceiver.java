package org.mavlink.qgroundcontrol;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class TaisyncReceiver extends BroadcastReceiver {
    // This declares the C++ function we will link to
    public native void nativeOnReceive(String json);

    @Override
    public void onReceive(Context context, Intent intent) {
        String json = intent.getStringExtra("info");
        if (json != null) {
            nativeOnReceive(json);
        }
    }
}