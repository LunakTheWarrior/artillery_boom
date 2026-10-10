#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")/../.."
exec bazelisk test //book_openener:control_test //book_openener:driver_parity_test \
    --platforms=@platforms//host --test_output=errors "$@"
