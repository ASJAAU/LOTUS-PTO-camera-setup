#!/bin/bash

# Setup log dir
LOGDIR="./logs"
mkdir -p "$LOGDIR"
LOGFILE="$LOGDIR/$(date +%Y-%m-%d).log"

echo "ROUTINE: Started routine capture service at: $(date '+%Y-%m-%d-%H-%M-%S')"

# Setup working environment
cd /home/aau/LOTUS-PTO_studentwork/
source venv/bin/activate

# Run commands
./venv/bin/python3 snap_pic.py >> "$LOGFILE" 2>&1