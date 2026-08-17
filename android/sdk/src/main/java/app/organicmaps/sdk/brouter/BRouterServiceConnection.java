package app.organicmaps.sdk.brouter;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

import btools.routingapp.IBRouterService;

/**
 * Wraps the cross-process {@link ServiceConnection} to {@code btools.routingapp.BRouterService}.
 * <p>
 * Mirrors the public shape of the OsmAnd client (see
 * {@code btools/routingapp/BRouterServiceConnection.java}) but exposes a synchronous
 * {@link #getService(long)} for callers that run on a worker thread and need an
 * already-bound {@link IBRouterService} (the BRouter service runs in a separate
 * process, so {@link Context#bindService} is asynchronous by default).
 */
public final class BRouterServiceConnection implements ServiceConnection
{
  private static final String TAG = "BRouterServiceConnection";
  static final String BROUTER_PACKAGE = "btools.routingapp";
  private static final String SERVICE_CLASS = "btools.routingapp.BRouterService";

  private final AtomicReference<IBRouterService> mService = new AtomicReference<>();
  private final CountDownLatch mBindLatch = new CountDownLatch(1);

  @Override
  public void onServiceConnected(ComponentName name, IBinder binder)
  {
    mService.set(IBRouterService.Stub.asInterface(binder));
    mBindLatch.countDown();
  }

  @Override
  public void onServiceDisconnected(ComponentName name)
  {
    mService.set(null);
  }

  @Override
  public void onBindingDied(ComponentName name)
  {
    mBindLatch.countDown();
  }

  @Override
  public void onNullBinding(ComponentName name)
  {
    mBindLatch.countDown();
  }

  /**
   * Block the caller until the service is bound or the timeout elapses.
   * @return the bound service, or {@code null} on failure (timeout, no binding, exception).
   */
  @Nullable
  public IBRouterService getService(long timeoutMs)
  {
    try
    {
      if (!mBindLatch.await(timeoutMs, TimeUnit.MILLISECONDS))
      {
        Log.w(TAG, "Timed out waiting for BRouter service binding");
        return null;
      }
    }
    catch (InterruptedException e)
    {
      Thread.currentThread().interrupt();
      return null;
    }
    return mService.get();
  }

  public void disconnect(@NonNull Context ctx)
  {
    try
    {
      ctx.unbindService(this);
    }
    catch (IllegalArgumentException ignored)
    {
      // Already unbound.
    }
    mService.set(null);
  }

  /**
   * Bind to the BRouter service. Returns {@code null} if the BRouter app is not
   * installed or not visible to the caller (e.g. missing {@code <queries>}).
   */
  @Nullable
  public static BRouterServiceConnection connect(@NonNull Context ctx)
  {
    BRouterServiceConnection conn = new BRouterServiceConnection();
    Intent intent = new Intent().setClassName(BROUTER_PACKAGE, SERVICE_CLASS);
    boolean bound;
    try
    {
      bound = ctx.bindService(intent, conn, Context.BIND_AUTO_CREATE);
    }
    catch (SecurityException e)
    {
      Log.w(TAG, "bindService denied: " + e.getMessage());
      return null;
    }
    if (!bound)
    {
      Log.w(TAG, "bindService returned false (BRouter app not installed or not visible)");
      return null;
    }
    return conn;
  }
}
