/*
 * RFSim - Radio Propagation Simulator
 * Copyright (c) 2026 damplex23. All rights reserved.
 * 
 * This software is licensed for PERSONAL and NON-COMMERCIAL use only.
 * Selling or redistributing this software for profit is strictly prohibited.
 * See LICENSE.txt in the project root for full license details.
 */

#include "UIManager.hpp"
#include <array>
#include <string>

namespace RFSim {

UIManager::UIManager() {
}

UIManager::~UIManager() {
}

void UIManager::Initialize() {
    rlImGuiSetup(true);
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ApplyDarkTheme();
    m_logEntries.push_back("System Initialized.");
    m_logEntries.push_back("UI Manager standing by.");
}

void UIManager::Shutdown() {
    rlImGuiShutdown();
}

void UIManager::ApplyDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f); // #1A1A1A Cockpit Slate
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.14f, 0.14f, 0.14f, 0.94f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.05f, 0.05f, 0.05f, 0.53f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.40f, 0.40f, 0.40f, 0.78f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.30f, 0.30f, 0.30f, 0.20f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.40f, 0.40f, 0.40f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.50f, 0.50f, 0.50f, 0.95f);
    style.Colors[ImGuiCol_Tab]                   = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    style.Colors[ImGuiCol_TabActive]             = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.15f, 0.15f, 0.15f, 0.97f);
    style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_DockingPreview]        = ImVec4(0.60f, 0.60f, 0.60f, 0.70f);
    style.Colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);

    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 2.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 0.0f;
}

void UIManager::RenderUI(RFParameters& params) {
    rlImGuiBegin();

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0)); // Transparent background
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("RFSim Dockspace", nullptr, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::PopStyleColor(); // Restore WindowBg

    DrawLeftPanel(params);
    DrawRightPanel(params);
    DrawBottomPanel();

    ImGui::End(); // End Dockspace
    rlImGuiEnd();
}

void UIManager::AddLog(const std::string& message) {
    m_logEntries.push_back(message);
    if (m_logEntries.size() > 100) m_logEntries.erase(m_logEntries.begin());
}

void UIManager::DrawLeftPanel(RFParameters& params) {
    ImGui::Begin("Station Transmit Configurations");

    ImGui::Text("Global Settings");
    ImGui::InputFloat("Operating Frequency (MHz)", &params.operatingFrequencyMhz, 0.1f, 1.0f, "%.3f");
    
    ImGui::Separator();
    ImGui::Text("Transmitter (TX) Hardware Variables");
    ImGui::InputFloat("TX Power (Watts)", &params.txPowerWatts, 1.0f, 10.0f, "%.1f");
    ImGui::InputFloat("TX Line Loss (dB)", &params.txLineLossDb, 0.1f, 1.0f, "%.2f");
    ImGui::InputFloat("TX Antenna Gain (dBi)", &params.txAntennaGainDbi, 0.1f, 1.0f, "%.2f");
    ImGui::InputFloat("TX Antenna Height (m AGL)", &params.txAntennaHeightMeters, 1.0f, 5.0f, "%.1f");
    ImGui::InputFloat("TX Azimuth (Degrees)", &params.txAzimuthDegrees, 1.0f, 10.0f, "%.1f");
    ImGui::InputFloat("TX Elevation Tilt (Degrees)", &params.txElevationTiltDegrees, 1.0f, 5.0f, "%.1f");

    const char* antennaItems[] = {
        "Isotropic", "Center-Fed Dipole", "Vertical Monopole", 
        "Mobile Ground-Plane Whip", "3-Element Yagi-Uda", "10-Element Long-Boom Yagi",
        "Parabolic Microwave Dish", "Magnetic Shielded Loop", "Flat Microstrip Patch"
    };
    int currentAntenna = static_cast<int>(params.txAntennaType);
    if (ImGui::Combo("TX Antenna Type", &currentAntenna, antennaItems, IM_ARRAYSIZE(antennaItems))) {
        params.txAntennaType = static_cast<AntennaType>(currentAntenna);
    }

    ImGui::Separator();
    ImGui::Text("Transmitter Coordinates (Interactive via Map)");
    ImGui::InputDouble("TX Latitude", &params.txLatitude, 0.0001, 0.001, "%.6f");
    ImGui::InputDouble("TX Longitude", &params.txLongitude, 0.0001, 0.001, "%.6f");

    ImGui::Separator();
    ImGui::Text("Visualization & Display");
    const char* displayModes[] = { "2D Map View", "3D Surface Plot", "3D Global View" };
    int currentMode = static_cast<int>(params.displayMode);
    if (ImGui::Combo("Display Mode", &currentMode, displayModes, IM_ARRAYSIZE(displayModes))) {
        params.displayMode = static_cast<DisplayMode>(currentMode);
    }
    
    if (params.displayMode == DisplayMode::Surface3D || params.displayMode == DisplayMode::Globe3D) {
        if (params.displayMode == DisplayMode::Surface3D) {
            ImGui::InputFloat("Vertical Exaggeration", &params.surfaceExaggeration, 0.1f, 1.0f, "%.1f");
        }
        ImGui::TextDisabled("(Use Mouse to Orbit/Zoom 3D View)");
    }

    ImGui::End();
}

void UIManager::DrawRightPanel(RFParameters& params) {
    ImGui::Begin("Receiver System & Environment");

    ImGui::Text("Receiver (RX) Target Variables");
    ImGui::InputFloat("RX Antenna Height (m AGL)", &params.rxAntennaHeightMeters, 0.1f, 1.0f, "%.1f");
    ImGui::InputFloat("RX Antenna Gain (dBi)", &params.rxAntennaGainDbi, 0.1f, 1.0f, "%.2f");
    ImGui::InputFloat("RX Line Loss (dB)", &params.rxLineLossDb, 0.1f, 1.0f, "%.2f");
    ImGui::InputFloat("RX Sensitivity (dBm)", &params.rxSensitivityDbm, 1.0f, 5.0f, "%.1f");

    ImGui::Separator();
    ImGui::Text("Space Weather & Ionospheric Variables");
    ImGui::InputInt("Solar Flux Index (SFI)", &params.solarFluxIndex, 1, 10);
    ImGui::InputInt("Geomagnetic K-Index", &params.geomagneticKIndex, 1, 1);
    ImGui::InputInt("Sunspot Number (SSN)", &params.sunspotNumber, 1, 10);

    ImGui::Separator();
    ImGui::Text("Atmospheric & Ground Dielectric Variables");
    ImGui::InputFloat("Ground Conductivity (S/m)", &params.groundConductivity, 0.001f, 0.01f, "%.4f");
    ImGui::InputFloat("Ground Permittivity (Epsilon)", &params.groundPermittivity, 1.0f, 5.0f, "%.1f");
    ImGui::InputFloat("Surface Refractivity (N-units)", &params.surfaceRefractivity, 1.0f, 5.0f, "%.1f");

    const char* polarizationItems[] = { "Vertical", "Horizontal" };
    int currentPol = static_cast<int>(params.polarization);
    if (ImGui::Combo("Polarization Type", &currentPol, polarizationItems, IM_ARRAYSIZE(polarizationItems))) {
        params.polarization = static_cast<Polarization>(currentPol);
    }

    const char* climateItems[] = {
        "Continental Temperate", "Maritime Land", "Maritime Sea", "Desert", "Equatorial"
    };
    int currentClimate = static_cast<int>(params.climateZone);
    if (ImGui::Combo("Climate Zone", &currentClimate, climateItems, IM_ARRAYSIZE(climateItems))) {
        params.climateZone = static_cast<ClimateZone>(currentClimate);
    }

    ImGui::End();
}

void UIManager::DrawBottomPanel() {
    ImGui::Begin("System Debug Log & Status");
    
    ImGui::BeginChild("LogRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& log : m_logEntries) {
        ImGui::TextUnformatted(log.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::Text("Application Running | C++20 | ImGui Docking | Raylib 5.0 Backend Active");

    ImGui::End();
}

} // namespace RFSim