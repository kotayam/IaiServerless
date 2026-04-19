#!/usr/bin/env python3
import requests
import sys
import time
import subprocess
import os
import argparse

GATEWAY_PORT = 8080

def test_endpoint(name):
    print(f"--- Testing endpoint: /{name} ---")
    try:
        start_time = time.time()
        response = requests.get(f"{GATEWAY_URL}/{name}")
        duration = time.time() - start_time
        
        print(f"Status Code: {response.status_code}")
        print(f"Response Body:\n{response.text}")
        print(f"Request Duration: {duration:.3f}s")
        print("-" * 30)
        return response.status_code == 200
    except Exception as e:
        print(f"Error calling /{name}: {e}")
        return False

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("endpoints", nargs="*", default=["c/hello", "c/prime", "c/net_query", "c/weather", "c/json_builder"])
    parser.add_argument("--port", type=int, default=GATEWAY_PORT)
    args = parser.parse_args()

    global GATEWAY_URL
    GATEWAY_URL = f"http://localhost:{args.port}"

    # Check if gateway is running
    try:
        requests.get(GATEWAY_URL)
    except requests.exceptions.ConnectionError:
        print(f"Error: Gateway is not running on {GATEWAY_URL}")
        print("Please start it first: cd gateway && go run main.go -runtime=kvm")
        sys.exit(1)

    endpoints = args.endpoints

    success_count = 0
    for ep in endpoints:
        if test_endpoint(ep):
            success_count += 1

    print(f"\nSummary: {success_count}/{len(endpoints)} tests passed.")

if __name__ == "__main__":
    main()
