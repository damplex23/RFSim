# RFSim: Handoff & Architectural Source of Truth
**Version:** 1.2.0  
**Stack:** C++20, Raylib 5.5, Dear ImGui (Docking), libcurl  
**Lead Architect:** Gemini CLI

---

## 1. System Architecture & Data-Flow Map

### Asynchronous Data Pipeline & Optimized Cache
The application operates on a non-blocking, decoupled architecture to ensure 60 FPS rendering regardless of network latency or physics complexity.

1.  **Optimized Tile Cache System**:
    - **Bit-Packed Keys**: Replaced string-based lookups with a `uint64_t` key `[16 bits Z | 24 bits X | 24 bits Y]` for O(1) performance.
    - **LRU Management**: Implements a Least Recently Used (LRU) pruning strategy with a **1024-tile capacity**.
    - **In-Flight Tracking**: Uses a `m_pendingRequests` vector to prevent redundant network threads.
2.  **libcurl Web Request Thread**:
    - Detached `std::thread` execution of `FetchTileAsync`.
    - Retrieves PNG bytes from OSM using `libcurl` (Built from source via FetchContent).
3.  **Raylib Texture Binding**:
    - Main thread executes `LoadTextureFromImage` and manages GPU texture lifecycle.
4.  **Display Mode Dispatcher**:
    - **Map2D**: Standard Web Mercator flat map with high-performance 2D heatmap overlay.
    - **Surface3D**: Local 3D terrain plot with high-resolution (80x80) heatmap grid, wireframe depth perception, and transparent radiation lobes.
    - **Globe3D**: A full planetary perspective using a spherical Earth model and Great Circle multi-hop propagation.

---

## 2. Build System & Dependency Management

- **CMake 3.20+**: Primary build system.
- **FetchContent Integration**: All dependencies (Raylib, ImGui, rlImGui, and **libcurl**) are fetched and built from source to ensure architecture-specific compatibility.
- **Cross-Compilation**: Support for building Windows binaries from Linux using `x86_64-w64-mingw32-gcc`.
- **Static Linking**: Windows builds are statically linked (`-static-libgcc -static-libstdc++`) for high portability.

---

## 3. Physics & Math Reference Contracts

### A. Planetary Geometry
- **Lat/Lon to Cartesian**: $X = R \cos(lat) \sin(lon)$, $Y = R \sin(lat)$, $Z = R \cos(lat) \cos(lon)$.
- **Web Mercator Sphere Mapping**: Latitude is mapped to texture V-coordinates using $V = (1 - \ln(\tan(\phi) + \sec(\phi)) / \pi) / 2$.

### B. HF Skywave & Multi-Hop Multi-Path
- **Virtual Height**: F2 layer modeled at **300 km**.
- **Multi-Hop Reflection**: 
  - **Hop 1 (Green)**, **Hop 2 (Gold)**, **Hop 3 (Red)**.
- **Central Angle of Hop**: $\beta = 2 \cdot (\arccos(\frac{R_e \cos(\alpha)}{R_e + h_{F2}}) - \alpha)$.

---

## 4. Troubleshooting & Known Conflicts

- **Windows Header Conflicts**: `NOGDI` is defined project-wide to resolve `Rectangle` naming collisions with Raylib.
- **libcurl/Windows Interaction**: Header overrides in `MapEngine.cpp` prevent conflicts with `CloseWindow` and `ShowCursor`.

---

## 5. Cross-Platform Build & Verification Topology

### Binaries
- **Linux Binary**: `dist/RFSim`
- **Windows Binary**: `dist/RFSim.exe` (Portable, statically linked)

### Verification Commands
- **Verify 3D Globe Projection**: Ensure Prime Meridian (Lon 0) aligns with X=0 in `Globe3D` mode.
- **Verify Cache Capacity**: Monitor memory usage; it should plateau after ~1024 tiles are cached.
- **Verify Pole Safety**: Place TX pin at +89.0 Latitude and ensure arcs do not cause NaNs or vertex explosions.
