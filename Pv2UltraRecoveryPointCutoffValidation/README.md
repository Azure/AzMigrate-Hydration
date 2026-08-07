# Pv2UltraRecoveryPointCutoffValidation

A pair of tiny "ticker" scripts that let you **prove exactly where an Azure Site
Recovery recovery point cuts off your data** after a failover — so you can
validate that a recovered VM contains data *only up to* the recovery-point time,
and not any newer, over-shot data.

## Quick Start

Run the ticker on the **source VM**, on the **replicated data disk**, and leave
it running. Then do a test failover to a recovery point and read the last line.

**Windows guest**

```powershell
# Run on the SOURCE VM. Choose a FOLDER on the replicated DATA disk (e.g. F:), not C:.
# The log file defaults to ticker.log inside that folder.
powershell -ExecutionPolicy Bypass -File .\asr-rp-ticker.ps1 -Folder F:\asr-validate

# Choose folder + file name + interval (seconds) explicitly:
powershell -ExecutionPolicy Bypass -File .\asr-rp-ticker.ps1 -Folder F:\asr-validate -FileName ticker.log -IntervalSec 10

# Or pass one full path (overrides -Folder/-FileName):
powershell -ExecutionPolicy Bypass -File .\asr-rp-ticker.ps1 -Path F:\asr-validate\ticker.log
```

**Linux guest**

```bash
chmod +x asr-rp-ticker.sh

# Choose a FOLDER on the replicated DATA disk mount. File defaults to ticker.log.
./asr-rp-ticker.sh -d /mnt/asr-data/asr-validate

# Choose folder + file name + interval explicitly:
./asr-rp-ticker.sh -d /mnt/asr-data/asr-validate -f ticker.log -i 10

# Or pass one full file path as a positional arg (overrides -d/-f):
./asr-rp-ticker.sh /mnt/asr-data/asr-validate/ticker.log 10

# Keep it running after you log out:
nohup ./asr-rp-ticker.sh -d /mnt/asr-data/asr-validate >/dev/null 2>&1 &
```

Each line written is `<seq>,<utc-iso8601-ms>`, e.g. `842,2026-08-07T05:44:10.123Z`.

## Problem

Azure Site Recovery recovery points are **crash-consistent** block-level
snapshots: they have a single, write-order-consistent cut-off across all of a
VM's disks at the recovery-point time. On certain premium disk types
(**PremiumV2_LRS / Ultra / Direct-Drive**), a defect could cause a recovered
disk to contain writes that landed **slightly newer than the selected recovery
point** ("recovery-point over-shoot"). After the fix, the recovered data must
stop **at or before** the recovery-point time.

The challenge for a customer is that ordinary application data gives no precise
marker of "where did my data actually stop?". These scripts provide that marker:
a durable, timestamped heartbeat written to the data disk every few seconds. The
**last durable line that survives a failover is the true data cut-off**, which
you compare against the recovery-point time shown in the portal.

## How It Works

The ticker opens the log file on the replicated data disk and, every `IntervalSec`
seconds, appends one line and **forces it to physical disk** before continuing:

* **Windows** — the file is opened `Append` + `FileOptions.WriteThrough`, and each
  tick calls `FileStream.Flush($true)`, which issues **`FlushFileBuffers`**. The
  line is on the platter (not just in the OS cache) before the next tick.
* **Linux** — each tick appends the line and then calls **`sync <file>`** (falling
  back to a global `sync`) to push the data through to the disk.

Because every line is durable the instant it is written, the file's **last line at
the crash-consistent cut-off is deterministic**: it reflects exactly what had
reached disk at the recovery-point moment. The monotonically increasing `seq`
counter lets you spot the exact cut-off index and detect any gaps.

### Validation flow

1. Start the ticker on the source VM (on the replicated data disk) and let
   replication run long enough to build the recovery point you want to test.
2. **Test-fail over** (recommended — non-disruptive) to a chosen recovery point.
   Note the recovery-point **time** shown in the portal (it is UTC).
3. On the recovered VM, read the **last line** of the ticker file:

   ```powershell
   # Windows (recovered VM)
   Get-Content F:\asr-validate\ticker.log -Tail 1
   ```

   ```bash
   # Linux (recovered VM)
   tail -n 1 /mnt/asr-data/asr-validate/ticker.log
   ```

4. Compare the last timestamp against the recovery-point time.

### How to interpret the result

| Observation | Meaning |
|---|---|
| Last timestamp **≤ recovery-point time** (within ~one tick interval) | **Expected / fixed.** The recovered data cuts off at or before the recovery point. |
| Last timestamp **meaningfully newer** than the recovery-point time | **Over-shoot.** The recovered disk contains data past the recovery point — capture the file and open a support case. |

> **Note on "the database looks like it has newer rows":** a *running* service
> (e.g. a DB) will **auto-start on the failover boot and write brand-new rows**.
> Those post-boot writes are NOT recovered over-shoot. This ticker avoids that
> confusion by design — it does not auto-restart on the recovered VM, so its last
> line is purely the recovered cut-off. If you use your own app's data instead,
> split rows at the VM boot time before judging over-shoot.

## Configuration Details

| Setting | Windows | Linux | Default |
|---|---|---|:-:|
| Log folder | `-Folder` | `-d` | `F:\asr-validate` / `/mnt/asr-data/asr-validate` |
| Log file name | `-FileName` | `-f` | `ticker.log` |
| Full log path (overrides folder+name) | `-Path` | positional arg 1 | *(built from folder + file name)* |
| Write interval (seconds) | `-IntervalSec` | `-i` / positional arg 2 | `10` |

* The log folder **must be on the replicated data disk** — not the OS disk, and not
  the Azure ephemeral/resource disk (`D:` on Windows, usually `/mnt` on Linux is
  the resource disk, so pick a real data-disk mount).
* A 10-second interval keeps the file tiny (≈ 3 KB/hour) while bounding the
  cut-off uncertainty to at most one interval. Lower it for a tighter marker.
* The scripts run indefinitely until you stop them (Ctrl+C, or `kill` the
  backgrounded Linux process). They flush and close cleanly on stop.

## Testing

Both scripts are self-contained writers with no dependencies beyond the OS shell.
To smoke-test locally before deploying to a guest:

```powershell
# Windows: write to a temp folder for a bit, then Ctrl+C, and inspect
powershell -ExecutionPolicy Bypass -File .\asr-rp-ticker.ps1 -Folder $env:TEMP\asr-validate -IntervalSec 2
Get-Content $env:TEMP\asr-validate\ticker.log -Tail 5
```

```bash
# Linux: write to /tmp for a bit, then Ctrl+C, and inspect
./asr-rp-ticker.sh -d /tmp/asr-validate -i 2
tail -n 5 /tmp/asr-validate/ticker.log
```

You should see continuous `<seq>,<utc>` lines with no gaps and a monotonic `seq`.

## Files

| File | Runs on | Purpose |
|---|---|---|
| `asr-rp-ticker.ps1` | Windows source VM | Durable 10s timestamp ticker (WriteThrough + FlushFileBuffers). |
| `asr-rp-ticker.sh` | Linux source VM | Durable 10s timestamp ticker (per-line `sync`). |

---

These scripts are provided **as-is**, without warranty of any kind. See the
repository `LICENSE.txt`.
