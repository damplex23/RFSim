/*
 * RFSim - Radio Propagation Simulator
 * Copyright (c) 2026 damplex23. All rights reserved.
 * 
 * This software is licensed for PERSONAL and NON-COMMERCIAL use only.
 * Selling or redistributing this software for profit is strictly prohibited.
 * See LICENSE.txt in the project root for full license details.
 */

#include "PropagationEngine.hpp"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace RFSim {

float PropagationEngine::CalculateReceivedPower(const RFParameters& params, double targetLat, double targetLon) {
    // 1. Calculate Geodetic Distance and Bearing
    double dLat = (targetLat - params.txLatitude) * M_PI / 180.0;
    double dLon = (targetLon - params.txLongitude) * M_PI / 180.0;
    double lat1 = params.txLatitude * M_PI / 180.0;
    double lat2 = targetLat * M_PI / 180.0;

    double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
               std::sin(dLon / 2) * std::sin(dLon / 2) * std::cos(lat1) * std::cos(lat2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    double distanceKm = 6371.0 * c;

    if (distanceKm < 0.001) distanceKm = 0.001;

    // Calculate Bearing from TX to Target for Antenna Pattern
    double y_bearing = std::sin(dLon) * std::cos(lat2);
    double x_bearing = std::cos(lat1) * std::sin(lat2) - std::sin(lat1) * std::cos(lat2) * std::cos(dLon);
    double bearingDegrees = std::atan2(y_bearing, x_bearing) * 180.0 / M_PI;
    if (bearingDegrees < 0) bearingDegrees += 360.0;

    // 2. Determine Effective Antenna Gain based on Pattern
    float effectiveTxGain = params.txAntennaGainDbi;
    float angleDiff = std::abs((float)bearingDegrees - params.txAzimuthDegrees);
    if (angleDiff > 180.0f) angleDiff = 360.0f - angleDiff;

    switch (params.txAntennaType) {
        case AntennaType::Isotropic:
            // Constant gain in all directions
            break;

        case AntennaType::CenterFedDipole:
        case AntennaType::VerticalMonopole:
        case AntennaType::MobileGroundPlaneWhip:
            // Toroidal pattern: Gain is maximum at horizon, zero at poles.
            // Simplified horizontal approximation (assuming vertical orientation): 
            // Gain is constant in azimuth.
            break;

        case AntennaType::MagneticShieldedLoop:
            {
                // Figure-8 pattern: Nulls at 90/270 degrees relative to loop plane
                float angleRad = angleDiff * (float)M_PI / 180.0f;
                float patternFactor = std::abs(std::cos(angleRad));
                float patternLoss = -20.0f * std::log10(std::max(patternFactor, 0.0316f)); // Max 30dB null
                effectiveTxGain -= patternLoss;
            }
            break;

        case AntennaType::Yagi3Element:
        case AntennaType::Yagi10Element:
        case AntennaType::ParabolicMicrowaveDish:
        case AntennaType::FlatMicrostripPatch:
            {
                float beamwidth = 360.0f;
                float fbr = 20.0f; // Front-to-back ratio

                if (params.txAntennaType == AntennaType::Yagi3Element) { beamwidth = 60.0f; fbr = 15.0f; }
                else if (params.txAntennaType == AntennaType::Yagi10Element) { beamwidth = 30.0f; fbr = 25.0f; }
                else if (params.txAntennaType == AntennaType::ParabolicMicrowaveDish) { beamwidth = 5.0f; fbr = 35.0f; }
                else if (params.txAntennaType == AntennaType::FlatMicrostripPatch) { beamwidth = 90.0f; fbr = 20.0f; }

                // Gaussian pattern approximation
                float patternLoss = 12.0f * std::pow(angleDiff / beamwidth, 2.0f);
                if (patternLoss > fbr) patternLoss = fbr;
                effectiveTxGain -= patternLoss;
            }
            break;

        default:
            break;
    }

    // 3. Solve Propagation
    float freqMhz = params.operatingFrequencyMhz;
    float rxPowerDbm = -999.0f;
    float txEirpDbm = 10.0f * std::log10(params.txPowerWatts * 1000.0f) - params.txLineLossDb + effectiveTxGain;

    if (freqMhz < 3.0f) {
        float lossDb = CalculateGroundWaveLoss(params, (float)distanceKm);
        rxPowerDbm = txEirpDbm - lossDb - params.rxLineLossDb + params.rxAntennaGainDbi;
    } else if (freqMhz >= 3.0f && freqMhz < 30.0f) {
        float gwLossDb = CalculateGroundWaveLoss(params, (float)distanceKm);
        float swLossDb = CalculateSkyWaveLoss(params, (float)distanceKm);
        
        float pGwDbm = txEirpDbm - gwLossDb - params.rxLineLossDb + params.rxAntennaGainDbi;
        float pSwDbm = txEirpDbm - swLossDb - params.rxLineLossDb + params.rxAntennaGainDbi;

        double pGwWatts = std::pow(10.0, pGwDbm / 10.0);
        double pSwWatts = std::pow(10.0, pSwDbm / 10.0);
        double pTotalWatts = pGwWatts + pSwWatts;
        if (pTotalWatts > 0.0) rxPowerDbm = 10.0f * (float)std::log10(pTotalWatts);
    } else {
        float lossDb = CalculateVHFUHFLoss(params, (float)distanceKm);
        rxPowerDbm = txEirpDbm - lossDb - params.rxLineLossDb + params.rxAntennaGainDbi;
    }

    if (rxPowerDbm < params.rxSensitivityDbm) return -999.0f;
    return rxPowerDbm;
}

float PropagationEngine::CalculateFreeSpaceLoss(float freqMhz, float distanceKm) {
    return 20.0f * std::log10(distanceKm) + 20.0f * std::log10(freqMhz) + 32.44f;
}

std::complex<double> PropagationEngine::ComputeSommerfeldFactor(float frequencyMhz, float distanceKm, float sigma, float epsilon) {
    double freqHz = frequencyMhz * 1e6;
    double omega = 2.0 * M_PI * freqHz;
    double epsilon0 = 8.8541878128e-12; // Farads/meter
    
    // Complex dielectric constant
    std::complex<double> eps_c(epsilon, -sigma / (omega * epsilon0));
    
    // Numerical distance (p) calculation for vertical polarization (simplified flat earth)
    double k0 = omega / 299792458.0; // Wavenumber in free space
    double d_meters = distanceKm * 1000.0;
    
    std::complex<double> p = -std::complex<double>(0.0, 1.0) * (k0 * d_meters / 2.0) * ((eps_c - 1.0) / (eps_c * eps_c));
    
    // Sommerfeld attenuation function A(p) approximation
    // A(p) approx 1 / (1 + p) for large p, or 1 for small p
    std::complex<double> A = 1.0 / (1.0 + p);
    
    return A;
}

float PropagationEngine::CalculateGroundWaveLoss(const RFParameters& params, float distanceKm) {
    float fspl = CalculateFreeSpaceLoss(params.operatingFrequencyMhz, distanceKm);
    
    // Calculate complex surface attenuation factor
    std::complex<double> A = ComputeSommerfeldFactor(params.operatingFrequencyMhz, distanceKm, params.groundConductivity, params.groundPermittivity);
    
    double lossFactorLinear = std::abs(A);
    float surfaceLossDb = 0.0f;
    if (lossFactorLinear > 0.0) {
        surfaceLossDb = -20.0f * (float)std::log10(lossFactorLinear);
    } else {
        surfaceLossDb = 1000.0f; // effectively infinite loss
    }
    
    return fspl + surfaceLossDb;
}

float PropagationEngine::GetCriticalFrequency(int sfi) {
    // Approximate critical vertical reflection frequency based on SFI
    // N_max derived from SFI. N_max ~ SFI * 1e10
    double nMax = (double)sfi * 1.0e10;
    return 9.0f * (float)std::sqrt(nMax) / 1e6f; // MHz
}

float PropagationEngine::GetDLayerAbsorption(float freqMhz, int kIndex, float theta) {
    // D-layer sponge attenuation scaled by Geomagnetic K-Index
    // Loss_dB = (C * (1.0 + 0.1 * K_index)) / ((frequency_mhz + gyro_freq) ^ 2 * cos(theta))
    float gyroFreqMhz = 1.2f; // Typical gyrofrequency
    float c = 500.0f; // Empirical constant
    
    float cosTheta = std::cos(theta);
    if (cosTheta < 0.01f) cosTheta = 0.01f;
    
    float denom = std::pow(freqMhz + gyroFreqMhz, 2.0f) * cosTheta;
    return (c * (1.0f + 0.1f * (float)kIndex)) / denom;
}

float PropagationEngine::CalculateSkyWaveLoss(const RFParameters& params, float distanceKm) {
    float hF2 = 300.0f; // Typical F2 layer virtual height in km
    
    // Calculate angle of incidence (theta)
    float dHalf = distanceKm / 2.0f;
    float thetaRad = std::atan(dHalf / hF2);
    
    float foF2 = GetCriticalFrequency(params.solarFluxIndex);
    
    // Secant Law for Maximum Usable Frequency (MUF)
    float mufMhz = foF2 * (1.0f / std::cos(thetaRad));
    
    if (params.operatingFrequencyMhz > mufMhz) {
        return 9999.0f; // Operating frequency > MUF, infinite loss (penetrates ionosphere)
    }
    
    // Slant distance path loss
    float slantDistanceKm = 2.0f * std::sqrt(dHalf * dHalf + hF2 * hF2);
    float fspl = CalculateFreeSpaceLoss(params.operatingFrequencyMhz, slantDistanceKm);
    
    // D-Layer absorption
    float dLayerLossDb = GetDLayerAbsorption(params.operatingFrequencyMhz, params.geomagneticKIndex, thetaRad);
    
    return fspl + dLayerLossDb;
}

float PropagationEngine::GetEarthRadiusFactor(float surfaceRefractivity) {
    // k = 1 / (1 - (a / 1e6) * (dN/dh))
    // Standard atmosphere N = 301, dN/dh approx -39 N-units/km
    // Approximate relation: k = 157 / (157 - dN/dh)
    float dNdh = -39.0f * (surfaceRefractivity / 301.0f);
    return 157.0f / (157.0f + dNdh);
}

float PropagationEngine::GetGeometricHorizon(float heightMeters, float kFactor) {
    float earthRadiusKm = 6371.0f;
    return std::sqrt(2.0f * kFactor * earthRadiusKm * (heightMeters / 1000.0f));
}

float PropagationEngine::CalculateKnifeEdgeDiffraction(float v) {
    if (v <= -1.0f) return 0.0f;
    if (v <= 0.0f) return 20.0f * std::log10(0.5f - 0.62f * v);
    if (v <= 1.0f) return 20.0f * std::log10(0.5f * std::exp(-0.95f * v));
    if (v <= 2.4f) return 20.0f * std::log10(0.4f - std::sqrt(0.1184f - pow(0.38f - 0.1f * v, 2.0f)));
    return 20.0f * std::log10(0.225f / v);
}

float PropagationEngine::CalculateVHFUHFLoss(const RFParameters& params, float distanceKm) {
    float fspl = CalculateFreeSpaceLoss(params.operatingFrequencyMhz, distanceKm);
    
    float kFactor = GetEarthRadiusFactor(params.surfaceRefractivity);
    float dHorizonTx = GetGeometricHorizon(params.txAntennaHeightMeters, kFactor);
    float dHorizonRx = GetGeometricHorizon(params.rxAntennaHeightMeters, kFactor);
    float maxLosDistanceKm = dHorizonTx + dHorizonRx;
    
    if (distanceKm <= maxLosDistanceKm) {
        return fspl;
    } else {
        // --- Smooth-Earth Diffraction (Harsh OTH Model) ---
        // Beyond the radio horizon, signal strength drops precipitously.
        float excessDistance = distanceKm - maxLosDistanceKm;
        
        // Beta parameter for smooth-earth diffraction (frequency dependent)
        // Beta ~ 0.03 * f^(1/3) in units of dB/km
        float beta = 0.035f * std::pow(params.operatingFrequencyMhz, 0.333f); 
        
        // Additional diffraction loss using aggressive slope for Telecommunications Physics
        float diffractionLossDb = 30.0f * std::log10(distanceKm / maxLosDistanceKm) + (beta * excessDistance * 20.0f);
        
        return fspl + diffractionLossDb;
    }
}

} // namespace RFSim