# Aether

 Minimal C++20 3D engine prototype (GLFW + OpenGL + ImGui + ImGuizmo).

## Repo Layout

- `Engine/`: static library target (`Aether::Engine`)
- `Sandbox/`: editor executable target (`Sandbox`)
- `exe/`: post-build copied executable (`Sandbox.exe` / `Sandbox`)
- `build/`: build artifacts (CMake out-of-source)

## Build

### Windows (Visual Studio / MSVC)

Run from a Visual Studio Developer Command Prompt (so `cl` is available).

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Linux (gcc/clang)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run

Windows (multi-config generator):

```powershell
.\build\bin\Release\Sandbox.exe
```

Linux (single-config):

```bash
./build/bin/Sandbox
```

### Automated Runs

```powershell
.\build\bin\Release\Sandbox.exe --auto-exit=1
.\build\bin\Release\Sandbox.exe --auto-exit=10
```

## Controls

- Viewport focus: hover viewport and hold `RMB`
- Camera move: `W/A/S/D`, `Space` up, `Ctrl` down, `Shift` faster
- Select: `LMB` in viewport (ray-pick)
- Gizmo mode: `W` translate, `E` rotate, `R` scale
- Logo: `View -> Show Logo`

## Notes / TODO

- ImGui docking is enabled (using the `imgui` docking branch) to keep the editor layout consistent.
- Basic ECS is minimal (single mesh type: cube).
- Add scene save/load and additional primitives.

## Changelog (Prototype)

- Phase 0: CMake + FetchContent deps, `Aether::Engine`, `Sandbox`, post-build copy to `exe/`.
- Phase 1: GLFW window wrapper + app loop + `--auto-exit`/`--windowed`.
- Phase 2: GLAD init + simple renderer + shader + cube draw.
- Phase 3: Framebuffer rendering + ImGui viewport.
- Phase 4: FPS camera + input capture (viewport + RMB).
- Phase 5: Minimal ECS + ray-picking + Hierarchy/Inspector/Console.
- Phase 6: ImGuizmo translate/rotate/scale.
- Phase 7: Selected wireframe overlay + per-frame clear (no trails).
- Phase 8: Embedded logo overlay + View menu toggle.
