# ESP32-S3 Smart Clock - Complete User Manual (English)

## Table of Contents
1. [Hardware Setup](#hardware-setup)
2. [Software Installation](#software-installation)
3. [First Time Configuration](#first-time-configuration)
4. [Web Interface Guide](#web-interface-guide)
5. [Serial Console Guide](#serial-console-guide)
6. [Alarm System](#alarm-system)
7. [Timer System](#timer-system)
8. [WiFi Management](#wifi-management)
9. [Night Mode](#night-mode)
10. [Melody Customization](#melody-customization)
11. [Smart Home Integration](#smart-home-integration)
12. [Troubleshooting](#troubleshooting)
13. [API Reference](#api-reference)

---

## Hardware Setup

### Required Components

| Component | Specification | Quantity |
|-----------|--------------|----------|
| ESP32-S3 Dev Board | Any ESP32-S3 module | 1 |
| OLED Display | 0.96" 128x64 I2C SSD1306 | 1 |
| Temperature Sensor | AHT20 (I2C) | 1 |
| Pressure Sensor | BMP280 (I2C) | 1 |
| Buzzer | Passive or Active | 1 |
| RGB LED | WS2812B or individual R/G/B | 1 |
| Push Button | Normally Open | 1 |
| Resistors | 10kΩ pull-up for button | 1 |
| Power Supply | 5V USB or DC | 1 |

### Wiring Diagram

```
ESP32-S3 Pin Connections:

Display (I2C OLED SSD1306):
  SDA (GPIO 10) ────────► Display SDA
  SCL (GPIO 11) ────────► Display SCL
  3.3V ─────────────────► Display VCC
  GND ──────────────────► Display GND

Sensors (I2C):
  SDA (GPIO 10) ────────► AHT20 SDA ────► BMP280 SDA
  SCL (GPIO 11) ────────► AHT20 SCL ────► BMP280 SCL
  3.3V ─────────────────► Sensors VCC
  GND ──────────────────► Sensors GND

Buzzer:
  GPIO 12 ──────────────► Buzzer (+)
  GND ──────────────────► Buzzer (-)
  
RGB LED (WS2812B):
  GPIO 21 ──────────────► LED DIN
  5V ───────────────────► LED VCC
  GND ──────────────────► LED GND

Button:
  GPIO 0 (BOOT) ────┬───► Button Terminal 1
  GND ──────────────┴───► Button Terminal 2
  (Internal pull-up resistor enabled)
```

### I2C Address Verification

After connecting sensors, verify their I2C addresses:

**Expected addresses:**
- AHT20: `0x38`
- BMP280: `0x76`

You can check this in Serial Console after first boot - device will report sensor status.

---

## Software Installation

### Step 1: Install Arduino IDE

1. Download Arduino IDE from https://www.arduino.cc/en/software
2. Install version 2.0 or later (recommended)

### Step 2: Add ESP32 Board Support

1. Open Arduino IDE
2. Go to **File → Preferences**
3. In "Additional Board Manager URLs", add:
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```
4. Go to **Tools → Board → Boards Manager**
5. Search for "esp32"
6. Install "esp32 by Espressif Systems" (version 2.0.0 or later)

### Step 3: Install Required Libraries

Open **Tools → Manage Libraries** and install:

```
Library Name              | Version  | Author
--------------------------|----------|------------------
U8g2                      | 2.35.x   | oliver
Adafruit AHTX0            | 2.0.x    | Adafruit
Adafruit BMP280 Library   | 2.6.x    | Adafruit
Adafruit NeoPixel         | 1.12.x   | Adafruit
ArduinoJson               | 6.21.x   | Benoit Blanchon
```

**Installation steps for each library:**
1. Click "Library Manager" icon (books) in left sidebar
2. Search for library name
3. Click "Install"
4. Wait for download and installation

### Step 4: Configure Board Settings

1. Connect ESP32-S3 to computer via USB
2. In Arduino IDE, select:
   - **Board:** "ESP32S3 Dev Module"
   - **Upload Speed:** 921600
   - **USB Mode:** "Hardware CDC and JTAG"
   - **USB CDC On Boot:** "Enabled"
   - **Partition Scheme:** "Huge APP (3MB No OTA/1MB SPIFFS)"
   - **Port:** Select your ESP32-S3 port (e.g., COM3 on Windows, /dev/ttyUSB0 on Linux)

### Step 5: Configure Hardware Settings in Code

Before uploading, verify pin configurations in `ESP32-S3-Z-Clock.ino`:

```cpp
// Line 25-33: Pin definitions
#define BOOT_PIN 0           // Button pin (BOOT)
#define BUZZER_PIN 12        // Buzzer control
#define WS2812_PIN 21        // RGB LED data
#define SDA_PIN 10           // I2C data (display + sensors)
#define SCL_PIN 11           // I2C clock

// Line 20-23: Buzzer type
#define BUZZER_TYPE BUZZER_PASSIVE  // Change to BUZZER_ACTIVE if needed
#define LOW_LEVEL_TRIGGER 1         // 1 for LOW trigger, 0 for HIGH
```

**If your wiring is different, modify these pin numbers!**

### Step 6: Upload Firmware

1. Open `ESP32-S3-Z-Clock.ino` in Arduino IDE
2. Click **Sketch → Verify/Compile** (Ctrl+R)
3. Wait for compilation to complete (may take 1-2 minutes)
4. If successful, click **Upload** button (→)
5. Wait for upload (30-60 seconds)
6. Look for "Done uploading" message

**If upload fails:**
- Try holding BOOT button while uploading
- Check USB cable (use data cable, not charge-only)
- Verify correct port selection
- Try different USB port on computer

### Step 7: First Boot

1. Open **Tools → Serial Monitor** (Ctrl+Shift+M)
2. Set baud rate to **115200**
3. Set line ending to **Newline**
4. Press RESET button on ESP32-S3
5. You should see startup messages:

```
Initializing...
NVS initialized OK
WiFi SSID: Enter_SSID
WARNING: Using default WiFi settings!
Configure WiFi via Serial commands or web interface
Initializing sensors...
AHT20: OK (0x38 on Wire1)
BMP280: OK (direct access)
Setup complete!
> 
```

---

## First Time Configuration

### Method 1: Setup Mode (Easiest - Recommended)

**Perfect for:** First-time setup, no computer needed

**Steps:**

1. **Enter Setup Mode:**
   - During device startup (first 5 seconds), press and hold BOOT button
   - Hold for **10 seconds** until LED turns purple
   - Display shows: "SETUP MODE"
   - Release button

2. **Connect to Setup WiFi:**
   - On your phone/computer, open WiFi settings
   - Connect to network: `ESP32-Clock-Setup`
   - Password: `12345678`

3. **Open Setup Page:**
   - Open web browser
   - Go to: `http://192.168.4.1`
   - You should see setup interface:

```
┌─────────────────────────────────────┐
│     ESP32-S3 Smart Clock Setup      │
├─────────────────────────────────────┤
│                                     │
│  WiFi Configuration                 │
│  ┌────────────────────────────┐    │
│  │ SSID: [MyHomeWiFi        ] │    │
│  └────────────────────────────┘    │
│  ┌────────────────────────────┐    │
│  │ Password: [**********    ] │    │
│  └────────────────────────────┘    │
│                                     │
│  Language                           │
│  ( ) English  (•) Русский          │
│                                     │
│  ┌──────────────────────┐          │
│  │  Save and Reboot     │          │
│  └──────────────────────┘          │
│                                     │
└─────────────────────────────────────┘
```

4. **Enter WiFi Credentials:**
   - Type your WiFi network name in SSID field
   - Type your WiFi password
   - Select your preferred language
   - Click "Save and Reboot"

5. **Device Reboots:**
   - Wait 10-15 seconds
   - Device connects to your WiFi
   - Display shows IP address
   - LED turns blue (WiFi connected)

### Method 2: Serial Console (Advanced)

**Perfect for:** Developers, automation, headless setup

**Steps:**

1. **Open Serial Console:**
   - Arduino IDE: **Tools → Serial Monitor**
   - PuTTY (Windows): Port COM3, Speed 115200
   - screen (Linux/Mac): `screen /dev/ttyUSB0 115200`

2. **Wait for Prompt:**
   ```
   Setup complete!
   > _
   ```

3. **Configure WiFi:**
   ```
   > WIFI SET MyHomeWiFi MyPassword
   WiFi settings updated:
   WiFi SSID: MyHomeWiFi
   Password: **********
   
   Connecting to WiFi...
   .....
   WiFi connected successfully!
   IP Address: 192.168.1.100
   
   Use SAVE command to store settings to NVS
   > 
   ```

4. **Save Settings:**
   ```
   > SAVE
   Saving settings to NVS...
     - WiFi config saved
     - Display settings saved
     - Melody settings saved
   All settings saved successfully!
   > 
   ```

5. **Optional - Set Language:**
   ```
   > LANGUAGE RU
   Language changed to: Русский
   Settings saved. Restart to see changes in startup messages.
   > 
   ```

### Method 3: Manual Configuration in Code (Not Recommended)

**Only use if you cannot use other methods**

1. Edit `ESP32-S3-Z-Clock.ino`
2. Find lines 54-55:
   ```cpp
   String defSSID = "Enter_SSID";
   String defPASS = "Enter_Pass";
   ```
3. Change to your WiFi:
   ```cpp
   String defSSID = "MyHomeWiFi";
   String defPASS = "MyPassword";
   ```
4. Re-upload firmware

**Note:** This hardcodes credentials in firmware - not secure for sharing code!

---

## Web Interface Guide

### Accessing Web Interface

Once connected to WiFi, the device displays its IP address on the OLED screen:

```
┌─────────────────┐
│ T:25.2°C  P:1020│  Status bar
├─────────────────┤
│                 │
│   12:34:56      │  Time
│                 │
│  25.01.2026     │  Date
│   Saturday      │  Day of week
│                 │
│ IP:192.168.1.100│  ← Your IP address
└─────────────────┘
```

**To access web interface:**
1. Open web browser on any device on same WiFi network
2. Type IP address: `http://192.168.1.100` (use your device's IP)
3. Press Enter

### Main Interface Overview

```
┌────────────────────────────────────────────────────────────┐
│  ESP32-S3 Smart Clock                    [EN] [Settings ⚙] │
├────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────┐  ┌──────────────────────────────┐   │
│  │                 │  │  System Status               │   │
│  │   12:34:56      │  │  ───────────────────────────  │   │
│  │                 │  │  📶 WiFi: Connected (-42 dBm) │   │
│  │  25.01.2026     │  │  🌡 Temperature: 25.2°C       │   │
│  │   Saturday      │  │  💧 Humidity: 45.3%           │   │
│  └─────────────────┘  │  🌫 Pressure: 1020 hPa        │   │
│                        │  ☀ Weather: Clear             │   │
│                        └──────────────────────────────┘   │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  Quick Actions                                       │  │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌────────┐ │  │
│  │  │ Sync Time│ │Set Alarm │ │Set Timer │ │ Night  │ │  │
│  │  │    ⏱     │ │    ⏰    │ │    ⏲     │ │  Mode  │ │  │
│  │  └──────────┘ └──────────┘ └──────────┘ └────────┘ │  │
│  └─────────────────────────────────────────────────────┘  │
│                                                             │
│  [⏰ Alarms]  [⏲ Timer]  [⚙ Settings]  [📊 Status]       │
│                                                             │
└────────────────────────────────────────────────────────────┘
```

### Settings Tab - Time Configuration

Click **Settings** → **Time** to configure time and date settings:

```
┌─────────────────────────────────────────────────┐
│  ⏱ Time Settings                                │
├─────────────────────────────────────────────────┤
│                                                  │
│  NTP Server                                      │
│  ┌──────────────────────────────────────┐      │
│  │ pool.ntp.org                         │      │
│  └──────────────────────────────────────┘      │
│  Common servers: time.google.com,              │
│                  time.cloudflare.com            │
│                                                  │
│  Timezone (GMT Offset)                          │
│  ┌────┐ hours                                   │
│  │ +3 │ (GMT+3 = Moscow, GMT-5 = New York)     │
│  └────┘                                         │
│                                                  │
│  Daylight Saving Time (DST)                     │
│  ┌────┐ hours                                   │
│  │  0 │ (0 = no DST, +1 = summer time)         │
│  └────┘                                         │
│                                                  │
│  Manual Time Set                                │
│  ┌──────┐  ┌──────┐  ┌──────┐                 │
│  │ 2026 │  │  01  │  │  25  │                 │
│  └──────┘  └──────┘  └──────┘                 │
│   Year      Month      Day                      │
│                                                  │
│  ┌────┐ : ┌────┐ : ┌────┐                     │
│  │ 12 │   │ 34 │   │ 56 │                     │
│  └────┘   └────┘   └────┘                     │
│  Hour      Min      Sec                         │
│                                                  │
│  ┌──────────────┐  ┌──────────────┐           │
│  │ Sync Now ⏱  │  │ Save ✓       │           │
│  └──────────────┘  └──────────────┘           │
│                                                  │
│  ⓘ Time syncs automatically every 6 hours      │
│                                                  │
└─────────────────────────────────────────────────┘
```

**Field descriptions:**

- **NTP Server:** Server for automatic time synchronization
  - Default: `pool.ntp.org`
  - Alternatives: `time.google.com`, `time.cloudflare.com`, `time.nist.gov`
  
- **Timezone:** Your offset from GMT
  - Examples: GMT+3 (Moscow), GMT-5 (New York), GMT+1 (Berlin)
  - Range: -12 to +14 hours
  
- **DST:** Additional offset for daylight saving time
  - 0 = No DST
  - +1 = Summer time (most common)
  - Applied automatically, no date switching
  
- **Manual Time Set:** Override NTP time (temporary until next NTP sync)
  - Use only if NTP unavailable
  - Format: YYYY-MM-DD HH:MM:SS

**Example scenarios:**

1. **Moscow (no DST):**
   - Timezone: +3
   - DST: 0

2. **New York (with DST):**
   - Timezone: -5 (EST)
   - DST: +1 (EDT in summer)

3. **Tokyo (no DST):**
   - Timezone: +9
   - DST: 0

### Settings Tab - Display Configuration

Click **Settings** → **Display**:

```
┌─────────────────────────────────────────────────┐
│  🖥 Display Settings                             │
├─────────────────────────────────────────────────┤
│                                                  │
│  Brightness                                      │
│  ┌────────────────────────────────────────┐    │
│  │░░░░░░░░░░░░░░░░░░○───────────────────│ │    │
│  └────────────────────────────────────────┘    │
│  Current: 200 / 255                             │
│  (0 = off, 255 = maximum)                       │
│                                                  │
│  Night Mode                                      │
│  ☐ Enable night mode                           │
│                                                  │
│  ┌ When enabled: ────────────────────────┐     │
│  │                                        │     │
│  │  Active time:                          │     │
│  │  From: ┌────┐ : ┌────┐                │     │
│  │        │ 23 │   │ 00 │                │     │
│  │        └────┘   └────┘                │     │
│  │                                        │     │
│  │  To:   ┌────┐ : ┌────┐                │     │
│  │        │ 07 │   │ 00 │                │     │
│  │        └────┘   └────┘                │     │
│  │                                        │     │
│  │  Night brightness:                     │     │
│  │  ┌──────────────────────────────┐     │     │
│  │  │○──────────────────────────── │     │     │
│  │  └──────────────────────────────┘     │     │
│  │  Current: 10 / 255                     │     │
│  │                                        │     │
│  └────────────────────────────────────────┘     │
│                                                  │
│  ⓘ Night mode automatically reduces             │
│     brightness during specified hours            │
│                                                  │
│  ┌──────────────┐                               │
│  │ Save ✓       │                               │
│  └──────────────┘                               │
│                                                  │
└─────────────────────────────────────────────────┘
```

**Tips:**
- **Daytime brightness:** 150-255 for normal use
- **Night brightness:** 5-20 for bedroom use
- **Night mode example:** 23:00-07:00 (11 PM to 7 AM)
- Brightness can be adjusted temporarily with BOOT button

### Settings Tab - WiFi Management

Click **Settings** → **WiFi**:

```
┌─────────────────────────────────────────────────┐
│  📶 WiFi Settings                                │
├─────────────────────────────────────────────────┤
│                                                  │
│  Current Status                                  │
│  ┌────────────────────────────────────────┐    │
│  │ ✓ Connected                             │    │
│  │ SSID: MyHomeWiFi                        │    │
│  │ Signal: -42 dBm (Excellent)             │    │
│  │ IP: 192.168.1.100                       │    │
│  └────────────────────────────────────────┘    │
│                                                  │
│  WiFi Mode                                       │
│  ( ) Always ON - WiFi always enabled            │
│  (•) Smart WiFi - Auto on/off (power saving)   │
│                                                  │
│  ┌ Smart WiFi Settings (when enabled): ──┐     │
│  │                                        │     │
│  │  Auto-off timeout:                     │     │
│  │  ┌────┐ minutes                        │     │
│  │  │ 10 │ (WiFi off after inactivity)    │     │
│  │  └────┘                                │     │
│  │                                        │     │
│  │  NTP sync interval:                    │     │
│  │  ┌────┐ minutes                        │     │
│  │  │360 │ (sync time every 6 hours)      │     │
│  │  └────┘                                │     │
│  │                                        │     │
│  │  Weather update:                       │     │
│  │  ┌────┐ minutes                        │     │
│  │  │ 60 │ (update every hour)            │     │
│  │  └────┘                                │     │
│  │                                        │     │
│  │  Manual WiFi enable:                   │     │
│  │  ┌────────────────┐                    │     │
│  │  │ Enable for 10m │                    │     │
│  │  └────────────────┘                    │     │
│  │                                        │     │
│  └────────────────────────────────────────┘     │
│                                                  │
│  Change WiFi Network                             │
│  ┌──────────────────────────────────────┐      │
│  │ SSID: [                            ] │      │
│  └──────────────────────────────────────┘      │
│  ┌──────────────────────────────────────┐      │
│  │ Password: [                        ] │      │
│  └──────────────────────────────────────┘      │
│                                                  │
│  ┌──────────────┐                               │
│  │ Save ✓       │                               │
│  └──────────────┘                               │
│                                                  │
│  ⓘ Smart WiFi saves ~50mA when off              │
│     (good for battery operation)                 │
│                                                  │
└─────────────────────────────────────────────────┘
```

**WiFi Mode comparison:**

| Feature | Always ON | Smart WiFi |
|---------|-----------|------------|
| Power consumption | ~130mA | ~80mA avg |
| Web access | Instant | 2-5 sec delay |
| NTP sync | Every 6h | Every 6h |
| Weather updates | Configurable | Configurable |
| Best for | Always powered | Battery operation |

### Alarms Tab

Click **Alarms** to manage up to 10 independent alarms:

```
┌──────────────────────────────────────────────────────────┐
│  ⏰ Alarms Management                   [+ Add New Alarm] │
├──────────────────────────────────────────────────────────┤
│                                                           │
│  Alarm 0                                    [ON] [Edit]  │
│  ┌─────────────────────────────────────────────────────┐│
│  │ ⏰ 07:00                                             ││
│  │ 📅 Weekdays: Mon Tue Wed Thu Fri                    ││
│  │ 🔁 Repeat: Yes                                       ││
│  │ 📝 Text: "Morning alarm"                             ││
│  │ 🎵 Melody: Default                                   ││
│  └─────────────────────────────────────────────────────┘│
│                                                           │
│  Alarm 1                                    [OFF] [Edit] │
│  ┌─────────────────────────────────────────────────────┐│
│  │ ⏰ 09:00                                             ││
│  │ 📅 Weekend: Sat Sun                                  ││
│  │ 🔁 Repeat: Yes                                       ││
│  │ 📝 Text: "Weekend"                                   ││
│  └─────────────────────────────────────────────────────┘│
│                                                           │
│  Alarm 2                                    [OFF] [Edit] │
│  ┌─────────────────────────────────────────────────────┐│
│  │ ⏰ 14:30                                             ││
│  │ 📅 Date: 2026-01-26 (One-time)                      ││
│  │ 📝 Text: "Doctor appointment"                        ││
│  └─────────────────────────────────────────────────────┘│
│                                                           │
│  [Show more alarms...]                                   │
│                                                           │
└──────────────────────────────────────────────────────────┘
```

**Add/Edit Alarm Interface:**

Click **+ Add New Alarm** or **Edit**:

```
┌─────────────────────────────────────────────────┐
│  ⏰ Add/Edit Alarm                               │
├─────────────────────────────────────────────────┤
│                                                  │
│  Alarm Index: [0] (0-9)                         │
│                                                  │
│  Time                                            │
│  ┌────┐ : ┌────┐                                │
│  │ 07 │   │ 00 │                                │
│  └────┘   └────┘                                │
│  Hour      Minute                                │
│                                                  │
│  Alarm Type                                      │
│  (•) Recurring alarm                            │
│  ( ) One-time alarm (specific date)             │
│                                                  │
│  ┌ Recurring alarm: ───────────────────┐        │
│  │                                      │        │
│  │  Repeat on:                          │        │
│  │  [✓] Mon  [✓] Tue  [✓] Wed           │        │
│  │  [✓] Thu  [✓] Fri  [ ] Sat           │        │
│  │  [ ] Sun                             │        │
│  │                                      │        │
│  │  ( ) Daily (every day)               │        │
│  │                                      │        │
│  └──────────────────────────────────────┘        │
│                                                  │
│  ┌ One-time alarm: ────────────────────┐        │
│  │                                      │        │
│  │  Date: ┌──────┐ ┌────┐ ┌────┐      │        │
│  │        │ 2026 │ │ 01 │ │ 26 │      │        │
│  │        └──────┘ └────┘ └────┘      │        │
│  │        Year     Month   Day         │        │
│  │                                      │        │
│  └──────────────────────────────────────┘        │
│                                                  │
│  Options                                         │
│  [✓] Repeat alarm (ring until stopped)          │
│  [✓] Save to memory (persistent)                │
│                                                  │
│  Custom Text (optional)                          │
│  ┌──────────────────────────────────────┐      │
│  │ Morning wake-up                      │      │
│  └──────────────────────────────────────┘      │
│  (Max 50 characters)                            │
│                                                  │
│  Melody (Passive buzzer only)                   │
│  ( ) Default alarm melody                       │
│  (•) Custom melody:                             │
│  ┌──────────────────────────────────────┐      │
│  │ E5 E P S E5 E P S E5 Q               │      │
│  └──────────────────────────────────────┘      │
│  [Test Melody]                                  │
│                                                  │
│  ┌──────────────┐  ┌──────────────┐           │
│  │ Save ✓       │  │ Cancel       │           │
│  └──────────────┘  └──────────────┘           │
│                                                  │
└─────────────────────────────────────────────────┘
```

**Alarm configuration examples:**

1. **Weekday morning alarm:**
   - Time: 07:00
   - Type: Recurring
   - Days: Mon-Fri
   - Repeat: Yes
   - Save: Yes
   - Text: "Work day"

2. **Weekend alarm:**
   - Time: 09:00
   - Type: Recurring  
   - Days: Sat-Sun
   - Repeat: Yes
   - Text: "Weekend"

3. **One-time reminder:**
   - Time: 14:30
   - Type: One-time
   - Date: 2026-01-26
   - Text: "Doctor appointment"

### Timer Tab

Click **Timer**:

```
┌─────────────────────────────────────────────────┐
│  ⏲ Timer                                         │
├─────────────────────────────────────────────────┤
│                                                  │
│  ┌────────────────────────────────────────┐    │
│  │                                         │    │
│  │        Timer: Not Active                │    │
│  │                                         │    │
│  │         00:00:00                        │    │
│  │                                         │    │
│  └────────────────────────────────────────┘    │
│                                                  │
│  Set Timer Duration                              │
│                                                  │
│  ┌────┐  ┌────┐  ┌────┐                        │
│  │ 00 │ :│ 15 │ :│ 00 │                        │
│  └────┘  └────┘  └────┘                        │
│  Hours    Minutes  Seconds                      │
│                                                  │
│  Quick preset buttons:                           │
│  [1m] [5m] [10m] [15m] [30m] [1h]              │
│                                                  │
│  Timer Label (optional)                          │
│  ┌──────────────────────────────────────┐      │
│  │ Pizza                                 │      │
│  └──────────────────────────────────────┘      │
│  (Max 50 characters)                            │
│                                                  │
│  Melody (Passive buzzer only)                   │
│  ( ) Default timer melody                       │
│  (•) Custom melody:                             │
│  ┌──────────────────────────────────────┐      │
│  │ G6 E A6 E G6 E F6 E P W               │      │
│  └──────────────────────────────────────┘      │
│  [Test Melody]                                  │
│                                                  │
│  ┌──────────────┐  ┌──────────────┐           │
│  │ Start ▶      │  │ Clear        │           │
│  └──────────────┘  └──────────────┘           │
│                                                  │
└─────────────────────────────────────────────────┘
```

**When timer is running:**

```
┌─────────────────────────────────────────────────┐
│  ⏲ Timer                                         │
├─────────────────────────────────────────────────┤
│                                                  │
│  ┌────────────────────────────────────────┐    │
│  │                                         │    │
│  │        Timer: Active                    │    │
│  │        "Pizza"                          │    │
│  │                                         │    │
│  │         00:12:34                        │    │
│  │                                         │    │
│  │   ████████████████░░░░░░░░░░░  67%     │    │
│  │                                         │    │
│  └────────────────────────────────────────┘    │
│                                                  │
│  Started: 12:34:56                               │
│  Will expire: 12:47:30                           │
│                                                  │
│  ┌──────────────┐                               │
│  │ Stop ⏹       │                               │
│  └──────────────┘                               │
│                                                  │
└─────────────────────────────────────────────────┘
```

---

## Serial Console Guide

### Opening Serial Console

**Arduino IDE:**
1. **Tools → Serial Monitor** (Ctrl+Shift+M)
2. Set baud rate: **115200**
3. Set line ending: **Newline**

**PuTTY (Windows):**
1. Connection type: **Serial**
2. Serial line: **COM3** (check Device Manager)
3. Speed: **115200**
4. Click **Open**

**screen (Linux/Mac):**
```bash
screen /dev/ttyUSB0 115200
```

**minicom (Linux):**
```bash
minicom -D /dev/ttyUSB0 -b 115200
```

### Console Interface

When connected, you'll see:
```
Setup complete!
Web server: RUNNING
Web interface: http://192.168.1.100
WiFi Signal: -42 dBm

> _
```

The `>` prompt indicates device is ready for commands.

### Basic Commands

#### Show System Status

```
> status

=== SYSTEM STATUS ===
WiFi SSID: MyHomeWiFi
WiFi Status: CONNECTED
IP Address: 192.168.1.100
Signal Strength: -42 dBm
NTP Server: pool.ntp.org
Time Sync: YES
Timezone: GMT+3
DST Offset: +0 hours
Weather City: Moscow
Display Brightness: 200
Night Mode: ENABLED
  Active: YES
  Time: 23:00 - 07:00
  Brightness: 10
WiFi Mode: SMART
  Auto-off timeout: 10 min
  NTP Sync interval: 360 min
  Weather Sync interval: 60 min
  Time until auto-off: 582 sec (web activity)
Buzzer Type: PASSIVE
Alarm Melody: E5 E P S E5 E P S E5 Q P Q...
Timer Melody: G6 E A6 E G6 E F6 E P W
Current Time: 12:34:56
Current Date: 25.01.2026
Alarms: 2 active (use ALARM LIST for details)
Timer: OFF
AHT20: OK
BMP280: OK
Temperature: 25.2 C
Humidity: 45.3 %
Pressure: 1020.0 hPa
Free RAM: 226 KB

> _
```

#### Show Help

```
> help

=== CLOCK COMMANDS ===
  TIME YYYY-MM-DD HH:MM:SS - Set time manually
  NTP <server> - Set NTP server
  TZ <+/-hours> - Set timezone offset
  DST <+/-hours> - Set daylight saving offset
  CITY <name> - Set city for weather
  BRIGHTNESS <0-255> - Set display brightness
  SAVE - Save settings to NVS
  RESTORE - Restore settings from NVS
  ERASE - Erase all NVS data
  STATUS - Show system status
  SYNC - Force NTP time sync
  WEATHER - Update weather data
  REBOOT - Restart device
  LANGUAGE <EN|RU> - Switch interface language

=== ALARM COMMANDS ===
  ALARM LIST - List all alarms
  ALARM SET <index> [YYYY-MM-DD|1234567] HH:MM [TEXT] [R] [S]
  ALARM CLEAR <index> - Clear alarm
  ALARM CLEARALL - Clear all alarms
  ALARM TOGGLE <index> - Toggle alarm on/off

=== TIMER COMMANDS ===
  TIMER HH:MM:SS [TEXT] - Start timer
  TIMER MM:SS [TEXT] - Start timer
  TIMER SS [TEXT] - Start timer
  TIMER CLEAR - Clear timer

=== MELODY COMMANDS (Passive Buzzer) ===
  MELODY ALARM <notes> - Set alarm melody
  MELODY TIMER <notes> - Set timer melody
  MELODY TEST <notes> - Test melody
  MELODY SAVE - Save melodies to NVS

=== WiFi MODE COMMANDS ===
  WIFI SET <ssid> <password> - Set WiFi credentials
  WIFI ALWAYS - Set WiFi mode to ALWAYS ON
  WIFI SMART - Set WiFi mode to SMART WiFi
  WIFI ON - Manually enable WiFi for 10 min (Smart mode)
  WIFI OFF - Turn WiFi off
  WIFI STATUS - Show WiFi mode and status

=== NIGHT MODE COMMANDS ===
  NIGHT ON <HH:MM-HH:MM> <brightness> - Enable night mode
  NIGHT OFF - Disable night mode
  NIGHT STATUS - Show night mode settings

> _
```

### Time Management Commands

#### Set Time Manually

```
> time 2026-01-25 12:34:56
Time set to: 2026-01-25 12:34:56
> _
```

#### Sync with NTP Server

```
> sync
Synchronizing time with NTP server...
Time synchronized successfully!
Current time: 12:34:56
Current date: 25.01.2026
> _
```

#### Change NTP Server

```
> ntp time.google.com
NTP server set to: time.google.com
> _
```

#### Set Timezone

```
> tz +3
Timezone set to: GMT+3
> _
```

```
> tz -5
Timezone set to: GMT-5
> _
```

#### Set Daylight Saving Time

```
> dst +1
DST offset set to: +1 hour
> _
```

```
> dst 0
DST offset set to: 0 hours
> _
```

### WiFi Commands

#### Check WiFi Status

```
> wifi status
[WiFi] Mode: SMART
[WiFi] Status: CONNECTED
> _
```

#### Change WiFi Network

```
> wifi set MyNewWiFi NewPassword
WiFi settings updated:
WiFi SSID: MyNewWiFi
Password: **********

Connecting to WiFi...
.....
WiFi connected successfully!
IP Address: 192.168.1.101

Use SAVE command to store settings to NVS
> _
```

#### Switch WiFi Modes

**Always ON mode:**
```
> wifi always
WiFi mode set to: ALWAYS ON
WiFi will stay enabled at all times
Use SAVE to make permanent
> _
```

**Smart WiFi mode:**
```
> wifi smart
WiFi mode set to: SMART WiFi
WiFi will auto on/off for power saving
Use SAVE to make permanent
> _
```

#### Manual WiFi Control (Smart Mode Only)

**Turn WiFi on for 10 minutes:**
```
> wifi on

===== LONG PRESS DETECTED =====
WiFi manually enabled for 10 minutes
Current WiFi status: CONNECTED
IP Address: 192.168.1.100
WiFi will turn ON in next loop cycle
==============================

> _
```

**Turn WiFi off:**
```
> wifi off
Disabling WiFi...
WiFi disabled
> _
```

### Display Commands

#### Set Brightness

```
> brightness 200
Display brightness set to: 200
> _
```

**Brightness levels:**
- 0 = Display off
- 1-50 = Very dim (night use)
- 51-150 = Dim (power saving)
- 151-255 = Bright (normal use)

### Night Mode Commands

#### Enable Night Mode

```
> night on 23:00-07:00 10
[Night Mode] ENABLED
[Night Mode] Time: 23:00 - 07:00
[Night Mode] Brightness: 10
> _
```

#### Disable Night Mode

```
> night off
[Night Mode] DISABLED
[Night Mode] Screen brightness restored
> _
```

#### Check Night Mode Status

```
> night status
[Night Mode] ENABLED
[Night Mode] Time: 23:00 - 07:00
[Night Mode] Brightness: 10
[Night Mode] Currently Active: NO
> _
```

---

## Alarm System

### Listing Alarms

```
> alarm list

=== ALARMS ===
[0] ON 07:00 R "Morning"
[1] ON 09:00 R Days:96 "Weekend"
[2] OFF
[3] ON 14:30 "Doctor" 2026-01-26
[4] OFF
[5] OFF
[6] OFF
[7] OFF
[8] OFF
[9] OFF

> _
```

**Legend:**
- `ON` / `OFF` = Alarm enabled/disabled
- `07:00` = Alarm time
- `R` = Repeat flag (rings until stopped)
- `Days:96` = Weekdays bitmask (binary: 1100000 = Sat+Sun)
- `"Text"` = Custom alarm text
- `2026-01-26` = One-time alarm date

### Setting Alarms

#### Daily Alarm (Every Day)

```
> alarm set 0 07:30
[Alarm] Set alarm 0 for Daily 07:30
> _
```

**With custom text:**
```
> alarm set 0 07:30 Morning wake-up
[Alarm] Set alarm 0 for Daily 07:30 'Morning wake-up'
> _
```

**With repeat flag:**
```
> alarm set 0 07:30 Morning R
[Alarm] Set alarm 0 for Daily 07:30 [R] 'Morning'
> _
```

**With save flag (persistent across reboots):**
```
> alarm set 0 07:30 Morning R S
[Alarm] Set alarm 0 for Daily 07:30 [R] [S] 'Morning'
> _
```

#### Weekday Alarms (Specific Days)

**Format:** `ALARM SET <index> <days> HH:MM [text] [flags]`

**Days notation:**
- 1 = Monday
- 2 = Tuesday
- 3 = Wednesday
- 4 = Thursday
- 5 = Friday
- 6 = Saturday
- 7 = Sunday

**Monday to Friday at 07:00:**
```
> alarm set 1 12345 07:00 Weekdays R S
[Alarm] Set alarm 1 for Weekdays: Mon Tue Wed Thu Fri 07:00 [R] [S] 'Weekdays'
> _
```

**Saturday and Sunday at 09:00:**
```
> alarm set 2 67 09:00 Weekend R S
[Alarm] Set alarm 2 for Weekdays: Sat Sun 09:00 [R] [S] 'Weekend'
> _
```

**Every day at 08:00 (alternative notation):**
```
> alarm set 3 1234567 08:00 Daily R S
[Alarm] Set alarm 3 for Weekdays: Mon Tue Wed Thu Fri Sat Sun 08:00 [R] [S] 'Daily'
> _
```

#### One-Time Alarms (Specific Date)

**Format:** `ALARM SET <index> YYYY-MM-DD HH:MM [text] [flags]`

**Single reminder:**
```
> alarm set 5 2026-01-26 14:30 Doctor appointment
[Alarm] Set alarm 5 for 2026-01-26 14:30 'Doctor appointment'
> _
```

**One-time with repeat (rings until stopped):**
```
> alarm set 6 2026-02-14 20:00 Valentine R
[Alarm] Set alarm 6 for 2026-02-14 20:00 [R] 'Valentine'
> _
```

### Managing Alarms

#### Turn Alarm On/Off

```
> alarm toggle 0
[Alarm] Alarm 0 is now ON
> _
```

```
> alarm toggle 0
[Alarm] Alarm 0 is now OFF
> _
```

#### Clear Single Alarm

```
> alarm clear 5
[Alarm] Cleared alarm 5
> _
```

#### Clear All Alarms

```
> alarm clearall
[Alarm] All alarms cleared
> _
```

### Custom Alarm Melodies

**Set custom melody for specific alarm:**

```
> alarm melody 0 C5 Q D5 Q E5 H F5 H G5 W
[Alarm] Melody set for alarm 0
> _
```

**Melody format:**
- `[Note][Octave] [Duration]`
- Notes: C, D, E, F, G, A, B (use # for sharp: C#, D#, F#, G#, A#)
- Octave: 3, 4, 5, 6, 7
- Duration: W (whole), H (half), Q (quarter), E (eighth), S (sixteenth)
- P = Pause

**Examples:**

**Simple scale:**
```
> melody test C5 Q D5 Q E5 Q F5 Q G5 Q A5 Q B5 Q C6 H
(Plays C major scale)
> _
```

**Happy Birthday melody:**
```
> alarm melody 1 G4 S G4 S A4 Q G4 Q C5 Q B4 H P Q G4 S G4 S A4 Q G4 Q D5 Q C5 H
[Alarm] Melody set for alarm 1
> _
```

---

## Timer System

### Starting Timers

#### Timer with Hours, Minutes, Seconds

```
> timer 01:30:00
[Timer] Set for 1:30:00
> _
```

#### Timer with Minutes and Seconds

```
> timer 45:30
[Timer] Set for 45:30
> _
```

#### Timer with Seconds Only

```
> timer 120
[Timer] Set for 2:00
> _
```

#### Timer with Custom Text

```
> timer 00:15:00 Pizza in oven
[Timer] Set for 15:00 'Pizza in oven'
> _
```

### Stopping Timer

```
> timer clear
Timer cleared
> _
```

### Timer Display

When timer is active, display shows:

```
┌─────────────────┐
│ Timer: 00:14:23 │
│  Pizza in oven  │
└─────────────────┘
```

When timer expires:
- Buzzer plays melody once (for passive buzzer)
- Display shows: "TIMER EXPIRED"
- LED turns green
- Automatic stop after 3 minutes
- Manual stop: Press BOOT button or send `timer clear`

---

## WiFi Management

### WiFi Mode Comparison

| Feature | Always ON | Smart WiFi |
|---------|-----------|------------|
| **Power** | ~130mA | ~80mA average |
| **Web access** | Instant | 2-5 second delay on first request |
| **NTP sync** | Every 6 hours | Every 6 hours (auto wake) |
| **Weather** | Every 1 hour | Every 1 hour (auto wake) |
| **Manual control** | Not needed | Can enable for 10 min via button |
| **Best for** | Wall powered | Battery operated |

### Smart WiFi Behavior

**WiFi turns ON when:**
1. NTP sync needed (configurable, default 6 hours)
2. Weather update needed (configurable, default 1 hour)
3. Web request received (stays on 10 minutes)
4. Manual enable (button hold 3+ sec or `wifi on` command)

**WiFi turns OFF when:**
1. 10 minutes of inactivity (configurable)
2. All scheduled tasks complete

**Activity log in console:**

```
[Smart WiFi] ===== TURNING WiFi ON =====
[Smart WiFi] Reason: NTP sync needed
[Smart WiFi] Connecting...
[Smart WiFi] ============================

[Smart WiFi] ===== CONNECTION SUCCESS =====
[Smart WiFi] IP Address: 192.168.1.100
[Smart WiFi] Signal: -42 dBm
[Smart WiFi] ===========================

[Smart WiFi] NTP sync

[Smart WiFi] ===== TURNING WiFi OFF =====
[Smart WiFi] No activity, powering down
[Smart WiFi] WiFi powered OFF
[Smart WiFi] =============================
```

### Configuring Smart WiFi Intervals

**Via web interface:**
- Settings → WiFi → Smart WiFi Settings
- Adjust sliders for timeouts and intervals

**Via Serial (requires code modification):**
Currently not available via Serial commands. Future enhancement.

---

## Night Mode

### Night Mode Operation

Night mode automatically:
1. Reduces display brightness at specified start time
2. Restores normal brightness at specified end time
3. Can be temporarily overridden by pressing BOOT button

### Configuration Examples

#### Bedroom Use (11 PM - 7 AM)

```
> night on 23:00-07:00 5
[Night Mode] ENABLED
[Night Mode] Time: 23:00 - 07:00
[Night Mode] Brightness: 5
> _
```

#### Office Use (6 PM - 8 AM)

```
> night on 18:00-08:00 20
[Night Mode] ENABLED
[Night Mode] Time: 18:00 - 08:00
[Night Mode] Brightness: 20
> _
```

#### Disable Night Mode

```
> night off
[Night Mode] DISABLED
[Night Mode] Screen brightness restored
> _
```

### Time Range Spanning Midnight

Night mode correctly handles time ranges that cross midnight:

**Example: 11 PM to 7 AM**
- Active from 23:00 on Day 1 to 07:00 on Day 2
- Not active from 07:00 to 23:00

**Logic:**
```
Current time is in range if:
  (current >= start) OR (current < end)
```

---

## Melody Customization

### Understanding Melody Format

**Format:** `[Note][Octave] [Duration] [Pause] [Duration]`

**Components:**

1. **Note:**
   - Natural: C, D, E, F, G, A, B
   - Sharp: C#, D#, F#, G#, A#
   - No flat notation (use sharp of lower note)

2. **Octave:**
   - 3 = Low
   - 4 = Middle-low
   - 5 = Middle (most common)
   - 6 = High
   - 7 = Very high

3. **Duration:**
   - W = Whole note (1000ms)
   - H = Half note (500ms)
   - Q = Quarter note (250ms)
   - E = Eighth note (125ms)
   - S = Sixteenth note (63ms)

4. **Pause:**
   - P [Duration] = Silence

### Melody Examples

#### Simple Melodies

**C Major Scale:**
```
> melody test C5 Q D5 Q E5 Q F5 Q G5 Q A5 Q B5 Q C6 H
(Plays: do-re-mi-fa-sol-la-si-do)
> _
```

**Twinkle Twinkle Little Star:**
```
> melody test C5 Q C5 Q G5 Q G5 Q A5 Q A5 Q G5 H F5 Q F5 Q E5 Q E5 Q D5 Q D5 Q C5 H
> _
```

#### Default Melodies

**Default Alarm (Morning Beeps):**
```
E5 E P S E5 E P S E5 Q P Q
E5 E P S E5 E P S E5 Q P Q
E5 E P S G5 E P S C5 E P S D5 E P S E5 H P H
```

**Default Timer (Completion Signal):**
```
G6 E A6 E G6 E F6 E P W
```

### Setting Melodies

#### Set Default Alarm Melody

```
> melody alarm C5 Q E5 Q G5 Q C6 H
Default alarm melody updated
> _
```

#### Set Default Timer Melody

```
> melody timer G6 E A6 E G6 E F6 E
Default timer melody updated
> _
```

#### Test Melody Before Saving

```
> melody test C5 Q D5 Q E5 Q F5 Q G5 Q
(Plays melody once)
> _
```

#### Save Melodies to Memory

```
> melody save
  - Melody settings saved
Melodies saved to NVS
> _
```

### Melody Best Practices

1. **Keep melodies short:** 20-40 notes maximum
2. **Use pauses:** P Q between note groups for rhythm
3. **Test first:** Use `melody test` before setting
4. **Save favorites:** Use `melody save` to persist across reboots
5. **Octave 5-6:** Most pleasant for alarm/timer sounds

---

## Smart Home Integration

### Integration Methods

1. **HTTP Requests** - Web interface API calls
2. **Serial Commands** - Direct Serial communication
3. **GPIO Monitoring** - Watch LED/buzzer pins
4. **MQTT** - Requires custom code modification

### Home Assistant Integration

#### Method 1: RESTful Sensors

**Read temperature from web page:**

```yaml
# configuration.yaml
sensor:
  - platform: rest
    name: "ESP32 Clock Temperature"
    resource: "http://192.168.1.100/"
    value_template: >
      {% set temp = value | regex_findall_index('Temperature: ([0-9.]+)') %}
      {{ temp if temp else 'unknown' }}
    unit_of_measurement: "°C"
    scan_interval: 60
```

#### Method 2: Serial Monitor Automation

**Monitor Serial output for alarm events:**

```yaml
# configuration.yaml
sensor:
  - platform: serial
    name: "ESP32 Clock Serial"
    serial_port: /dev/ttyUSB0
    baudrate: 115200

automation:
  - alias: "ESP32 Alarm Triggered"
    trigger:
      - platform: template
        value_template: >
          {{ '[Alarm]' in states('sensor.esp32_clock_serial') }}
    action:
      - service: notify.mobile_app_iphone
        data:
          title: "Alarm"
          message: "ESP32 clock alarm triggered!"
      - service: light.turn_on
        entity_id: light.bedroom
        data:
          brightness: 255
```

#### Method 3: Shell Commands

**Send Serial commands via shell_command:**

```yaml
# configuration.yaml
shell_command:
  esp32_alarm_on: 'echo "ALARM SET 0 {{ time }} {{ text }}" > /dev/ttyUSB0'
  esp32_alarm_off: 'echo "ALARM CLEAR 0" > /dev/ttyUSB0'
  esp32_timer_start: 'echo "TIMER {{ duration }} {{ text }}" > /dev/ttyUSB0'
  esp32_brightness: 'echo "BRIGHTNESS {{ level }}" > /dev/ttyUSB0'

script:
  esp32_morning_alarm:
    sequence:
      - service: shell_command.esp32_alarm_on
        data:
          time: "07:30"
          text: "Morning"
```

**Use in automation:**

```yaml
automation:
  - alias: "Set ESP32 Alarm for Work Days"
    trigger:
      - platform: time
        at: "22:00:00"
    condition:
      - condition: time
        weekday:
          - mon
          - tue
          - wed
          - thu
          - fri
    action:
      - service: script.esp32_morning_alarm
```

### Node-RED Integration

#### Serial Command Flow

```json
[
  {
    "type": "inject",
    "name": "Morning Alarm",
    "topic": "",
    "payload": "ALARM SET 0 07:30 Morning",
    "payloadType": "str",
    "repeat": "",
    "crontab": "",
    "once": false,
    "onceDelay": 0.1
  },
  {
    "type": "function",
    "name": "Add Newline",
    "func": "msg.payload = msg.payload + '\\n';\nreturn msg;"
  },
  {
    "type": "serial out",
    "name": "ESP32 Serial",
    "serial": "/dev/ttyUSB0",
    "x": 400,
    "y": 200
  }
]
```

#### HTTP Request Flow (for future API)

```json
[
  {
    "type": "inject",
    "name": "Get Status",
    "topic": "",
    "payload": "",
    "payloadType": "date",
    "repeat": "60",
    "crontab": ""
  },
  {
    "type": "http request",
    "name": "ESP32 API",
    "method": "GET",
    "url": "http://192.168.1.100/api/status",
    "x": 300,
    "y": 100
  },
  {
    "type": "json",
    "name": "Parse JSON"
  },
  {
    "type": "debug",
    "name": "Show Data"
  }
]
```

### Python Script Examples

#### Send Serial Command

```python
import serial
import time

def send_command(port, command):
    """Send command to ESP32 clock"""
    ser = serial.Serial(port, 115200, timeout=1)
    time.sleep(2)  # Wait for serial to initialize
    
    ser.write(f"{command}\n".encode())
    
    # Read response
    time.sleep(0.5)
    response = ser.read_all().decode()
    
    ser.close()
    return response

# Examples
print(send_command('/dev/ttyUSB0', 'STATUS'))
print(send_command('/dev/ttyUSB0', 'ALARM SET 0 07:30 Morning'))
print(send_command('/dev/ttyUSB0', 'TIMER 15:00 Tea'))
```

#### Monitor Serial Output

```python
import serial

def monitor_serial(port):
    """Monitor ESP32 serial output for events"""
    ser = serial.Serial(port, 115200, timeout=1)
    
    print("Monitoring ESP32 clock...")
    
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode().strip()
            print(f"Received: {line}")
            
            # Detect alarm trigger
            if '[Alarm]' in line and 'triggered' in line.lower():
                print("🔔 ALARM TRIGGERED!")
                # Trigger action here
                
            # Detect timer expire
            if '[Timer]' in line and 'expired' in line.lower():
                print("⏰ TIMER EXPIRED!")
                # Trigger action here

monitor_serial('/dev/ttyUSB0')
```

### IFTTT Integration

**Webhook trigger when alarm rings:**

1. Monitor Serial output with Python script
2. When alarm detected, send HTTP POST:

```python
import requests

def trigger_ifttt(event, key, value1='', value2='', value3=''):
    """Trigger IFTTT webhook"""
    url = f"https://maker.ifttt.com/trigger/{event}/with/key/{key}"
    data = {'value1': value1, 'value2': value2, 'value3': value3}
    requests.post(url, json=data)

# When alarm triggers:
trigger_ifttt('esp32_alarm', 'YOUR_KEY', 
              value1='Morning Alarm', 
              value2='07:30')
```

3. Create IFTTT applet:
   - **IF** Webhooks event "esp32_alarm"
   - **THEN** Send notification / Turn on lights / etc.

---

## Troubleshooting

### WiFi Issues

**Problem: Cannot connect to WiFi**

Check:
1. SSID and password are correct
2. WiFi is 2.4 GHz (ESP32 doesn't support 5 GHz)
3. WiFi network is visible and in range

Solutions:
```
> wifi status
[WiFi] Mode: ALWAYS ON
[WiFi] Status: DISCONNECTED

> wifi set MyHomeWiFi CorrectPassword
> save
> reboot
```

**Problem: WiFi keeps disconnecting**

Check:
1. WiFi signal strength: `> status` (should be > -70 dBm)
2. WiFi mode setting

Solutions:
```
> wifi always      # Force WiFi always on
> save
```

**Problem: Web interface not loading**

Check:
1. Verify IP address on display or via Serial: `> status`
2. Ping device: `ping 192.168.1.100`
3. Check firewall settings
4. Try different browser

**Problem: Smart WiFi not working**

Check timing settings:
```
> status
(Look for Smart WiFi settings)

WiFi Mode: SMART
  Auto-off timeout: 10 min
  NTP Sync interval: 360 min
```

Force WiFi on:
```
> wifi on
(WiFi stays on for 10 minutes)
```

### Time Sync Issues

**Problem: Time not syncing**

Check:
1. WiFi connection: `> wifi status`
2. NTP server accessibility

Solutions:
```
> ntp pool.ntp.org
> sync

# Try alternative servers:
> ntp time.google.com
> sync

> ntp time.cloudflare.com
> sync
```

**Problem: Wrong time displayed**

Check timezone:
```
> status
(Look for Timezone and DST)

Timezone: GMT+3
DST Offset: +0 hours
```

Fix:
```
> tz +3            # Your timezone
> dst +1           # If DST active
> sync
> save
```

**Problem: Time drifting**

NTP sync should occur every 6 hours. Check:
```
> status
(Look for Time Sync and last sync time)

Time Sync: YES
Next NTP sync in: 342 min
```

Force sync:
```
> sync
```

### Display Issues

**Problem: Display is blank**

Check:
1. Power connections
2. I2C connections (SDA/SCL)
3. Brightness setting

Solutions:
```
> brightness 200
```

Check with multimeter:
- Display VCC: 3.3V
- Display SDA/SCL: pull-up to 3.3V

**Problem: Display too dim**

```
> brightness 255    # Maximum
> night off         # Disable night mode
```

**Problem: Display shows garbage**

I2C communication issue. Check:
1. Wiring (short wires, < 20cm recommended)
2. Pull-up resistors (may need 4.7kΩ)
3. I2C address

Reboot:
```
> reboot
```

### Sensor Issues

**Problem: Temperature/Humidity not reading**

Check Serial output:
```
> status

AHT20: NOT FOUND
```

Solutions:
1. Check wiring (SDA, SCL, VCC, GND)
2. Verify I2C address with scanner
3. Try different sensor
4. Check for shorts

**Problem: Pressure not reading**

```
> status

BMP280: NOT FOUND
```

Same as above. BMP280 address should be 0x76.

**Problem: Sensor readings incorrect**

AHT20 may need calibration:
```
Temperature: 50.0 C   (Obviously wrong)
```

This is a hardware issue. Try:
1. Power cycle device
2. Replace sensor
3. Check sensor placement (away from heat sources)

### Alarm Issues

**Problem: Alarm not triggering**

Check:
1. Alarm is enabled: `> alarm list`
2. Time is correct: `> status`
3. Alarm time/date configuration

Solutions:
```
> alarm list
[0] OFF 07:30

> alarm toggle 0
[Alarm] Alarm 0 is now ON

> save
```

**Problem: Alarm triggers at wrong time**

Check timezone:
```
> status
Timezone: GMT+3      # Verify this is correct
Current Time: 12:34:56
```

Fix timezone:
```
> tz +3     # Your correct timezone
> save
```

**Problem: No sound when alarm triggers**

Check:
1. Buzzer connections
2. Buzzer type configuration (line 22 in code)
3. Volume (for passive buzzer, check tone() function)

Test buzzer:
```
> melody test C5 Q D5 Q E5 Q
(Should hear three notes)
```

**Problem: Alarm only plays once (should repeat)**

Check repeat flag:
```
> alarm list
[0] ON 07:00 "Morning"     (No R flag)
```

Add repeat flag:
```
> alarm set 0 07:00 Morning R S
[Alarm] Set alarm 0 for Daily 07:00 [R] [S] 'Morning'
```

### Buzzer Issues

**Problem: No sound from buzzer**

Check:
1. Buzzer type in code (line 22):
   ```cpp
   #define BUZZER_TYPE BUZZER_PASSIVE  // or BUZZER_ACTIVE
   ```
2. Wiring (GPIO 12 to buzzer +)
3. Buzzer polarity

Test:
```
> melody test C5 Q E5 Q G5 Q
```

**Problem: Wrong pitch/no melody (only beeps)**

You have active buzzer but code set to passive:
```cpp
#define BUZZER_TYPE BUZZER_ACTIVE  // Change line 22
```

Re-upload firmware.

**Problem: Melody sounds weird**

For passive buzzer:
1. Check trigger level (line 23):
   ```cpp
   #define LOW_LEVEL_TRIGGER 1  // Try 0 if doesn't work
   ```
2. Some buzzers need external resistor
3. Try simpler melody first

### Memory Issues

**Problem: Device crashes/reboots randomly**

Check free memory:
```
> status

Free RAM: 50 KB   (Low! Should be > 100 KB)
```

This indicates memory leak or issue. Try:
1. Reboot: `> reboot`
2. Factory reset: `> erase` then `> reboot`
3. Check for recursive functions in custom code

**Problem: Cannot save settings**

NVS may be full:
```
> save
Error: NVS write failed
```

Solutions:
```
> erase      # Warning: Erases ALL settings!
> reboot
(Re-configure from scratch)
```

### LED Indicator Issues

**Problem: LED not lighting**

Check:
1. WS2812B wiring (GPIO 21, VCC, GND)
2. Power supply (WS2812B needs 5V)
3. LED type (individual RGB vs WS2812B)

For individual RGB LEDs, modify code pins (lines 28-30).

**Problem: LED shows wrong colors**

RGB order may be different:
- WS2812B: GRB order
- WS2812: RGB order

Check datasheet and adjust in code.

### Button Issues

**Problem: Button not responding**

Check:
1. Button connection (GPIO 0 to GND)
2. Button is Normally Open type
3. Try Serial commands instead:
   ```
   > alarm toggle 0    # Instead of button
   ```

**Problem: Button too sensitive/triggers randomly**

May need external pull-up resistor:
- 10kΩ between GPIO 0 and 3.3V

Or enable internal pull-up in code (already enabled by default).

---

## API Reference

### Current API Endpoints

The device currently has minimal HTTP API:

#### GET /

Returns: HTML page with system status

Example:
```bash
curl http://192.168.1.100/
```

Response: Full HTML page

#### GET /generate_204

Captive portal detection endpoint

#### GET /hotspot-detect.html

Captive portal detection endpoint

#### POST /setup/save

Save configuration in setup mode

### Future API (Planned Enhancements)

**GET /api/status**
- Returns JSON with system status

**POST /api/alarm**
- Set alarm via JSON

**POST /api/timer**
- Set timer via JSON

**POST /api/brightness**
- Set brightness

**GET /api/sensors**
- Get sensor readings

---

## Best Practices

### Power Management

1. **Use Smart WiFi mode** for battery operation
2. **Enable night mode** to reduce display power
3. **Lower brightness** when possible
4. **Configure longer sync intervals** if network unreliable

### Reliability

1. **Save important settings:** Use `> save` after configuration
2. **Backup alarm settings:** Write down or export alarm configurations
3. **Regular NTP sync:** Keep automatic sync enabled
4. **Monitor sensors:** Check `> status` periodically

### Security

1. **Change default credentials** if hardcoded
2. **Use strong WiFi password**
3. **Disable setup mode** after initial configuration (hold BOOT 10+ sec to re-enter)
4. **Keep web interface on private network** (no port forwarding)

### Maintenance

1. **Check updates:** Look for firmware updates on GitHub
2. **Clean display:** Gently wipe OLED with soft cloth
3. **Check connections:** Verify wiring periodically
4. **Monitor free RAM:** Should stay > 150 KB

---

## Example Use Cases

### Use Case 1: Smart Wake-Up System

**Goal:** Gradually wake up with lights and alarm

**Setup:**
1. Set ESP32 alarm for 7:00 AM
2. Configure Home Assistant automation:
   ```yaml
   automation:
     - alias: "Gradual Wake Up"
       trigger:
         - platform: time
           at: "06:50:00"
       action:
         - service: light.turn_on
           entity_id: light.bedroom
           data:
             brightness: 10
         - delay: '00:05:00'
         - service: light.turn_on
           entity_id: light.bedroom
           data:
             brightness: 100
   ```
3. ESP32 alarm triggers at 7:00 AM (lights already at 100%)

### Use Case 2: Cooking Timer with Notification

**Goal:** Get phone notification when cooking timer expires

**Setup:**
1. Start timer on ESP32:
   ```
   > timer 15:00 Pizza
   ```

2. Monitor with Python script:
   ```python
   # monitor_timer.py
   import serial
   import requests
   
   ser = serial.Serial('/dev/ttyUSB0', 115200)
   
   while True:
       line = ser.readline().decode()
       if 'Timer' in line and 'expired' in line:
           # Send to phone via Pushover
           requests.post('https://api.pushover.net/1/messages.json', data={
               'token': 'APP_TOKEN',
               'user': 'USER_KEY',
               'message': 'Pizza is ready!'
           })
   ```

### Use Case 3: Energy-Saving Weather Station

**Goal:** Battery-powered weather station with long runtime

**Setup:**
1. Enable Smart WiFi mode:
   ```
   > wifi smart
   > save
   ```

2. Configure long intervals via web interface:
   - NTP sync: Every 12 hours (720 min)
   - Weather update: Every 2 hours (120 min)
   - Auto-off: 5 minutes

3. Enable night mode:
   ```
   > night on 22:00-08:00 5
   > save
   ```

**Result:** ~50% power savings, good for 2-3 days on battery

### Use Case 4: Multi-Alarm Work Schedule

**Goal:** Different alarms for different days

**Setup:**
```
# Early shift (Mon, Wed, Fri)
> alarm set 0 135 06:00 Early shift R S

# Late shift (Tue, Thu)
> alarm set 1 24 08:00 Late shift R S

# Weekend alarm (Sat, Sun)
> alarm set 2 67 09:00 Weekend R S

# Lunch break (Mon-Fri)
> alarm set 3 12345 12:30 Lunch R S

> alarm list
(Verify all alarms)

> save
```

---

## Appendix

### Pin Reference Table

| Function | GPIO | Alternative | Notes |
|----------|------|-------------|-------|
| OLED SDA | 10 | Any I2C SDA | Shared with sensors |
| OLED SCL | 11 | Any I2C SCL | Shared with sensors |
| AHT20 SDA | 10 | Same as OLED | I2C address 0x38 |
| AHT20 SCL | 11 | Same as OLED | |
| BMP280 SDA | 10 | Same as OLED | I2C address 0x76 |
| BMP280 SCL | 11 | Same as OLED | |
| Buzzer | 12 | Any PWM pin | GPIO 12 default |
| RGB LED | 21 | Any pin | WS2812B data |
| Button | 0 | Any pin | Internal pull-up |

### Command Quick Reference

```
TIME YYYY-MM-DD HH:MM:SS    Set time manually
SYNC                         Sync with NTP server
TZ +3                        Set timezone
DST +1                       Set DST offset
NTP pool.ntp.org            Set NTP server
CITY Moscow                  Set weather city
BRIGHTNESS 200               Set display brightness
SAVE                         Save all settings
STATUS                       Show system status
REBOOT                       Restart device
LANGUAGE EN/RU               Switch language

ALARM LIST                   List all alarms
ALARM SET 0 07:30 Text R S  Set alarm (daily)
ALARM SET 1 12345 07:00 R S Set alarm (weekdays)
ALARM SET 2 2026-01-26 14:30 Set alarm (one-time)
ALARM TOGGLE 0               Enable/disable alarm
ALARM CLEAR 0                Clear alarm
ALARM CLEARALL               Clear all alarms

TIMER 15:00 Text             Start 15-minute timer
TIMER 01:30:00 Text          Start 1.5-hour timer
TIMER CLEAR                  Stop timer

WIFI SET ssid pass           Set WiFi credentials
WIFI ALWAYS                  WiFi always on mode
WIFI SMART                   Smart WiFi mode
WIFI ON                      Manual WiFi on (10 min)
WIFI STATUS                  Show WiFi status

NIGHT ON 23:00-07:00 10     Enable night mode
NIGHT OFF                    Disable night mode
NIGHT STATUS                 Show night mode status

MELODY ALARM notes          Set default alarm melody
MELODY TIMER notes          Set default timer melody
MELODY TEST notes           Test melody
MELODY SAVE                 Save melodies to NVS
```

### Melody Note Reference

| Note | Frequency | | Note | Frequency |
|------|-----------|---|------|-----------|
| C3 | 131 Hz | | C6 | 1047 Hz |
| D3 | 147 Hz | | D6 | 1175 Hz |
| E3 | 165 Hz | | E6 | 1319 Hz |
| F3 | 175 Hz | | F6 | 1397 Hz |
| G3 | 196 Hz | | G6 | 1568 Hz |
| A3 | 220 Hz | | A6 | 1760 Hz |
| B3 | 247 Hz | | B6 | 1976 Hz |
| C4 | 262 Hz | | C7 | 2093 Hz |
| D4 | 294 Hz | | D7 | 2349 Hz |
| E4 | 330 Hz | | E7 | 2637 Hz |
| F4 | 349 Hz | | F7 | 2794 Hz |
| G4 | 392 Hz | | G7 | 3136 Hz |
| A4 | 440 Hz | | A7 | 3520 Hz |
| B4 | 494 Hz | | B7 | 3951 Hz |
| C5 | 523 Hz |
| D5 | 587 Hz |
| E5 | 659 Hz |
| F5 | 698 Hz |
| G5 | 784 Hz |
| A5 | 880 Hz |
| B5 | 988 Hz |

Duration codes:
- W = Whole (1000ms)
- H = Half (500ms)
- Q = Quarter (250ms)
- E = Eighth (125ms)
- S = Sixteenth (63ms)

### Error Codes

Currently no formal error codes. Errors reported as text messages.

Common error messages:
- "WiFi connection failed!" - Cannot connect to network
- "Time sync failed!" - NTP server unreachable
- "Invalid index" - Alarm index out of range (0-9)
- "NVS write failed" - Storage full or hardware issue

### Glossary

- **NTP** - Network Time Protocol, for time synchronization
- **NVS** - Non-Volatile Storage, for persistent settings
- **DST** - Daylight Saving Time offset
- **GMT** - Greenwich Mean Time (UTC+0)
- **I2C** - Inter-Integrated Circuit, serial protocol for sensors
- **SPI** - Serial Peripheral Interface (not used in this project)
- **PWM** - Pulse Width Modulation, for buzzer tones
- **RSSI** - Received Signal Strength Indicator, WiFi signal quality
- **WS2812B** - Type of RGB LED strip
- **Passive buzzer** - Requires PWM signal, can play melodies
- **Active buzzer** - Just on/off, single frequency

---

**End of Manual**

For latest updates and support, visit project repository.
