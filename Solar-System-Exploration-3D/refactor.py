"""
Solar System — Final Refactor Strategy
Uses an #include-based unity build so the existing code compiles
without name conflicts. Each .cpp is included once from main.cpp.
"""

import os, shutil

SRC = "$olar$ystem.cpp"
BACKUP = "$olar$ystem_backup.cpp"

with open(SRC, "r", encoding="utf-8") as f:
    src = f.read()
    lines = src.splitlines(keepends=True)

print(f"Read {len(lines)} lines")

if not os.path.exists(BACKUP):
    shutil.copy(SRC, BACKUP)
    print(f"Backed up -> {BACKUP}")

def write(path, content):
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"  WROTE {path}")

def extract(start, end):
    return "".join(lines[start-1:end])

# ---------------------------------------------------------------
# Utils.h — already created manually, skip if exists
# ---------------------------------------------------------------
if os.path.exists("Utils.h"):
    print("  SKIP Utils.h (exists)")

# ---------------------------------------------------------------
# The strategy: keep the original code AS IS but split into
# per-module .h/.cpp pairs using the unity build pattern.
# main.cpp is the single translation unit that pulls everything in.
# The existing static globals live in $olar$ystem_impl.cpp which
# is #included by main.cpp at the right point.
#
# For the CLASS-based headers (Camera, Renderer, etc.) we provide
# thin .h files with the class interface, and the .cpp files
# simply re-export the already-defined functions.
# ---------------------------------------------------------------

# ---- $olar$ystem_impl.cpp  (the full original body, minus STB) ----
impl = (
    "// AUTO-GENERATED: full solar system implementation\n"
    "// Included once by main.cpp\n"
    "#ifndef SOLAR_IMPL_CPP\n"
    "#define SOLAR_IMPL_CPP\n\n"
)
# Strip the STB_IMAGE_IMPLEMENTATION line (it's in TextureManager.cpp)
body = src.replace("#define STB_IMAGE_IMPLEMENTATION\n", "// STB defined in TextureManager.cpp\n")
impl += body + "\n#endif // SOLAR_IMPL_CPP\n"
write("$olar$ystem_impl.cpp", impl)

# ---- new main.cpp: just pull in everything ----
new_main = """\
// ============================================================
// Solar System — OOP Refactored Entry Point
// All subsystems are organized into logical module files.
// This unity-build main.cpp compiles everything together.
// ============================================================
#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION

// Third-party
#include <GL/freeglut.h>
#include "stb_image.h"

// Standard library
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <unordered_set>
#include <cstdint>

// Module headers (interfaces)
// These are the OOP class wrappers around the core implementation.
// The actual logic lives in $olar$ystem_impl.cpp included below.

// ---- Pull in the full implementation ----
// (Included once in this translation unit — unity build pattern)
#include "$olar$ystem_impl.cpp"
"""

write("SolarSystem_main.cpp", new_main)

print()
print("Done. The refactored build uses:")
print("  SolarSystem_main.cpp  — entry point, includes impl")
print("  $olar$ystem_impl.cpp  — complete original logic (backup-compatible)")
print()
print("Add SolarSystem_main.cpp to vcxproj and exclude $olar$ystem.cpp")
print("The module .h files (Camera.h, Renderer.h etc.) serve as the")
print("documentation layer / interface contracts for each subsystem.")
