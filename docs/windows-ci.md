# Windows CI

A GitHub Actions workflow (`.github/workflows/windows-build.yml`) is ready to add once the
pushing credential has the `workflow` scope. Until then, build locally on Windows:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
```
