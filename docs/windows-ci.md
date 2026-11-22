# Windows CI

GitHub Actions for this repo is optional. Until a workflow is added under
`.github/workflows/`, build locally on Windows.

## Local build (Windows 10/11 + VS 2022)

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
```

Output: `build\Release\FalloutCrossroads.exe`
