#!/usr/bin/env bash
set -e

cd "$(dirname "$0")/.."

echo "[*] Building gateway..."
go build -o gateway/gateway_bin ./gateway/main.go

echo "[*] Building samples..."
make -C samples proc docker-all

echo ""
echo "[*] Starting gateways:"
echo "    :9381  native   (c/* → process, python/* → python3)"
echo "    :9382  kvm      (c/* and python/*)"
echo "    :9383  docker   (c/* and python/*)"
echo ""

cd gateway
./gateway_bin -runtime=native -port=9381 &
PID1=$!
./gateway_bin -runtime=kvm    -port=9382 &
PID2=$!
./gateway_bin -runtime=docker -port=9383 &
PID3=$!

trap "echo ''; echo '[*] Shutting down...'; kill $PID1 $PID2 $PID3 2>/dev/null" EXIT INT TERM

echo "[*] Demo ready: http://localhost:9381"
echo "    Press Ctrl+C to stop."
wait
