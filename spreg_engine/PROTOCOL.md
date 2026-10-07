# GeoDa ⇄ spreg solver protocol, version 1

The solver is a separate process. GeoDa writes one *job directory*, launches the
solver, and reads back one result. Nothing else is shared: no sockets, no ports,
no shared memory, no GIL. A crashed solver is an error dialog, not a crashed
application.

```
        GeoDa (C++/wx)                          solver (python)
        ──────────────                          ───────────────
  write  job.json  ──┐
  write  data.bin  ──┴─▶  <jobdir>  ──▶  solve.py --job <jobdir>
                                               │
                     <jobdir>  ◀──  write  result.json
                                   write  result.bin   (optional, observations)
                                   write  report.txt   (spreg's own text report)
                                   write  solver.log
                     ──▶  read result.json / result.bin / report.txt
```

Directory: one per run, created by GeoDa in the system temp dir and deleted
afterwards (kept on failure for bug reports):

```
geoda-spreg-<pid>-<n>/
    job.json        request          (GeoDa writes)
    data.bin        array payload    (GeoDa writes)
    result.json     response         (solver writes)
    result.bin      array payload    (solver writes, only if outputs requested)
    report.txt      human-readable   (solver writes)
    solver.log      solver stdout/stderr, incl. tracebacks (solver writes)
```

All numbers are IEEE-754 **little-endian**, C order (row-major). x86-64 and
arm64 hosts are little-endian, so GeoDa can `fwrite` its `double[]` buffers
directly; the spec is written down so this is a contract, not an accident.

---

## 1. `job.json`

```jsonc
{
  "protocol": 1,                       // required, integer
  "job_id": "6f1c9a3e",                // opaque, echoed back in result.json
  "created": "2026-10-07T17:04:11Z",   // informational
  "app": { "name": "GeoDa", "version": "1.22.1", "platform": "macos-arm64" },

  "model": "ML_Lag",                   // model id from the registry (§4)
  "options": {                         // model specific, validated (§4)
    "method": "LU",
    "slx_lags": 1
  },

  "data": {
    "n": 49,                           // number of observations (after listwise deletion)
    "y":  { "name": "HOVAL", "array": "y" },
    "x":  { "names": ["INC", "CRIME"], "array": "x" },   // WITHOUT the constant
    "constant": true,                  // see §3.1: the solver handles the intercept
    "regimes":     { "name": "CRIME_BIN", "array": "regime" },     // regimes models only
    "endogenous":  { "names": ["CRIME"], "array": "yend" },        // endog models only
    "instruments": { "names": ["INC_LAG"], "array": "q" },         // endog models only
    "coords":      { "names": ["X", "Y"], "array": "coords" }      // NSLX only
  },

  "weights": {                         // omit for pure OLS without spatial diagnostics
    "format": "csr",
    "n": 49,
    "indptr":  "w_indptr",
    "indices": "w_indices",
    "data":    "w_data",
    "transform": "r",                  // "r" row-standardize | "b" binary | "o" as sent
    "name": "columbus_q.gal",          // shown in the report header
    "is_symmetric": true               // advisory only; asymmetric W is allowed
  },

  "outputs": { "observations": true, "report": true },

  "arrays": {                          // every array used above, byte offsets into data.bin
    "y":         { "dtype": "f8", "shape": [49, 1],   "offset": 0,    "nbytes": 392  },
    "x":         { "dtype": "f8", "shape": [49, 2],   "offset": 392,  "nbytes": 784  },
    "w_indptr":  { "dtype": "i8", "shape": [50],     "offset": 1176, "nbytes": 400  },
    "w_indices": { "dtype": "i4", "shape": [236],    "offset": 1576, "nbytes": 944  },
    "w_data":    { "dtype": "f8", "shape": [236],    "offset": 2520, "nbytes": 1888 }
  }
}
```

### 1.1 `arrays`

| field    | meaning |
|----------|---------|
| `dtype`  | `f8` float64, `i8` int64, `i4` int32, `u1` uint8 |
| `shape`  | row-major shape. `[n]`, `[n,1]`, `[n,k]` |
| `offset` | byte offset into `data.bin` |
| `nbytes` | expected byte count (`prod(shape) * itemsize`); the solver verifies it |

Arrays may appear in any order and may overlap in principle, but GeoDa writes
them back-to-back in declaration order. Unused bytes in `data.bin` are ignored.
Arrays not referenced by `data`/`weights` are not read.

### 1.2 weights

CSR is used because GeoDa's `GalElement[]` already *is* CSR: for each row, a
neighbor list and a parallel weight list. Convert with

```cpp
indptr[0] = 0;
for (i = 0; i < n; ++i) {
    indptr[i+1] = indptr[i] + gal[i].Size();
    for (j = 0; j < gal[i].Size(); ++j) {
        indices[p] = gal[i].GetNbrs()[j];
        data[p++]  = gal[i].GetNbrWeights()[j];   // 1.0 for a binary GAL
    }
}
```

`transform` is applied by the **solver**, never by GeoDa, so the same weights
object can be sent once and reused for several models:

| value | meaning |
|-------|---------|
| `"r"` | row-standardize (`w_ij / sum_j w_ij`). Equivalent to spreg's `w.transform = "r"` |
| `"b"` | binary: every stored weight becomes 1.0, no normalization (spreg `"b"`) |
| `"o"` | use the stored weights exactly as sent (spreg leaves the W untouched) |

A **GWT** file loaded in GeoDa carries real weights; a **GAL** carries 1.0. So
`transform:"r"` + GAL reproduces a row-standardized binary W, and
`transform:"r"` + GWT reproduces a row-standardized weighted W.

Notes:

* Self-loops (`indices[i] == i`) are rejected by the solver with a clear error.
* Asymmetric W (KNN, kernel) is fully supported. This is a *gain* over the
  current engine, which rejects KNN for lag/error models.
* Zero-row weights are rejected for `"r"` (division by zero).
* For panel-style or multiple W's, a future protocol version will add a list;
  v1 has exactly one.

### 1.3 Missing values

GeoDa performs listwise deletion **before** writing the job: `n` is the number
of complete observations, and the CSR weights must already be the submatrix
over those observations (the existing dialog code that rebuilds a filtered
`GalElement[]` does exactly this today). The solver rejects `NaN`/`Inf` in
`y`, `x`, `yend`, `q`, `coords`, and non-finite weights.

---

## 2. `result.json`

```jsonc
{
  "protocol": 1,
  "job_id": "6f1c9a3e",
  "status": "ok",                      // "ok" | "error"
  "engine": {                          // what actually ran; show this in the report header
    "solver": "1.0.0", "python": "3.13.14", "spreg": "1.9.1",
    "libpysal": "4.15.0", "numpy": "2.5.3", "pandas": "3.0.6", "scipy": "1.18.1"
  },
  "model": {
    "requested": "ML_Lag",
    "class": "spreg.ml_lag.ML_Lag",    // dotted path of the class that ran
    "family": "ml_lag",
    "title": "MAXIMUM LIKELIHOOD SPATIAL LAG (METHOD = LU)",
    "n": 49, "k": 4
  },

  "coefficients": {                   // row i of each vector belongs to names[i]
    "names":    ["CONSTANT", "INC", "CRIME", "W_HOVAL"],
    "roles":    ["exog", "exog", "exog", "lag"],
    "estimate": [39.6683, 0.5793, -0.4635, 0.1748],
    "std_err":  [14.3249, 0.5189, 0.1800, 0.1669],
    "z":        [2.7692, 1.1164, -2.5751, 1.0474],
    "p":        [0.00562, 0.26425, 0.01002, 0.29492]
  },
  "spatial_parameters": [            // convenience copies of the lag/error parameters
    { "name": "rho", "role": "lag", "estimate": 0.1748, "std_err": 0.1669, "z": 1.0474, "p": 0.29492 }
  ],

  "fit": { "n": 49, "k": 4, "logll": -200.880237, "aic": 409.760474,
           "schwarz": 417.327755, "sigma2": 211.531691, "pr2": 0.366798,
           "mean_y": 38.436224, "std_y": 18.466127 },

  "diagnostics": [
    { "group": "spatial dependence", "name": "lm_lag",  "label": "LM (lag)",
      "stat": 0.98155, "df": 1, "p": 0.32182 },
    { "group": "heteroskedasticity", "name": "jarque_bera", "label": "Jarque-Bera",
      "stat": 39.7062, "df": 2, "p": 2.3e-09 }
  ],

  "observations": {                    // only when outputs.observations == true
    "n": 49,
    "arrays": { "yhat": { "dtype": "f8", "shape": [49], "offset": 0,   "nbytes": 392 },
                "resid":{ "dtype": "f8", "shape": [49], "offset": 392, "nbytes": 392 } }
  },

  "impacts": [                         // present when the estimator reports them
    { "kind": "simple", "direct": 1.0, "indirect": 0.2119, "total": 1.2119 }
  ],

  "report": "report.txt",
  "warnings": ["GM_Combo: std_err not reported for row 4 (lambda); see report text"],
  "error":  { "type": "ValueError", "message": "...", "traceback": "...", "where": "estimating" }
}
```

Rules:

* `status` is `"error"` iff the run failed; `coefficients`/`fit`/… may then be
  absent. **GeoDa must handle a missing `result.json` too** (killed process,
  missing engine) — treat that as an error with `solver.log` as the message.
* `std_err`, `z`, `p` may contain `null` where the estimator does not report
  them for a given row (this happens: `GM_Combo` reports `lambda` without a
  standard error). GeoDa renders those cells blank and shows the warning.
* `coefficients.names` may be prefixed for regimes models (`0_INC`, `1_INC`)
  or for a coefficient that is global across regimes (`_Global_W_HOVAL`).
* `report.txt` is spreg's own `summary` text, verbatim. It is always written
  when the model has one, and it is the authority whenever a structured field
  is missing or was not extracted.

### 2.1 Results that are not a regression

`SKATER_reg` regionalises the observations instead of estimating coefficients,
so it returns no `coefficients` and no `fit`. Its result carries a `clusters`
section and puts the per-observation region id in `result.bin` as `region`:

```jsonc
{
  "clusters": { "n": 60, "n_regions": 3, "sizes": [21, 18, 21], "label_array": "region" },
  "observations": {
    "n": 60,
    "arrays": { "region": { "dtype": "i4", "shape": [60], "offset": 0, "nbytes": 240 } }
  }
}
```

GeoDa turns that into a new integer column in the table, which the map and the
other views can then use like any other categorical variable.

### 2.2 `result.bin`

Concatenated little-endian arrays described by `observations.arrays`, using the
same `{dtype, shape, offset, nbytes}` scheme as `data.bin`. `f8` is used for
predicted values, residuals and prediction errors; `i4` for region labels.
These feed GeoDa's *Save to Table*.

Keys currently used: `yhat` (`predy`), `resid` (`u`), `pred_err` (`e_pred`,
lag/error models only), `predy_e` (filtered prediction, where the estimator has
one) and `region` (`SKATER_reg`). Extra keys may appear; ignore what you do not
know.

---

## 3. Semantics that the solver owns

Putting these on the solver side keeps GeoDa's C++ free of econometric
conventions and lets them change with spreg:

1. **Constant term.** GeoDa never sends a constant column: spreg's own
   `check_constant` inserts one for the classical, ML, GMM, IV and probit
   families, so there the request is always honoured and `data.constant: false`
   only produces a warning. For regimes models the solver passes the request on
   as spreg's `constant_regi` (`"many"` when a constant is wanted, i.e. one per
   regime, `False` when it is not).
2. **Weight transform.** §1.2.
3. **Argument naming.** spreg's keyword names (`slx_lags`, `constant_regi`, …)
   never leak into the UI: the job carries GeoDa's option names, the registry
   maps them.
4. **Missing diagnostics.** A probed diagnostic that the estimator did not
   produce is simply absent, and `warnings` says so once.

---

## 4. Model registry

`solver/spreg_models.py` is the single source of truth. `solve.py --list-models`
prints it as JSON so that the C++ side can **build the model list and its option
widgets from the engine itself** instead of hard-coding them — the dialog stays
in sync with whatever spreg version is installed.

Each entry:

```jsonc
{
  "id": "ML_Lag",
  "label": "Spatial lag (ML)",
  "family": "ml_lag",
  "class": "spreg.ml_lag:ML_Lag",
  "requires": { "weights": true, "regimes": false, "endog": false, "coords": false },
  "options": {
    "method":       { "type": "enum", "values": ["full", "LU", "ord"], "default": "LU" },
    "slx_lags":     { "type": "int",  "default": 0, "min": 0, "max": 3 },
    "epsilon":      { "type": "float","default": 1e-7 },
    "spat_diag":    { "type": "bool", "default": true }
  }
}
```

Protocol v1 model ids (cross-sectional only):

| id | class | notes |
|----|-------|-------|
| `OLS` | `spreg.ols:OLS` | includes LM / Moran / Jarque-Bera / BP / KB / White |
| `OLS_Regimes` | `spreg.ols_regimes:OLS_Regimes` | regimes |
| `ML_Lag` | `spreg.ml_lag:ML_Lag` | `slx_lags=1` gives the spatial Durbin model |
| `ML_Error` | `spreg.ml_error:ML_Error` | |
| `ML_Lag_Regimes` | `spreg.ml_lag_regimes:ML_Lag_Regimes` | regimes, `regime_lag_sep` |
| `ML_Error_Regimes` | `spreg.ml_error_regimes:ML_Error_Regimes` | regimes, `regime_err_sep` |
| `GM_Error`, `GM_Error_Het`, `GM_Error_Hom` | `spreg.error_sp*` | robust / homoskedastic GMM |
| `GM_Endog_Error`(`_Het`/`_Hom`) | `spreg.error_sp*` | needs endogenous + instruments |
| `GM_Combo`(`_Het`/`_Hom`) | `spreg.error_sp*` | SARAR (lag + error) |
| `GMM_Error` | `spreg.error_sp:GMM_Error` | spreg's automatic GMM variant |
| `GM_Lag` | `spreg.twosls_sp:GM_Lag` | GS2SLS / spatial 2SLS |
| `TSLS` | `spreg.twosls:TSLS` | needs instruments (`q`) |
| `*_Regimes` variants | `spreg.*_regimes` | for every family above |
| `Probit` | `spreg.probit:Probit` | binary y, values must be 0/1 |
| `NSLX` | `spreg.nslx:NSLX` | nonlinear SLX; needs `coords` |
| `SKATER_reg` | `spreg.skater_reg:Skater_reg` | regression-based regionalization |

Deliberately **not** in v1: the panel estimators (`PooledOLS`, `PanelFE`,
`PanelRE`, `Panel_*_Lag/Error`, `GM_KKP`, `ML_*Pooled/*FE/*RE`) and the SUR
family (`SUR`, `ThreeSLS`, `SURlagIV`, `SURerrorGM`, `SURerrorML`). They require
stacked panel or multi-equation data, which GeoDa's table has no model for yet.
They can be added later as new model ids without a protocol break.

---

## 5. Versioning

* `protocol` is an integer and is checked first, in both directions:
  the solver refuses a job whose `protocol` it does not implement (exit 2), and
  GeoDa must refuse a `result.json` whose `protocol` differs (it means a stale
  engine directory from another GeoDa version).
* Additive changes (new model ids, new optional options, new diagnostics keys)
  do **not** bump the protocol. Removing or re-typing a field does — that is
  protocol 2.
* The engine directory name embeds the versions
  (`spreg-1.9.1-py313`), so two GeoDa versions can coexist without interfering,
  and `engine.spreg` in the result lets support staff see which engine produced
  a number.

---

## 6. Worked example

See `examples/write_job.cpp` (a complete, dependency-free C++ writer) and
`tools/make_demo_job.py` (the same thing from the Columbus sample). Both
produce a job directory that

```bash
"$ENGINE/bin/python3" solver/solve.py --job /path/to/geoda-spreg-1234-0
```

runs to completion, writing `result.json`, `result.bin` and `report.txt`.
