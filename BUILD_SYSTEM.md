# CI/CD and Build System Documentation

This project uses CMake for cross-platform builds and GitHub Actions for continuous integration and delivery.

## Supported Platforms

- **Windows**: Built using MSVC (Visual Studio 2022) on `windows-latest`.
- **Linux**: Built using GCC on `ubuntu-latest`.
- **Android**: Cross-compiled using the Android NDK (r25b) for multiple ABIs:
  - `armeabi-v7a`
  - `arm64-v8a`
  - `x86`
  - `x86_64`

## Local Build Instructions

### Windows
```powershell
cmake -B build -A x64
cmake --build build --config Release
```

### Linux
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Android (Cross-compilation)
Requires Android NDK.
```bash
cmake -B build_android \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-21 \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build_android
```

## GitHub Actions Workflow

The workflow is defined in [build.yml](.github/workflows/build.yml).

- **On Push/PR**: Automatically builds all platforms to ensure code quality.
- **On Release**: Builds all platforms and automatically attaches the binaries to the GitHub Release.

### Artifacts
- `pylua-windows`: Windows executable and static library.
- `pylua-linux`: Linux executable and static library.
- `pylua-android-<abi>`: Android binaries for specified ABI.
