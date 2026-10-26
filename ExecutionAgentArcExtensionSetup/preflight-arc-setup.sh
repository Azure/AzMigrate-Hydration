#!/usr/bin/env bash
# Arc-extension setup preflight for the Azure Migrate Execution Agent (Linux).
#
# Runs manually on the Arc-enabled Linux VM (console or SSH) under the
# customer's `az login`. Uses that identity for every ARM / Key Vault
# control-plane call.
#
# The Arc VM's system-assigned managed identity gets exactly ONE grant, and it
# is scoped to the Key Vault only:
#   * keys: get, wrapKey, unwrapKey   (read + wrap/unwrap the RSA key)
# That is the Key Vault Crypto User permission set, expressed as an ACCESS
# POLICY. Deliberately NOT Crypto Officer / 'create delete purge': those would
# also let a token stolen off the VM destroy the key that wraps that machine's
# own credential store, which is unrecoverable credential loss.
#
# Access policies, not Azure RBAC. Vaults this script creates are created with
# --enable-rbac-authorization false --no-self-perms, and both grants below are
# `az keyvault set-policy` calls. Two reasons. A policy is scoped to this vault
# and to key permissions only -- there is no equivalent of a role that someone
# later re-scopes up to the resource group or the subscription. And a Key Vault
# RBAC role is a CONTROL-plane write that ARM accepts on ANY vault, so granting
# one looks identical to success even on a vault that will never honour it; a
# policy write either lands on this vault's own resource or fails loudly.
# Vaults preflight did NOT create are left exactly as they are: if such a vault
# runs on RBAC the script uses roles, and if it runs on policies and the
# principal is missing one, it stops and prints the command rather than editing
# somebody else's resource.
#
# Nothing on the resource group, nothing on the subscription, nothing on the
# migrate project. All ARM control-plane work happens here under the customer's
# identity; the Arc MI's only job at runtime is data-plane crypto against the
# one Key Vault it needs.
#
# Division of responsibility:
#   * Preflight creates the Key Vault, the RSA KEY inside it, the master-site
#     record and the key-access grant -- all under the operator's own
#     credentials -- and puts the vault URI and key name in the extension's
#     public settings (Step 6).
#   * The extension handler (enable.sh) only RESOLVES that key and records its
#     URI, in /etc/azure-migrate/extension.env -- the one file the agent and
#     `amcredential` actually read. It cannot create, delete or purge a key, and
#     refuses to start the service if the key is missing or unusable rather than
#     running without it.
#
# A healthy rerun is read-only. A fresh setup always creates a uniquely named
# RSA key; --force-refresh first removes this Execution Agent's local state.
# Arc, the vault, existing vault keys and unrelated extensions are retained.
#
# Steps:
#   1. Prerequisites & Arc onboarding   in this order, cheapest and most
#                                       reversible first:
#                                         a. tooling -- az CLI, the
#                                            connectedmachine az extension and
#                                            azcmagent, each installed only
#                                            after asking
#                                         b. the az login actually works, and
#                                            the providers are registered
#                                         c. is this machine ALREADY registered
#                                            and polling? if so, exit 0 having
#                                            changed nothing (--force-refresh overrides)
#                                         d. has discovery been run? no master
#                                            site -> stop here, before anything
#                                            is created or onboarded
#                                         e. azcmagent connect, if the VM isn't
#                                            already Arc-Connected
#                                       (c) and (d) come before (e) on purpose:
#                                       both can end the run, and neither should
#                                       cost the operator an Arc resource first.
#   2. Key Vault                        BYOKV -> project-linked -> create-in-RG
#                                       (created in ACCESS-POLICY mode, with no
#                                       self-permissions for the creator)
#   3. RSA key + key access             grant the operator 'get create list',
#                                       create the wrapping key under the
#                                       operator's credentials, grant the Arc MI
#                                       'get wrapKey unwrapKey', then record the
#                                       vault on the discovery solution
#   4. Resolve host identity            read this Arc machine's ARM id
#   5. Register executionAgent          check existing hostIdentifier registrations first;
#                                       create a new name only for an unregistered host;
#                                       reuse an existing registration only with consent
#   6. Install Arc extension            `az connectedmachine extension create`;
#                                       settings carry the vault URI, key name
#                                       and the Arc MI identity block. The
#                                       handler resolves the key and writes its
#                                       URI to /etc/azure-migrate/extension.env.
#
# Once Step 6 succeeds the Arc Guest Agent invokes the handler (install.sh ->
# enable.sh) automatically. This script does NOT invoke the handler directly.
#
# usage:
#   sudo -E ./preflight-arc-setup.sh \
#     [--tenant            <tenant-id>]                   # defaults to the signed-in az profile's tenant
#     --migrate-project-id <migrate-project-arm-id>       # sub + RG derived from this
#     [--arc-vm            <arc-machine-name>]            # defaults to $(hostname)
#     [--arc-resource-group <rg>]                         # where to ONBOARD a not-yet-Arc machine;
#                                                         # defaults to the project's RG. An already
#                                                         # Arc-enabled machine reports its own RG and
#                                                         # that wins, so cross-RG needs no flag.
#     [--arc-subscription  <sub-id>]                      # same, for the subscription: only needed to
#                                                         # choose where a NEW Arc machine is created, or
#                                                         # when the project's own subscription does not
#                                                         # offer Arc in the project's region (availability
#                                                         # is per subscription -- Step 1 checks)
#     [--keyvault-resource-group <rg>]                    # RG for a created vault; defaults to the project's RG
#     [--keyvault-subscription <sub-id>]                  # sub for a created vault; defaults to the project's sub
#     [--keyvault-arm-id   <existing-kv-arm-id>]          # BYOKV, overrides all lookup
#     [--keyvault-name     <name>]                        # otherwise: create/reuse this name in the RG
#     [--key-name          execagent-rsakey]              # prefix for a NEW uniquely named RSA key
#     [--location          <region>]                      # defaults to migrate project location
#     [--arc-location      eastus2]                       # region to ONBOARD the Arc VM into;
#                                                         # defaults to --location. Use it when the
#                                                         # project's region is one Arc cannot serve
#                                                         # for this subscription.
#     [--master-site-id    <master-site-arm-id>]          # required only if the project's discovery
#                                                         # solution carries no masterSiteId
#     [--execution-agent-id <name>]                       # override <sanitized-hostname>-<8 GUID chars>-executionAgent
#     [--type-handler-version <x.y.z.n>]                  # default: newest in this machine's regional catalogue
#     [--enable-auto-upgrade]                             # also move EXISTING installs to new versions
#     [--no-solution-link]                                # do NOT record the vault on the migrate
#                                                         # project's discovery solution (Step 3e)
#     [--dry-run]                                         # print what would change; make no writes
#     [--force-refresh]                                   # uninstall this Execution Agent and remove
#                                                         # its local credentials, key ring, scripts,
#                                                         # work files and logs before fresh setup;
#                                                         # keep Arc, the vault and existing vault keys
#     [--yes]                                             # accept tooling installs and destructive refresh;
#                                                         # never accepts adoption of an existing
#                                                         # executionAgent registration
#     [--explain]                                         # print the design and security rationale
#     [--help]                                            # print this usage block
#
# examples:
#   # rehearse first. Writes nothing, prints every change it would make.
#   sudo -E ./preflight-arc-setup.sh --dry-run \
#     --migrate-project-id /subscriptions/SUB/resourceGroups/RG/providers/Microsoft.Migrate/migrateProjects/PROJECT
#
#   # the usual run. Everything else is discovered from the project or defaulted.
#   sudo -E ./preflight-arc-setup.sh \
#     --migrate-project-id /subscriptions/SUB/resourceGroups/RG/providers/Microsoft.Migrate/migrateProjects/PROJECT
#
#   # this machine is not Arc-enabled yet, and must not be onboarded into the
#   # project's own resource group -- or the project's subscription does not
#   # offer Arc in the project's region (Step 1 says so if that is the case).
#   sudo -E ./preflight-arc-setup.sh \
#     --migrate-project-id /subscriptions/SUB/resourceGroups/RG/providers/Microsoft.Migrate/migrateProjects/PROJECT \
#     --arc-subscription ARC_SUB --arc-resource-group ARC_RG
#
# There is no --subscription, --resource-group or --project. All three are read
# out of --migrate-project-id, which is why that one flag is the only required
# input -- passing the pieces separately is the commonest first-run mistake.
#
# --migrate-project-id is the one thing this script cannot work out for itself;
# omit it and you are prompted, not aborted with a cryptic "missing required".
#
# Where the Arc machine lives is NOT something to tell us when it already exists:
# an Arc-enabled machine knows its own name, resource group and subscription, and
# this script runs on it, so it reads them. The --arc-* flags only choose where a
# machine that is NOT yet Arc-enabled gets created. Passing them for a machine
# that is already enabled somewhere else stops the run rather than acting on the
# wrong resource.

set -uo pipefail

# --- inputs ------------------------------------------------------------------

# Mandatory inputs (2). Everything else is derived or defaulted below.
TENANT=""
MIGRATE_PROJECT_ID=""

# Optional inputs with defaults.
ARC_VM=""          # Defaults to `hostname` (the VM the script runs on).
KV_ARM_ID=""       # BYOKV; if set, wins over other KV resolution paths.
KV_ID_SOURCE=""    # byokv|linked|tagged -- how KV_ARM_ID was obtained, once it is.
KV_NAME=""         # Create-this-vault path.
KEY_NAME="execagent-rsakey"
LOCATION=""        # Defaults to migrate project's location.
# Region the Arc MACHINE is onboarded into. Empty means "same as $LOCATION", i.e. the migrate
# project's region, so project and machine stay together unless that region is one Arc cannot
# serve. Only affects `azcmagent connect`; the Key Vault still follows $LOCATION.
ARC_LOCATION_OVERRIDE=""
# Empty means resolve and explicitly install the newest version in this machine's
# regional Arc catalogue. Pass --type-handler-version to pin a specific build.
TYPE_HANDLER_VERSION=""
# Opt-in. Without this the platform never touches an already-installed handler,
# so a machine onboarded today stays on today's version forever. With it, the
# platform rolls new versions out to existing machines as they publish.
ENABLE_AUTO_UPGRADE=0
# Opt-out. Step 3e writes the vault back onto the discovery solution so the next machine, and
# the portal, can see it. It is the only write this script makes to the migrate project itself,
# so there is a flag to refuse it.
LINK_KV_TO_SOLUTION=1
DRY_RUN=0
# Consent, not silence. Nothing that changes the machine itself -- the az CLI, the
# connectedmachine az extension, the Arc agent -- is installed without being asked for first.
# --yes is the escape hatch for unattended runs; with no flag and no terminal to ask at, every
# prompt answers itself "no" and the script stops with the command to run by hand.
ASSUME_YES=0
FORCE_REFRESH=0
SETUP_WAIT_SECONDS=600
SETUP_POLL_SECONDS=10
# How long to wait for Azure to report a just-connected machine as Connected, and for a write
# to the Migrate project / master site to become visible on read. Both are eventually
# consistent: the first read after the write routinely returns the old value.
ARC_CONNECT_WAIT_SECONDS=120
ARC_CONNECT_POLL_SECONDS=10
SOLUTION_READBACK_WAIT_SECONDS=60
READBACK_POLL_SECONDS=5

# Derived from --migrate-project-id after the prompts loop below.
SUBSCRIPTION=""
RG=""

# The Arc machine and the Key Vault may each live in their own subscription.
# Both default to the migrate project's subscription, so the single-subscription
# layout behaves exactly as before and neither flag needs to be passed.
ARC_SUBSCRIPTION=""
KV_SUBSCRIPTION=""

# The Arc VM does NOT have to live in the migrate project's resource group,
# and in real customer environments it usually does not -- Arc machines are
# commonly onboarded into an infrastructure or landing-zone RG managed
# separately from the Azure Migrate project.
#
# DESIGN DECISION: project-owned resources (the Key Vault, and anything else
# this script creates) go in the MIGRATE PROJECT's RG regardless of where the
# Arc VM lives. The VM's RG is only ever read from, never written to.
#
# This is safe because nothing in the flow is RG-bound:
#   * a managed identity is an AAD directory object of type ManagedIdentity.
#     It has no resource group. `az ad sp show` on the Arc MI returns only
#     appId/displayName/type -- there is no RG field to mismatch.
#   * role assignments take an ABSOLUTE --scope (the vault's ARM id) and an
#     ABSOLUTE --assignee (the principal id). Neither is relative to an RG.
#   * 'Key Vault Crypto User' has assignableScopes ["/"], i.e. it may be
#     assigned at any scope in the tenant.
#   * ARM accepts a cross-RG resource reference: an executionAgent whose
#     hostIdentifier names a machine in one resource group can be registered
#     under a master site in a different one.
#
# The same reasoning extends across SUBSCRIPTIONS, which the service supports:
# a master site in one subscription may carry an executionAgent whose host is
# an Arc machine in another, provided the RBAC requirements are met. None of
# the four points above is subscription-bound either -- directory objects have
# no subscription, and scopes/assignees are absolute ARM ids. What does NOT
# carry over is the ambient subscription of the az CLI: `az account set` is a
# single global, so every call that names a resource by -g/-n rather than by
# absolute id must be told which subscription to look in. That is why
# --arc-subscription and --keyvault-subscription exist and are threaded
# through those call sites explicitly.
#
# Trade-off to be aware of: the vault follows the project's REGION, so a VM in
# a distant region pays cross-region latency on Key Vault data-plane calls.
# That is a handful of calls at agent start-up, not per-credential, so it is
# accepted. --keyvault-resource-group remains available as an escape hatch,
# and --keyvault-arm-id (BYOKV) already accepts a vault anywhere.
ARC_RG=""
KV_RG=""

# State populated by steps below. Initialised here so `set -u` cannot abort on
# an error path that skips those steps (e.g. master-site id unresolved -> the
# `soft` branch never assigns these, then the heredoc that emits the extension
# settings would reference an unbound variable).
DISCOVERY_SERVICE_ENDPOINT=""
MASTER_SITE_ID=""
EXECUTION_AGENT_ID=""
# Escape hatch, same spirit as --master-site-id: pin an existing executionAgent
# by name instead of using the derived one. Needed to point this VM at a record
# somebody else registered -- notably one created with the managed-identity auth
# shape, which this script does not produce.
EXECUTION_AGENT_ID_OVERRIDE=""
ARC_MACHINE_RESOURCE_ID=""

PUBLISHER="Microsoft.Azure.Migrate"
EXT_TYPE="AzureMigrateExecutionAgentForLinux"
EXT_NAME="AzureMigrateExecutionAgentForLinux"

# Kept verbatim so a failure can print the exact command to run again, rather than asking the
# operator to reconstruct it.
SCRIPT_ARGS=("$@")

while [[ $# -gt 0 ]]; do
    case "$1" in
        --tenant)                TENANT="${2:-}"; shift 2 ;;
        --migrate-project-id)    MIGRATE_PROJECT_ID="${2:-}"; shift 2 ;;
        --arc-vm)                ARC_VM="${2:-}"; shift 2 ;;
        --arc-resource-group)    ARC_RG="${2:-}"; shift 2 ;;
        --arc-subscription)      ARC_SUBSCRIPTION="${2:-}"; shift 2 ;;
        --keyvault-subscription) KV_SUBSCRIPTION="${2:-}"; shift 2 ;;
        --keyvault-resource-group) KV_RG="${2:-}"; shift 2 ;;
        --keyvault-arm-id)       KV_ARM_ID="${2:-}"; shift 2 ;;
        --keyvault-name)         KV_NAME="${2:-}"; shift 2 ;;
        --master-site-id)        MASTER_SITE_ID="${2:-}"; shift 2 ;;
        --execution-agent-id)    EXECUTION_AGENT_ID_OVERRIDE="${2:-}"; shift 2 ;;
        --key-name)              KEY_NAME="${2:-}"; shift 2 ;;
        --location)              LOCATION="${2:-}"; shift 2 ;;
        --arc-location)          ARC_LOCATION_OVERRIDE="${2:-}"; shift 2 ;;
        --type-handler-version)  TYPE_HANDLER_VERSION="${2:-}"; shift 2 ;;
        --enable-auto-upgrade)   ENABLE_AUTO_UPGRADE=1; shift ;;
        --no-solution-link)      LINK_KV_TO_SOLUTION=0; shift ;;
        --dry-run)               DRY_RUN=1; shift ;;
        --force-refresh)         FORCE_REFRESH=1; shift ;;
        --force)
            echo "--force was replaced by --force-refresh (destructive clean setup; see --help)" >&2
            exit 2 ;;
        -y|--yes)                ASSUME_YES=1; shift ;;
        -h|--help)
            # Only the usage block, with the comment markers stripped. The design notes above
            # it are for whoever edits this script, not for someone trying to run it -- 70
            # lines of rationale before the first option is not help.
            sed -n '/^# usage:/,/^[^#]/p' "$0" | sed '$d; s/^#\{1\} \{0,1\}//; s/^#$//'
            echo "why it does all this, and exactly what it grants:  $0 --explain"
            exit 0 ;;
        --explain)
            # The rationale above the usage block. A hardcoded line range drifted every time
            # this file was edited, and telling an operator to run sed against the script was
            # never an answer to "why does it need these permissions?".
            sed -n '2,/^# usage:/p' "$0" | sed '$d; s/^#\{1\} \{0,1\}//; s/^#$//'
            exit 0 ;;
        *)
            # Someone who mistypes a flag is one character away from the right
            # command, so show them the flags instead of only the rejection.
            echo "unknown arg: $1" >&2
            echo "valid options:" >&2
            sed -n 's/^ *\(--[a-z-]*\)).*/  \1/p' "$0" | sort -u >&2
            echo "run '$0 --help' for what each one means" >&2
            exit 2 ;;
    esac
done

if [[ ! "$KEY_NAME" =~ ^[a-zA-Z0-9-]+$ || ${#KEY_NAME} -gt 90 ]]; then
    echo "--key-name must be a 1-90 character prefix containing only letters, digits and hyphens" >&2
    exit 2
fi
if [[ -n "$EXECUTION_AGENT_ID_OVERRIDE" && ! "$EXECUTION_AGENT_ID_OVERRIDE" =~ ^[a-zA-Z0-9._-]+$ ]]; then
    echo "--execution-agent-id must be a single resource name containing letters, digits, dots, underscores or hyphens" >&2
    exit 2
fi
if [[ -n "$TYPE_HANDLER_VERSION" && ! "$TYPE_HANDLER_VERSION" =~ ^[0-9]+(\.[0-9]+){3}$ ]]; then
    echo "--type-handler-version must be a four-part numeric version" >&2
    exit 2
fi

# Interactive prompt for anything mandatory that wasn't passed. The customer
# runs this script manually -- a bare "missing required --foo" abort was hostile,
# and Bash's `${v,,//_/-}` doesn't chain lowercase + substitution in one
# expansion anyway, so the earlier error message was showing the wrong flag name.
prompt_if_missing() {
    local varname="$1" description="$2" example="$3"
    if [[ -z "${!varname}" ]]; then
        printf '\n%s\n  example: %s\n> ' "$description" "$example"
        local answer=""
        # Read from the controlling terminal so this works even when stdin is a pipe.
        if [[ -r /dev/tty ]]; then
            IFS= read -r answer </dev/tty || answer=""
        else
            IFS= read -r answer || answer=""
        fi
        if [[ -z "$answer" ]]; then
            echo "empty input for $varname; aborting" >&2
            exit 2
        fi
        printf -v "$varname" '%s' "$answer"
    fi
}

prompt_if_missing MIGRATE_PROJECT_ID \
    "Enter the Azure Migrate project ARM id" \
    "/subscriptions/<sub>/resourceGroups/<rg>/providers/Microsoft.Migrate/migrateProjects/<name>"

# The tenant is NOT asked for here. It is read from the signed-in az profile once Step 1 has
# confirmed a working login (see "the tenant is read, not asked for" below). On a fresh VM az
# is not even installed yet at this point, so asking now would stop a customer who passed
# only --migrate-project-id on a question the script can answer itself a minute later.

# Derive subscription + resource group from the migrate project's ARM id.
# ARM shape: /subscriptions/{sub}/resourceGroups/{rg}/providers/{ns}/{type}/{name}
if [[ "$MIGRATE_PROJECT_ID" =~ ^/subscriptions/([^/]+)/resourceGroups/([^/]+)/providers/ ]]; then
    SUBSCRIPTION="${BASH_REMATCH[1]}"
    RG="${BASH_REMATCH[2]}"
else
    echo "malformed --migrate-project-id: $MIGRATE_PROJECT_ID" >&2
    echo "expected: /subscriptions/<sub>/resourceGroups/<rg>/providers/..." >&2
    exit 2
fi

# $RG stays the migrate project's RG and is used only to find the master site.
# Everything that touches the machine or the vault uses its own RG so that a
# cross-RG layout -- the normal case in production -- works without edits.
# Whether the operator named the Arc machine's home themselves. An already-Arc-enabled
# machine reports its own, which is authoritative; these only decide whether a
# disagreement with it is a conflict to stop on or just a default to replace.
ARC_RG_EXPLICIT=1
ARC_SUBSCRIPTION_EXPLICIT=1
ARC_VM_EXPLICIT=1
[[ -n "$ARC_RG"           ]] || { ARC_RG="$RG";                     ARC_RG_EXPLICIT=0; }
[[ -n "$ARC_SUBSCRIPTION" ]] || { ARC_SUBSCRIPTION="$SUBSCRIPTION"; ARC_SUBSCRIPTION_EXPLICIT=0; }
[[ -n "$KV_RG"  ]] || KV_RG="$RG"
# Whether the operator named the vault's subscription themselves. A resolved vault that
# turns out to live somewhere else is a conflict worth stopping on only if they asked for
# somewhere specific; otherwise it is just the project's own vault.
KV_SUBSCRIPTION_EXPLICIT=1
[[ -n "$KV_SUBSCRIPTION"  ]] || { KV_SUBSCRIPTION="$SUBSCRIPTION"; KV_SUBSCRIPTION_EXPLICIT=0; }
if [[ "$ARC_RG" != "$RG" ]] || [[ "$KV_RG" != "$RG" ]]; then
    echo "note: project RG '$RG'; Arc VM RG '$ARC_RG'; Key Vault RG '$KV_RG'"
fi
if [[ "$ARC_SUBSCRIPTION" != "$SUBSCRIPTION" ]] || [[ "$KV_SUBSCRIPTION" != "$SUBSCRIPTION" ]]; then
    echo "note: project subscription '$SUBSCRIPTION'"
    echo "note: Arc VM subscription  '$ARC_SUBSCRIPTION'"
    echo "note: Key Vault subscription '$KV_SUBSCRIPTION'"
    echo "note: a cross-subscription layout needs the caller to hold the required roles in each"
fi

# --arc-vm defaults to hostname of the VM this script runs on.
if [[ -z "$ARC_VM" ]]; then
    ARC_VM="$(hostname)"
    ARC_VM_EXPLICIT=0
    echo "note: --arc-vm not passed; using hostname '$ARC_VM'"
fi

# --- helpers -----------------------------------------------------------------

pass=0; fail=0; warn=0
ok()    { echo "  PASS  $1"; pass=$((pass+1)); }
bad()   { echo "  FAIL  $1"; fail=$((fail+1)); }
soft()  { echo "  WARN  $1"; warn=$((warn+1)); }
note()  { echo "        $1"; }
# Every FAIL (and every WARN that needs action) explains itself the same way, so a customer
# never has to guess what to do next:
#   FAIL  <what went wrong>
#         Cause: <why it happened>
#         Fix:   <what to do>             -- or, when it takes more than one step:
#         Fix:
#           1. <first step>
#           2. <second step>
cause() { note "Cause: ${1:-unknown (the command returned no error message)}"; }
fix()   { if (( $# == 1 )); then note "Fix:   $1"; else note "Fix:"; local i=0 s; for s in "$@"; do i=$((i+1)); note "  $i. $s"; done; fi; }
# The exact command this run was started with, ready to paste back. Each extra argument is
# either a bare flag ("--yes"), added unless already present, or "--flag value" as ONE string,
# which replaces any value the run was started with -- so the printed command never carries
# the same flag twice.
SCRIPT_PATH="$(readlink -f -- "${BASH_SOURCE[0]:-$0}" 2>/dev/null || printf '%s' "$0")"
rerun_cmd() {
    local -a args=(${SCRIPT_ARGS[@]+"${SCRIPT_ARGS[@]}"}) keep=() extra=()
    local e a flag i present out="sudo -E bash $SCRIPT_PATH"
    for e in "$@"; do
        flag="${e%% *}"
        if [[ "$e" == *" "* ]]; then
            keep=() i=0
            while (( i < ${#args[@]} )); do
                if [[ "${args[i]}" == "$flag" ]]; then i=$((i + 2)); continue; fi
                keep+=("${args[i]}"); i=$((i + 1))
            done
            args=(${keep[@]+"${keep[@]}"})
            extra+=("$e")
        else
            present=0
            for a in ${args[@]+"${args[@]}"}; do [[ "$a" == "$flag" ]] && present=1; done
            (( present )) || extra+=("$e")
        fi
    done
    for a in ${args[@]+"${args[@]}"}; do out+=" $(printf '%q' "$a")"; done
    for e in ${extra[@]+"${extra[@]}"}; do out+=" $e"; done
    printf '%s' "$out"
}
head1() { echo; echo "=== $1 ==="; }
did()   { echo "   DO   $1"; }
skip()  { echo "  SKIP  $1  (already in desired state)"; }

# Report a write that succeeded. run() returns 0 in --dry-run without writing
# anything, so a plain ok() after it would claim the vault, the key and the role
# assignment all exist. Use this instead wherever the message describes a
# completed mutation: a rehearsal must never be mistaken for the real thing.
okdid() {
    if (( DRY_RUN )); then
        echo "   DRY  not performed: $1"
    else
        ok "$1"
    fi
}

# Guard every write. In --dry-run, print the command and return 0.
# The preview goes to stderr because several call sites redirect this function's
# stdout to /dev/null to suppress az's JSON output -- on stdout the preview would
# be swallowed by exactly the commands an operator most wants to see.
run() {
    if (( DRY_RUN )); then
        # To the script's ORIGINAL stderr (fd saved in DRY_FD): callers also send this
        # function's stderr to $ERR_FILE to capture az's error, and the preview must not land
        # there with it.
        echo "  DRY   $*" >&"${DRY_FD:-2}"
        return 0
    fi
    "$@"
}
exec 4>&2
DRY_FD=4

# Ask before changing the machine. Returns 0 for yes, non-zero for no.
#
# Reads from /dev/tty rather than stdin on purpose: this script is routinely fed to bash over
# a pipe, where stdin is the script itself and a `read` would silently swallow the next line
# of it. With no terminal to ask at -- cron, a pipeline, a run-command -- there is nobody to
# answer, so the answer is no and the caller prints the command to run by hand. --yes is the
# way to say yes in advance.
confirm() {
    local question="$1" reply=""
    if (( ASSUME_YES )); then
        note "$question -- yes (--yes)"
        return 0
    fi
    # /dev/tty can exist and still not be openable (piped into bash, cron, CI).
    if ! ( : </dev/tty ) 2>/dev/null; then
        note "$question -- no terminal to ask at; assuming no (pass --yes to accept)"
        return 1
    fi
    read -r -p "        $question [y/N] " reply </dev/tty || return 1
    [[ "${reply,,}" == y || "${reply,,}" == yes ]]
}

# The documented way to install az on THIS distro. Printed whether or not the operator lets
# the script run it, so a "no" still leaves them with the exact command.
az_install_cmd() {
    local id="" like=""
    if [[ -r /etc/os-release ]]; then
        id="$(sed -n 's/^ID=//p' /etc/os-release | tr -d '"' | head -1)"
        like="$(sed -n 's/^ID_LIKE=//p' /etc/os-release | tr -d '"' | head -1)"
    fi
    case "${id,,} ${like,,}" in
        *azurelinux*|*mariner*) echo "sudo tdnf install -y azure-cli" ;;
        *ubuntu*|*debian*)      echo "curl -sL https://aka.ms/InstallAzureCLIDeb | sudo bash" ;;
        *rhel*|*fedora*|*centos*) echo "sudo dnf install -y azure-cli" ;;
        *)                      echo "curl -L https://aka.ms/InstallAzureCli | bash" ;;
    esac
}

# --- ~/.azure ownership -----------------------------------------------------
#
# Under `sudo -E` -- the documented way to run this script -- HOME stays the operator's, so
# every az call below runs as ROOT inside the operator's ~/.azure: the token cache, logs,
# telemetry and the connectedmachine extension all end up owned by root. The operator's own az,
# which is how they sign in, then dies on its next write with a 40-line Python traceback ending
# in "PermissionError: [Errno 13] Permission denied: '/home/<user>/.azure/config'". Hand back
# what this run created, on every exit path. Only root-owned entries are touched, symlinks are
# never followed (-h, and find does not descend through them) and the walk stays on one
# filesystem. Real root, and plain sudo (HOME=/root), are left alone.
restore_azure_dir_owner() {
    [[ -n "${SUDO_UID:-}" && "$SUDO_UID" != 0 && "$(id -u)" == 0 ]] || return 0
    local dir="${AZURE_CONFIG_DIR:-$HOME/.azure}" home
    home="$(getent passwd "$SUDO_UID" 2>/dev/null | cut -d: -f6)"
    [[ -n "$home" && "$home" != / && "$dir" == "$home"/* ]] || return 0
    [[ -d "$dir" && ! -L "$dir" ]] || return 0
    find "$dir" -xdev -uid 0 -exec chown -h "$SUDO_UID:${SUDO_GID:-$SUDO_UID}" {} + 2>/dev/null || true
    return 0
}

# stderr of the last probe, kept apart from its stdout. `2>&1` would mix az's WARNING lines
# into the value being read (a location, an id) whenever the call succeeds, while `2>/dev/null`
# throws away the only explanation when it fails. Callers write `2>"$ERR_FILE"`, then quote
# $(last_err) -- az's ERROR line if there is one, else the first non-blank line.
ERR_FILE="$(mktemp "${TMPDIR:-/tmp}/preflight-err.XXXXXX" 2>/dev/null || echo /dev/null)"
last_err() {
    [[ -s "$ERR_FILE" ]] || return 0
    local line
    line="$(grep -m1 '^ERROR' "$ERR_FILE" 2>/dev/null || grep -m1 -v '^[[:space:]]*$' "$ERR_FILE" 2>/dev/null || true)"
    printf '%s' "${line#ERROR: }"
}
# "Does not exist" has two spellings for Arc resources. ARM answers ResourceNotFound when it
# has no record; seconds after a delete ARM still routes to the Arc RP, which answers HCRP404.
arc_not_found() { grep -qiE 'HCRP404|ResourceNotFound|\(NotFound\)' <<<"$1"; }
on_exit() {
    restore_azure_dir_owner
    [[ "$ERR_FILE" == /dev/null ]] || rm -f "$ERR_FILE"
}
trap on_exit EXIT

# --- installer output ---------------------------------------------------------
#
# The Microsoft installers print ~150 lines of apt and azcmagent progress -- package lists,
# "N packages can be upgraded", an `apt autoremove` hint, lsof and netstat warnings -- all
# harmless, and all burying this script's own PASS/WARN/FAIL lines and reading like errors.
# Their output goes to a log instead, and only the tail is shown, and only when one fails.
INSTALL_LOG="/var/log/azure-migrate/preflight-install.log"
install_logged() { # LABEL COMMAND...
    local label="$1" rc=0
    shift
    if ! { mkdir -p "${INSTALL_LOG%/*}" \
           && printf '\n===== %s  %s\n' "$(date -u '+%Y-%m-%dT%H:%M:%SZ')" "$label" >>"$INSTALL_LOG"; } 2>/dev/null; then
        INSTALL_LOG="$(mktemp "${TMPDIR:-/tmp}/preflight-install.XXXXXX" 2>/dev/null)" || INSTALL_LOG=/dev/null
    fi
    did "$label  (installer output: $INSTALL_LOG)"
    "$@" >>"$INSTALL_LOG" 2>&1 </dev/null || rc=$?
    return "$rc"
}
show_install_log() {
    [[ -s "$INSTALL_LOG" ]] || return 0
    note "last lines of $INSTALL_LOG:"
    tail -n 20 "$INSTALL_LOG" 2>/dev/null | sed 's/^/          | /'
    return 0
}

# "a", "a and b", "a, b and c".
join_and() {
    local out="" i=0 a
    for a in "$@"; do
        i=$((i+1))
        if (( i == 1 )); then out="$a"; elif (( i == $# )); then out+=" and $a"; else out+=", $a"; fi
    done
    printf '%s' "$out"
}

# --- Step 1: prerequisites ---------------------------------------------------
#
# Tooling first, before a single Azure call. All three of these -- az, the connectedmachine
# az extension, the Arc agent -- are things this script cannot work without, and each one is
# cheap to check and slow to discover the hard way. Finding out at Step 6 that the extension
# is missing costs the operator everything the script already did.

head1 "Step 1 -- Prerequisites & Arc onboarding"

# Everything missing is listed first and asked about ONCE. A fresh VM is missing all three,
# and three separate [y/N] prompts in a row read like three separate problems.
missing_tools=()
command -v az >/dev/null 2>&1 || missing_tools+=("the az CLI")
if ! command -v az >/dev/null 2>&1 || ! az extension show --name connectedmachine >/dev/null 2>&1; then
    missing_tools+=("the az 'connectedmachine' extension")
fi
command -v azcmagent >/dev/null 2>&1 || missing_tools+=("the Arc agent (azcmagent)")
MISSING_TOOLS="$(join_and ${missing_tools[@]+"${missing_tools[@]}"})"
INSTALL_CONSENT=""
TOOLS_INSTALLED=0
install_consent() {
    if [[ -z "$INSTALL_CONSENT" ]]; then
        if confirm "install ${MISSING_TOOLS:-the missing tools} now?"; then
            INSTALL_CONSENT=yes
        else
            INSTALL_CONSENT=no
        fi
    fi
    [[ "$INSTALL_CONSENT" == yes ]]
}
if [[ -n "$MISSING_TOOLS" ]]; then
    soft "this machine does not have $MISSING_TOOLS yet"
    note "this script needs them to talk to Azure and to connect this machine to Azure Arc."
    note "this is expected on a machine that has never run it. Installer output goes to"
    note "$INSTALL_LOG, not to this screen."
fi

# 1/3 az itself. Nothing below can run without it, including the check for the extension.
if command -v az >/dev/null 2>&1; then
    ok "az CLI is present"
else
    az_cmd="$(az_install_cmd)"
    note "az CLI install command for this machine:  $az_cmd"
    if (( DRY_RUN )); then
        echo "  DRY   $az_cmd"
    else
        if install_consent; then
            install_logged "installing the az CLI" bash -c "$az_cmd" || true
            TOOLS_INSTALLED=1
        fi
        if command -v az >/dev/null 2>&1; then
            ok "az CLI installed"
        else
            bad "the az CLI is not installed"
            if [[ "$INSTALL_CONSENT" == yes ]]; then
                cause "its installer did not succeed (see the log lines below)"
                show_install_log
                fix "make sure this machine can reach packages.microsoft.com, then run this script again:  $(rerun_cmd)"
            else
                cause "the install was declined, or there was no terminal to ask at"
                fix "either let this script install it:  $(rerun_cmd --yes)" \
                    "or install it yourself and run this script again:  $az_cmd"
            fi
            exit 1
        fi
    fi
fi

# 2/3 `az connectedmachine` is NOT part of core az -- it ships as a CLI extension, and every
# Arc read in this script plus the extension install in Step 6 goes through it. Without it az
# falls back to its dynamic-install prompt, which defaults to asking: an unattended run then
# stops at the first Arc read on a question nobody is there to answer, instead of here with a
# name for the problem.
if az extension show --name connectedmachine >/dev/null 2>&1; then
    ok "az 'connectedmachine' extension is present"
else
    note "every Arc read below and the extension install in Step 6 depend on the 'connectedmachine' az extension"
    note "its install command:  az extension add --name connectedmachine"
    if (( DRY_RUN )); then
        echo "  DRY   az extension add --name connectedmachine"
    else
        if install_consent; then
            install_logged "installing the 'connectedmachine' az CLI extension" \
                az extension add --name connectedmachine --only-show-errors || true
            TOOLS_INSTALLED=1
        fi
        if az extension show --name connectedmachine >/dev/null 2>&1; then
            ok "az 'connectedmachine' extension installed"
        else
            bad "the az 'connectedmachine' extension is not installed"
            if [[ "$INSTALL_CONSENT" == yes ]]; then
                cause "'az extension add' did not succeed (see the log lines below)"
                show_install_log
                fix "make sure this machine can reach aka.ms and azcliextensionsync.blob.core.windows.net, then run this script again:  $(rerun_cmd)"
            else
                cause "the install was declined, or there was no terminal to ask at"
                fix "either let this script install it:  $(rerun_cmd --yes)" \
                    "or install it yourself and run this script again:  az extension add --name connectedmachine"
            fi
            exit 1
        fi
    fi
fi

# 3/3 The Arc agent. Present on every machine that is already Arc-connected, so a missing one
# means this machine is not onboarded yet and Step 1 is about to onboard it. Asked for here
# rather than halfway through onboarding, so an operator who does not want the agent on this
# box finds out before anything else has been touched. Staged in a private mktemp directory,
# never a fixed /tmp path: a fixed path is pre-creatable by any local user -- /tmp's sticky
# bit stops them deleting the file but not rewriting one they own, and `curl -o` truncates
# rather than replaces -- so they could swap the body, or plant a symlink, between the
# download and `sudo bash`, which runs it as root on the host that is about to hold the Arc
# identity.
AZCMAGENT_INSTALL='d="$(mktemp -d)" || exit 1; chmod 700 "$d"; curl -sSL https://aka.ms/azcmagent -o "$d/install_linux_azcmagent.sh" && sudo bash "$d/install_linux_azcmagent.sh"; rc=$?; rm -rf "$d"; exit $rc'
if command -v azcmagent >/dev/null 2>&1; then
    ok "Arc agent (azcmagent) is present"
else
    note "the Arc agent is what connects this machine to Azure and holds the managed identity every later step uses"
    note "its install command:  curl -sSL https://aka.ms/azcmagent -o install_linux_azcmagent.sh && sudo bash install_linux_azcmagent.sh"
    if (( DRY_RUN )); then
        echo "  DRY   install_linux_azcmagent.sh (from https://aka.ms/azcmagent)"
    else
        if install_consent; then
            install_logged "installing the Arc agent (install_linux_azcmagent.sh)" bash -c "$AZCMAGENT_INSTALL" || true
            TOOLS_INSTALLED=1
        fi
        if command -v azcmagent >/dev/null 2>&1; then
            # Only the PACKAGE is installed here. Saying just "installed" made customers ask
            # whether the machine was now in Azure; it is not, until the connect further down.
            ok "Arc agent (azcmagent) software installed -- this machine is NOT connected to Azure yet;"
            note "that happens later in this step, after the sign-in check"
        else
            bad "the Arc agent (azcmagent) is not installed"
            if [[ "$INSTALL_CONSENT" == yes ]]; then
                cause "its installer did not succeed (see the log lines below)"
                show_install_log
                fix "make sure this machine can reach aka.ms, gbl.his.arc.azure.com and packages.microsoft.com, then run this script again:  $(rerun_cmd)"
            else
                cause "the install was declined, or there was no terminal to ask at"
                fix "either let this script install it:  $(rerun_cmd --yes)" \
                    "or install it yourself and run this script again:  curl -sSL https://aka.ms/azcmagent -o install_linux_azcmagent.sh && sudo bash install_linux_azcmagent.sh"
            fi
            exit 1
        fi
    fi
fi

signed_in="$(az account show --query user.name -o tsv 2>/dev/null || true)"
if [[ -z "$signed_in" ]]; then
    # Who the operator really is, and where their own az keeps its login. Under sudo that is
    # NOT root's home: plain `sudo` resets HOME to /root, so a login the operator made a
    # minute ago is invisible to this run and looks exactly like "never signed in".
    login_user="$(id -un 2>/dev/null || echo root)" login_home="$HOME"
    if [[ -n "${SUDO_USER:-}" && "$SUDO_USER" != root ]]; then
        login_user="$SUDO_USER"
        login_home="$(getent passwd "$SUDO_USER" 2>/dev/null | cut -d: -f6)"
    fi
    login_cmd="az login --use-device-code"
    [[ -n "$TENANT" ]] && login_cmd="az login --tenant $TENANT --use-device-code"
    if [[ -n "${SUDO_USER:-}" && "$SUDO_USER" != root && -n "$login_home" && "$HOME" != "$login_home" ]]; then
        bad "not signed in to Azure (this run looked in $HOME/.azure)"
        cause "the script was started with plain 'sudo', which switches HOME to $HOME. Your own az login lives in $login_home/.azure, which this run cannot see."
        fix "run this script again with 'sudo -E' (keeps your HOME):  $(rerun_cmd)"
        note "if you have not signed in yet, first run as $login_user, WITHOUT sudo:  $login_cmd"
    else
        if (( TOOLS_INSTALLED )); then
            note "the tools are installed; sign in to Azure to continue."
        fi
        bad "not signed in to Azure"
        cause "az has no sign-in for $login_user (looked in $HOME/.azure). This is expected the first time this script runs on a machine."
        if [[ "$login_user" == root && -z "${SUDO_USER:-}" ]]; then
            fix "sign in:  $login_cmd" \
                "run this script again:  $(rerun_cmd)"
        else
            fix "as $login_user, WITHOUT sudo, sign in:  $login_cmd" \
                "run this script again with 'sudo -E':  $(rerun_cmd)"
            note "do not use 'sudo az login': that signs in root, not $login_user, and leaves root-owned files in your ~/.azure"
        fi
        [[ -n "$TENANT" ]] || note "if your account is in more than one tenant, add  --tenant <tenant-id>  to az login"
    fi
    exit 1
fi
ok "signed in as $signed_in"

if ! az account set --subscription "$SUBSCRIPTION" >/dev/null 2>&1; then
    bad "cannot switch to subscription $SUBSCRIPTION (the Migrate project's subscription)"
    cause "the signed-in account ($signed_in) cannot see that subscription, or signed in to a different tenant"
    fix "check it is listed:  az account list --query \"[].{name:name, id:id, tenant:tenantId}\" -o table" \
        "if it is not, sign in to its tenant (as your normal user, WITHOUT sudo):  az login --tenant <its-tenant-id> --use-device-code" \
        "run this script again:  $(rerun_cmd)"
    exit 1
fi
ok "subscription set to $SUBSCRIPTION"

# The tenant is read, not asked for. It is read AFTER the subscription switch, so it is the
# tenant of the Migrate project's subscription -- the one every later call really runs in --
# and not that of whatever default subscription a multi-tenant login happened to land on.
# A --tenant that disagrees would onboard the machine into one tenant while every other call
# runs in another, so that is a stop, not a guess.
profile_tenant="$(az account show --query tenantId -o tsv 2>/dev/null || true)"
if [[ -z "$TENANT" ]]; then
    TENANT="$profile_tenant"
    [[ -n "$TENANT" ]] && note "using tenant $TENANT (the tenant of subscription $SUBSCRIPTION)"
elif [[ -n "$profile_tenant" && "${TENANT,,}" != "${profile_tenant,,}" ]]; then
    bad "--tenant $TENANT is not the tenant of subscription $SUBSCRIPTION ($profile_tenant)"
    cause "the Migrate project's subscription belongs to tenant $profile_tenant, but --tenant names another one"
    fix "run this script again without --tenant (the right tenant is read automatically), or with  --tenant $profile_tenant"
    exit 1
fi
prompt_if_missing TENANT \
    "Enter the Entra tenant id (Azure portal > Microsoft Entra ID > Overview > Tenant ID)" \
    "00000000-0000-0000-0000-000000000000"

# `az account show` reads the cached profile. It answers "is there a login on this disk",
# never "does that login still work" -- a refresh token killed by a conditional-access sign-in
# frequency policy leaves a profile that looks perfectly healthy. Without this probe the script
# reports a confident PASS and then every later call fails one at a time, each printing the
# same multi-line AADSTS block, until it dies somewhere misleading like "could not resolve
# location from migrate project". Force one real token acquisition here so the failure is
# reported once, at its cause, with the command that fixes it.
if ! az_err="$(az account get-access-token --output none 2>&1)"; then
    bad "signed in as $signed_in, but that sign-in no longer works"
    case "$az_err" in
        *AADSTS700082*|*AADSTS50173*|*"refresh token has expired"*)
            cause "the saved sign-in has expired (your organisation's sign-in frequency policy ends it after a set time)" ;;
        *)
            cause "az could not get a token: ${az_err%%$'\n'*}" ;;
    esac
    fix "as your normal user, WITHOUT sudo:  az logout" \
        "sign in again:  az login --tenant $TENANT --use-device-code" \
        "run this script again:  $(rerun_cmd)"
    exit 1
fi
ok "credentials are valid"

# Providers we depend on. Registration is idempotent; only warn if pending.
#
# Provider registration is per SUBSCRIPTION, so each namespace must be checked in the
# subscription that will actually use it. Checking all four against the project's
# subscription was worse than useless in a cross-subscription layout: it reported
# "Microsoft.HybridCompute Registered" -- true there -- while the subscription the machine
# is really onboarded into had it unregistered, so the check handed out a false pass and
# the failure surfaced later as an unrelated-looking onboarding error.
for ns in "Microsoft.HybridCompute" "Microsoft.KeyVault" "Microsoft.OffAzure" "Microsoft.Migrate"; do
    case "$ns" in
        Microsoft.HybridCompute) ns_sub="$ARC_SUBSCRIPTION" ;;
        Microsoft.KeyVault)      ns_sub="$KV_SUBSCRIPTION" ;;
        *)                       ns_sub="$SUBSCRIPTION" ;;
    esac
    # Only name the subscription when it is not the project's, so the common
    # single-subscription run keeps its short output.
    ns_where=""
    [[ "$ns_sub" == "$SUBSCRIPTION" ]] || ns_where=" in $ns_sub"
    state="$(az provider show --subscription "$ns_sub" -n "$ns" --query registrationState -o tsv 2>/dev/null || true)"
    if [[ "$state" == "Registered" ]]; then
        ok "provider $ns Registered$ns_where"
    elif [[ -z "$state" ]]; then
        # Empty is not a state -- it means the query itself failed. Printing "is ; registering"
        # and then attempting a registration would be two wrong moves on one unknown.
        soft "could not read whether resource provider $ns is registered$ns_where; leaving it alone"
        cause "the signed-in account may not have read access to subscription ${ns_sub}"
        fix "if a later step fails on $ns, ask a subscription Owner/Contributor to run:  az provider register --subscription $ns_sub -n $ns"
    else
        soft "provider $ns is $state$ns_where; registering"
        # Show the reason, not az's full multi-line block -- four of those is a wall of text
        # that buries whatever the operator actually needs to read.
        if ! reg_err="$(run az provider register --subscription "$ns_sub" -n "$ns" 2>&1 >/dev/null)"; then
            soft "could not register resource provider $ns$ns_where"
            cause "${reg_err%%$'\n'*}"
            fix "ask a subscription Owner/Contributor to run:  az provider register --subscription $ns_sub -n $ns"
        fi
    fi
done

# Location defaults to the migrate project's location so extension + KV land
# together. Resolved BEFORE the Arc onboarding branch below, which needs it.
if [[ -z "$LOCATION" ]]; then
    if ! loc_out="$(az resource show --ids "$MIGRATE_PROJECT_ID" --query location -o tsv 2>"$ERR_FILE")" || [[ -z "$loc_out" ]]; then
        bad "could not read the Azure Migrate project $MIGRATE_PROJECT_ID"
        loc_err="$(last_err)"
        case "$loc_err" in
            *AuthorizationFailed*|*"does not have authorization"*)
                cause "the signed-in account ($signed_in) has no read access to the project"
                fix "ask for at least the Reader role on the project's resource group, then run this script again:  $(rerun_cmd)" ;;
            *ResourceNotFound*|*ResourceGroupNotFound*|*"was not found"*|*"could not be found"*)
                cause "no project exists at that id (check spelling and case of the subscription, resource group and project name)"
                fix "copy the id from Azure portal > Azure Migrate project > Properties > Resource ID, and run this script again with that --migrate-project-id" ;;
            *)
                cause "${loc_err:-az returned no location and no error}"
                fix "run this script again:  $(rerun_cmd)" \
                    "if it keeps failing, pass the project's region explicitly:  $(rerun_cmd '--location <region>')" ;;
        esac
        exit 1
    fi
    LOCATION="$loc_out"
    note "using location $LOCATION (from migrate project)"
fi

# Microsoft.Migrate/migrateProjects advertises exactly these api-versions:
# 2018-09-01-preview, 2019-06-01, 2020-05-01, 2020-06-01-preview. 2020-05-01 is
# the newest GA one and is what we use -- a preview version can be withdrawn
# without notice, which is not something a production install script should
# depend on. The extendedDetails payload this script reads is the same on both
# api-versions.
SOLUTIONS_API_VER="2020-05-01"

# ONE GET, shared by every reader.
#
# This script reads exactly two keys off the discovery solution -- masterSiteId for the
# discovery check below, and keyVaultId for the KV resolution chain and again in Step 3e.
# Those are all reads of the SAME document, and that document can be large (the
# extendedDetails payload can run to well over 100 KB), so it is fetched once.
#
# Both keys come back in one query, so readers share a consistent snapshot.
# Step 3e refreshes it before deciding whether to write, and again after a write.
#
# `|| ''` keeps a missing key as an empty field instead of the literal "None" that tsv renders
# for null, and az walks the document itself via --query so none of this needs an interpreter
# on the box. Only the keys named here are fetched: values are read RAW, not through
# to_string, because to_string would wrap each one in its JSON quotes -- and that deliberately
# leaves the appliance certificates and per-discovery error blobs in the payload untouched.
SOLUTION_CACHE_LOADED=0
SOLUTION_MASTER_SITE_ID=""
SOLUTION_KEY_VAULT_ID=""
# Whether the GET itself succeeded. An empty masterSiteId and a failed read are the same
# value but very different facts, and telling a customer to "run discovery" when the real
# problem was a 403 on the Migrate project sends them somewhere they cannot fix it.
SOLUTION_READ_OK=1
# Why the read failed, for the message: "access" = az refused (its error is in
# SOLUTION_READ_ERR), "shape" = az succeeded but returned something this script cannot parse.
SOLUTION_READ_FAIL=""
SOLUTION_READ_ERR=""

load_discovery_solution() {
    local line rc=0
    line="$(az rest --method get \
        --url "https://management.azure.com${MIGRATE_PROJECT_ID}/solutions/Servers-Discovery-ServerDiscovery?api-version=${SOLUTIONS_API_VER}" \
        --query "[[properties.details.extendedDetails.masterSiteId || '', properties.details.extendedDetails.keyVaultId || '']]" \
        -o tsv 2>"$ERR_FILE")" || rc=$?
    SOLUTION_READ_OK=1 SOLUTION_READ_FAIL="" SOLUTION_READ_ERR=""
    if (( rc != 0 )); then
        SOLUTION_READ_OK=0 SOLUTION_READ_FAIL=access SOLUTION_READ_ERR="$(last_err)"
    fi
    # The query is a list INSIDE a list on purpose. az -o tsv prints each element of a
    # top-level list on its own line, so "[a, b]" yields "a\nb" and the tab check below would
    # reject every successful read. "[[a, b]]" is one row, printed as "a\tb".
    # read with a tab IFS collapses a leading empty field, moving keyVaultId
    # into masterSiteId. Split explicitly so an existing vault cannot disappear.
    SOLUTION_MASTER_SITE_ID=""
    SOLUTION_KEY_VAULT_ID=""
    if [[ "$line" == *$'\t'* ]]; then
        SOLUTION_MASTER_SITE_ID="${line%%$'\t'*}"
        SOLUTION_KEY_VAULT_ID="${line#*$'\t'}"
    elif (( SOLUTION_READ_OK )); then
        SOLUTION_READ_OK=0 SOLUTION_READ_FAIL=shape
    fi
    SOLUTION_CACHE_LOADED=1
    return 0
}

# Error/Cause/Fix for a failed discovery-solution read. Shared by the master-site check, the
# Key Vault resolution and the Key Vault link. Pass "discovery" where the read decides whether
# discovery has run, so the operator is not sent off to rerun discovery over a failed read.
explain_solution_read_failure() {
    local get="az rest --method get --url \"https://management.azure.com${MIGRATE_PROJECT_ID}/solutions/Servers-Discovery-ServerDiscovery?api-version=${SOLUTIONS_API_VER}\""
    if [[ "$SOLUTION_READ_FAIL" == shape ]]; then
        cause "Azure answered, but not in the shape this script expects (this is a script defect, not a problem with your project)"
        fix "send this output and the result of the command below to the extension provider:  $get"
        return 0
    fi
    case "$SOLUTION_READ_ERR" in
        *AuthorizationFailed*|*"does not have authorization"*|*Forbidden*)
            cause "the signed-in account ($signed_in) cannot read the Migrate project"
            fix "ask for at least the Reader role on the Migrate project's resource group ($RG), then run this script again:  $(rerun_cmd)" ;;
        *ResourceNotFound*|*"was not found"*|*NotFound*)
            cause "the project has no 'Servers-Discovery-ServerDiscovery' solution, or --migrate-project-id points at the wrong project"
            fix "check --migrate-project-id (Azure portal > Azure Migrate project > Properties > Resource ID), then run this script again" ;;
        *)
            cause "$SOLUTION_READ_ERR"
            fix "run this script again (a transient error clears on retry):  $(rerun_cmd)" ;;
    esac
    [[ "${1:-}" == discovery ]] && note "this is a failed READ, not an answer: it does not mean discovery has not been run."
    return 0
}

refresh_discovery_solution() {
    # Step 3e must neither overwrite a newly linked vault using a stale empty
    # snapshot nor verify its own write against that snapshot.
    SOLUTION_CACHE_LOADED=0
    return 0
}

solution_extended_detail() {
    # Prints extendedDetails["$1"], or nothing. The four solutions are created with the
    # project, so the GET succeeds even on an untouched project -- but extendedDetails stays
    # {} until something populates it. Empty is therefore a normal answer here, not a failure.
    #
    # The lazy load below is a safety net, NOT the normal path. Every caller reads this
    # through `$(...)`, which is a subshell: a snapshot taken in there dies with it, so a
    # purely lazy cache would re-fetch on every single read and quietly do nothing at all.
    # load_discovery_solution is therefore called once from the main shell, before the first
    # reader, and the subshells inherit the loaded globals.
    local key="$1"
    [[ -n "$key" ]] || return 0
    (( SOLUTION_CACHE_LOADED )) || load_discovery_solution
    case "$key" in
        masterSiteId) printf '%s' "$SOLUTION_MASTER_SITE_ID" ;;
        keyVaultId)   printf '%s' "$SOLUTION_KEY_VAULT_ID" ;;
        *)
            # Unreachable without editing this script, and silently returning empty would
            # read as "the project does not have that" -- which is how a one-character typo
            # becomes a resource created twice. Fail loudly instead.
            bad "internal error: the discovery solution snapshot does not carry '$key'"
            note "add it to load_discovery_solution's --query and to the case below it"
            exit 1 ;;
    esac
    return 0
}

# --- Already-installed check (runs before anything is touched) ---------------
#
# Asked FIRST, before the discovery check below and before this machine is onboarded,
# because the answer can end the run having changed nothing at all. A machine that is
# already registered and polling needs no vault resolved, no key created, no grant made, no
# record PUT and no extension installed -- and re-running those on it is not free: Step 3
# re-reads the vault, Step 5 re-LISTs the master site, and Step 6 re-sends the extension
# settings, which makes the Arc Guest Agent run enable.sh again against a service that is
# already healthy.
#
# Three legs, because "already installed" is three separate facts and an operator needs to
# be told which one is false:
#   1. the service is running              systemctl is-active
#   2. it has a registration to run with   execution.json says Registered and carries an
#                                          ExecutionAgentId and a ServiceEndpoint
#   3. that registration actually works    the agent's own poll answers 200
#
# Leg 3 is the only one that shows the install is FUNCTIONING rather than merely present,
# and it is the exact request the agent makes in its job loop -- GET
# {ServiceEndpoint}/agentrestapi/agents/{ExecutionAgentId}/agentjobs, authenticated with the
# Arc MI token, exactly as the agent does. What that proves is the POLLING PATH: identity,
# endpoint, route and authorisation. It does NOT prove jobs are flowing. This script never
# creates a job, so an empty 200 is the expected healthy answer and the wording
# below is careful never to claim more than that.
#
# All three true -> report and exit 0, having written nothing. Otherwise name the failed
# check and look for leftover state before attempting setup. No local state means a
# normal first run; an existing installation requires explicit clean refresh.
EXECUTION_SERVICE_NAME="azmigrate-execution.service"
EXECUTION_CONFIG_FILE="${AZMIGRATE_CONFIG_DIR:-/etc/azure-migrate/config}/execution.json"
# The directory the Arc agent keeps its challenge files in. A plain assignment, NOT
# ${VAR:-default}: this is a security pin (see arc_mi_token below) and must not be
# influenceable by the environment the script is invoked with. It is a variable only so the
# tests can point it at a temp directory after extracting the function.
ARC_TOKEN_DIR="/var/opt/azcmagent/tokens"

read_execution_config() {
    # Prints execution.json, or nothing. Its directory is 0700 root-owned, so a non-root run
    # needs sudo -- and -n, non-interactive, on purpose: a probe that stops to ask for a
    # password turns a two-second check into a prompt nobody is expecting, and "could not
    # read it" is a perfectly good answer here. The run continues either way.
    if [[ -r "$EXECUTION_CONFIG_FILE" ]]; then
        cat "$EXECUTION_CONFIG_FILE" 2>/dev/null
    else
        sudo -n cat "$EXECUTION_CONFIG_FILE" 2>/dev/null
    fi
    return 0
}

json_string_field() {
    # <json> <key> -> the first "key": "value" string at any depth. Suffices for this file:
    # the handler writes it (common.sh write_execution_config) with one key per line and no
    # duplicate names, and ArcTokenAudience is nested one level down.
    sed -n "s/.*\"$2\"[[:space:]]*:[[:space:]]*\"\([^\"]*\)\".*/\1/p" <<<"$1" | head -1
    return 0
}

curl_with_authorization() {
    # <authorization value> <curl args...> -> runs curl with "Authorization: <value>".
    #
    # The header goes in through --config on stdin, never -H: curl's argv is readable from
    # /proc/<pid>/cmdline by every local user, and these values speak for the machine identity.
    # printf is a builtin, so the value never appears in any process's argv. Same approach as
    # the handler's common.sh. The config line is quote-delimited and processes backslash
    # escapes, so a value carrying either, or a control character, is refused rather than
    # allowed to reshape the request.
    local authorization="$1"
    shift
    case "$authorization" in
        *\"*|*\\*|*[![:print:]]*) return 1 ;;
    esac
    printf 'header = "Authorization: %s"\n' "$authorization" | curl --config - "$@"
}

arc_mi_token() {
    # <audience> -> an Arc MI access token on stdout, nothing on any failure.
    #
    # Two round trips: the Arc endpoint answers 401 with WWW-Authenticate: Basic
    # realm=<challenge-file>, and that file is readable only by the Arc agent's own account.
    # Mirrors get_arc_token in the handler's common.sh, path pinning included -- 40342 is an
    # unprivileged port, so while himds is down any local user can answer on it, name some
    # other root-readable file as the challenge, and be handed its contents.
    local audience="$1" encoded url challenge realm basic realm_dir dir_owner file_owner dir_mode
    command -v curl >/dev/null 2>&1 || return 0
    encoded="$(sed 's|:|%3A|g; s|/|%2F|g' <<<"$audience")"
    url="http://localhost:40342/metadata/identity/oauth2/token?resource=${encoded}&api-version=2020-06-01"

    challenge="$(curl -s -i --max-time 10 -H "Metadata: true" "$url" 2>/dev/null)" || return 0
    realm="$(grep -i '^WWW-Authenticate:' <<<"$challenge" | sed 's/.*realm=//I' | tr -d '\r"')"
    [[ -n "$realm" ]] || return 0
    case "$realm" in
        "$ARC_TOKEN_DIR"/*.key) ;;
        *) return 0 ;;
    esac
    case "$realm" in *..*) return 0 ;; esac
    [[ -f "$realm" && ! -L "$realm" ]] || return 0

    # NOT "owned by root": Arc ships these files owned by the himds service account on a
    # stock agent, so a root check rejects every token request on a normal machine. Sharing
    # an owner with the token directory still closes the squat, because creating a file
    # there means already owning the Arc agent's own directory -- and the directory must not
    # be world-writable, or that owner check could simply be sidestepped.
    realm_dir="$(dirname "$realm")"
    [[ ! -L "$realm_dir" ]] || return 0
    dir_owner="$(stat -c '%u' "$realm_dir" 2>/dev/null)"
    file_owner="$(stat -c '%u' "$realm" 2>/dev/null)"
    dir_mode="$(stat -c '%a' "$realm_dir" 2>/dev/null)"
    [[ -n "$dir_owner" && "$file_owner" == "$dir_owner" ]] || return 0
    case "$dir_mode" in *[2367]) return 0 ;; esac

    if [[ -r "$realm" ]]; then
        basic="$(cat "$realm" 2>/dev/null)"
    else
        basic="$(sudo -n cat "$realm" 2>/dev/null)"
    fi
    [[ -n "$basic" ]] || return 0

    curl_with_authorization "Basic $basic" -s --max-time 15 -H "Metadata: true" "$url" 2>/dev/null \
        | sed -n 's/.*"access_token"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -1
    return 0
}

BF_SERVICE=0 BF_CONFIG=0 BF_POLL=0
bf_agent_id="" bf_endpoint="" bf_api="" bf_audience="" bf_status="" bf_code="" bf_why=""

if (( FORCE_REFRESH )); then
    note "--force-refresh: replacing this Execution Agent, not reusing its local state"
else
    bf_cfg="$(read_execution_config)"
    if [[ -n "$bf_cfg" ]]; then
        bf_status="$(json_string_field "$bf_cfg" RegistrationStatus)"
        bf_agent_id="$(json_string_field "$bf_cfg" ExecutionAgentId)"
        bf_endpoint="$(json_string_field "$bf_cfg" ServiceEndpoint)"
        bf_api="$(json_string_field "$bf_cfg" ApiVersion)"
        bf_audience="$(json_string_field "$bf_cfg" ArcTokenAudience)"
    fi

    systemctl is-active --quiet "$EXECUTION_SERVICE_NAME" 2>/dev/null && BF_SERVICE=1
    if [[ "$bf_status" == "Registered" && -n "$bf_agent_id" && -n "$bf_endpoint" ]]; then
        BF_CONFIG=1
    fi

    # Leg 3 only runs when leg 2 gave us something to call with. The api-version comes out of
    # execution.json rather than from this script's own constant: the file records the
    # version the agent was actually registered with, and guessing a different one here
    # would report a mismatch as a broken install.
    if (( BF_CONFIG )); then
        if [[ -z "$bf_api" ]]; then
            bf_why="the registration records no ApiVersion, so its poll cannot be reproduced"
        else
            bf_token="$(arc_mi_token "${bf_audience:-https://management.azure.com/}")"
            if [[ -z "$bf_token" ]]; then
                bf_why="no Arc MI token (himds down, or this machine is not Arc-connected yet)"
            else
                bf_code="$(curl_with_authorization "Bearer $bf_token" -s -o /dev/null -w '%{http_code}' --max-time 20 \
                    -H "Accept: application/json" \
                    "${bf_endpoint%/}/agentrestapi/agents/${bf_agent_id}/agentjobs?api-version=${bf_api}&jobStatus=NotStarted" \
                    2>/dev/null || true)"
                if [[ "$bf_code" == "200" ]]; then
                    BF_POLL=1
                else
                    bf_why="the agent's own poll answered ${bf_code:-no response}, not 200"
                fi
            fi
            unset bf_token
        fi
    fi

    if (( BF_SERVICE && BF_CONFIG && BF_POLL )); then
        ok "$EXECUTION_SERVICE_NAME is active"
        ok "execution.json records agent '$bf_agent_id' against $bf_endpoint"
        ok "the agent's own poll answered 200 -- identity, endpoint, route and auth all work"
        note "this machine is already set up. Nothing was changed by this run."
        note "that 200 confirms the POLLING PATH, not that jobs are flowing: an empty 200 is"
        note "the healthy answer when no work is queued."
        note "to replace it with a clean setup:  $(rerun_cmd --force-refresh)"
        note "to take the agent off this machine, remove the extension and then its record:"
        note "  az connectedmachine extension delete -g <arc-rg> --machine-name <arc-vm> -n $EXT_NAME"
        exit 0
    fi

    if (( BF_SERVICE || BF_CONFIG )); then
        soft "this machine is partly set up; checking whether a clean refresh is required"
        if (( BF_SERVICE )); then
            note "  service   $EXECUTION_SERVICE_NAME is active"
        else
            note "  service   $EXECUTION_SERVICE_NAME is NOT active"
        fi
        if (( BF_CONFIG )); then
            note "  registry  execution.json records agent '$bf_agent_id' against $bf_endpoint"
        else
            note "  registry  $EXECUTION_CONFIG_FILE does not record a completed registration"
        fi
        if (( BF_POLL )); then
            note "  polling   the agent's own poll answered 200"
        else
            note "  polling   ${bf_why:-not attempted -- there is no registration to poll with}"
        fi
    fi
fi

# --- Discovery check (runs before this machine is onboarded) -----------------
#
# The master site is resolved HERE: before `azcmagent connect`, before the Key Vault, the
# wrapping key, the role grants and the extension. Discovery creates the master site; this
# script never does and must not. The executionAgent record is a child of it, and the
# discovery endpoint is read from one of its sibling sites, so a run that reached Step 5
# without one would already have Arc-onboarded the machine, created a vault and created a
# key -- and would then stop, leaving all of it behind for a project that cannot use it.
#
# The ONLY accepted sources are the project's own discovery solution and --master-site-id.
#
# The master site is deliberately NOT guessed by listing Microsoft.OffAzure/MasterSites in
# the project's resource group and matching on name. A master-site name keeps only the
# first 14 characters of the project name
# ("ContosoMigrateProject-VMware" -> "ContosoMigrate1234mastersite"), so such a match is a
# 14-character prefix -- with nothing confirming the site belongs to this project at all.
# Nor is there a cheap way to confirm it: portal-created master sites leave
# properties.migrateProjectId empty (see patch_master_site_project in Step 5), and the
# "Migrate Project" tag is not reliably present. Getting it wrong does not fail loudly either -- the executionAgent
# is registered under a stranger's master site and the agent comes up healthy, polling the
# wrong project's discovery endpoint. A stop the operator answers with --master-site-id is
# strictly better than a guess nobody can check.
#
# Fetch the solution ONCE, here, in the main shell. Every reader goes through
# `$(solution_extended_detail ...)` and therefore runs in a subshell, where a snapshot it
# took would be discarded the moment the substitution ended -- this is the call that makes
# the cache real, and it is why the fetch is not simply left to the first reader.
load_discovery_solution

if [[ -n "$MASTER_SITE_ID" ]]; then
    note "master site pinned by --master-site-id $MASTER_SITE_ID"
else
    # Authoritative: the same link the Azure Migrate service itself follows.
    MASTER_SITE_ID="$(solution_extended_detail masterSiteId)"
    if [[ -n "$MASTER_SITE_ID" ]]; then
        ok "resolved master site $(basename "$MASTER_SITE_ID") from the discovery solution"
    elif (( ! SOLUTION_READ_OK )); then
        # An empty answer and no answer at all are not the same thing. Sending someone off to
        # run discovery because their identity cannot read the Migrate project would waste
        # their time on the one thing that is not wrong.
        if (( DRY_RUN )); then
            soft "could not read the project's discovery solution; a real run would STOP here"
            explain_solution_read_failure discovery
        else
            bad "could not read the project's discovery solution"
            explain_solution_read_failure discovery
            exit 1
        fi
    elif (( DRY_RUN )); then
        soft "no master site for this project -- discovery has not been run; a real run would STOP here"
        note "everything below is rehearsed against a project that cannot accept an agent yet"
    else
        bad "discovery has not been run on this Azure Migrate project"
        cause "the project's discovery solution has no master site yet. The master site is created when discovery runs (by the Azure Migrate appliance), never by this script, and the agent cannot be registered without it."
        fix "in the Azure portal, run discovery on this Azure Migrate project and wait for it to finish" \
            "run this script again:  $(rerun_cmd)"
        note "nothing in Azure was created or changed by this run, and this machine is not connected to Arc."
        note "if the Arc agent was installed above, it is only installed ('azcmagent show' reports Disconnected)."
        exit 1
    fi
fi

# Arc VM must be Connected before KV / MI / extension steps can run. If it isn't
# yet, install azcmagent + run `azcmagent connect` -- same shape as the portal's
# OnboardingScript.sh, driven from the customer's `az login` context.
#
# Requirements for the Arc-enable branch: sudo on this VM; outbound HTTPS to
# *.his.arc.azure.com / management.azure.com / login.microsoftonline.com; and
# a customer identity with the "Azure Connected Machine Onboarding" role (or
# Contributor on the RG). azcmagent prompts device-code auth if it cannot
# inherit from az login.
# An Arc-enabled machine already knows its own name, resource group AND
# subscription, and we are running ON it. Reading that is strictly better than
# guessing from flags: it needs no permissions, searches nothing, and is right
# by construction wherever the machine lives -- which is what makes a machine in
# another RG or another subscription work without the operator telling us.
#
# Without this, a machine already Arc-enabled outside the project's RG would not
# be found by the lookup below, would fall into the onboarding branch, and would
# then be reported as a clone -- with a remedy (disconnect --force-local-only)
# that would sever a perfectly healthy machine from Azure.
#
# -j is the documented machine-readable form, but it is not on every agent
# version in use, and the exact key spelling is not something to stake the
# whole behaviour on. So the table is read too when JSON yields nothing. Losing
# both only costs us the adoption -- the run then falls back to the lookup below.
arc_local_name="" arc_local_rg="" arc_local_sub="" arc_local_status=""
arc_field() { # table label -> value, tolerant of label case and spacing
    sed -n "s/^[[:space:]]*$1[[:space:]]*:[[:space:]]*//Ip" <<<"$2" | head -1 | tr -d '[:space:]'
}
if command -v azcmagent >/dev/null 2>&1; then
    arc_local_json="$(azcmagent show -j 2>/dev/null || true)"
    if [[ -n "$arc_local_json" ]]; then
        # `azcmagent show -j` is a flat object printed one key per line, so the four values
        # can be read with sed. A key whose value is null or absent simply yields nothing,
        # which the "-" check below already treats as "ask the table instead".
        arc_json_field() { # <key> -- prints the first string value for that key
            sed -n "s/.*\"$1\"[[:space:]]*:[[:space:]]*\"\([^\"]*\)\".*/\1/p" <<<"$arc_local_json" \
                | head -1 | tr -d '[:space:]'
        }
        arc_local_name="$(arc_json_field resourceName)"
        arc_local_rg="$(arc_json_field resourceGroup)"
        arc_local_sub="$(arc_json_field subscriptionId)"
        arc_local_status="$(arc_json_field status)"
    fi
    if [[ -z "$arc_local_name" || "$arc_local_name" == "-" ]]; then
        # "Resource Namespace" also begins with "Resource Name", so the colon in the
        # pattern is load-bearing -- without it the namespace line wins and every
        # machine looks like it is called Microsoft.HybridCompute.
        arc_local_table="$(azcmagent show 2>/dev/null || true)"
        if [[ -n "$arc_local_table" ]]; then
            arc_local_name="$(arc_field 'Resource Name' "$arc_local_table")"
            arc_local_rg="$(arc_field 'Resource Group' "$arc_local_table")"
            arc_local_sub="$(arc_field 'Subscription ID' "$arc_local_table")"
            arc_local_status="$(arc_field 'Agent Status' "$arc_local_table")"
        fi
    fi
fi

if [[ "$arc_local_status" == "Connected" && -n "$arc_local_name" && "$arc_local_name" != "-" ]]; then
    # Pinning a field to something the machine contradicts would make every call
    # below act on a different resource than the one in front of us, so that is a
    # stop, not a default to overwrite -- the same rule the Key Vault id follows.
    arc_conflict=""
    if (( ARC_VM_EXPLICIT )) && [[ "$ARC_VM" != "$arc_local_name" ]]; then
        arc_conflict+="  --arc-vm '$ARC_VM', but this machine is registered as '$arc_local_name'"$'\n'
    fi
    if (( ARC_RG_EXPLICIT )) && [[ "$ARC_RG" != "$arc_local_rg" ]]; then
        arc_conflict+="  --arc-resource-group '$ARC_RG', but this machine is in '$arc_local_rg'"$'\n'
    fi
    if (( ARC_SUBSCRIPTION_EXPLICIT )) && [[ "$ARC_SUBSCRIPTION" != "$arc_local_sub" ]]; then
        arc_conflict+="  --arc-subscription '$ARC_SUBSCRIPTION', but this machine is in '$arc_local_sub'"$'\n'
    fi
    if [[ -n "$arc_conflict" ]]; then
        bad "this machine is already connected to Azure Arc, but not where the flags say"
        printf '%s' "$arc_conflict"
        cause "Azure acts on the registration the machine itself holds, so a flag cannot override it"
        fix "run this script again WITHOUT --arc-vm / --arc-resource-group / --arc-subscription to use the existing registration" \
            "only if the machine really belongs elsewhere: disconnect it first (sudo azcmagent disconnect), then run this script again with the flags"
        exit 1
    fi
    if [[ "$ARC_VM" != "$arc_local_name" || "$ARC_RG" != "$arc_local_rg" || "$ARC_SUBSCRIPTION" != "$arc_local_sub" ]]; then
        note "this VM is already Arc-enabled; using its own registration rather than the defaults"
        note "  machine '$arc_local_name' in RG '$arc_local_rg', subscription '$arc_local_sub'"
    fi
    ARC_VM="$arc_local_name"; ARC_RG="$arc_local_rg"; ARC_SUBSCRIPTION="$arc_local_sub"
fi

arc_state="$(az connectedmachine show --subscription "$ARC_SUBSCRIPTION" -g "$ARC_RG" -n "$ARC_VM" --query status -o tsv 2>/dev/null || true)"
if [[ "$arc_state" == "Connected" ]]; then
    ok "Arc VM '$ARC_VM' is Connected"
else
    soft "Arc VM '$ARC_VM' status is '${arc_state:-not-found}'; onboarding via azcmagent"

    # The machine is onboarded into the migrate project's region by default, so the project and
    # the machine stay together unless the operator asks otherwise.
    #
    # Arc's advertised region list IS subscription-aware, and this is worth being precise about
    # because it is easy to assume the opposite and ignore it. Two
    # subscriptions can get different region lists from the same query on the same day, depending
    # on which regions each subscription is enabled for. So a region missing here is a real signal
    # about THIS subscription, not a stale global list.
    #
    # Still a hint rather than a gate: azcmagent is the only thing that truly knows, a dry run
    # must be able to rehearse against a machine that is not onboarded yet, and blocking would
    # lock out exactly the Canary subscriptions this script is used in. Queried rather than
    # hardcoded so it stays current as Arc expands.
    ARC_ONBOARD_LOCATION="${ARC_LOCATION_OVERRIDE:-$LOCATION}"
    ARC_REGION_UNLISTED=0
    arc_supported="$(az provider show --subscription "$ARC_SUBSCRIPTION" -n Microsoft.HybridCompute \
        --query "resourceTypes[?resourceType=='machines'].locations[]" -o tsv 2>/dev/null \
        | tr '[:upper:]' '[:lower:]' | tr -d ' ' || true)"

    if [[ -z "$arc_supported" ]]; then
        soft "could not read Arc's region list; proceeding with '$ARC_ONBOARD_LOCATION'"
    elif ! grep -qx "$ARC_ONBOARD_LOCATION" <<<"$arc_supported"; then
        ARC_REGION_UNLISTED=1
        soft "subscription $ARC_SUBSCRIPTION does not offer Arc in '$ARC_ONBOARD_LOCATION'"
        note "this list is read per subscription, so a region missing here is not offered to"
        note "THIS subscription and onboarding into it will most likely be refused."
        note "if a different subscription is enrolled, onboard the machine there:"
        note "  --arc-subscription <sub-id> --arc-resource-group <rg>"
        note "or pick a region this subscription does offer:  --arc-location <region>"
        note "continuing anyway -- azcmagent is the only authority on this"
    else
        ok "Arc advertises '$ARC_ONBOARD_LOCATION'"
    fi

    # The Arc agent itself was dealt with at the top of Step 1, with the operator's consent,
    # before anything else had been touched. By here it is present or the script has already
    # stopped -- except in a dry run, which installs nothing and previews the connect anyway.

    # `azcmagent connect` refuses outright whenever the agent already holds local onboarding
    # state. That happens most often on a VM cloned from an already-onboarded machine, and the
    # agent's own error names neither the clone nor the stale resource. Plain
    # `azcmagent disconnect` DELETES the Azure resource that state points at -- which may belong
    # to someone else entirely -- so the guidance here is always --force-local-only.
    stale_arc_name=""
    if command -v azcmagent >/dev/null 2>&1; then
        stale_arc_name="$(sudo azcmagent show 2>/dev/null \
            | sed -n 's/^Resource Name[[:space:]][[:space:]]*:[[:space:]]*//p' | head -1)"
    fi
    if [[ -n "$stale_arc_name" ]]; then
        bad "this machine holds Arc onboarding data for '$stale_arc_name', which Azure does not confirm"
        if [[ "$stale_arc_name" != "$ARC_VM" ]]; then
            cause "'$stale_arc_name' is a different machine from '$ARC_VM', so this VM was most likely cloned from it (a clone copies the Arc agent's local data). 'azcmagent connect' refuses while that data is present."
        else
            cause "the Arc agent has local onboarding data that Azure does not confirm -- left over from an earlier onboarding, or in a subscription this sign-in cannot read. 'azcmagent connect' refuses while that data is present."
        fi
        # Two very different situations look identical from here, and guessing between
        # them is how an operator gets told to break a working machine. A clone cannot
        # be told apart locally either: cloned VMs inherit the original's SMBIOS UUID,
        # so even the vmUuid Azure stores matches on both.
        fix "if the data is stale or cloned, clear it on THIS machine only:  sudo azcmagent disconnect --force-local-only" \
            "then run this script again:  $(rerun_cmd)"
        note "if '$stale_arc_name' is a real machine in a subscription you cannot read from this sign-in,"
        note "  instead run:  $(rerun_cmd '--arc-subscription <its-sub-id>' '--arc-resource-group <its-rg>')"
        note "do NOT drop --force-local-only: a plain 'azcmagent disconnect' DELETES '$stale_arc_name' from Azure"
        if (( DRY_RUN )); then
            note "dry run: continuing so the rest of the rehearsal still prints"
        else
            exit 1
        fi
    fi

    did "azcmagent connect (device-code auth may be prompted in a browser)"
    connect_rc=0
    run sudo azcmagent connect \
        --resource-group  "$ARC_RG" \
        --tenant-id       "$TENANT" \
        --location        "$ARC_ONBOARD_LOCATION" \
        --subscription-id "$ARC_SUBSCRIPTION" \
        --resource-name   "$ARC_VM" \
        --cloud           AzureCloud || connect_rc=$?

    # A dry run never actually onboards, so a not-yet-Arc machine can never reach Connected
    # here. Failing would make --dry-run useless on exactly the machine it is most useful
    # for: a fresh VM an operator wants to rehearse against before committing to anything.
    if (( DRY_RUN )); then
        note "dry run: Arc onboarding was not performed, continuing as if it succeeded"
    else
        # `azcmagent connect` returns once the LOCAL agent is connected, but Azure's view of the
        # machine lags it: the first `az connectedmachine show` routinely still says
        # AwaitingConnection (or the resource is not readable yet) for a few seconds up to a
        # couple of minutes. Reading it once and failing told customers onboarding had failed
        # when it had in fact succeeded. Poll, and only stop early on a state that cannot
        # turn into Connected by waiting.
        arc_waited=0 arc_state=""
        if (( connect_rc == 0 )); then
            while :; do
                arc_state="$(az connectedmachine show --subscription "$ARC_SUBSCRIPTION" -g "$ARC_RG" -n "$ARC_VM" --query status -o tsv 2>/dev/null || true)"
                case "$arc_state" in
                    Connected|Error|Disconnected|Expired) break ;;
                esac
                (( arc_waited >= ARC_CONNECT_WAIT_SECONDS )) && break
                note "waiting for Azure to report '$ARC_VM' as Connected (now '${arc_state:-not visible yet}', ${arc_waited}s of ${ARC_CONNECT_WAIT_SECONDS}s)"
                sleep "$ARC_CONNECT_POLL_SECONDS"
                arc_waited=$((arc_waited + ARC_CONNECT_POLL_SECONDS))
            done
        fi
        if [[ "$arc_state" == "Connected" ]]; then
            arc_after=""
            (( arc_waited > 0 )) && arc_after=" (after ${arc_waited}s)"
            ok "Arc VM '$ARC_VM' now Connected$arc_after"
        else
            local_status="$(sudo azcmagent show 2>/dev/null | sed -n 's/^[[:space:]]*Agent Status[[:space:]]*:[[:space:]]*//p' | head -1)"
            local_error="$(sudo azcmagent show 2>/dev/null | sed -n 's/^[[:space:]]*Agent Error[[:space:]]*:[[:space:]]*//p' | head -1)"
            if (( connect_rc != 0 )); then
                bad "connecting this machine to Azure Arc failed ('azcmagent connect' exit code $connect_rc)"
            else
                bad "this machine did not show as Connected in Azure (Azure reports '${arc_state:-not found}' after ${arc_waited}s)"
            fi
            note "  this machine's own Arc agent reports: ${local_status:-unknown}${local_error:+ -- $local_error}"
            if (( ARC_REGION_UNLISTED )); then
                cause "subscription $ARC_SUBSCRIPTION does not offer Arc in '$ARC_ONBOARD_LOCATION' (flagged above)"
                fix "run this script again, onboarding into a region this subscription offers:  $(rerun_cmd '--arc-location <region>')" \
                    "or into a subscription that is enrolled for that region:  $(rerun_cmd '--arc-subscription <sub-id>' '--arc-resource-group <rg>')"
                note "either way the Key Vault stays in $LOCATION"
            elif (( connect_rc == 0 )) && [[ "${local_status,,}" == connected ]]; then
                cause "the machine IS connected; Azure has not caught up yet"
                fix "wait a minute, then run the same command again (safe: completed steps are skipped):  $(rerun_cmd)"
            else
                cause "${local_error:-see the azcmagent output above}"
                fix "check this machine's connectivity to Azure Arc:  sudo azcmagent check" \
                    "fix what it reports (usually a proxy or firewall rule), then run this script again:  $(rerun_cmd)"
            fi
            exit 1
        fi
    fi
fi

ARC_MI_PRINCIPAL_ID="$(az connectedmachine show --subscription "$ARC_SUBSCRIPTION" -g "$ARC_RG" -n "$ARC_VM" --query identity.principalId -o tsv 2>"$ERR_FILE")"
if [[ -z "$ARC_MI_PRINCIPAL_ID" ]]; then
    # Same reasoning as above: no onboarding means no identity yet. The placeholder is obviously
    # not a principal id, so it cannot be mistaken for one in the dry-run transcript.
    if (( DRY_RUN )); then
        ARC_MI_PRINCIPAL_ID="<arc-mi-principal-id-not-yet-created>"
        note "dry run: using a placeholder Arc MI principalId"
    else
        bad "could not read the managed identity of Arc machine '$ARC_VM'"
        mi_err="$(last_err)"
        cause "${mi_err:-the machine resource has no system-assigned identity yet (Azure is still finishing the onboarding)}"
        fix "wait a minute, then run this script again:  $(rerun_cmd)"
        exit 1
    fi
fi

# An extension resource must be created in ITS MACHINE'S region, which is not
# necessarily the migrate project's region -- $LOCATION comes from the project
# and is right for the Key Vault, wrong for the extension. Read the machine's
# own location instead of assuming they match.
ARC_LOCATION="$(az connectedmachine show --subscription "$ARC_SUBSCRIPTION" -g "$ARC_RG" -n "$ARC_VM" --query location -o tsv 2>"$ERR_FILE" || true)"
if [[ -z "$ARC_LOCATION" ]]; then
    if (( DRY_RUN )); then
        ARC_LOCATION="$LOCATION"
    else
        bad "could not read the region of Arc machine '$ARC_VM'"
        cause "$(last_err)"
        fix "run this script again:  $(rerun_cmd)"
        exit 1
    fi
fi
[[ "$ARC_LOCATION" == "$LOCATION" ]] || \
    note "Arc VM is in '$ARC_LOCATION'; migrate project is in '$LOCATION'"
ok "Arc MI principalId $ARC_MI_PRINCIPAL_ID"

# --- Refresh: remove only this Execution Agent before creating anything ------

execution_extensions() {
    local base url row next names all="" seen="" deadline=$((SECONDS + SETUP_WAIT_SECONDS))
    base="https://management.azure.com/subscriptions/$ARC_SUBSCRIPTION/resourceGroups/$ARC_RG/providers/Microsoft.HybridCompute/machines/$ARC_VM/extensions"
    url="$base?api-version=2024-07-10"
    while [[ -n "$url" ]]; do
        if [[ "$url" != "$base?"* || "$seen" == *"|$url|"* ]] || (( SECONDS >= deadline )); then
            echo "invalid, repeated or timed-out extension list page" >&2
            return 1
        fi
        seen="${seen}|${url}|"
        row="$(timeout 30 az rest --method get --url "$url" \
            --query "[[nextLink || '-', join('|', value[?properties.publisher=='$PUBLISHER' && properties.type=='$EXT_TYPE'].name) || '-']]" \
            -o tsv)" || return 1
        IFS=$'\t' read -r next names <<<"${row//$'\r'/}"
        [[ "$names" == "-" ]] || all="${all}${names//|/$'\n'}"$'\n'
        if [[ "$next" == "-" ]]; then url=""; else url="$next"; fi
    done
    printf '%s' "$all"
}

# A step of the local cleanup failed part-way. Everything before it is already gone, and the
# refresh is written to be re-run, so the fix is always the same.
refresh_step_failed() { # WHAT
    bad "--force-refresh could not $1; the cleanup is incomplete"
    cause "$(last_err)"
    fix "run the same command again; --force-refresh is safe to repeat and continues where it stopped:  $(rerun_cmd --force-refresh)"
    return 1
}

refresh_execution_agent() {
    local names="$1" name remaining deadline path resolved workdirs pid
    # The script store belongs to the customer and is never touched, not even its artifacts/
    # folder: the reinstalled agent reuses that folder and its retention ages old output out.
    local directories=(
        /opt/azure-migrate/execution-agent /var/opt/azure-migrate/keys
        /var/log/azure-migrate/execution /etc/systemd/system/azmigrate-execution.service.d
    )
    head1 "Clean refresh -- uninstall the Execution Agent first"
    soft "this deletes local Execution Agent credentials, encryption keys, work files and logs"
    note "running jobs will be stopped; credentials must be added again; save required diagnostics first"
    note "the script store /var/opt/microsoft/azuremigrate/scriptstore is not touched; customer scripts and job output are kept"
    note "Arc, its managed identity, the Key Vault, existing vault keys and unrelated extensions are retained"
    if [[ "$EXECUTION_CONFIG_FILE" != "/etc/azure-migrate/config/execution.json" ]]; then
        bad "--force-refresh cannot clean this installation automatically"
        cause "it uses a custom configuration directory (AZMIGRATE_CONFIG_DIR=$EXECUTION_CONFIG_FILE), and erasing an unknown path is not safe"
        fix "remove that installation by hand, then run this script again without --force-refresh"
        return 1
    fi
    if [[ "${AZMIGRATE_KEYRING_PATH:-/var/opt/azure-migrate/keys}" != /var/opt/azure-migrate/keys \
        || "${AZMIGRATE_CREDENTIAL_FILE:-/etc/azure-migrate/credentials.json}" != /etc/azure-migrate/credentials.json \
        || "${AZMIGRATE_LOG_DIR:-/var/log/azure-migrate/execution}" != /var/log/azure-migrate/execution ]]; then
        bad "--force-refresh cannot clean this installation automatically"
        cause "it uses custom credential, key ring or log paths (AZMIGRATE_* variables), and erasing an unknown path is not safe"
        fix "unset those AZMIGRATE_* variables if they are left over in your shell, or remove that installation by hand; then run this script again"
        return 1
    fi
    if (( ! DRY_RUN )) && [[ "$(id -u)" != 0 ]]; then
        bad "--force-refresh needs root"
        cause "it removes the agent's package, service and files, which only root may do"
        fix "run this script again with sudo:  $(rerun_cmd)"
        return 1
    fi
    if ! command -v timeout >/dev/null 2>&1; then
        bad "--force-refresh needs the 'timeout' command, which this machine does not have"
        cause "the coreutils package is missing or incomplete"
        fix "install coreutils (for example: sudo apt-get install -y coreutils), then run this script again"
        return 1
    fi
    # Validate every target before the first uninstall, not halfway through deleting state.
    for path in "${directories[@]}" /etc/azure-migrate /etc/azure-migrate/config; do
        [[ -e "$path" || -L "$path" ]] || continue
        resolved="$(readlink -f -- "$path")"
        if [[ "$resolved" != "$path" ]]; then
            bad "--force-refresh stopped before changing anything: $path is a symbolic link (to $resolved)"
            cause "deleting through a link could erase files outside the Execution Agent"
            fix "check where $path points, remove the link by hand if it is safe, then run this script again"
            return 1
        fi
    done
    if ! workdirs="$(find /tmp -maxdepth 1 -type d -name 'am-exec-*' -print 2>"$ERR_FILE")"; then
        refresh_step_failed "list the agent's job folders in /tmp"
        return 1
    fi
    while IFS= read -r path; do
        [[ -n "$path" ]] || continue
        resolved="$(readlink -f -- "$path")"
        [[ "$resolved" == /tmp/am-exec-* && "$resolved" == "$path" ]] \
            || { bad "--force-refresh stopped before changing anything: job folder $path resolves outside /tmp"; cause "it is a symbolic link"; fix "remove the link by hand, then run this script again"; return 1; }
    done <<<"$workdirs"
    if (( ! DRY_RUN )) && ! confirm "uninstall and erase this Execution Agent's local state?"; then
        bad "clean refresh was not confirmed; nothing was uninstalled"
        cause "the answer was no, or there was no terminal to ask at"
        fix "run this script again and answer y, or pass --yes:  $(rerun_cmd --yes)"
        return 1
    fi

    while IFS= read -r name; do
        name="${name%$'\r'}"
        [[ -n "$name" ]] || continue
        did "uninstalling Execution Agent extension '$name'"
        if ! run az connectedmachine extension delete --subscription "$ARC_SUBSCRIPTION" \
            -g "$ARC_RG" --machine-name "$ARC_VM" -n "$name" --yes --no-wait true 2>"$ERR_FILE"; then
            bad "could not uninstall extension '$name'; nothing on this machine was erased"
            cause "$(last_err)"
            fix "run this script again (a transient error clears on retry):  $(rerun_cmd)"
            return 1
        fi
    done <<<"$names"

    if (( ! DRY_RUN )); then
        if [[ -n "$names" ]]; then
            note "Azure Arc removes the extension in the background; this usually takes 2-5 minutes (limit $((SETUP_WAIT_SECONDS / 60)) minutes). Please wait."
        fi
        deadline=$((SECONDS + SETUP_WAIT_SECONDS))
        local started=$SECONDS next_tick=$((SECONDS + 60))
        while :; do
            if ! remaining="$(execution_extensions 2>"$ERR_FILE")"; then
                bad "could not check whether the extension was removed; nothing on this machine was erased"
                cause "$(last_err)"
                fix "run this script again:  $(rerun_cmd)"
                return 1
            fi
            [[ -z "$remaining" ]] && break
            if (( SECONDS >= deadline )); then
                bad "the extension was still being removed after ${SETUP_WAIT_SECONDS}s; nothing on this machine was erased"
                cause "Azure Arc has not finished the uninstall (it is slow when the machine is busy or briefly offline)"
                fix "check its state:  az connectedmachine extension list --subscription $ARC_SUBSCRIPTION -g $ARC_RG --machine-name $ARC_VM -o table" \
                    "once it is gone, run this script again:  $(rerun_cmd)"
                return 1
            fi
            if (( SECONDS >= next_tick )); then
                note "still uninstalling... $((SECONDS - started))s elapsed"
                next_tick=$((SECONDS + 60))
            fi
            sleep "$SETUP_POLL_SECONDS"
        done
        if [[ -n "$names" ]]; then
            note "extension removed after $((SECONDS - started))s"
        fi
    fi

    if systemctl is-active --quiet "$EXECUTION_SERVICE_NAME" 2>/dev/null; then
        if ! run timeout "$SETUP_WAIT_SECONDS" systemctl stop "$EXECUTION_SERVICE_NAME"; then
            bad "could not stop the Execution Agent service; nothing on this machine was erased"
            cause "systemd did not stop $EXECUTION_SERVICE_NAME within ${SETUP_WAIT_SECONDS}s (a running job may be holding it)"
            fix "look at:  sudo systemctl status $EXECUTION_SERVICE_NAME" \
                "stop it:  sudo systemctl stop $EXECUTION_SERVICE_NAME" \
                "run this script again:  $(rerun_cmd)"
            return 1
        fi
    fi
    if (( ! DRY_RUN )); then
        if ! pid="$(systemctl show "$EXECUTION_SERVICE_NAME" --property=MainPID --value)"; then
            bad "could not confirm the Execution Agent service stopped; nothing on this machine was erased"
            cause "systemctl could not report the service's process"
            fix "check:  sudo systemctl status $EXECUTION_SERVICE_NAME  -- then run this script again"
            return 1
        fi
        if [[ "$pid" != 0 ]]; then
            bad "the Execution Agent service still has a running process (PID $pid); nothing on this machine was erased"
            cause "the service did not exit when stopped"
            fix "stop it:  sudo systemctl kill $EXECUTION_SERVICE_NAME  -- then run this script again:  $(rerun_cmd)"
            return 1
        fi
    fi
    # Do not use dpkg --purge: its legacy postrm removes shared azure-migrate parent directories.
    if command -v dpkg-query >/dev/null 2>&1; then
        if dpkg-query -W azure-migrate-execution-agent >/dev/null 2>&1; then
            run dpkg --remove azure-migrate-execution-agent 2>"$ERR_FILE" || { refresh_step_failed "remove the azure-migrate-execution-agent package"; return 1; }
        fi
    fi
    if command -v rpm >/dev/null 2>&1; then
        if rpm -q azure-migrate-execution-agent >/dev/null 2>&1; then
            run rpm -e azure-migrate-execution-agent 2>"$ERR_FILE" || { refresh_step_failed "remove the azure-migrate-execution-agent package"; return 1; }
        fi
    fi

    for path in "${directories[@]}"; do
        [[ -e "$path" || -L "$path" ]] || continue
        resolved="$(readlink -f -- "$path")"
        if [[ "$resolved" != "$path" ]]; then
            bad "--force-refresh stopped: $path is a symbolic link (to $resolved)"
            cause "deleting through a link could erase files outside the Execution Agent"
            fix "check where $path points, remove the link by hand if it is safe, then run this script again"
            return 1
        fi
        run rm -rf --one-file-system -- "$resolved" 2>"$ERR_FILE" || { refresh_step_failed "remove $path"; return 1; }
    done
    run rm -f -- /etc/azure-migrate/credentials.json /etc/azure-migrate/credentials.json.lock \
        /etc/azure-migrate/extension.env \
        /etc/azure-migrate/config/execution.json \
        /usr/bin/amcredential /etc/logrotate.d/azure-migrate-execution \
        /etc/systemd/system/azmigrate-execution.service \
        /usr/lib/systemd/system/azmigrate-execution.service \
        /lib/systemd/system/azmigrate-execution.service \
        /etc/systemd/system/multi-user.target.wants/azmigrate-execution.service \
        2>"$ERR_FILE" || { refresh_step_failed "remove the agent's configuration files"; return 1; }

    while IFS= read -r path; do
        [[ -n "$path" ]] || continue
        resolved="$(readlink -f -- "$path")"
        [[ "$resolved" == /tmp/am-exec-* && "$resolved" == "$path" ]] \
            || { bad "--force-refresh stopped: job folder $path resolves outside /tmp"; cause "it is a symbolic link"; fix "remove the link by hand, then run this script again"; return 1; }
        run rm -rf --one-file-system -- "$resolved" 2>"$ERR_FILE" || { refresh_step_failed "remove $path"; return 1; }
    done <<<"$workdirs"
    run systemctl daemon-reload 2>"$ERR_FILE" || { refresh_step_failed "reload systemd"; return 1; }
    okdid "Execution Agent local state removed; Arc and Key Vault retained"
}

if ! existing_execution_extensions="$(execution_extensions 2>"$ERR_FILE")"; then
    ext_list_err="$(last_err)"
    if (( DRY_RUN )) && { arc_not_found "$ext_list_err" || [[ "$ext_list_err" == *NotFound* ]]; }; then
        # A dry run never onboards, so on a fresh machine there is no Arc resource whose
        # extensions could be listed. That is the expected answer, not a failure: it has none.
        existing_execution_extensions=""
        note "dry run: '$ARC_VM' is not an Arc machine yet, so it has no extensions to check"
    else
        bad "could not list the extensions installed on Arc machine '$ARC_VM'"
        cause "$ext_list_err"
        fix "run this script again (a transient error clears on retry):  $(rerun_cmd)"
        exit 1
    fi
fi
if (( FORCE_REFRESH )); then
    refresh_execution_agent "$existing_execution_extensions" || exit 1
elif [[ -n "$existing_execution_extensions" || -e /etc/azure-migrate/credentials.json \
    || -d /var/opt/azure-migrate/keys || -d /opt/azure-migrate/execution-agent ]]; then
    bad "this machine already has an Execution Agent installation that is not working (or is incomplete)"
    cause "a new install creates a new Key Vault key, which cannot read the credentials and key ring left by the old installation"
    fix "save anything you need from it (credentials are NOT kept and must be added again with amcredential)" \
        "run this script again with --force-refresh, which uninstalls it cleanly first:  $(rerun_cmd --force-refresh)"
    exit 1
fi

# --- Step 2: Key Vault (check-then-create) -----------------------------------

head1 "Step 2 -- Key Vault"

# Resolution order (matches the portal UX):
#   1. --keyvault-arm-id (BYOKV) wins.        <- user picked a vault in the portal blade
#   2. Else: a KV already linked to this migrate project.
#   3. Else: create a new KV named --keyvault-name in --resource-group.
#
# The vault can be reused, including BYOKV. Step 3 always creates a new RSA key
# under the operator's identity; the handler and Arc MI never create keys.
#
# The project -> vault link lives on the discovery SOLUTION, not on the migrate
# project and not on the vault: properties.details.extendedDetails.keyVaultId.
# resolve_kv_from_project() below reads it. When it is absent -- the common case
# on this path -- callers pass --keyvault-arm-id (BYOKV) or --keyvault-name.
#
# Do not try to DERIVE the vault name instead. The portal's shape looks like
# <first 14 chars of project name><4 digits>kv, but those 4 digits cannot be
# derived from (subscription, resource group, project), and many existing
# projects do not follow the shape at all. A guessed name would also have to win a
# global uniqueness check and dodge soft-deleted vaults. Read the id; never
# guess the name.


extract_service_endpoint() {
    # Prints the discovery service endpoint carried by an executionAgents PUT or
    # LIST response, or nothing. The field is not in the published contract and
    # older service versions omit it, so the key is matched by suffix at any depth rather
    # than by a fixed path, and the caller keeps its sibling-site fallback.
    #
    # Matched by scanning the text rather than by parsing: "any key whose name
    # ends in serviceEndpoint, anywhere in the document" is not expressible in
    # JMESPath, and a flat scan is exactly that query. Safe for this value in
    # particular -- a URL contains no quote and no backslash, so the first
    # unescaped quote after http really is the end of the string.
    local json="$1"
    [[ -n "$json" ]] || return 0
    printf '%s' "$json" \
        | tr -d '\n' \
        | grep -o -i '"[A-Za-z0-9_]*serviceEndpoint"[[:space:]]*:[[:space:]]*"http[^"]*"' \
        | head -n1 \
        | sed 's/.*:[[:space:]]*"//; s/"$//'
    return 0
}

resolve_kv_from_project() {
    # Returns non-zero = "could not resolve", which makes the caller fall
    # through to --keyvault-arm-id / --keyvault-name. That fallback is not a
    # corner case but the NORMAL path: keyVaultId is only ever a pointer to a
    # vault some other flow already created, and the Arc path creates none, so
    # most projects do not carry it. Reuse it when it is there so we do not
    # create a second vault behind the portal's back.
    local kv
    kv="$(solution_extended_detail keyVaultId)"
    [[ -n "$kv" ]] || return 1
    printf '%s' "$kv"
}

find_kv_by_tag() {
    # A vault WE create never shows up in the discovery solution: extendedDetails
    # is written only by the Azure Migrate service, and an ARM resource create
    # does not trigger it. So we need our own idempotency handle, and we use the
    # one Azure Migrate itself uses -- a 'Migrate Project' tag carrying the
    # project name (e.g. vault ContosoMigrate1234kv tagged
    # 'Migrate Project=ContosoMigrateProject').
    #
    # Diagnostics go to stderr on purpose: this function's stdout is captured by
    # the caller and becomes the vault ARM id, so anything else printed there
    # would be silently glued onto it.
    local proj matches count
    proj="$(basename "$MIGRATE_PROJECT_ID")"
    matches="$(az resource list --subscription "$KV_SUBSCRIPTION" --resource-type Microsoft.KeyVault/vaults \
        -g "$KV_RG" --query "[?tags.\"Migrate Project\"=='${proj}'].id" \
        -o tsv 2>/dev/null | sed '/^$/d;/^None$/d' | sort || true)"
    [[ -n "$matches" ]] || return 1
    count="$(printf '%s\n' "$matches" | wc -l)"
    if (( count > 1 )); then
        # Sorted above, so the pick is at least stable across runs rather than
        # dependent on the order ARM happened to return.
        soft "RG $KV_RG holds $count vaults tagged for project '$proj'; using the first" >&2
        printf '%s\n' "$matches" | sed 's/^/        /' >&2
    fi
    printf '%s' "$(printf '%s\n' "$matches" | head -n1)"
}

kv_name_from_project() {
    # Portal-shaped vault name: up to 14 chars of the project name, 4 digits, "kv".
    # The 4 digits are OURS (sha256 of subscription+rg+project, mod 10000), not the
    # portal's -- see the note above for why theirs cannot be reproduced. The shape
    # is matched so an operator recognises the vault at a glance; what we actually
    # need from it is determinism, so that a re-run whose tag lookup failed still
    # lands on the same name instead of creating a second vault.
    # Result is <=20 chars, within the 24-char vault-name limit.
    local proj stem digits hex d i
    proj="$(basename "$MIGRATE_PROJECT_ID")"
    stem="$(printf '%s' "$proj" | tr -cd 'A-Za-z0-9-' | sed -E 's/-+/-/g' | cut -c1-14)"
    stem="${stem%-}"
    [[ "$stem" =~ ^[A-Za-z] ]] || stem="am${stem}"
    stem="$(printf '%s' "$stem" | cut -c1-14)"
    stem="${stem%-}"
    # sha256sum is coreutils, so it is on every machine this runs on. The modulo is taken
    # 16 bits at a time because a 256-bit integer does not fit in shell arithmetic, while
    # (d * 65536 + next-4-hex-digits) % 10000 never exceeds ~655 million and does. That is
    # exactly int(hexdigest, 16) % 10000, so the same inputs always derive the same
    # name -- which is the whole point of deriving it.
    hex="$(printf '%s' "${SUBSCRIPTION}${KV_RG}${proj}" | sha256sum 2>/dev/null | cut -d' ' -f1)"
    [[ "$hex" =~ ^[0-9a-f]{64}$ ]] || return 1
    d=0
    for (( i = 0; i < 64; i += 4 )); do d=$(( (d * 65536 + 16#${hex:i:4}) % 10000 )); done
    digits="$(printf '%04d' "$d")"
    [[ -n "$digits" ]] || return 1
    printf '%s%skv' "$stem" "$digits"
}


# --- KV resolution chain ---
# Each read of the discovery solution below issues its own GET; nothing is warmed here.

if [[ -n "$KV_ARM_ID" ]]; then
    note "BYOKV: using caller-supplied vault $KV_ARM_ID"
    KV_NAME="$(basename "$KV_ARM_ID")"
    KV_ID_SOURCE=byokv
elif [[ -n "$KV_NAME" ]]; then
    # Above the discovered paths deliberately. A name the operator typed is as explicit as
    # an ARM id they typed; ranked below `linked`, it would be silently overwritten by whatever
    # the project pointed at, so --keyvault-name could never be used to steer away from the
    # project's own vault -- which is exactly what the cross-subscription conflict below
    # tells them to do.
    note "using caller-supplied vault name $KV_NAME in RG $KV_RG"
elif linked="$(resolve_kv_from_project)" && [[ -n "$linked" ]]; then
    KV_ARM_ID="$linked"
    KV_NAME="$(basename "$KV_ARM_ID")"
    KV_ID_SOURCE=linked
    ok "resolved Key Vault $KV_NAME from the discovery solution"
    note "vault id $KV_ARM_ID"
elif tagged="$(find_kv_by_tag)" && [[ -n "$tagged" ]]; then
    KV_ARM_ID="$tagged"
    KV_NAME="$(basename "$KV_ARM_ID")"
    KV_ID_SOURCE=tagged
    note "reusing vault tagged for this migrate project: $KV_ARM_ID"
elif KV_NAME="$(kv_name_from_project)" && [[ -n "$KV_NAME" ]]; then
    note "no vault supplied or linked; using derived name $KV_NAME in RG $KV_RG"
else
    bad "could not choose a Key Vault for this project"
    cause "the project has no linked or tagged vault, and a vault name could not be derived from the project name"
    fix "run this script again naming a new, globally unique vault:  $(rerun_cmd '--keyvault-name <name>')" \
        "or use an existing vault:  $(rerun_cmd '--keyvault-arm-id <vault-arm-id>')"
    exit 1
fi

# An ARM id is self-describing, so once a vault id is known it -- not the flags -- says
# where that vault lives. Reading the subscription from a flag while the id said otherwise
# is how a vault that already existed in one subscription was reported as being CREATED in
# another: the "lives elsewhere" guard below compares only the resource group, so a
# same-named resource group in a different subscription slipped straight through to the
# create branch. Vault names are globally unique, so that create could never have worked.
#
# ARM emits both 'resourceGroups' and 'resourcegroups' in ids, sometimes within one
# response, so both parses are case-insensitive.
if [[ -n "$KV_ARM_ID" ]]; then
    kv_id_sub="$(sed -n 's#^/subscriptions/\([^/]*\)/.*#\1#Ip' <<<"$KV_ARM_ID")"
    kv_id_rg="$(sed -n 's#^/subscriptions/[^/]*/resourcegroups/\([^/]*\)/.*#\1#Ip' <<<"$KV_ARM_ID")"

    if [[ -n "$kv_id_sub" && "$kv_id_sub" != "$KV_SUBSCRIPTION" ]]; then
        # BYOKV names an exact vault, so the id simply wins over a redundant flag. A vault
        # we resolved ourselves is different: the operator asked for one subscription and
        # the project points at another, and picking either silently is wrong. Using the
        # project's vault means granting roles on a vault in someone else's subscription;
        # ignoring it means the project ends up with two vaults. That is their call.
        if [[ "$KV_ID_SOURCE" != byokv ]] && (( KV_SUBSCRIPTION_EXPLICIT )); then
            bad "this project's Key Vault is in a different subscription than --keyvault-subscription"
            note "  you asked for: $KV_SUBSCRIPTION"
            note "  the vault is:  $KV_ARM_ID"
            cause "the project already uses a vault in subscription $kv_id_sub; both choices are valid, so the script does not pick one for you"
            fix "use the project's vault (access is granted on it, in $kv_id_sub):  $(rerun_cmd "--keyvault-arm-id $KV_ARM_ID")" \
                "or create your own in $KV_SUBSCRIPTION:  $(rerun_cmd '--keyvault-name <globally-unique-name>')"
            exit 1
        fi
        note "vault subscription read from its ARM id: $kv_id_sub"
        KV_SUBSCRIPTION="$kv_id_sub"
    fi

    if [[ -n "$kv_id_rg" && "$kv_id_rg" != "$KV_RG" ]]; then
        note "vault resource group read from its ARM id: $kv_id_rg"
        KV_RG="$kv_id_rg"
    fi
fi

# CHECK before CREATE. az keyvault show returns non-zero if missing.
if az keyvault show --subscription "$KV_SUBSCRIPTION" -n "$KV_NAME" -g "$KV_RG" >/dev/null 2>&1; then
    skip "Key Vault '$KV_NAME' exists in RG $KV_RG"
    KV_ARM_ID="$(az keyvault show --subscription "$KV_SUBSCRIPTION" -n "$KV_NAME" -g "$KV_RG" --query id -o tsv)"
else
    if [[ -n "$KV_ARM_ID" ]] && [[ "$KV_ARM_ID" != */"$KV_RG"/* ]]; then
        # BYOKV vault lives in a different RG; that's fine, verify it exists at all.
        # 'az keyvault show' has no --ids; it rejects the flag outright, so it would fail
        # for every vault that exists. 'az resource show' does take --ids,
        # which is what the location and endpoint lookups elsewhere already rely on.
        if ! az resource show --ids "$KV_ARM_ID" >/dev/null 2>"$ERR_FILE"; then
            bad "could not read the Key Vault given with --keyvault-arm-id: $KV_ARM_ID"
            cause "$(last_err)"
            fix "check the id (Azure portal > the Key Vault > Properties > Resource ID) and that you can read it, then run this script again"
            exit 1
        fi
        skip "BYOKV vault exists"
    else
        did "creating Key Vault $KV_NAME in $KV_RG ($LOCATION), ACCESS-POLICY mode"
        # Purge protection is not reversible and cannot be waived later, so say so
        # before the write rather than leaving the operator to discover it when a
        # cleanup fails. This matters most in a shared resource group.
        note "purge protection will be ON: this vault cannot be deleted or purged for 90 days"
        # Vault names are globally unique and survive deletion: a soft-deleted
        # vault keeps its name until the retention window expires, and because we
        # create with purge-protection it cannot simply be purged. Catch that here
        # so the operator gets an actionable message instead of a raw ARM conflict.
        deleted="$(az keyvault list-deleted --subscription "$KV_SUBSCRIPTION" --query "[?name=='${KV_NAME}'].id" -o tsv 2>/dev/null | head -n1 || true)"
        if [[ -n "$deleted" && "$deleted" != "None" ]]; then
            bad "the Key Vault name '$KV_NAME' is still held by a deleted vault"
            cause "deleted vaults keep their name for their retention period (up to 90 days) and cannot be purged while purge protection is on"
            fix "recover that vault, then run this script again:  az keyvault recover --subscription $KV_SUBSCRIPTION -n $KV_NAME" \
                "or use a different name:  $(rerun_cmd '--keyvault-name <globally-unique-name>')"
            exit 1
        fi
        # ACCESS POLICIES, not Azure RBAC. Two reasons, and the second is the one that
        # matters. First, an access policy is scoped to this vault and to key permissions
        # only -- there is no equivalent of a role that someone later re-scopes to the
        # resource group or the subscription. Second, a Key Vault RBAC role is a CONTROL-plane
        # write that ARM accepts on any vault, so granting one is indistinguishable from
        # success even when the vault will never honour it; a policy write either lands on
        # this vault's own resource or fails loudly.
        #
        # --no-self-perms is deliberate. Without it az grants the creating identity the FULL
        # key/secret/certificate permission set as a self-policy, which is far more than this
        # flow needs and would sit on the vault indefinitely. Step 3a grants the operator
        # exactly 'get create list' on keys instead -- the minimum to create the wrapping key,
        # equivalent to Crypto Officer but without delete, purge, restore or any secret and
        # certificate access. Step 3c grants the Arc MI 'get wrapKey unwrapKey', the Crypto
        # User equivalent, which is all the agent ever does at runtime.
        #
        # Purge-protection on because a deleted key with credentials wrapped by it makes the
        # store unreadable. The tag is what find_kv_by_tag reads on the next run, so this
        # vault is created exactly once no matter how often preflight is re-run.
        if ! run az keyvault create \
            --subscription "$KV_SUBSCRIPTION" \
            -n "$KV_NAME" -g "$KV_RG" -l "$LOCATION" \
            --enable-rbac-authorization false \
            --no-self-perms \
            --enable-purge-protection true \
            --tags "Migrate Project=$(basename "$MIGRATE_PROJECT_ID")" \
            >/dev/null 2>"$ERR_FILE"; then
            bad "could not create Key Vault '$KV_NAME' in resource group $KV_RG"
            kv_create_err="$(last_err)"
            case "$kv_create_err" in
                *VaultAlreadyExists*|*"already in use"*)
                    cause "the name '$KV_NAME' is already taken by a vault in another subscription or tenant (vault names are global)"
                    fix "run this script again with a different name:  $(rerun_cmd '--keyvault-name <globally-unique-name>')" ;;
                *AuthorizationFailed*|*"does not have authorization"*)
                    cause "the signed-in account ($signed_in) may not create resources in resource group $KV_RG"
                    fix "ask for Contributor on resource group $KV_RG, or use an existing vault:  $(rerun_cmd '--keyvault-arm-id <vault-arm-id>')" ;;
                *RequestDisallowedByPolicy*)
                    cause "an Azure Policy blocks this vault: $kv_create_err"
                    fix "ask your Azure administrator for an exemption, or use an existing vault:  $(rerun_cmd '--keyvault-arm-id <vault-arm-id>')" ;;
                *)
                    cause "$kv_create_err"
                    fix "run this script again:  $(rerun_cmd)" ;;
            esac
            note "nothing else was created: no access policies, no keys."
            exit 1
        fi
        # Preflight owns this vault, so 3a and 3c may write access policies on it without
        # asking. On a vault it did not create it still asks first -- see kv_policy_grant.
        KV_CREATED_BY_PREFLIGHT=1
        KV_ARM_ID="$(az keyvault show --subscription "$KV_SUBSCRIPTION" -n "$KV_NAME" -g "$KV_RG" --query id -o tsv 2>/dev/null || true)"
        if [[ -z "$KV_ARM_ID" ]] && (( DRY_RUN )); then
            # A dry run created nothing, so the show above cannot succeed. An
            # ARM id is fully determined by (subscription, rg, name), so
            # synthesise it rather than carrying an empty string forward.
            KV_ARM_ID="/subscriptions/${KV_SUBSCRIPTION}/resourceGroups/${KV_RG}/providers/Microsoft.KeyVault/vaults/${KV_NAME}"
        fi
        # Guard BEFORE the PASS line: an empty id would otherwise print
        # "Key Vault created: " and be counted as a pass.
        if [[ -z "$KV_ARM_ID" ]]; then
            bad "Key Vault '$KV_NAME' was created but cannot be read back yet"
            cause "Azure has not finished creating it (a new vault can take a minute to appear)"
            fix "wait a minute, then run this script again (the vault is reused, not created twice):  $(rerun_cmd)"
            exit 1
        fi
        okdid "Key Vault created: $KV_ARM_ID"
    fi
fi

# Hard stop on an empty id. Step 3 passes this straight to --scope, and an
# empty --scope does not fail -- it silently changes what the role is granted
# on. Previously an unreadable vault still printed PASS and reached that point.
if [[ -z "$KV_ARM_ID" ]]; then
    bad "could not read Key Vault '$KV_NAME' in resource group $KV_RG"
    cause "the vault could not be read with the signed-in account ($signed_in)"
    fix "check you can see it:  az keyvault show --subscription $KV_SUBSCRIPTION -g $KV_RG -n $KV_NAME" \
        "run this script again:  $(rerun_cmd)"
    exit 1
fi

KV_URI="$(az resource show --ids "$KV_ARM_ID" --query properties.vaultUri -o tsv 2>"$ERR_FILE" || true)"
if [[ -z "$KV_URI" ]]; then
    if (( DRY_RUN )); then
        # Vault does not exist in a dry run; the public-cloud URI is derivable.
        KV_URI="https://${KV_NAME}.vault.azure.net/"
    else
        # Discarding stderr turns a usage error into "not found". The first version of this
        # check called 'az keyvault show --ids', which that command rejects, and the symptom
        # read as an unreadable vault every time.
        bad "could not read the address (vaultUri) of Key Vault $KV_NAME"
        cause "$(last_err)"
        fix "run this script again:  $(rerun_cmd)"
        exit 1
    fi
fi
note "vault uri $KV_URI"

# --- Step 3: Create the RSA wrapping key, then grant the Arc MI wrap/unwrap ---
#
# Both halves exist to keep the machine's standing permission as small as it can be. The RSA
# key is created HERE, under the operator's own credentials, so the Arc MI never needs
# keys/create -- and therefore never needs anything resembling Key Vault Crypto Officer, whose
# keys/* would also let a token lifted off the VM delete or purge the very key that wraps that
# machine's credential store. The MI is left with the Crypto User permission set: get, wrapKey,
# unwrapKey. No create, no delete, no purge. A stolen token can then read credentials, which is
# bad, but it cannot destroy every credential irreversibly, which is worse and unrecoverable.
#
# Neither half grants data-plane access by owning the vault in ARM: that is true under access
# policies and under RBAC alike. The operator identity therefore needs its own key access too --
# so every actual setup needs create permission for its fresh wrapping key.
#
# Which mechanism is used is decided by the vault, not by this script. Vaults preflight creates
# are access-policy vaults, so 3a and 3c call `az keyvault set-policy` with the minimum key
# permissions. On a pre-existing vault the script follows whatever that vault already honours:
# roles if it is on RBAC, policies if it is not. ROLE_KV_CRYPTO_OFFICER and ROLE_KV_CRYPTO_USER
# below are therefore used only on the RBAC branch; the access-policy branch spells out the
# equivalent permission sets directly.

head1 "Step 3 -- Create the RSA wrapping key and grant the Arc MI wrap/unwrap"

ROLE_KV_CRYPTO_OFFICER="Key Vault Crypto Officer"
ROLE_KV_CRYPTO_USER="Key Vault Crypto User"

# `az role assignment list/create` resolve the principal and the role through Microsoft Graph.
# An Arc server is not a compliant device, so Conditional Access commonly refuses it a Graph
# token (AADSTS53003) -- and the command then fails even though the operator holds every role
# the assignment actually needs, with an error that blames permissions it does have.
#
# A role assignment is pure ARM, so issue it over ARM. Nothing is bypassed: the same
# Microsoft.Authorization/roleAssignments/write permission is still required and still enforced
# by ARM. The scope is an absolute id, so these calls are subscription-independent.
ROLE_API=2022-04-01

role_def_id() {   # <role-name> <scope>
    az role definition list --name "$1" --scope "$2" --query '[0].id' -o tsv 2>/dev/null || true
}

role_assignment_id() {   # <scope> <role-definition-id> <principal-id>
    az rest --method GET --resource https://management.azure.com \
        --url "https://management.azure.com$1/providers/Microsoft.Authorization/roleAssignments?api-version=$ROLE_API" \
        --query "value[?properties.principalId=='$3' && contains(properties.roleDefinitionId, '$(basename "$2")')] | [0].id" \
        -o tsv 2>/dev/null || true
}

guid_v4() {
    # ARM requires the role-assignment NAME to be a GUID. Three sources, all already on the
    # machine: the kernel's uuid file (part of the kernel, not a package), uuidgen from
    # util-linux, and failing both, 16 bytes of /dev/urandom shaped into a v4 GUID by hand --
    # version nibble forced to 4 and the variant nibble into 8..b, which is all that makes a
    # random 128-bit value a well-formed v4.
    local u h
    u="$(cat /proc/sys/kernel/random/uuid 2>/dev/null || true)"
    [[ -n "$u" ]] && { printf '%s' "$u"; return 0; }
    u="$(uuidgen 2>/dev/null || true)"
    [[ -n "$u" ]] && { printf '%s' "$u"; return 0; }
    h="$(od -An -tx1 -N16 /dev/urandom 2>/dev/null | tr -d ' \n')"
    [[ ${#h} -eq 32 ]] || return 1
    printf '%s-%s-4%s-%x%s-%s' "${h:0:8}" "${h:8:4}" "${h:13:3}" \
        "$(( (0x${h:16:1} & 0x3) | 0x8 ))" "${h:17:3}" "${h:20:12}"
}

role_assign() {   # <scope> <role-definition-id> <principal-id> <principal-type>
    local name
    name="$(guid_v4 || true)"
    if [[ ! "$name" =~ ^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$ ]]; then
        bad "could not generate a unique id for the role assignment"
        cause "none of /proc/sys/kernel/random/uuid, uuidgen or /dev/urandom produced one"
        fix "install util-linux (it provides uuidgen), then run this script again:  $(rerun_cmd)"
        exit 1
    fi
    run az rest --method PUT --resource https://management.azure.com \
        --url "https://management.azure.com$1/providers/Microsoft.Authorization/roleAssignments/$name?api-version=$ROLE_API" \
        --body "{\"properties\":{\"roleDefinitionId\":\"$2\",\"principalId\":\"$3\",\"principalType\":\"$4\"}}"
}

# Say what actually refused. Reporting every failure as a missing permission would send an
# operator who already holds Owner hunting for a role they already have; a Conditional
# Access refusal looks nothing like a permissions problem and is not fixed
# by granting anything.
ROLE_ERR=""
try_role_assign() {   # <scope> <role-definition-id> <principal-id> <principal-type>
    if (( DRY_RUN )); then role_assign "$@" >/dev/null; return 0; fi
    local rc=0
    role_assign "$@" >/dev/null 2>"$ERR_FILE" || rc=$?
    (( rc == 0 )) && return 0
    ROLE_ERR="$(last_err)"
    return 1
}

# Cause/Fix for a failed try_role_assign. $1 = role, $2 = who it was for.
explain_role_failure() {
    if grep -qiE 'AADSTS|Conditional Access' <<<"$ROLE_ERR"; then
        cause "a sign-in (Conditional Access) policy refused this machine a token -- this is NOT a missing role: $ROLE_ERR"
        fix "run this script from a device your organisation's sign-in policy allows; the roles you hold do not matter here"
    elif grep -qiE 'AuthorizationFailed|does not have authorization' <<<"$ROLE_ERR"; then
        cause "the signed-in account ($signed_in) is not allowed to grant roles on Key Vault $KV_NAME"
        fix "ask someone with Owner or User Access Administrator on the vault to grant '$1' to $2 on $KV_ARM_ID, or to give you one of those roles" \
            "run this script again:  $(rerun_cmd)"
    else
        cause "$ROLE_ERR"
        fix "run this script again:  $(rerun_cmd)"
    fi
}

# -- 3a: make sure THIS operator can reach the vault's data plane -------------
#
# `az ad signed-in-user` works for a human login and fails for a service-principal one, so
# fall back to resolving the SP behind the current account.
CALLER_OID="$(az ad signed-in-user show --query id -o tsv 2>/dev/null || true)"
CALLER_PTYPE="User"
if [[ -z "$CALLER_OID" ]]; then
    caller_appid="$(az account show --query user.name -o tsv 2>/dev/null || true)"
    if [[ -n "$caller_appid" ]]; then
        CALLER_OID="$(az ad sp show --id "$caller_appid" --query id -o tsv 2>/dev/null || true)"
        CALLER_PTYPE="ServicePrincipal"
    fi
fi

# Both lookups above go to Microsoft Graph, which a tenant can withhold from a login that
# reaches ARM perfectly well -- and a role assignment needs no Graph at all, only the caller's
# object id. That id is already a claim in the ARM token this script holds, so read it from
# there instead of giving up. 'idtyp' separates a user login from an app one.
if [[ -z "$CALLER_OID" ]]; then
    # The token is a JWT: three base64url segments, the middle one the claims. az hands over
    # the token itself with --query, and base64/grep do the rest -- `oid` is a GUID and
    # `idtyp` a short word, so neither can contain a quote to confuse the match.
    tok="$(az account get-access-token --query accessToken -o tsv 2>/dev/null || true)"
    claims_payload=""
    if [[ "$tok" == *.*.* ]]; then
        seg="${tok#*.}"
        seg="${seg%%.*}"
        case $(( ${#seg} % 4 )) in
            2) seg="${seg}==" ;;
            3) seg="${seg}=" ;;
        esac
        claims_payload="$(printf '%s' "$seg" | tr '_-' '/+' | base64 -d 2>/dev/null || true)"
    fi
    if [[ -n "$claims_payload" ]]; then
        tok_oid="$(grep -o '"oid"[[:space:]]*:[[:space:]]*"[^"]*"' <<<"$claims_payload" \
            | head -n1 | sed 's/.*"\([^"]*\)"$/\1/')"
        if [[ -n "$tok_oid" ]]; then
            CALLER_OID="$tok_oid"
            if grep -q '"idtyp"[[:space:]]*:[[:space:]]*"user"' <<<"$claims_payload" \
               || grep -q '"upn"[[:space:]]*:' <<<"$claims_payload"; then
                CALLER_PTYPE="User"
            else
                CALLER_PTYPE="ServicePrincipal"
            fi
            note "resolved the caller from the access token; Microsoft Graph was not reachable"
        fi
    fi
fi

# Probe the data plane instead of reading role assignments: the operator may already hold
# access through a group or an inherited grant, and adding one on top would leave behind a
# standing permission nobody asked for.
kv_dataplane_ok() {
    az keyvault key list --vault-name "$KV_NAME" --maxresults 1 >/dev/null 2>&1
}

# Neither authorisation system is read-your-writes on the data plane: the vault keeps refusing
# for a while after a grant lands, whether that grant was a role assignment or an access
# policy. Both grant paths below therefore wait the same two minutes rather than racing
# straight into the key create -- one definition, so the two can never drift apart.
#
# A dry run makes no grant, so there is nothing to wait for and nothing to report as a
# timeout: it succeeds immediately rather than sleeping for two minutes to fail.
wait_for_kv_dataplane() {
    (( DRY_RUN )) && return 0
    local _attempt
    for _attempt in $(seq 1 24); do
        kv_dataplane_ok && return 0
        sleep 5
    done
    return 1
}

# A vault has two independent authorisation systems for its DATA plane and honours exactly
# one of them, chosen by properties.enableRbacAuthorization. Every grant this script makes on
# the vault is a CONTROL-plane write, so ARM accepts it either way -- a vault left on access
# policies simply never reads that table back. That is the whole reason a vault can answer 403
# to the operator seconds after a role assignment reported success, and why waiting longer
# never helps.
#
# The operator half of this is at least loud: kv_dataplane_ok probes the vault and the run
# stops. The Arc MI half in 3c is the dangerous one -- this script holds the operator's token
# and can never hold the MI's, so it cannot probe that access at all. Granting RBAC there on a
# vault that ignores RBAC, then printing success, makes a claim this script is in no position
# to make and defers the failure to the agent's first wrapKey, far from here.
#
KV_API_VER=2023-07-01
KV_RBAC=1
kv_arm_url="https://management.azure.com${KV_ARM_ID}?api-version=${KV_API_VER}"
if (( ${KV_CREATED_BY_PREFLIGHT:-0} )); then
    # Preflight created this vault moments ago with --enable-rbac-authorization false, so the
    # answer is already known. Asserting it here rather than re-reading matters for a dry run,
    # where the vault does not exist, the ARM read cannot succeed, and the default of 1 would
    # otherwise walk the rehearsal through the RBAC branch for a vault that will be created in
    # access-policy mode -- previewing role assignments that a real run would never make.
    KV_RBAC=0
    note "$KV_NAME was created in ACCESS-POLICY mode, so Key Vault RBAC roles do not apply"
else
    if ! kv_auth_mode="$(az rest --method get --url "$kv_arm_url" \
        --query properties.enableRbacAuthorization -o tsv 2>"$ERR_FILE")"; then
        bad "could not read how Key Vault $KV_NAME controls access (RBAC or access policies); nothing was granted"
        cause "$(last_err)"
        fix "check you can read the vault:  az keyvault show --subscription $KV_SUBSCRIPTION -g $KV_RG -n $KV_NAME" \
            "run this script again:  $(rerun_cmd)"
        exit 1
    fi
    case "${kv_auth_mode,,}" in
        true) KV_RBAC=1 ;;
        false)
            KV_RBAC=0
            note "$KV_NAME authorises its data plane through ACCESS POLICIES, not Azure RBAC" ;;
        *)
            bad "Key Vault $KV_NAME reported an unexpected access mode ('$kv_auth_mode'); nothing was granted"
            cause "Azure returned a value this script does not recognise (this is a script defect, not a problem with your vault)"
            fix "send this output to the extension provider"
            exit 1 ;;
    esac
fi

# The access-policy counterpart of a role-assignment lookup. It reads the vault resource, which
# is control plane, so unlike kv_dataplane_ok it answers for ANY principal -- including one this
# script could never authenticate as.
kv_policy_keys() { # $1 = objectId -> that principal's key permissions, one per line, lowercased
    az rest --method get --url "$kv_arm_url" \
        --query "properties.accessPolicies[?objectId=='$1'].permissions.keys[]" -o tsv 2>/dev/null \
        | tr '[:upper:]' '[:lower:]'
}

kv_policy_has() { # $1 = objectId, $2.. = required key permissions
    local perms oid="$1" need; shift
    perms="$(kv_policy_keys "$oid")"
    [[ -n "$perms" ]] || return 1
    grep -qx all <<<"$perms" && return 0
    for need in "$@"; do
        grep -qx "$(tr '[:upper:]' '[:lower:]' <<<"$need")" <<<"$perms" || return 1
    done
    return 0
}

# Grants a minimal key access policy -- or stops and says exactly what to run, on a vault this
# script does not own.
#
# The split matters. When preflight created the vault it is the owner, nobody else has a policy
# on it, and writing one is the access-policy equivalent of the role assignment the RBAC branch
# makes without asking; refusing to do it there would just make the operator paste back a
# command the script already knows. On a vault somebody else created, a set-policy is a change
# to a resource with other principals on it, so it stays the operator's call.
#
# What is deliberately NOT offered either way is "switch the vault to Azure RBAC". The
# switch leaves the accessPolicies array completely untouched and merely
# stops honouring it, so nothing is destroyed and it reverses cleanly -- but every principal
# holding one of those policies loses data-plane access the instant it is flipped, and on a
# vault this script did not create that principal is somebody it has never heard of.
kv_policy_grant() { # $1 = objectId, $2 = who, $3.. = key permissions to grant
    local oid="$1" who="$2"; shift 2
    local perms="$*"
    local cmd="az keyvault set-policy -n $KV_NAME -g $KV_RG --subscription $KV_SUBSCRIPTION --object-id $oid --key-permissions $perms"

    if (( DRY_RUN )); then
        if (( ${KV_CREATED_BY_PREFLIGHT:-0} )); then
            did "would grant $who key permissions '$perms' on $KV_NAME via access policy"
        else
            soft "$who has no access policy on $KV_NAME -- a real run would STOP here"
        fi
        note "  $cmd"
        return 0
    fi

    if (( ${KV_CREATED_BY_PREFLIGHT:-0} )); then
        did "granting $who key permissions '$perms' on $KV_NAME via access policy"
        # shellcheck disable=SC2086 -- $perms is a deliberate word-split list of permissions
        if ! az keyvault set-policy -n "$KV_NAME" -g "$KV_RG" --subscription "$KV_SUBSCRIPTION" \
                --object-id "$oid" --key-permissions $perms >/dev/null 2>"$ERR_FILE"; then
            bad "could not give $who access to Key Vault $KV_NAME (access policy)"
            cause "$(last_err)"
            fix "run this script again (the vault is reused):  $(rerun_cmd)" \
                "if it keeps failing, ask the vault's owner to run:  $cmd"
            exit 1
        fi
        # Verify against the resource rather than trusting the exit code. This read is control
        # plane, so it answers for the Arc MI too -- the one principal this script holds no
        # token for and could never probe through the data plane.
        if kv_policy_has "$oid" $perms; then
            okdid "access policy in place for $who"
        else
            bad "the access policy for $who was accepted but Key Vault $KV_NAME does not show it"
            note "  expected key permissions '$perms' for object id $oid"
            cause "Azure has not applied the change yet, or another process overwrote the vault's policies at the same time"
            fix "run this script again in a minute:  $(rerun_cmd)"
            exit 1
        fi
        return 0
    fi

    bad "$who has no access to Key Vault $KV_NAME"
    cause "$KV_NAME uses access policies, so Azure RBAC roles are ignored on it; and it was not created by this script, so the script does not change its policies for you"
    fix "ask the vault's owner to run:  $cmd" \
        "run this script again:  $(rerun_cmd)"
    note "switching the vault to Azure RBAC is the vault owner's call, not a workaround."
    note "  the switch keeps the access policies on the resource but stops enforcing them,"
    note "  so every principal that relies on one loses access until it holds an equivalent role."
    note "  if the owner does switch, those roles must be granted BEFORE flipping."
    exit 1
}

# The probe is a read, so it is safe in a dry run as well -- and skipping the whole chain there
# hid the Crypto Officer grant from the preview completely. That grant is a standing permission
# that outlives the script, which makes it exactly the kind of thing a rehearsal must disclose.
if kv_dataplane_ok; then
    skip "operator already has Key Vault data-plane access on $KV_NAME"
elif [[ -z "$CALLER_OID" ]] && (( DRY_RUN )); then
    soft "the signed-in identity could not be resolved -- a real run would STOP here"
elif [[ -z "$CALLER_OID" ]]; then
    bad "could not identify the signed-in account, so it cannot be given access to Key Vault $KV_NAME"
    # Say what actually refused. Discarding this is how the earlier version presented a
    # Graph outage as an unexplained inability to identify a perfectly valid login.
    az ad signed-in-user show --query id -o tsv >/dev/null 2>"$ERR_FILE" || true
    cause "Microsoft Graph and the access token both failed to give its object id: $(last_err)"
    fix "ask the vault's owner to give you key permissions 'get create list' on $KV_ARM_ID ('$ROLE_KV_CRYPTO_OFFICER' if it uses Azure RBAC)" \
        "run this script again:  $(rerun_cmd)"
    exit 1
elif (( ! KV_RBAC )); then
    # Reached only because the probe above already found no access. Roles are ignored on this
    # vault, so grant the policy equivalent of Crypto Officer: 'get create list' on keys and
    # nothing else -- no delete, no purge, no restore, and no secret or certificate access.
    # That is the minimum 3b needs to create the wrapping key.
    kv_policy_grant "$CALLER_OID" "the operator" get create list
    # A policy write is control plane and the data plane picks it up separately, so the same
    # read-your-writes gap that affects RBAC applies here too -- just far shorter in practice.
    # Poll rather than racing straight into the key create below. On a vault preflight did not
    # create, kv_policy_grant has already exited, so this is only ever reached after a grant
    # this script actually made.
    if wait_for_kv_dataplane; then
        okdid "operator Key Vault access granted"
    else
        bad "Key Vault $KV_NAME still refuses your account two minutes after access was granted"
        cause "most likely the vault's network rules (firewall or private endpoint) block this machine -- they refuse access no matter which permissions are held"
        fix "check the vault's networking:  az keyvault show -n $KV_NAME -g $KV_RG --query properties.networkAcls" \
            "allow this machine (or its network), then run this script again:  $(rerun_cmd)"
        exit 1
    fi
else
    did "granting $ROLE_KV_CRYPTO_OFFICER to the operator on $KV_NAME (needed to create the key)"
    note "this role assignment stays on the vault after the script finishes"
    officer_def="$(role_def_id "$ROLE_KV_CRYPTO_OFFICER" "$KV_ARM_ID")"
    if [[ -z "$officer_def" ]]; then
        bad "could not look up the built-in role '$ROLE_KV_CRYPTO_OFFICER'"
        az role definition list --name "$ROLE_KV_CRYPTO_OFFICER" --scope "$KV_ARM_ID" --query '[0].id' -o tsv >/dev/null 2>"$ERR_FILE" || true
        cause "$(last_err)"
        fix "run this script again:  $(rerun_cmd)"
        exit 1
    fi
    if ! try_role_assign "$KV_ARM_ID" "$officer_def" "$CALLER_OID" "$CALLER_PTYPE"; then
        bad "could not give your account ($signed_in) access to Key Vault $KV_NAME"
        explain_role_failure "$ROLE_KV_CRYPTO_OFFICER" "$signed_in"
        exit 1
    fi
    # Data-plane RBAC is not read-your-writes: the vault keeps refusing for a while after the
    # assignment lands, so poll rather than racing straight into the key create below.
    if wait_for_kv_dataplane; then
        okdid "operator Key Vault access granted"
    else
        bad "Key Vault $KV_NAME still refuses your account two minutes after the role was granted"
        cause "Azure RBAC changes can take a few minutes to reach Key Vault; if it persists, the vault's network rules (firewall or private endpoint) are blocking this machine"
        fix "wait a few minutes, then run this script again (the grant is kept):  $(rerun_cmd)" \
            "if it still fails, check:  az keyvault show -n $KV_NAME -g $KV_RG --query properties.networkAcls"
        exit 1
    fi
fi

# -- 3b: always create a new, uniquely named RSA wrapping key ----------------

create_fresh_rsa_key() {
    local suffix key_uri
    if [[ ! "$KEY_NAME" =~ ^[a-zA-Z0-9-]+$ || ${#KEY_NAME} -gt 90 ]]; then
        bad "--key-name '$KEY_NAME' is not a valid key name prefix"
        cause "it must be 1-90 characters of letters, digits and hyphens"
        fix "run this script again with a valid --key-name, or without it"
        return 1
    fi
    suffix="$(guid_v4)"
    if [[ ! "$suffix" =~ ^[0-9a-fA-F-]{36}$ ]]; then
        bad "could not generate a unique name for the new Key Vault key"
        cause "none of /proc/sys/kernel/random/uuid, uuidgen or /dev/urandom produced a unique id"
        fix "install util-linux (it provides uuidgen), then run this script again:  $(rerun_cmd)"
        return 1
    fi
    KEY_NAME="${KEY_NAME}-${suffix}"
    note "new RSA key name: $KEY_NAME; existing vault keys will not be reused, recovered or deleted"
    if (( DRY_RUN )); then
        run az keyvault key create --vault-name "$KV_NAME" --name "$KEY_NAME" \
            --kty RSA --size 3072 --ops wrapKey unwrapKey
        return
    fi
    if ! key_uri="$(az keyvault key create --vault-name "$KV_NAME" --name "$KEY_NAME" \
        --kty RSA --size 3072 --ops wrapKey unwrapKey --query key.kid -o tsv 2>"$ERR_FILE")"; then
        bad "could not create the encryption key $KEY_NAME in Key Vault $KV_NAME"
        key_err="$(last_err)"
        case "$key_err" in
            *Forbidden*|*"does not have"*|*ForbiddenByPolicy*|*ForbiddenByRbac*)
                cause "your account was refused by the vault: $key_err"
                fix "make sure you have key permissions 'get create list' on $KV_NAME, then run this script again:  $(rerun_cmd)" ;;
            *ForbiddenByConnection*|*"Public network access"*|*"not authorized to access this resource"*)
                cause "the vault's network rules block this machine: $key_err"
                fix "allow this machine in the vault's Networking settings, then run this script again:  $(rerun_cmd)" ;;
            *)
                cause "$key_err"
                fix "run this script again (a new key name is generated each run):  $(rerun_cmd)" ;;
        esac
        return 1
    fi
    if [[ "$key_uri" != "${KV_URI%/}/keys/$KEY_NAME/"* || -z "${key_uri##*/}" ]]; then
        bad "Key Vault returned an unexpected identifier for the new key: ${key_uri:-<empty>}"
        cause "the key id does not start with ${KV_URI%/}/keys/$KEY_NAME/ (this is a script defect, not a problem with your vault)"
        fix "send this output to the extension provider"
        return 1
    fi
    okdid "RSA key created: $KEY_NAME"
}

create_fresh_rsa_key || exit 1

# -- 3c: grant the Arc MI wrap/unwrap on the key -------------------------------
#
# Additive only. Preflight never removes a grant: it cannot tell one it created from one the
# customer's own policy put there, and deleting the wrong one would break access it knows
# nothing about. Any broader grant that already exists on the vault (for example Crypto
# Officer) is left in place; removing it is an operator decision, not this script's.
user_def=""
if (( ! KV_RBAC )); then
    # On a vault that ignores roles the only thing worth asserting is whether a POLICY gives
    # the MI what it needs. That read is control plane, so it answers for a principal this
    # script cannot authenticate as -- which turns the one grant nobody could ever verify into
    # the one grant that is checked rather than assumed.
    if kv_policy_has "$ARC_MI_PRINCIPAL_ID" get wrapKey unwrapKey; then
        skip "Arc MI already has key wrap/unwrap on $KV_NAME through an access policy"
    else
        kv_policy_grant "$ARC_MI_PRINCIPAL_ID" "the Arc machine's managed identity" get wrapKey unwrapKey
    fi
else
    user_def="$(role_def_id "$ROLE_KV_CRYPTO_USER" "$KV_ARM_ID")"
    if [[ -z "$user_def" ]]; then
        bad "could not look up the built-in role '$ROLE_KV_CRYPTO_USER'"
        az role definition list --name "$ROLE_KV_CRYPTO_USER" --scope "$KV_ARM_ID" --query '[0].id' -o tsv >/dev/null 2>"$ERR_FILE" || true
        cause "$(last_err)"
        fix "run this script again:  $(rerun_cmd)"
        exit 1
    fi
    existing="$(role_assignment_id "$KV_ARM_ID" "$user_def" "$ARC_MI_PRINCIPAL_ID")"

    if [[ -n "$existing" ]]; then
        skip "Arc MI already has $ROLE_KV_CRYPTO_USER on $KV_NAME"
    else
        did "granting $ROLE_KV_CRYPTO_USER to Arc MI on $KV_NAME"
        if ! try_role_assign "$KV_ARM_ID" "$user_def" "$ARC_MI_PRINCIPAL_ID" ServicePrincipal; then
            bad "could not give this machine's managed identity access to Key Vault $KV_NAME"
            explain_role_failure "$ROLE_KV_CRYPTO_USER" "the Arc machine's managed identity ($ARC_MI_PRINCIPAL_ID)"
            exit 1
        fi
        okdid "role assigned"
    fi
fi

# -- 3e: record the vault on the discovery solution ----------------------------------------
#
# Step 2 reads properties.details.extendedDetails.keyVaultId off the discovery solution and
# reuses the vault it names. Nothing on this path ever WRITES it: extendedDetails is populated
# by the Azure Migrate service when a portal flow creates a vault,
# and an ARM resource create does not trigger it. So a vault preflight created stays invisible
# to the project, and the gap is not cosmetic:
#
#   * the next machine onboarded against this project does not see the vault, falls through to
#     find_kv_by_tag, and avoids creating a duplicate only because of OUR tag -- which does not
#     survive someone passing a different --keyvault-resource-group.
#   * the portal shows the project as having no vault, so an operator can be led into creating
#     a second one behind this script's back.
#
# Writing both keys closes that. The service writes keyVaultId AND keyVaultUrl, so this writes
# both; a consumer reading only the URL would otherwise see a project with a vault id and no
# URL, a shape the service never produces.
#
# WHY THE BODY CARRIES ONLY OUR TWO KEYS -- behaviour of the discovery solution
# (Servers-Discovery-ServerDiscovery, api-version 2020-05-01):
#
#   * PATCH merges extendedDetails PER KEY. A body containing one key left masterSiteId, and
#     every sibling under properties (tool, status, summary, groupCount, assessmentCount),
#     untouched. So a minimal body cannot destroy the master-site pointer Step 5 depends on,
#     and re-sending the whole map would be actively worse: on a project with an appliance that
#     map holds appliance certificates, registration state and per-discovery error blobs the
#     service rewrites constantly, and echoing a stale snapshot back could roll them back.
#   * If-Match is IGNORED. A stale etag still returns 200 and the write lands, so
#     there is no optimistic concurrency to lean on here. Per-key merge is what makes that
#     acceptable: this writes only keys the service is not concurrently touching.
#   * The PATCH RESPONSE ECHOES THE OLD ETAG even though the resource's etag changed, so the
#     verification below re-reads the resource instead of trusting the response.
#   * extendedDetails is ADDITIVE-ONLY: neither null nor omission removes a key. There is no
#     undo for this write, which is why it never overwrites a vault somebody else recorded.
#
# Every failure here is a WARN, never a FAIL. The pointer is a convenience for the next run and
# for the portal; the agent reads its vault from the extension settings written in Step 6 and
# never consults the solution. An operator without Microsoft.Migrate/.../solutions/write must
# still be able to finish an otherwise valid install.
link_kv_to_discovery_solution() {
    local linked body url ver patched kv_url patch_err=""

    refresh_discovery_solution
    load_discovery_solution
    if (( ! SOLUTION_READ_OK )); then
        soft "could not read the discovery solution's current Key Vault link, so it was not updated (no PATCH was sent)"
        explain_solution_read_failure
        note "not fatal: the agent reads its vault from the extension settings, not the solution."
        return 0
    fi
    linked="$(solution_extended_detail keyVaultId)"
    if [[ -n "$linked" && "${linked,,}" == "${KV_ARM_ID,,}" ]]; then
        skip "discovery solution Key Vault link is up to date ($KV_NAME); skipping PATCH for this run"
        return 0
    fi
    if [[ -n "$linked" ]]; then
        # Only reachable when --keyvault-arm-id or --keyvault-name steered away from the linked
        # vault, because resolve_kv_from_project would otherwise have selected it. The write has
        # no undo, and repointing a project at a different vault is not this script's call.
        soft "the Azure Migrate project already records a DIFFERENT Key Vault; leaving that record untouched"
        note "  recorded by the project: $linked"
        note "  used by this install:    $KV_ARM_ID"
        cause "--keyvault-arm-id or --keyvault-name chose a vault other than the project's own"
        fix "nothing is needed for the agent (it uses $KV_NAME from the extension settings). To use the project's vault instead, run this script again without --keyvault-arm-id / --keyvault-name."
        return 0
    fi

    # The service stores the vault URL without a trailing slash; ARM's properties.vaultUri has
    # one. Match the service so this project looks like every other project.
    kv_url="${KV_URI%/}"
    # Built with printf rather than a JSON library: both values are machine-generated and
    # cannot contain a character that needs escaping -- an ARM id is [A-Za-z0-9._/-] and the
    # vault URI is an https host. The guard below is what makes that an assertion instead of
    # an assumption, so a future change that routes operator input through here fails loudly
    # rather than emitting broken JSON.
    if [[ "$KV_ARM_ID$kv_url" == *[\"\\]* ]]; then
        soft "the vault id or URL contains a quote or backslash; the vault was not recorded"
        return 0
    fi
    body="$(printf '{"properties": {"details": {"extendedDetails": {"keyVaultId": "%s", "keyVaultUrl": "%s"}}}}' \
        "$KV_ARM_ID" "$kv_url")"
    if [[ -z "$body" ]]; then
        soft "could not build the solution patch body; the vault was not recorded"
        return 0
    fi

    url="https://management.azure.com${MIGRATE_PROJECT_ID}/solutions/Servers-Discovery-ServerDiscovery"
    if (( DRY_RUN )); then
        did "would record $KV_NAME on the discovery solution (keyVaultId + keyVaultUrl)"
        note "  az rest --method patch --url '${url}?api-version=${SOLUTIONS_API_VER}' --body '$body'"
        return 0
    fi

    did "recording $KV_ARM_ID on the discovery solution"
    patched=0
    # PATCH is specified only in the 2018-09-01-preview contract, but the RP serves it on the GA
    # version too -- that is the one described above. Try GA first and keep the preview version as
    # a fallback so a service that only honours the contract version is not reported as a failure.
    for ver in "$SOLUTIONS_API_VER" 2018-09-01-preview; do
        if az rest --method patch --url "${url}?api-version=${ver}" \
                --headers "Content-Type=application/json" --body "$body" >/dev/null 2>"$ERR_FILE"; then
            patched=1
            break
        fi
        [[ "$ver" == "$SOLUTIONS_API_VER" ]] && patch_err="$(last_err)"
    done

    if (( ! patched )); then
        soft "could not record the Key Vault on the Azure Migrate project (discovery solution)"
        case "${patch_err:-}" in
            *AuthorizationFailed*|*"does not have authorization"*)
                cause "the signed-in account ($signed_in) is not allowed to update the Migrate project's solutions" ;;
            *)
                cause "$patch_err" ;;
        esac
        note "not fatal: the agent reads its vault from the extension settings, not the solution."
        fix "to record it later, as an account with Contributor on the project:  az rest --method patch --url '${url}?api-version=${SOLUTIONS_API_VER}' --body '$body'"
        return 0
    fi

    # Re-read the resource. A 200 says the RP accepted the body, not that it kept the key, and
    # the response cannot be trusted for this -- it echoes the pre-write etag. Dropping the
    # snapshot first is what makes this a real read: without it the reader would be served the
    # pre-PATCH value and this check would always report failure. The RP can serve the old
    # document for a few seconds after the write, so a mismatch is retried before it is reported.
    local waited=0
    while :; do
        refresh_discovery_solution
        load_discovery_solution
        linked=""
        (( SOLUTION_READ_OK )) && linked="$(solution_extended_detail keyVaultId)"
        [[ -n "$linked" && "${linked,,}" == "${KV_ARM_ID,,}" ]] && break
        (( waited >= SOLUTION_READBACK_WAIT_SECONDS )) && break
        sleep "$READBACK_POLL_SECONDS"; waited=$((waited + READBACK_POLL_SECONDS))
    done
    if [[ -n "$linked" && "${linked,,}" == "${KV_ARM_ID,,}" ]]; then
        okdid "discovery solution now records $KV_NAME"
    elif (( ! SOLUTION_READ_OK )); then
        soft "the project accepted the Key Vault record, but it could not be verified because reading it back failed"
        explain_solution_read_failure
        note "the record may have applied; it is not sent again blindly. A later run re-reads it and records it only if it is still missing."
        note "not fatal: the agent reads its vault from the extension settings, not the solution."
    else
        soft "the project accepted the Key Vault record, but still does not show it after ${waited}s"
        note "  expected: $KV_ARM_ID"
        note "  reported: ${linked:-<empty>}"
        cause "the Azure Migrate service has not applied the update yet, or did not keep it"
        fix "not fatal for the agent. Running this script again later re-checks it and records it if it is still missing."
    fi
    return 0
}

if (( LINK_KV_TO_SOLUTION )); then
    link_kv_to_discovery_solution
else
    note "--no-solution-link given: the vault was not recorded on the discovery solution"
fi

# --- Step 4: Resolve the host identity -------------------------------------

head1 "Step 4 -- Resolve the Arc host identity"

ARC_MACHINE_RESOURCE_ID="$(az connectedmachine show --subscription "$ARC_SUBSCRIPTION" -g "$ARC_RG" -n "$ARC_VM" --query id -o tsv 2>"$ERR_FILE" || true)"

# A dry run never onboards, so on a machine that is not Arc-connected yet this
# lookup legitimately returns nothing. Predict the ARM id from the validated
# arguments so the Step 5 PUT preview never shows a blank hostIdentifier.
if [[ -z "$ARC_MACHINE_RESOURCE_ID" ]]; then
    if (( DRY_RUN )); then
        ARC_MACHINE_RESOURCE_ID="/subscriptions/$ARC_SUBSCRIPTION/resourceGroups/$ARC_RG/providers/Microsoft.HybridCompute/machines/$ARC_VM"
        note "dry run: Arc machine record does not exist; predicting $ARC_MACHINE_RESOURCE_ID"
    else
        bad "could not read the Arc machine '$ARC_VM' in resource group '$ARC_RG'"
        cause "$(last_err)"
        fix "make sure your account has at least Reader on /subscriptions/$ARC_SUBSCRIPTION/resourceGroups/$ARC_RG" \
            "run this script again:  $(rerun_cmd)"
        exit 1
    fi
fi

# ARM name contract: sanitizedHostName-<8 random GUID characters>-executionAgent.
# The agent consumes this name from settings; it never regenerates it. The hostIdentifier
# remains the Arc machine ARM id. Generate a name only after checking its registrations.
derive_execution_agent_id() {
    local host guid
    if ! host="$(hostname)"; then
        {
            bad "could not read this machine's hostname (needed to name the Execution Agent)"
            cause "the 'hostname' command failed"
            fix "pass a name explicitly, e.g.  $(rerun_cmd '--execution-agent-id <name>-executionAgent')"
        } >&2
        return 1
    fi
    host="$(LC_ALL=C tr '[:upper:]' '[:lower:]' <<<"$host" \
        | LC_ALL=C sed -E 's/[^a-z0-9]+/-/g; s/^-+//; s/-+$//')"
    if [[ -z "$host" ]]; then
        {
            bad "this machine's hostname cannot be used to name the Execution Agent"
            cause "the hostname contains no letters or digits"
            fix "pass a name explicitly, e.g.  $(rerun_cmd '--execution-agent-id <name>-executionAgent')"
        } >&2
        return 1
    fi
    if ! guid="$(guid_v4)" \
        || [[ ! "$guid" =~ ^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$ ]]; then
        {
            bad "could not generate a unique suffix for the Execution Agent name"
            cause "none of /proc/sys/kernel/random/uuid, uuidgen or /dev/urandom produced a unique id"
            fix "install util-linux (it provides uuidgen), then run this script again:  $(rerun_cmd)"
        } >&2
        return 1
    fi
    printf '%s-%s-executionAgent' "$host" "${guid:0:8}"
}

select_execution_agent_id() {
    if [[ -n "$EXECUTION_AGENT_ID_OVERRIDE" ]]; then
        EXECUTION_AGENT_ID="$EXECUTION_AGENT_ID_OVERRIDE"
        note "executionAgentId pinned by --execution-agent-id $EXECUTION_AGENT_ID"
    else
        EXECUTION_AGENT_ID="$(derive_execution_agent_id)" || return 1
        note "candidate executionAgentId $EXECUTION_AGENT_ID"
    fi
}

# --- Step 5: Register executionAgent on master site ------------------------

head1 "Step 5 -- Register executionAgent on master site"

# Check all existing registrations by hostIdentifier before generating a candidate or PUT.
# A pre-existing name is never overwritten. Reuse requires explicit consent; the duplicate-host
# error path below handles a registration created concurrently after the initial check.
#
# PUT body is UNWRAPPED: the swagger types the body as ExecutionAgentProperties,
# so hostIdentifier and agentAuthenticationSpn sit at the top level. The usual
# ARM {"properties":{...}} envelope is rejected with a generic 400
# InvalidParameter that names no field. Both are required on write, and
# agentAuthenticationSpn itself requires tenantId, applicationId and objectId.
# The GET response IS enveloped, so never copy one straight back into a PUT.
#
# Master site comes from the discovery solution at
# properties.details.extendedDetails.masterSiteId -- the service's own link, so
# it is authoritative. The name-based lookup below is only a fallback for a
# project with no master site yet; master-site names take three inconsistent
# shapes in the wild and can be ambiguous, in which case --master-site-id is
# required.
#
# discoveryServiceEndpoint is NOT on this resource. It is regional and is read
# from a sibling site's properties.serviceEndpoint instead.
#
# Identity: the operator's own `az login`. The Arc MI deliberately holds only
# Key Vault Crypto User and never talks to ARM. No executionAgents/write
# operation is published, so only wildcard roles reach this PUT ("Azure Migrate
# Owner" or Contributor); "Azure Migrate Execute Expert" is read-only.
#
# A missing record 404s as RunAsAccountNotFound, because the RP serves this
# unlisted preview child type through its RunAsAccounts handler.
#
# Must stay on the preview api-version: the executionAgents child path exists
# only in 2025-12-12-preview and returns InvalidHttpRequestPath on every GA
# version and on 2024-12-01-preview. Revisit when the type GAs.
API_VER="2025-12-12-preview"

# Dataplane (agentrestapi/*) version, emitted into the extension settings so the
# handler does not hardcode it when it seeds execution.json. Versioned separately
# from the ARM control plane above and NOT covered by the published ARM contract;
# this value must match the dataplane version the agent uses.
DATAPLANE_API_VER="2024-12-12-preview"


# One GET supplies the project-link check, PATCH body and preservation snapshot.
# Header: properties type, project-id type, project id. Row 2: JSON PATCH body.
# Remaining rows: path<TAB>JSON-array-wrapped value. Wrapping preserves JSON types
# even for strings, and keeps tabs/newlines escaped inside one TSV cell.
ms_read() {
    local url="$1" raw query properties wrap
    properties="not_null(properties, \`{}\`)"
    wrap="&to_string(not_null([@], \`[null]\`))"
    query="[[type(properties), type(properties.migrateProjectId), properties.migrateProjectId || ''],"
    query+="[to_string(merge({properties: merge($properties, {migrateProjectId: '$MIGRATE_PROJECT_ID'})},"
    query+="contains(keys(@), 'tags') && {tags: tags} || \`{}\`))],"
    query+="keys(@), map($wrap, values(@)), keys($properties), map($wrap, values($properties))]"
    raw="$(az rest --method get --url "$url" \
        --query "$query" \
        -o tsv 2>"$ERR_FILE")" || return 1
    [[ -n "$raw" ]] || return 1
    printf '%s\n' "$raw" | awk -F'\t' '
        NR == 1 { header = $0; next }
        NR == 2 { body = $0; next }
        NR == 3 { tn = NF; for (i = 1; i <= NF; i++) tk[i] = $i; next }
        NR == 4 { vn = NF; for (i = 1; i <= NF; i++) tv[i] = $i; next }
        NR == 5 { pn = NF; for (i = 1; i <= NF; i++) pk[i] = $i; next }
        NR == 6 { for (i = 1; i <= NF; i++) pv[i] = $i }
        END {
            if (tn == 0 || tn != vn || body == "") exit 1
            print header
            print body
            for (i = 1; i <= tn; i++)
                if (tk[i] != "" && tk[i] != "properties" &&
                    tk[i] != "etag" && tk[i] != "eTag" &&
                    tk[i] != "provisioningState" && tk[i] != "systemData")
                    print "$." tk[i] "\t" tv[i]
            for (i = 1; i <= pn; i++)
                if (pk[i] != "" && pk[i] != "provisioningState" && pk[i] != "migrateProjectId")
                    print "$.properties." pk[i] "\t" pv[i]
        }'
}

# Let the CLI compare JSON values on the read-back. Object key order is irrelevant;
# array order, string contents and JSON types must still match exactly.
ms_verify_query() {
    local before="$1" query k value parent field present
    query="[[type(properties), type(properties.migrateProjectId), properties.migrateProjectId || '']"
    while IFS=$'\t' read -r k value; do
        [[ -n "$k" ]] || continue
        if [[ ! "$k" =~ ^\$\.(properties\.)?[A-Za-z_][A-Za-z0-9_]*$ || "$value" != \[*\] ]]; then
            printf 'Cannot compare master-site field: %s\n' "$k" >&2
            return 1
        fi
        # An omitted empty tag map has no configured tag values to lose.
        if [[ "$k" == '$.tags' && ( "$value" == '[{}]' || "$value" == '[null]' ) ]]; then
            query+=",['$.tags', ((tags == \`null\` || tags == \`{}\`) && 'unchanged') || 'altered']"
            continue
        fi
        parent="@"; field="${k#$.}"
        if [[ "$field" == properties.* ]]; then
            parent="properties"; field="${field#properties.}"
        fi
        value="${value//\`/\\\`}"
        present="contains(keys(not_null($parent, \`{}\`)), '$field')"
        query+=",['$k', ($present && ([${k#$.}] == \`$value\`) && 'unchanged') || ($present && 'altered') || 'removed']"
    done <<<"$before"
    printf '%s]\n' "$query"
}

# Fill only a missing/null/empty project link. Carry the full properties bag:
# a one-field PATCH request is rejected by the service.
# Preserve top-level tags explicitly; some handlers clear them when omitted.
# Read/rejected-write failures remain warnings; changed or unverifiable state
# after an accepted write stops setup without attempting an automatic repair.
patch_master_site_project() {
    local url resp snap cur before body query after dropped altered k verdict
    local properties_type project_type expected_count actual_count
    url="https://management.azure.com${MASTER_SITE_ID}?api-version=${API_VER}"

    if ! snap="$(ms_read "$url")"; then
        soft "could not read the discovery master site to check that it is linked to this Migrate project"
        cause "$(last_err)"
        note "this does not block setup: registering the Execution Agent below does not depend on it"
        return 0
    fi

    IFS=$'\t' read -r properties_type project_type cur <<<"$(printf '%s\n' "$snap" | head -1)"
    if [[ "$properties_type" != object || ( "$project_type" != string && "$project_type" != null ) ]]; then
        soft "the discovery master site returned data in an unexpected shape; it was left unchanged"
        cause "properties is '$properties_type' and migrateProjectId is '$project_type' (expected object / string)"
        note "this does not block setup: registering the Execution Agent below does not depend on it"
        return 0
    fi

    if [[ -n "$cur" ]]; then
        if [[ "$(tr '[:upper:]' '[:lower:]' <<<"$cur")" \
           == "$(tr '[:upper:]' '[:lower:]' <<<"$MIGRATE_PROJECT_ID")" ]]; then
            skip "master site already carries this migrateProjectId; properties are up to date, skipping PATCH for this run"
        else
            soft "the discovery master site is linked to a different Migrate project; it was left unchanged"
            note "  on the site: $cur"
            note "  this run:    $MIGRATE_PROJECT_ID"
            cause "the appliance's discovery belongs to the other project"
            fix "if this machine belongs to that project, run this script again with  --migrate-project-id $cur"
        fi
        return 0
    fi

    body="$(printf '%s\n' "$snap" | sed -n '2p')"
    before="$(printf '%s\n' "$snap" | tail -n +3)"
    if [[ -z "$before" ]] || ! query="$(ms_verify_query "$before")"; then
        soft "could not prepare the safety checks for updating the discovery master site; it was left unchanged"
        cause "the master site's current properties could not be snapshotted for comparison"
        note "this does not block setup: registering the Execution Agent below does not depend on it"
        return 0
    fi
    did "PATCH $url  (properties.migrateProjectId)"
    if (( DRY_RUN )); then
        echo "  DRY   body: the master site's own properties and existing tags, with migrateProjectId set to"
        echo "  DRY         $MIGRATE_PROJECT_ID"
        return 0
    fi

    if ! az rest --method patch --url "$url" --body "$body" >/dev/null 2>"$ERR_FILE"; then
        soft "could not link the discovery master site to this Migrate project"
        cause "$(last_err)"
        note "this does not block setup: registering the Execution Agent below does not depend on it"
        return 0
    fi

    # The service applies the PATCH asynchronously in some regions, so poll the read-back for a
    # short while before calling it empty.
    local waited=0
    while :; do
        if ! snap="$(az rest --method get --url "$url" --query "$query" -o tsv 2>"$ERR_FILE")"; then
            bad "the discovery master site was updated, but the change could not be verified: reading it back failed"
            cause "$(last_err)"
            fix "check the master site:  az rest --method get --url \"https://management.azure.com${MASTER_SITE_ID}?api-version=${API_VER}\"" \
                "if it carries migrateProjectId $MIGRATE_PROJECT_ID, run this script again:  $(rerun_cmd)"
            exit 1
        fi
        IFS=$'\t' read -r properties_type project_type cur <<<"$(printf '%s\n' "$snap" | head -1)"
        [[ -n "$cur" ]] && break
        (( waited >= SOLUTION_READBACK_WAIT_SECONDS )) && break
        sleep "$READBACK_POLL_SECONDS"
        waited=$((waited + READBACK_POLL_SECONDS))
    done
    after="$(printf '%s\n' "$snap" | tail -n +2)"
    expected_count="$(printf '%s\n' "$before" | wc -l)"
    actual_count="$(printf '%s\n' "$after" | wc -l)"
    if [[ -z "$after" || "$expected_count" -ne "$actual_count" ]]; then
        bad "the discovery master site was updated, but the change could not be verified: the read-back is incomplete"
        cause "expected $expected_count fields back, got $actual_count"
        fix "check the master site:  az rest --method get --url \"https://management.azure.com${MASTER_SITE_ID}?api-version=${API_VER}\"" \
            "send this output to the extension provider before running this script again"
        exit 1
    fi
    dropped=""; altered=""
    while IFS=$'\t' read -r k verdict; do
        case "$verdict" in
            unchanged) ;;
            removed) dropped="$dropped $k" ;;
            altered) altered="$altered $k" ;;
            *)
                bad "the discovery master site was updated, but the change could not be verified against its earlier state"
                cause "Azure returned the comparison result '$verdict' for $k"
                fix "check the master site:  az rest --method get --url \"https://management.azure.com${MASTER_SITE_ID}?api-version=${API_VER}\"" \
                    "send this output to the extension provider before running this script again"
                exit 1 ;;
        esac
    done <<<"$after"

    if [[ -n "$dropped$altered" ]]; then
        bad "linking the discovery master site changed other settings on it; stopping before anything else is written"
        [[ -n "$dropped" ]] && note "  removed:$dropped"
        [[ -n "$altered" ]] && note "  altered:$altered"
        cause "the service, or another update running at the same time, changed fields this script did not ask to change"
        fix "do NOT run this script again yet; save this output and send it to the extension provider (no automatic repair is attempted)"
        exit 1
    fi

    if [[ "$properties_type" != object || ( "$project_type" != string && "$project_type" != null ) ]]; then
        bad "the discovery master site was updated, but the change could not be verified: it reads back in an unexpected shape"
        cause "properties is '$properties_type' and migrateProjectId is '$project_type' (expected object / string)"
        fix "send this output to the extension provider before running this script again"
        exit 1
    fi
    if [[ -z "$cur" ]]; then
        soft "the discovery master site accepted the link but still shows no migrateProjectId after ${waited}s"
        cause "the service applies this change in the background and has not finished yet"
        note "this does not block setup. To confirm later:"
        note "  az rest --method get --url \"https://management.azure.com${MASTER_SITE_ID}?api-version=${API_VER}\" --query properties.migrateProjectId"
        return 0
    fi
    if [[ "${cur,,}" != "${MIGRATE_PROJECT_ID,,}" ]]; then
        bad "the discovery master site now shows a different Migrate project; stopping without another write"
        note "  on the site: $cur"
        note "  this run:    $MIGRATE_PROJECT_ID"
        cause "another process linked the site to a different project at the same time"
        fix "save this output and send it to the extension provider; the link is not overwritten automatically"
        exit 1
    fi

    okdid "master site linked to project $(basename "$MIGRATE_PROJECT_ID")"
    note "every other master-site property verified intact after the merge"
}


confirm_registration_reuse() {
    local reply=""
    # --yes covers tooling/refresh consent, not adoption of an existing service identity.
    if ! ( : </dev/tty ) 2>/dev/null; then
        note "no terminal for registration reuse consent; refusing reuse (even with --yes)"
        return 1
    fi
    read -r -p "        reuse this existing registration for this same physical host? [y/N] " reply </dev/tty || return 1
    [[ "${reply,,}" == y || "${reply,,}" == yes ]]
}

execution_registrations_for_host() {
    local base url row next entries entry name host machine extra matches="" seen=""
    local deadline=$((SECONDS + SETUP_WAIT_SECONDS))
    base="https://management.azure.com${MASTER_SITE_ID}/executionAgents"
    url="$base?api-version=${API_VER}"
    while [[ -n "$url" ]]; do
        if [[ "$url" != "$base?"* || "$seen" == *"|$url|"* ]] || (( SECONDS >= deadline )); then
            echo "invalid, repeated or timed-out executionAgent list page" >&2
            return 1
        fi
        seen="${seen}|${url}|"
        if ! row="$(timeout 30 az rest --method get --url "$url" \
            --query "[[nextLink || '-', join('|', value[].join('~', [name || '-', properties.hostIdentifier || '-', properties.agentAuthenticationSpn.machineResourceId || '-'])) || '-']]" \
            -o tsv)"; then
            return 1
        fi
        IFS=$'\t' read -r next entries extra <<<"${row//$'\r'/}"
        if [[ -z "$next" || -z "$entries" || -n "$extra" ]]; then
            echo "invalid executionAgent list response" >&2
            return 1
        fi
        if [[ "$entries" != "-" ]]; then
            while IFS= read -r entry; do
                IFS='~' read -r name host machine extra <<<"$entry"
                if [[ -z "$name" || -z "$host" || -z "$machine" || -n "$extra" ]]; then
                    echo "invalid executionAgent registration row" >&2
                    return 1
                fi
                if [[ "${host,,}" == "${ARC_MACHINE_RESOURCE_ID,,}" ]]; then
                    matches="${matches}${name}"$'\t'"${host}"$'\t'"${machine}"$'\n'
                fi
            done <<<"${entries//|/$'\n'}"
        fi
        if [[ "$next" == "-" ]]; then url=""; else url="$next"; fi
    done
    printf '%s' "$matches"
}

reuse_execution_registration() {
    local rows="$1" name host machine chosen="" chosen_machine="" count=0
    while IFS=$'\t' read -r name host machine; do
        name="${name%$'\r'}"; host="${host%$'\r'}"; machine="${machine%$'\r'}"
        [[ "${host,,}" == "${ARC_MACHINE_RESOURCE_ID,,}" ]] || continue
        chosen="$name"
        chosen_machine="$machine"
        count=$((count + 1))
    done <<<"$rows"
    if (( count != 1 )) || [[ ! "$chosen" =~ ^[a-zA-Z0-9._-]+$ \
        || "${chosen_machine,,}" != "${ARC_MACHINE_RESOURCE_ID,,}" ]]; then
        bad "this machine is registered more than once (or inconsistently) as an Execution Agent; nothing was changed"
        cause "found $count registration(s) for $ARC_MACHINE_RESOURCE_ID, expected exactly one that uses this machine's identity"
        fix "save this output and send it to the extension provider; do not pick or delete a registration by hand"
        return 1
    fi
    soft "hostIdentifier is already registered as '$chosen'"
    note "hostIdentifier identifies this connected Arc machine; duplicate registrations can route jobs ambiguously"
    note "reuse is valid only for a reinstall on this same physical host, never for a cloned/different host"
    note "the existing registration will not be overwritten; its actual name must be used"
    if (( DRY_RUN )); then
        note "dry run: a real run would ask yes/no before using '$chosen'"
    elif ! confirm_registration_reuse; then
        bad "you declined to reuse the existing registration '$chosen'; no extension was installed"
        cause "this machine already has an Execution Agent registration, and a second one is never created for the same machine"
        fix "if this is the same machine being reinstalled, run this script again and answer 'y':  $(rerun_cmd)"
        return 1
    fi
    EXECUTION_AGENT_ID="$chosen"
    okdid "existing executionAgent '$EXECUTION_AGENT_ID' selected with reuse consent"
}

register_execution_agent() {
    local agent_url rows resp put_rc probe body
    if ! rows="$(execution_registrations_for_host 2>"$ERR_FILE")"; then
        bad "could not list the existing Execution Agent registrations; nothing was registered"
        cause "$(last_err)"
        fix "make sure your account has 'Azure Migrate Owner' or Contributor on the Migrate project's resource group" \
            "run this script again:  $(rerun_cmd)"
        return 1
    fi
    if [[ -n "$rows" ]]; then
        reuse_execution_registration "$rows"
        return
    fi
    select_execution_agent_id || return 1
    agent_url="https://management.azure.com${MASTER_SITE_ID}/executionAgents/${EXECUTION_AGENT_ID}?api-version=${API_VER}"
    # PUT is create-or-update, so check this name before risking another host's registration.
    if rows="$(az rest --method get --url "$agent_url" \
        --query "[[name || '-', properties.hostIdentifier || '-', properties.agentAuthenticationSpn.machineResourceId || '-']]" \
        -o tsv 2>&1)"; then
        reuse_execution_registration "$rows"
        return
    elif ! grep -qiE 'RunAsAccountNotFound|ResourceNotFound|\(NotFound\)|"code"[[:space:]]*:[[:space:]]*"NotFound"' <<<"$rows"; then
        bad "could not check whether the name '$EXECUTION_AGENT_ID' is already in use; nothing was registered"
        cause "$(grep -m1 '^ERROR' <<<"$rows" | sed 's/^ERROR: //' || true)"
        fix "run this script again:  $(rerun_cmd)"
        return 1
    fi

    body="$(cat <<JSON
{
  "hostIdentifier": "$ARC_MACHINE_RESOURCE_ID",
  "osType": "Linux",
  "agentAuthenticationSpn": {
    "machineResourceId": "$ARC_MACHINE_RESOURCE_ID"
  }
  }
JSON
)"
    did "PUT $agent_url"
    if (( DRY_RUN )); then
        echo "  DRY   body: $body"
        note "a real duplicate-host error would require explicit registration reuse consent"
        return 0
    fi
    resp="$(az rest --method put --url "$agent_url" --body "$body" 2>&1)"; put_rc=$?
    if (( put_rc == 0 )) && ! grep -q '"error"' <<<"$resp"; then
        okdid "executionAgent '$EXECUTION_AGENT_ID' registered"
        DISCOVERY_SERVICE_ENDPOINT="$(extract_service_endpoint "$resp")"
        return 0
    fi

    if grep -qiE 'host.?identifier|HostAlreadyRegistered' <<<"$resp" \
        && grep -qiE 'already|duplicate|exists' <<<"$resp"; then
        note "${resp%%$'\n'*}"
        if ! rows="$(execution_registrations_for_host 2>"$ERR_FILE")"; then
            bad "Azure says this machine is already registered, but the registration could not be read"
            cause "$(last_err)"
            fix "run this script again:  $(rerun_cmd)"
            return 1
        fi
        reuse_execution_registration "$rows"
        return
    fi

    if grep -q 'Unknown error occurred in Migration Service' <<<"$resp"; then
        probe="$(az rest --method get \
            --url "https://management.azure.com${MASTER_SITE_ID}/preflightRouteProbeDoesNotExist?api-version=${API_VER}" 2>&1)"
        if grep -q 'Unknown error occurred in Migration Service' <<<"$probe"; then
            REGISTRATION_API_MISSING=1
            bad "Execution Agent registration is not enabled for this Migrate project yet"
            cause "the discovery master site rejects every executionAgents request (a deliberately invalid request returns the identical error)"
            fix "ask the extension provider to enable Execution Agents for: $MASTER_SITE_ID" \
                "run this script again:  $(rerun_cmd)"
            return 1
        fi
    fi
    bad "could not register this machine as Execution Agent '$EXECUTION_AGENT_ID'"
    reg_err="$(grep -m1 '^ERROR' <<<"$resp" | sed 's/^ERROR: //' || true)"
    [[ -n "$reg_err" ]] || reg_err="$(grep -m1 -v '^[[:space:]]*$' <<<"$resp" || true)"
    case "$resp" in
        *AuthorizationFailed*|*"does not have authorization"*)
            cause "your account ($signed_in) may not create Execution Agents: $reg_err"
            fix "ask for 'Azure Migrate Owner' or Contributor on the Migrate project's resource group ('Azure Migrate Execute Expert' is read-only)" \
                "run this script again:  $(rerun_cmd)" ;;
        *)
            cause "$reg_err"
            fix "run this script again:  $(rerun_cmd)" ;;
    esac
    return 1
}

if [[ -z "$MASTER_SITE_ID" ]]; then
    if (( DRY_RUN )); then
        soft "master site id unresolved; skipping LIST and PUT -- a real run would STOP here"
    else
        # Hard stop. The master-site PUT is the only thing that creates the
        # executionAgent record, and it is also where the discovery endpoint now
        # comes from. Continuing would install the extension with an agent id that
        # exists nowhere and an empty endpoint, which the agent accepts as a valid
        # registration and only fails on much later, while polling.
        bad "could not find the discovery master site for this Migrate project; nothing was registered"
        cause "the project has no appliance discovery yet, or its master site could not be read"
        fix "complete discovery from the Azure Migrate appliance for this project" \
            "run this script again:  $(rerun_cmd)"
        exit 1
    fi
else
    # Before the agent record, not after: the link is a property of the site
    # itself and is wanted whether this run creates an executionAgent or reuses
    # one that is already there. Putting it inside the PUT branch would mean a
    # re-run on an already-registered host silently skipped it.
    patch_master_site_project

    register_execution_agent || exit 1

    # Fallback only. Step 5 above reads the endpoint straight off the PUT or
    # LIST response; this runs when that response did not carry one, which is
    # what older service versions do. The sibling site holds the exact value the
    # Windows appliance persists in its configuration.
    #
    # Reads, so it runs under --dry-run too: this value is emitted into the
    # extension settings below, and a preview that always showed it empty hid
    # the one field whose absence stops the agent from ever reaching the
    # Azure Migrate discovery service.
    if [[ -z "$DISCOVERY_SERVICE_ENDPOINT" ]]; then
        site_id="$(az rest --method get \
            --url "https://management.azure.com${MASTER_SITE_ID}?api-version=${API_VER}" \
            --query "properties.sites[0] || ''" -o tsv 2>/dev/null || true)"

        if [[ -n "$site_id" ]]; then
            DISCOVERY_SERVICE_ENDPOINT="$(az resource show --ids "$site_id" \
                --query "properties.serviceEndpoint" -o tsv 2>/dev/null || true)"
        fi

        if [[ -n "$DISCOVERY_SERVICE_ENDPOINT" ]]; then
            ok "discoveryServiceEndpoint $DISCOVERY_SERVICE_ENDPOINT (from the sibling site)"
        elif (( DRY_RUN )); then
            soft "discoveryServiceEndpoint unresolved -- a real run would STOP here"
        else
            # Hard stop, deliberately. The agent does not treat a missing service
            # endpoint as an incomplete registration, so an execution.json written
            # without this value would be accepted and the agent would then poll an
            # empty host. Failing now turns that silent runtime break into a named
            # setup error.
            bad "could not find the discovery service address for this Migrate project; no extension was installed"
            cause "neither the Execution Agent registration nor the project's discovery sites carry a service endpoint yet (the agent cannot receive jobs without it)"
            fix "make sure the Azure Migrate appliance for this project has completed discovery" \
                "run this script again:  $(rerun_cmd)"
            exit 1
        fi
    fi
fi

# --- Step 6: extension install (check-then-create) --------------------------

# Logs worth attaching when a Step 6 failure has to go to the extension provider.
show_extension_logs() {
    note "Logs (attach these if you contact the extension provider):"
    # Name the installed version's folder: a cloned VM still carries the source machine's
    # older folders, and a `-*` glob would print that machine's agent and vault as well.
    note "  extension handler: sudo bash -c 'tail -n 60 /var/lib/GuestConfig/extension_logs/${PUBLISHER}.${EXT_TYPE}-${TYPE_HANDLER_VERSION:-*}/handler.log'"
    note "  service:           sudo journalctl -u $EXECUTION_SERVICE_NAME --no-pager -n 60"
    note "  agent:             sudo bash -c 'tail -n 60 /var/log/azure-migrate/execution/*.log'"
}

# extension_setup_failed <what> <cause> [fix step]...
# Without fix steps the default is to look at the logs and re-run with --force-refresh, which
# removes whatever half-installed state the failed attempt left before installing again.
extension_setup_failed() {
    local what="$1" why="${2:-}"
    shift 2 2>/dev/null || shift $#
    bad "$what"
    cause "$why"
    if (( $# )); then
        fix "$@"
    else
        fix "read the logs below for the reason" \
            "after fixing it, reinstall cleanly:  $(rerun_cmd --force-refresh)"
    fi
    show_extension_logs
    exit 1
}

# Turn wait_for_execution_agent's last observation into a Cause/Fix.
explain_health_failure() {
    local reason="$1" ext_show
    ext_show="az connectedmachine extension show --subscription $ARC_SUBSCRIPTION -g $ARC_RG --machine-name $ARC_VM -n $EXT_NAME --query properties.instanceView.status.message -o tsv"
    case "$reason" in
        "extension provisioning ended with"*)
            extension_setup_failed "the Execution Agent extension did not install successfully ($reason)" \
                "the extension handler on this machine reported a failure" \
                "read the handler's message:  $ext_show" \
                "after fixing the reason it gives, reinstall cleanly:  $(rerun_cmd --force-refresh)" ;;
        *"is not active"*)
            extension_setup_failed "the extension installed but the Execution Agent service is not running" \
                "$EXECUTION_SERVICE_NAME failed to start or stopped" \
                "see why:  sudo journalctl -u $EXECUTION_SERVICE_NAME --no-pager -n 60" \
                "after fixing it, reinstall cleanly:  $(rerun_cmd --force-refresh)" ;;
        *"does not match"*)
            extension_setup_failed "the Execution Agent is running with a different registration or key than this run set up" \
                "the agent's local configuration is left over from an earlier installation" \
                "reinstall cleanly:  $(rerun_cmd --force-refresh)" ;;
        *"managed-identity token"*)
            extension_setup_failed "the Execution Agent cannot get a token from this machine's Arc identity" \
                "the Arc agent's local identity service (HIMDS) did not issue a token" \
                "check the Arc agent:  sudo azcmagent show   and   sudo azcmagent check" \
                "run this script again:  $(rerun_cmd)" ;;
        *"job polling returned"*)
            extension_setup_failed "the Execution Agent is running but the discovery service rejected its job poll ($reason)" \
                "401/403 means the registration is not accepted for this machine yet; 404 means the registration was not found; no response means this machine cannot reach the service endpoint ($DISCOVERY_SERVICE_ENDPOINT)" \
                "for 'no response', check outbound HTTPS from this machine to that endpoint" \
                "otherwise wait a few minutes and run this script again (it changes nothing when already set up):  $(rerun_cmd)" ;;
        *)
            extension_setup_failed "the Execution Agent extension was not confirmed healthy" "$reason" ;;
    esac
}

latest_extension_version() {
    local base url row next versions all="" seen="" deadline=$((SECONDS + SETUP_WAIT_SECONDS))
    base="https://management.azure.com/subscriptions/$ARC_SUBSCRIPTION/providers/Microsoft.HybridCompute/locations/$ARC_LOCATION/publishers/$PUBLISHER/extensionTypes/$EXT_TYPE/versions"
    url="$base?api-version=2024-07-10"
    while [[ -n "$url" ]]; do
        if [[ "$url" != "$base?"* || "$seen" == *"|$url|"* ]] || (( SECONDS >= deadline )); then
            echo "invalid, repeated or timed-out extension catalogue page" >&2
            return 1
        fi
        seen="${seen}|${url}|"
        # Catalogue entries carry only id and properties.version -- there is no name field,
        # so value[].name is always empty and every lookup would report nothing published.
        if ! row="$(timeout 30 az rest --method get --url "$url" \
            --query "[[nextLink || '-', join('|', value[].properties.version) || '-']]" -o tsv 2>"$ERR_FILE")"; then
            echo "could not read the Arc extension catalogue in $ARC_LOCATION: $(last_err)" >&2
            return 1
        fi
        IFS=$'\t' read -r next versions <<<"${row//$'\r'/}"
        all="${all}${versions}"$'\n'
        if [[ "$next" == "-" ]]; then url=""; else url="$next"; fi
    done
    versions="$(tr '|' '\n' <<<"$all" | grep -E '^[0-9]+(\.[0-9]+){3}$' | sort -V | tail -1)"
    [[ -n "$versions" ]] || { echo "no published version of $EXT_TYPE is available in $ARC_LOCATION" >&2; return 1; }
    printf '%s\n' "$versions"
}

read_execution_key_uri() {
    if [[ -r /etc/azure-migrate/extension.env ]]; then
        sed -n 's/^AZMIGRATE_KEYVAULT_KEY_URI=//p' /etc/azure-migrate/extension.env 2>/dev/null
    else
        sudo -n sed -n 's/^AZMIGRATE_KEYVAULT_KEY_URI=//p' /etc/azure-migrate/extension.env 2>/dev/null
    fi | head -1
}

wait_for_execution_agent() {
    local deadline state cfg agent endpoint api audience token code reason="" previous=""
    local started next_tick
    if (( DRY_RUN )); then
        note "dry run: would wait for extension Succeeded, active service, matching registration and polling HTTP 200"
        return 0
    fi
    note "installing the extension usually takes 3-5 minutes (limit $((SETUP_WAIT_SECONDS / 60)) minutes); progress is shown below. Please wait."
    started=$SECONDS next_tick=$((SECONDS + 60))
    deadline=$((SECONDS + SETUP_WAIT_SECONDS))
    while :; do
        # The extension resource reports its state under properties; a top-level
        # provisioningState is always empty, so reading it would make every wait time out.
        if state="$(timeout 30 az connectedmachine extension show --subscription "$ARC_SUBSCRIPTION" \
            -g "$ARC_RG" --machine-name "$ARC_VM" -n "$EXT_NAME" \
            --query properties.provisioningState -o tsv 2>&1)"; then
            state="${state//$'\r'/}"
            case "$state" in
                Failed|Canceled|Deleting)
                    HEALTH_FAILURE="extension provisioning ended with $state"
                    return 1 ;;
            esac
            reason="extension provisioningState=${state:-unknown}"
            if [[ "$state" == "Succeeded" ]]; then
                reason="$EXECUTION_SERVICE_NAME is not active"
                if systemctl is-active --quiet "$EXECUTION_SERVICE_NAME" 2>/dev/null; then
                    cfg="$(read_execution_config)"
                    agent="$(json_string_field "$cfg" ExecutionAgentId)"
                    endpoint="$(json_string_field "$cfg" ServiceEndpoint)"
                    api="$(json_string_field "$cfg" ApiVersion)"
                    audience="$(json_string_field "$cfg" ArcTokenAudience)"
                    reason="the local registration or wrapping-key URI does not match this setup"
                    if [[ "$(json_string_field "$cfg" RegistrationStatus)" == "Registered" \
                        && "$agent" == "$EXECUTION_AGENT_ID" \
                        && "${endpoint%/}" == "${DISCOVERY_SERVICE_ENDPOINT%/}" \
                        && "$api" == "$DATAPLANE_API_VER" \
                        && "$(read_execution_key_uri)" == "${KV_URI%/}/keys/$KEY_NAME" ]]; then
                        token="$(arc_mi_token "${audience:-https://management.azure.com/}")"
                        reason="could not obtain an Arc managed-identity token"
                        if [[ -n "$token" ]]; then
                            code="$(curl_with_authorization "Bearer $token" -s -o /dev/null -w '%{http_code}' --max-time 20 \
                                -H "Accept: application/json" \
                                "${endpoint%/}/agentrestapi/agents/$agent/agentjobs?api-version=$api&jobStatus=NotStarted" 2>/dev/null)"
                            unset token
                            reason="job polling returned ${code:-no response}, not HTTP 200"
                            if [[ "$code" == "200" ]]; then
                                ok "extension provisioningState=Succeeded (after $((SECONDS - started))s)"
                                ok "$EXECUTION_SERVICE_NAME is active with registration '$agent'"
                                ok "job polling answered HTTP 200 using this machine's Arc identity"
                                note "polling is reachable; an empty queue does not prove end-to-end job execution"
                                return 0
                            fi
                        fi
                        unset token
                    fi
                fi
            fi
        else
            # Right after `create --no-wait` the resource does not exist yet; that is the
            # expected first answer, not a read failure.
            if arc_not_found "$state"; then
                reason="Azure has not created the extension resource yet"
            else
                reason="could not read extension status: ${state%%$'\n'*}"
            fi
        fi
        if [[ "$reason" != "$previous" ]]; then
            note "waiting: $reason"; previous="$reason"
        elif (( SECONDS >= next_tick )); then
            note "still waiting ($reason)... $((SECONDS - started))s elapsed"
        fi
        if (( SECONDS >= next_tick )); then next_tick=$((SECONDS + 60)); fi
        if (( SECONDS >= deadline )); then
            HEALTH_FAILURE="$reason"
            return 2
        fi
        sleep "$SETUP_POLL_SECONDS"
    done
}

# A timeout is not evidence of failure: the install may still be running, or may have
# finished in a way this poll could not observe. Say so, and hand over the exact checks.
extension_wait_timed_out() {
    bad "the Execution Agent was not confirmed healthy within $SETUP_WAIT_SECONDS seconds"
    cause "last observed: $1 (this alone does not mean the install failed -- it may still be running)"
    fix "check the extension's state:  az connectedmachine extension show --subscription $ARC_SUBSCRIPTION -g $ARC_RG --machine-name $ARC_VM -n $EXT_NAME --query \"[properties.provisioningState, properties.instanceView.status.message]\" -o tsv" \
        "Creating/Updating: wait a few minutes and check again" \
        "Succeeded: run this script again WITHOUT --force-refresh; it verifies the agent and changes nothing:  $(rerun_cmd)" \
        "Failed: read the message it shows, fix the reason, then reinstall cleanly:  $(rerun_cmd --force-refresh)"
    show_extension_logs
    exit 1
}

head1 "Step 6 -- Install Arc extension"

# The target flags are repeated verbatim on the create below rather than hoisted into an
# array. A static check asserts that EVERY `az connectedmachine` call names its
# subscription -- a check that cannot see through an array expansion. Four
# duplicated flags are a cheaper price than a correctness guard that has to trust an alias.
if ! ext_state="$(az connectedmachine extension show \
    --subscription "$ARC_SUBSCRIPTION" \
    -g "$ARC_RG" \
    --machine-name "$ARC_VM" \
    -n "$EXT_NAME" \
    --query properties.provisioningState -o tsv 2>&1)"; then
    if arc_not_found "$ext_state"; then
        ext_state=""
    else
        ext_state_err="$(grep -m1 '^ERROR' <<<"$ext_state" | sed 's/^ERROR: //' || true)"
        bad "could not check whether the Execution Agent extension is already on '$ARC_VM'; nothing was installed"
        cause "${ext_state_err:-${ext_state%%$'\n'*}}"
        fix "run this script again:  $(rerun_cmd)"
        exit 1
    fi
fi

if [[ -n "$ext_state" ]]; then
    bad "an Execution Agent extension appeared on '$ARC_VM' while this script was running (state=$ext_state); it was left alone"
    cause "someone or something else installed it at the same time; it may be using a different key than the one this run created"
    fix "make sure no other setup is running on this machine, then reinstall cleanly:  $(rerun_cmd --force-refresh)"
    exit 1
else
    did "az connectedmachine extension create ($EXT_NAME @ ${TYPE_HANDLER_VERSION:-latest})"

    # Settings emitted for the Linux Arc-extension flow.
    #
    # Alignment: master-site PUT in Step 5 is the service registration. The agent
    # needs only the id it should POLL under (executionAgentId), the endpoint,
    # the Arc machine ARM id, the token audience, and the KV vault URI + key
    # name. Everything else it derives locally.
    #
    # Both spellings of the agent id are emitted on purpose.
    #
    # The agent reads 'executionAgentId' first and falls back to the legacy key
    # 'discoveryAgentId', so a current agent ignores the second key entirely.
    # Older agents predate the new spelling and read 'discoveryAgentId' only:
    # given settings that carry just 'executionAgentId' they stall forever on
    # "cannot register yet", and their error text does not even name the key
    # they are missing, so the failure is very hard to diagnose from the logs.
    # Emitting both costs one identical line and removes that whole class of
    # version-skew failure, which matters while agent builds older than the
    # new spelling are still deployed.
    #
    # Handler design: the extension handler (enable.sh) calls resolve_kv_key
    # with (keyVaultUri, keyVaultKeyName). Step 3 already created that RSA key
    # under the operator's credentials, so the handler only reads it with the
    # Arc MI's Crypto User role, derives the versionless key URI, and writes it
    # into /etc/azure-migrate/extension.env for both systemd and interactive use.
    # Preflight therefore emits vault URI + key name, NOT the full key URI.
    #
    # ${DISCOVERY_SERVICE_ENDPOINT:-} is defensive: if any future refactor
    # introduces a code path where Step 5 doesn't populate the variable, set -u
    # would otherwise abort here inside the heredoc.
    #
    # agentAuthenticationSpn carries the Arc managed-identity form only, not the
    # classic triplet (tenantId + applicationId + objectId). The executionAgent
    # record is created by the master-site PUT above, and the handler seeds
    # execution.json from these settings, so the agent reads an already-registered
    # configuration and never registers itself. The triplet would also need
    # 'az ad sp show', i.e. directory read on the operator identity, for a field
    # nothing consumes. The Arc-MI form matches both the published ARM contract
    # and what the agent actually presents.
    #
    # apiVersion is emitted so the handler does not hardcode the dataplane
    # version. The dataplane (agentrestapi/*) is versioned separately from the
    # ARM control plane and is not covered by the published ARM contract.
    settings="$(cat <<JSON
{
  "migrateProjects": [ { "id": "$MIGRATE_PROJECT_ID" } ],
  "executionAgentId": "$EXECUTION_AGENT_ID",
  "discoveryAgentId": "$EXECUTION_AGENT_ID",
  "machineResourceId": "$ARC_MACHINE_RESOURCE_ID",
  "arcTokenAudience": "https://management.azure.com/",
  "apiVersion": "$DATAPLANE_API_VER",
  "tenantId": "$TENANT",
  "agentAuthenticationSpn": {
    "tenantId": "$TENANT",
    "machineResourceId": "$ARC_MACHINE_RESOURCE_ID"
  },
  "discoveryServiceEndpoint": "${DISCOVERY_SERVICE_ENDPOINT:-}",
  "keyVaultUri": "$KV_URI",
  "keyVaultKeyName": "$KEY_NAME"
}
JSON
)"
    # Explicit and catalogue-resolved versions use the same install flags.
    #
    # The ${a[@]+"${a[@]}"} form is deliberate: under `set -u` an empty array
    # expanded as "${a[@]}" aborts on bash < 4.4 (RHEL/CentOS 7 ships 4.2), and
    # Keep the compatible expansion if future optional flags leave this array empty.
    ver_args=()
    if [[ -n "$TYPE_HANDLER_VERSION" ]]; then
        ver_args+=(--type-handler-version "$TYPE_HANDLER_VERSION")
        note "explicit version override: pinning handler to $TYPE_HANDLER_VERSION"
    else
        if ! TYPE_HANDLER_VERSION="$(latest_extension_version 2>"$ERR_FILE")"; then
            bad "could not find which Execution Agent extension version is available in $ARC_LOCATION; nothing was installed"
            cause "$(cat "$ERR_FILE" 2>/dev/null | head -n1)"
            fix "run this script again in a few minutes:  $(rerun_cmd)" \
                "or, if you were given a specific version, pin it:  $(rerun_cmd '--type-handler-version <a.b.c.d>')"
            exit 1
        fi
        ver_args+=(--type-handler-version "$TYPE_HANDLER_VERSION")
        note "latest available handler in $ARC_LOCATION: $TYPE_HANDLER_VERSION"
    fi
    if (( ENABLE_AUTO_UPGRADE )); then
        ver_args+=(--enable-auto-upgrade true)
        note "automatic upgrade enabled; the platform will move this machine to future versions"
    fi

    # Wrapped so the failing path can capture stderr without also swallowing run()'s
    # dry-run preview, which that function deliberately writes to stderr.
    ext_create() {
        run az connectedmachine extension create \
            --subscription "$ARC_SUBSCRIPTION" \
            -g "$ARC_RG" \
            --machine-name "$ARC_VM" \
            -n "$EXT_NAME" \
            --location "$ARC_LOCATION" \
            --publisher "$PUBLISHER" \
            --type "$EXT_TYPE" \
            --no-wait true \
            "${ver_args[@]+"${ver_args[@]}"}" \
            --settings "$settings"
    }

    # This create must be guarded. The script has no `set -e`, so a failed install would
    # fall straight through to the PASS line below and the summary would still read "0 failed"
    # while nothing had been installed -- the worst thing preflight can do, because the
    # operator goes on to wait for an agent that will never check in.
    if (( DRY_RUN )); then
        ext_create >/dev/null
        okdid "extension install queued; Arc Guest Agent will run install.sh -> enable.sh"
    elif ext_err="$(ext_create 2>&1 >/dev/null)"; then
        okdid "extension install queued; Arc Guest Agent will run install.sh -> enable.sh"
    else
        ext_msg="$(grep -m1 '^ERROR' <<<"$ext_err" | sed 's/^ERROR: //' || true)"
        [[ -n "$ext_msg" ]] || ext_msg="$(grep -m1 -v '^[[:space:]]*$' <<<"$ext_err" || true)"
        if grep -qiE 'Failed to find executable' <<<"$ext_err"; then
            # The Guest Agent takes the first whitespace-separated token of a handler
            # command as a path relative to the unpacked handler directory, so a manifest
            # saying "bash bin/install.sh" makes it look for a file literally named `bash`
            # there. Builds published before that was corrected still carry it.
            extension_setup_failed "could not install the Execution Agent extension $TYPE_HANDLER_VERSION on '$ARC_VM'" \
                "extension version $TYPE_HANDLER_VERSION is defective (it names a program it does not ship) -- not a problem with this machine or your permissions: $ext_msg" \
                "ask the extension provider which version in $ARC_LOCATION is fixed" \
                "reinstall with it:  $(rerun_cmd --force-refresh '--type-handler-version <a.b.c.d>')"
        elif grep -qiE 'AADSTS|Conditional Access' <<<"$ext_err"; then
            extension_setup_failed "could not install the Execution Agent extension on '$ARC_VM'" \
                "a sign-in (Conditional Access) policy refused this machine a token -- NOT a missing role: $ext_msg" \
                "run this script from a device your organisation's sign-in policy allows"
        elif grep -qiE 'AuthorizationFailed|does not have authorization' <<<"$ext_err"; then
            extension_setup_failed "could not install the Execution Agent extension on '$ARC_VM'" \
                "your account ($signed_in) may not install extensions on this Arc machine: $ext_msg" \
                "ask for 'Azure Connected Machine Resource Administrator' (or Contributor) on /subscriptions/$ARC_SUBSCRIPTION/resourceGroups/$ARC_RG" \
                "run this script again:  $(rerun_cmd)"
        else
            extension_setup_failed "could not install the Execution Agent extension on '$ARC_VM'" \
                "$ext_msg" \
                "run this script again:  $(rerun_cmd)"
        fi
    fi
fi

wait_for_execution_agent
case $? in
    0) ;;
    2) extension_wait_timed_out "$HEALTH_FAILURE" ;;
    *) explain_health_failure "$HEALTH_FAILURE" ;;
esac

# --- Summary -----------------------------------------------------------------

head1 "Summary"
echo "  $pass passed, $fail failed, $warn warning(s)"

if (( ${REGISTRATION_API_MISSING:-0} )); then
    echo
    echo "  !! NOT READY FOR JOBS"
    echo "     this machine was never registered as an execution agent, because the"
    echo "     executionAgents API is not enabled on:"
    echo "       $MASTER_SITE_ID"
    echo "     the local setup above is complete and correct -- vault, key and role"
    echo "     assignments are really in place -- but without that record the service"
    echo "     has no way to dispatch jobs here. this exits non-zero on purpose so a"
    echo "     caller is never told the machine is ready when it is not."
fi
echo
echo "Next step -- store the credential for your targets:"
echo "    sudo -i"
echo "    set -a; . /etc/azure-migrate/extension.env; set +a"
echo "    amcredential add --name <friendly-name> --user <user> --type LinuxServer"
echo "  that file is written by the extension handler in Step 6 with the key URI"
echo "  this script just created, so there is nothing to copy by hand."
echo "  the friendly name is letters and digits only; the Id is generated from it."
echo "  use the friendly name as runAsAccount in jobs."
echo "  the password is always prompted for and never written anywhere."
echo "  customer scripts go in /var/opt/microsoft/azuremigrate/scriptstore."

if (( fail > 0 )); then exit 1; fi
if (( ${REGISTRATION_API_MISSING:-0} )); then exit 1; fi
exit 0
