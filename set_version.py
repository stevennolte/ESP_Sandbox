import sys
import json
import re

version = sys.argv[1]

# Update firmware.json
with open("firmware.json", "r") as f:
    data = json.load(f)
data["version"] = int(version)
with open("firmware.json", "w") as f:
    json.dump(data, f, indent=2)

# Update main.cpp
with open("src/main.cpp", "r") as f:
    content = f.read()

# Replace a line like: const int FIRMWARE_VERSION = ...;
# Use string concatenation instead of f-string to avoid regex group reference issues
replacement = r'\1' + version + ';'
content = re.sub(r'(const\s+int\s+FIRMWARE_VERSION\s*=\s*)\d+;', replacement, content)

with open("src/main.cpp", "w") as f:
    f.write(content)