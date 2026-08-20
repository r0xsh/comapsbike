package app.organicmaps.routing;

import static app.organicmaps.sdk.util.Utils.dimen;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.AbsoluteSizeSpan;
import android.text.style.ForegroundColorSpan;
import android.text.style.StyleSpan;
import android.text.style.TypefaceSpan;
import android.view.View;
import android.view.Gravity;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.core.widget.TextViewCompat;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.DistanceAndAzimut;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.routing.RouteMarkType;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.routing.TransitRouteInfo;
import app.organicmaps.sdk.routing.TransitStepInfo;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.recycler.DotDividerItemDecoration;
import app.organicmaps.widget.recycler.MultilineLayoutManager;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.imageview.ShapeableImageView;
import com.google.android.material.textview.MaterialTextView;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;

final class RoutingBottomMenuController implements View.OnClickListener
{
  private static final String STATE_ALTITUDE_CHART_SHOWN = "altitude_chart_shown";
  private static final String STATE_ERROR = "error";

  @NonNull
  private final Activity mContext;
  @NonNull
  private final View mTimeElevationLine;
  @NonNull
  private final View mAltitudeChartFrame;
  @NonNull
  private final View mTransitFrame;
  @NonNull
  private final MaterialTextView mError;
  @NonNull
  private final MaterialButton mStart;
  @NonNull
  private final ShapeableImageView mAltitudeChart;
@NonNull
  private final MaterialTextView mTime;
  @NonNull
  private final MaterialTextView mAltitudeDifference;
  @NonNull
  private final View mSurfaceSection;
  @NonNull
  private final View mSurfaceHeader;
  @NonNull
  private final TextView mSurfaceChevron;
  @NonNull
  private final SurfaceBarView mSurfaceBar;
  @NonNull
  private final LinearLayout mSurfaceLegend;
  private boolean mSurfaceLegendExpanded = false;
  @Nullable
  private final MaterialTextView mTimeVehicle;
  @Nullable
  private final MaterialTextView mArrival;
  @NonNull
  private final View mActionFrame;
  @NonNull
  private final MaterialTextView mActionMessage;
  @NonNull
  private final View mActionButton;
  @NonNull
  private final ShapeableImageView mActionIcon;
  @NonNull
  private final DotDividerItemDecoration mTransitViewDecorator;

  @Nullable
  private final RoutingBottomMenuListener mListener;

  @NonNull
  static RoutingBottomMenuController newInstance(@NonNull Activity activity, @NonNull View frame,
                                                 @NonNull RoutingBottomMenuListener listener)
  {
    View altitudeChartFrame = getViewById(activity, frame, R.id.altitude_chart_panel);
    View timeElevationLine = getViewById(activity, frame, R.id.time_elevation_line);
    View transitFrame = getViewById(activity, frame, R.id.transit_panel);
    MaterialTextView error = (MaterialTextView) getViewById(activity, frame, R.id.error);
    MaterialButton start = (MaterialButton) getViewById(activity, frame, R.id.start);
    ShapeableImageView altitudeChart = (ShapeableImageView) getViewById(activity, frame, R.id.altitude_chart);
    MaterialTextView time = (MaterialTextView) getViewById(activity, frame, R.id.time);
    MaterialTextView timeVehicle = (MaterialTextView) getViewById(activity, frame, R.id.time_vehicle);
    MaterialTextView altitudeDifference = (MaterialTextView) getViewById(activity, frame, R.id.altitude_difference);
    MaterialTextView arrival = (MaterialTextView) getViewById(activity, frame, R.id.arrival);
    View actionFrame = getViewById(activity, frame, R.id.routing_action_frame);
    View surfaceSection = getViewById(activity, frame, R.id.surface_section);
    View surfaceHeader = getViewById(activity, frame, R.id.surface_header);
    TextView surfaceChevron = (TextView) getViewById(activity, frame, R.id.surface_chevron);
    SurfaceBarView surfaceBar = (SurfaceBarView) getViewById(activity, frame, R.id.surface_bar);
    LinearLayout surfaceLegend = (LinearLayout) getViewById(activity, frame, R.id.surface_legend);

    return new RoutingBottomMenuController(activity, altitudeChartFrame, timeElevationLine, transitFrame, error, start,
                                           altitudeChart, time, altitudeDifference, timeVehicle, arrival, actionFrame,
                                           surfaceSection, surfaceHeader, surfaceChevron, surfaceBar, surfaceLegend,
                                           listener);
  }

  @NonNull
  private static View getViewById(@NonNull Activity activity, @NonNull View frame, @IdRes int resourceId)
  {
    View view = frame.findViewById(resourceId);
    return view == null ? activity.findViewById(resourceId) : view;
  }

  private RoutingBottomMenuController(@NonNull Activity context, @NonNull View altitudeChartFrame,
                                      @NonNull View timeElevationLine, @NonNull View transitFrame,
                                      @NonNull MaterialTextView error, @NonNull MaterialButton start,
                                      @NonNull ShapeableImageView altitudeChart, @NonNull MaterialTextView time,
                                      @NonNull MaterialTextView altitudeDifference,
                                      @NonNull MaterialTextView timeVehicle, @Nullable MaterialTextView arrival,
                                      @NonNull View actionFrame, @NonNull View surfaceSection,
                                      @NonNull View surfaceHeader, @NonNull TextView surfaceChevron,
                                      @NonNull SurfaceBarView surfaceBar, @NonNull LinearLayout surfaceLegend,
                                      @Nullable RoutingBottomMenuListener listener)
  {
    mContext = context;
    mAltitudeChartFrame = altitudeChartFrame;
    mTimeElevationLine = timeElevationLine;
    mTransitFrame = transitFrame;
    mError = error;
    mStart = start;
    mAltitudeChart = altitudeChart;
    mTime = time;
    mAltitudeDifference = altitudeDifference;
    mTimeVehicle = timeVehicle;
    mArrival = arrival;
    mActionFrame = actionFrame;
    mSurfaceSection = surfaceSection;
    mSurfaceHeader = surfaceHeader;
    mSurfaceChevron = surfaceChevron;
    mSurfaceBar = surfaceBar;
    mSurfaceLegend = surfaceLegend;
    mSurfaceHeader.setOnClickListener(v -> {
      mSurfaceLegendExpanded = !mSurfaceLegendExpanded;
      applySurfaceLegendVisibility();
    });
    mActionMessage = actionFrame.findViewById(R.id.tv__message);
    mActionButton = actionFrame.findViewById(R.id.btn__my_position_use);
    mActionButton.setOnClickListener(this);
    View actionSearchButton = actionFrame.findViewById(R.id.btn__search_point);
    actionSearchButton.setOnClickListener(this);
    mActionIcon = mActionButton.findViewById(R.id.iv__icon);
    UiUtils.hide(mAltitudeChartFrame, mActionFrame);
    mListener = listener;
    int dividerRes = ThemeUtils.getResource(mContext, R.attr.transitStepDivider);
    Drawable dividerDrawable = ContextCompat.getDrawable(mContext, dividerRes);
    Resources res = mContext.getResources();
    mTransitViewDecorator =
        new DotDividerItemDecoration(dividerDrawable, res.getDimensionPixelSize(R.dimen.margin_base),
                                     res.getDimensionPixelSize(R.dimen.margin_half));
    MaterialButton manageRouteButton = altitudeChartFrame.findViewById(R.id.btn__manage_route);
    manageRouteButton.setOnClickListener(this);

    MaterialButton directionsPreviewButton = altitudeChartFrame.findViewById(R.id.btn__directions_preview);
    directionsPreviewButton.setOnClickListener(this);

    MaterialButton saveButton = altitudeChartFrame.findViewById(R.id.btn__save);
    saveButton.setOnClickListener(this);
  }

  void showAltitudeChartAndRoutingDetails()
  {
    UiUtils.hide(mError, mActionFrame, mAltitudeChart, mTimeElevationLine, mTransitFrame);

    if (!RoutingController.get().isVehicleRouterType() && !RoutingController.get().isRulerRouterType())
      showRouteAltitudeChart();
    showRoutingDetails();
    updateSurfaceSection();
    UiUtils.show(mAltitudeChartFrame);
    MaterialButton saveButton = mAltitudeChartFrame.findViewById(R.id.btn__save);
    saveButton.setText(R.string.save);
    saveButton.setEnabled(true);
  }

  void hideAltitudeChartAndRoutingDetails()
  {
    UiUtils.hide(mAltitudeChartFrame, mTransitFrame);
  }

  @SuppressLint("SetTextI18n")
  void showTransitInfo(@NonNull TransitRouteInfo info)
  {
    UiUtils.hide(mError, mAltitudeChartFrame, mActionFrame);
    showStartButton(false);
    UiUtils.show(mTransitFrame);
    RecyclerView rv = mTransitFrame.findViewById(R.id.transit_recycler_view);
    TransitStepAdapter adapter = new TransitStepAdapter();
    rv.setLayoutManager(new MultilineLayoutManager(mTransitFrame.getLayoutDirection()));
    rv.setNestedScrollingEnabled(false);
    rv.removeItemDecoration(mTransitViewDecorator);
    rv.addItemDecoration(mTransitViewDecorator);
    rv.setAdapter(adapter);
    adapter.setItems(info.getTransitSteps());

    scrollToBottom(rv);

    MaterialTextView totalTimeView = mTransitFrame.findViewById(R.id.total_time);
    totalTimeView.setText(Utils.formatRoutingTime(mContext, info.getTotalTime(), R.dimen.text_size_routing_number));
    View dotView = mTransitFrame.findViewById(R.id.dot);
    View pedestrianIcon = mTransitFrame.findViewById(R.id.pedestrian_icon);
    MaterialTextView distanceView = mTransitFrame.findViewById(R.id.total_distance);
    UiUtils.showIf(info.getTotalPedestrianTimeInSec() > 0, dotView, pedestrianIcon, distanceView);
    distanceView.setText(info.getTotalPedestrianDistance() + " " + info.getTotalPedestrianDistanceUnits());
  }

  @SuppressLint("SetTextI18n")
  void showRulerInfo(@NonNull RouteMarkData[] points, Distance totalLength)
  {
    UiUtils.hide(mError, mAltitudeChartFrame, mActionFrame, mAltitudeChartFrame);
    showStartButton(false);
    UiUtils.show(mTransitFrame);
    final RecyclerView rv = mTransitFrame.findViewById(R.id.transit_recycler_view);
    if (points.length > 2)
    {
      UiUtils.show(rv);
      final TransitStepAdapter adapter = new TransitStepAdapter();
      rv.setLayoutManager(new MultilineLayoutManager(mTransitFrame.getLayoutDirection()));
      rv.setNestedScrollingEnabled(false);
      rv.removeItemDecoration(mTransitViewDecorator);
      rv.addItemDecoration(mTransitViewDecorator);
      rv.setAdapter(adapter);
      adapter.setItems(pointsToRulerSteps(points));

      scrollToBottom(rv);
    }
    else
      UiUtils.hide(rv); // Show only distance between start and finish

    MaterialTextView totalTimeView = mTransitFrame.findViewById(R.id.total_time);
    totalTimeView.setText(mContext.getString(R.string.placepage_distance) + ": " + totalLength.mDistanceStr + " "
                          + totalLength.getUnitsStr(mContext));

    UiUtils.hide(mTransitFrame, R.id.dot);
    UiUtils.hide(mTransitFrame, R.id.pedestrian_icon);
    UiUtils.hide(mTransitFrame, R.id.total_distance);
  }

  // Create steps info to use in TransitStepAdapter.
  private List<TransitStepInfo> pointsToRulerSteps(RouteMarkData[] points)
  {
    List<TransitStepInfo> transitSteps = new LinkedList<>();
    for (int i = 1; i < points.length; i++)
    {
      RouteMarkData segmentStart = points[i - 1];
      RouteMarkData segmentEnd = points[i];
      DistanceAndAzimut dist = Framework.nativeGetDistanceAndAzimuthFromLatLon(segmentStart.mLat, segmentStart.mLon,
                                                                               segmentEnd.mLat, segmentEnd.mLon, 0);
      if (i > 1)
        transitSteps.add(TransitStepInfo.intermediatePoint(i - 2));
      transitSteps.add(
          TransitStepInfo.ruler(dist.getDistance().mDistanceStr, dist.getDistance().getUnitsStr(mContext)));
    }

    return transitSteps;
  }

  void showAddStartFrame()
  {
    UiUtils.hide(mError, mTransitFrame);
    UiUtils.show(mActionFrame);
    mActionMessage.setText(R.string.routing_add_start_point);
    mActionMessage.setTag(RouteMarkType.Start);
    if (MwmApplication.from(mContext).getLocationHelper().getMyPosition() != null)
    {
      UiUtils.show(mActionButton);
      Drawable icon = ContextCompat.getDrawable(mContext, R.drawable.ic_location_crosshair);
      int colorSecondary = ContextCompat.getColor(
          mContext, UiUtils.getStyledResourceId(mContext, com.google.android.material.R.attr.colorSecondary));
      mActionIcon.setImageDrawable(Graphics.tint(icon, colorSecondary));
    }
    else
    {
      UiUtils.hide(mActionButton);
    }
  }

  void showAddFinishFrame()
  {
    UiUtils.hide(mError, mTransitFrame);
    UiUtils.show(mActionFrame);
    mActionMessage.setText(R.string.routing_add_finish_point);
    mActionMessage.setTag(RouteMarkType.Finish);
    UiUtils.hide(mActionButton);
  }

  void hideActionFrame()
  {
    UiUtils.hide(mActionFrame);
  }

  void setStartButton(boolean show)
  {
    if (show)
    {
      mStart.setText(mContext.getText(R.string.p2p_start));
      mStart.setOnClickListener(v -> {
        // Ignore the event if the back and start buttons are pressed at the same time.
        // See {@link #RoutingPlanController.onUpClick()}.
        // https://github.com/organicmaps/organicmaps/issues/6628
        if (mListener != null && RoutingController.get().isPlanning())
          mListener.onRoutingStart();
      });
    }

    showStartButton(show);
  }

  private void showError(@NonNull String message)
  {
    UiUtils.hide(mAltitudeChartFrame, mActionFrame, mTransitFrame);
    mError.setText(message);
    mError.setVisibility(View.VISIBLE);
    showStartButton(false);
  }

  void showStartButton(boolean show)
  {
    boolean result = show && RoutingController.get().isBuilt();
    UiUtils.showIf(result, mStart);
  }

  void saveRoutingPanelState(@NonNull Bundle outState)
  {
    outState.putBoolean(STATE_ALTITUDE_CHART_SHOWN, UiUtils.isVisible(mAltitudeChartFrame));
    if (UiUtils.isVisible(mError))
      outState.putString(STATE_ERROR, mError.getText().toString());
  }

  void restoreRoutingPanelState(@NonNull Bundle state)
  {
    if (state.getBoolean(STATE_ALTITUDE_CHART_SHOWN))
      showAltitudeChartAndRoutingDetails();

    String error = state.getString(STATE_ERROR);
    if (!TextUtils.isEmpty(error))
      showError(error);
  }

  private void showRouteAltitudeChart()
  {
    if (RoutingController.get().isVehicleRouterType())
    {
      UiUtils.hide(mTimeElevationLine, mAltitudeChart);
      return;
    }

    UiUtils.hide(mTimeVehicle);

    int chartWidth = dimen(mContext, R.dimen.altitude_chart_image_width);
    int chartHeight = dimen(mContext, R.dimen.altitude_chart_image_height);
    Framework.RouteAltitudeLimits limits = new Framework.RouteAltitudeLimits();
    Bitmap bm = Framework.generateRouteAltitudeChart(chartWidth, chartHeight, limits);
    if (bm != null)
    {
      mAltitudeChart.setImageBitmap(bm);
      UiUtils.show(mAltitudeChart);
      final String unit = limits.isMetricUnits
                            ? mAltitudeDifference.getResources().getString(app.organicmaps.sdk.R.string.m)
                            : mAltitudeDifference.getResources().getString(app.organicmaps.sdk.R.string.ft);
      mAltitudeDifference.setText("↗ " + limits.totalAscentString + " " + unit + " ↘ " + limits.totalDescentString + " "
                                  + unit);
      UiUtils.show(mAltitudeDifference);
    }
  }

  private void showRoutingDetails()
  {
    final RoutingInfo rinfo = RoutingController.get().getCachedRoutingInfo();
    if (rinfo == null)
    {
      UiUtils.hide(mTimeElevationLine, mTimeVehicle);
      return;
    }

    Spanned spanned = makeSpannedRoutingDetails(mContext, rinfo);
    if (RoutingController.get().isVehicleRouterType())
    {
      UiUtils.show(mTimeVehicle);
      mTimeVehicle.setText(spanned);
    }
    else
    {
      UiUtils.show(mTimeElevationLine);
      mTime.setText(spanned);
    }

    if (mArrival != null)
    {
      String arrivalTime = Utils.formatArrivalTime(rinfo.totalTimeInSeconds);
      mArrival.setText(arrivalTime);
    }
  }

  /**
   * Per-surface breakdown (stacked bar + legend) of the currently active
   * route. The whole section is hidden when the engine produced no surface
   * data (non-BRouter routes, BRouter data sets without way tags).
   */
  private void updateSurfaceSection()
  {
    final Distance[] surface =
        RoutingController.get().getRouteAlternativeSurface(RoutingController.get().getActiveRouteIndex());
    if (surface == null || surface.length != SurfaceBarView.SURFACE_COLOR_RES.length)
    {
      UiUtils.hide(mSurfaceSection);
      return;
    }

    final float[] surfaceMeters = new float[SurfaceBarView.SURFACE_COLOR_RES.length];
    double total = 0.0;
    for (int i = 0; i < surfaceMeters.length; ++i)
    {
      surfaceMeters[i] = (float) surface[i].mDistance;
      total += surface[i].mDistance;
    }
    if (total <= 0.0)
    {
      UiUtils.hide(mSurfaceSection);
      return;
    }

    mSurfaceBar.setSurfaceMeters(surfaceMeters);
    mSurfaceBar.setContentDescription(buildSurfaceContentDescription(mContext, surface));
    mSurfaceLegend.removeAllViews();

    // Most ridable first, Unknown last; rows below 1% are dropped. Every row
    // is a share of the whole route (unknown included), so the rows sum to
    // 100% and never exceed it.
    for (int i = 0; i < SurfaceBarView.SURFACE_NAME_RES.length; ++i)
    {
      final double percent = surface[i].mDistance / total * 100.0;
      if (percent < 1.0)
        continue;
      mSurfaceLegend.addView(buildSurfaceLegendRow(i, surface[i], percent));
    }
    // The legend starts collapsed when the section is first shown so the sheet
    // stays compact on small screens; the user's choice is kept across route
    // updates within the same routing session.
    if (mSurfaceSection.getVisibility() != View.VISIBLE)
      mSurfaceLegendExpanded = false;
    applySurfaceLegendVisibility();
    UiUtils.show(mSurfaceSection);
  }

  private void applySurfaceLegendVisibility()
  {
    UiUtils.showIf(mSurfaceLegendExpanded, mSurfaceLegend);
    mSurfaceChevron.setText(mSurfaceLegendExpanded ? "\u25BE" : "\u25B8");
    mSurfaceHeader.setContentDescription(
        mContext.getString(R.string.routing_surface_title) + " "
        + mContext.getString(mSurfaceLegendExpanded ? R.string.surface_legend_expanded
                                                    : R.string.surface_legend_collapsed));
  }

  @NonNull
  private View buildSurfaceLegendRow(int surfaceIdx, @NonNull Distance distance, double percent)
  {
    final Resources res = mContext.getResources();
    final LinearLayout row = new LinearLayout(mContext);
    row.setOrientation(LinearLayout.HORIZONTAL);
    row.setGravity(Gravity.CENTER_VERTICAL);
    row.setPadding(0, 0, 0, res.getDimensionPixelSize(R.dimen.margin_eighth));

    final View dot = new View(mContext);
    final int dotSize = res.getDimensionPixelSize(R.dimen.elevation_profile_difficulty_dot_size);
    final LinearLayout.LayoutParams dotLp = new LinearLayout.LayoutParams(dotSize, dotSize);
    dotLp.setMarginEnd(res.getDimensionPixelSize(R.dimen.margin_half));
    dot.setLayoutParams(dotLp);
    final GradientDrawable dotBg = new GradientDrawable();
    dotBg.setShape(GradientDrawable.OVAL);
    dotBg.setColor(ContextCompat.getColor(mContext, SurfaceBarView.SURFACE_COLOR_RES[surfaceIdx]));
    dot.setBackground(dotBg);
    row.addView(dot);

    final MaterialTextView text = new MaterialTextView(mContext);
    TextViewCompat.setTextAppearance(text, R.style.MwmTextAppearance_Body3);
    text.setText(String.format(Locale.getDefault(), "%s · %s — %d%%",
                               mContext.getString(SurfaceBarView.SURFACE_NAME_RES[surfaceIdx]),
                               distance.toString(mContext), Math.round(percent)));
    row.addView(text);
    return row;
  }

  @NonNull
  private static String buildSurfaceContentDescription(@NonNull Context context, @NonNull Distance[] surface)
  {
    double total = 0.0;
    for (Distance d : surface)
      total += d.mDistance;
    if (total <= 0.0)
      return "";
    final StringBuilder sb = new StringBuilder();
    for (int i = 0; i < surface.length; ++i)
    {
      final int percent = (int) Math.round(surface[i].mDistance / total * 100.0);
      if (percent <= 0)
        continue;
      if (sb.length() > 0)
        sb.append(", ");
      sb.append(percent).append("% ")
        .append(context.getString(SurfaceBarView.SURFACE_NAME_RES[i]).toLowerCase(Locale.getDefault()));
    }
    return sb.toString();
  }

  // Scroll RecyclerView to bottom using parent ScrollView.
  private static void scrollToBottom(RecyclerView rv)
  {
    final ScrollView parentScroll = (ScrollView) rv.getParent();
    if (parentScroll != null)
      parentScroll.postDelayed(() -> parentScroll.fullScroll(ScrollView.FOCUS_DOWN), 100);
  }

  @NonNull
  private static Spanned makeSpannedRoutingDetails(@NonNull Context context, @NonNull RoutingInfo routingInfo)

  {
    CharSequence time =
        Utils.formatRoutingTime(context, routingInfo.totalTimeInSeconds, R.dimen.text_size_routing_number);

    SpannableStringBuilder builder = new SpannableStringBuilder();
    initTimeBuilderSequence(context, time, builder);

    String dot = "\u00A0• ";
    initDotBuilderSequence(context, dot, builder);

    initDistanceBuilderSequence(context, routingInfo.distToTarget.toString(context), builder);

    return builder;
  }

  private static void initTimeBuilderSequence(@NonNull Context context, @NonNull CharSequence time,
                                              @NonNull SpannableStringBuilder builder)
  {
    builder.append(time);

    builder.setSpan(new TypefaceSpan(context.getResources().getString(R.string.robotoMedium)), 0, builder.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(
        new AbsoluteSizeSpan(context.getResources().getDimensionPixelSize(R.dimen.text_size_routing_number)), 0,
        builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new StyleSpan(Typeface.BOLD), 0, builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new ForegroundColorSpan(ThemeUtils.getColor(context, android.R.attr.textColorPrimary)), 0,
                    builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
  }

  private static void initDotBuilderSequence(@NonNull Context context, @NonNull String dot,
                                             @NonNull SpannableStringBuilder builder)
  {
    builder.append(dot);
    builder.setSpan(new TypefaceSpan(context.getResources().getString(R.string.robotoMedium)),
                    builder.length() - dot.length(), builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(
        new AbsoluteSizeSpan(context.getResources().getDimensionPixelSize(R.dimen.text_size_routing_number)),
        builder.length() - dot.length(), builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new ForegroundColorSpan(ThemeUtils.getColor(context, R.attr.secondary)),
                    builder.length() - dot.length(), builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
  }

  private static void initDistanceBuilderSequence(@NonNull Context context, @NonNull String arrivalTime,
                                                  @NonNull SpannableStringBuilder builder)
  {
    builder.append(arrivalTime);
    builder.setSpan(new TypefaceSpan(context.getResources().getString(R.string.robotoMedium)),
                    builder.length() - arrivalTime.length(), builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(
        new AbsoluteSizeSpan(context.getResources().getDimensionPixelSize(R.dimen.text_size_routing_number)),
        builder.length() - arrivalTime.length(), builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new StyleSpan(Typeface.NORMAL), builder.length() - arrivalTime.length(), builder.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new ForegroundColorSpan(ThemeUtils.getColor(context, android.R.attr.textColorPrimary)),
                    builder.length() - arrivalTime.length(), builder.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
  }

  @Override
  public void onClick(View v)
  {
    final int id = v.getId();
    if (id == R.id.btn__my_position_use)
      mListener.onUseMyPositionAsStart();
    else if (id == R.id.btn__search_point)
    {
      final RouteMarkType pointType = (RouteMarkType) mActionMessage.getTag();
      mListener.onSearchRoutePoint(pointType);
    }
    else if (id == R.id.btn__manage_route)
      mListener.onManageRouteOpen();
    else if (id == R.id.btn__directions_preview)
      mListener.onDirectionsPreviewOpen();
    else if (id == R.id.btn__save)
    {
      Framework.nativeSaveRoute();
      Button saveButton = v.findViewById(R.id.btn__save);
      saveButton.setEnabled(false);
      saveButton.setText(R.string.saved);
    }
  }
}
