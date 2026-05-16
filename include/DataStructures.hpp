/*
 * RFSim - Radio Propagation Simulator
 * Copyright (c) 2026 damplex23. All rights reserved.
 * 
 * This software is licensed for PERSONAL and NON-COMMERCIAL use only.
 * Selling or redistributing this software for profit is strictly prohibited.
 * See LICENSE.txt in the project root for full license details.
 */

#ifndef DATA_STRUCTURES_HPP
#define DATA_STRUCTURES_HPP

#include <vector>
#include <string>

namespace RFSim {

enum class AntennaType : int {
    Isotropic = 0,
    CenterFedDipole,
    VerticalMonopole,
    MobileGroundPlaneWhip,
    Yagi3Element,
    Yagi10Element,
    ParabolicMicrowaveDish,
    MagneticShieldedLoop,
    FlatMicrostripPatch,
    Count
};

enum class Polarization : int {
    Vertical = 0,
    Horizontal = 1
};

enum class ClimateZone : int {
    ContinentalTemperate = 0,
    MaritimeLand,
    MaritimeSea,
    Desert,
    Equatorial,
    Count
};

enum class DisplayMode : int {
    Map2D = 0,
    Surface3D,
    Globe3D
};

struct RFParameters {
    // ... (TX/RX variables unchanged)
    float txPowerWatts = 100.0f;
    float txLineLossDb = 1.5f;
    float txAntennaGainDbi = 2.15f;
    float txAntennaHeightMeters = 10.0f;
    float txAzimuthDegrees = 0.0f;
    float txElevationTiltDegrees = 0.0f;
    AntennaType txAntennaType = AntennaType::Isotropic;

    float rxAntennaHeightMeters = 2.0f;
    float rxAntennaGainDbi = 2.15f;
    float rxLineLossDb = 0.5f;
    float rxSensitivityDbm = -110.0f;

    int solarFluxIndex = 150;
    int geomagneticKIndex = 2;
    int sunspotNumber = 70;

    float groundConductivity = 0.005f; 
    float groundPermittivity = 15.0f;  
    float surfaceRefractivity = 301.0f; 
    Polarization polarization = Polarization::Vertical;
    ClimateZone climateZone = ClimateZone::ContinentalTemperate;

    float operatingFrequencyMhz = 144.0f;

    double txLatitude = 0.0;
    double txLongitude = 0.0;

    // Visualization State
    DisplayMode displayMode = DisplayMode::Map2D;
    float surfaceExaggeration = 1.0f;
};

struct TileCoordinate {
    int x;
    int y;
    int z;
};

struct GeoCoord {
    double lat;
    double lon;
};

} // namespace RFSim

#endif // DATA_STRUCTURES_HPP
