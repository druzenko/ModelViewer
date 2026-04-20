# ModelViewer

A Direct3D 12 model viewer for Windows.

## Requirements

- **Windows 10 or later**
- **Visual Studio 2019 or later** (MSVC toolchain)
- **Windows SDK** (installed via Visual Studio Installer)
- **CMake 3.23 or later**
- **Git** (required by CMake to fetch dependencies)

> This project targets Windows only and is not intended to be cross-platform.

## Building

Dependencies are fetched automatically by CMake via `FetchContent`:
- [DirectXTex](https://github.com/microsoft/DirectXTex)
- [DirectX-Headers](https://github.com/microsoft/DirectX-Headers)
- [Assimp](https://github.com/assimp/assimp) (OBJ importer)
- [ImGui](https://github.com/ocornut/imgui)
- [WinPixEventRuntime](https://www.nuget.org/packages/WinPixEventRuntime)

### Generate the Visual Studio solution

```bat
git clone https://github.com/druzenko/ModelViewer.git
cd ModelViewer
cmake -B build
```

Then open `build/ModelViewer.sln` in Visual Studio and build the `model_viewer` project.

### Build configurations

| Configuration | Description |
|---|---|
| `Debug` | Includes PIX event runtime for GPU debugging |
| `Release` | Optimized build |
