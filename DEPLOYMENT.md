# ESP32 OTA Deployment Guide

This guide covers the complete deployment process for the ESP32 OTA system.

## Prerequisites

- ESP32 development board
- PlatformIO IDE or CLI
- AWS account with S3 bucket
- GitHub repository
- WiFi network

## 1. Initial Setup

### 1.1 Clone and Configure

```bash
git clone https://github.com/your-username/ESP32_OTA.git
cd ESP32_OTA
```

### 1.2 Update Configuration

Edit `src/main.cpp` and update these variables:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* OTA_SERVER = "your-s3-bucket.s3.amazonaws.com";
const char* VERSION_CHECK_ENDPOINT = "https://your-s3-bucket.s3.amazonaws.com/version.json";
```

### 1.3 Set Version

Edit `platformio.ini` and update the firmware version:

```ini
build_flags = 
    -DFIRMWARE_VERSION=\"1.0.0\"
```

## 2. AWS S3 Setup

Follow the [AWS_S3_SETUP.md](AWS_S3_SETUP.md) guide to:

1. Create S3 bucket
2. Set up IAM user
3. Configure bucket policy
4. Set up GitHub secrets

## 3. GitHub Repository Setup

### 3.1 Create Repository

1. Create a new GitHub repository
2. Push the code to the repository
3. Set up GitHub secrets:
   - `AWS_ACCESS_KEY_ID`
   - `AWS_SECRET_ACCESS_KEY`
   - `AWS_S3_BUCKET`
   - `AWS_REGION`

### 3.2 Branch Structure

Create and push branches:

```bash
git checkout -b dev
git push origin dev
git checkout main
git push origin main
```

## 4. Initial Deployment

### 4.1 Build and Test Locally

```bash
# Build firmware
pio run

# Upload to ESP32
pio run --target upload

# Monitor output
pio device monitor
```

### 4.2 Push to Main Branch

```bash
git add .
git commit -m "Initial release v1.0.0"
git push origin main
```

This will trigger the GitHub Actions workflow:
1. Build firmware
2. Upload to S3
3. Update version.json
4. Create GitHub release

## 5. Development Workflow

### 5.1 Development Branch

For new features and bug fixes:

```bash
git checkout dev
# Make changes
git add .
git commit -m "Add new feature"
git push origin dev
```

This triggers the dev workflow:
- Build and test
- No deployment to S3
- Build artifacts saved for 7 days

### 5.2 Release Process

When ready to release:

```bash
git checkout main
git merge dev
git tag v1.0.1
git push origin main --tags
```

This triggers the main workflow:
- Build firmware
- Deploy to S3
- Update version.json
- Create GitHub release

## 6. OTA Update Process

### 6.1 Automatic Updates

The ESP32 device will:

1. Check for updates on startup
2. Check for updates every hour
3. Download new firmware if available
4. Install and restart

### 6.2 Manual Update Check

To force an update check, restart the device or modify the update interval in code.

### 6.3 Update Verification

Monitor serial output at 115200 baud:

```bash
pio device monitor
```

Look for these messages:
- "Checking for updates..."
- "Update available!"
- "OTA update completed successfully!"

## 7. Troubleshooting

### 7.1 Common Issues

**WiFi Connection Failed**
- Check SSID and password
- Verify WiFi network is available
- Check signal strength

**Update Failed**
- Verify S3 bucket permissions
- Check firmware URL in version.json
- Ensure firmware file is accessible

**Build Failed**
- Check PlatformIO configuration
- Verify library dependencies
- Check for syntax errors

### 7.2 Debug Mode

Enable detailed logging by increasing debug level in `platformio.ini`:

```ini
build_flags = 
    -DCORE_DEBUG_LEVEL=5
    -DCONFIG_OTA_UPDATE
    -DFIRMWARE_VERSION=\"1.0.0\"
```

### 7.3 Recovery

If device fails after update:

1. Flash firmware manually using USB
2. Check version.json for correct firmware URL
3. Verify firmware integrity

## 8. Monitoring and Maintenance

### 8.1 Version Management

- Use semantic versioning (MAJOR.MINOR.PATCH)
- Update version in platformio.ini
- Maintain changelog in release notes

### 8.2 S3 Bucket Monitoring

- Monitor storage usage
- Check access logs
- Set up lifecycle policies

### 8.3 GitHub Actions Monitoring

- Check workflow runs in GitHub
- Monitor build times
- Review deployment logs

## 9. Security Best Practices

### 9.1 Firmware Security

- Sign firmware binaries (optional)
- Validate firmware integrity
- Use HTTPS for all communications

### 9.2 AWS Security

- Rotate IAM credentials regularly
- Use least privilege principle
- Enable MFA for AWS accounts

### 9.3 Git Security

- Use protected branches
- Require pull requests for main branch
- Review code changes

## 10. Performance Optimization

### 10.1 Update Frequency

Adjust update interval based on use case:
- Development: 5-15 minutes
- Production: 1-24 hours

### 10.2 Firmware Size

- Optimize code size
- Remove unused libraries
- Use appropriate partition scheme

### 10.3 Network Optimization

- Use efficient HTTP requests
- Implement retry logic
- Handle network timeouts

## 11. Advanced Features

### 11.1 Rollback Capability

- Maintain previous versions in S3
- Implement rollback logic in firmware
- Use version constraints

### 11.2 A/B Updates

- Implement dual partition scheme
- Test updates before committing
- Automatic rollback on failure

### 11.3 Staged Rollouts

- Deploy to subset of devices
- Monitor for issues
- Gradual rollout to all devices
