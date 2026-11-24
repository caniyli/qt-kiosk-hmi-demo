#!/bin/sh
# Regenerates docs/demo.gif without a display. --record plays a short scripted
# demo (motor overload trip, acknowledgement, switch to Turkish, restart) on
# Qt's "offscreen" platform and saves it as PNG frames at 10 fps; ffmpeg turns
# them into a GIF with a single 256-colour palette.
#
# Usage: scripts/make-demo-gif.sh [path/to/hmi-demo]   (needs ffmpeg)
set -eu

cd "$(dirname "$0")/.."
app=${1:-build/hmi-demo}
frames=$(mktemp -d)
trap 'rm -rf "$frames"' EXIT
export QT_QPA_PLATFORM=offscreen

"$app" --lang en --size 960x600 --record "$frames"
# -framerate must match the rate of --record (10 fps).
ffmpeg -v error -y -framerate 10 -i "$frames/frame-%04d.png" \
    -filter_complex "[0:v]split[a][b];[a]palettegen[p];[b][p]paletteuse=dither=none:diff_mode=rectangle" \
    docs/demo.gif
echo "GIF written to docs/demo.gif"
