#!/bin/bash
cd /home/aau/LOTUS-PTO_studentwork/

LOGDIR="./logs"
mkdir -p "$LOGDIR"
LOGFILE="$LOGDIR/$(date +%Y-%m-%d).log"

source venv/bin/activate

./venv/bin/python3 snap_pic.py >> "$LOGFILE" 2>&1