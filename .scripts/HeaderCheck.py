import subprocess, sys, os

def headercheck():
    print("\n=== running header-check ===", flush=True)

    res = subprocess.run(
        ["bash", ".scripts/HeaderCompile.sh"],
        env={**os.environ, "FORCE_COLOR": "1"},
    )
    if res.returncode != 0:
        sys.exit("Header check failed.")

headercheck()
