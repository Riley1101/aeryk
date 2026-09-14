#!/usr/bin/env bash
# Headless QEMU boot smoke test: boots the built ISO with no display and
# asserts a late-boot marker appears on the serial console within a
# timeout. Catches boot hangs, triple faults, and taskswitch-class
# regressions that host-side unit tests (`make test`) can't see, since
# those never actually run the kernel.
set -uo pipefail

ISO="${1:-template-x86_64.iso}"
TIMEOUT="${QEMU_SMOKE_TIMEOUT:-30}"
MARKER="[8] /bin/sh loaded, switching to Ring 3..."

if [ ! -f "$ISO" ]; then
  echo "error: ISO image '$ISO' not found (run 'make' first)" >&2
  exit 1
fi

LOGDIR="$(mktemp -d)"
SERIAL_LOG="$LOGDIR/serial.log"
QEMU_LOG="$LOGDIR/qemu.log"

# -no-reboot/-no-shutdown so a triple fault halts the VM instead of
# resetting it, and the process just sits there until the timeout kills it
# rather than looping forever.
timeout --signal=KILL "$TIMEOUT" qemu-system-x86_64 \
  -M q35 \
  -cdrom "$ISO" \
  -boot d \
  -m 2G \
  -d guest_errors,int \
  -D "$QEMU_LOG" \
  -display none \
  -serial file:"$SERIAL_LOG" \
  -no-reboot -no-shutdown

echo "=== serial output ==="
cat "$SERIAL_LOG" 2>/dev/null || true
echo "======================"

if ! grep -qF "$MARKER" "$SERIAL_LOG" 2>/dev/null; then
  echo "FAIL: expected marker not found in serial output within ${TIMEOUT}s: $MARKER" >&2
  echo "--- qemu.log (guest_errors,int) ---" >&2
  cat "$QEMU_LOG" 2>/dev/null >&2 || true
  exit 1
fi

echo "PASS: kernel booted to Ring 3 shell within ${TIMEOUT}s"
