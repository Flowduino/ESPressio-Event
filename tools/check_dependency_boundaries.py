#!/usr/bin/env python3

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
MANIFEST = ROOT / "library.json"

FORBIDDEN_PREFIXES = (
    "ESPressio_ESPNow",
    "ESPressio_Socket",
    "ESPressio_Command",
    "ESPressio_Security",
)
FORBIDDEN_MANIFEST_TERMS = (
    "ESPressio-ESP-Now",
    "ESPressio-Sockets",
    "ESPressio-Command",
    "ESPressio-Security",
)

include_pattern = re.compile(r'^\s*#\s*include\s*[<\"]([^>\"]+)[>\"]')
failures = []

for path in sorted(SRC.rglob("*")):
    if not path.is_file() or path.suffix not in {".h", ".hh", ".hpp", ".cpp", ".cc"}:
        continue
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        match = include_pattern.match(line)
        if not match:
            continue
        include = Path(match.group(1)).name
        if include.startswith(FORBIDDEN_PREFIXES):
            failures.append(f"{path.relative_to(ROOT)}:{line_number}: forbidden include {include}")

manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
for dependency in manifest.get("dependencies", []):
    rendered = json.dumps(dependency, sort_keys=True)
    for term in FORBIDDEN_MANIFEST_TERMS:
        if term in rendered:
            failures.append(f"library.json: forbidden direct dependency {term}")

if failures:
    print("Event dependency-boundary violations detected:")
    for failure in failures:
        print(f"  - {failure}")
    sys.exit(1)

print("Event dependency boundaries OK: no ESP-Now, Sockets, Command, or Security reverse dependencies.")
