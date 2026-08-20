#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace routing
{
// Surface classification of BRouter way tags (see SurfaceFromWayTags).
enum class RouteSurface : uint8_t
{
  Unknown = 0,
  Paved,
  Gravel,
  Dirt,
  Singletrack,
  Count
};

size_t constexpr kRouteSurfaceCount = static_cast<size_t>(RouteSurface::Count);

// Per-route accumulated surface statistics, in meters.
struct SurfaceStats
{
  // Distance per RouteSurface bucket. Index 0 (Unknown) also accumulates
  // beeline legs and untagged segments.
  std::array<double, kRouteSurfaceCount> m_distanceM = {};
  // Full route length (sum over all buckets).
  double m_totalM = 0.0;

  // True when at least one of the four named buckets (paved..singletrack)
  // has any distance, i.e. the data set carried way tags at all.
  bool HasNamedSurface() const
  {
    for (size_t i = 1; i < kRouteSurfaceCount; ++i)
    {
      if (m_distanceM[i] > 0.0)
        return true;
    }
    return false;
  }
};

// Parses a "<brouter:way>" value ("k=v k=v ..." pairs) into a RouteSurface
// bucket. Unknown is returned for empty input, beeline "direct_segment=" legs
// and unrecognized tags.
RouteSurface SurfaceFromWayTags(std::string const & wayTags);

std::string DebugPrint(RouteSurface surface);
}  // namespace routing