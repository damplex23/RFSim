# 📡 RFSim: High-Fidelity Radio Propagation Simulator

![C++ Version](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Graphics](https://img.shields.io/badge/Graphics-Raylib_5.5-orange.svg)
![UI](https://img.shields.io/badge/UI-Dear_ImGui_Docking-purple.svg)
![License](https://img.shields.io/badge/License-Non--Commercial-red.svg)

**RFSim** is a high-performance, real-time radio frequency propagation simulator designed to visualize HF/VHF/UHF signal coverage across a global scale. Combining modern C++20, advanced planetary geometry, and a decoupled asynchronous data pipeline, RFSim provides a bridge between complex RF physics and intuitive 3D visualization.

---

## 🚀 Core Features

### 🌍 Triple-View Visualization Engine
Seamlessly switch between different perspectives to analyze propagation across various scales:
*   **2D Map Projection**: Traditional Web Mercator flat map with sparse real-time heatmap overlays.
*   **3D Surface Plot**: Local terrain visualization with vertical exaggeration and 3D radiation patterns.
*   **3D Global Globe**: A full planetary model using spherical Earth geometry, perfect for visualizing HF Skywave and Great Circle multi-hop propagation.

### ⚡ Optimized Asynchronous Tile Engine
*   **Bit-Packed Tile Cache**: O(1) tile lookup using 64-bit keys `[16-bit Z | 24-bit X | 24-bit Y]`.
*   **LRU Pruning**: Maintains a smooth 60 FPS experience by caching up to 1024 tiles with smart memory management.
*   **Non-Blocking Network I/O**: Multi-threaded `libcurl` integration for background tile fetching from OpenStreetMap servers.

### 📡 Physics-Grounded Propagation
*   **HF Skywave Model**: Simulates Ionospheric reflections (F2 layer at ~300km) with multi-hop support.
*   **Volumetric Coverage**: Samples multiple azimuths and takeoff angles (10° - 35°) to detect skip zones and signal footprints.
*   **Dynamic Heatmaps**: Visualizes signal strength (dBm) with discrete color bands representing reliability levels (Weak, Marginal, Reliable, Strong).

---

## 🛠 Tech Stack

*   **Language**: C++20
*   **Graphics**: [Raylib 5.5](https://www.raylib.com/)
*   **UI Framework**: [Dear ImGui](https://github.com/ocornut/imgui) (Docking Branch)
*   **Backend**: `rlImGui` (Raylib-ImGui integration)
*   **Networking**: `libcurl` for asynchronous GIS data fetching.
*   **Build System**: CMake 3.20+

---

## 📦 Installation & Build

### Prerequisites

#### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential cmake libcurl4-openssl-dev libx11-dev \
                 libxcursor-dev libxinerama-dev libxrandr-dev libxi-dev \
                 libasound2-dev libmesa-dev
```

### Compiling from Source

1.  **Clone the repository**:
    ```bash
    git clone https://github.com/yourusername/RFSim.git
    cd RFSim
    ```

2.  **Generate Build Files**:
    ```bash
    mkdir build && cd build
    cmake ..
    ```

3.  **Build**:
    ```bash
    make -j$(nproc)
    ```

4.  **Run**:
    ```bash
    ./bin/RFSim
    ```

---

## 📐 Mathematical Foundation

### Planetary Geometry
The globe is modeled as a sphere where geographic coordinates (Lat/Lon) are transformed into 3D Cartesian space:
*   $X = R \cos(lat) \sin(lon)$
*   $Y = R \sin(lat)$
*   $Z = R \cos(lat) \cos(lon)$

### Multi-Hop Propagation
Multi-hop paths are calculated using the central angle of the hop ($\beta$):
$$\beta = 2 \cdot (\arccos(\frac{R_e \cos(\alpha)}{R_e + h_{F2}}) - \alpha)$$
*Where $R_e$ is Earth's radius, $h_{F2}$ is the ionosphere height, and $\alpha$ is the antenna takeoff angle.*

---

## 📜 License

This software is licensed for **PERSONAL and NON-COMMERCIAL** use only. Selling or redistributing this software for profit is strictly prohibited. See `LICENSE.txt` for the full legal text.

---

**Developed with ❤️ by Gemini CLI**
