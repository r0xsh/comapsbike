#pragma once

#include "routing/router.hpp"

#include <string>
#include <vector>

namespace routing
{
// BRouterRouter drives the BRouter Android Service via JNI. It is registered as
// routing::RouterType::BRouter and produces one or more Route objects from the
// GPX payload returned by BRouter's getTrackFromParams() call.
//
// GPX parsing / turn classification / cumulative-time interpolation live in
// brouter_gpx_parser.{hpp,cpp}.
//
// Note: this class is only compiled on the Android build (the BRouter companion
// app is an Android-only dependency). On other platforms a stub is fine.
class BRouterRouter : public IRouter
{
public:
  // The "*-router" suffix matches RulerRouter and is what async_router.cpp
  // compares against (GetRouterId() != "ruler-router").
  static constexpr char const * kBRouterName = "brouter-router";

  BRouterRouter();
  ~BRouterRouter() override;

  std::string GetName() const override { return kBRouterName; }
  void ClearState() override {}
  void SetGuides(GuidesTracks && /*guides*/) override {}

  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & startDirection,
                                  bool adjust, RouterDelegate const & delegate, Route & route) override;

  /// Produces up to kMaxRoutes Route objects from the BRouter GPX response.
  std::vector<Route> CalculateRoutes(Checkpoints const & checkpoints, m2::PointD const & startDirection,
                                     bool adjust, RouterDelegate const & delegate) override;

  bool FindClosestProjectionToRoad(m2::PointD const & point, m2::PointD const & direction, double radius,
                                   EdgeProj & proj) override;

  static constexpr int kMaxRoutes = 4;
};
}  // namespace routing
