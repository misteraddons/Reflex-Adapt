#!/usr/bin/env bash

TIMESTAMP=$(date +%s)
HASH=$(md5sum "_bin/reflex_updater.sh" | cut -d' ' -f1)
SIZE=$(stat -c%s "_bin/reflex_updater.sh")

cat > "_bin/reflexadapt.json" << EOF
{
    "db_id": "misteraddons/reflexadapt",
    "timestamp": ${TIMESTAMP},
    "files": {
        "Scripts/reflex_updater.sh": {
            "hash": "${HASH}",
            "size": ${SIZE},
            "url": "https://github.com/misteraddons/Reflex-Adapt/releases/latest/download/reflex_updater.sh"
        }
    },
    "folders": {
        "Scripts": {}
    }
}
EOF
