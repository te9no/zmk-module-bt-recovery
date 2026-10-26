#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -f "$tmp/test"; rmdir "$tmp"' EXIT
cc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined,address -Iinclude src/recovery_policy.c tests/test_policy.c -o "$tmp/test"
"$tmp/test"
