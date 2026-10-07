#!/usr/bin/env python3
"""GeoDa spatial regression solver, backed by spreg.

    solve.py --job <jobdir>        run the job described in <jobdir>/job.json
    solve.py --selftest            end-to-end protocol test on synthetic data
    solve.py --list-models         the model registry, as JSON
    solve.py --engine-info         versions of the interpreter and packages

The wire format is documented in PROTOCOL.md.  This program is deliberately
stateless and single-shot: it reads a job directory, writes a result, exits.
GeoDa owns the directory, the timeout and the process lifetime.

Exit codes
    0  ok                 (result.json written, status "ok")
    1  estimation failed  (result.json written, status "error")
    2  protocol error     (job.json unreadable, protocol mismatch, bad arrays)
"""

from __future__ import annotations

import argparse
import contextlib
import json
import os
import shutil
import sys
import tempfile
import time
import traceback
from typing import Any, Dict, List, Optional, Sequence, Tuple

PROTOCOL_VERSION = 1
SOLVER_VERSION = "1.0.0"

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import spreg_models as REG  # noqa: E402  (path set up above)

DTYPES = {"f8": "float64", "i8": "int64", "i4": "int32", "u1": "uint8"}
NPY_DTYPE = {
    "f8": "float64", "i8": "int64", "i4": "int32", "u1": "uint8",
    "f4": "float32", "i2": "int16",
}


class ProtocolError(Exception):
    """The job directory does not satisfy the protocol."""


# ---------------------------------------------------------------------------
# logging
# ---------------------------------------------------------------------------

class Log:
    def __init__(self, path: Optional[str]) -> None:
        self.path = path
        self.fh = open(path, "w", encoding="utf-8") if path else None

    def __call__(self, message: str) -> None:
        line = "[geoda-spreg] %s" % message
        print(line, flush=True)
        if self.fh:
            self.fh.write(line + "\n")
            self.fh.flush()

    def close(self) -> None:
        if self.fh:
            self.fh.close()
            self.fh = None


class _Chatter:
    """Catch what spreg prints so that stdout carries only solver progress."""

    def __init__(self, log: "Log") -> None:
        self.log = log

    def write(self, text: str) -> int:
        if self.log.fh and text.strip():
            self.log.fh.write("    " + text.rstrip() + "\n")
            self.log.fh.flush()
        return len(text)

    def flush(self) -> None:
        if self.log.fh:
            self.log.fh.flush()


# ---------------------------------------------------------------------------
# reading a job
# ---------------------------------------------------------------------------

def read_json(path: str) -> Dict[str, Any]:
    with open(path, "r", encoding="utf-8") as fh:
        return json.load(fh)


def read_arrays(jobdir: str, job: Dict[str, Any], wanted: Sequence[str]):
    """Load the arrays referenced by the job out of data.bin."""
    import numpy as np

    table = job.get("arrays")
    if not isinstance(table, dict) or not table:
        raise ProtocolError("job.json has no 'arrays' table")
    path = os.path.join(jobdir, "data.bin")
    if not os.path.isfile(path):
        raise ProtocolError("data.bin is missing")

    arrays: Dict[str, Any] = {}
    with open(path, "rb") as fh:
        payload = fh.read()
    for name in wanted:
        if name is None:
            continue
        desc = table.get(name)
        if desc is None:
            raise ProtocolError("array %r is referenced but not declared in 'arrays'" % name)
        dtype = NPY_DTYPE.get(desc.get("dtype"))
        if dtype is None:
            raise ProtocolError("array %r has unsupported dtype %r" % (name, desc.get("dtype")))
        shape = tuple(int(s) for s in desc.get("shape", []))
        offset = int(desc.get("offset", 0))
        nbytes = int(desc.get("nbytes", 0))
        expected = int(np.prod(shape)) * np.dtype(dtype).itemsize if shape else 0
        if nbytes != expected:
            raise ProtocolError("array %r: nbytes %d does not match shape %s of %s (%d)"
                                % (name, nbytes, shape, desc["dtype"], expected))
        if offset < 0 or offset + nbytes > len(payload):
            raise ProtocolError("array %r: [%d, %d) is outside data.bin (%d bytes)"
                                % (name, offset, offset + nbytes, len(payload)))
        flat = np.frombuffer(payload, dtype=dtype, count=int(np.prod(shape)) if shape else 0,
                             offset=offset)
        arrays[name] = np.array(flat).reshape(shape) if shape else flat
    return arrays


def collect_array_names(job: Dict[str, Any]) -> List[str]:
    names: List[str] = []

    def add(name: Optional[str]) -> None:
        if name and name not in names:
            names.append(name)

    data = job.get("data", {})
    add((data.get("y") or {}).get("array"))
    for key in ("x", "regimes", "endogenous", "instruments", "coords"):
        block = data.get(key)
        if isinstance(block, dict):
            add(block.get("array"))
    weights = job.get("weights")
    if isinstance(weights, dict):
        for key in ("indptr", "indices", "data"):
            add(weights.get(key))
    return names


# ---------------------------------------------------------------------------
# building the estimation inputs
# ---------------------------------------------------------------------------

def build_data(job: Dict[str, Any], arrays: Dict[str, Any], log: Log) -> Dict[str, Any]:
    import numpy as np

    spec = job.get("data")
    if not isinstance(spec, dict):
        raise ProtocolError("job.json has no 'data' section")
    n = int(spec.get("n", 0))
    if n <= 0:
        raise ProtocolError("job.json: data.n must be positive")

    def block_array(name: str, block: Dict[str, Any], required: bool = True):
        arr = arrays.get(block.get("array")) if isinstance(block, dict) else None
        if arr is None:
            if required:
                raise ProtocolError("array %r is missing for %s" % (block, name))
            return None
        arr = np.asarray(arr, dtype=float)
        if arr.ndim == 1:
            arr = arr.reshape(-1, 1)
        if arr.shape[0] != n:
            raise ProtocolError("%s has %d rows, expected %d" % (name, arr.shape[0], n))
        if not np.isfinite(arr).all():
            raise ProtocolError("%s contains NaN or infinite values" % name)
        return arr

    y_block = spec.get("y") or {}
    y = block_array("y", y_block)
    y_name = str(y_block.get("name") or "y")

    x_block = spec.get("x") or {}
    x = block_array("x", x_block)
    x_names = [str(v) for v in (x_block.get("names") or
                                ["x%d" % (i + 1) for i in range(x.shape[1])])]
    if x.shape[1] != len(x_names):
        raise ProtocolError("data.x has %d columns but %d names" % (x.shape[1], len(x_names)))

    data: Dict[str, Any] = {"n": n, "y": y, "y_name": y_name, "x": x, "x_names": x_names,
                            "constant": bool(spec.get("constant", True))}

    reg_block = spec.get("regimes")
    if isinstance(reg_block, dict):
        arr = np.asarray(arrays[reg_block["array"]]).reshape(-1)
        if arr.shape[0] != n:
            raise ProtocolError("regimes has %d rows, expected %d" % (arr.shape[0], n))
        values = [_scalar_id(v) for v in arr.tolist()]
        order: List[Any] = []
        for v in values:
            if v not in order:
                order.append(v)
        if len(order) < 2:
            raise ProtocolError("the regime variable has a single value; at least two are needed")
        data["regimes"] = values
        data["regime_name"] = str(reg_block.get("name") or "regime")
        data["regime_levels"] = order

    for key, label in (("endogenous", "endogenous variables"), ("instruments", "instruments")):
        block = spec.get(key)
        if isinstance(block, dict):
            arr = block_array(label, block)
            names = [str(v) for v in (block.get("names") or [])]
            data[key] = arr
            data[key + "_names"] = names or ["%s%d" % (key[:3], i + 1)
                                             for i in range(arr.shape[1])]

    coords_block = spec.get("coords")
    if isinstance(coords_block, dict):
        arr = np.asarray(arrays[coords_block["array"]], dtype=float).reshape(-1, 2)
        if arr.shape[0] != n:
            raise ProtocolError("coords has %d rows, expected %d" % (arr.shape[0], n))
        data["coords"] = arr
        data["coords_names"] = [str(v) for v in (coords_block.get("names") or ["X", "Y"])]

    return data


def _scalar_id(value: Any) -> Any:
    """Regime ids keep their identity but must be hashable and JSON-safe."""
    if isinstance(value, float) and value.is_integer():
        return int(value)
    return value


def build_weights(job: Dict[str, Any], arrays: Dict[str, Any], data: Dict[str, Any], log: Log):
    """CSR + transform -> libpysal W."""
    import numpy as np
    import scipy.sparse as sp
    from libpysal import weights as libw

    spec = job.get("weights")
    if not isinstance(spec, dict):
        return None
    n = data["n"]
    if spec.get("format", "csr") != "csr":
        raise ProtocolError("weights.format %r is not supported (use 'csr')" % spec.get("format"))
    if int(spec.get("n", n)) != n:
        raise ProtocolError("weights.n (%s) does not match data.n (%d)" % (spec.get("n"), n))

    indptr = np.asarray(arrays[spec["indptr"]], dtype=np.int64).reshape(-1)
    indices = np.asarray(arrays[spec["indices"]], dtype=np.int32).reshape(-1)
    values = np.asarray(arrays[spec["data"]], dtype=float).reshape(-1)
    if indptr.shape[0] != n + 1:
        raise ProtocolError("weights.indptr must have n+1 = %d entries, got %d"
                            % (n + 1, indptr.shape[0]))
    if indices.shape != values.shape:
        raise ProtocolError("weights.indices and weights.data must have the same length")
    if indptr[-1] != indices.shape[0]:
        raise ProtocolError("weights.indptr[-1] (%d) does not match the number of stored "
                            "weights (%d)" % (indptr[-1], indices.shape[0]))
    if len(indices) and (indices.min() < 0 or indices.max() >= n):
        raise ProtocolError("weights.indices contains out-of-range neighbor ids")
    if not np.isfinite(values).all():
        raise ProtocolError("weights.data contains NaN or infinite values")
    if len(indices) and (indices == np.repeat(np.arange(n), np.diff(indptr))).any():
        raise ProtocolError("weights contain self-loops; spreg requires a zero diagonal")

    transform = str(spec.get("transform", "r")).lower()
    if transform not in ("r", "b", "o"):
        raise ProtocolError("weights.transform must be 'r', 'b' or 'o', got %r" % transform)

    if transform == "b":
        values = np.ones_like(values)
    if transform == "r":
        # bincount, not reduceat: reduceat misreports rows with no stored weights
        owner = np.repeat(np.arange(n), np.diff(indptr))
        row_sums = np.bincount(owner, weights=values, minlength=n)
        empty = np.where(row_sums == 0)[0]
        if empty.size:
            raise ProtocolError(
                "cannot row-standardize: %d observation(s) have no neighbors (first: %d)"
                % (empty.size, int(empty[0])))

    csr = sp.csr_matrix((values, indices, indptr), shape=(n, n))
    w = libw.WSP(csr).to_W()
    if transform == "r":
        w.transform = "r"
    elif transform == "b":
        w.transform = "b"
    # 'o': leave the weights exactly as sent (transform stays unset)
    log("weights: n=%d, %d stored links, transform=%s, name=%s"
        % (n, len(values), transform, spec.get("name", "")))
    return w


# ---------------------------------------------------------------------------
# running the model
# ---------------------------------------------------------------------------

def call_estimator(entry: Dict[str, Any], cls: Any, data: Dict[str, Any],
                   kwargs: Dict[str, Any], log: Log):
    y, x, w = data["y"], data["x"], data.get("w")
    style = entry["style"]
    regimes = data.get("regimes")
    yend = data.get("endogenous")
    q = data.get("instruments")

    if style == "standard":
        return cls(y, x, w, **kwargs)
    if style == "regimes":
        return cls(y, x, regimes, w, **kwargs)
    if style == "endog":
        return cls(y, x, yend, q, w, **kwargs)
    if style == "endog_regimes_a":
        return cls(y, x, yend, q, regimes, w, **kwargs)
    if style == "endog_regimes_b":
        return cls(y, x, regimes, yend, q, w, **kwargs)
    if style == "endog_regimes_c":                    # GMM_Error_Regimes: w before yend/q
        return cls(y, x, regimes, w, yend, q, **kwargs)
    if style == "nslx":
        return cls(y, x, data["coords"], **kwargs)
    if style == "skater":
        n_clusters = int(kwargs.pop("n_clusters", 5))
        quorum = int(kwargs.pop("quorum", -1))
        model_family = kwargs.pop("model_family", "both")
        estimator = cls()
        try:
            return estimator.fit(n_clusters, W=w, data=x, quorum=quorum,
                                 model_family=model_family, **kwargs)
        except TypeError:
            return estimator.fit(n_clusters, W=w, data=x, quorum=quorum, **kwargs)
    raise ProtocolError("unknown call style %r" % style)


def run_job(jobdir: str, log: Log) -> Tuple[Dict[str, Any], Dict[str, Any]]:
    """Run one job.  Returns (result, observation arrays to write)."""
    import numpy as np
    import spreg

    job = read_json(os.path.join(jobdir, "job.json"))
    protocol = job.get("protocol")
    if protocol != PROTOCOL_VERSION:
        raise ProtocolError("job protocol %r is not supported by this solver (expected %d); "
                            "the engine directory is probably from another GeoDa version"
                            % (protocol, PROTOCOL_VERSION))

    entry = REG.get_model(job.get("model"))
    log("model: %s (%s)" % (entry["id"], entry["class"]))

    arrays = read_arrays(jobdir, job, collect_array_names(job))
    data = build_data(job, arrays, log)
    log("data: n=%d, %d covariate(s), y=%s" % (data["n"], data["x"].shape[1], data["y_name"]))

    requires = entry["requires"]
    if requires["weights"] and not job.get("weights"):
        raise ProtocolError("model %s needs a spatial weights matrix" % entry["id"])
    if requires["regimes"] and "regimes" not in data:
        raise ProtocolError("model %s needs a regime variable" % entry["id"])
    if requires["endog"] and "endogenous" not in data:
        raise ProtocolError("model %s needs at least one endogenous variable" % entry["id"])
    if requires["instruments"] and "instruments" not in data:
        raise ProtocolError("model %s needs at least one instrument" % entry["id"])
    if requires["coords"] and "coords" not in data:
        raise ProtocolError("model %s needs coordinates" % entry["id"])
    if entry["id"] == "Probit":
        values = set(float(v) for v in np.asarray(data["y"]).reshape(-1).tolist())
        if not values <= {0.0, 1.0}:
            raise ProtocolError("Probit needs a dependent variable with values 0 and 1 "
                                "(found %s)" % ", ".join(repr(v) for v in sorted(values)[:4]))

    data["w"] = build_weights(job, arrays, data, log) if job.get("weights") else None

    try:
        kwargs, warnings = REG.validate_options(entry, job.get("options") or {})
    except ValueError as exc:
        raise ProtocolError(str(exc))
    if entry["style"] in ("regimes", "endog_regimes_a", "endog_regimes_b"):
        # the solver owns the constant for regimes models
        kwargs["constant_regi"] = kwargs.get("constant_regi", "many") if data["constant"] else False
    elif not data["constant"]:
        warnings.append("model %s always includes a constant term in spreg" % entry["id"])

    module_name, _, class_name = entry["class"].partition(":")
    cls = getattr(__import__(module_name, fromlist=[class_name]), class_name)

    log("estimating %s%s" % (entry["id"], "" if not kwargs else " with %s" % _short(kwargs)))
    started = time.time()
    with contextlib.redirect_stdout(_Chatter(log)):     # spreg prints the model name
        model = call_estimator(entry, cls, data, kwargs, log)
    log("done in %.2fs" % (time.time() - started))

    ctx = {"x_names": data["x_names"], "y_name": data["y_name"], "n": data["n"]}
    result = REG.extract_result(model, entry, ctx)
    warnings += result.setdefault("warnings", [])
    result["warnings"] = warnings

    observations = {}
    obs_arrays: Dict[str, Any] = {}
    if (job.get("outputs") or {}).get("observations", True):
        n = data["n"]
        per_observation = [("yhat", "predy", "f8"), ("resid", "u", "f8"),
                           ("pred_err", "e_pred", "f8"), ("predy_e", "predy_e", "f8"),
                           ("region", "current_labels_", "i4")]     # region: SKATER_reg
        for key, attr, dtype in per_observation:
            value = getattr(model, attr, None)
            if value is None:
                continue
            arr = np.asarray(value).reshape(-1)
            if arr.shape[0] != n:
                continue
            obs_arrays[key] = arr.astype("float64" if dtype == "f8" else "int32")
        if obs_arrays:
            observations = {"n": n, "arrays": {}}
            offset = 0
            for key, arr in obs_arrays.items():
                code = "f8" if arr.dtype.kind == "f" else "i4"
                observations["arrays"][key] = {
                    "dtype": code, "shape": [int(arr.shape[0])],
                    "offset": offset, "nbytes": int(arr.nbytes),
                }
                offset += int(arr.nbytes)
            result["observations"] = observations

    report = getattr(model, "summary", None)
    if not report and "clusters" in result:                     # regionalization
        clusters = result["clusters"]
        report = ("SUMMARY OF OUTPUT: %s\n%s\nRegion sizes: %s\n"
                  % (result["model"]["title"], "-" * 68,
                     ", ".join(str(v) for v in clusters["sizes"])))
    if report and (job.get("outputs") or {}).get("report", True):
        with open(os.path.join(jobdir, "report.txt"), "w", encoding="utf-8") as fh:
            fh.write(report if report.endswith("\n") else report + "\n")
        result["report"] = "report.txt"

    result["dataset"] = {"y": data["y_name"], "x": data["x_names"],
                         "regimes": data.get("regime_name"),
                         "weights": (job.get("weights") or {}).get("name", "")}
    result["_observations"] = obs_arrays
    return result, obs_arrays


def _short(kwargs: Dict[str, Any]) -> str:
    return ", ".join("%s=%s" % (k, v) for k, v in sorted(kwargs.items()) if v is not None)


# ---------------------------------------------------------------------------
# writing a result
# ---------------------------------------------------------------------------

def engine_info() -> Dict[str, Any]:
    import platform

    info = {"solver": SOLVER_VERSION, "python": platform.python_version(),
            "protocol": PROTOCOL_VERSION}
    for mod, key in (("spreg", "spreg"), ("libpysal", "libpysal"), ("numpy", "numpy"),
                     ("pandas", "pandas"), ("scipy", "scipy"), ("sklearn", "scikit-learn")):
        try:
            info[key] = getattr(__import__(mod), "__version__", "unknown")
        except Exception:
            pass
    return info


def write_result(jobdir: str, job_id: Any, result: Dict[str, Any],
                 obs_arrays: Optional[Dict[str, Any]] = None) -> None:
    if obs_arrays:
        path = os.path.join(jobdir, "result.bin")
        with open(path, "wb") as fh:
            for arr in obs_arrays.values():
                fh.write(arr.tobytes(order="C"))
    payload = {"protocol": PROTOCOL_VERSION, "job_id": job_id, "status": "ok",
               "engine": engine_info()}
    payload.update({k: v for k, v in result.items() if not k.startswith("_")})
    with open(os.path.join(jobdir, "result.json"), "w", encoding="utf-8") as fh:
        json.dump(payload, fh, indent=2, allow_nan=False)
        fh.write("\n")


def write_error(jobdir: str, job_id: Any, exc: BaseException, where: str,
                status: str = "error") -> None:
    payload = {"protocol": PROTOCOL_VERSION, "job_id": job_id, "status": status,
               "engine": engine_info(),
               "error": {"type": type(exc).__name__, "message": str(exc),
                         "traceback": "".join(traceback.format_exception(
                             type(exc), exc, exc.__traceback__)), "where": where}}
    try:
        with open(os.path.join(jobdir, "result.json"), "w", encoding="utf-8") as fh:
            json.dump(payload, fh, indent=2)
            fh.write("\n")
    except OSError:
        pass


# ---------------------------------------------------------------------------
# commands
# ---------------------------------------------------------------------------

def cmd_run(jobdir: str) -> int:
    log = Log(os.path.join(jobdir, "solver.log"))
    result_path = os.path.join(jobdir, "result.json")
    if os.path.exists(result_path):
        os.remove(result_path)
    job_id = None
    try:
        try:
            job_id = read_json(os.path.join(jobdir, "job.json")).get("job_id")
        except Exception:
            pass
        import warnings as _warnings

        with _warnings.catch_warnings(record=True) as caught:
            _warnings.simplefilter("always")
            result, obs = run_job(jobdir, log)
        seen = set()
        for item in caught:
            text = "%s: %s" % (item.category.__name__, item.message)
            if text not in seen:
                seen.add(text)
                log(text)
                result.setdefault("warnings", []).append(text)
        write_result(jobdir, job_id, result, obs)
        log("result written: %s" % ", ".join(
            "%s=%s" % (k, v) for k, v in (result.get("fit") or {}).items()))
        return 0
    except ProtocolError as exc:
        log("protocol error: %s" % exc)
        write_error(jobdir, job_id, exc, "protocol", status="error")
        return 2
    except Exception as exc:                                  # estimation failure
        log("estimation failed: %s: %s" % (type(exc).__name__, exc))
        log(traceback.format_exc())
        write_error(jobdir, job_id, exc, "estimation", status="error")
        return 1
    finally:
        log.close()


def cmd_selftest() -> int:
    """End-to-end test of the protocol, with a numerical anchor.

    Builds a synthetic job, writes it exactly the way GeoDa does, runs it, and
    checks the OLS coefficients against a direct least-squares solve, then runs
    a lag model and a regimes model.
    """
    import numpy as np

    failures: List[str] = []
    report: Dict[str, Any] = {"checks": []}

    def check(name: str, ok: bool, detail: str = "") -> None:
        report["checks"].append({"name": name, "ok": bool(ok), "detail": detail})
        print("[selftest] %-38s %s %s" % (name, "ok" if ok else "FAIL", detail), flush=True)
        if not ok:
            failures.append(name)

    jobdir = tempfile.mkdtemp(prefix="geoda-spreg-selftest-")
    try:
        # --- a deterministic dataset: 6x10 grid, rook contiguity -----------
        rows, cols = 6, 10
        n = rows * cols
        xs = np.array([[float(i % cols), float(i // cols)] for i in range(n)])
        rng = np.random.default_rng(20261007)
        x1 = rng.normal(size=(n, 1))
        x2 = rng.normal(size=(n, 1))
        y = 1.5 + 2.0 * x1 - 0.75 * x2 + rng.normal(scale=0.5, size=(n, 1))
        x = np.hstack([x1, x2])

        rows_idx, cols_idx, vals = [], [], []
        for r in range(rows):
            for c in range(cols):
                i = r * cols + c
                for dr, dc in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                    rr, cc = r + dr, c + dc
                    if 0 <= rr < rows and 0 <= cc < cols:
                        rows_idx.append(i)
                        cols_idx.append(rr * cols + cc)
                        vals.append(1.0)
        order = np.argsort(np.array(rows_idx), kind="stable")
        rows_idx, cols_idx, vals = np.array(rows_idx)[order], np.array(cols_idx)[order], \
            np.array(vals)[order]
        indptr = np.zeros(n + 1, dtype=np.int64)
        np.add.at(indptr, rows_idx + 1, 1)
        indptr = np.cumsum(indptr)
        regime = np.array([int(c >= cols // 2) for c in range(n)])

        arrays = {"y": y.reshape(-1, 1), "x": x, "regime": regime.reshape(-1, 1),
                  "w_indptr": indptr, "w_indices": cols_idx.astype(np.int32),
                  "w_data": vals, "coords": xs}
        spec = {
            "protocol": PROTOCOL_VERSION, "job_id": "selftest",
            "created": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "app": {"name": "GeoDa", "version": "selftest", "platform": sys.platform},
            "model": "OLS",
            "options": {},
            "data": {"n": n, "y": {"name": "y", "array": "y"},
                     "x": {"names": ["x1", "x2"], "array": "x"}, "constant": True,
                     "regimes": {"name": "regime", "array": "regime"},
                     "coords": {"names": ["X", "Y"], "array": "coords"}},
            "weights": {"format": "csr", "n": n, "indptr": "w_indptr", "indices": "w_indices",
                        "data": "w_data", "transform": "r", "name": "grid-rook"},
            "outputs": {"observations": True, "report": True},
        }
        write_job_dir(jobdir, spec, arrays)
        check("job directory written", os.path.isfile(os.path.join(jobdir, "data.bin")))

        def run(model: str, options: Optional[Dict[str, Any]] = None):
            job = json.loads(json.dumps(spec))
            job["model"] = model
            job["options"] = options or {}
            write_job_dir(jobdir, job, arrays)
            code = cmd_run(jobdir)
            result = read_json(os.path.join(jobdir, "result.json"))
            return code, result

        code, res = run("OLS")
        check("OLS runs", code == 0 and res["status"] == "ok",
              res.get("error", {}).get("message", ""))

        # numerical anchor: OLS coefficients must equal a direct least-squares solve
        design = np.hstack([np.ones((n, 1)), x1, x2])
        beta_ref = np.linalg.lstsq(design, y, rcond=None)[0].reshape(-1)
        beta = np.array([float(v) for v in (res.get("coefficients") or {}).get("estimate", [])])
        same = beta.shape == beta_ref.shape and np.allclose(beta, beta_ref, rtol=1e-8, atol=1e-8)
        check("OLS coefficients match numpy least squares", same,
              "max|diff|=%.3g" % (np.abs(beta - beta_ref).max() if beta.shape == beta_ref.shape
                                  else float("nan")))

        names = (res.get("coefficients") or {}).get("names", [])
        check("coefficient names rewritten", names[:3] == ["CONSTANT", "x1", "x2"], str(names[:3]))
        check("fit statistics present", {"r2", "logll", "aic"} <= set(res["fit"]),
              str(sorted(res["fit"])))
        check("diagnostics present", any(d["name"] == "lm_lag" for d in res["diagnostics"]),
              "%d diagnostic(s)" % len(res["diagnostics"]))
        check("engine info present", "spreg" in res.get("engine", {}),
              str(res.get("engine", {}).get("spreg")))
        check("observations written", os.path.getsize(os.path.join(jobdir, "result.bin")) == 2 * n * 8)
        check("report written", os.path.isfile(os.path.join(jobdir, "report.txt")))

        code, res = run("ML_Lag", {"method": "LU"})
        rho = (res.get("spatial_parameters") or [{}])[0].get("estimate")
        check("ML_Lag runs", code == 0 and res["status"] == "ok",
              res.get("error", {}).get("message", ""))
        check("ML_Lag rho in (-1, 1)", rho is not None and -1.0 < float(rho) < 1.0, "rho=%s" % rho)

        code, res = run("ML_Lag_Regimes", {"method": "LU"})
        check("ML_Lag_Regimes runs", code == 0 and res["status"] == "ok",
              res.get("error", {}).get("message", ""))
        check("regimes table regimized",
              any("0_" in nm for nm in (res.get("coefficients") or {}).get("names", [])),
              str((res.get("coefficients") or {}).get("names", [])[:3]))

        code, res = run("ML_Lag", {"method": "nosuchmethod"})
        check("bad option reported as an error", code != 0 and res["status"] == "error",
              res.get("error", {}).get("message", "")[:70])

        broken = json.loads(json.dumps(spec))
        broken["protocol"] = 99
        write_job_dir(jobdir, broken, arrays)
        code = cmd_run(jobdir)
        res = read_json(os.path.join(jobdir, "result.json"))
        check("protocol mismatch refused", code == 2 and "protocol" in res["error"]["message"],
              res["error"]["message"][:70])

        broken = json.loads(json.dumps(spec))
        write_job_dir(jobdir, broken, arrays)
        path = os.path.join(jobdir, "job.json")
        doc = read_json(path)
        doc["arrays"]["y"]["nbytes"] = 7                      # inconsistent descriptor
        with open(path, "w", encoding="utf-8") as fh:
            json.dump(doc, fh, indent=2)
        code = cmd_run(jobdir)
        res = read_json(os.path.join(jobdir, "result.json"))
        check("inconsistent array refused", code == 2 and res["status"] == "error",
              res["error"]["message"][:70])
    finally:
        shutil.rmtree(jobdir, ignore_errors=True)

    report["ok"] = not failures
    print(json.dumps({"selftest": "passed" if not failures else "failed",
                      "failures": failures}, indent=2))
    return 0 if not failures else 1


def write_job_dir(jobdir: str, spec: Dict[str, Any], arrays: Dict[str, Any]) -> None:
    """Write job.json + data.bin.  The same layout the C++ side uses."""
    import numpy as np

    os.makedirs(jobdir, exist_ok=True)
    offsets: Dict[str, Any] = {}
    offset = 0
    with open(os.path.join(jobdir, "data.bin"), "wb") as fh:
        for name, arr in arrays.items():
            arr = np.ascontiguousarray(arr)
            fh.write(arr.tobytes(order="C"))
            offsets[name] = {"dtype": {"float64": "f8", "int64": "i8", "int32": "i4",
                                       "uint8": "u1"}[arr.dtype.name],
                             "shape": [int(s) for s in arr.shape],
                             "offset": offset, "nbytes": int(arr.nbytes)}
            offset += int(arr.nbytes)
    spec = dict(spec)
    spec["arrays"] = offsets
    with open(os.path.join(jobdir, "job.json"), "w", encoding="utf-8") as fh:
        json.dump(spec, fh, indent=2)
        fh.write("\n")


def cmd_list_models() -> int:
    print(json.dumps({"protocol": PROTOCOL_VERSION, "engine": engine_info(),
                      "models": REG.list_models()}, indent=2))
    return 0


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="GeoDa spreg solver")
    parser.add_argument("--job", metavar="DIR", help="job directory to run")
    parser.add_argument("--selftest", action="store_true", help="run the end-to-end protocol test")
    parser.add_argument("--list-models", action="store_true", help="print the model registry")
    parser.add_argument("--engine-info", action="store_true", help="print versions")
    args = parser.parse_args(argv)

    if args.engine_info:
        print(json.dumps(engine_info(), indent=2))
        return 0
    if args.list_models:
        return cmd_list_models()
    if args.selftest:
        return cmd_selftest()
    if args.job:
        return cmd_run(args.job)
    parser.print_help()
    return 2


if __name__ == "__main__":
    sys.exit(main())
