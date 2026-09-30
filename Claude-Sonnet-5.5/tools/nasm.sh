#!/usr/bin/env bash
# nasm wrapper. Uses nasm from the PATH when there is one. Otherwise it gets nasm through nix (see shell.nix): with nix-shell, using the
# system channel when NIX_PATH is not set, and as a last resort through the flake CLI.
set -euo pipefail
here="$(cd "$(dirname "$0")/.." && pwd)"
if command -v nasm >/dev/null 2>&1; then exec nasm "$@"; fi
if [ -z "${NIX_PATH:-}" ] && [ -e /nix/var/nix/profiles/per-user/root/channels/nixos ]; then
  export NIX_PATH=nixpkgs=/nix/var/nix/profiles/per-user/root/channels/nixos
fi
q="$(printf '%q ' "$@")"
if command -v nix-shell >/dev/null 2>&1 && nix-shell "$here/shell.nix" --run "nasm $q"; then exit 0; fi
if command -v nix >/dev/null 2>&1; then exec nix --extra-experimental-features 'nix-command flakes' shell nixpkgs#nasm --command nasm "$@"; fi
echo "nasm not found: install it, or run the build inside 'nix-shell'" >&2; exit 127
