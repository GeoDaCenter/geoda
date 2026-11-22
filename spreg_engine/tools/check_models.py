#!/usr/bin/env python3
"""Run every model in the registry against synthetic data.

This is the gate for an engine artifact: it proves that each model id GeoDa can
ask for actually runs against the pinned spreg, that the call style and option
mapping in solver/spreg_models.py are right, and that the protocol round trip
works for all of them.  It is also how you find out what a spreg upgrade broke.

    tools/check_models.py [--models ML_Lag,GM_Combo] [-v]
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import sys
import tempfile
import time
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(ROOT / "solver"))

import solve            # noqa: E402
import spreg_models     # noqa: E402

ROWS, COLS = 6, 10


def synthetic() -> dict:
    rows, cols = ROWS, COLS
    n = rows * cols
    rng = np.random.default_rng(20261007)
    x1 = rng.normal(size=(n, 1))
    x2 = rng.normal(size=(n, 1))
    latent = rng.normal(size=(n, 1))
    yend = latent + 0.3 * rng.normal(size=(n, 1))
    q = latent + 0.3 * rng.normal(size=(n, 1))
    y = 1.0 + 2.0 * x1 - 0.5 * x2 + 0.8 * yend + rng.normal(scale=0.5, size=(n, 1))
    ybin = (y > np.median(y)).astype(float)

    r, c, v = [], [], []
    for i in range(rows):
        for j in range(cols):
            k = i * cols + j
            for di, dj in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                ii, jj = i + di, j + dj
                if 0 <= ii < rows and 0 <= jj < cols:
                    r.append(k)
                    c.append(ii * cols + jj)
                    v.append(1.0)
    r, c, v = np.array(r), np.array(c), np.array(v)
    order = np.argsort(r, kind="stable")
    r, c, v = r[order], c[order], v[order]
    indptr = np.zeros(n + 1, dtype=np.int64)
    np.add.at(indptr, r + 1, 1)
    indptr = np.cumsum(indptr)

    regime = np.array([[float(j >= cols // 2)] for _ in range(rows) for j in range(cols)])
    coords = np.array([[float(j), float(i)] for i in range(rows) for j in range(cols)])

    return {
        "n": n,
        "arrays": {"y": y, "ybin": ybin, "x": np.hstack([x1, x2]),
                   "yend": yend, "q": q, "regime": regime, "coords": coords,
                   "w_indptr": indptr, "w_indices": c.astype(np.int32), "w_data": v},
    }


def job_for(model_id: str, data: dict) -> dict:
    entry = spreg_models.get_model(model_id)
    n = data["n"]
    options = {}
    if "method" in entry["options"]:
        options["method"] = "LU"                  # sparse and quick at any n
    if model_id == "SKATER_reg":
        options.update({"n_clusters": 2, "model_family": "spreg"})
    spec = {
        "protocol": solve.PROTOCOL_VERSION, "job_id": "check-" + model_id,
        "created": "check", "app": {"name": "check_models", "version": "1"},
        "model": model_id, "options": options,
        "data": {"n": n,
                 "y": {"name": "ybin" if model_id == "Probit" else "y",
                       "array": "ybin" if model_id == "Probit" else "y"},
                 "x": {"names": ["x1", "x2"], "array": "x"},
                 "constant": True},
        "weights": {"format": "csr", "n": n, "indptr": "w_indptr", "indices": "w_indices",
                    "data": "w_data", "transform": "r", "name": "grid-rook"},
        "outputs": {"observations": True, "report": True},
    }
    needs = entry["requires"]
    if needs["regimes"]:
        spec["data"]["regimes"] = {"name": "regime", "array": "regime"}
    if needs["endog"]:
        spec["data"]["endogenous"] = {"names": ["yend"], "array": "yend"}
    if needs["instruments"]:
        spec["data"]["instruments"] = {"names": ["q"], "array": "q"}
    if needs["coords"]:
        spec["data"]["coords"] = {"names": ["X", "Y"], "array": "coords"}
    if not needs["weights"]:
        spec.pop("weights")
    return spec


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--models", default="", help="comma separated subset of model ids")
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("--json", default="", help="write the report here")
    args = parser.parse_args()

    wanted = [m for m in args.models.split(",") if m] or [m["id"] for m in spreg_models.MODELS]
    data = synthetic()
    results = []
    jobdir = tempfile.mkdtemp(prefix="geoda-spreg-check-")
    try:
        for model_id in wanted:
            spec = job_for(model_id, data)
            solve.write_job_dir(jobdir, spec, data["arrays"])
            started = time.time()
            stdout, stderr = sys.stdout, sys.stderr
            if not args.verbose:
                sys.stdout = open(os.devnull, "w")
            try:
                code = solve.cmd_run(jobdir)
            finally:
                if not args.verbose:
                    sys.stdout.close()
                    sys.stdout = stdout
            elapsed = time.time() - started
            result = solve.read_json(os.path.join(jobdir, "result.json"))
            ok = code == 0 and result.get("status") == "ok"
            message = "" if ok else str(result.get("error", {}).get("message", ""))[:110]
            results.append({"model": model_id, "ok": ok, "seconds": round(elapsed, 2),
                            "coefficients": len((result.get("coefficients") or {})
                                                .get("estimate", [])),
                            "diagnostics": len(result.get("diagnostics", [])),
                            "message": message})
            print("%-28s %-4s %6.2fs  k=%s  %s"
                  % (model_id, "ok" if ok else "FAIL", elapsed,
                     results[-1]["coefficients"], message), flush=True)
    finally:
        shutil.rmtree(jobdir, ignore_errors=True)

    failed = [r for r in results if not r["ok"]]
    print("\n%d/%d models ran" % (len(results) - len(failed), len(results)))
    for r in failed:
        print("  FAILED %-26s %s" % (r["model"], r["message"]))
    if args.json:
        Path(args.json).write_text(json.dumps(results, indent=2) + "\n")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
