#!/bin/bash

#Retry counter
MAX_RETRIES=3
count=0
retry_timer=30

# Setup log dir
LOGDIR="/home/aau/LOTUS-PTO_studentwork/logs/"
mkdir -p "$LOGDIR"
LOGFILE="$LOGDIR/$(date +%Y-%m-%d).log"

# Setup Logging function (to collate with python)
log_system() {
    echo "[SYSTEM] $*"
}

# Apply logging to all stdouts (Bash and Python)
exec > >(awk '{ print strftime("%Y-%m-%d %H:%M:%S"), $0; fflush(); }' >> "$LOGFILE") 2>&1

# Start logging service
log_system "Started routine capture service at: $(date '+%Y-%m-%d %H:%M:%S')"

# Set cwd and python env path
cd /home/aau/LOTUS-PTO_studentwork
PYTHON_BIN="./venv/bin/python3"

# Keep retrying until
until [ $count -ge $MAX_RETRIES ]; do
    $PYTHON_BIN capture.py usb_cam && break
    count=$((count+1))
    log_system "Attempt $count/$MAX_RETRIES failed, retrying in 60 seconds..."
    sleep $retry_timer
done

if [ $count -eq $MAX_RETRIES ]; then
    log_system "All $MAX_RETRIES retries failed, exiting with error"
    exit 1
fi
