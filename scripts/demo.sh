#!/usr/bin/env bash
set -e

cd "$(dirname "$0")/.."

echo "[*] Building gateway..."
go build -o gateway/gateway_bin ./gateway/main.go

echo "[*] Building samples..."
make -C samples proc docker-all

echo ""
echo "[*] Starting gateways:"
echo "    :8081  native   (c/* → process, python/* → python3)"
echo "    :8082  kvm      (c/* and python/*)"
echo "    :8083  docker   (c/* and python/*)"
echo ""

cd gateway
./gateway_bin -runtime=native -port=8081 &
PID1=$!
./gateway_bin -runtime=kvm    -port=8082 &
PID2=$!
./gateway_bin -runtime=docker -port=8083 &
PID3=$!

trap "echo ''; echo '[*] Shutting down...'; kill $PID1 $PID2 $PID3 2>/dev/null" EXIT INT TERM

echo "[*] Demo ready: http://localhost:8081"
echo "    Press Ctrl+C to stop."
wait
