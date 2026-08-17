package app.organicmaps.settings;

import android.app.Activity;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.brouter.BRouterServiceClient;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.settings.RoadType;
import app.organicmaps.sdk.util.Constants;
import app.organicmaps.util.Utils;
import com.google.android.material.materialswitch.MaterialSwitch;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

public class CyclingOptionsFragment extends Fragment
{
  private static final float DISABLED_OPTIONS_ALPHA = 0.4f;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_cycling_options, container, false);
    initViews(root);
    return root;
  }

  private void initViews(@NonNull View root)
  {
    MaterialSwitch ferriesBtn = root.findViewById(R.id.avoid_ferries_bicycle_btn);
    ferriesBtn.setChecked(RoutingOptions.hasOption(RoadType.Ferry, Router.Bicycle));
    CompoundButton.OnCheckedChangeListener ferryBtnListener =
        new ToggleRoutingOptionListener(RoadType.Ferry, root, Router.Bicycle);
    ferriesBtn.setOnCheckedChangeListener(ferryBtnListener);

    MaterialSwitch dirtyRoadsBtn = root.findViewById(R.id.avoid_dirty_roads_bicycle_btn);
    dirtyRoadsBtn.setChecked(RoutingOptions.hasOption(RoadType.Dirty, Router.Bicycle));
    CompoundButton.OnCheckedChangeListener dirtyBtnListener =
        new ToggleRoutingOptionListener(RoadType.Dirty, root, Router.Bicycle);
    dirtyRoadsBtn.setOnCheckedChangeListener(dirtyBtnListener);

    MaterialSwitch stepsBtn = root.findViewById(R.id.avoid_steps_bicycle_btn);
    stepsBtn.setChecked(RoutingOptions.hasOption(RoadType.Steps, Router.Bicycle));
    CompoundButton.OnCheckedChangeListener stepsBtnListener =
        new ToggleRoutingOptionListener(RoadType.Steps, root, Router.Bicycle);
    stepsBtn.setOnCheckedChangeListener(stepsBtnListener);

    MaterialSwitch pavedBtn = root.findViewById(R.id.avoid_paved_roads_bicycle_btn);
    pavedBtn.setChecked(RoutingOptions.hasOption(RoadType.Paved, Router.Bicycle));
    CompoundButton.OnCheckedChangeListener pavedBtnListener =
        new ToggleRoutingOptionListener(RoadType.Paved, root, Router.Bicycle);
    pavedBtn.setOnCheckedChangeListener(pavedBtnListener);

    initEngineSelector(root);
  }

  private void initEngineSelector(@NonNull View root)
  {
    final SharedPreferences prefs = MwmApplication.prefs(requireContext());
    boolean brouterEngine = prefs.getBoolean(RoutingController.PREF_BROUTER_ENGINE, false);
    final boolean brouterInstalled = BRouterServiceClient.isBRouterInstalled(requireContext());
    if (!brouterInstalled && brouterEngine)
    {
      brouterEngine = false;
      prefs.edit().putBoolean(RoutingController.PREF_BROUTER_ENGINE, false).apply();
    }

    MaterialSwitch brouterBtn = root.findViewById(R.id.bicycle_engine_brouter_btn);
    brouterBtn.setChecked(brouterEngine);
    brouterBtn.setEnabled(brouterInstalled);
    brouterBtn.setAlpha(brouterInstalled ? 1.0f : DISABLED_OPTIONS_ALPHA);
    if (!brouterInstalled)
    {
      TextView subtitle = root.findViewById(R.id.bicycle_engine_brouter_subtitle);
      subtitle.setText(R.string.cycling_engine_brouter_not_installed);
      subtitle.setAlpha(DISABLED_OPTIONS_ALPHA);
    }
    applyEngineState(root, brouterEngine);

    brouterBtn.setOnCheckedChangeListener((button, isChecked) -> {
      prefs.edit().putBoolean(RoutingController.PREF_BROUTER_ENGINE, isChecked).apply();
      applyEngineState(root, isChecked);
    });

    root.findViewById(R.id.brouter_site_link)
        .setOnClickListener(v -> Utils.openUrl(requireContext(), Constants.Url.BROUTER_SITE));

    final View profilesBtn = root.findViewById(R.id.brouter_profiles_btn);
    if (brouterInstalled)
    {
      profilesBtn.setOnClickListener(v -> {
        if (!BRouterServiceClient.openProfileManager(requireContext()))
          Utils.openUrl(requireContext(), Constants.Url.BROUTER_SITE);
      });
    }
    else
    {
      profilesBtn.setVisibility(View.GONE);
    }
  }

  // The avoid-* options only apply to the built-in bicycle router; grey them
  // out when the BRouter engine is selected so the user understands that.
  private void applyEngineState(@NonNull View root, boolean brouterEngine)
  {
    View avoidOptions = root.findViewById(R.id.cycling_avoid_options);
    avoidOptions.setEnabled(!brouterEngine);
    avoidOptions.setAlpha(brouterEngine ? DISABLED_OPTIONS_ALPHA : 1.0f);
    avoidOptions.setClickable(!brouterEngine);
    avoidOptions.setFocusable(!brouterEngine);
    for (int childId : new int[] {R.id.avoid_ferries_bicycle_btn, R.id.avoid_dirty_roads_bicycle_btn,
                                  R.id.avoid_steps_bicycle_btn, R.id.avoid_paved_roads_bicycle_btn})
      root.findViewById(childId).setEnabled(!brouterEngine);
  }

  private record
      ToggleRoutingOptionListener(@NonNull RoadType mRoadType, @NonNull View mRoot, @NonNull Router mRouterType)
      implements CompoundButton.OnCheckedChangeListener {
    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
    {
      if (isChecked)
      {
        if (mRoadType == RoadType.Dirty)
        {
          MaterialSwitch btn = mRoot.findViewById(R.id.avoid_paved_roads_bicycle_btn);
          btn.setChecked(false);
        }
        else if (mRoadType == RoadType.Paved)
        {
          MaterialSwitch btn = mRoot.findViewById(R.id.avoid_dirty_roads_bicycle_btn);
          btn.setChecked(false);
        }
        RoutingOptions.addOption(mRoadType, mRouterType);
      }
      else
        RoutingOptions.removeOption(mRoadType, mRouterType);
    }
  }
}
