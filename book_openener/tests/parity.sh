#!/usr/bin/env bash
set -euo pipefail
exec python3 "$TEST_SRCDIR/$TEST_WORKSPACE/book_openener/tests/check_driver_parity.py"
