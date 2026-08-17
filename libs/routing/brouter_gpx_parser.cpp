#include "routing/brouter_gpx_parser.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"

#include "pugixml.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace routing
{
namespace
{
// BRouter OsmAnd format turn codes (strings in <rtept><extensions><turn>).
constexpr int kTurnC = 0;       // Continue (no turn)
constexpr int kTurnTSLL = 1;    // Turn slight left
constexpr int kTurnTSLR = 2;    // Turn slight right
constexpr int kTurnTL = 3;      // Turn left
constexpr int kTurnTRL = 4;     // Turn right (left-hand bend)
constexpr int kTurnTR = 5;      // Turn right
constexpr int kTurnTRU = 6;     // U-turn right
constexpr int kTurnTU = 7;      // U-turn
constexpr int kTurnRNDB1 = 11;  // Roundabout exit 1
constexpr int kTurnRNDB2 = 12;  // Roundabout exit 2
constexpr int kTurnRNDB3 = 13;
constexpr int kTurnRNDB4 = 14;
constexpr int kTurnRNDB5 = 15;
constexpr int kTurnRNDB6 = 16;
constexpr int kTurnRNDB7 = 17;
constexpr int kTurnRNDB8 = 18;

// Map a BRouter turn string (uppercase) to a numeric code. Returns -1
// (unknown) so callers can fall back to angle-based classification.
int ParseBrouterTurnCode(std::string const & s)
{
  if (s.empty())
    return -1;
  char const * p = s.c_str();
  // Two-character codes from the OsmAnd format.
  if (std::strcmp(p, "C") == 0)      return kTurnC;
  if (std::strcmp(p, "TSLL") == 0)   return kTurnTSLL;
  if (std::strcmp(p, "TSLR") == 0)   return kTurnTSLR;
  if (std::strcmp(p, "TL") == 0)     return kTurnTL;
  if (std::strcmp(p, "TRL") == 0)    return kTurnTRL;
  if (std::strcmp(p, "TR") == 0)     return kTurnTR;
  if (std::strcmp(p, "TRU") == 0)    return kTurnTRU;
  if (std::strcmp(p, "TU") == 0)     return kTurnTU;
  if (std::strcmp(p, "RNDB1") == 0)  return kTurnRNDB1;
  if (std::strcmp(p, "RNDB2") == 0)  return kTurnRNDB2;
  if (std::strcmp(p, "RNDB3") == 0)  return kTurnRNDB3;
  if (std::strcmp(p, "RNDB4") == 0)  return kTurnRNDB4;
  if (std::strcmp(p, "RNDB5") == 0)  return kTurnRNDB5;
  if (std::strcmp(p, "RNDB6") == 0)  return kTurnRNDB6;
  if (std::strcmp(p, "RNDB7") == 0)  return kTurnRNDB7;
  if (std::strcmp(p, "RNDB8") == 0)  return kTurnRNDB8;
  return -1;
}

// Classify a BRouter turn-angle (in degrees, -180..180) into our CarDirection.
turns::CarDirection AngleToCarDirection(double angleDeg)
{
  if (std::isnan(angleDeg))
    return turns::CarDirection::None;
  double const a = std::fabs(angleDeg);
  if (a < 5.0)
    return turns::CarDirection::GoStraight;
  if (a < 25.0)
    return angleDeg > 0 ? turns::CarDirection::TurnSlightRight : turns::CarDirection::TurnSlightLeft;
  if (a < 70.0)
    return angleDeg > 0 ? turns::CarDirection::TurnRight : turns::CarDirection::TurnLeft;
  if (a < 135.0)
    return angleDeg > 0 ? turns::CarDirection::TurnSharpRight : turns::CarDirection::TurnSharpLeft;
  return angleDeg > 0 ? turns::CarDirection::UTurnRight : turns::CarDirection::UTurnLeft;
}

double ReadDoubleAttr(pugi::xml_node const & node, char const * name, double fallback)
{
  auto attr = node.attribute(name);
  if (attr.empty())
    return fallback;
  try
  {
    return std::stod(attr.value());
  }
  catch (std::exception const &)
  {
    return fallback;
  }
}

std::string ReadStringAttr(pugi::xml_node const & node, char const * name)
{
  auto attr = node.attribute(name);
  if (attr.empty())
    return {};
  return attr.value();
}

double ReadChildDouble(pugi::xml_node const & node, char const * name, double fallback)
{
  for (pugi::xml_node child : node.children())
  {
    if (std::strcmp(child.name(), name) == 0)
    {
      try { return std::stod(child.child_value()); }
      catch (std::exception const &) { return fallback; }
    }
  }
  return fallback;
}

std::string ReadChildString(pugi::xml_node const & node, char const * name, std::string fallback)
{
  for (pugi::xml_node child : node.children())
  {
    if (std::strcmp(child.name(), name) == 0)
      return std::string(child.child_value());
  }
  return fallback;
}

void ParseRouteTurnHints(pugi::xml_node const & routeNode, std::vector<TurnHint> & hints)
{
  for (pugi::xml_node rtept : routeNode.children("rtept"))
  {
    TurnHint h;
    h.angleDeg = ReadDoubleAttr(rtept, "turn-angle", 0.0);
    h.streetName = ReadStringAttr(rtept, "street-name");
    h.ref = ReadStringAttr(rtept, "ref");
    h.destination = ReadStringAttr(rtept, "dest");
    // BRouter OsmAnd plugin nests <turn>, <turn-angle>, <time>, <offset> as
    // child elements of <rtept>/<extensions>.
    for (pugi::xml_node ext : rtept.children("extensions"))
    {
      std::string const turnStr = ReadChildString(ext, "turn", std::string{});
      if (!turnStr.empty())
        h.turnCode = ParseBrouterTurnCode(turnStr);
      h.angleDeg = ReadChildDouble(ext, "turn-angle", h.angleDeg);
      h.timeSec = ReadChildDouble(ext, "time", h.timeSec);
      h.offset = static_cast<int>(ReadChildDouble(ext, "offset", h.offset));
    }
    hints.push_back(std::move(h));
  }
}
}  // namespace

BrouterTrack ParseGpxResponse(std::string const & gpx)
{
  BrouterTrack result;

  pugi::xml_document doc;
  pugi::xml_parse_result const res = doc.load_string(gpx.c_str());
  if (!res)
  {
    LOG(LWARNING, ("BRouterRouter: failed to parse BRouter response: ", res.description()));
    return result;
  }

  pugi::xml_node const gpxNode = doc.child("gpx");
  if (!gpxNode)
    return result;

  for (pugi::xml_node trk : gpxNode.children("trk"))
  {
    for (pugi::xml_node seg : trk.children("trkseg"))
    {
      for (pugi::xml_node pt : seg.children("trkpt"))
      {
        result.points.emplace_back(mercator::FromLatLon(ReadDoubleAttr(pt, "lat", 0.0),
                                                        ReadDoubleAttr(pt, "lon", 0.0)));
        result.altitudes.push_back(static_cast<geometry::Altitude>(
            ReadChildDouble(pt, "ele", static_cast<double>(geometry::kInvalidAltitude))));
      }
    }
    break;  // single track per response
  }

  for (pugi::xml_node rte : gpxNode.children("rte"))
  {
    ParseRouteTurnHints(rte, result.hints);
    break;  // single rte per response
  }
  return result;
}

turns::CarDirection BrouterTurnToCarDirection(int code, double angleDeg)
{
  switch (code)
  {
  case kTurnTSLL: return turns::CarDirection::TurnSlightLeft;
  case kTurnTSLR: return turns::CarDirection::TurnSlightRight;
  case kTurnTL:   return turns::CarDirection::TurnLeft;
  case kTurnTRL:  return turns::CarDirection::TurnRight;
  case kTurnTR:   return turns::CarDirection::TurnRight;
  case kTurnTRU:  return turns::CarDirection::UTurnRight;
  case kTurnTU:   return turns::CarDirection::UTurnLeft;
  case kTurnRNDB1:
  case kTurnRNDB2:
  case kTurnRNDB3:
  case kTurnRNDB4:
  case kTurnRNDB5:
  case kTurnRNDB6:
  case kTurnRNDB7:
  case kTurnRNDB8: return turns::CarDirection::EnterRoundAbout;
  case kTurnC:
  default:
    // No explicit turn: fall back to the angle if the GPX provides one.
    if (angleDeg != 0.0)
      return AngleToCarDirection(angleDeg);
    return turns::CarDirection::None;
  }
}

std::vector<double> BuildCumulativeTimes(std::vector<m2::PointD> const & track,
                                         std::vector<TurnHint> const & hints)
{
  if (track.size() < 2)
    return {};
  std::vector<double> times(track.size() - 1, 0.0);

  // Build sorted list of (offset, time_delta) pairs from the rtept entries.
  // The first rtept is at offset=0 (start); subsequent rtepts are at later
  // trkpt offsets. Each <time> is the seconds spent between the previous
  // rtept and this one.
  std::vector<std::pair<int, double>> entries;
  for (auto const & h : hints)
  {
    if (h.offset >= 0)
      entries.emplace_back(h.offset, h.timeSec);
  }
  if (entries.empty())
    return times;  // no time info available; leave all 0
  std::sort(entries.begin(), entries.end());

  if (entries.back().first <= 0)
    return times;  // degenerate

  // prefix[j] = cumulative time at entries[j].offset (sum of deltas up to j).
  std::vector<double> prefix(entries.size(), 0.0);
  for (size_t j = 0; j < entries.size(); ++j)
    prefix[j] = (j == 0 ? 0.0 : prefix[j - 1]) + entries[j].second;

  for (size_t i = 0; i < track.size() - 1; ++i)
  {
    int const segEndOffset = static_cast<int>(i + 1);
    // Find the first entry whose offset > segEndOffset; (it - 1) is the
    // latest entry with offset <= segEndOffset.
    auto it = std::upper_bound(entries.begin(), entries.end(), segEndOffset,
                               [](int val, auto const & p) { return val < p.first; });
    if (it == entries.begin())
    {
      // Before the first entry (shouldn't happen for offset >= 1, but be safe).
      times[i] = 0.0;
      continue;
    }
    size_t const loIdx = static_cast<size_t>(it - entries.begin()) - 1;
    // Linear interpolation between the surrounding entries.
    if (it != entries.end())
    {
      size_t const hiIdx = static_cast<size_t>(it - entries.begin());
      int const span = entries[hiIdx].first - entries[loIdx].first;
      if (span > 0)
      {
        double const frac = static_cast<double>(segEndOffset - entries[loIdx].first) / span;
        times[i] = prefix[loIdx] + frac * (prefix[hiIdx] - prefix[loIdx]);
        continue;
      }
    }
    times[i] = prefix[loIdx];
  }
  return times;
}
}  // namespace routing