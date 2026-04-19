#!/usr/bin/env python3
# Wraps a Python serverless function for process/docker runtimes.
# Imports the module, calls handler(), emits X-Exec-Time to stderr.
import sys, time, importlib.util

spec = importlib.util.spec_from_file_location("function", sys.argv[1])
mod = importlib.util.module_from_spec(spec)
t0 = time.monotonic()
spec.loader.exec_module(mod)
if hasattr(mod, "handler"):
    mod.handler()
print(f"X-Exec-Time: {(time.monotonic() - t0) * 1000:.4f}", file=sys.stderr)
