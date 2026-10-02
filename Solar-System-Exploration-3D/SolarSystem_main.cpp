// ============================================================
// Solar System — OOP Refactored Entry Point
// All subsystems are organized into logical module files.
// This unity-build main.cpp compiles everything together.
// ============================================================
#define _CRT_SECURE_NO_WARNINGS

// STB_IMAGE_IMPLEMENTATION must be defined exactly once,
// in the file that is actually compiled by Visual Studio.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

// ---- Pull in the full implementation ----
// ($olar$ystem_impl.cpp is excluded from build in vcxproj;
//  it is compiled only by being #included here.)
#include "$olar$ystem_impl.cpp"
