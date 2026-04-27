# ESP32 OTA Firmware Project

This project implements a complete Over-The-Air (OTA) update system for ESP32 devices with automatic firmware updates from an S3 bucket.

## Features

- **Secure OTA Updates**: Encrypted firmware downloads from S3
- **User-Configurable WiFi**: No hardcoded credentials, setup via Serial Monitor
- **Persistent Settings**: WiFi credentials stored in EEPROM, survive OTA updates
- **Version Management**: Automatic version checking and comparison
- **First-Time Setup**: User-friendly configuration mode
- **Factory Reset**: Clear credentials and reconfigure anytime
- **Rollback Protection**: Only installs valid firmware
- **Error Handling**: Comprehensive error recovery
- **Debug Support**: Detailed serial output for troubleshooting

## Project Structure

```
ESP32_OTA/
├── src/
│   ├── main.cpp              # Main application logic
│   ├── ota_manager.h         # OTA update manager header
│   ├── ota_manager.cpp       # OTA update manager implementation
│   ├── led_controller.h      # LED controller header
│   └── led_controller.cpp    # LED controller implementation
├── .github/
│   └── workflows/
│       ├── dev-build.yml     # Dev branch CI/CD
│       └── main-deploy.yml   # Main branch deployment
├── platformio.ini            # PlatformIO configuration
├── README.md                 # This file
└── version.json              # Version information template
```

## CI/CD Pipeline

### Dev Branch
- Trigger: Push to `dev` branch
- Actions:
  - Build firmware
  - Run tests
  - Check for compilation errors

### Main Branch
- Trigger: Push or Pull Request to `main` branch
- Actions:
  - Build firmware
  - Upload binary to S3 bucket
  - Update version.json

## Setup Instructions

### 1. Prerequisites
- PlatformIO IDE or CLI
- AWS Account with S3 bucket
- GitHub repository
- ESP32 development board

### 2. Configuration
Update the following in `platformio.ini`:
```ini
build_flags = 
    -DCORE_DEBUG_LEVEL=3
    -DFIRMWARE_VERSION=\"1.0.5\"
```

Create `.env` file locally:
```bash
OTA_SERVER=your-s3-bucket.s3.amazonaws.com
VERSION_CHECK_ENDPOINT=https://your-s3-bucket.s3.amazonaws.com/version.json
```

**Note**: WiFi credentials are configured by user at first boot via Serial Monitor - no hardcoded credentials needed!

### 3. GitHub Secrets
Set up these secrets in your GitHub repository:
- `OTA_SERVER`: S3 bucket URL (e.g., `esp32-ota-bucket.s3.amazonaws.com`)
- `VERSION_CHECK_ENDPOINT`: Version check URL (e.g., `https://esp32-ota-bucket.s3.amazonaws.com/version.json`)
- `AWS_ACCESS_KEY_ID`: AWS access key (for S3 upload)
- `AWS_SECRET_ACCESS_KEY`: AWS secret key (for S3 upload)
- `AWS_S3_BUCKET`: S3 bucket name (for S3 upload)
- `AWS_REGION`: AWS region (for S3 upload)

**Note**: Only `OTA_SERVER` and `VERSION_CHECK_ENDPOINT` are required for the ESP32 firmware. AWS credentials are only needed for GitHub Actions deployment.

### 4. S3 Bucket Setup
1. Create an S3 bucket for firmware storage
2. Enable public access for version.json
3. Set up bucket policy for firmware access
4. Create version.json with latest version info

### 5. First-Time User Setup
After flashing, open Serial Monitor (115200 baud) and configure WiFi:

```
=== ESP32 OTA Setup Mode ===
Please configure your WiFi credentials
Format: SSID,PASSWORD (e.g., MyWiFi,MyPassword)
Type 'reset' to clear credentials and restart setup
Waiting for input...

MyHomeWiFi,MySecretPassword123
Setting credentials:
SSID: MyHomeWiFi
Password: 15 characters
Setup complete! Credentials saved.
```

### 6. Building and Flashing
```bash
# Build the firmware
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

## Version Management

The system uses semantic versioning (e.g., 1.0.0, 1.0.1, 1.1.0). Update the version in `platformio.ini`:

```ini
build_flags = 
    -DFIRMWARE_VERSION=\"1.0.0\"
```

## Update Flow

1. **Startup**: Device checks for updates immediately after connecting to WiFi
2. **Hourly Check**: Device checks for updates every hour
3. **Update Available**: If new version is found, firmware is downloaded
4. **Installation**: Firmware is installed and device restarts
5. **Verification**: New version is confirmed running

## LED Indicators

- **Solid ON**: WiFi connected and device running normally
- **Blinking**: Device is running (continuous blink in loop)
- **3 Blinks**: WiFi connection failed (rapid blink on connection failure)
- **5 Blinks**: OTA update failed (rapid blink on update failure)

## EEPROM Memory Layout

The ESP32 uses 128 bytes of EEPROM to store persistent settings:

```
Address 0-15:    Firmware version (16 bytes)
Address 16-47:   User WiFi SSID (32 bytes)
Address 48-111:  User WiFi Password (64 bytes)
Address 112:      Configuration flags (1 byte)
                  0xAA = Configured, 0x00 = Not configured
Address 113-127:  Reserved (15 bytes)
```

## Troubleshooting

### Common Issues
1. **First Boot Setup**: Device enters setup mode - configure WiFi via Serial Monitor
2. **WiFi Connection Fails**: Check SSID and password format (SSID,PASSWORD)
3. **Update Fails**: Verify S3 bucket permissions and URL
4. **Build Fails**: Check PlatformIO configuration and dependencies
5. **Credentials Reset**: Type 'reset' in setup mode to clear saved credentials

### Debug Output
Enable serial monitor at 115200 baud to see detailed debug information:
```bash
pio device monitor
```

## License

This project is open source and available under the MIT License.
