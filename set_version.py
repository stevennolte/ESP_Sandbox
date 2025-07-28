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

# Replace a line like: const int VERSION = ...;
content = re.sub(r'(const\s+int\s+VERSION\s*=\s*)\d+;', rf'\1{version};', content)

with open("main.cpp", "w") as f:
    f.write(content)