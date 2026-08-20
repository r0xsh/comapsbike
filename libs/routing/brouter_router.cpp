#include "routing/brouter_router.hpp"

#include "routing/brouter_gpx_parser.hpp"
#include "routing/route.hpp"
#include "routing/route_surface.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/segment.hpp"
#include "routing/turns.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

// Android-only JNI bridge to the BRouter companion app.
#ifdef __ANDROID__
#include "app/organicmaps/sdk/brouter/BRouterServiceClient.hpp"
#endif

namespace routing
{
namespace
{
// Find the hint for the segment ending at polyline point `idx + 1`. Each
// <rtept> carries an `offset` which is the trkpt (polyline point) index of the
// maneuver.
// With exact=true the hint must land exactly on the segment end (offset + 1 ==
// segEnd): one turn per maneuver, arrow on the road leaving the junction.
// With exact=false the nearest hint is returned; used for road-name
// propagation, which tolerates offset drift.
TurnHint const * FindHintForSegment(std::vector<TurnHint> const & hints, int idx, bool exact)
{
  if (hints.empty())
    return nullptr;
  int const segEnd = idx + 1;
  if (exact)
  {
    for (auto const & h : hints)
    {
      if (h.offset + 1 == segEnd)
        return &h;
    }
    return nullptr;
  }
  TurnHint const * best = &hints.front();
  int bestDelta = std::abs(hints.front().offset - segEnd);
  for (auto const & h : hints)
  {
    int const d = std::abs(h.offset - segEnd);
    if (d < bestDelta)
    {
      bestDelta = d;
      best = &h;
    }
  }
  return best;
}

// Build the TurnItem for the segment starting at `points[idx]`.
// `pointIndex` is the polyline point index (segment idx + 1).
turns::TurnItem BuildTurnItem(int idx, std::vector<TurnHint> const & hints, uint32_t pointIndex)
{
  TurnHint const * h = FindHintForSegment(hints, idx, true /* exact */);
  if (h == nullptr)
    return turns::TurnItem(pointIndex, turns::CarDirection::None);
  // Mode 9 voicehints carry no turn angle; direction comes from the turn code.
  return turns::TurnItem(pointIndex, BrouterTurnToCarDirection(h->turnCode, 0.0));
}

std::vector<Route> BuildRoutes(std::vector<BrouterTrack> const & tracks)
{
  std::vector<Route> routes;
  routes.reserve(tracks.size());
  uint64_t routeId = 1;
  Segment const segment(kFakeNumMwmId, 0, 0, false);

  for (auto const & trackData : tracks)
  {
    auto const & track = trackData.points;
    std::vector<TurnHint> const & hints = trackData.hints;
    std::vector<geometry::Altitude> const & trackAlts = trackData.altitudes;
    std::vector<std::string> const & wayTagsPerPoint = trackData.wayTagsPerPoint;
    if (track.size() < 2)
      continue;
    auto const safeAlt = [](geometry::Altitude a) {
      return a == geometry::kInvalidAltitude ? geometry::kDefaultAltitudeMeters : a;
    };
    auto const toPointWAIdx = [&safeAlt, &trackAlts](m2::PointD const & p, size_t idx) {
      return geometry::PointWithAltitude(
          p, (idx < trackAlts.size()) ? safeAlt(trackAlts[idx]) : geometry::kDefaultAltitudeMeters);
    };
    Route route(BRouterRouter::kBRouterName, routeId);

    std::vector<RouteSegment> routeSegments;
    routeSegments.reserve(track.size() - 1);
    std::vector<double> times = BuildCumulativeTimes(trackData);

    // Accumulate per-surface distances while building the segments. BRouter
    // attaches the way tags to the point the way ends at, so segment i
    // (points i -> i + 1) inherits the tags carried by point i + 1.
    SurfaceStats surfaceStats;
    for (size_t i = 0; i < track.size() - 1; ++i)
    {
      // m_index is the polyline point index that this turn applies to, which
      // is the end of segment i (i.e. point i + 1).
      uint32_t const pointIndex = static_cast<uint32_t>(i) + 1u;
      turns::TurnItem turn = BuildTurnItem(static_cast<int>(i), hints, pointIndex);
      if (i == track.size() - 2)
        turn = turns::TurnItem(pointIndex, turns::CarDirection::ReachedYourDestination);
      geometry::PointWithAltitude const junction = toPointWAIdx(track[i], i);
      RouteSegment::RoadNameInfo roadNameInfo;
      // Propagate BRouter street name / ref / dest when present (nearest hint
      // tolerates small offset drift from BRouter).
      if (TurnHint const * best = FindHintForSegment(hints, static_cast<int>(i), false /* exact */))
      {
        roadNameInfo.m_name = best->streetName;
        roadNameInfo.m_ref = best->ref;
        roadNameInfo.m_destination = best->destination;
      }
      routeSegments.emplace_back(segment, turn, junction, roadNameInfo);

      std::string const & wayTags =
          (i + 1 < wayTagsPerPoint.size()) ? wayTagsPerPoint[i + 1] : std::string{};
      RouteSurface const surface = SurfaceFromWayTags(wayTags);
      routeSegments.back().SetSurface(surface);
      double const segDistM = mercator::DistanceOnEarth(track[i], track[i + 1]);
      surfaceStats.m_distanceM[static_cast<size_t>(surface)] += segDistM;
      surfaceStats.m_totalM += segDistM;
    }
    FillSegmentInfo(times, routeSegments);

    std::vector<Route::SubrouteAttrs> subroutes;
    subroutes.reserve(1);
    // One subroute covering the whole track (matches RulerRouter / IndexRouter
    // fallback for single-segment GPX routes).
    subroutes.emplace_back(toPointWAIdx(track.front(), 0),
                           toPointWAIdx(track.back(), track.size() - 1), 0, track.size() - 1);

    route.SetRouteSegments(std::move(routeSegments));
    route.SetSubroteAttrs(std::move(subroutes));
    route.SetGeometry(track.begin(), track.end());
    route.SetSurfaceStats(std::move(surfaceStats));
    routes.push_back(std::move(route));
    ++routeId;
  }
  return routes;
}
}  // namespace

BRouterRouter::BRouterRouter() = default;
BRouterRouter::~BRouterRouter() = default;

RouterResultCode BRouterRouter::CalculateRoute(Checkpoints const & checkpoints,
                                               m2::PointD const & /*startDirection*/, bool /*adjust*/,
                                               RouterDelegate const & delegate, Route & route)
{
  auto const routes = CalculateRoutes(checkpoints, m2::PointD::Zero(), false, delegate);
  if (routes.empty())
    return RouterResultCode::RouteNotFound;
  route = routes.front();
  return RouterResultCode::NoError;
}

std::vector<Route> BRouterRouter::CalculateRoutes(Checkpoints const & checkpoints,
                                                  m2::PointD const & /*startDirection*/, bool /*adjust*/,
                                                  RouterDelegate const & delegate)
{
  std::vector<m2::PointD> const points = checkpoints.GetPoints();
  if (points.size() < 2)
    return {};

  std::vector<double> lats;
  std::vector<double> lons;
  lats.reserve(points.size());
  lons.reserve(points.size());
  for (auto const & pt : points)
  {
    ms::LatLon const ll = mercator::ToLatLon(pt);
    lats.push_back(ll.m_lat);
    lons.push_back(ll.m_lon);
  }

  std::vector<BrouterTrack> tracks;
#ifdef __ANDROID__
  // One bind cycle fetches all kMaxRoutes alternatives (Java side iterates
  // the BRouter alternative indexes and stops on the first failure). Each
  // mode 9 document carries per-point way tags and, via the injected
  // profile:showspeed variable, a per-point <brouter:speed> that yields
  // exact per-alternative route times (the <brouter:info> metadata total is
  // shared by all alternatives and thus not per-route).
  std::vector<std::string> const gpxList = jni::BRouterCalculateRoutes(lats, lons, kMaxRoutes);
  for (auto const & gpx : gpxList)
  {
    if (delegate.IsCancelled())
      break;

    BrouterTrack track = ParseGpxResponse(gpx);
    if (track.points.size() < 2)
      break;

    bool const duplicate = std::any_of(
        tracks.begin(), tracks.end(),
        [&](auto const & existing) { return existing.points == track.points; });
    if (duplicate)
      break;  // BRouter returned the same path for this altIdx; nothing more to fetch

    tracks.push_back(std::move(track));
  }
#else
  LOG(LWARNING, ("BRouterRouter: BRouter is Android-only; returning empty result"));
  return {};
#endif

  if (tracks.empty())
  {
    LOG(LWARNING, ("BRouterRouter: no routes returned by BRouter service"));
    return {};
  }

  LOG(LINFO, ("BRouterRouter: produced ", tracks.size(), " alternative route(s)"));
  return BuildRoutes(tracks);
}

bool BRouterRouter::FindClosestProjectionToRoad(m2::PointD const & /*point*/, m2::PointD const & /*direction*/,
                                                double /*radius*/, EdgeProj & /*proj*/)
{
  return false;
}
}  // namespace routing