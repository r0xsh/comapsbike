package btools.routingapp;

interface IBRouterService
{
    // param params --> Map of params:
    //   "pathToFileResult" --> String with the path to where the result must be saved,
    //                          including file name and extension. If null, the track is
    //                          passed via the return argument.
    //   "maxRunningTime"   --> String with a number of seconds for the routing timeout (default 60).
    //   "turnInstructionFormat" --> osmand | locus
    //   "trackFormat"      --> kml | gpx | json (default gpx)
    //   "acceptCompressedResult" --> "true" sends a compressed result when output format is gpx
    //   "lats"             --> double[] latitudes (2 entries minimum)
    //   "lons"             --> double[] longitudes (2 entries minimum)
    //   "nogoLats"         --> double[] nogo latitudes (may be null)
    //   "nogoLons"         --> double[] nogo longitudes (may be null)
    //   "nogoRadi"         --> double[] nogo radius in meters (may be null)
    //   "fast"             --> "0" | "1"
    //   "v"                --> motorcar | bicycle | foot
    //   "remoteProfile"    --> String with net-content of a profile. If remoteProfile != null,
    //                          v+fast are ignored.
    //   "lonlats"          --> lon,lat|... (lon,lat waypoints separated by |)
    //                          - lon,lat,d   : from this point to the next do a direct line
    //                          - lon,lat,m   : route point has no name and works as meeting point
    //                          - lon,lat,name: route point has a name and should not be ignored
    //   "straight"         --> idx1,idx2,.. (optional, indices of direct routing points in the waypoint list)
    //   "nogos"            --> lon,lat,radius,weight|... (optional)
    //   "polylines"        --> lon,lat,lon,lat,...,weight|... (optional)
    //   "polygons"         --> lon,lat,lon,lat,...,weight|... (optional)
    //   "profile"          --> profile file name without .brf
    //   "alternativeidx"   --> 0 | 1 | 2 | 3 (default 0)
    //   "exportWaypoints"  --> 1 to export them (optional)
    //   "exportCorrectedWaypoints" --> 1 to export them (optional)
    //   "pois"             --> lon,lat,name|... (optional)
    //   "extraParams"      --> Bundle key=value list for a profile setup (e.g. "profile:")
    //   "timode"           --> 0..7 turn-instruction mode (default 0)
    //   "heading"          --> angle (optional start direction)
    //   "direction"        --> angle (optional, used for recalculation / round trip)
    //   "engineMode"       --> 0 default, 2 elevation, 3 segment info, 4 round trip
    //   "roundTripDistance"--> meters (default 1500)
    //   "roundTripPoints"  --> number of used points (default 5)
    //
    // return null if all ok and no path given, the track if ok and path given, an error message if it was wrong
    //         the result as string when 'pathToFileResult' is null.
    // call in a background thread, heavy task!
    String getTrackFromParams(in Bundle params);
}
