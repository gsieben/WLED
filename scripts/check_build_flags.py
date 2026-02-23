#!/usr/bin/env python3
"""
Simple repository guard to detect malformed build flags and embedded secrets in INI files.
Exits with non-zero when it finds problems.
"""
import sys
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Files that legitimately contain local secrets and should be ignored by this check
IGNORE_FILES = {'platformio_override.ini'}

BAD_D_RE = re.compile(r"(^|\s)-\s+D")  # matches '- D'
SECRET_RE = re.compile(r"-D\s*([A-Za-z0-9_]*?(PASS|PASSWORD|SECRET|KEY)[A-Za-z0-9_]*)\s*=\s*['\"].+['\"]", re.IGNORECASE)

errors = []

for path in ROOT.rglob('*.ini'):
    if path.name in IGNORE_FILES:
        # skip legitimate local override files that store secrets
        continue
    rel = path.relative_to(ROOT)
    with path.open('r', encoding='utf-8', errors='ignore') as f:
        for i, line in enumerate(f, start=1):
            s = line.strip()
            if not s or s.startswith(';') or s.startswith('#'):
                continue
            if BAD_D_RE.search(line):
                errors.append(f"{rel}:{i}: malformed '- D' (space between '-' and 'D') -> '{line.strip()}'")
            m = SECRET_RE.search(line)
            if m:
                errors.append(f"{rel}:{i}: secret detected in build flags for '{m.group(1)}' -> please remove from versioned files and use .env or runtime config")

if errors:
    print("ERROR: Build flag checks failed:\n")
    for e in errors:
        print(" - ", e)
    print("\nRemediation suggestions:\n - Remove sensitive -D flags from .ini files and put them in a local .env (see .env.sample).")
    print(" - Fix occurrences of '- D' (space between - and D) so flags are '-D...'.")
    sys.exit(1)

print("OK: No malformed build flags or embedded secrets found in .ini files.")
sys.exit(0)
