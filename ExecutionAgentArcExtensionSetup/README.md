# Set up the Azure Migrate Execution Agent on an Azure Arc-enabled Linux server

This guide shows you how to install the **Azure Migrate Execution Agent** on a Linux server by
running the setup script `preflight-arc-setup.sh`. You run the script once, on the Linux server
itself. It prepares everything in Azure, installs the agent as an Azure Arc extension, and
confirms that the agent is healthy before it finishes.

**Time required:** about 10–15 minutes.

## In this guide

1. [What the script does](#what-the-script-does)
2. [Before you begin](#before-you-begin)
3. [Step 1 — Get your Azure Migrate project resource ID](#step-1--get-your-azure-migrate-project-resource-id)
4. [Step 2 — Copy the script to the Linux server](#step-2--copy-the-script-to-the-linux-server)
5. [Step 3 — Sign in to Azure](#step-3--sign-in-to-azure)
6. [Step 4 — Preview the changes (dry run)](#step-4--preview-the-changes-dry-run)
7. [Step 5 — Run the setup](#step-5--run-the-setup)
8. [Step 6 — Verify the installation](#step-6--verify-the-installation)
9. [Step 7 — Add credentials for your target machines](#step-7--add-credentials-for-your-target-machines)
10. [Run the script again, reinstall, or remove the agent](#run-the-script-again-reinstall-or-remove-the-agent)
11. [Script options](#script-options)
12. [Troubleshooting](#troubleshooting)
13. [Collect logs for support](#collect-logs-for-support)

---

## What the script does

The script works through six stages and prints a heading for each one:

| Stage | What happens |
|---|---|
| 1. Prerequisites and Azure Arc | Checks for the required tools and offers to install any that are missing. Confirms your Azure sign-in, confirms that discovery has run on your project, and connects the server to Azure Arc if it isn't connected already. |
| 2. Key Vault | Uses the Key Vault linked to your Azure Migrate project. If there isn't one, it creates a Key Vault in the project's resource group. |
| 3. Encryption key and access | Creates a new RSA key in the Key Vault. Gives the server's Arc identity permission to use that key, and nothing else. |
| 4. Server identity | Reads the Azure Arc resource ID of this server. |
| 5. Registration | Registers this server as an execution agent in your Azure Migrate project. |
| 6. Agent installation | Installs the Execution Agent extension, then waits until the agent is running and can reach Azure Migrate. |

### What the script changes, and what it doesn't

- **Access is kept to a minimum.** The server's Arc identity gets exactly one permission set: `get`,
  `wrapKey` and `unwrapKey` on keys, in one Key Vault. It can't create, delete or purge keys. It
  gets no roles on your resource groups, your subscription or your Azure Migrate project.
- **Every Azure change is made with your own sign-in.** Nothing runs under a hidden or
  shared account.
- **Your credentials stay private.** Passwords you store later are encrypted on the server with
  the Key Vault key. They are never written in clear text and never shown on screen.
- **It's safe to run again.** If the agent is already installed and healthy, a second run reports
  that and changes nothing.

> **Tip:** To see the full security reasoning behind each change, run
> `bash ./preflight-arc-setup.sh --explain`.

---

## Before you begin

Confirm each item before you start.

### Server requirements

| Requirement | Details |
|---|---|
| Operating system | Ubuntu, Debian, Red Hat Enterprise Linux (or a compatible distribution) or Azure Linux 3, on 64-bit x86 hardware. |
| Administrator access | An account that can run `sudo`. |
| Azure Arc | Optional. If the server isn't connected to Azure Arc yet, the script connects it for you. |
| Outbound internet access (HTTPS, port 443) | The server must reach Azure Resource Manager, Microsoft Entra ID, Azure Key Vault and Azure Migrate. If the script needs to install tools, it also needs `packages.microsoft.com` and `aka.ms`. For the Azure Arc endpoints, see [Azure Arc network requirements](https://learn.microsoft.com/azure/azure-arc/servers/network-requirements). |

### Azure requirements

| Requirement | Details |
|---|---|
| An Azure Migrate project | You need its resource ID. [Step 1](#step-1--get-your-azure-migrate-project-resource-id) shows where to find it. |
| Discovery has run | Discovery must have run in the project at least once and finished. The script stops if it hasn't, before it creates or changes anything. |
| Permissions for your account | See the table below. |

### Permissions your Azure account needs

The script makes every Azure change with **your** sign-in. Your account needs these roles. The
simplest option is **Contributor** on the resource groups listed.

| Task | Role required | Scope |
|---|---|---|
| Read the Azure Migrate project and register the agent | **Azure Migrate Owner** or **Contributor** | The Azure Migrate project's resource group |
| Connect the server to Azure Arc (only if it isn't connected yet) | **Azure Connected Machine Onboarding** or **Contributor** | The resource group the server is onboarded into |
| Install the agent extension | **Azure Connected Machine Resource Administrator** or **Contributor** | The Arc server's resource group |
| Create a Key Vault (only if the project doesn't have one) | **Contributor** | The Key Vault's resource group (the project's resource group by default) |
| Use an existing Key Vault that uses **Azure RBAC** | **Owner** or **User Access Administrator** | That Key Vault |
| Register required resource providers (only if not registered yet) | **Owner** or **Contributor** | The subscription |

> **Note:** The **Azure Migrate Execute Expert** role is read-only, so it isn't enough to run setup.

> **Note:** If you use an existing Key Vault that uses **access policies**, the script doesn't edit
> that vault's policies. If a permission is missing, it stops and prints the exact command that the
> vault's owner needs to run.

---

## Step 1 — Get your Azure Migrate project resource ID

The project resource ID is the only value you must provide. The script reads the subscription,
resource group and region from it.

**In the Azure portal:** open your Azure Migrate project, select **Properties**, and copy the
**Resource ID**.

**Or with the Azure CLI:**

```bash
az resource list --resource-type Microsoft.Migrate/migrateProjects --query "[].id" -o tsv
```

The ID looks like this:

```text
/subscriptions/<subscription-id>/resourceGroups/<resource-group>/providers/Microsoft.Migrate/migrateProjects/<project-name>
```

To make the commands in this guide easier to use, save the ID in a shell variable on the server:

```bash
PROJECT_ID="/subscriptions/<subscription-id>/resourceGroups/<resource-group>/providers/Microsoft.Migrate/migrateProjects/<project-name>"
```

> **Tip:** Keep the ID in double quotes. Check that it starts with `/subscriptions/` and has no
> spaces or line breaks.

---

## Step 2 — Copy the script to the Linux server

Sign in to the Linux server (by SSH or the console) and download the script to your home folder:

```bash
cd ~
curl -fsSL -o preflight-arc-setup.sh \
  https://raw.githubusercontent.com/Azure/AzMigrate-Hydration/asr-am-support-scripts/ExecutionAgentArcExtensionSetup/preflight-arc-setup.sh
```

> **Important:** Save the script to a file, as shown above. Don't pipe it straight into `bash`
> (for example `curl … | sudo bash`). The script asks you questions, and a piped script can't
> read your answers.

**If the server can't reach GitHub**, download the script on your workstation and copy it to the
server instead:

```bash
scp preflight-arc-setup.sh <user>@<server-address>:~/
```

**If you copied the file from a Windows computer**, convert it to Linux line endings. Windows line
endings stop the script from running (see [Troubleshooting](#troubleshooting)):

```bash
sed -i 's/\r$//' preflight-arc-setup.sh
```

Confirm that the script runs and lists its options:

```bash
bash ./preflight-arc-setup.sh --help
```

---

## Step 3 — Sign in to Azure

Sign in with **your own user account, without `sudo`**:

```bash
az login --use-device-code
```

Open the link that's shown, enter the code, and sign in. If your account belongs to more than one
tenant, add `--tenant <tenant-id>`.

> **Important:** Don't run `sudo az login`. That signs in the `root` account instead of you, and
> the script won't find your sign-in.

### If the Azure CLI isn't installed yet

On a new server, the Azure CLI might not be installed, so `az login` fails. In that case, skip
the sign-in for now and run the script. It needs **two runs** on a new server:

1. **First run:** the script lists the missing tools and asks once before installing them. Answer
   `y`. It then stops at the sign-in check, because only you can sign in:

   ```text
           the tools are installed; sign in to Azure to continue.
     FAIL  not signed in to Azure
           Cause: az has no sign-in for azureuser (looked in /home/azureuser/.azure). This is expected the first time this script runs on a machine.
           Fix:
             1. as azureuser, WITHOUT sudo, sign in:  az login --use-device-code
             2. run this script again with 'sudo -E':  sudo -E bash ./preflight-arc-setup.sh --migrate-project-id …
   ```

2. **Sign in, then run the script again.** Follow the two `Fix:` steps exactly as shown.

The script never installs anything without asking. The tools it can install are:

- the Azure CLI (`az`);
- the Azure CLI `connectedmachine` extension;
- the Azure Arc agent (`azcmagent`).

Installer output goes to `/var/log/azure-migrate/preflight-install.log`.

---

## Step 4 — Preview the changes (dry run)

Run the script with `--dry-run` first. A dry run doesn't create, change or install anything. It
prints every change that the real run would make, so you can review it before you commit.

```bash
sudo -E bash ./preflight-arc-setup.sh --dry-run --migrate-project-id "$PROJECT_ID"
```

> **Why `sudo -E`?** The script needs administrator rights on the server. The `-E` keeps your
> environment so the script can use the Azure sign-in from Step 3. Plain `sudo` hides your
> sign-in, and the script stops (see [Troubleshooting](#troubleshooting)).

### How to read the output

Each line starts with a label:

| Label | Meaning |
|---|---|
| `PASS` | A check succeeded. |
| `DO` | The script is making a change (or would make it, in a dry run). |
| `DRY` | Dry run only: the command that would run. Nothing was changed. |
| `SKIP` | Already in the right state, so nothing to do. |
| `WARN` | Something to review. The script continues. |
| `FAIL` | The script stopped. The lines underneath explain why. |

Every `FAIL` line is followed by a **`Cause:`** line, which explains why it happened, and a
**`Fix:`** line, which gives the exact command to run next. Usually you can copy the `Fix:`
command as it is.

---

## Step 5 — Run the setup

If the dry run looks right, run the same command without `--dry-run`:

```bash
sudo -E bash ./preflight-arc-setup.sh --migrate-project-id "$PROJECT_ID"
```

The script might ask you to confirm some actions. Answer `y` to continue or `n` to stop. If the
server isn't connected to Azure Arc yet, the Arc agent might also ask you to sign in with a device
code while it connects. Follow the link and code it shows.

In the final stage it installs the agent extension and waits for it to become healthy. This usually takes
**3–5 minutes**, and the script waits for up to 10 minutes while it shows progress:

```text
        installing the extension usually takes 3-5 minutes (limit 10 minutes); progress is shown below. Please wait.
        waiting: Azure has not created the extension resource yet
        waiting: extension provisioningState=Creating
```

A successful run ends like this:

```text
  PASS  extension provisioningState=Succeeded (after 214s)
  PASS  azmigrate-execution.service is active with registration 'server-01-a1b2c3d4-executionAgent'
  PASS  job polling answered HTTP 200 using this machine's Arc identity

=== Summary ===
  27 passed, 0 failed, 0 warning(s)
```

The numbers you see will differ. What matters is **`0 failed`**. The script exits with code `0`
on success and a non-zero code on any failure.

### Common situations

| Situation | Command |
|---|---|
| The server isn't connected to Azure Arc yet, and it should be onboarded into a different subscription or resource group than the Azure Migrate project's | `sudo -E bash ./preflight-arc-setup.sh --migrate-project-id "$PROJECT_ID" --arc-subscription <subscription-id> --arc-resource-group <resource-group>` |
| You want to use a Key Vault that you already own | `sudo -E bash ./preflight-arc-setup.sh --migrate-project-id "$PROJECT_ID" --keyvault-arm-id <key-vault-resource-id>` |
| You run the script from automation, with no one to answer prompts | Add `--yes`. See [Script options](#script-options) for exactly what `--yes` accepts. |

> **Note:** If the server is already connected to Azure Arc, you don't need to say where it is.
> The script reads the server's own subscription, resource group and name.

---

## Step 6 — Verify the installation

Run these checks on the server.

1. **The server is connected to Azure Arc.** Check that `Agent Status` shows `Connected`:

   ```bash
   sudo azcmagent show
   ```

2. **The agent service is running.** The expected output is `active`:

   ```bash
   systemctl is-active azmigrate-execution.service
   ```

3. **The extension installed successfully.** The expected output is `Succeeded`:

   ```bash
   az connectedmachine extension show \
     --resource-group <arc-resource-group> \
     --machine-name <arc-server-name> \
     --name AzureMigrateExecutionAgentForLinux \
     --query properties.provisioningState -o tsv
   ```

4. **The setup is complete.** Run the setup script again. On a healthy server it changes nothing
   and reports:

   ```text
     PASS  azmigrate-execution.service is active
     PASS  execution.json records agent '<agent-name>' against https://…
     PASS  the agent's own poll answered 200 -- identity, endpoint, route and auth all work
           this machine is already set up. Nothing was changed by this run.
   ```

> **Note:** These checks prove that the agent is running and can reach Azure Migrate. An idle agent
> with no queued work is healthy, so they don't prove that a job has run yet.

---

## Step 7 — Add credentials for your target machines

The agent needs credentials to sign in to the machines it runs scripts on. Add them on this server
as `root`:

```bash
sudo -i
set -a; . /etc/azure-migrate/extension.env; set +a
amcredential add --name <friendly-name> --user <user-name> --type LinuxServer
```

- The second command loads the Key Vault key settings that the installation wrote, so you don't
  copy anything by hand.
- You're prompted for the password. It's encrypted with your Key Vault key, never shown, and never
  stored in clear text.
- Use letters and digits only for `<friendly-name>`. This is the name your jobs use as the run-as
  account.

Place your own scripts in `/var/opt/microsoft/azuremigrate/scriptstore`.

---

## Run the script again, reinstall, or remove the agent

### Run the script again

Running the script again is always safe. It checks the current state first, and if everything
is healthy it exits without changing anything. Run it again whenever you want a health check.

### Reinstall from scratch (clean refresh)

If the agent is installed but not working, or you want a fresh start, use `--force-refresh`.
Preview it first:

```bash
sudo -E bash ./preflight-arc-setup.sh --dry-run --force-refresh --migrate-project-id "$PROJECT_ID"
sudo -E bash ./preflight-arc-setup.sh --force-refresh --migrate-project-id "$PROJECT_ID"
```

> **Warning — this deletes data on the server.** A clean refresh uninstalls the agent and
> deletes its stored credentials, encryption keys, configuration, working files and logs on this
> server. Any job that's running is interrupted. **You must add your credentials again
> afterwards.** The script asks for confirmation first.

A clean refresh **keeps**:

- the server's Azure Arc connection;
- your Key Vault and the keys already in it;
- your other extensions;
- your scripts in `/var/opt/microsoft/azuremigrate/scriptstore`.

It then creates a new encryption key and installs the agent again.

### Remove the agent

Delete the extension from the Arc server:

```bash
az connectedmachine extension delete \
  --resource-group <arc-resource-group> \
  --machine-name <arc-server-name> \
  --name AzureMigrateExecutionAgentForLinux
```

Removing the extension leaves your stored credentials and encryption key on the server, so a later
reinstall can still decrypt them. To remove those too, use the clean refresh above.

---

## Script options

`--migrate-project-id` is the only required option. If you leave it out, the script asks for it.

| Option | When to use it |
|---|---|
| `--migrate-project-id <id>` | **Required.** The resource ID of your Azure Migrate project. |
| `--dry-run` | Preview every change without making any. |
| `--yes` | Answer **yes** automatically to tool installs and to a clean refresh. It never approves reusing an existing registration; that always needs you to answer. |
| `--force-refresh` | Uninstall the agent and erase its local data, then install again. See [Reinstall from scratch](#reinstall-from-scratch-clean-refresh). |
| `--tenant <tenant-id>` | Only if you want to state the tenant explicitly. By default the script uses the tenant of the project's subscription. |
| `--arc-subscription <id>` / `--arc-resource-group <name>` | Choose where a server that **isn't yet** connected to Azure Arc is onboarded. Don't use these for a server that's already connected. |
| `--arc-location <region>` | Onboard a new Arc server into a different region than the project's. |
| `--keyvault-arm-id <id>` | Use a Key Vault that you already own. |
| `--keyvault-name <name>` | Name to use when the script creates a new Key Vault. |
| `--keyvault-resource-group <name>` / `--keyvault-subscription <id>` | Where to create a new Key Vault. The default is the project's resource group and subscription. |
| `--key-name <prefix>` | Prefix for the new RSA key name (letters, digits and hyphens only). A unique suffix is always added. |
| `--location <region>` | Override the region. The default is the project's region. |
| `--master-site-id <id>` | Only if the script asks for it: the site that your project's discovery created. |
| `--execution-agent-id <name>` | A custom name for a new agent registration. |
| `--type-handler-version <a.b.c.d>` | Install a specific extension version. The default is the latest version available in your region. |
| `--enable-auto-upgrade` | Let Azure upgrade this extension automatically when new versions are released. |
| `--no-solution-link` | Don't record the Key Vault on the Azure Migrate project. |
| `--help` | Show all options. |
| `--explain` | Show what the script grants, and why. |

---

## Troubleshooting

When the script stops, read the `FAIL` line, then the `Cause:` and `Fix:` lines under it. The
`Fix:` line usually contains the exact command to run next. The most common problems are listed
below.

### Common mistakes

| What you did | What you see | Why it happens | What to do |
|---|---|---|---|
| Ran the script with plain `sudo` | `FAIL  not signed in to Azure (this run looked in /root/.azure)` | Plain `sudo` runs as `root`, which can't see your Azure sign-in. | Run it with `sudo -E`. |
| Signed in with `sudo az login` | `FAIL  not signed in to Azure` | That signed in `root`, not your account. | Sign in as yourself, without `sudo`: `az login --use-device-code`. Then run with `sudo -E`. |
| Passed the subscription, resource group or project separately | `unknown arg: --subscription`, followed by the list of valid options | The script reads all three from `--migrate-project-id`. | Pass only `--migrate-project-id "$PROJECT_ID"`. |
| Copied the script from Windows without converting it | `/usr/bin/env: 'bash\r': No such file or directory` or `$'\r': command not found` | The file has Windows line endings. | Run `sed -i 's/\r$//' preflight-arc-setup.sh`, then try again. |
| Piped the script into `bash` (for example `curl … \| sudo bash`) | `-- no terminal to ask at; assuming no (pass --yes to accept)` | A piped script can't ask you questions, so every prompt is answered **no**. | Download the script and run it from a file, or add `--yes`. |
| Ran setup before discovery had finished | `FAIL  discovery has not been run on this Azure Migrate project` | Registration needs the site that discovery creates. Nothing was created or changed. | Run discovery in the Azure portal, wait for it to finish, then run the script again. |

### Errors and how to fix them

| Message | Cause | Fix |
|---|---|---|
| `FAIL  could not read the Azure Migrate project …` | The project ID is wrong, or your account can't read the project. | Copy the ID again from **Azure Migrate project > Properties > Resource ID**. Make sure you have at least **Reader** on the project's resource group. |
| `FAIL  cannot switch to subscription …` | You're signed in to a different tenant, or your account can't see that subscription. | Run `az login --tenant <tenant-id> --use-device-code` without `sudo`, then run the script again. |
| `FAIL  signed in as …, but that sign-in no longer works` | Your saved sign-in has expired, usually because of your organization's sign-in policy. | Run `az logout`, then `az login --use-device-code` (both without `sudo`), then run the script again. |
| `FAIL  the az 'connectedmachine' extension is not installed` (or the Azure CLI, or the Arc agent) | You answered `n` to the install prompt, or the install failed. | Run again and answer `y`, or install the tool yourself using the command printed under `Fix:`. If the install failed, check `/var/log/azure-migrate/preflight-install.log` and confirm the server can reach `packages.microsoft.com` and `aka.ms`. |
| `FAIL  this machine holds Arc onboarding data for '<name>', which Azure does not confirm` | The server was most likely copied (cloned) from another Arc-enabled server, or holds leftover Arc data. | Clear the data on **this server only** with `sudo azcmagent disconnect --force-local-only`, then run the script again. **Always include `--force-local-only`:** without it, `azcmagent disconnect` deletes the other server's Arc resource from Azure. |
| `WARN  subscription … does not offer Arc in '<region>'` | Azure Arc isn't available in that region for your subscription. | Add `--arc-location <region>` to choose a supported region, or onboard into another subscription with `--arc-subscription` and `--arc-resource-group`. |
| `FAIL  this machine already has an Execution Agent installation that is not working (or is incomplete)` | An earlier installation is broken or unfinished. The script won't pair a new key with old data. | Save anything you need from the server, then run a [clean refresh](#reinstall-from-scratch-clean-refresh). |
| `FAIL  clean refresh was not confirmed; nothing was uninstalled` | You answered `n` to the clean-refresh question, or there was no one to answer it. | Nothing changed. Run again and answer `y`, or add `--yes`. |
| `reuse this existing registration for this same physical host? [y/N]` | This server is already registered in the project. | Answer `y` only if this is the **same physical server**. Never reuse a registration for a cloned or different server. |
| `FAIL  could not install the Execution Agent extension on '<server>'`, with an authorization error | Your account isn't allowed to install extensions on this server. | Ask for **Azure Connected Machine Resource Administrator** or **Contributor** on the Arc server's resource group, then run the script again. |
| `FAIL  could not find which Execution Agent extension version is available in <region>` | The extension catalogue for your region couldn't be read. Nothing was installed. | Wait a few minutes and run the script again. |
| `FAIL  the Execution Agent was not confirmed healthy within 600 seconds` | The installation took longer than 10 minutes. It might still be running. | Run the extension status command printed under `Fix:`. If it shows **Succeeded**, run the script again **without** `--force-refresh`. If it shows **Failed**, fix the reason in the message, then run a clean refresh. |
| `FAIL  the extension installed but the Execution Agent service is not running` | The agent service started and then stopped. | Run `sudo journalctl -u azmigrate-execution.service --no-pager -n 60` to see why. After you fix the reason, run a clean refresh. |
| `FAIL  the Execution Agent cannot get a token from this machine's Arc identity` | The Azure Arc agent on the server isn't issuing identity tokens. | Run `sudo azcmagent show` and `sudo azcmagent check`, fix any problem they report, then run the script again. |
| `FAIL  the Execution Agent is running but the discovery service rejected its job poll (…)` | **401 or 403:** the registration isn't active for this server yet. **404:** the registration wasn't found. **No response:** the server can't reach Azure Migrate. | For no response, check outbound HTTPS from the server. Otherwise, wait a few minutes and run the script again. |
| `!! NOT READY FOR JOBS` in the summary | Setup on the server succeeded, but the server couldn't be registered in your project. | Contact Microsoft support and include the full script output. |

> **Tip:** Keep the output of a failed run. Don't run a clean refresh repeatedly, because each
> refresh deletes the logs you need to find the cause.

---

## Collect logs for support

If you contact Microsoft support, include the complete script output and these logs:

```bash
# Extension installation log
sudo bash -c 'tail -n 60 /var/lib/GuestConfig/extension_logs/Microsoft.Azure.Migrate.AzureMigrateExecutionAgentForLinux-*/handler.log'

# Agent service log
sudo journalctl -u azmigrate-execution.service --no-pager -n 60

# Agent log
sudo bash -c 'tail -n 60 /var/log/azure-migrate/execution/*.log'

# Tool installation log (only if a tool install failed)
sudo tail -n 60 /var/log/azure-migrate/preflight-install.log
```

> **Note:** These logs never contain your passwords. Review them before you share them, because
> they include resource names and IDs from your environment.
