#pragma once

#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include "routing/turns.hpp"

#include <string>
#include <vector>

namespace routing
{
// Holds one parsed maneuver. In BRouter mode 9 ("BRouter style") maneuvers
// arrive per-trackpoint as <brouter:voicehint>cmd;distanceToNext,geometry
// inside <trkpt><extensions>.
struct TurnHint
{
  int turnCode = 0;       // BRouter turn code (0 = continue; see parser constants)
  double distanceToNextM = 0.0;  // distance from this point to the next maneuver
  int offset = -1;        // trkpt offset within the track
  std::string streetName;
  std::string ref;
  std::string destination;
};

// A single parsed BRouter response: one track polyline plus its turn
// instructions, per-point elevations, way tags and speeds. BRouter returns
// exactly one <trk> per response (the requested alternative index selects
// which).
struct BrouterTrack
{
  std::vector<m2::PointD> points;
  std::vector<TurnHint> hints;
  std::vector<geometry::Altitude> altitudes;
  // Run-length decoded <brouter:way> content per trackpoint: each point
  // carries the key=value pairs of the way it belongs to ("" before the
  // first tagged point).
  std::vector<std::string> wayTagsPerPoint;
  // Per-point <brouter:speed> in km/h, 0 when absent.
  std::vector<double> speedKphPerPoint;
  // Per-point <time> as UTC epoch seconds, 0 when absent.
  std::vector<double> timeEpochPerPoint;
  // Total route time in seconds from the <brouter:info> metadata, 0 when
  // absent. This is only a fallback: BRouter 1.7.x ignores a plain showspeed
  // request parameter (it is a profile expression variable), so the app
  // injects "profile:showspeed=1" to make mode 9 emit exact per-point
  // <brouter:speed> elements instead. Also note BRouter writes the same
  // <brouter:info> on every alternative, so the total is only exact for the
  // primary route.
  double totalTimeSec = 0.0;
};

// Parse the BRouter GPX payload once (single pass over the document), filling
// all BrouterTrack members. Returns an empty points vector on failure.
BrouterTrack ParseGpxResponse(std::string const & gpx);

// Map a BRouter OsmAnd turn code to a OM CarDirection. Returns None for
// "continue" and unknown codes (no arrow rendered).
turns::CarDirection BrouterTurnToCarDirection(int code, double angleDeg);

// Build a per-segment cumulative-time vector for the track. Priority: exact
// per-point speeds (<brouter:speed>, enabled via the injected
// profile:showspeed profile variable and thus present per alternative), then
// <time> stamp deltas, then the <brouter:info> total distributed
// proportionally to distance. Returns an all-zero vector when the track
// carries none of them.
std::vector<double> BuildCumulativeTimes(BrouterTrack const & track);
}  // namespace routing
