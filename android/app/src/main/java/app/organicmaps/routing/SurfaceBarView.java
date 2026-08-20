package app.organicmaps.routing;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.View;
import androidx.annotation.ColorRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.core.content.ContextCompat;
import app.organicmaps.R;

/**
 * Horizontal stacked bar visualizing the per-surface distance breakdown of a
 * route. Segments are drawn in the order paved, gravel, dirt, singletrack and
 * unknown, normalized to the route length (all buckets, including unknown),
 * and clipped to the rounded bar outline. Pass the raw per-surface distances
 * via {@link #setSurfaceMeters(float[])}.
 */
public class SurfaceBarView extends View
{
  /** Resource ids in the same order as the surface buckets. */
  @ColorRes
  public static final int[] SURFACE_COLOR_RES = {
      R.color.surface_paved,
      R.color.surface_gravel,
      R.color.surface_dirt,
      R.color.surface_singletrack,
      R.color.surface_unknown
  };

  @StringRes
  public static final int[] SURFACE_NAME_RES = {
      R.string.surface_paved,
      R.string.surface_gravel,
      R.string.surface_dirt,
      R.string.surface_singletrack,
      R.string.surface_unknown
  };

  @NonNull
  private final Paint mSegmentPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  @NonNull
  private final Path mClipPath = new Path();
  @NonNull
  private final RectF mBarRect = new RectF();
  private float[] mWeights = new float[SURFACE_COLOR_RES.length];

  public SurfaceBarView(Context context)
  {
    this(context, null);
  }

  public SurfaceBarView(Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
  }

  /**
   * @param meters per-surface distances in the same order as
   *               {@link #SURFACE_COLOR_RES}; normalized to their sum.
   */
  public void setSurfaceMeters(@NonNull float[] meters)
  {
    float total = 0.0f;
    for (float m : meters)
      total += m;
    mWeights = new float[SURFACE_COLOR_RES.length];
    if (total > 0.0f)
    {
      for (int i = 0; i < SURFACE_COLOR_RES.length && i < meters.length; ++i)
        mWeights[i] = meters[i] / total;
    }
    invalidate();
  }

  @Override
  protected void onDraw(@NonNull Canvas canvas)
  {
    super.onDraw(canvas);

    final float width = getWidth();
    final float height = getHeight();
    if (width <= 0.0f || height <= 0.0f)
      return;

    final float radius = height / 2.0f;
    mBarRect.set(0.0f, 0.0f, width, height);
    mClipPath.reset();
    mClipPath.addRoundRect(mBarRect, radius, radius, Path.Direction.CW);

    canvas.save();
    canvas.clipPath(mClipPath);
    float x = 0.0f;
    for (int i = 0; i < mWeights.length; ++i)
    {
      final float segWidth = width * mWeights[i];
      if (segWidth <= 0.0f)
        continue;
      mSegmentPaint.setColor(ContextCompat.getColor(getContext(), SURFACE_COLOR_RES[i]));
      canvas.drawRect(x, 0.0f, x + segWidth, height, mSegmentPaint);
      x += segWidth;
    }
    canvas.restore();
  }
}