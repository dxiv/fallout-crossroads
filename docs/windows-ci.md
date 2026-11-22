# Windows build

## Prerequisites (VS 2022 Build Tools)

- Workload: **Desktop development with C++** / `Microsoft.VisualStudio.Workload.VCTools`
- **Windows 10/11 SDK** (so `Windows.h` and `rc.exe` resolve)
- CMake on PATH

## Recommended: Developer Command Prompt + NMake

```bat
cd C:\Users\...\fallout-crossroads
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
build\FalloutCrossroads.exe
```

## Alternate: Visual Studio generator

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
build\Release\FalloutCrossroads.exe
```

Outputs: `FalloutCrossroads.exe` and `CrossroadsPlugin.dll`.
