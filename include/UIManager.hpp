/*
 * RFSim - Radio Propagation Simulator
 * Copyright (c) 2026 damplex23. All rights reserved.
 * 
 * This software is licensed for PERSONAL and NON-COMMERCIAL use only.
 * Selling or redistributing this software for profit is strictly prohibited.
 * See LICENSE.txt in the project root for full license details.
 */

#ifndef UI_MANAGER_HPP
#define UI_MANAGER_HPP

#include "imgui.h"
#include "rlImGui.h"
#include "DataStructures.hpp"

namespace RFSim {

class UIManager {
public:
    UIManager();
    ~UIManager();

    void Initialize();
    void Shutdown();

    // Renders the entire ImGui interface
    void RenderUI(RFParameters& params);

    // Custom Dark Slate Cockpit Layout
    void ApplyDarkTheme();

    // Logging
    void AddLog(const std::string& message);

private:
    void DrawLeftPanel(RFParameters& params);
    void DrawRightPanel(RFParameters& params);
    void DrawBottomPanel();
    
    // UI State
    bool m_showDebugLog = true;
    std::vector<std::string> m_logEntries;
};

} // namespace RFSim

#endif // UI_MANAGER_HPP
