#include "routing/brouter_gpx_parser.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"

#include "pugixml.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace routing
{
namespace
{
// BRouter turn codes (strings in <brouter:voicehint>, identical to the
// OsmAnd format command strings).
constexpr int kTurnC = 0;       // Continue (no turn)
constexpr int kTurnTSLL = 1;    // Turn slight left
constexpr int kTurnTSLR = 2;    // Turn slight right
constexpr int kTurnTL = 3;      // Turn left
constexpr int kTurnTR = 4;      // Turn right
constexpr int kTurnTRU = 5;     // U-turn right
constexpr int kTurnTU = 6;      // U-turn (180 degrees)
constexpr int kTurnTSHL = 7;    // Turn sharply left
constexpr int kTurnTSHR = 8;    // Turn sharply right
constexpr int kTurnKL = 9;      // Keep left
constexpr int kTurnKR = 10;     // Keep right
constexpr int kTurnRNDB1 = 11;  // Roundabout exit 1
constexpr int kTurnRNDB2 = 12;  // Roundabout exit 2
constexpr int kTurnRNDB3 = 13;
constexpr int kTurnRNDB4 = 14;
constexpr int kTurnRNDB5 = 15;
constexpr int kTurnRNDB6 = 16;
constexpr int kTurnRNDB7 = 17;
constexpr int kTurnRNDB8 = 18;
constexpr int kTurnRNLB1 = 21;  // Roundabout (left turn) exit 1
constexpr int kTurnRNLB2 = 22;
constexpr int kTurnRNLB3 = 23;
constexpr int kTurnRNLB4 = 24;
constexpr int kTurnRNLB5 = 25;
constexpr int kTurnRNLB6 = 26;
constexpr int kTurnRNLB7 = 27;
constexpr int kTurnRNLB8 = 28;
constexpr int kTurnEL = 29;     // Exit left (motorway exit)
constexpr int kTurnER = 30;     // Exit right (motorway exit)

// Map a BRouter turn string (uppercase) to a numeric code. Returns -1
// (unknown) so callers can fall back to angle-based classification.
int ParseBrouterTurnCode(std::string const & s)
{
  if (s.empty())
    return -1;
  char const * p = s.c_str();
  if (std::strcmp(p, "C") == 0)      return kTurnC;
  if (std::strcmp(p, "TSLL") == 0)   return kTurnTSLL;
  if (std::strcmp(p, "TSLR") == 0)   return kTurnTSLR;
  if (std::strcmp(p, "TL") == 0)     return kTurnTL;
  if (std::strcmp(p, "TR") == 0)     return kTurnTR;
  if (std::strcmp(p, "TRU") == 0)    return kTurnTRU;
  if (std::strcmp(p, "TU") == 0)     return kTurnTU;
  if (std::strcmp(p, "TSHL") == 0)   return kTurnTSHL;
  if (std::strcmp(p, "TSHR") == 0)   return kTurnTSHR;
  if (std::strcmp(p, "KL") == 0)     return kTurnKL;
  if (std::strcmp(p, "KR") == 0)     return kTurnKR;
  if (std::strcmp(p, "EL") == 0)     return kTurnEL;
  if (std::strcmp(p, "ER") == 0)     return kTurnER;
  if (std::strcmp(p, "RNDB1") == 0)  return kTurnRNDB1;
  if (std::strcmp(p, "RNDB2") == 0)  return kTurnRNDB2;
  if (std::strcmp(p, "RNDB3") == 0)  return kTurnRNDB3;
  if (std::strcmp(p, "RNDB4") == 0)  return kTurnRNDB4;
  if (std::strcmp(p, "RNDB5") == 0)  return kTurnRNDB5;
  if (std::strcmp(p, "RNDB6") == 0)  return kTurnRNDB6;
  if (std::strcmp(p, "RNDB7") == 0)  return kTurnRNDB7;
  if (std::strcmp(p, "RNDB8") == 0)  return kTurnRNDB8;
  if (std::strcmp(p, "RNLB1") == 0)  return kTurnRNLB1;
  if (std::strcmp(p, "RNLB2") == 0)  return kTurnRNLB2;
  if (std::strcmp(p, "RNLB3") == 0)  return kTurnRNLB3;
  if (std::strcmp(p, "RNLB4") == 0)  return kTurnRNLB4;
  if (std::strcmp(p, "RNLB5") == 0)  return kTurnRNLB5;
  if (std::strcmp(p, "RNLB6") == 0)  return kTurnRNLB6;
  if (std::strcmp(p, "RNLB7") == 0)  return kTurnRNLB7;
  if (std::strcmp(p, "RNLB8") == 0)  return kTurnRNLB8;
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

// Parse a BRouter <time> stamp ("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", UTC) into
// epoch seconds. Returns 0 on malformed input.
double ParseGpxTimeEpoch(std::string const & s)
{
  int y = 0, mo = 0, d = 0, h = 0, mi = 0, se = 0;
  if (std::sscanf(s.c_str(), "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &se) != 6)
    return 0.0;
  // Days from civil date (Howard Hinnant's algorithm, proleptic Gregorian).
  y -= mo <= 2;
  int64_t const era = (y >= 0 ? y : y - 399) / 400;
  unsigned const yoe = static_cast<unsigned>(y - era * 400);
  unsigned const doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  unsigned const doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  int64_t const days = era * 146097 + static_cast<int64_t>(doe) - 719468;
  return static_cast<double>(days) * 86400.0 + h * 3600.0 + mi * 60.0 + se;
}

// Parse the total route time out of a <brouter:info> metadata string like
// "track-length = 17975 ... energy=.1kwh time=58m 17s" (see BRouter's
// Formatter.getFormattedTime2: "Xh Ym Zs" with optional hours/seconds).
// Returns 0 when absent or malformed.
double ParseInfoTime(std::string const & info)
{
  size_t const pos = info.find("time=");
  if (pos == std::string::npos)
    return 0.0;
  std::string const rest = info.substr(pos + 5);
  double total = 0.0;
  size_t i = 0;
  while (i < rest.size())
  {
    while (i < rest.size() && rest[i] == ' ')
      ++i;
    size_t j = i;
    while (j < rest.size() && std::isdigit(static_cast<unsigned char>(rest[j])))
      ++j;
    if (j == i)
      break;
    double const value = std::strtod(rest.c_str() + i, nullptr);
    while (j < rest.size() && rest[j] == ' ')
      ++j;
    if (j < rest.size() && rest[j] == 'h')
      total += value * 3600.0;
    else if (j < rest.size() && rest[j] == 'm')
      total += value * 60.0;
    else if (j < rest.size() && rest[j] == 's')
      total += value;
    else
      break;
    i = j + 1;
  }
  return total;
}

// Parse the per-trackpoint <extensions> block (BRouter mode 9). Fills the
// way tags (run-length decoded via |lastWay|), speed and the voice hint.
void ParsePointExtensions(pugi::xml_node const & point, size_t pointIdx, std::string & lastWay,
                          BrouterTrack & track)
{
  double speed = 0.0;
  double timeEpoch = 0.0;
  std::string wayTags;
  std::string voiceHint;

  for (pugi::xml_node child : point.children())
  {
    if (std::strcmp(child.name(), "ele") == 0)
      continue;
    if (std::strcmp(child.name(), "time") == 0)
      timeEpoch = ParseGpxTimeEpoch(child.child_value());
    else if (std::strcmp(child.name(), "extensions") == 0)
    {
      speed = ReadChildDouble(child, "brouter:speed", 0.0);
      wayTags = ReadChildString(child, "brouter:way", std::string{});
      voiceHint = ReadChildString(child, "brouter:voicehint", std::string{});
    }
  }

  if (!wayTags.empty())
    lastWay = wayTags;
  track.wayTagsPerPoint.push_back(lastWay);
  track.speedKphPerPoint.push_back(speed);
  track.timeEpochPerPoint.push_back(timeEpoch);

  if (voiceHint.empty())
    return;
  // "cmd;distanceToNext,geometry" (geometry is a list of coordinates).
  size_t const semi = voiceHint.find(';');
  std::string const cmd = semi == std::string::npos ? voiceHint : voiceHint.substr(0, semi);
  TurnHint h;
  h.turnCode = ParseBrouterTurnCode(cmd);
  h.offset = static_cast<int>(pointIdx);
  if (semi != std::string::npos)
  {
    std::string const rest = voiceHint.substr(semi + 1);
    size_t const comma = rest.find(',');
    std::string const distStr = comma == std::string::npos ? rest : rest.substr(0, comma);
    try { h.distanceToNextM = std::stod(distStr); }
    catch (std::exception const &) {}
  }
  track.hints.push_back(std::move(h));
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

  // <metadata><extensions><brouter:info> carries the total route time. Note
  // that BRouter writes the same info block on every alternative, so this is
  // only exact for the primary route (per-alternative times come from the
  // injected profile:showspeed speeds instead).
  if (pugi::xml_node const meta = gpxNode.child("metadata"))
  {
    if (pugi::xml_node const metaExt = meta.child("extensions"))
      result.totalTimeSec = ParseInfoTime(ReadChildString(metaExt, "brouter:info", std::string{}));
  }

  std::string lastWay;
  for (pugi::xml_node trk : gpxNode.children("trk"))
  {
    size_t pointIdx = 0;
    for (pugi::xml_node seg : trk.children("trkseg"))
    {
      for (pugi::xml_node pt : seg.children("trkpt"))
      {
        result.points.emplace_back(mercator::FromLatLon(ReadDoubleAttr(pt, "lat", 0.0),
                                                        ReadDoubleAttr(pt, "lon", 0.0)));
        result.altitudes.push_back(static_cast<geometry::Altitude>(
            ReadChildDouble(pt, "ele", static_cast<double>(geometry::kInvalidAltitude))));
        ParsePointExtensions(pt, pointIdx, lastWay, result);
        ++pointIdx;
      }
    }
    break;  // single track per response
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
  case kTurnTR:   return turns::CarDirection::TurnRight;
  case kTurnTSHL: return turns::CarDirection::TurnSharpLeft;
  case kTurnTSHR: return turns::CarDirection::TurnSharpRight;
  case kTurnKL:   return turns::CarDirection::None;
  case kTurnKR:   return turns::CarDirection::None;
  case kTurnTRU:  return turns::CarDirection::UTurnRight;
  case kTurnTU:   return turns::CarDirection::UTurnLeft;
  case kTurnEL:   return turns::CarDirection::ExitHighwayToLeft;
  case kTurnER:   return turns::CarDirection::ExitHighwayToRight;
  case kTurnRNDB1:
  case kTurnRNDB2:
  case kTurnRNDB3:
  case kTurnRNDB4:
  case kTurnRNDB5:
  case kTurnRNDB6:
  case kTurnRNDB7:
  case kTurnRNDB8:
  case kTurnRNLB1:
  case kTurnRNLB2:
  case kTurnRNLB3:
  case kTurnRNLB4:
  case kTurnRNLB5:
  case kTurnRNLB6:
  case kTurnRNLB7:
  case kTurnRNLB8: return turns::CarDirection::LeaveRoundAbout;
  case kTurnC:
  default:
    // No explicit turn: fall back to the angle if the GPX provides one.
    if (angleDeg != 0.0)
      return AngleToCarDirection(angleDeg);
    return turns::CarDirection::None;
  }
}

// BRouter converts "keep left/right" turn hints to LeaveRoundAbout arrows on
// maps, which is visually misleading on a straight continue. Drop them so the
// line has no arrow on keep-lane segments — the standard bicycle router does
// not produce a turn on keep-lane segments either.
uint32_t BrouterTurnExitNumber(int code)
{
  if (code >= kTurnRNDB1 && code <= kTurnRNDB8)
    return static_cast<uint32_t>(code - kTurnRNDB1 + 1);
  if (code >= kTurnRNLB1 && code <= kTurnRNLB8)
    return static_cast<uint32_t>(code - kTurnRNLB1 + 1);
  return 0;
}

std::vector<double> BuildCumulativeTimes(BrouterTrack const & track)
{
  auto const & points = track.points;
  if (points.size() < 2)
    return {};
  size_t const segCount = points.size() - 1;
  std::vector<double> times(segCount, 0.0);

  bool const hasSpeed = std::any_of(track.speedKphPerPoint.begin(), track.speedKphPerPoint.end(),
                                    [](double s) { return s > 0.0; });
  bool const hasTime = std::any_of(track.timeEpochPerPoint.begin(), track.timeEpochPerPoint.end(),
                                   [](double t) { return t > 0.0; });
  if (!hasSpeed && !hasTime && track.totalTimeSec <= 0.0)
    return times;

  std::vector<double> segDists(segCount);
  double totalDist = 0.0;
  for (size_t i = 0; i < segCount; ++i)
  {
    segDists[i] = mercator::DistanceOnEarth(points[i], points[i + 1]);
    totalDist += segDists[i];
  }

  auto const speedAt = [&track](size_t idx) {
    if (idx < track.speedKphPerPoint.size() && track.speedKphPerPoint[idx] > 0.0)
      return track.speedKphPerPoint[idx];
    if (idx > 0 && idx - 1 < track.speedKphPerPoint.size() && track.speedKphPerPoint[idx - 1] > 0.0)
      return track.speedKphPerPoint[idx - 1];
    return 0.0;
  };

  double accumulated = 0.0;
  double travelled = 0.0;
  for (size_t i = 0; i < segCount; ++i)
  {
    double segSec = 0.0;
    if (hasSpeed)
    {
      // Speed is reported per point for the segment arriving at that point.
      double const speedKph = speedAt(i + 1);
      if (speedKph > 0.0)
        segSec = segDists[i] / (speedKph * 1000.0 / 3600.0);
    }
    if (segSec == 0.0 && hasTime)
    {
      double const dt = track.timeEpochPerPoint[i + 1] - track.timeEpochPerPoint[i];
      if (dt > 0.0)
        segSec = dt;
    }
    if (segSec == 0.0 && track.totalTimeSec > 0.0 && totalDist > 0.0)
    {
      // Fallback when no per-point speeds or time stamps arrived (e.g. a
      // BRouter companion that ignores the injected profile:showspeed
      // variable): the metadata total is shared by all alternatives, so this
      // is only exact for the primary route; distribute it proportionally.
      travelled += segDists[i];
      segSec = track.totalTimeSec * travelled / totalDist - accumulated;
    }
    accumulated += segSec;
    times[i] = accumulated;
  }
  return times;
}
}  // namespace routing