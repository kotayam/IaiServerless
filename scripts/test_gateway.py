#!/usr/bin/env python3
import requests
import sys
import time
import subprocess
import os
import argparse

GATEWAY_PORT = 8080

def test_endpoint(name, gateway_port):
    print(f"--- Testing endpoint: /{name} ---")
    try:
        start_time = time.time()
        
        # Determine if POST is needed
        is_prime = name.endswith("/prime")
        is_weather_fmt = name.endswith("/weather_fmt")
        
        if is_prime:
            response = requests.post(f"{GATEWAY_URL}/{name}", data="10000")
        elif is_weather_fmt:
            response = requests.post(f"{GATEWAY_URL}/{name}", data=str(gateway_port))
        else:
            response = requests.get(f"{GATEWAY_URL}/{name}")
        
        duration = time.time() - start_time
        
        cold = response.headers.get("X-Cold-Start", "N/A")
        exec_t = response.headers.get("X-Exec-Time", "N/A")
        e2e = response.headers.get("X-E2E-Latency", "N/A")

        print(f"Status Code: {response.status_code}")
        print(f"Cold Start: {cold} ms | Exec Time: {exec_t} ms | E2E Latency: {e2e} ms")
        print(f"Response Body:\n{response.text}")
        print(f"Request Duration: {duration:.3f}s")
        print("-" * 30)
        return response.status_code == 200
    except Exception as e:
        print(f"Error calling /{name}: {e}")
        return False

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("endpoints", nargs="*", default=[
        "c/hello", "c/prime", "c/weather", "c/weather_fmt", "c/json_builder",
        "python/hello", "python/prime", "python/weather", "python/weather_fmt"
    ])
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
        if test_endpoint(ep, args.port):
            success_count += 1

    print(f"\nSummary: {success_count}/{len(endpoints)} tests passed.")

if __name__ == "__main__":
    main()
