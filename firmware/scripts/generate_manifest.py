#!/usr/bin/env python3
import os, json, subprocess, time, sys

def git_commit():
    try:
        return subprocess.check_output(["git", "rev-parse", "--short", "HEAD"]).decode().strip()
    except Exception:
        return "unknown"

def main():
    # PlatformIO provides environment variables
    fw_name = os.environ.get("IZOD_FW_NAME", "iZod Mini")
    fw_version = os.environ.get("IZOD_FW_VERSION", "0.1.0")
    env_name = os.environ.get("PIOENV", "unknown")
    build_dir = os.environ.get("BUILD_DIR", ".pio/build/{}".format(env_name))

    # Find firmware binary
    bin_path = None
    for fn in os.listdir(build_dir):
        if fn.endswith(".bin") and "firmware" in fn:
            bin_path = os.path.join(build_dir, fn)
            break
    if not bin_path:
        # fallback: first bin
        for fn in os.listdir(build_dir):
            if fn.endswith(".bin"):
                bin_path = os.path.join(build_dir, fn)
                break

    manifest = {
        "name": fw_name,
        "version": fw_version,
        "build_date": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "git_commit": git_commit(),
        "environment": env_name,
        "binary": os.path.basename(bin_path) if bin_path else None
    }

    out_path = os.path.join(build_dir, "firmware.json")
    with open(out_path, "w") as f:
        json.dump(manifest, f, indent=2)
    print("Wrote manifest:", out_path)

if __name__ == "__main__":
    main()


