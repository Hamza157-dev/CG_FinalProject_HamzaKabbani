# 3D Maze Game — Computer Graphics Final Project

A first-person 3D maze game built from scratch using **C++** and **Modern OpenGL (Core Profile 3.3)**.  
No game engines were used. All rendering is implemented directly through the OpenGL pipeline.

---

## 🎮 Game Description

Navigate a 3D maze from a first-person perspective and collect all **5 gold cubes** scattered throughout the corridors.  
The game tracks your score in real time and displays a win message once all collectibles are found.

---

## 🕹️ Controls

| Key | Action |
|-----|--------|
| `W` | Move forward |
| `S` | Move backward |
| `A` | Strafe left |
| `D` | Strafe right |
| `Mouse` | Look around |
| `ESC` | Quit game |

---

## ✅ Features

- **Window & Render Loop** — GLFW window with OpenGL Core Profile context and stable game loop
- **3D Scene Geometry** — Floor and walls built using VAO/VBO/EBO with Model/View/Projection matrices
- **First-Person Camera** — Perspective camera with mouse look, pitch clamping, and deltaTime-based movement
- **Lighting** — Ambient + diffuse lighting implemented in GLSL vertex and fragment shaders
- **Collectibles** — 5 gold cubes placed throughout the maze, disappear when collected
- **Collision Detection** — AABB (Axis-Aligned Bounding Box) collision prevents walking through walls
- **Game Logic & HUD** — Score and win condition displayed in the window title bar

---

## 🛠️ Libraries Used

| Library | Purpose |
|---------|---------|
| [GLFW](https://www.glfw.org/) | Window creation and input handling |
| [GLAD](https://glad.dav1d.de/) | OpenGL function loader |
| [GLM](https://github.com/g-truc/glm) | Math library (vectors, matrices) |

---

## 🔧 Build Instructions

### Requirements
- Windows 10/11
- Visual Studio 2022 with **Desktop development with C++** workload
- [vcpkg](https://github.com/microsoft/vcpkg) package manager

### Setup

1. **Install dependencies via vcpkg:**
   ```bash
   git clone https://github.com/microsoft/vcpkg
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg install glfw3 glm glad
   .\vcpkg integrate install
   ```

2. **Clone this repository:**
   ```bash
   git clone <your-repo-url>
   cd <repo-folder>
   ```

3. **Open in Visual Studio:**
   - Open the `.sln` solution file
   - Set configuration to `Debug` or `Release`, platform to `x64`
   - Press `Ctrl+F5` to build and run

### Important Notes
- Make sure the build platform is set to **x64**
- vcpkg integration must be applied before opening the project (`vcpkg integrate install`)
- All file paths in the project are relative — no changes needed after cloning

---

## 📁 Project Structure

```
MazeGame/
├── src/
│   └── main.cpp
├── MazeGame/
│   ├── MazeGame.vcxproj
│   └── MazeGame.vcxproj.filters
├── MazeGame.sln
├── README.md
└── .gitignore
```

---

