# AWS S3 Setup Guide

This guide explains how to set up an S3 bucket for ESP32 OTA firmware distribution.

## 1. Create S3 Bucket

1. Sign in to AWS Management Console
2. Navigate to S3 service
3. Click "Create bucket"
4. Enter bucket name (e.g., `esp32-ota-firmware-bucket`)
5. Select your AWS region
6. Block all public access (we'll handle permissions via bucket policy)
7. Enable versioning (recommended)
8. Click "Create bucket"

## 2. Create IAM User

1. Navigate to IAM service
2. Click "Users" → "Add user"
3. Enter username (e.g., `esp32-ota-deployer`)
4. Select "Access key - Programmatic access"
5. Click "Next: Permissions"
6. Attach existing policy: "AmazonS3FullAccess" (or create custom policy)
7. Click "Create user"
8. Save the access key and secret key

## 3. Custom IAM Policy (Optional)

For better security, create a custom policy with minimal permissions:

```json
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": [
                "s3:PutObject",
                "s3:GetObject",
                "s3:DeleteObject",
                "s3:ListBucket",
                "s3:PutObjectAcl"
            ],
            "Resource": [
                "arn:aws:s3:::your-bucket-name",
                "arn:aws:s3:::your-bucket-name/*"
            ]
        }
    ]
}
```

## 4. Bucket Policy

Create a bucket policy to allow public access to version.json while keeping firmware secure:

```json
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "PublicReadForVersionFile",
            "Effect": "Allow",
            "Principal": "*",
            "Action": "s3:GetObject",
            "Resource": "arn:aws:s3:::your-bucket-name/version.json"
        },
        {
            "Sid": "AllowFirmwareAccess",
            "Effect": "Allow",
            "Principal": {
                "AWS": "arn:aws:iam::ACCOUNT-ID:user/esp32-ota-deployer"
            },
            "Action": [
                "s3:PutObject",
                "s3:GetObject",
                "s3:DeleteObject"
            ],
            "Resource": "arn:aws:s3:::your-bucket-name/*"
        }
    ]
}
```

## 5. CORS Configuration

Set up CORS for the bucket if needed:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<CORSConfiguration xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
<CORSRule>
    <AllowedOrigin>*</AllowedOrigin>
    <AllowedMethod>GET</AllowedMethod>
    <AllowedMethod>HEAD</AllowedMethod>
    <MaxAgeSeconds>3000</MaxAgeSeconds>
    <AllowedHeader>Authorization</AllowedHeader>
</CORSRule>
</CORSConfiguration>
```

## 6. GitHub Secrets

Add these secrets to your GitHub repository:

- `AWS_ACCESS_KEY_ID`: Your IAM user access key
- `AWS_SECRET_ACCESS_KEY`: Your IAM user secret key
- `AWS_S3_BUCKET`: Your S3 bucket name
- `AWS_REGION`: Your AWS region (e.g., `us-east-1`)

## 7. Test Setup

1. Create a test version.json file:
```json
{
  "version": "1.0.0",
  "firmware_url": "https://your-bucket.s3.amazonaws.com/firmware/test.bin",
  "release_notes": "Test release",
  "required": false,
  "released_at": "2024-01-01T00:00:00Z"
}
```

2. Upload to S3:
```bash
aws s3 cp version.json s3://your-bucket/
aws s3api put-object-acl --bucket your-bucket --key version.json --acl public-read
```

3. Test access:
```bash
curl https://your-bucket.s3.amazonaws.com/version.json
```

## 8. Security Considerations

- **Firmware Files**: Keep firmware files private - only accessible via signed URLs or IAM credentials
- **Version File**: Make version.json publicly readable so ESP32 can check for updates
- **IAM Permissions**: Use principle of least privilege - only grant necessary permissions
- **Versioning**: Enable S3 versioning to maintain firmware history
- **Access Logging**: Enable S3 access logging to monitor firmware downloads

## 9. Cost Optimization

- **Lifecycle Policies**: Set up lifecycle rules to archive old firmware versions
- **Storage Class**: Use S3 Standard for current firmware, Glacier for old versions
- **Request Monitoring**: Monitor S3 requests to detect unusual activity

## 10. Backup Strategy

- **Cross-Region Replication**: Replicate bucket to another AWS region
- **Version History**: Maintain multiple firmware versions for rollback capability
- **Local Backups**: Keep local copies of critical firmware versions
