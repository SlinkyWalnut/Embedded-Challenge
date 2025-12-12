#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

// Data structure for sharing tremor/dyskinesia detection data
// between P team (detection algorithms) and U team (UI)

struct DetectionData {
    float intensity;        // Intensity value (0.0 to max)
    unsigned long timestamp; // Timestamp of detection
    bool detected;          // Whether condition is currently detected
};

// Global variables for data sharing
// P team should update these, U team reads them
extern DetectionData tremorData;
extern DetectionData dyskinesiaData;
extern DetectionData fogData;  // Freezing of Gait

// Function declarations for P team to call
void updateTremorData(float intensity, bool detected);
void updateDyskinesiaData(float intensity, bool detected);
void updateFOGData(float intensity, bool detected);

#endif

