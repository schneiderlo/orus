#!/usr/bin/env bash
set -euo pipefail

revision="$(git rev-parse --verify HEAD)"
if [[ -n "$(git status --porcelain --untracked-files=normal)" ]]; then
  revision="${revision}-dirty"
fi
printf 'STABLE_ORUS_SOURCE_REVISION %s\n' "$revision"
