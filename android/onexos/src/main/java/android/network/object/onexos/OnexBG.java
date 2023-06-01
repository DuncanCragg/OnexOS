// THANKS to https://fabcirablog.weebly.com/blog/creating-a-never-ending-background-service-in-android-gt-7

package network.object.onexos;

import java.io.ByteArrayOutputStream;

import android.os.*;
import android.view.inputmethod.*;
import android.view.ViewGroup.LayoutParams;
import android.widget.*;
import android.text.*;
import android.app.NativeActivity;
import android.content.*;
import android.util.Log;
import android.app.*;

public class OnexBG extends Service {

    private static String LOGNAME = "Onex OnexBG";
    public static final boolean logReadWrite = true;

    static private OnexBG self=null;

    static public void delay(int ms){ try{ Thread.sleep(ms); }catch(Exception e){}; }

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(LOGNAME, "*onBind");
        return null;
    }

    static private boolean initialised=false;

    static public native void initOnex();
    static public native void loopOnex();

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(LOGNAME, "*onCreate");

        String CHANNEL_ID = "onex-activity";

        NotificationChannel channel = new NotificationChannel(CHANNEL_ID, "Onex Activity", NotificationManager.IMPORTANCE_DEFAULT);
        ((NotificationManager)getSystemService(Context.NOTIFICATION_SERVICE)).createNotificationChannel(channel);

        Intent intent = new Intent(this, OnexUI.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, PendingIntent.FLAG_IMMUTABLE);

        Notification.Builder notifbuilder = new Notification.Builder(this, CHANNEL_ID);
        notifbuilder.setSmallIcon(R.drawable.icon)
                    .setContentTitle("Onex is running")
                    .setContentText("(long press for notif settings)")
                    .setContentIntent(pendingIntent)
                    .setAutoCancel(true);
        startForeground(54321, notifbuilder.build());
    }

    // onStartCommand may be called many times
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags, startId);
        Log.d(LOGNAME, "*onStartCommand");

        self=this;

        if(!initialised){

          System.loadLibrary("native-lib");

          new Thread(){
             @Override
             public void run(){

               Log.d(LOGNAME, "============== calling initOnex()");
               initOnex();

               Log.d(LOGNAME, "============== calling loopOnex()");
               loopOnex();
             }
          }.start();
          initialised=true;
        }
        return START_STICKY;
    }

    // -----------------------------------------------------------

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(LOGNAME, "*onDestroy");

        OnexUI.restartOnexBG();
    }

    @Override
    public void onTaskRemoved(Intent rootIntent) {
        super.onTaskRemoved(rootIntent);
        Log.d(LOGNAME, "*onTaskRemoved");

        OnexUI.restartOnexBG();
    }
}
