#include "routing/router.hpp"

#include "routing/route.hpp"

#include "base/assert.hpp"

#include <utility>
#include <vector>

namespace routing
{
std::string ToString(RouterType type)
{
  switch (type)
  {
  case RouterType::Vehicle: return "vehicle";
  case RouterType::Pedestrian: return "pedestrian";
  case RouterType::Bicycle: return "bicycle";
  case RouterType::Transit: return "transit";
  case RouterType::Ruler: return "ruler";
  case RouterType::BRouter: return "brouter";
  case RouterType::Count: return "count";
  }
  ASSERT(false, ());
  return "Error";
}

RouterType FromString(std::string const & str)
{
  if (str == "vehicle")
    return RouterType::Vehicle;
  if (str == "pedestrian")
    return RouterType::Pedestrian;
  if (str == "bicycle")
    return RouterType::Bicycle;
  if (str == "transit")
    return RouterType::Transit;
  if (str == "ruler")
    return RouterType::Ruler;
  if (str == "brouter")
    return RouterType::BRouter;

  ASSERT(false, ("Incorrect routing string:", str));
  return RouterType::Vehicle;
}

std::string DebugPrint(RouterType type)
{
  return ToString(type);
}

std::vector<Route> IRouter::CalculateRoutes(Checkpoints const & checkpoints, m2::PointD const & startDirection,
                                            bool adjust, RouterDelegate const & delegate)
{
  std::vector<Route> routes;
  routes.emplace_back(GetName(), 0 /* route id */);
  auto const code = CalculateRoute(checkpoints, startDirection, adjust, delegate, routes.back());
  if (code != RouterResultCode::NoError)
    routes.clear();
  return routes;
}
}  //  namespace routing
