#pragma once

#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include "routing/turns.hpp"

#include <string>
#include <vector>

namespace routing
{
// Holds the parsed <rtept> turn instruction.
struct TurnHint
{
  int turnCode = 0;       // BRouter turn code (0 = continue; see parser constants)
  double angleDeg = 0.0;
  double timeSec = 0.0;   // segment duration in seconds (delta, not cumulative)
  int offset = -1;        // trkpt offset within the track (from <offset> child)
  std::string streetName;
  std::string ref;
  std::string destination;
};

// A single parsed BRouter response: one track polyline plus its turn
// instructions and per-point elevations. BRouter returns exactly one <trk>
// and one <rte> per response (the requested alternative index selects which).
struct BrouterTrack
{
  std::vector<m2::PointD> points;
  std::vector<TurnHint> hints;
  std::vector<geometry::Altitude> altitudes;
};

// Parse the BRouter GPX payload once (single pass over the document), filling
// all three BrouterTrack members. Returns an empty points vector on failure.
BrouterTrack ParseGpxResponse(std::string const & gpx);

// Map a BRouter OsmAnd turn code to a OM CarDirection. Returns None for
// "continue" and unknown codes (no arrow rendered).
turns::CarDirection BrouterTurnToCarDirection(int code, double angleDeg);

// Build a per-segment cumulative-time vector from BRouter <rtept> entries.
// Each rtept has a trkpt `offset` and a `time` which is the SEGMENT duration
// (delta from the previous rtept to this one, not cumulative). Total time is
// the sum of all segment durations; we linearly interpolate per segment based
// on the offset positions.
std::vector<double> BuildCumulativeTimes(std::vector<m2::PointD> const & track,
                                         std::vector<TurnHint> const & hints);
}  // namespace routing