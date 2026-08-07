#!/usr/bin/env bash
# asr-rp-ticker.sh  -  ASR recovery-point cut-off validator (Linux guest)
#
# Writes one line every N seconds to a file on the REPLICATED DATA DISK and
# fsyncs each line to physical disk so the LAST line in the file is
# guaranteed-durable at the crash-consistent cut-off.
#
# After you fail over (or test-fail over) to a chosen recovery point, open this
# file on the recovered VM and look at the LAST line: that timestamp is the
# exact point the data was cut off. With the fix in place it must be <= the
# recovery-point time. If it is meaningfully NEWER than the RP time, the RP
# over-shot (recovered more data than the point represents).
#
# Line format (CSV, no header):  <seq>,<utc-iso8601-ms>
#   seq = monotonic counter ; utc = e.g. 2026-08-07T05:44:10.123Z (UTC)
#
# USAGE (run on the SOURCE VM):
#   chmod +x asr-rp-ticker.sh
#   # Pick a folder on the replicated DATA disk; file defaults to ticker.log:
#   ./asr-rp-ticker.sh -d /mnt/asr-data/asr-validate
#   # Or choose folder + file name + interval explicitly:
#   ./asr-rp-ticker.sh -d /mnt/asr-data/asr-validate -f ticker.log -i 10
#   # Or give one full file path as a positional arg (overrides -d/-f):
#   ./asr-rp-ticker.sh /mnt/asr-data/asr-validate/ticker.log 10
#   # The folder MUST be on the replicated DATA disk mount, NOT the OS disk or the
#   # ephemeral resource disk (usually /mnt is the resource disk -> pick a real
#   # data-disk mount).
#
# For an unattended run that survives your SSH session, use nohup:
#   nohup ./asr-rp-ticker.sh -d /mnt/asr-data/asr-validate >/dev/null 2>&1 &
#
# This script is provided as-is with no warranty (see repo LICENSE).
set -u

DIR='/mnt/asr-data/asr-validate'
NAME='ticker.log'
INTERVAL='10'

while getopts ':d:f:i:h' opt; do
    case "$opt" in
        d) DIR="$OPTARG" ;;
        f) NAME="$OPTARG" ;;
        i) INTERVAL="$OPTARG" ;;
        h) grep '^#' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        :) echo "Option -$OPTARG requires an argument." >&2; exit 1 ;;
        \?) echo "Unknown option -$OPTARG." >&2; exit 1 ;;
    esac
done
shift $((OPTIND - 1))

# Positional back-compat:  arg1 = full file path, arg2 = interval.
if [ "${1:-}" != "" ]; then FILE="$1"; else FILE="$DIR/$NAME"; fi
if [ "${2:-}" != "" ]; then INTERVAL="$2"; fi

case "$INTERVAL" in
    ''|*[!0-9]*) echo "Interval must be a positive integer (seconds)." >&2; exit 1 ;;
esac

mkdir -p "$(dirname "$FILE")"
seq=0
trap 'echo; echo "Ticker stopped. Last seq = $seq. File: $FILE"; exit 0' INT TERM

echo "Ticker writing to $FILE every ${INTERVAL}s. Press Ctrl+C to stop."
while true; do
    seq=$((seq + 1))
    # GNU date: %3N = milliseconds. Falls back if %N is unsupported.
    utc="$(date -u +%Y-%m-%dT%H:%M:%S.%3NZ 2>/dev/null || date -u +%Y-%m-%dT%H:%M:%SZ)"
    printf '%s,%s\n' "$seq" "$utc" >> "$FILE"
    # Force this file's data to physical disk. Prefer per-file sync; fall back to global.
    sync "$FILE" 2>/dev/null || sync
    sleep "$INTERVAL"
done
