/*
 * RFSim - Radio Propagation Simulator
 * Copyright (c) 2026 damplex23. All rights reserved.
 * 
 * This software is licensed for PERSONAL and NON-COMMERCIAL use only.
 * Selling or redistributing this software for profit is strictly prohibited.
 * See LICENSE.txt in the project root for full license details.
 */

#include "MapEngine.hpp"
#include "imgui.h"
#include "PropagationEngine.hpp"
#include <cmath>
#include <iostream>
#include <thread>
#ifdef _WIN32
    #define CloseWindow CloseWindowWin
    #define ShowCursor ShowCursorWin
    #define Rectangle RectangleWin
#endif
#include <curl/curl.h>
#ifdef _WIN32
    #undef CloseWindow
    #undef ShowCursor
    #undef Rectangle
#endif
#include <cstring>
#include <algorithm>
#include "rlgl.h"
#include "raymath.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace RFSim {

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == nullptr) {
        return 0;  // out of memory
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

MapEngine::MapEngine() {
    m_camera.target = Vector2{ 0.0f, 0.0f };
    m_camera.offset = Vector2{ 0.0f, 0.0f };
    m_camera.rotation = 0.0f;
    m_camera.zoom = 1.0f;

    m_camera3D.position = Vector3{ 25.0f, 25.0f, 25.0f };
    m_camera3D.target = Vector3{ 0.0f, 0.0f, 0.0f };
    m_camera3D.up = Vector3{ 0.0f, 1.0f, 0.0f };
    m_camera3D.fovy = 45.0f;
    m_camera3D.projection = CAMERA_PERSPECTIVE;

    m_viewLat = 0.0;
    m_viewLon = 0.0;

    m_globeTexture = { 0 };
    m_globeTextureRequested = false;
}

MapEngine::~MapEngine() {
}

void MapEngine::Initialize() {
    static bool curl_initialized = false;
    if (!curl_initialized) {
        curl_global_init(CURL_GLOBAL_ALL);
        curl_initialized = true;
    }

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    m_camera.offset = Vector2{ (float)screenWidth / 2.0f, (float)screenHeight / 2.0f };
}

void MapEngine::Shutdown() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    for (auto& pair : m_tileCache) {
        if (pair.second && pair.second->loaded) {
            UnloadTexture(pair.second->texture);
        }
    }
    m_tileCache.clear();
    if (m_globeTexture.id != 0) UnloadTexture(m_globeTexture);
    curl_global_cleanup();
}
Vector2 MapEngine::LatLonToTile(double lat, double lon, int zoom) {
    double n = std::pow(2.0, zoom);
    double x = n * ((lon + 180.0) / 360.0);
    double lat_rad = lat * M_PI / 180.0;
    double y = n * (1.0 - (std::log(std::tan(lat_rad) + 1.0 / std::cos(lat_rad)) / M_PI)) / 2.0;
    return Vector2{ (float)x, (float)y };
}

GeoCoord MapEngine::PixelToLatLon(Vector2 pixel, Vector2 centerTile, int zoom) {
    double n = std::pow(2.0, zoom);
    double tileX = centerTile.x + (pixel.x / 256.0);
    double tileY = centerTile.y + (pixel.y / 256.0);

    double lon = (tileX / n) * 360.0 - 180.0;
    double lat_rad = std::atan(std::sinh(M_PI * (1.0 - 2.0 * tileY / n)));
    double lat = lat_rad * 180.0 / M_PI;

    return GeoCoord{ lat, lon };
}

uint64_t MapEngine::GetTileKey(int x, int y, int z) {
    return ((uint64_t)z << 48) | ((uint64_t)(x & 0xFFFFFF) << 24) | (uint64_t)(y & 0xFFFFFF);
}

void MapEngine::FetchTileAsync(int x, int y, int z) {
    uint64_t key = GetTileKey(x, y, z);
    
    CURL *curl_handle;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = (char*)malloc(1);
    chunk.size = 0;

    std::string url = "https://tile.openstreetmap.org/" + std::to_string(z) + "/" + std::to_string(x) + "/" + std::to_string(y) + ".png";

    curl_handle = curl_easy_init();
    if(curl_handle) {
        curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "RFSim-Client/1.0 (https://github.com/damplex/RFSim)");
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10L);
        
        res = curl_easy_perform(curl_handle);
        
        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            if(res == CURLE_OK && chunk.size > 0) {
                auto it = m_tileCache.find(key);
                if (it != m_tileCache.end() && !it->second->loaded) {
                    it->second->filePath = std::string(chunk.memory, chunk.size);
                }
            } else {
                // If fetch failed, remove from cache so it can be retried later
                m_tileCache.erase(key);
            }
            // Remove from pending
            m_pendingRequests.erase(std::remove(m_pendingRequests.begin(), m_pendingRequests.end(), key), m_pendingRequests.end());
        }
        curl_easy_cleanup(curl_handle);
    }
    free(chunk.memory);
}

void MapEngine::PruneCache() {
    const size_t maxTiles = 1024;
    if (m_tileCache.size() <= maxTiles) return;

    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    std::vector<uint64_t> keysToPrune;
    uint64_t globeKey = GetTileKey(0, 0, 0);

    for (auto& [key, tile] : m_tileCache) {
        if (key == globeKey) continue;
        
        bool isPending = std::find(m_pendingRequests.begin(), m_pendingRequests.end(), key) != m_pendingRequests.end();
        if (isPending) continue;

        keysToPrune.push_back(key);
    }

    std::sort(keysToPrune.begin(), keysToPrune.end(), [&](uint64_t a, uint64_t b) {
        return m_tileCache[a]->lastAccessTime < m_tileCache[b]->lastAccessTime;
    });

    size_t toRemove = m_tileCache.size() - maxTiles;
    for (size_t i = 0; i < std::min(toRemove, keysToPrune.size()); ++i) {
        uint64_t key = keysToPrune[i];
        if (m_tileCache[key]->loaded) {
            UnloadTexture(m_tileCache[key]->texture);
        }
        m_tileCache.erase(key);
    }
}

void MapEngine::Update(const RFParameters& params) {
    if (params.displayMode == DisplayMode::Surface3D || params.displayMode == DisplayMode::Globe3D) {
        UpdateCamera(&m_camera3D, CAMERA_ORBITAL);
    }

    double currentTime = GetTime();

    // Ensure globe texture is requested at zoom level 0
    uint64_t globeKey = GetTileKey(0, 0, 0);
    if (!m_globeTextureRequested) {
        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            if (m_tileCache.find(globeKey) == m_tileCache.end()) {
                m_tileCache[globeKey] = std::make_shared<MapTile>();
                m_pendingRequests.push_back(globeKey);
            }
        }
        std::thread(&MapEngine::FetchTileAsync, this, 0, 0, 0).detach();
        m_globeTextureRequested = true;
    }

    // Check if globe texture is ready to be loaded
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto it = m_tileCache.find(globeKey);
        if (it != m_tileCache.end() && it->second != nullptr) {
            auto& tile = it->second;
            tile->lastAccessTime = currentTime;
            if (!tile->loaded && tile->filePath.size() > 0) {
                Image img = LoadImageFromMemory(".png", (const unsigned char*)tile->filePath.c_str(), tile->filePath.size());
                if (img.data != nullptr) {
                    m_globeTexture = LoadTextureFromImage(img);
                    tile->loaded = true;
                    UnloadImage(img);
                }
                tile->filePath.clear();
            }
        }
    }

    if (params.displayMode == DisplayMode::Globe3D) return;

    Vector2 centerTile = LatLonToTile(m_viewLat, m_viewLon, m_zoom);
    int centerTx = (int)centerTile.x;
    int centerTy = (int)centerTile.y;

    int radius = 3;
    bool requestLaunched = false;
    for (int y = centerTy - radius; y <= centerTy + radius; ++y) {
        for (int x = centerTx - radius; x <= centerTx + radius; ++x) {
            uint64_t key = GetTileKey(x, y, m_zoom);
            
            bool needsFetch = false;
            {
                std::lock_guard<std::mutex> lock(m_cacheMutex);
                auto it = m_tileCache.find(key);
                if (it == m_tileCache.end()) {
                    if (std::find(m_pendingRequests.begin(), m_pendingRequests.end(), key) == m_pendingRequests.end()) {
                        m_tileCache[key] = std::make_shared<MapTile>();
                        m_tileCache[key]->lastAccessTime = currentTime;
                        m_pendingRequests.push_back(key);
                        needsFetch = true;
                    }
                } else {
                    auto& tile = it->second;
                    tile->lastAccessTime = currentTime;
                    if (!tile->loaded && tile->filePath.size() > 0) {
                        Image img = LoadImageFromMemory(".png", (const unsigned char*)tile->filePath.c_str(), tile->filePath.size());
                        if (img.data != nullptr) {
                            tile->texture = LoadTextureFromImage(img);
                            tile->loaded = true;
                            UnloadImage(img);
                        }
                        tile->filePath.clear();
                    }
                }
            }

            if (needsFetch) {
                std::thread(&MapEngine::FetchTileAsync, this, x, y, m_zoom).detach();
                requestLaunched = true;
            }
        }
    }

    static int frameCounter = 0;
    if (!requestLaunched && ++frameCounter % 300 == 0) {
        PruneCache();
    }
}

void MapEngine::Draw(const RFParameters& params) {
    if (params.displayMode == DisplayMode::Map2D) {
        Draw2D();
    } else if (params.displayMode == DisplayMode::Surface3D) {
        Draw3DSurface(params);
    } else {
        DrawGlobe(params);
    }
}

static Vector3 LatLonToCartesian(double lat, double lon, float radius) {
    float latRad = (float)lat * (float)M_PI / 180.0f;
    float lonRad = (float)lon * (float)M_PI / 180.0f;
    return Vector3{
        radius * std::cos(latRad) * std::sin(lonRad),
        radius * std::sin(latRad),
        radius * std::cos(latRad) * std::cos(lonRad)
    };
}

void MapEngine::DrawGlobe(const RFParameters& params) {
    BeginMode3D(m_camera3D);
    
    float earthRadius = 10.0f;

    // Draw Textured Globe
    rlPushMatrix();
    
    if (m_globeTexture.id != 0) {
        rlSetTexture(m_globeTexture.id);
        rlBegin(RL_QUADS);
            const int res = 32;
            for (int i = 0; i < res; i++) {
                for (int j = 0; j < res; j++) {
                    float lat1 = -90.0f + (float)i * 180.0f / (float)res;
                    float lat2 = -90.0f + (float)(i + 1) * 180.0f / (float)res;
                    float lon1 = -180.0f + (float)j * 360.0f / (float)res;
                    float lon2 = -180.0f + (float)(j + 1) * 360.0f / (float)res;

                    auto getSphereV = [&](float lat, float lon) {
                        float lr = lat * (float)M_PI / 180.0f;
                        float nr = lon * (float)M_PI / 180.0f;
                        return Vector3{ 
                            earthRadius * std::cos(lr) * std::sin(nr), 
                            earthRadius * std::sin(lr), 
                            earthRadius * std::cos(lr) * std::cos(nr) 
                        };
                    };

                    auto getTexV = [&](float lat) {
                        if (lat >= 85.0511f) return 0.0f;
                        if (lat <= -85.0511f) return 1.0f;
                        float lr = lat * (float)M_PI / 180.0f;
                        return (1.0f - (std::log(std::tan(lr) + 1.0f/std::cos(lr)) / (float)M_PI)) / 2.0f;
                    };

                    float u1 = (float)j/(float)res;
                    float u2 = (float)(j+1)/(float)res;
                    float v1 = getTexV(lat1);
                    float v2 = getTexV(lat2);

                    rlColor4ub(255, 255, 255, 255);
                    Vector3 vtx1 = getSphereV(lat1, lon1);
                    Vector3 vtx2 = getSphereV(lat1, lon2);
                    Vector3 vtx3 = getSphereV(lat2, lon2);
                    Vector3 vtx4 = getSphereV(lat2, lon1);

                    rlTexCoord2f(u1, v1); rlVertex3f(vtx1.x, vtx1.y, vtx1.z);
                    rlTexCoord2f(u2, v1); rlVertex3f(vtx2.x, vtx2.y, vtx2.z);
                    rlTexCoord2f(u2, v2); rlVertex3f(vtx3.x, vtx3.y, vtx3.z);
                    rlTexCoord2f(u1, v2); rlVertex3f(vtx4.x, vtx4.y, vtx4.z);
                }
            }
        rlEnd();
        rlSetTexture(0);
    } else {
        DrawSphereWires(Vector3{0, 0, 0}, earthRadius, 16, 16, DARKGRAY);
    }
    rlPopMatrix();

    // Draw TX Position
    if (params.txLatitude != 0.0 || params.txLongitude != 0.0) {
        Vector3 txPos = LatLonToCartesian(params.txLatitude, params.txLongitude, earthRadius + 0.1f);
        DrawSphere(txPos, 0.2f, RED);
        
        // Draw Skywave DX Arcs (Volumetric Coverage Map)
        if (params.operatingFrequencyMhz >= 3.0f && params.operatingFrequencyMhz <= 30.0f) {
            float beamwidth = 360.0f;
            float stepAz = 10.0f;
            
            switch (params.txAntennaType) {
                case AntennaType::Yagi3Element: beamwidth = 60.0f; stepAz = 5.0f; break;
                case AntennaType::Yagi10Element: beamwidth = 30.0f; stepAz = 3.0f; break;
                case AntennaType::ParabolicMicrowaveDish: beamwidth = 15.0f; stepAz = 2.0f; break;
                case AntennaType::FlatMicrostripPatch: beamwidth = 90.0f; stepAz = 10.0f; break;
                case AntennaType::MagneticShieldedLoop: beamwidth = 180.0f; stepAz = 15.0f; break;
                default: beamwidth = 360.0f; stepAz = 15.0f; break;
            }

            float hF2_km = 300.0f;
            float earthRadius_km = 6371.0f;
            float skyRadius = earthRadius * (1.0f + hF2_km / earthRadius_km);

            rlBegin(RL_LINES);
            float startAz = (beamwidth >= 360.0f) ? 0.0f : params.txAzimuthDegrees - beamwidth/2.0f;
            float endAz = (beamwidth >= 360.0f) ? 360.0f : params.txAzimuthDegrees + beamwidth/2.0f;

            for (float az = startAz; az < endAz; az += stepAz) {
                float azRad = (90.0f - az) * (float)M_PI / 180.0f;
                for (int angleDeg = 10; angleDeg <= 35; angleDeg += 10) {
                    float alpha = (float)angleDeg * (float)M_PI / 180.0f;
                    float beta = 2.0f * (std::acos(earthRadius_km * std::cos(alpha) / (earthRadius_km + hF2_km)) - alpha);
                    int segmentsPerHop = 16;
                    Vector3 currentOrigin = txPos;
                    for (int hop = 0; hop < 3; hop++) {
                        Vector3 north = {0, 1, 0};
                        Vector3 east;
                        if (std::abs(currentOrigin.y) > earthRadius * 0.999f) east = {1, 0, 0};
                        else east = Vector3Normalize(Vector3CrossProduct(north, currentOrigin));
                        Vector3 actualNorthAtPoint = Vector3Normalize(Vector3CrossProduct(currentOrigin, east));
                        Vector3 dir = Vector3Add(Vector3Scale(actualNorthAtPoint, std::cos(azRad)), Vector3Scale(east, std::sin(azRad)));
                        Vector3 rotAxis = Vector3Normalize(Vector3CrossProduct(currentOrigin, dir));
                        auto rotateVec = [&](Vector3 v, float angle) {
                            float cosA = std::cos(angle);
                            float sinA = std::sin(angle);
                            return Vector3Add(Vector3Add(Vector3Scale(v, cosA), Vector3Scale(Vector3CrossProduct(rotAxis, v), sinA)),
                                Vector3Scale(rotAxis, Vector3DotProduct(rotAxis, v) * (1.0f - cosA)));
                        };
                        Vector3 prevSegPoint = currentOrigin;
                        for (int s = 1; s <= segmentsPerHop; s++) {
                            float t = (float)s / (float)segmentsPerHop;
                            float hFactor = std::sin(t * (float)M_PI);
                            float currentR = earthRadius + (skyRadius - earthRadius) * hFactor;
                            Vector3 arcPoint = rotateVec(currentOrigin, t * beta);
                            arcPoint = Vector3Scale(Vector3Normalize(arcPoint), currentR);
                            unsigned char alphaByte = (unsigned char)(200 / (hop + 1));
                            if (hop == 0) rlColor4ub(0, 255, 121, alphaByte); 
                            else if (hop == 1) rlColor4ub(255, 203, 0, alphaByte); 
                            else rlColor4ub(230, 41, 55, alphaByte); 
                            rlVertex3f(prevSegPoint.x, prevSegPoint.y, prevSegPoint.z);
                            rlVertex3f(arcPoint.x, arcPoint.y, arcPoint.z);
                            prevSegPoint = arcPoint;
                        }
                        currentOrigin = Vector3Scale(Vector3Normalize(rotateVec(currentOrigin, beta)), earthRadius + 0.05f);
                    }
                }
            }
            rlEnd();
        }
    }

    EndMode3D();
}

void MapEngine::Draw2D() {
    Vector2 centerTileRaw = LatLonToTile(m_viewLat, m_viewLon, m_zoom);
    int centerTx = (int)centerTileRaw.x;
    int centerTy = (int)centerTileRaw.y;

    float offsetX = (centerTileRaw.x - centerTx) * 256.0f;
    float offsetY = (centerTileRaw.y - centerTy) * 256.0f;

    BeginMode2D(m_camera);
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    int radius = 3;
    double currentTime = GetTime();
    for (int y = centerTy - radius; y <= centerTy + radius; ++y) {
        for (int x = centerTx - radius; x <= centerTx + radius; ++x) {
            uint64_t key = GetTileKey(x, y, m_zoom);
            auto it = m_tileCache.find(key);
            if (it != m_tileCache.end() && it->second->loaded) {
                it->second->lastAccessTime = currentTime;
                float posX = (x - centerTx) * 256.0f - offsetX;
                float posY = (y - centerTy) * 256.0f - offsetY;
                DrawTexture(it->second->texture, (int)posX, (int)posY, WHITE);
            }
        }
    }
    EndMode2D();
}


static void DrawAntennaPatternLobes(const RFParameters& params, Vector3 position) {
    rlPushMatrix();
    rlTranslatef(position.x, position.y + 0.5f, position.z); // Slightly above ground
    rlRotatef(-params.txAzimuthDegrees, 0, 1, 0); 

    const int segments = 32;
    const int rings = 16;
    
    // Dynamic scale for HF DX visualization
    float visualScale = 3.0f; 
    if (params.operatingFrequencyMhz >= 3.0f && params.operatingFrequencyMhz <= 30.0f) {
        visualScale = 6.0f; // Larger for HF to show skip projection
    }

    auto getLobeRadius = [&](float t, float p) {
        float localAz = p * 180.0f / (float)M_PI;
        if (localAz > 180.0f) localAz -= 360.0f;
        
        float gainFactor = 0.1f; 

        switch (params.txAntennaType) {
            case AntennaType::Isotropic:
                gainFactor = 1.0f;
                break;
            case AntennaType::CenterFedDipole:
            case AntennaType::VerticalMonopole:
            case AntennaType::MobileGroundPlaneWhip:
                gainFactor = std::abs(std::sin(t));
                break;
            case AntennaType::MagneticShieldedLoop:
                gainFactor = std::abs(std::cos(p)) * std::sin(t);
                break;
            case AntennaType::Yagi3Element:
            case AntennaType::Yagi10Element:
            case AntennaType::ParabolicMicrowaveDish:
            case AntennaType::FlatMicrostripPatch:
                {
                    float bw = 60.0f;
                    if (params.txAntennaType == AntennaType::Yagi10Element) bw = 30.0f;
                    else if (params.txAntennaType == AntennaType::ParabolicMicrowaveDish) bw = 10.0f;
                    else if (params.txAntennaType == AntennaType::FlatMicrostripPatch) bw = 90.0f;
                    
                    float angle = localAz;
                    float patternLoss = 12.0f * std::pow(angle / bw, 2.0f);
                    float gainDb = -patternLoss;
                    if (gainDb < -20.0f) gainDb = -20.0f;
                    gainFactor = std::pow(10.0f, gainDb / 20.0f) * std::sin(t);
                }
                break;
            default:
                gainFactor = 0.5f;
                break;
        }
        return gainFactor * visualScale;
    };

    rlDisableBackfaceCulling();
    rlBegin(RL_TRIANGLES);
    for (int i = 0; i < rings; i++) {
        float theta1 = (float)i * (float)M_PI / rings;
        float theta2 = (float)(i + 1) * (float)M_PI / rings;

        for (int j = 0; j < segments; j++) {
            float phi1 = (float)j * 2.0f * (float)M_PI / segments;
            float phi2 = (float)(j + 1) * 2.0f * (float)M_PI / segments;

            auto getPos = [&](float t, float p) {
                float r = getLobeRadius(t, p);
                return Vector3{ 
                    r * std::sin(t) * std::cos(p), 
                    r * std::cos(t), 
                    r * std::sin(t) * std::sin(p) 
                };
            };

            Vector3 v1 = getPos(theta1, phi1);
            Vector3 v2 = getPos(theta1, phi2);
            Vector3 v3 = getPos(theta2, phi2);
            Vector3 v4 = getPos(theta2, phi1);

            // Triangle 1
            rlColor4ub(255, 100, 0, 100);
            rlVertex3f(v1.x, v1.y, v1.z);
            rlVertex3f(v2.x, v2.y, v2.z);
            rlVertex3f(v3.x, v3.y, v3.z);
            
            // Triangle 2
            rlColor4ub(255, 150, 0, 100);
            rlVertex3f(v1.x, v1.y, v1.z);
            rlVertex3f(v3.x, v3.y, v3.z);
            rlVertex3f(v4.x, v4.y, v4.z);
        }
    }
    rlEnd();
    rlEnableBackfaceCulling();
    rlPopMatrix();
}

static void DrawSkywaveVisualization(const RFParameters& params, Vector3 txPos, float kmPerUnit) {
    if (params.operatingFrequencyMhz < 3.0f || params.operatingFrequencyMhz > 30.0f) return;

    float hF2_km = 300.0f; // Standard F2 virtual height
    float hF2_units = hF2_km / kmPerUnit;
    
    // 1. Draw Ionosphere representation (Semi-transparent shell)
    rlDisableBackfaceCulling();
    rlBegin(RL_QUADS);
        rlColor4ub(50, 120, 255, 40); 
        float sz = 60.0f;
        rlVertex3f(txPos.x - sz, hF2_units, txPos.z - sz);
        rlVertex3f(txPos.x + sz, hF2_units, txPos.z - sz);
        rlVertex3f(txPos.x + sz, hF2_units, txPos.z + sz);
        rlVertex3f(txPos.x - sz, hF2_units, txPos.z + sz);
    rlEnd();
    
    // 2. Draw Grid for Ionosphere to give it depth
    rlBegin(RL_LINES);
    rlColor4ub(100, 200, 255, 100);
    for (float i = -sz; i <= sz; i += 10.0f) {
        rlVertex3f(txPos.x + i, hF2_units, txPos.z - sz);
        rlVertex3f(txPos.x + i, hF2_units, txPos.z + sz);
        rlVertex3f(txPos.x - sz, hF2_units, txPos.z + i);
        rlVertex3f(txPos.x + sz, hF2_units, txPos.z + i);
    }
    rlEnd();
    rlEnableBackfaceCulling();

    // 3. Draw Skip Rays (Propagation paths)
    // We sample different take-off angles that would actually skip
    rlBegin(RL_LINES);
    float azRad = -params.txAzimuthDegrees * (float)M_PI / 180.0f;
    
    for (int angleDeg = 10; angleDeg <= 40; angleDeg += 10) {
        float alpha = (float)angleDeg * (float)M_PI / 180.0f;
        float theta = (float)M_PI / 2.0f - alpha; // Angle of incidence
        float dist_km = 2.0f * hF2_km * std::tan(theta);
        float dist_units = dist_km / kmPerUnit;
        
        // Ray Up to Ionosphere
        rlColor4ub(0, 255, 121, 255); // Neon Green for high-energy radiation
        rlVertex3f(txPos.x, txPos.y + 0.5f, txPos.z);
        
        float hopX = txPos.x + (dist_units / 2.0f) * std::cos(azRad);
        float hopZ = txPos.z + (dist_units / 2.0f) * std::sin(azRad);
        
        rlVertex3f(hopX, hF2_units, hopZ);
        
        // Ray Down to Earth
        rlColor4ub(0, 255, 255, 180); // Cyan for returning signal
        rlVertex3f(hopX, hF2_units, hopZ);
        
        float endX = txPos.x + dist_units * std::cos(azRad);
        float endZ = txPos.z + dist_units * std::sin(azRad);
        
        rlVertex3f(endX, 0.1f, endZ);
    }
    rlEnd();
}

void MapEngine::Draw3DSurface(const RFParameters& params) {
    BeginMode3D(m_camera3D);
    
    // Draw Textured Ground Plane
    Vector2 centerTileRaw = LatLonToTile(m_viewLat, m_viewLon, m_zoom);
    int centerTx = (int)centerTileRaw.x;
    int centerTy = (int)centerTileRaw.y;
    float offsetX = (centerTileRaw.x - centerTx) * 256.0f;
    float offsetY = (centerTileRaw.y - centerTy) * 256.0f;

    // Recalculate offsetY to match Draw2D logic (this was correct before, but let's be careful)
    offsetY = (centerTileRaw.y - centerTy) * 256.0f;

    // Calculate world scale for km-to-unit conversion
    double n_tiles = std::pow(2.0, m_zoom);
    double circumferenceKm = 40075.0;
    double kmPerPixel = circumferenceKm / (256.0 * n_tiles);
    float kmPerUnit = (float)kmPerPixel * 50.0f;

    std::lock_guard<std::mutex> lock(m_cacheMutex);
    int radius = 3;
    double currentTime = GetTime();
    for (int y = centerTy - radius; y <= centerTy + radius; ++y) {
        for (int x = centerTx - radius; x <= centerTx + radius; ++x) {
            uint64_t key = GetTileKey(x, y, m_zoom);
            auto it = m_tileCache.find(key);
            if (it != m_tileCache.end() && it->second->loaded) {
                it->second->lastAccessTime = currentTime;
                float posX = (x - centerTx) * 256.0f - offsetX;
                float posZ = (y - centerTy) * 256.0f - offsetY;

                float wX = posX / 50.0f;
                float wZ = posZ / 50.0f;
                float wSize = 256.0f / 50.0f;

                rlSetTexture(it->second->texture.id);
                rlBegin(RL_QUADS);
                    rlColor4ub(255, 255, 255, 255);
                    // Fixed winding order for top-down visibility (CCW)
                    rlTexCoord2f(0.0f, 0.0f); rlVertex3f(wX, 0.0f, wZ);
                    rlTexCoord2f(0.0f, 1.0f); rlVertex3f(wX, 0.0f, wZ + wSize);
                    rlTexCoord2f(1.0f, 1.0f); rlVertex3f(wX + wSize, 0.0f, wZ + wSize);
                    rlTexCoord2f(1.0f, 0.0f); rlVertex3f(wX + wSize, 0.0f, wZ);
                rlEnd();
                rlSetTexture(0);
            }
        }
    }

    if (params.txLatitude != 0.0 || params.txLongitude != 0.0) {
        const int gridSize = 80; // Increased resolution
        const float spacing = 0.35f; 
        const float startPos = -(gridSize * spacing) / 2.0f;

        Vector2 centerTile = LatLonToTile(m_viewLat, m_viewLon, m_zoom);

        rlDisableBackfaceCulling();
        rlSetBlendMode(BLEND_ALPHA);
        for (int z = 0; z < gridSize - 1; ++z) {
            rlBegin(RL_TRIANGLES);
            for (int x = 0; x < gridSize - 1; ++x) {
                float pts[4][3];
                Color cls[4];

                int coords[4][2] = {{x, z}, {x + 1, z}, {x + 1, z + 1}, {x, z + 1}};

                for (int i = 0; i < 4; ++i) {
                    float worldX = startPos + coords[i][0] * spacing;
                    float worldZ = startPos + coords[i][1] * spacing;
                    Vector2 pixelPos = { worldX * 50.0f, worldZ * 50.0f };
                    GeoCoord geo = PixelToLatLon(pixelPos, centerTile, m_zoom);
                    float power = PropagationEngine::CalculateReceivedPower(params, geo.lat, geo.lon);
                    
                    float h = 0.1f; // Slightly higher base to avoid Z-fighting
                    cls[i] = Color{ 0, 0, 0, 0 }; 
                    if (power != -999.0f) {
                        float margin = power - params.rxSensitivityDbm;
                        float norm = margin / (-30.0f - params.rxSensitivityDbm);
                        if (norm < 0.0f) norm = 0.0f;
                        if (norm > 1.0f) norm = 1.0f;
                        h += norm * 8.0f * params.surfaceExaggeration;
                        
                        // Brighter, more opaque colors for 3D surface
                        if (margin < 10.0f)      cls[i] = Color{ 0, 121, 241, 180 };  
                        else if (margin < 20.0f) cls[i] = Color{ 0, 255, 255, 190 };  
                        else if (margin < 30.0f) cls[i] = Color{ 0, 228, 48, 200 };   
                        else if (margin < 45.0f) cls[i] = Color{ 255, 203, 0, 220 };  
                        else                     cls[i] = Color{ 230, 41, 55, 240 };  
                    }
                    pts[i][0] = worldX; pts[i][1] = h; pts[i][2] = worldZ;
                }

                if (cls[0].a > 0 || cls[1].a > 0 || cls[2].a > 0 || cls[3].a > 0) {
                    // Triangle 1 (CCW)
                    rlColor4ub(cls[0].r, cls[0].g, cls[0].b, cls[0].a); rlVertex3f(pts[0][0], pts[0][1], pts[0][2]);
                    rlColor4ub(cls[1].r, cls[1].g, cls[1].b, cls[1].a); rlVertex3f(pts[1][0], pts[1][1], pts[1][2]);
                    rlColor4ub(cls[2].r, cls[2].g, cls[2].b, cls[2].a); rlVertex3f(pts[2][0], pts[2][1], pts[2][2]);

                    // Triangle 2 (CCW)
                    rlColor4ub(cls[0].r, cls[0].g, cls[0].b, cls[0].a); rlVertex3f(pts[0][0], pts[0][1], pts[0][2]);
                    rlColor4ub(cls[2].r, cls[2].g, cls[2].b, cls[2].a); rlVertex3f(pts[2][0], pts[2][1], pts[2][2]);
                    rlColor4ub(cls[3].r, cls[3].g, cls[3].b, cls[3].a); rlVertex3f(pts[3][0], pts[3][1], pts[3][2]);
                }
            }
            rlEnd();
        }
        rlSetBlendMode(BLEND_ALPHA); // Reset to default just in case

        // Draw wireframe overlay for depth perception
        rlBegin(RL_LINES);
        for (int z = 0; z < gridSize - 1; z += 2) {
            for (int x = 0; x < gridSize - 1; x += 2) {
                float worldX = startPos + x * spacing;
                float worldZ = startPos + z * spacing;
                Vector2 pixelPos = { worldX * 50.0f, worldZ * 50.0f };
                GeoCoord geo = PixelToLatLon(pixelPos, centerTile, m_zoom);
                float power = PropagationEngine::CalculateReceivedPower(params, geo.lat, geo.lon);
                
                if (power != -999.0f) {
                    float margin = power - params.rxSensitivityDbm;
                    float norm = std::clamp(margin / (-30.0f - params.rxSensitivityDbm), 0.0f, 1.0f);
                    float h = 0.1f + norm * 8.0f * params.surfaceExaggeration;
                    
                    rlColor4ub(255, 255, 255, 60); // Subtle white grid
                    rlVertex3f(worldX, h, worldZ);
                    rlVertex3f(worldX + spacing * 2, h, worldZ);
                    rlVertex3f(worldX, h, worldZ);
                    rlVertex3f(worldX, h, worldZ + spacing * 2);
                }
            }
        }
        rlEnd();
        rlEnableBackfaceCulling();

        Vector2 txTile = LatLonToTile(params.txLatitude, params.txLongitude, m_zoom);
        float txX = (txTile.x - centerTile.x) * 256.0f / 50.0f;
        float txZ = (txTile.y - centerTile.y) * 256.0f / 50.0f;
        Vector3 txWorldPos = {txX, 0.5f, txZ};

        DrawCube(txWorldPos, 0.2f, 1.0f, 0.2f, RED);
        DrawAntennaPatternLobes(params, Vector3{txX, 0.0f, txZ});
        DrawSkywaveVisualization(params, txWorldPos, kmPerUnit);
    }

    EndMode3D();
}

void MapEngine::HandleInput(RFParameters& params) {
    if (params.displayMode == DisplayMode::Surface3D || params.displayMode == DisplayMode::Globe3D) return;

    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        int oldZoom = m_zoom;
        m_zoom += (int)wheel;
        if (m_zoom < 2) m_zoom = 2;
        if (m_zoom > 18) m_zoom = 18;

        if (params.txLatitude != 0.0 || params.txLongitude != 0.0) {
            double tx_rad = params.txLatitude * M_PI / 180.0;
            double tx_tile_x0 = (params.txLongitude + 180.0) / 360.0;
            double tx_tile_y0 = (1.0 - (std::log(std::tan(tx_rad) + 1.0 / std::cos(tx_rad)) / M_PI)) / 2.0;
            double view_rad = m_viewLat * M_PI / 180.0;
            double view_tile_x0 = (m_viewLon + 180.0) / 360.0;
            double view_tile_y0 = (1.0 - (std::log(std::tan(view_rad) + 1.0 / std::cos(view_rad)) / M_PI)) / 2.0;
            double zoomFactor = std::pow(2.0, oldZoom - m_zoom);
            double new_view_tile_x0 = tx_tile_x0 - (tx_tile_x0 - view_tile_x0) * zoomFactor;
            double new_view_tile_y0 = tx_tile_y0 - (tx_tile_y0 - view_tile_y0) * zoomFactor;
            m_viewLon = new_view_tile_x0 * 360.0 - 180.0;
            double lat_rad = std::atan(std::sinh(M_PI * (1.0 - 2.0 * new_view_tile_y0)));
            m_viewLat = lat_rad * 180.0 / M_PI;
        }
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        Vector2 centerTile = LatLonToTile(m_viewLat, m_viewLon, m_zoom);
        GeoCoord newCenter = PixelToLatLon(Vector2{-delta.x, -delta.y}, centerTile, m_zoom);
        m_viewLat = newCenter.lat;
        m_viewLon = newCenter.lon;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (!ImGui::GetIO().WantCaptureMouse) {
            Vector2 mousePos = GetMousePosition();
            Vector2 screenCenter = m_camera.offset;
            Vector2 adjustedPixel = { mousePos.x - screenCenter.x, mousePos.y - screenCenter.y };
            Vector2 centerTile = LatLonToTile(m_viewLat, m_viewLon, m_zoom);
            GeoCoord clickLatLon = PixelToLatLon(adjustedPixel, centerTile, m_zoom);
            params.txLatitude = clickLatLon.lat;
            params.txLongitude = clickLatLon.lon;
            m_viewLat = params.txLatitude;
            m_viewLon = params.txLongitude;
        }
    }
}
}
