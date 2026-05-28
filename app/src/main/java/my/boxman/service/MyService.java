package my.boxman.service;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;

import my.boxman.BoxMan;
import my.boxman.R;

public class MyService extends Service {
	private static final String CHANNEL_ID = "boxman_service";

	@Override
	public IBinder onBind(Intent arg0) {
		// TODO Auto-generated method stub
		return null;
	}

  	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		init( intent,startId);
		return  START_NOT_STICKY;
	}

	private void init(Intent intent,int startId)
	{
		Log.e("service","startService");
		Notification notification = createNotification();
		startForeground(101, notification);
	}

	private Notification createNotification() {
		Intent launchIntent = new Intent(this, BoxMan.class);
		PendingIntent pendingIntent = PendingIntent.getActivity(
				this,
				0,
				launchIntent,
				Build.VERSION.SDK_INT >= Build.VERSION_CODES.M ? PendingIntent.FLAG_IMMUTABLE : 0);

		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
			NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
			NotificationChannel channel = new NotificationChannel(
					CHANNEL_ID,
					getString(R.string.app_name),
					NotificationManager.IMPORTANCE_LOW);
			if (manager != null) manager.createNotificationChannel(channel);

			return new Notification.Builder(this, CHANNEL_ID)
					.setSmallIcon(R.drawable.icon)
					.setContentTitle(getString(R.string.app_name))
					.setContentText("运行中")
					.setContentIntent(pendingIntent)
					.setOngoing(true)
					.build();
		}

		return new Notification.Builder(this)
				.setSmallIcon(R.drawable.icon)
				.setContentTitle(getString(R.string.app_name))
				.setContentText("运行中")
				.setContentIntent(pendingIntent)
				.setOngoing(true)
				.build();
	}

	@Override
	public boolean onUnbind(Intent intent) {
		return super.onUnbind(intent);
	}


	@Override
	public void onDestroy() {
		super.onDestroy();
		stopForeground(true);
	}
}
