# BlessiMart

Multi-seller online shopping website (capstone project).
Stack: C++20 + Drogon + SQLite + HTML/CSS/JS + CMake.

## Prerequisites (Windows)

1. Install Visual Studio 2022 with "Desktop development with C++" + CMake tools.
2. Install Drogon: follow https://github.com/drogonframework/drogon (vcpkg recommended):
   `vcpkg install drogon`
3. Install Git and CMake 3.20+.

## Build & Run (Step 1)

```powershell
cd BlessiMart
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
.\build\Release\BlessiMart.exe
```

Then open:
- http://localhost:8080/ -> frontend/index.html
- http://localhost:8080/ping -> "BlessiMart server is running!"

IMPORTANT: run the .exe from the `BlessiMart/` folder so it finds `config.json` and `frontend/`:
```powershell
cd BlessiMart
.\build\Release\BlessiMart.exe
```

## Structure
See Step 1 plan: src/, frontend/, config.json, schema.sql.
