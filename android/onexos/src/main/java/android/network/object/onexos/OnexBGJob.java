// THANKS to https://fabcirablog.weebly.com/blog/creating-a-never-ending-background-service-in-android-gt-7

package network.object.onexos;

import android.app.job.JobParameters;
import android.content.*;
import android.util.Log;

public class OnexBGJob extends android.app.job.JobService {
    private static String LOGNAME="Onex OnexBGJob";

    @Override
    public boolean onStartJob(JobParameters jobParameters) {
        Log.d(LOGNAME, "*onStartJob");

        startForegroundService(new Intent(this, OnexBG.class));
        return false;
    }

    @Override
    public boolean onStopJob(JobParameters jobParameters) {
        Log.d(LOGNAME, "*onStopJob");

        OnexUI.restartOnexBG();

        return false;
    }
}
