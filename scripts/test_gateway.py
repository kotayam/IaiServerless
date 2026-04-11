#!/usr/bin/env python3
import requests
import sys
import time
import subprocess
import os

GATEWAY_URL = "http://localhost:8080"

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
    # Check if gateway is running
    try:
        requests.get(GATEWAY_URL)
    except requests.exceptions.ConnectionError:
        print("Error: Gateway is not running on http://localhost:8080")
        print("Please start it first: cd gateway && go run main.go -runtime=kvm")
        sys.exit(1)

    endpoints = ["hello", "prime", "net_query", "weather"]
    if len(sys.argv) > 1:
        endpoints = sys.argv[1:]

    success_count = 0
    for ep in endpoints:
        if test_endpoint(ep):
            success_count += 1

    print(f"\nSummary: {success_count}/{len(endpoints)} tests passed.")

if __name__ == "__main__":
    main()
