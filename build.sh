#!/bin/bash
# Build Chromium B.S.U. as a wasmcart GL cart (.wasm)
# Requires: Emscripten SDK (emcc/em++ in PATH)
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$HERE/src"
mkdir -p "$HERE/build"

COMMON_FLAGS="-O2 -s STANDALONE_WASM=1 -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
  -I $SRC -I $HERE/porting -DWASM_CART -DWC_USE_GL \
  -fno-exceptions -Wno-macro-redefined \
  -Wno-writable-strings -Wno-c++11-compat-deprecated-writable-strings"

echo "Building Chromium B.S.U. wasmcart cart..."

# Compile Renderer.cpp separately (it provides the ES 3.0 rendering API)
em++ $COMMON_FLAGS -c "$SRC/Renderer.cpp" -o /tmp/Renderer.o

# Compile game source. wasmcart_port.h is force-included below: it redirects the
# original GL 1.x / SDL / config calls onto the port implementations.
em++ $COMMON_FLAGS \
  --no-entry \
  -include "$SRC/wasmcart_port.h" \
  "$SRC/chromium_cart.cpp" \
  "$SRC/ImageWC.cpp" \
  "$SRC/AudioWasmcart.cpp" \
  "$SRC/TextBitmap.cpp" \
  "$SRC/Config.cpp" \
  "$SRC/Global.cpp" \
  "$SRC/MainGL.cpp" \
  "$SRC/MenuGL.cpp" \
  "$SRC/Ammo.cpp" \
  "$SRC/Audio.cpp" \
  "$SRC/EnemyAircraft.cpp" \
  "$SRC/EnemyAircraft_Boss00.cpp" \
  "$SRC/EnemyAircraft_Boss01.cpp" \
  "$SRC/EnemyAircraft_Gnat.cpp" \
  "$SRC/EnemyAircraft_Omni.cpp" \
  "$SRC/EnemyAircraft_RayGun.cpp" \
  "$SRC/EnemyAircraft_Straight.cpp" \
  "$SRC/EnemyAircraft_Tank.cpp" \
  "$SRC/EnemyAmmo.cpp" \
  "$SRC/EnemyFleet.cpp" \
  "$SRC/Explosions.cpp" \
  "$SRC/Ground.cpp" \
  "$SRC/GroundMetal.cpp" \
  "$SRC/GroundMetalSegment.cpp" \
  "$SRC/GroundSea.cpp" \
  "$SRC/GroundSeaSegment.cpp" \
  "$SRC/GroundSegment.cpp" \
  "$SRC/HeroAircraft.cpp" \
  "$SRC/HeroAmmo.cpp" \
  "$SRC/HiScore.cpp" \
  "$SRC/MainToolkit.cpp" \
  "$SRC/PowerUps.cpp" \
  "$SRC/ScreenItem.cpp" \
  "$SRC/ScreenItemAdd.cpp" \
  "$SRC/StatusDisplay.cpp" \
  "$SRC/Text.cpp" \
  "$SRC/textGeometryBSU.cpp" \
  "$SRC/textGeometryChromium.cpp" \
  /tmp/Renderer.o \
  -o "$HERE/build/chromium_bsu.wasm"

echo "Built: build/chromium_bsu.wasm"
ls -la "$HERE/build/chromium_bsu.wasm"
