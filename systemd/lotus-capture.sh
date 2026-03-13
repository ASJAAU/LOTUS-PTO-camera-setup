#!/bin/bash

#Retry counter
MAX_RETRIES=3
count=0
retry_timer=30

# Setup log dir
LOGDIR="/home/aau/LOTUS-PTO_studentwork/logs/"
mkdir -p "$LOGDIR"
LOGFILE="$LOGDIR/$(date +%Y-%m-%d).log"

# Start service
echo "Started routine capture service at: $(date '+%Y-%m-%d %H:%M:%S')" >> "$LOGFILE" 2>&1

# Set cwd and python env path
cd /home/aau/LOTUS-PTO_studentwork
PYTHON_BIN="./venv/bin/python3"

# Keep retrying until 
until [ $count -ge $MAX_RETRIES ]; do
    $PYTHON_BIN capture.py usb_cam >> "$LOGFILE" 2>&1 && break
    count=$((count+1))
    echo "Attempt $count/$MAX_RETRIES failed, retrying in 60 seconds..." >> "$LOGFILE" 2>&1
    sleep $retry_timer
done

if [ $count -eq $MAX_RETRIES ]; then
    echo "All $MAX_RETRIES retries failed, exiting with error" >> "$LOGFILE" 2>&1
    exit 1
fi