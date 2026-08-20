package app.organicmaps.sdk.brouter;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.RemoteException;
import android.os.SystemClock;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Arrays;

import btools.routingapp.IBRouterService;

/**
 * Synchronous high-level entry point for the BRouter companion app.
 * <p>
 * Uses {@link BRouterServiceConnection} to bind to {@code btools.routingapp.BRouterService},
 * invokes {@link IBRouterService#getTrackFromParams} on the calling worker thread,
 * and returns the resulting GPX payloads (or {@code null} on failure).
 * <p>
 * The native {@code BRouterRouter} calls {@link #calculateRoutesBlocking} on a
 * worker thread managed by {@code AsyncRouter}. The call is synchronous and
 * performs exactly one bind/unbind cycle, regardless of how many alternatives
 * are requested.
 * <p>
 * Manifest must declare
 * <pre>{@code <queries><package android:name="btools.routingapp"/></queries>}</pre>
 * for Android 11+ package visibility.
 */
public final class BRouterServiceClient
{
  private static final String TAG = "BRouterServiceClient";
  private static final long BIND_TIMEOUT_MS = 4000;
  private static final long PROBE_CACHE_TTL_MS = 30_000;

  /** Package name of the BRouter companion app. */
  public static final String BROUTER_PACKAGE = BRouterServiceConnection.BROUTER_PACKAGE;
  /** Main activity of the companion app: hosts the profile selector and downloader. */
  private static final String BROUTER_MAIN_ACTIVITY = "btools.routingapp.BRouterActivity";

  private static volatile long sProbeTime;
  private static volatile boolean sProbeResult;

  private BRouterServiceClient() {}

  /**
   * Probe whether the BRouter companion app is installed by attempting to bind
   * to its routing service (the same probe OsmAnd uses). Safe on the UI thread:
   * {@link Context#bindService} resolves the intent synchronously and returns
   * immediately; the probe performs a single bind/unbind cycle.
   * <p>
   * The result is cached for a short time so that repeated checks (e.g. on
   * every route rebuild) do not bind/unbind the companion service each time.
   */
  public static boolean isBRouterInstalled(@NonNull Context ctx)
  {
    final long now = SystemClock.elapsedRealtime();
    if (now - sProbeTime < PROBE_CACHE_TTL_MS)
      return sProbeResult;

    BRouterServiceConnection conn = BRouterServiceConnection.connect(ctx);
    sProbeTime = now;
    sProbeResult = conn != null;
    if (conn != null)
      conn.disconnect(ctx);
    return sProbeResult;
  }

  /**
   * Launches the BRouter app's profile management screen (its main activity,
   * which hosts the profile selector and the profile downloader). Returns
   * false when the app cannot be launched.
   */
  public static boolean openProfileManager(@NonNull Context context)
  {
    final Intent intent = new Intent(Intent.ACTION_MAIN)
        .addCategory(Intent.CATEGORY_LAUNCHER)
        .setClassName(BROUTER_PACKAGE, BROUTER_MAIN_ACTIVITY)
        .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    if (intent.resolveActivity(context.getPackageManager()) == null)
      return false;
    context.startActivity(intent);
    return true;
  }

  /**
   * Synchronously call the BRouter service and return all requested tracks.
   * Must be called off the UI thread. Performs a single bind/unbind cycle.
   * <p>
   * Each alternative is fetched once as a mode 9 GPX document, which carries
   * both the per-point way tags the surface analysis needs and, thanks to the
   * injected {@code profile:showspeed} profile variable, a per-point
   * {@code <brouter:speed>} giving exact per-alternative route times (the
   * {@code <brouter:info>} metadata total is shared by all alternatives and
   * thus unusable for per-route times).
   *
   * @param ctx       application context used for binding the service
   * @param lats      latitudes of the waypoints (length ≥ 2)
   * @param lons      longitudes of the waypoints (length ≥ 2)
   * @param count     number of alternatives to fetch (BRouter alternative index 0..count-1)
   * @return GPX content per alternative (index 0 = primary, in order), or {@code null}
   *         if the service is unavailable or no alternative could be fetched.
   */
  @Nullable
  public static String[] calculateRoutesBlocking(@NonNull Context ctx, @NonNull double[] lats,
                                                 @NonNull double[] lons, int count)
  {
    if (lats.length < 2 || lats.length != lons.length)
    {
      Log.w(TAG, "calculateRoutesBlocking: invalid waypoint arrays");
      return null;
    }

    BRouterServiceConnection conn = BRouterServiceConnection.connect(ctx);
    if (conn == null)
      return null;
    try
    {
      IBRouterService service = conn.getService(BIND_TIMEOUT_MS);
      if (service == null)
      {
        Log.w(TAG, "BRouter service not available");
        return null;
      }

      final String[] results = new String[count];
      int n = 0;
      for (int idx = 0; idx < count; ++idx)
      {
        final Bundle params = new Bundle();
        params.putDoubleArray("lats", lats);
        params.putDoubleArray("lons", lons);
        // BRouter is a bicycle-first engine; pass the bicycle vehicle + fast
        // flag so the companion app picks the right profile via its
        // serviceconfig.dat. The "profile" parameter is deliberately NOT set:
        // its value depends on the exact profile file installed in the
        // companion app.
        params.putString("v", "bicycle");
        params.putString("fast", "1");
        // Ask BRouter for "BRouter style" mode 9 output: the only instruction
        // format that includes per-point way tags (<brouter:way> with
        // highway/surface/tracktype key=value pairs), which the surface
        // analysis relies on. It also writes a <brouter:speed> per point
        // (derived from the exact per-node route times) once the showspeed
        // profile variable is set.
        params.putString("turnInstructionMode", "9");
        params.putString("acceptCompressedResult", "true");
        params.putString("trackFormat", "gpx");
        params.putString("alternativeidx", Integer.toString(idx));
        // Inject the showspeed profile variable via the "profile:" request
        // prefix: RoutingParamCollector turns it into an assignment expression
        // in the profile context, which makes mode 9 emit <brouter:speed> per
        // point. That gives the app exact per-alternative route times (the
        // <brouter:info> metadata total is shared by all alternatives and thus
        // not per-route). A plain "showspeed=1" parameter is ignored: BRouter
        // treats it as a profile expression variable, not a request option.
        params.putString("profile:showspeed", "1");
        try
        {
          results[n++] = service.getTrackFromParams(params);
        }
        catch (RemoteException e)
        {
          Log.w(TAG, "getTrackFromParams(" + idx + ") failed: " + e.getMessage());
          break;  // BRouter returns alternatives in order; a failure ends the batch
        }
      }
      if (n == 0)
        return null;
      return Arrays.copyOf(results, n);
    }
    finally
    {
      conn.disconnect(ctx);
    }
  }
}