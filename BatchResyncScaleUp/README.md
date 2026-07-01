# BatchResyncScaleUp

## Quick Start

```bash
# Copy the script to the source VM, then:

# Option 1: Set batch size to 8 (default)
sudo bash set-max-resync-batch-size.sh

# Option 2: Set a specific batch size
sudo bash set-max-resync-batch-size.sh 10

# Option 3: Auto-detect optimal value based on VM vCPU count (capped at 12)
sudo bash set-max-resync-batch-size.sh auto
```

## Problem

When protecting VMware VMs with a large number of disks using Azure Site Recovery (V2A with RCM Appliance), initial replication can take significantly longer than expected. This is because the Mobility Agent on the source VM limits concurrent disk replication to **3 disks at a time** by default (`MaxResyncBatchSize=3` in `drscout.conf`).

For VMs with many disks, only 3 replicate concurrently — the remaining disks queue and wait. Each disk takes several hours depending on size, so total IR time scales linearly with disk count.

Increasing the batch size allows more disks to replicate in parallel, significantly reducing overall IR duration.

## How It Works

The `set-max-resync-batch-size.sh` script safely updates the `MaxResyncBatchSize` setting in the Mobility Agent configuration (`drscout.conf`) on the source VM.

### What the script does

1. **Validates** input (batch size must be 1–12, or `auto`)
2. **Stops** the svagent service (`/usr/local/ASR/Vx/bin/stop`)
3. **Backs up** `drscout.conf` with a timestamped copy
4. **Adds or replaces** `MaxResyncBatchSize` under the `[vxagent]` section
   - Handles whitespace variations (`key=value`, `key = value`, tab-indented, etc.)
5. **Verifies** the config change was applied
6. **Restarts** the svagent service (`/usr/local/ASR/Vx/bin/start`)

If anything goes wrong (missing section, invalid input), the script restores the backup automatically.

### Backup

Before making any changes, the script creates a timestamped copy of `drscout.conf` in the same directory:

```
/usr/local/ASR/Vx/etc/drscout.conf.bak.20260702001500
```

The backup is a full copy of the original file. If the config edit or service restart fails, the script automatically restores from this backup. You can also restore manually at any time (see [Rollback](#rollback)).

### Auto-detect mode

When called with `auto`, the script reads the VM's vCPU count and selects an appropriate batch size:

| Source VM vCPUs | Batch Size | Rationale |
|:-:|:-:|---|
| 1–4 | 3 | Default — no change needed |
| 5–8 | 5 | Moderate parallelism |
| 9–16 | 8 | Good balance of throughput and I/O headroom |
| 17+ | 12 (max) | High parallelism, suitable for large VMs |

Detection uses `nproc`, `/proc/cpuinfo`, or `sysctl` (in that order).

## Configuration Details

| Setting | Location | Default |
|---------|----------|:-------:|
| `MaxResyncBatchSize` | `/usr/local/ASR/Vx/etc/drscout.conf` `[vxagent]` section | **3** |

The setting controls how many `DataProtectionSyncRcm` processes run concurrently on the source VM during initial replication. The agent reads this value once at startup (`static const`), so a **service restart is required** for changes to take effect.

> **Important:** Increasing this value increases disk I/O and network load on the source VM during initial replication. Monitor source VM performance (disk queue depth, CPU, network) after making this change. If the source VM becomes I/O saturated, reduce the batch size.

## Rollback

If needed, restore the original configuration:

```bash
# The script creates a timestamped backup, e.g.:
# /usr/local/ASR/Vx/etc/drscout.conf.bak.20260701153000

sudo /usr/local/ASR/Vx/bin/stop
sudo cp /usr/local/ASR/Vx/etc/drscout.conf.bak.<timestamp> /usr/local/ASR/Vx/etc/drscout.conf
sudo /usr/local/ASR/Vx/bin/start
```

## Testing

A comprehensive test suite is included that validates all scenarios using a mock environment (no real agent or root access required):

```bash
bash test-set-max-resync-batch-size.sh
```

### Test coverage (44 tests)

| Category | Tests |
|----------|-------|
| Add key (absent) | Adds under `[vxagent]`, verifies backup, agent stop/start |
| Update key (present) | Replaces existing value, preserves other keys |
| Default value | No argument → batch size 8 |
| Input validation | Rejects 0, negative, >12, non-numeric, missing config |
| Error recovery | Missing `[vxagent]` section → restores backup |
| Whitespace handling | `key = value`, tab-indented, spaces around `=` |
| Multi-section config | Only modifies `[vxagent]`, leaves `[transport]`, `[cdp]` etc. intact |
| Idempotency | Running twice produces identical result, key appears once |
| Realistic config | Full production-like `drscout.conf` with all standard sections |
| Boundary values | Accepts 1 and 12, rejects 0 and 13 |
| Auto-detect | 4/8/16/32/64 vCPU scenarios with correct batch sizing |

## Files

| File | Description |
|------|-------------|
| `set-max-resync-batch-size.sh` | Main script — run on source VMs |
| `test-set-max-resync-batch-size.sh` | Test suite — run on any Linux/Git Bash machine |
| `README.md` | This file |
