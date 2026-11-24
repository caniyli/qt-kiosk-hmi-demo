#!/bin/sh
# Regenerates docs/screenshot-*.png without a display, using Qt's "offscreen"
# platform. --screenshot replays a fixed, seeded demo scenario first, so the
# pictures show trend data and alarm history.
#
# Usage: scripts/take-screenshots.sh [path/to/hmi-demo]
set -eu

cd "$(dirname "$0")/.."
app=${1:-build/hmi-demo}
export QT_QPA_PLATFORM=offscreen

"$app" --page overview --lang en --screenshot docs/screenshot-overview.png
"$app" --page alarms --lang en --screenshot docs/screenshot-alarms.png
"$app" --page settings --lang tr --screenshot docs/screenshot-settings-tr.png
