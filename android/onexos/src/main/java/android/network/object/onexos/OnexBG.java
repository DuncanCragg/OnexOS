// THANKS to https://fabcirablog.weebly.com/blog/creating-a-never-ending-background-service-in-android-gt-7

package network.object.onexos;

import java.io.ByteArrayOutputStream;

import android.os.*;
import android.view.inputmethod.*;
import android.view.ViewGroup.LayoutParams;
import android.widget.*;
import android.text.*;
import android.app.NativeActivity;
import android.hardware.usb.*;
import android.content.*;
import android.util.Log;
import android.app.*;

import com.felhr.usbserial.UsbSerialInterface;
import com.felhr.usbserial.UsbSerialDevice;

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

    private static UsbSerialDevice serialPort = null;

    static public void onUSBAttached(Intent intent){

      UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE, UsbDevice.class);
      UsbManager usbManager = self.getSystemService(UsbManager.class);
      final UsbDeviceConnection connection = usbManager.openDevice(device);
      UsbInterface interf = device.getInterface(0);
      connection.claimInterface(interf, true);
      serialPort = UsbSerialDevice.createUsbSerialDevice(device, connection);

      if(serialPort == null) { Log.e(LOGNAME, "No serial port!"); return; }
      if(!serialPort.open()) { Log.e(LOGNAME, "Could not open serial port!"); return; }

      serialPort.setBaudRate(9600);
      serialPort.setDataBits(UsbSerialInterface.DATA_BITS_8);
      serialPort.setStopBits(UsbSerialInterface.STOP_BITS_1);
      serialPort.setParity(UsbSerialInterface.PARITY_NONE);
      serialPort.setFlowControl(UsbSerialInterface.FLOW_CONTROL_OFF);
      serialPort.read(recvCB);

      asyncConnected();
    }

    static public UsbSerialInterface.UsbReadCallback recvCB = new UsbSerialInterface.UsbReadCallback() {
        @Override
        public void onReceivedData(byte[] data) { dataRecv(data); }
    };

    static public native void serialOnRecv(String b);

    static public void asyncConnected(){
        new Thread(){ public void run(){ serialOnRecv(null); }}.start();
    }

    static private ByteArrayOutputStream recvBuff = new ByteArrayOutputStream();

    static private void dataRecv(byte[] data) {
      try{
        recvBuff.write(data);
        String chars = recvBuff.toString("UTF-8");
        int x = chars.lastIndexOf('\n');
        if(x == -1) return;
        String newChars = chars.substring(0,x+1);
        recvBuff.reset();
        recvBuff.write(chars.substring(x+1).getBytes());
        if(logReadWrite) Log.d(LOGNAME, "read (" + newChars + ")" );
        serialOnRecv(newChars);
      }catch(Exception e){}
    }

    static public void serialSend(String chars){
      if(logReadWrite) Log.d(LOGNAME, "write (" + chars + ")");
      try {
        if(self.serialPort!=null){
          self.serialPort.write(chars.getBytes("UTF-8"));
        }
      }catch(Exception e){
        e.printStackTrace();
      }
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
