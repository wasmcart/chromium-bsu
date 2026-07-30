#!/bin/bash
# Pack chromium_bsu.wasc from compiled .wasm + game assets
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
WASM="$HERE/build/chromium_bsu.wasm"
# Staging dir for the assets the packer reads. Kept inside build/ rather than
# /tmp: /tmp wipes on reboot, and build/ is already gitignored.
ASSETS_DIR="$HERE/build/assets"

if [ ! -f "$WASM" ]; then
    echo "ERROR: build/chromium_bsu.wasm not found. Run build.sh first."
    exit 1
fi

# Gather assets
rm -rf "$ASSETS_DIR"
mkdir -p "$ASSETS_DIR/png"
mkdir -p "$ASSETS_DIR/wav"

cp "$HERE/data/png"/*.png "$ASSETS_DIR/png/"
cp "$HERE/data/wav"/*.wav "$ASSETS_DIR/wav/"

echo "Assets gathered:"
find "$ASSETS_DIR" -type f | wc -l
echo " files"

# Pack using wasmcart-pack if available, otherwise manual zip
if command -v wasmcart-pack &> /dev/null; then
    wasmcart-pack \
        --wasm "$WASM" \
        --assets "$ASSETS_DIR" \
        --name "Chromium B.S.U." \
        -o "$HERE/build/chromium_bsu.wasc"
else
    # Manual pack: create zip with manifest + wasm + assets
    PACK_DIR="/tmp/chromium_bsu_pack"
    rm -rf "$PACK_DIR"
    mkdir -p "$PACK_DIR/assets"
    cp "$WASM" "$PACK_DIR/cart.wasm"
    cp -r "$ASSETS_DIR"/* "$PACK_DIR/assets/"
    cat > "$PACK_DIR/manifest.json" << 'EOF'
{
  "name": "Chromium B.S.U.",
  "description": "Fast-paced scrolling space shooter",
  "author": "Mark B. Allan",
  "version": "0.9.16.1-wasmcart",
  "gpu_api": 1,
  "players": 1
}
EOF
    cd "$PACK_DIR"
    zip -rq "$HERE/build/chromium_bsu.wasc" manifest.json cart.wasm assets/
fi

echo "Done!"
ls -lh "$HERE/build/chromium_bsu.wasc"
