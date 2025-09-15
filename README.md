# Power Grid Simulation

A real-time power grid simulation built with OpenGL, GLFW, GLAD, Dear ImGui, and stb_image. Simulates a nuclear power plant transmitting electricity through a tower to 5 houses, with load monitoring, overload detection, manual cut, and auto-shed functionality.

## Features

- **Power Grid Elements**: 1 nuclear plant → transmission tower → 5 houses
- **House Load Simulation**: Each house has a dynamic load percentage (starts at 20-90%)
- **State Management**:
  - Normal: Powered and load < 100%
  - Overload: Load ≥ 100% (shows overload texture, power off)
  - Off: Manual cut or auto-shed (power off until repair)
- **Manual Cut**: Cuts power for 20 seconds, auto-restore
- **Auto-Shed**: Triggered at load ≥ 100%, requires repair (15 seconds)
- **Animations**: Electricity flows as moving bright quads along wires
- **GUI**: Dear ImGui interface showing house status, load, power status, timers, and controls

## Dependencies

- OpenGL 3.3+
- GLFW
- GLAD
- Dear ImGui (GLFW + OpenGL3 backend)
- stb_image.h
- C++17
- CMake

## Build Instructions

### Windows

1. **Prerequisites**: Ensure CMake, GLFW, and OpenGL are installed.

2. **Build with CMake**:
   ```bash
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```

3. **Run**:
   ```bash
   ./power_grid.exe
   ```

### Linux

1. **Install dependencies**:
   ```bash
   sudo apt-get update
   sudo apt-get install cmake libglfw3-dev libgl1-mesa-dev
   ```

2. **Build**:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

3. **Run**:
   ```bash
   ./power_grid
   ```

## Project Structure

```
ProjectRoot/
├─ images/                (textures: powerPlant.png, tower.png, house_normal.png, house_off.png, house_overload.png)
├─ include/               (stb_image.h, imgui headers, glm)
├─ src/
│   ├─ main.cpp           (main simulation code)
│   ├─ glad.c             (GLAD implementation)
│   └─ imgui*.cpp         (ImGui implementations)
├─ shaders/
│   ├─ vertex.glsl
│   └─ fragment.glsl
├─ CMakeLists.txt
└─ README.md
```

## Controls

- **Manual Cut**: Button in GUI to cut power for 20 seconds
- **Repair**: Button appears for overloaded houses, takes 15 seconds to repair
- **Close**: ESC or window close

## Simulation Details

- Houses load fluctuates randomly
- Overload triggers auto-shed: power off, shows overload texture
- Repair restores power and resets load to 30%
- Electricity animation: bright quads move from plant → tower → houses
- Wires: gray stretched quads
- All rendering in normalized device coordinates (-1 to 1)
