#!/usr/bin/env python3
"""Write a solver job from the Columbus sample, and optionally run it.

Useful for smoke testing an engine artifact, and for the parity work: point
GeoDa at the same data set and the same weights and compare the numbers.

    tools/make_demo_job.py --outdir /tmp/columbus --model ML_Lag --run
    tools/make_demo_job.py --model ML_Lag_Regimes --regime CRIME_BIN --run

Runs with the engine's interpreter (it needs libpysal for the sample data):

    "$ENGINE/bin/python3" tools/make_demo_job.py ...
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(ROOT / "solver"))
import solve           # noqa: E402
import spreg_models    # noqa: E402


def columbus() -> dict:
    import libpysal
    from libpysal import examples

    db = libpysal.io.open(examples.get_path("columbus.dbf"), "r")
    y = np.asarray(db.by_col("HOVAL"), dtype=float).reshape(-1, 1)
    x_names = ["INC", "CRIME"]
    x = np.column_stack([np.asarray(db.by_col(c), dtype=float) for c in x_names])
    w = libpysal.weights.Queen.from_shapefile(examples.get_path("columbus.shp"))
    csr = w.sparse.tocsr()
    coords = np.column_stack([np.asarray(db.by_col("X"), dtype=float),
                              np.asarray(db.by_col("Y"), dtype=float)])
    crime = np.asarray(db.by_col("CRIME"), dtype=float)
    regime = (crime > float(np.median(crime))).astype(float).reshape(-1, 1)
    return {"n": int(y.shape[0]), "y": y, "x": x, "x_names": x_names,
            "names": list(db.header), "csr": csr, "coords": coords, "regime": regime,
            "table": {name: np.asarray(db.by_col(name), dtype=float) for name in db.header}}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--outdir", default="/tmp/geoda-spreg-columbus")
    parser.add_argument("--model", default="ML_Lag")
    parser.add_argument("--x", default="INC,CRIME", help="covariates (without the constant)")
    parser.add_argument("--y", default="HOVAL")
    parser.add_argument("--regime", default="", help="column to use as the regime variable")
    parser.add_argument("--endog", default="", help="endogenous variable(s), comma separated")
    parser.add_argument("--instruments", default="", help="instrument(s), comma separated")
    parser.add_argument("--transform", default="r", choices=["r", "b", "o"])
    parser.add_argument("--method", default="LU", choices=["LU", "full", "ord"])
    parser.add_argument("--run", action="store_true", help="run the solver afterwards")
    args = parser.parse_args()

    data = columbus()
    table = data["table"]

    def columns(names: str) -> np.ndarray:
        wanted = [n.strip() for n in names.split(",") if n.strip()]
        missing = [n for n in wanted if n not in table]
        if missing:
            raise SystemExit("no such column(s): %s\navailable: %s"
                             % (", ".join(missing), ", ".join(table)))
        return np.column_stack([table[n] for n in wanted])

    x_names = [n.strip() for n in args.x.split(",") if n.strip()]
    x = np.column_stack([table[n] for n in x_names])
    y = table[args.y].reshape(-1, 1)
    csr = data["csr"]

    arrays = {
        "y": y, "x": x,
        "w_indptr": csr.indptr.astype(np.int64),
        "w_indices": csr.indices.astype(np.int32),
        "w_data": csr.data.astype(float),
    }
    spec_data = {
        "n": data["n"],
        "y": {"name": args.y, "array": "y"},
        "x": {"names": x_names, "array": "x"},
        "constant": True,
    }
    if args.regime:
        values = table[args.regime]
        name = args.regime
        if len(np.unique(values)) > 12:                 # a continuous column: split it
            values = (values > float(np.median(values))).astype(float)
            name = args.regime + "_BIN"
            print("regime column %s is continuous; using the median split %s"
                  % (args.regime, name))
        arrays["regime"] = values.reshape(-1, 1)
        spec_data["regimes"] = {"name": name, "array": "regime"}
    if args.endog:
        names = [n.strip() for n in args.endog.split(",")]
        arrays["yend"] = np.column_stack([table[n] for n in names])
        spec_data["endogenous"] = {"names": names, "array": "yend"}
    if args.instruments:
        names = [n.strip() for n in args.instruments.split(",")]
        arrays["q"] = np.column_stack([table[n] for n in names])
        spec_data["instruments"] = {"names": names, "array": "q"}

    entry = spreg_models.get_model(args.model)          # rejects a typo immediately
    options = {"method": args.method} if "method" in entry["options"] else {}
    spec = {
        "protocol": solve.PROTOCOL_VERSION,
        "job_id": "columbus",
        "created": "demo",
        "app": {"name": "make_demo_job", "version": "1", "platform": sys.platform},
        "model": args.model,
        "options": options,
        "data": spec_data,
        "weights": {"format": "csr", "n": data["n"], "indptr": "w_indptr",
                    "indices": "w_indices", "data": "w_data",
                    "transform": args.transform, "name": "columbus queen",
                    "is_symmetric": True},
        "outputs": {"observations": True, "report": True},
    }

    outdir = Path(args.outdir)
    solve.write_job_dir(str(outdir), spec, arrays)
    print("job written to %s (model %s, %d observations)"
          % (outdir, args.model, data["n"]))
    print("  + %s" % json.dumps(spec["data"]))

    if args.run:
        solver = ROOT / "solver" / "solve.py"
        code = subprocess.call([sys.executable, str(solver), "--job", str(outdir)])
        result_path = outdir / "result.json"
        if result_path.exists():
            result = json.loads(result_path.read_text())
            if result.get("status") == "ok":
                print("\ncoefficients:")
                for name, est, se in zip(result["coefficients"]["names"],
                                         result["coefficients"]["estimate"],
                                         result["coefficients"]["std_err"] or
                                         [None] * len(result["coefficients"]["estimate"])):
                    print("  %-14s %12.6g %12s" % (name, est, "" if se is None else "%.6g" % se))
                print("fit: %s" % json.dumps(result.get("fit", {})))
                print("spatial: %s" % json.dumps(result.get("spatial_parameters", [])))
                print("diagnostics: %d, warnings: %d, report: %s"
                      % (len(result.get("diagnostics", [])), len(result.get("warnings", [])),
                         result.get("report", "-")))
            else:
                print("FAILED: %s" % result.get("error", {}).get("message"))
        return code
    return 0


if __name__ == "__main__":
    sys.exit(main())
