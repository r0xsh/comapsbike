#include "testing/testing.hpp"

#include "routing/brouter_gpx_parser.hpp"
#include "routing/route_surface.hpp"

#include "base/math.hpp"

#include <string>
#include <vector>

namespace routing_test
{
using namespace routing;

UNIT_TEST(SurfaceFromWayTags_Paved)
{
  TEST_EQUAL(SurfaceFromWayTags("highway=residential surface=asphalt"), RouteSurface::Paved, ());
  TEST_EQUAL(SurfaceFromWayTags("highway=tertiary surface=concrete:lanes"), RouteSurface::Paved, ());
  TEST_EQUAL(SurfaceFromWayTags("highway=track tracktype=grade1"), RouteSurface::Paved, ());
  TEST_EQUAL(SurfaceFromWayTags("surface=paving_stones"), RouteSurface::Paved, ());
  TEST_EQUAL(SurfaceFromWayTags("surface=cobblestone"), RouteSurface::Paved, ());
  TEST_EQUAL(SurfaceFromWayTags("highway=path tracktype=grade1"), RouteSurface::Paved, ());
}

UNIT_TEST(SurfaceFromWayTags_Gravel)
{
  TEST_EQUAL(SurfaceFromWayTags("highway=track surface=gravel tracktype=grade2"),
             RouteSurface::Gravel, ());
  TEST_EQUAL(SurfaceFromWayTags("surface=fine_gravel"), RouteSurface::Gravel, ());
  TEST_EQUAL(SurfaceFromWayTags("surface=compacted"), RouteSurface::Gravel, ());
  TEST_EQUAL(SurfaceFromWayTags("highway=track tracktype=grade3"), RouteSurface::Gravel, ());
}

UNIT_TEST(SurfaceFromWayTags_Dirt)
{
  TEST_EQUAL(SurfaceFromWayTags("highway=track surface=dirt"), RouteSurface::Dirt, ());
  TEST_EQUAL(SurfaceFromWayTags("surface=earth"), RouteSurface::Dirt, ());
  TEST_EQUAL(SurfaceFromWayTags("highway=track tracktype=grade4"), RouteSurface::Dirt, ());
  TEST_EQUAL(SurfaceFromWayTags("highway=track tracktype=grade5"), RouteSurface::Dirt, ());
  TEST_EQUAL(SurfaceFromWayTags("surface=unpaved"), RouteSurface::Dirt, ());
  TEST_EQUAL(SurfaceFromWayTags("surface=mud"), RouteSurface::Dirt, ());
}

UNIT_TEST(SurfaceFromWayTags_Singletrack)
{
  TEST_EQUAL(SurfaceFromWayTags("highway=bridleway"), RouteSurface::Singletrack, ());
  TEST_EQUAL(SurfaceFromWayTags("highway=path"), RouteSurface::Singletrack, ());
  TEST_EQUAL(SurfaceFromWayTags("highway=path tracktype=grade3"), RouteSurface::Singletrack, ());
  TEST_EQUAL(SurfaceFromWayTags("highway=track mtb:scale=1"), RouteSurface::Singletrack, ());
  TEST_EQUAL(SurfaceFromWayTags("highway=track sac_scale=hiking"), RouteSurface::Singletrack, ());
}

UNIT_TEST(SurfaceFromWayTags_Unknown)
{
  TEST_EQUAL(SurfaceFromWayTags(""), RouteSurface::Unknown, ());
  TEST_EQUAL(SurfaceFromWayTags("direct_segment=7"), RouteSurface::Unknown, ());
  TEST_EQUAL(SurfaceFromWayTags("highway=track"), RouteSurface::Unknown, ());
  TEST_EQUAL(SurfaceFromWayTags("surface=wood"), RouteSurface::Unknown, ());
  TEST_EQUAL(SurfaceFromWayTags("highway=service surface=acacia_wood"), RouteSurface::Unknown, ());
  // Case-insensitive keys and values.
  TEST_EQUAL(SurfaceFromWayTags("Surface=Asphalt"), RouteSurface::Paved, ());
}

namespace
{
// A minimal mode-9 style payload. Way tags appear only when they change
// (run-length encoded); speeds appear per point; hints per maneuver.
std::string const kMode9Gpx = R"(<?xml version="1.0"?>
<gpx version="1.1" creator="BRouter">
  <metadata>
    <extensions>
      <brouter:info>track-length = 400 filtered ascend = 5 plain-ascend = 2 cost=42 energy=.1kwh time=2m 40s</brouter:info>
    </extensions>
  </metadata>
  <trk>
    <trkseg>
      <trkpt lon="2.000000" lat="48.000000"><ele>10</ele><desc>start</desc><type>via</type></trkpt>
      <trkpt lon="2.001000" lat="48.001000"><ele>12</ele>
        <extensions><brouter:speed>20.4</brouter:speed><brouter:way>highway=residential surface=asphalt</brouter:way></extensions>
      </trkpt>
      <trkpt lon="2.002000" lat="48.002000"><ele>14</ele>
        <extensions><brouter:speed>21.1</brouter:speed></extensions>
      </trkpt>
      <trkpt lon="2.003000" lat="48.003000"><ele>15</ele><desc>Turn left</desc><sym>TL</sym>
        <extensions><brouter:speed>18.0</brouter:speed><brouter:voicehint>TL;234,(2.002000,48.002000)(2.003000,48.003000)</brouter:voicehint><brouter:way>highway=track surface=gravel tracktype=grade2</brouter:way></extensions>
      </trkpt>
      <trkpt lon="2.004000" lat="48.004000"><ele>16</ele>
        <extensions><brouter:speed>19.5</brouter:speed></extensions>
      </trkpt>
      <trkpt lon="2.005000" lat="48.005000"><ele>18</ele><desc>end</desc><type>via</type>
        <extensions><brouter:speed>0.0</brouter:speed></extensions>
      </trkpt>
    </trkseg>
  </trk>
</gpx>)";
}  // namespace

UNIT_TEST(ParseGpxResponse_Mode9)
{
  BrouterTrack const track = ParseGpxResponse(kMode9Gpx);
  TEST_EQUAL(track.points.size(), 6, ());
  TEST_EQUAL(track.altitudes.size(), 6, ());
  TEST_EQUAL(track.altitudes[0], 10, ());
  TEST_EQUAL(track.altitudes[5], 18, ());
  // Total route time from the <brouter:info> metadata: "time=2m 40s".
  TEST_ALMOST_EQUAL_ABS(track.totalTimeSec, 160.0, 1e-6, ());

  // Way tags are run-length decoded: points 0..2 asphalt, points 3..5 gravel.
  TEST_EQUAL(track.wayTagsPerPoint.size(), 6, ());
  TEST_EQUAL(track.wayTagsPerPoint[0], "", ());
  TEST_EQUAL(track.wayTagsPerPoint[1], "highway=residential surface=asphalt", ());
  TEST_EQUAL(track.wayTagsPerPoint[2], "highway=residential surface=asphalt", ());
  TEST_EQUAL(track.wayTagsPerPoint[3], "highway=track surface=gravel tracktype=grade2", ());
  TEST_EQUAL(track.wayTagsPerPoint[4], "highway=track surface=gravel tracktype=grade2", ());
  TEST_EQUAL(track.wayTagsPerPoint[5], "highway=track surface=gravel tracktype=grade2", ());

  // Speeds per point; absent entries are 0.
  TEST_EQUAL(track.speedKphPerPoint.size(), 6, ());
  TEST_EQUAL(track.speedKphPerPoint[0], 0.0, ());
  TEST_EQUAL(track.speedKphPerPoint[1], 20.4, ());
  TEST_EQUAL(track.speedKphPerPoint[5], 0.0, ());

  // One maneuver at trkpt 3 with the BRouter turn code and distance.
  TEST_EQUAL(track.hints.size(), 1, ());
  TEST_EQUAL(track.hints[0].offset, 3, ());
  TEST_EQUAL(track.hints[0].distanceToNextM, 234.0, ());
  TEST_EQUAL(track.hints[0].turnCode, 3, ());  // TL
}

UNIT_TEST(BuildCumulativeTimes_SpeedBased)
{
  BrouterTrack track = ParseGpxResponse(kMode9Gpx);
  std::vector<double> const times = BuildCumulativeTimes(track);
  TEST_EQUAL(times.size(), track.points.size() - 1, ());
  // Cumulative times must be non-decreasing and positive where speed exists.
  for (size_t i = 0; i < times.size(); ++i)
  {
    TEST_GREATER_OR_EQUAL(times[i], 0.0, ());
    if (i > 0)
      TEST_GREATER_OR_EQUAL(times[i], times[i - 1], ());
  }
  TEST_GREATER(times.back(), 0.0, ());
  // The fixture also carries a <brouter:info> total of 160 s; per-point
  // speeds must win over it. BRouter writes the same info on every
  // alternative, so following the total would make all routes show the same
  // time (the bug this guards against).
  TEST_NOT_EQUAL(times.back(), 160.0, ());
}

UNIT_TEST(BuildCumulativeTimes_TimeFallback)
{
  // No speeds, but <time> stamps: fall back to date deltas.
  std::string const gpx = R"(<?xml version="1.0"?>
<gpx version="1.1" creator="BRouter">
  <trk>
    <trkseg>
      <trkpt lon="2.000000" lat="48.000000"><ele>10</ele><time>2026-01-01T00:00:00.000Z</time></trkpt>
      <trkpt lon="2.001000" lat="48.001000"><ele>12</ele><time>2026-01-01T00:01:00.000Z</time></trkpt>
      <trkpt lon="2.002000" lat="48.002000"><ele>14</ele><time>2026-01-01T00:03:00.000Z</time></trkpt>
    </trkseg>
  </trk>
</gpx>)";
  BrouterTrack const track = ParseGpxResponse(gpx);
  std::vector<double> const times = BuildCumulativeTimes(track);
  TEST_EQUAL(times.size(), 2, ());
  // 60 s for the first segment, 180 s cumulative after the second.
  TEST_ALMOST_EQUAL_ABS(times[0], 60.0, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(times[1], 180.0, 1e-6, ());
}

UNIT_TEST(BuildCumulativeTimes_InfoBased)
{
  // No speeds, no time stamps, but a brouter:info total: distribute it
  // proportionally to distance. The fixture's 5 segments are ~138 m each,
  // so the cumulative times ramp up to the full 160 s.
  std::string const gpx = R"(<?xml version="1.0"?>
<gpx version="1.1" creator="BRouter">
  <metadata>
    <extensions>
      <brouter:info>track-length = 690 filtered ascend = 5 plain-ascend = 2 cost=42 energy=.1kwh time=2m 40s</brouter:info>
    </extensions>
  </metadata>
  <trk>
    <trkseg>
      <trkpt lon="2.000000" lat="48.000000"><ele>10</ele></trkpt>
      <trkpt lon="2.001000" lat="48.001000"><ele>12</ele></trkpt>
      <trkpt lon="2.002000" lat="48.002000"><ele>14</ele></trkpt>
      <trkpt lon="2.003000" lat="48.003000"><ele>15</ele></trkpt>
      <trkpt lon="2.004000" lat="48.004000"><ele>16</ele></trkpt>
      <trkpt lon="2.005000" lat="48.005000"><ele>18</ele></trkpt>
    </trkseg>
  </trk>
</gpx>)";
  BrouterTrack const track = ParseGpxResponse(gpx);
  TEST_ALMOST_EQUAL_ABS(track.totalTimeSec, 160.0, 1e-6, ());
  std::vector<double> const times = BuildCumulativeTimes(track);
  TEST_EQUAL(times.size(), 5, ());
  for (size_t i = 0; i < times.size(); ++i)
  {
    TEST_GREATER(times[i], 0.0, ());
    if (i > 0)
      TEST_GREATER(times[i], times[i - 1], ());
  }
  // The last cumulative time reproduces the full metadata total.
  TEST_ALMOST_EQUAL_ABS(times.back(), 160.0, 1e-3, ());
}

UNIT_TEST(ParseInfoTime_Formats)
{
  // Handled via ParseGpxResponse metadata; exercise the "1h 58m 17s" shape.
  std::string const gpx = R"(<?xml version="1.0"?>
<gpx version="1.1" creator="BRouter">
  <metadata>
    <extensions>
      <brouter:info>track-length = 17975 filtered ascend = 123 plain-ascend = 22 cost=26880 energy=.1kwh time=1h 58m 17s</brouter:info>
    </extensions>
  </metadata>
  <trk><trkseg>
    <trkpt lon="2.000000" lat="48.000000"><ele>10</ele></trkpt>
    <trkpt lon="2.001000" lat="48.001000"><ele>12</ele></trkpt>
  </trkseg></trk>
</gpx>)";
  BrouterTrack const track = ParseGpxResponse(gpx);
  TEST_ALMOST_EQUAL_ABS(track.totalTimeSec, 3600.0 + 58.0 * 60.0 + 17.0, 1e-6, ());
}

UNIT_TEST(ParseInfoTime_NoTime)
{
  std::string const gpx = R"(<?xml version="1.0"?>
<gpx version="1.1" creator="BRouter">
  <metadata>
    <extensions>
      <brouter:info>track-length = 100 filtered ascend = 0 plain-ascend = 0 cost=42</brouter:info>
    </extensions>
  </metadata>
  <trk><trkseg>
    <trkpt lon="2.000000" lat="48.000000"><ele>10</ele></trkpt>
    <trkpt lon="2.001000" lat="48.001000"><ele>12</ele></trkpt>
  </trkseg></trk>
</gpx>)";
  BrouterTrack const track = ParseGpxResponse(gpx);
  TEST_EQUAL(track.totalTimeSec, 0.0, ());
  // No speed, no stamps, no info total: all-zero times.
  std::vector<double> const times = BuildCumulativeTimes(track);
  TEST_EQUAL(times.size(), 1, ());
  TEST_EQUAL(times[0], 0.0, ());
}

UNIT_TEST(BuildCumulativeTimes_NoData)
{
  std::string const gpx = R"(<?xml version="1.0"?>
<gpx version="1.1" creator="BRouter">
  <trk>
    <trkseg>
      <trkpt lon="2.000000" lat="48.000000"><ele>10</ele></trkpt>
      <trkpt lon="2.001000" lat="48.001000"><ele>12</ele></trkpt>
    </trkseg>
  </trk>
</gpx>)";
  BrouterTrack const track = ParseGpxResponse(gpx);
  std::vector<double> const times = BuildCumulativeTimes(track);
  TEST_EQUAL(times.size(), 1, ());
  TEST_EQUAL(times[0], 0.0, ());
}
}  // namespace routing_test