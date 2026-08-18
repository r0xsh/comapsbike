# CoBike

A fork of [CoMaps](https://github.com/comaps/comaps) (itself a fork of Organic Maps), made mainly for my own use.

## Why this fork

The main goal is to add support for the [BRouter](https://github.com/abrensch/brouter) routing algorithm, since I really don't like the routing provided by CoMaps. I needed a fully offline solution that covers my needs.

CoMaps has stated that implementing BRouter is [not planned](https://codeberg.org/comaps/comaps/issues/1244), which is why this fork exists.

I also loved how the [Geovelo](https://www.geovelo.fr/) app offers multiple route alternatives. Sadly, Geovelo is not fully offline and can sometimes provide weird, unsafe paths.

## Demo

<video src="https://github.com/user-attachments/assets/dc448887-67b9-46f4-a8dc-66ff8268c6a7" width="300" controls muted loop></video>

## Contributing

- Issues are open for ideas and bugs.
- All pull requests unrelated to my changes should be sent to the [CoMaps upstream](https://github.com/comaps/comaps) instead.

## Building

See [docs/INSTALL.md](docs/INSTALL.md) for build instructions. To build the Android F-Droid release APK:

```bash
cd android
./gradlew assembleFdroidRelease
```

## License

Licensed under the Apache License 2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE).
