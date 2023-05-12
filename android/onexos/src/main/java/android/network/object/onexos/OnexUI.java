
package network.object.onexos;

import java.util.List;

import android.Manifest;
import android.os.*;
import android.net.Uri;
import android.view.ViewGroup.LayoutParams;
import android.widget.*;
import android.text.*;
import android.content.*;
import android.content.pm.*;
import android.provider.Settings;
import android.app.NativeActivity;
import android.util.Log;
import android.app.*;

import androidx.core.content.ContextCompat;
import androidx.core.app.ActivityCompat;

/**  NativeActivity wrapper offers Java API functions not available in JNI land.
  */
public class OnexUI extends NativeActivity {

    static OnexUI self=null;

    public static final String LOGNAME = "Onex OnexUI";

    private static final int REQUEST_FILE_RW = 5544;

    @Override
    public void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState); Log.d(LOGNAME, "onCreate");
        self=this;
    }

    @Override
    public void onNewIntent(Intent intent){
      super.onNewIntent(intent);
      Log.d(LOGNAME, "onNewIntent("+intent.getAction()+")");
      if(!"android.hardware.usb.action.USB_DEVICE_ATTACHED".equalsIgnoreCase(intent.getAction())) return;
      OnexBG.onUSBAttached(intent);
    }

    @Override
    public void onRestart(){
        super.onRestart(); Log.d(LOGNAME, "onRestart");
    }

    @Override
    public void onStart(){
        super.onStart(); Log.d(LOGNAME, "onStart");
    }

    static private int fileRWPerms = 0; // 0 waiting, 1 approved, -1 denied

    private void checkAndRequestFileRWPermission() {

        if(Build.VERSION.SDK_INT >= 30) {

            if(Environment.isExternalStorageManager()){
                Log.d(LOGNAME, "API 30+ has file RW permission");
                fileRWPerms=1;
                return;
            }
            try {
                Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                intent.addCategory("android.intent.category.DEFAULT");
                intent.setData(Uri.parse(String.format("package:%s",getApplicationContext().getPackageName())));
                startActivityForResult(intent, REQUEST_FILE_RW);

            } catch (Exception e) {
                Intent intent = new Intent();
                intent.setAction(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                startActivityForResult(intent, REQUEST_FILE_RW);
            }

        } else {

            int readext = ContextCompat.checkSelfPermission(OnexUI.this, Manifest.permission.READ_EXTERNAL_STORAGE);
            int writext = ContextCompat.checkSelfPermission(OnexUI.this, Manifest.permission.WRITE_EXTERNAL_STORAGE);

            if(readext==PackageManager.PERMISSION_GRANTED &&
               writext==PackageManager.PERMISSION_GRANTED    ){

                Log.d(LOGNAME, "API 29 has file RW permission");
                fileRWPerms=1;
                return;
            }
            ActivityCompat.requestPermissions(OnexUI.this, new String[]{ Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE }, REQUEST_FILE_RW);
        }
    }

    @Override
    public void onResume(){
        super.onResume(); Log.d(LOGNAME, "onResume");

        // relies on an onResume from the finishing of the file RW permissions dialogues

        if(fileRWPerms==0) checkAndRequestFileRWPermission();

        restartOnexBG();
    }

    @Override
    public void onPause(){
        super.onPause(); Log.d(LOGNAME, "onPause");
    }

    @Override
    public void onStop(){
        super.onStop(); Log.d(LOGNAME, "onStop");
    }

    @Override
    public void onDestroy(){
        super.onDestroy(); Log.d(LOGNAME, "onDestroy");
        self=null;
    }

    static public void restartOnexBG(){
        if(self==null){
            Log.d(LOGNAME, "calling restartOnexBG without a running activity");
            return;
        }
        if(fileRWPerms!=1){
            Log.d(LOGNAME, "calling restartOnexBG without file RW permissions");
            return;
        }
        Intent intent = new Intent("network.object.onexos.bg.restart").setFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
        PackageManager packageManager = self.getPackageManager();
        List<ResolveInfo> broadcastReceivers;
        if(Build.VERSION.SDK_INT >= 33) {
            broadcastReceivers = packageManager.queryBroadcastReceivers(intent, PackageManager.ResolveInfoFlags.of(0));
        } else {
            @SuppressWarnings("deprecation")
            List<ResolveInfo> wtf = packageManager.queryBroadcastReceivers(intent, 0);
            broadcastReceivers = wtf;
        }
        for(ResolveInfo broadcastReceiver: broadcastReceivers) {
            ComponentName cn = new ComponentName(broadcastReceiver.activityInfo.packageName, broadcastReceiver.activityInfo.name);
            Log.d(LOGNAME, "restartOnexBG on "+cn);
            intent.setComponent(cn);
            self.sendBroadcast(intent);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        if(requestCode==REQUEST_FILE_RW){
            fileRWPerms= -1;
            if (grantResults.length >= 1) {

                int readext = grantResults[0];
                int writext = grantResults[1];

                if(readext==PackageManager.PERMISSION_GRANTED &&
                   writext==PackageManager.PERMISSION_GRANTED    ){

                    Log.d(LOGNAME, "API 29 file RW permission granted");
                    fileRWPerms=1;
                }
            }
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
      super.onActivityResult(requestCode, resultCode, data);
      Log.d(LOGNAME, "onActivityResult()");

      if(requestCode==REQUEST_FILE_RW) {
          fileRWPerms= -1;
          if(Build.VERSION.SDK_INT >= 30) {
              if(Environment.isExternalStorageManager()) {

                  Log.d(LOGNAME, "API 30+ file RW permission granted");
                  fileRWPerms=1;
              }
          }
          return;
      }
    }

    // -----------------------------------------------------------

    public void setAlarm(long when, String uid){
      Intent intent = new Intent("Onex.Alarm");
      intent.putExtra("UID", uid);
      PendingIntent pendingIntent = PendingIntent.getBroadcast(this, uid.hashCode(), intent, 0);
      AlarmManager am=(AlarmManager)getSystemService(Context.ALARM_SERVICE);
      if(when!=0) am.set(AlarmManager.RTC_WAKEUP, when*1000, pendingIntent);
      else        am.cancel(pendingIntent);
    }

    public static native void onAlarmRecv(String uid);

    static public void alarmReceived(Context context, Intent intent){
      String uid=intent.getStringExtra("UID");
      if(self!=null) self.onAlarmRecv(uid);
      else context.startActivity(new Intent(context, OnexUI.class));
    }

    public void showNotification(String title, String text){
      Log.d(LOGNAME, "showNotification!!");
      PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, new Intent(this, OnexUI.class), PendingIntent.FLAG_UPDATE_CURRENT);
      Notification.Builder notifbuilder = new Notification.Builder(this, "Notification-Channel-ID");
      notifbuilder.setContentTitle(title)
                  .setContentText(text)
                  .setSmallIcon(R.drawable.icon)
                  .setContentIntent(pendingIntent);
      NotificationManager notifMgr = (NotificationManager)this.getSystemService(this.NOTIFICATION_SERVICE);
      notifMgr.notify(12345, notifbuilder.build());
    }

    private void showMessage(String msg) {
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show();
    }

    // -----------------------------------------------------------
}

