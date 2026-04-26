Import('env')
import os

# Required environment variables for ESP32 OTA (WiFi credentials optional)
required_vars = [
    'OTA_SERVER',
    'VERSION_CHECK_ENDPOINT'
]

# Optional environment variables (can be empty)
optional_vars = [
    'WIFI_SSID',
    'WIFI_PASSWORD'
]

# First try to load from .env file (for local development)
env_file = os.path.join(env['PROJECT_DIR'], '.env')
loaded_from_env = False

if os.path.exists(env_file):
    print("Loading environment variables from .env file...")
    with open(env_file, 'r') as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith('#') and '=' in line:
                key, value = line.split('=', 1)
                key = key.strip()
                value = value.strip()
                # Add to build environment
                env[key] = value
                # Add to build flags
                env.Append(CPPDEFINES=[f'{key}=\\"{value}\\"'])
                print(f"Loaded: {key}")
                loaded_from_env = True
else:
    print("No .env file found, checking for GitHub Secrets...")

# If no .env file, try to load from system environment (GitHub Actions)
if not loaded_from_env:
    print("Loading environment variables from GitHub Secrets...")
    for var in required_vars + optional_vars:
        value = os.environ.get(var)
        if value:
            # Add to build environment
            env[var] = value
            # Add to build flags
            env.Append(CPPDEFINES=[f'{var}=\\"{value}\\"'])
            print(f"Loaded: {var}")
        else:
            # For optional vars, set empty string if not found
            if var in optional_vars:
                env[var] = ""
                env.Append(CPPDEFINES=[f'{var}=\\"\\"'])
                print(f"Optional {var} not found, using empty string")
            else:
                print(f"Warning: {var} not found in environment!")

# Verify all required variables are loaded
missing_vars = []
for var in required_vars:
    if var not in env:
        missing_vars.append(var)

if missing_vars:
    print(f"ERROR: Missing required variables: {', '.join(missing_vars)}")
    print("For local development: create .env file with these variables")
    print("For GitHub Actions: add these as Repository Secrets")
    exit(1)
else:
    print("All required environment variables loaded successfully!")
    print("Optional variables (WiFi credentials) will be configured by user at boot")
