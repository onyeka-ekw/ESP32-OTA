# ESP32 OTA Firmware Project

This project implements a complete Over-The-Air (OTA) update system for ESP32 devices with automatic firmware updates from an S3 bucket.

## Features

- **Automatic OTA Updates**: Checks for updates every hour and on startup
- **GitHub CI/CD Pipeline**: Automated builds and deployments
- **Version Management**: Semantic versioning with update checking
- **LED Status Indicator**: Visual feedback for system status
- **WiFi Connectivity**: Automatic reconnection if connection is lost
- **Error Handling**: Robust error handling and recovery

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
Update the following in `src/main.cpp`:
```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* OTA_SERVER = "your-s3-bucket.s3.amazonaws.com";
const char* VERSION_CHECK_ENDPOINT = "https://your-s3-bucket.s3.amazonaws.com/version.json";
```

### 3. GitHub Secrets
Set up these secrets in your GitHub repository:
- `AWS_ACCESS_KEY_ID`: AWS access key
- `AWS_SECRET_ACCESS_KEY`: AWS secret key
- `AWS_S3_BUCKET`: S3 bucket name
- `AWS_REGION`: AWS region

### 4. S3 Bucket Setup
1. Create an S3 bucket for firmware storage
2. Enable public access for version.json
3. Set up bucket policy for firmware access
4. Create version.json with latest version info

### 5. Building and Flashing
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

## LED Status Codes

- **Solid ON**: WiFi connected and device running normally
- **Blinking**: Device is checking for updates
- **3 Blinks**: WiFi connection failed
- **5 Blinks**: OTA update failed

## Troubleshooting

### Common Issues
1. **WiFi Connection Fails**: Check SSID and password
2. **Update Fails**: Verify S3 bucket permissions and URL
3. **Build Fails**: Check PlatformIO configuration and dependencies

### Debug Output
Enable serial monitor at 115200 baud to see detailed debug information:
```bash
pio device monitor
```

## License

This project is open source and available under the MIT License.
