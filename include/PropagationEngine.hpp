/*
 * RFSim - Radio Propagation Simulator
 * Copyright (c) 2026 damplex23. All rights reserved.
 * 
 * This software is licensed for PERSONAL and NON-COMMERCIAL use only.
 * Selling or redistributing this software for profit is strictly prohibited.
 * See LICENSE.txt in the project root for full license details.
 */

#ifndef PROPAGATION_ENGINE_HPP
#define PROPAGATION_ENGINE_HPP

#include "DataStructures.hpp"
#include <complex>

namespace RFSim {

class PropagationEngine {
public:
    // Core analytical solver
    static float CalculateReceivedPower(const RFParameters& params, double targetLat, double targetLon);

private:
    // 1. LF/MF Ground Wave (0.1 MHz - 3 MHz)
    static float CalculateGroundWaveLoss(const RFParameters& params, float distanceKm);
    static std::complex<double> ComputeSommerfeldFactor(float frequencyMhz, float distanceKm, float sigma, float epsilon);

    // 2. HF Sky Wave (3 MHz - 30 MHz)
    static float CalculateSkyWaveLoss(const RFParameters& params, float distanceKm);
    static float GetCriticalFrequency(int sfi);
    static float GetDLayerAbsorption(float freqMhz, int kIndex, float theta);

    // 3. VHF/UHF/SHF Line-of-Sight (30 MHz - 6 GHz)
    static float CalculateVHFUHFLoss(const RFParameters& params, float distanceKm);
    static float CalculateKnifeEdgeDiffraction(float v);

    // Utilities
    static float CalculateFreeSpaceLoss(float freqMhz, float distanceKm);
    static float GetEarthRadiusFactor(float surfaceRefractivity);
    static float GetGeometricHorizon(float heightMeters, float kFactor);
};

} // namespace RFSim

#endif // PROPAGATION_ENGINE_HPP
