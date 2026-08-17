#pragma once

#include <jni.h>

#include <string>
#include <vector>

namespace jni
{
// Synchronously calls BRouterServiceClient.calculateRoutesBlocking on the
// current thread (the routing worker thread; never the UI thread, since the
// BRouter request can block for seconds). Attaches the thread to the JVM via
// ScopedEnv if needed.
//
// `maxCount` alternatives are requested from BRouter in a single bind cycle:
// the Java side fetches alternative indices 0..maxCount-1 through the already
// connected service and returns whatever succeeded. BRouter returns a GPX
// document (one <trk>, one <rte>) per alternative; when it is configured with
// acceptCompressedResult=true the payload is wrapped in an "ejY0" (base64 of
// "z64") + gzip envelope, which is decoded here before the GPX is handed to
// the router engine.
//
// Returns the decoded GPX documents in alternative order (index 0 = primary),
// or an empty vector on failure.
std::vector<std::string> BRouterCalculateRoutes(std::vector<double> const & lats, std::vector<double> const & lons,
                                                int maxCount);
}  // namespace jni