package uz.abdukamol.turbojang;

import android.app.NativeActivity;
import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;

/**
 * O'yinning o'zi to'liq C tilida (libturbojang.so). Bu kichik sinf faqat
 * Android'ga xos ikki ishni bajaradi:
 *  1) MulticastLock - aks holda ko'p telefonlar Wi-Fi/hotspotdagi broadcast
 *     paketlarni filtrlab tashlaydi va host avtomatik topilmaydi.
 *  2) Ekranni o'chirmaslik va tizim panellarini yashirish.
 */
public class GameActivity extends NativeActivity {
    private WifiManager.MulticastLock multicastLock;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        try {
            WifiManager wifi = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
            if (wifi != null) {
                multicastLock = wifi.createMulticastLock("turbojang");
                multicastLock.setReferenceCounted(false);
                multicastLock.acquire();
            }
        } catch (Exception ignored) {
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            getWindow().getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                    | View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
        }
    }

    @Override
    protected void onDestroy() {
        if (multicastLock != null && multicastLock.isHeld()) multicastLock.release();
        super.onDestroy();
    }
}
