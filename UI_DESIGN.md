# UI Design Documentation

## Overview
This document describes the User Interface (UI) design and implementation for the Embedded Challenge project. The UI is designed for a 2.4" TFT touchscreen display (320x240 pixels) running on an Adafruit Feather 32u4 with TFT FeatherWing V2.

## Hardware Specifications
- **Display**: Adafruit ILI9341 TFT (320x240 pixels, landscape orientation)
- **Touchscreen**: Adafruit TSC2007 (V2 FeatherWing, I2C interface)
- **Microcontroller**: Adafruit Feather 32u4
- **Display Pins**: CS=9, DC=10, RST=-1 (shared with Arduino reset)

## Screen States
The UI consists of four main screens:

1. **SCREEN_HOME** - Main statistics screen
2. **SCREEN_TREMOR** - Tremor monitoring screen
3. **SCREEN_DYSKINESIA** - Dyskinesia monitoring screen
4. **SCREEN_FOG_WARNING** - Freezing of Gait warning overlay

## Screen Layouts

### Home Screen (SCREEN_HOME)
**Purpose**: Display current statistics for both Tremor and Dyskinesia

**Layout**:
- **Title**: "Your Stats:" (centered, text size 3, white)
- **Tremor Section**:
  - Label: "Tremor:" (centered, text size 2, blue)
  - Value: Percentage (centered, text size 3, white) - e.g., "45.2 %"
  - Position: Y=60 (label), Y=85 (value)
- **Dyskinesia Section**:
  - Label: "Dyskinesia:" (centered, text size 2, orange)
  - Value: Percentage (centered, text size 3, white) - e.g., "32.1 %"
  - Position: Y=130 (label), Y=155 (value)
- **Navigation Triangles**:
  - **Left Triangle**: Orange triangle with "D" (Dyskinesia)
    - Position: Left edge (x=0), spans y=20 to y=220
    - Width: 40px extending into screen
    - Swipe left from this triangle → Navigate to Dyskinesia screen
  - **Right Triangle**: Blue triangle with "T" (Tremor)
    - Position: Right edge (x=320), spans y=20 to y=220
    - Width: 40px extending into screen
    - Swipe right from this triangle → Navigate to Tremor screen

**Update Behavior**:
- Percentages update only when values change by more than 0.1%
- Prevents flickering from constant redraws
- Uses event-driven updates triggered by sensor data changes

### Tremor Screen (SCREEN_TREMOR)
**Purpose**: Detailed tremor monitoring with graph visualization

**Layout**:
- **Title**: "TREMOR" (text size 2, blue, centered at x=100, y=10)
- **Graph Area**:
  - Label: "Tremor Intensity" (text size 1, white, x=50, y=35)
  - Border: White rectangle at (50, 50, 260, 150)
  - Axes: White lines for X and Y axes
  - Position: Adjusted to avoid left H triangle (starts at x=50)
- **Intensity Display**: 
  - Text: "Intensity: [value]" (text size 2, blue)
  - Position: (50, 210)
  - Updates when intensity changes by more than 0.01
- **Warning Box** (when tremor detected):
  - Red box at bottom (y=200-240, full width)
  - Message: "Tremor detected!" (text size 2, white)
  - Appears/disappears based on detection state
- **Navigation Triangle**:
  - **Left H Triangle**: Blue triangle with "H" (History/Home)
    - Position: Left edge (x=0), spans y=20 to y=220
    - Width: 40px
    - Swipe right from this triangle → Navigate to Home screen

**Update Behavior**:
- Graph area reserved for future FFT data visualization
- Intensity value updates only when changed
- Warning box appears/disappears based on detection state

### Dyskinesia Screen (SCREEN_DYSKINESIA)
**Purpose**: Detailed dyskinesia monitoring with graph visualization

**Layout**:
- **Title**: "DYSKINESIA" (text size 2, orange, centered at x=70, y=10)
- **Graph Area**:
  - Label: "Dyskinesia Intensity" (text size 1, white, x=10, y=35)
  - Border: White rectangle at (10, 50, 260, 150)
  - Axes: White lines for X and Y axes
  - Position: Adjusted to avoid right H triangle (ends at x=270)
- **Intensity Display**:
  - Text: "Intensity: [value]" (text size 2, orange)
  - Position: (10, 210)
  - Updates when intensity changes by more than 0.01
- **Warning Box** (when dyskinesia detected):
  - Red box at bottom (y=200-240, full width)
  - Message: "Dyskinesia detected!" (text size 2, white)
  - Appears/disappears based on detection state
- **Navigation Triangle**:
  - **Right H Triangle**: Orange triangle with "H" (History/Home)
    - Position: Right edge (x=320), spans y=20 to y=220
    - Width: 40px
    - Swipe left from this triangle → Navigate to Home screen

**Update Behavior**:
- Graph area reserved for future FFT data visualization
- Intensity value updates only when changed
- Warning box appears/disappears based on detection state

### FOG Warning Screen (SCREEN_FOG_WARNING)
**Purpose**: Critical alert for Freezing of Gait detection

**Layout**:
- **Background**: Full screen red (RED color)
- **Warning Text**:
  - "Warning!" (text size 2, white, x=40, y=60)
  - "Freezing of" (text size 2, white, x=20, y=100)
  - "Gait detected" (text size 2, white, x=20, y=130)
- **Dismiss Button**:
  - White rounded rectangle (x=100, y=180, width=120, height=40, radius=5)
  - Text: "Dismiss" (text size 2, red, centered)
  - Touch area: x=100-220, y=180-220

**Behavior**:
- Overlays any current screen when FOG is detected
- User must tap "Dismiss" button to return to home screen
- Auto-dismisses when FOG is no longer detected

## Touch and Swipe Detection

### Touch Calibration
- **Coordinate System**: Origin (0,0) at top-left corner
- **Axes Mapping**: Touchscreen axes are rotated 90° relative to display
  - Physical left/right swipes → Touchscreen Y-axis → Screen X-axis
  - Physical up/down swipes → Touchscreen X-axis → Screen Y-axis
- **Raw Values**: TSC2007 returns 0-4095 for both axes
- **Mapping**: `map(p.y, 0, 4095, 320, 0)` for X, `map(p.x, 0, 4095, 0, 240)` for Y

### Swipe Detection Parameters
- **Minimum Touch Duration**: 50ms (filters out noise)
- **Minimum Swipe Movement**: 5 pixels (distinguishes swipes from taps)
- **Touch Cooldown**: 300ms (prevents rapid re-detection)
- **Swipe Thresholds** (configurable by sensitivity level):
  - **Mild**: 40 pixels (~12% of screen width)
  - **Moderate**: 50 pixels (~16% of screen width) - default
  - **Severe**: 70 pixels (~22% of screen width)
- **Swipe Ratio**: Horizontal movement must be 1.5x greater than vertical movement

### Swipe Navigation Rules

**Home Screen**:
- Swipe **left** (from left D triangle area) → Dyskinesia screen
- Swipe **right** (from right T triangle area) → Tremor screen

**Tremor Screen**:
- Swipe **right** (from left H triangle, x<33, y=25-210) → Home screen
- All other swipes are ignored

**Dyskinesia Screen**:
- Swipe **left** (from right H triangle, x>135, y=20-220) → Home screen
- All other swipes are ignored

**FOG Warning Screen**:
- Tap "Dismiss" button → Home screen
- Swipes are disabled (must use button)

### Expanding Triangle Visual Feedback
During swipe gestures, the navigation triangles expand to provide visual feedback:
- **Home Screen**:
  - Left D triangle expands leftward when swiping left
  - Right T triangle expands rightward when swiping right
- **Tremor Screen**:
  - Left H triangle expands rightward when swiping right
- **Dyskinesia Screen**:
  - Right H triangle expands leftward when swiping left

**Expansion Details**:
- Base width: 40px
- Maximum expansion: 100px (or swipe distance, whichever is smaller)
- Triangle expands proportionally with swipe distance
- Automatically restores to original size when touch ends

## Color Scheme

| Color | Hex Value | Usage |
|-------|-----------|-------|
| BLACK | 0x0000 | Background, text clearing |
| WHITE | 0xFFFF | Text, borders, axes |
| BLUE | 0x001F | Tremor-related elements |
| ORANGE | 0xFDA0 | Dyskinesia-related elements |
| RED | 0xF800 | Warning boxes, FOG warning screen |
| GREEN | 0x07E0 | (Available) |
| CYAN | 0x07FF | (Available) |
| YELLOW | 0xFFE0 | (Available) |
| DARKGRAY | 0x4208 | (Available) |
| LIGHTGRAY | 0xC618 | (Available) |

## Data Integration

### Sensor Data Structure
The UI receives data from the P team (detection algorithms) via the `DetectionData` structure:

```cpp
struct DetectionData {
    float intensity;        // Intensity value (0.0 to max, typically 0-10)
    unsigned long timestamp; // Timestamp of detection
    bool detected;          // Whether condition is currently detected
};
```

### Update Functions
The P team should call these functions to update the UI:

- `updateTremorData(float intensity, bool detected)` - Updates tremor data
- `updateDyskinesiaData(float intensity, bool detected)` - Updates dyskinesia data
- `updateFOGData(float intensity, bool detected)` - Updates Freezing of Gait data

### Data Flow
1. P team calls update functions with new detection data
2. UI system flags data as changed (`tremorDataChanged`, `dyskinesiaDataChanged`)
3. In main loop, UI checks for changed data
4. Only changed elements are redrawn (event-driven updates)
5. Prevents unnecessary redraws and flickering

### Intensity to Percentage Conversion
- **Formula**: `percentage = (intensity / 10.0) * 100.0`
- **Clamping**: Values are clamped to maximum 100.0%
- **Display**: Shown with 1 decimal place (e.g., "45.2 %")

## Screen Inactivity
- **Timeout**: 10 seconds of inactivity
- **Behavior**: Automatically returns to Home screen
- **Exceptions**: FOG warning screen does not auto-return
- **Reset**: Timer resets on any screen navigation or touch interaction

## Performance Optimizations

### Event-Driven Updates
- UI elements only redraw when data actually changes
- Change detection thresholds:
  - Percentages: 0.1% change required
  - Intensity values: 0.01 change required
  - Detection states: Any change triggers update

### Partial Screen Updates
- Only dynamic elements are updated, not entire screens
- Static elements (titles, borders, triangles) drawn once
- Reduces flickering and improves performance

### Flash Memory Optimization
- Consolidated triangle drawing function (reduced code duplication)
- Removed unused/commented code
- Reduced debug Serial output
- Total flash usage: Under 28,672 bytes (target limit)

## Future Enhancements

### Graph Visualization
- Graph areas are reserved for future FFT data visualization
- Update functions ready: `updateTremorScreen()`, `updateDyskinesiaScreen()`
- Graph updates will only occur when new FFT data arrives

### History Features
- History tracking structures defined but currently commented out
- Can be enabled for future history chart functionality
- Circular buffer design supports up to 100 history entries

## Technical Notes

### Touchscreen Coordinate Mapping
The TSC2007 touchscreen has rotated axes relative to the display:
- Touchscreen X → Screen Y (vertical)
- Touchscreen Y → Screen X (horizontal, inverted)

This is handled in the `handleTouch()` function with coordinate mapping.

### Text Sizing
- Text size 1: ~6x8 pixels per character
- Text size 2: ~12x16 pixels per character
- Text size 3: ~18x24 pixels per character

### Screen Dimensions
- Width: 320 pixels
- Height: 240 pixels
- Landscape orientation (rotation 3)

## Code Organization

### Main Functions
- `setup()` - Initialization
- `loop()` - Main event loop
- `initializeDisplay()` - Display setup
- `initializeTouch()` - Touchscreen setup
- `updateSensorData()` - Sensor data processing
- `handleTouch()` - Touch input handling
- `detectSwipe()` - Swipe gesture detection

### Drawing Functions
- `drawHomeScreen()` - Draw home screen
- `updateHomeScreenStats()` - Update home screen statistics
- `drawTremorScreen()` - Draw tremor screen
- `updateTremorScreen()` - Update tremor screen elements
- `drawDyskinesiaScreen()` - Draw dyskinesia screen
- `updateDyskinesiaScreen()` - Update dyskinesia screen elements
- `drawFOGWarningScreen()` - Draw FOG warning overlay
- `drawWarningBox()` - Draw warning box at bottom of screen
- `drawExpandingTriangle()` - Draw expanding triangle during swipe

### Helper Functions
- `getSwipeThreshold()` - Get swipe threshold based on sensitivity
- `updateTremorData()` - Update tremor data (called by P team)
- `updateDyskinesiaData()` - Update dyskinesia data (called by P team)
- `updateFOGData()` - Update FOG data (called by P team)

