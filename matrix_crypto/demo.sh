#!/usr/bin/env bash
# Демонстрация CLI C++-порта (человекочитаемые режимы).
set -e
MC="$HOME/bignum/matrix_crypto/build/matrix_crypto"
DIR=$(mktemp -d)
cd "$DIR"

echo "=== keygen ==="
"$MC" keygen

echo
echo "=== encrypt -> decrypt (authenticated envelope) ==="
SEALED=$("$MC" encrypt "Совершенно секретно: 42")
echo "sealed length = ${#SEALED}"
echo "decrypt -> $("$MC" decrypt "$SEALED")"

echo
echo "=== sign -> verify ==="
SIG=$("$MC" sign "подпиши меня")
"$MC" verify "подпиши меня" "$SIG"

echo
echo "=== verify on tampered message ==="
"$MC" verify "подпиши МЕНЯ" "$SIG" || true

echo
echo "=== decrypt tampered envelope (must fail) ==="
BAD="${SEALED:0:50}AAAA"
"$MC" decrypt "$BAD" || true

echo
echo "=== authenticated DH (demo) ==="
"$MC" dh | tail -6

rm -rf "$DIR"
