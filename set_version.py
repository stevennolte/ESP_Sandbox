import sys
import json
import re

if len(sys.argv) != 3:
    print("Usage: python set_version.py <major_version> <minor_version>")
    sys.exit(1)

major_version = sys.argv[1]
minor_version = sys.argv[2]

# Calculate numeric version for firmware (e.g., 9.14 becomes 914)
# Format: MMMMNN where MMMM = major, NN = minor (up to 99)
numeric_version = int(major_version) * 100 + int(minor_version)
version_string = f"{major_version}.{minor_version}"

print(f"Setting version to {version_string} (numeric: {numeric_version})")

# Update firmware.json
with open("firmware.json", "r") as f:
    data = json.load(f)
data["version"] = numeric_version
data["version_string"] = version_string
with open("firmware.json", "w") as f:
    json.dump(data, f, indent=2)

# Update main.cpp
with open("src/main.cpp", "r") as f:
    content = f.read()

# Replace a line like: const int FIRMWARE_VERSION = ...;
# Use lambda function to avoid regex group reference issues
def replace_version(match):
    return match.group(1) + str(numeric_version) + ';'

content = re.sub(r'(const\s+int\s+FIRMWARE_VERSION\s*=\s*)\d+;', replace_version, content)

with open("src/main.cpp", "w") as f:
    f.write(content)

print(f"Updated firmware.json and main.cpp with version {version_string}")