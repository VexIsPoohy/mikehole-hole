# mikehole-hole

## Build the VST3

This project uses JUCE 8.0.6, fetched automatically by CMake. A C++17 compiler,
CMake 3.22 or newer, and the platform's JUCE GUI/audio development dependencies
are required. On Ubuntu/Debian, install the Linux dependencies with:

```sh
sudo apt-get install libasound2-dev libx11-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libxcomposite-dev libgl1-mesa-dev libgtk-3-dev \
  libfreetype-dev libfontconfig1-dev
```

Configure and build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The VST3 bundle is generated under
`build/MikeHolesHole_artefacts/Release/VST3/`.

The GitHub Actions workflow builds the Linux VST3 on pushes to `main` and can
also be started manually. To download its output, open the repository's
**Actions** tab, select **Build Linux VST3**, open a successful run, and download
the `Mike-Holes-Hole-Linux-x86_64-VST3` artifact.

The **Build Windows VST3** workflow similarly uploads a Windows x64 bundle as
`Mike-Holes-Hole-Windows-x64-VST3`.

The original source attachments did not include `Presets.h` or the Mike image
resource. `Source/Presets.h` supplies working preset definitions; the editor
continues to work without the optional image.