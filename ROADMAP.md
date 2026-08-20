# CoBike Roadmap

Offline-first bikepacking navigation on top of CoMaps / Organic Maps, with
[BRouter](https://github.com/abrensch/brouter) routing.

Status is tracked with checkboxes (`- [x]` done, `- [ ]` pending). Each item can
be copied 1:1 into a GitHub issue (title + first paragraph as body) and mapped
to the labels below in a GitHub Projects roadmap.

Suggested labels: `priority:high`, `priority:medium`, `priority:low`,
`size:s`, `size:m`, `size:l`, and one of `phase:1`, `phase:2`, `phase:3`.

## Phase 1 — Differentiate (quick wins, biggest value)

BRouter already returns most of the data these items need. Goal: features that
make CoBike the "gravel Komoot" of the FOSS world.

- [ ] **Surface analysis** — `size:m` `priority:high`
  Parse the `surface` / `smoothness` waytags from BRouter's GPX output and
  show a per-route breakdown (e.g. "62% paved / 30% gravel / 8% singletrack"),
  plus color the route line by surface type. This is the single feature that
  separates a bikepacking app from a cycling app.
- [ ] **Alternatives with stats** — `size:s` `priority:high`
  Routes already fetch `alternativeidx` 0..n. Show a per-alternative summary
  (distance, climbing, surface mix) so choosing one is not a gamble.
- [ ] **GPX export with turn instructions** — `size:s` `priority:high`
  Export the calculated route as GPX including BRouter's osmand turn
  instructions (`rtept`) for use on Garmin/Wahoo devices. Organic Maps only
  exports KML/KMZ today.
- [ ] **Cue sheet** — `size:s` `priority:medium`
  Plain-text / printable turn-by-turn list from the same instructions, as the
  paper-map backup every long-distance ride should have.

## Phase 2 — Bikepacking POIs (what makes it bikepacking)

OSM tags are already in the map data; the work is the "along route" query and
navigation-mode quick filters.

- [ ] **Water sources** — `size:m` `priority:high`
  `amenity=drinking_water`, `man_made=water_tap`, `natural=spring` as a
  toggleable overlay and "search along route". The #1 real-world need in the
  field.
- [ ] **Food resupply along route** — `size:m` `priority:high`
  Bakeries, groceries, supermarkets with opening hours, sorted by distance
  from the current route.
- [ ] **Shelter and bivouac spots** — `size:m` `priority:high`
  `tourism=wilderness_hut`, `amenity=shelter`, `tourism=camp_site` (bothies
  included) — critical information when weather turns bad.
- [ ] **Bail-out options** — `size:s` `priority:medium`
  Train stations near the route for emergencies and bike-train combos.
- [ ] **Bike support** — `size:s` `priority:low`
  Bike shops, repair stations and public pumps along the route.

## Phase 3 — Bigger bets

- [ ] **BRouter profile picker** — `size:l` `priority:medium`
  The client currently hardcodes `v=bicycle, fast=1`. Expose profile selection
  (trekking vs fastbike) and key parameters (`iswet`, avoid-steep, etc.) in the
  routing settings.
- [ ] **Follow-GPX-track mode** — `size:l` `priority:medium`
  Navigate along an imported track without re-routing it, for riders who
  strictly follow a planned route (BRouter can snap to the track).
- [ ] **Sunrise / sunset per stage** — `size:s` `priority:low`
  Fully offline-calculable; helps plan realistic daily stages.
- [ ] **Battery saver navigation mode** — `size:m` `priority:medium`
  Auto dim / screen-off between turns during navigation. Battery life is a
  safety concern, not a convenience, on multi-day trips.

## Non-goals (for now)

- Weather / wind forecasts (needs network; conflicts with offline-first)
- Community route library / heatmaps (needs a backend)
- Multi-device sync and co-owned collections (needs accounts)
