/*
 * RFSim - Radio Propagation Simulator
 * Copyright (c) 2026 damplex23. All rights reserved.
 * 
 * This software is licensed for PERSONAL and NON-COMMERCIAL use only.
 * Selling or redistributing this software for profit is strictly prohibited.
 * See LICENSE.txt in the project root for full license details.
 */

#ifndef MAP_ENGINE_HPP
#define MAP_ENGINE_HPP

#include "raylib.h"
#include "DataStructures.hpp"
#include <map>
#include <string>
#include <memory>
#include <mutex>
#include <vector>

namespace RFSim {

struct MapTile {
    Texture2D texture;
    bool loaded = false;
    std::string filePath;
    double lastAccessTime = 0.0;
};

class MapEngine {
public:
    MapEngine();
    ~MapEngine();

    void Initialize();
    void Update(const RFParameters& params);
    void Draw(const RFParameters& params);
    void Shutdown();

    // Coordinate Translation Math (Web Mercator)
    static Vector2 LatLonToTile(double lat, double lon, int zoom);
    static GeoCoord PixelToLatLon(Vector2 pixel, Vector2 centerTile, int zoom);
    
    // Interaction
    void HandleInput(RFParameters& params);
    
    // View State
    double GetViewLat() const { return m_viewLat; }
    double GetViewLon() const { return m_viewLon; }
    int GetZoom() const { return m_zoom; }

private:
    void Draw2D();
    void Draw3DSurface(const RFParameters& params);
    void DrawGlobe(const RFParameters& params);
    void FetchTileAsync(int x, int y, int z);
    void PruneCache();

    // Packed key: [16 bits Z | 24 bits X | 24 bits Y]
    static uint64_t GetTileKey(int x, int y, int z);

    double m_viewLat = 0.0;
    double m_viewLon = 0.0;
    int m_zoom = 10;

    std::map<uint64_t, std::shared_ptr<MapTile>> m_tileCache;
    std::vector<uint64_t> m_pendingRequests; // Tracks active downloads
    std::mutex m_cacheMutex;
    
    Camera2D m_camera;
    Camera3D m_camera3D;
    
    Texture2D m_globeTexture = { 0 };
    bool m_globeTextureRequested = false;
};

} // namespace RFSim

#endif // MAP_ENGINE_HPP
