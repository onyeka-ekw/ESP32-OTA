Import('env')
import os

# Load environment variables from .env file
env_file = os.path.join(env['PROJECT_DIR'], '.env')

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
else:
    print("Warning: .env file not found!")
    print("Please copy .env.example to .env and add your credentials")
