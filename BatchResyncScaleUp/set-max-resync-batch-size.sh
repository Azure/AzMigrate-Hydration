#!/bin/bash
#
# set-max-resync-batch-size.sh
#
# Updates MaxResyncBatchSize in drscout.conf on a Linux source VM
# to increase concurrent disk replication during Initial Replication.
#
# Usage:
#   sudo bash set-max-resync-batch-size.sh [batch_size | auto]
#
# Examples:
#   sudo bash set-max-resync-batch-size.sh          # defaults to 8
#   sudo bash set-max-resync-batch-size.sh 10        # explicit value
#   sudo bash set-max-resync-batch-size.sh auto      # auto-detect from vCPUs
#
# Auto-detection formula (capped at 12):
#   vCPUs 1-4   → batch size 3  (default, no change needed)
#   vCPUs 5-8   → batch size 5
#   vCPUs 9-16  → batch size 8
#   vCPUs 17+   → batch size 12
#
# Environment overrides (for testing):
#   ASR_CONF_FILE       path to drscout.conf
#   ASR_AGENT_STOP      path to stop script
#   ASR_AGENT_START     path to start script
#   ASR_SKIP_ROOT_CHECK set to "1" to skip root check
#   ASR_VCPU_OVERRIDE   override vCPU count (for testing)
#

set -euo pipefail

MAX_BATCH=12

CONF_FILE="${ASR_CONF_FILE:-/usr/local/ASR/Vx/etc/drscout.conf}"
AGENT_STOP="${ASR_AGENT_STOP:-/usr/local/ASR/Vx/bin/stop}"
AGENT_START="${ASR_AGENT_START:-/usr/local/ASR/Vx/bin/start}"
SECTION="vxagent"
KEY="MaxResyncBatchSize"

KEY_REGEX="^[[:space:]]*${KEY}[[:space:]]*="

# --- Detect vCPU count ---

detect_vcpus() {
    if [[ -n "${ASR_VCPU_OVERRIDE:-}" ]]; then
        echo "$ASR_VCPU_OVERRIDE"
        return
    fi

    local vcpus=0
    if command -v nproc &>/dev/null; then
        vcpus=$(nproc)
    elif [[ -f /proc/cpuinfo ]]; then
        vcpus=$(grep -c '^processor' /proc/cpuinfo)
    elif command -v sysctl &>/dev/null; then
        vcpus=$(sysctl -n hw.ncpu 2>/dev/null || echo 0)
    fi

    if [[ "$vcpus" -lt 1 ]]; then
        vcpus=1
    fi
    echo "$vcpus"
}

# --- Calculate batch size from vCPU count ---

calculate_batch_size() {
    local vcpus="$1"
    local batch

    if [[ "$vcpus" -le 4 ]]; then
        batch=3
    elif [[ "$vcpus" -le 8 ]]; then
        batch=5
    elif [[ "$vcpus" -le 16 ]]; then
        batch=8
    else
        batch=$MAX_BATCH
    fi

    echo "$batch"
}

# --- Resolve batch size ---

INPUT="${1:-8}"

if [[ "$INPUT" == "auto" ]]; then
    VCPUS=$(detect_vcpus)
    BATCH_SIZE=$(calculate_batch_size "$VCPUS")
    echo "Detected vCPUs: $VCPUS"
    echo "Recommended MaxResyncBatchSize: $BATCH_SIZE (max $MAX_BATCH)"
    echo ""
else
    BATCH_SIZE="$INPUT"
fi

# --- Validations ---

if [[ "${ASR_SKIP_ROOT_CHECK:-}" != "1" ]] && [[ $EUID -ne 0 ]]; then
    echo "ERROR: This script must be run as root (use sudo)."
    exit 1
fi

if ! [[ "$BATCH_SIZE" =~ ^[0-9]+$ ]] || [[ "$BATCH_SIZE" -lt 1 ]] || [[ "$BATCH_SIZE" -gt $MAX_BATCH ]]; then
    echo "ERROR: Batch size must be a number between 1 and $MAX_BATCH. Got: $BATCH_SIZE"
    exit 1
fi

if [[ ! -f "$CONF_FILE" ]]; then
    echo "ERROR: Config file not found: $CONF_FILE"
    echo "       Is the Mobility Agent installed on this machine?"
    exit 1
fi

# --- Read current value ---

CURRENT_VALUE=$(grep -iE "$KEY_REGEX" "$CONF_FILE" 2>/dev/null | tail -1 | sed 's/.*=\s*//' || echo "3 (default)")
echo "Current $KEY = $CURRENT_VALUE"
echo "New     $KEY = $BATCH_SIZE"
echo ""

# --- Stop svagent ---

echo "Stopping svagent service..."
if [[ -x "$AGENT_STOP" ]]; then
    "$AGENT_STOP"
    sleep 3
    echo "svagent stopped."
else
    echo "ERROR: Stop script not found or not executable: $AGENT_STOP"
    exit 1
fi

# --- Backup config ---

BACKUP="${CONF_FILE}.bak.$(date +%Y%m%d%H%M%S)"
cp "$CONF_FILE" "$BACKUP"
echo "Config backed up to: $BACKUP"

# --- Update drscout.conf ---

if grep -qiE "$KEY_REGEX" "$CONF_FILE"; then
    # Replace existing line (handles whitespace variations)
    sed -i -E "s/${KEY_REGEX}.*/${KEY}=${BATCH_SIZE}/i" "$CONF_FILE"
    echo "Updated existing $KEY to $BATCH_SIZE."
else
    # Add under [vxagent] section
    if grep -q "^\[${SECTION}\]" "$CONF_FILE"; then
        sed -i "/^\[${SECTION}\]/a ${KEY}=${BATCH_SIZE}" "$CONF_FILE"
        echo "Added $KEY=$BATCH_SIZE under [$SECTION]."
    else
        echo "ERROR: [$SECTION] section not found in $CONF_FILE"
        echo "       Restoring backup..."
        cp "$BACKUP" "$CONF_FILE"
        "$AGENT_START"
        exit 1
    fi
fi

# --- Verify ---

echo ""
echo "Verifying config:"
grep -iE "$KEY_REGEX" "$CONF_FILE"
echo ""

# --- Start svagent ---

echo "Starting svagent service..."
if [[ -x "$AGENT_START" ]]; then
    "$AGENT_START"
    sleep 3
    echo "svagent started."
else
    echo "ERROR: Start script not found or not executable: $AGENT_START"
    exit 1
fi

echo ""
echo "Done. $KEY set to $BATCH_SIZE."
echo "The agent will now replicate up to $BATCH_SIZE disks concurrently."
