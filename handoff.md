# RFSim: Handoff & Architectural Source of Truth
**Version:** 1.1.1  
**Stack:** C++20, Raylib 5.5, Dear ImGui (Docking), libcurl  
**Lead Architect:** Gemini CLI

---

## 1. System Architecture & Data-Flow Map

### Asynchronous Data Pipeline & Optimized Cache
The application operates on a non-blocking, decoupled architecture to ensure 60 FPS rendering regardless of network latency or physics complexity.

1.  **Optimized Tile Cache System**:
    - **Bit-Packed Keys**: Replaced string-based lookups with a `uint64_t` key `[16 bits Z | 24 bits X | 24 bits Y]` for O(1) performance.
    - **LRU Management**: Implements a Least Recently Used (LRU) pruning strategy with a **1024-tile capacity** to maintain a smooth experience during rapid zooming.
    - **In-Flight Tracking**: Uses a `m_pendingRequests` vector to prevent redundant network threads for the same geographic tile.
2.  **libcurl Web Request Thread**:
    - Identifying missing tiles triggers detached `std::thread` execution of `FetchTileAsync`.
    - Retrieves PNG bytes from `tile.openstreetmap.org` using standard OSM user-agent headers.
3.  **Raylib Texture Binding**:
    - Main Thread (OpenGL Context) detects `!tile->loaded && tile->filePath.size() > 0`.
    - Main thread executes `LoadTextureFromImage` and clears the temporary byte buffer.
4.  **Display Mode Dispatcher**:
    - **Map2D**: Standard Web Mercator flat map with sparse heatmap overlay.
    - **Surface3D**: Local 3D terrain plot with vertical exaggeration and double-sided RF radiation lobes.
    - **Globe3D**: A full planetary perspective using a spherical Earth model and Great Circle multi-hop propagation.

### Volumetric Global Coverage
- For HF DX (3-30 MHz), the `Globe3D` mode implements a **Volumetric Coverage Map**.
- It samples multiple azimuths based on the transmitter's **Antenna Beamwidth** (e.g., 360° for Isotropic, 30° for Yagi).
- It calculates multiple **Takeoff Angles** (10° - 35°) to visualize skip zones and return footprints.

---

## 2. Ionospheric & Propagation State Engine Schema

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "RFParametersContext",
  "type": "object",
  "properties": {
    "operation": {
      "type": "object",
      "properties": {
        "frequency_mhz": { "type": "number", "minimum": 0.1, "maximum": 6000.0 },
        "polarization": { "enum": [0, 1] },
        "display_mode": { "enum": [0, 1, 2] } 
      }
    }
  }
}
```
*(Enum Mapping: 0=Map2D, 1=Surface3D, 2=Globe3D)*

---

## 3. Physics & Math Reference Contracts

### A. Planetary Geometry
- **Lat/Lon to Cartesian**: $X = R \cos(lat) \sin(lon)$, $Y = R \sin(lat)$, $Z = R \cos(lat) \cos(lon)$.
- **Web Mercator Sphere Mapping**: Latitude is mapped to texture V-coordinates using $V = (1 - \ln(\tan(\phi) + \sec(\phi)) / \pi) / 2$ to eliminate polar distortion.

### B. HF Skywave & Multi-Hop Multi-Path
- **Virtual Height**: F2 layer modeled at **300 km** virtual height.
- **Great Circle Reflection**: Arcs are interpolated using spherical rotation matrices.
- **Multi-Hop Reflection**: 
  - **Hop 1 (Green)**: Tx -> Ionosphere -> Rx(1)
  - **Hop 2 (Gold)**: Rx(1) -> Ionosphere -> Rx(2)
  - **Hop 3 (Red)**: Rx(2) -> Ionosphere -> Rx(3)
- **Central Angle of Hop**: $\beta = 2 \cdot (\arccos(\frac{R_e \cos(\alpha)}{R_e + h_{F2}}) - \alpha)$, where $\alpha$ is takeoff angle.

---

## 4. Module API Contracts & Interfaces

### `MapEngine` Class
- **`DrawGlobe(params)`**: Renders the 3D planetary model, Great Circle arcs, and TX position pins.
- **`GetTileKey(x, y, z)`**: Static utility. Bit-packs tile coordinates into a 64-bit lookup key.
- **`PruneCache()`**: Thread-safe LRU cleanup routine.

### `PropagationEngine` Class
- **`CalculateReceivedPower(params, lat, lon)`**: Analytical solver for path loss, return power, and skip-zone detection.

---

## 5. Cross-Platform Build & Verification Topology

### Prerequisites
- **Linux**: `sudo apt install build-essential cmake libcurl4-openssl-dev libx11-dev libxcursor-dev libxinerama-dev libxrandr-dev libxi-dev libasound2-dev libmesa-dev`

### Binary Distribution
- **Linux Binary**: Located at **`dist/RFSim.bin`**.
- **Launch Command**: `./dist/RFSim.bin` (Ensure execute permissions: `chmod +x dist/RFSim.bin`).

### Verification Commands (Diagnostics)
- **Verify 3D Globe Projection**: Ensure Prime Meridian (Lon 0) aligns with X=0 in `Globe3D` mode.
- **Verify Cache Capacity**: Monitor memory usage; it should plateau after ~1024 tiles are cached.
- **Verify Pole Safety**: Place TX pin at +89.0 Latitude and ensure arcs do not cause NaNs or vertex explosions.
