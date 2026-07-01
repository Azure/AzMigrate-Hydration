#!/bin/bash
#
# test-set-max-resync-batch-size.sh
#
# Tests the set-max-resync-batch-size.sh script using a mock environment.
# Runs entirely in a temp directory — no real agent or root required.
#
# Usage:
#   bash test-set-max-resync-batch-size.sh
#

# Do NOT use set -e — we handle failures via counters
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SCRIPT_UNDER_TEST="$SCRIPT_DIR/set-max-resync-batch-size.sh"
PASS=0
FAIL=0

# --- Helpers ---

setup_mock_env() {
    MOCK_ROOT=$(mktemp -d)
    MOCK_CONF_DIR="$MOCK_ROOT/etc"
    MOCK_BIN_DIR="$MOCK_ROOT/bin"
    mkdir -p "$MOCK_CONF_DIR" "$MOCK_BIN_DIR"

    cat > "$MOCK_BIN_DIR/stop" << 'EOF'
#!/bin/bash
echo "MOCK: svagent stopped"
EOF
    cat > "$MOCK_BIN_DIR/start" << 'EOF'
#!/bin/bash
echo "MOCK: svagent started"
EOF
    chmod +x "$MOCK_BIN_DIR/stop" "$MOCK_BIN_DIR/start"

    MOCK_CONF="$MOCK_CONF_DIR/drscout.conf"
}

teardown_mock_env() {
    rm -rf "$MOCK_ROOT"
}

run_script() {
    local batch_size="${1:-}"
    local cmd_args=""
    if [[ -n "$batch_size" ]]; then
        cmd_args="$batch_size"
    fi
    ASR_CONF_FILE="$MOCK_CONF" \
    ASR_AGENT_STOP="$MOCK_BIN_DIR/stop" \
    ASR_AGENT_START="$MOCK_BIN_DIR/start" \
    ASR_SKIP_ROOT_CHECK=1 \
    ASR_VCPU_OVERRIDE="${ASR_VCPU_OVERRIDE:-}" \
    bash "$SCRIPT_UNDER_TEST" $cmd_args 2>&1
}

assert_equals() {
    local test_name="$1"
    local expected="$2"
    local actual="$3"
    if [[ "$expected" == "$actual" ]]; then
        echo "  [PASS] $test_name"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $test_name"
        echo "     Expected: '$expected'"
        echo "     Actual:   '$actual'"
        FAIL=$((FAIL + 1))
    fi
}

assert_contains() {
    local test_name="$1"
    local needle="$2"
    local haystack="$3"
    if echo "$haystack" | grep -q "$needle" 2>/dev/null; then
        echo "  [PASS] $test_name"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $test_name"
        echo "     Expected to contain: '$needle'"
        FAIL=$((FAIL + 1))
    fi
}

# Helper to read the final value of MaxResyncBatchSize from the config
get_key_value() {
    grep -iE "^[[:space:]]*MaxResyncBatchSize[[:space:]]*=" "$MOCK_CONF" 2>/dev/null \
        | tail -1 | sed 's/.*=[[:space:]]*//' || echo ""
}

get_key_count() {
    local count
    count=$(grep -ciE "^[[:space:]]*MaxResyncBatchSize[[:space:]]*=" "$MOCK_CONF" 2>/dev/null) || true
    echo "${count:-0}"
}

# =============================================
# TEST 1: Add key when it doesn't exist
# =============================================
echo ""
echo "TEST 1: Add MaxResyncBatchSize to config (key absent)"
echo "-------------------------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
HostId=abc123
EOF

output=$(run_script 8)
assert_equals "Key added with value 8" "8" "$(get_key_value)"
assert_contains "Output shows 'Added'" "Added" "$output"
assert_contains "Agent stopped" "MOCK: svagent stopped" "$output"
assert_contains "Agent started" "MOCK: svagent started" "$output"

backup_count=$(ls "$MOCK_CONF_DIR"/drscout.conf.bak.* 2>/dev/null | wc -l)
assert_equals "Backup file created" "1" "$(echo "$backup_count" | tr -d ' ')"
teardown_mock_env

# =============================================
# TEST 2: Update existing key
# =============================================
echo ""
echo "TEST 2: Update existing MaxResyncBatchSize (3 → 10)"
echo "----------------------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
MaxResyncBatchSize=3
HostId=abc123
EOF

output=$(run_script 10)
assert_equals "Key updated to 10" "10" "$(get_key_value)"
assert_contains "Output shows 'Updated'" "Updated" "$output"
assert_equals "LogLevel unchanged" "5" "$(grep '^LogLevel=' "$MOCK_CONF" | cut -d= -f2)"
assert_equals "HostId unchanged" "abc123" "$(grep '^HostId=' "$MOCK_CONF" | cut -d= -f2)"
teardown_mock_env

# =============================================
# TEST 3: Default batch size (no argument)
# =============================================
echo ""
echo "TEST 3: Default batch size (no argument → 8)"
echo "----------------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
EOF

output=$(run_script)
assert_equals "Default value is 8" "8" "$(get_key_value)"
teardown_mock_env

# =============================================
# TEST 4: Invalid batch size (too high)
# =============================================
echo ""
echo "TEST 4: Invalid batch size (100 → error)"
echo "------------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
EOF

output=$(run_script 100 || true)
assert_contains "Error message shown" "ERROR" "$output"
assert_equals "Key not added on error" "0" "$(get_key_count)"
teardown_mock_env

# =============================================
# TEST 5: Invalid batch size (non-numeric)
# =============================================
echo ""
echo "TEST 5: Invalid batch size (abc → error)"
echo "------------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
EOF

output=$(run_script "abc" || true)
assert_contains "Error message for non-numeric" "ERROR" "$output"
teardown_mock_env

# =============================================
# TEST 6: Missing [vxagent] section → error + restore
# =============================================
echo ""
echo "TEST 6: Missing [vxagent] section → error + restore"
echo "-----------------------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[othersection]
SomeKey=SomeValue
EOF
original_content=$(cat "$MOCK_CONF")

output=$(run_script 8 || true)
assert_contains "Error about missing section" "ERROR" "$output"
assert_equals "Config restored after error" "$original_content" "$(cat "$MOCK_CONF")"
teardown_mock_env

# =============================================
# TEST 7: Config file not found
# =============================================
echo ""
echo "TEST 7: Config file not found → error"
echo "---------------------------------------"
setup_mock_env
rm -f "$MOCK_CONF"

output=$(run_script 8 || true)
assert_contains "Error about missing file" "ERROR" "$output"
teardown_mock_env

# =============================================
# TEST 8: Multiple sections — only [vxagent] modified
# =============================================
echo ""
echo "TEST 8: Multiple sections — only [vxagent] touched"
echo "----------------------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[transport]
MaxBufferSize=1024
LogLevel=3

[vxagent]
LogLevel=5
MaxResyncBatchSize=3

[cdp]
MaxCdpThreads=4
EOF

output=$(run_script 12)
assert_equals "Key updated to 12" "12" "$(get_key_value)"
assert_equals "[transport] MaxBufferSize unchanged" "1024" "$(grep '^MaxBufferSize=' "$MOCK_CONF" | cut -d= -f2)"
assert_equals "[cdp] MaxCdpThreads unchanged" "4" "$(grep '^MaxCdpThreads=' "$MOCK_CONF" | cut -d= -f2)"
teardown_mock_env

# =============================================
# TEST 9: Idempotent — running twice gives same result
# =============================================
echo ""
echo "TEST 9: Idempotent — running twice gives same result"
echo "------------------------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
EOF

run_script 8 > /dev/null 2>&1
run_script 8 > /dev/null 2>&1
assert_equals "Value is 8" "8" "$(get_key_value)"
assert_equals "Key appears exactly once" "1" "$(get_key_count)"
teardown_mock_env

# =============================================
# TEST 10: Whitespace — "MaxResyncBatchSize = 3"
# =============================================
echo ""
echo "TEST 10: Whitespace around '=' (key = value)"
echo "-----------------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
MaxResyncBatchSize = 3
HostId=abc123
EOF

output=$(run_script 8)
assert_equals "Whitespace key replaced with 8" "8" "$(get_key_value)"
assert_contains "Output shows 'Updated'" "Updated" "$output"
assert_equals "Only one key line" "1" "$(get_key_count)"
teardown_mock_env

# =============================================
# TEST 11: Whitespace — leading spaces + tabs
# =============================================
echo ""
echo "TEST 11: Leading whitespace before key"
echo "----------------------------------------"
setup_mock_env
# Use printf to embed a tab
printf '[vxagent]\nLogLevel=5\n\tMaxResyncBatchSize=3\nHostId=abc123\n' > "$MOCK_CONF"

output=$(run_script 6)
assert_equals "Tab-indented key replaced with 6" "6" "$(get_key_value)"
assert_equals "Only one key line" "1" "$(get_key_count)"
teardown_mock_env

# =============================================
# TEST 12: Realistic drscout.conf (production-like)
# =============================================
echo ""
echo "TEST 12: Realistic drscout.conf (production-like config)"
echo "----------------------------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[application]
DelayBetweenAppShutdownAndTagIssue=60
MaxWaitTimeForTagArrival=300
[appagent]
SupportedApplications=BULK,CLOUD
CompressVolPacks=1
[vxagent.virtualvolumes]
ID=0
[vxagent]
MonitorHostInterval=900
RegisterLayoutOnDisks=0
HostAgentName="svagents"
MaxGenerateClusterBitmapThreads=1
FastSyncReadBufferSize=4194304
DPMaxIOSize=16777216
LogLevel=0
DataProtectionExePathname=/usr/local/ASR/Vx/bin/dataprotection
MaxOutpostThreads=4
[vxagent.transport]
CurlVerbose=0
TcpRecvWindowSize=1048576
TcpSendWindowSize=1048576
[vxagent.filterdriver]
DataPagePoolMemoryUsageInPercentage=6
EOF

output=$(run_script 10)
assert_equals "Key added to realistic config" "10" "$(get_key_value)"
assert_equals "Only one key line" "1" "$(get_key_count)"

# Verify no damage to surrounding sections
assert_equals "MonitorHostInterval intact" "900" "$(grep '^MonitorHostInterval=' "$MOCK_CONF" | cut -d= -f2)"
assert_equals "CurlVerbose intact" "0" "$(grep '^CurlVerbose=' "$MOCK_CONF" | cut -d= -f2)"
assert_equals "[vxagent.virtualvolumes] intact" "0" "$(grep '^ID=' "$MOCK_CONF" | cut -d= -f2)"
assert_equals "DPMaxIOSize intact" "16777216" "$(grep '^DPMaxIOSize=' "$MOCK_CONF" | cut -d= -f2)"
teardown_mock_env

# =============================================
# TEST 13: Batch size boundary values
# =============================================
echo ""
echo "TEST 13: Boundary values (1 and 12)"
echo "--------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
EOF

output=$(run_script 1)
assert_equals "Minimum value 1 accepted" "1" "$(get_key_value)"
teardown_mock_env

setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
EOF

output=$(run_script 12)
assert_equals "Maximum value 12 accepted" "12" "$(get_key_value)"
teardown_mock_env

setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
EOF

output=$(run_script 0 || true)
assert_contains "Value 0 rejected" "ERROR" "$output"
teardown_mock_env

setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
EOF

output=$(run_script 13 || true)
assert_contains "Value 13 rejected" "ERROR" "$output"
teardown_mock_env

# =============================================
# TEST 14: Auto-detect vCPU — 4 vCPUs → batch 3
# =============================================
echo ""
echo "TEST 14: Auto-detect — 4 vCPUs → batch 3"
echo "--------------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
EOF

output=$(ASR_VCPU_OVERRIDE=4 run_script auto)
assert_equals "4 vCPUs → batch 3" "3" "$(get_key_value)"
assert_contains "Shows detected vCPUs" "Detected vCPUs: 4" "$output"
teardown_mock_env

# =============================================
# TEST 15: Auto-detect vCPU — 8 vCPUs → batch 5
# =============================================
echo ""
echo "TEST 15: Auto-detect — 8 vCPUs → batch 5"
echo "--------------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
EOF

output=$(ASR_VCPU_OVERRIDE=8 run_script auto)
assert_equals "8 vCPUs → batch 5" "5" "$(get_key_value)"
teardown_mock_env

# =============================================
# TEST 16: Auto-detect vCPU — 16 vCPUs → batch 8
# =============================================
echo ""
echo "TEST 16: Auto-detect — 16 vCPUs → batch 8"
echo "---------------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
EOF

output=$(ASR_VCPU_OVERRIDE=16 run_script auto)
assert_equals "16 vCPUs → batch 8" "8" "$(get_key_value)"
teardown_mock_env

# =============================================
# TEST 17: Auto-detect vCPU — 32 vCPUs → batch 12 (capped)
# =============================================
echo ""
echo "TEST 17: Auto-detect — 32 vCPUs → batch 12 (capped)"
echo "------------------------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
EOF

output=$(ASR_VCPU_OVERRIDE=32 run_script auto)
assert_equals "32 vCPUs → batch 12 (capped)" "12" "$(get_key_value)"
assert_contains "Shows max cap" "max 12" "$output"
teardown_mock_env

# =============================================
# TEST 18: Auto-detect vCPU — 64 vCPUs → still 12
# =============================================
echo ""
echo "TEST 18: Auto-detect — 64 vCPUs → still 12"
echo "----------------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
EOF

output=$(ASR_VCPU_OVERRIDE=64 run_script auto)
assert_equals "64 vCPUs → still capped at 12" "12" "$(get_key_value)"
teardown_mock_env

# =============================================
# TEST 19: No argument defaults to 8 (not auto)
# =============================================
echo ""
echo "TEST 19: No argument defaults to 8"
echo "--------------------------------------"
setup_mock_env
cat > "$MOCK_CONF" << 'EOF'
[vxagent]
LogLevel=5
EOF

output=$(ASR_VCPU_OVERRIDE=32 run_script)
assert_equals "No arg → batch 8 (not auto)" "8" "$(get_key_value)"
teardown_mock_env

# =============================================
# Summary
# =============================================
echo ""
echo "==========================================="
echo " Results: $PASS passed, $FAIL failed"
echo "==========================================="

if [[ $FAIL -gt 0 ]]; then
    exit 1
fi
