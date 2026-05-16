/*
 * RFSim - Radio Propagation Simulator
 * Copyright (c) 2026 damplex23. All rights reserved.
 * 
 * This software is licensed for PERSONAL and NON-COMMERCIAL use only.
 * Selling or redistributing this software for profit is strictly prohibited.
 * See LICENSE.txt in the project root for full license details.
 */

#include "raylib.h"
#include "rlImGui.h"
#include "UIManager.hpp"
#include "MapEngine.hpp"
#include "PropagationEngine.hpp"
#include "DataStructures.hpp"
#include <iostream>
#include <vector>

int main() {
    // 1. Initialization
    const int screenWidth = 1280;
    const int screenHeight = 720;
    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "RFSim - High-Fidelity Radio Propagation Simulator");
    SetTargetFPS(60);

    RFSim::RFParameters systemParams;
    
    RFSim::UIManager uiManager;
    uiManager.Initialize();

    RFSim::MapEngine mapEngine;
    mapEngine.Initialize();

    // Setup Heatmap Overlay Texture
    RenderTexture2D heatmapTarget = LoadRenderTexture(screenWidth, screenHeight);
    
    // Main Simulation Loop
    while (!WindowShouldClose()) {
        // --- Handle Resizing ---
        if (IsWindowResized()) {
            int newWidth = GetScreenWidth();
            int newHeight = GetScreenHeight();
            
            // Recreate heatmap target at new size
            UnloadRenderTexture(heatmapTarget);
            heatmapTarget = LoadRenderTexture(newWidth, newHeight);
            
            // MapEngine might need internal updates for its camera
            mapEngine.Initialize(); 
        }

        // Handle input events
        mapEngine.HandleInput(systemParams);
        
        // Update components
        mapEngine.Update(systemParams);

        // --- Core Propagation Heatmap Generation ---
        // To keep the main thread responsive, we calculate the heatmap sparsely.
        // In a full production scenario with complex terrain, this would run in a compute shader.
        // We will compute a grid over the screen space and draw colored rectangles.
        
        BeginTextureMode(heatmapTarget);
        ClearBackground(BLANK);
        
        // Grid resolution for heatmap calculation (performance trade-off)
        const int stepSize = 8; 
        
        // Only calculate if TX coordinates are set
        if (systemParams.txLatitude != 0.0 && systemParams.txLongitude != 0.0) {
            Vector2 centerTile = RFSim::MapEngine::LatLonToTile(mapEngine.GetViewLat(), mapEngine.GetViewLon(), mapEngine.GetZoom());
            
            for (int y = 0; y < GetScreenHeight(); y += stepSize) {
                for (int x = 0; x < GetScreenWidth(); x += stepSize) {
                    
                    // Convert screen pixel to Lat/Lon
                    Vector2 pixelPos = { 
                        (float)(x - GetScreenWidth() / 2), 
                        (float)(y - GetScreenHeight() / 2) 
                    };
                    
                    RFSim::GeoCoord targetCoord = RFSim::MapEngine::PixelToLatLon(pixelPos, centerTile, mapEngine.GetZoom());
                    
                    // Calculate power at this pixel
                    float rxPowerDbm = RFSim::PropagationEngine::CalculateReceivedPower(systemParams, targetCoord.lat, targetCoord.lon);
                    
                    // If signal is above sensitivity, render it
                    if (rxPowerDbm != -999.0f) {
                        // Discrete color bands for better clarity on reliability
                        float margin = rxPowerDbm - systemParams.rxSensitivityDbm;
                        Color c;
                        
                        if (margin < 10.0f)      c = Color{ 0, 121, 241, 140 };  // Blue (Very Weak - Unreliable)
                        else if (margin < 20.0f) c = Color{ 0, 255, 255, 160 };  // Cyan (Weak - Marginal)
                        else if (margin < 30.0f) c = Color{ 0, 228, 48, 190 };   // Green (Good - Reliable)
                        else if (margin < 45.0f) c = Color{ 255, 203, 0, 220 };  // Yellow (Strong)
                        else                     c = Color{ 230, 41, 55, 240 };  // Red (Very Strong)
                        
                        DrawRectangle(x, y, stepSize, stepSize, c);
                    }
                }
            }
            
            // Draw TX Marker
            Vector2 txTile = RFSim::MapEngine::LatLonToTile(systemParams.txLatitude, systemParams.txLongitude, mapEngine.GetZoom());
            float txPixelX = (txTile.x - centerTile.x) * 256.0f + GetScreenWidth() / 2.0f;
            float txPixelY = (txTile.y - centerTile.y) * 256.0f + GetScreenHeight() / 2.0f;
            
            DrawCircle((int)txPixelX, (int)txPixelY, 6.0f, BLACK);
            DrawCircleLines((int)txPixelX, (int)txPixelY, 6.0f, WHITE);
            DrawCircleLines((int)txPixelX, (int)txPixelY, 8.0f, RED);
        }
        
        EndTextureMode();

        // --- Render Frame ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw Map or 3D Surface
        mapEngine.Draw(systemParams);

        // Draw Heatmap Overlay (Only in 2D mode)
        if (systemParams.displayMode == RFSim::DisplayMode::Map2D) {
            Rectangle sourceRec = { 0.0f, 0.0f, (float)heatmapTarget.texture.width, -(float)heatmapTarget.texture.height };
            Vector2 position = { 0.0f, 0.0f };
            DrawTextureRec(heatmapTarget.texture, sourceRec, position, WHITE);
        }

        // Draw UI
        uiManager.RenderUI(systemParams);

        EndDrawing();
    }

    // Cleanup
    UnloadRenderTexture(heatmapTarget);
    mapEngine.Shutdown();
    uiManager.Shutdown();
    CloseWindow();

    return 0;
}