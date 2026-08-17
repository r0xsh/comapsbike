#include <jni.h>
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "routing/router.hpp"
#include "routing/routing_options.hpp"

static routing::RoutingOptions::Option makeValue(jint option)
{
  auto const opt = static_cast<uint8_t>(1u << static_cast<int>(option));
  CHECK_LESS(opt, static_cast<uint8_t>(routing::RoutingOptions::Option::Max), ("invalid option", option));
  return static_cast<routing::RoutingOptions::Option>(opt);
}

// Java Router enum ordinal → routing::RouterType → routing::VehicleType. Used
// for routing options persistence so that Ruler/BRouter fall back to a sane
// vehicle without crashing the JNI CHECK. BRouter is treated as bicycle since
// BRouter's primary profile family is cycling.
static routing::VehicleType routerToVehicle(jint router)
{
  auto const r = static_cast<routing::RouterType>(router);
  switch (r)
  {
  case routing::RouterType::Vehicle: return routing::VehicleType::Car;
  case routing::RouterType::Pedestrian: return routing::VehicleType::Pedestrian;
  case routing::RouterType::Bicycle: return routing::VehicleType::Bicycle;
  case routing::RouterType::Transit: return routing::VehicleType::Transit;
  case routing::RouterType::Ruler: return routing::VehicleType::Car;
  case routing::RouterType::BRouter: return routing::VehicleType::Bicycle;
  case routing::RouterType::Count: break;
  }
  return routing::VehicleType::Car;
}

extern "C"
{
JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_routing_RoutingOptions_nativeHasOption(JNIEnv *, jclass,
                                                                                           jint option, jint vehicle)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  routing::RoutingOptions routingOptions = routing::RoutingOptions::LoadOptionsFromSettings(routerToVehicle(vehicle));
  routing::RoutingOptions::Option opt = makeValue(option);
  return static_cast<jboolean>(routingOptions.Has(opt));
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_routing_RoutingOptions_nativeAddOption(JNIEnv *, jclass, jint option,
                                                                                       jint vehicle)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  routing::RoutingOptions routingOptions = routing::RoutingOptions::LoadOptionsFromSettings(routerToVehicle(vehicle));
  routing::RoutingOptions::Option opt = makeValue(option);
  routingOptions.Add(opt);
  routing::RoutingOptions::SaveOptionsToSettings(routingOptions);
}

JNIEXPORT void JNICALL Java_app_organicmaps_sdk_routing_RoutingOptions_nativeRemoveOption(JNIEnv *, jclass, jint option,
                                                                                          jint vehicle)
{
  CHECK(g_framework, ("Framework isn't created yet!"));
  routing::RoutingOptions routingOptions = routing::RoutingOptions::LoadOptionsFromSettings(routerToVehicle(vehicle));
  routing::RoutingOptions::Option opt = makeValue(option);
  routingOptions.Remove(opt);
  routing::RoutingOptions::SaveOptionsToSettings(routingOptions);
}
}
