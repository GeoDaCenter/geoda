# The spreg engine for GeoDa

A second spatial regression backend for GeoDa, built on
[spreg](https://github.com/pysal/spreg), plus the machinery to get it onto a
user's machine without adding Python to the installer.

GeoDa keeps its own fast C++ engine (OLS, ML lag, ML error, and the diagnostics
that go with them) and the existing dialog, minus the parts that must change.
Everything GeoDa does not have today — regimes, spatial Durbin/SLX, the GMM and
IV families, probit, specification search, regression-based regionalization —
runs in a **separate process**, driven by a small file-based protocol.

```
GeoDa (C++/wx)                                   downloaded engine
──────────────                                   ─────────────────
Regression dialog                                 python/        standalone CPython 3.13
  │  builds a job directory                       licenses/      licences of the packages
  ├─ job.json  +  data.bin   ──▶  solve.py  ──▶
  │                                  │
  │  result.json + result.bin ◀──────┘
  │  report.txt
  ▼
RegressionReportDlg / Save to Table / Save to File
```

* **`PROTOCOL.md`** — the wire format: job.json, data.bin, result.json, the
  weights and constant conventions, versioning rules, the model registry.
* **`solver/`** — the solver the application ships and runs. Pure Python, no
  third-party imports beyond what the engine provides.
* **`solver/spreg_models.py`** — the model registry: ids, labels, required
  data, options, and how a spreg result becomes a protocol result.
* **`manifest/engines.json`** — what to download per platform, with sizes and
  sha256. Shipped inside the app.
* **`tools/build_engine.py`** — builds one engine archive (what CI runs).
* **`tools/check_models.py`** — runs every registered model; the gate for an
  engine artifact.
* **`tools/make_demo_job.py`** — writes a job from the Columbus sample; handy
  for smoke tests and for the parity work against the C++ engine.
* **`examples/write_job.cpp`** — the whole format written from C++, no
  dependencies.
* **`ci/spreg_engine.yml`** — four build jobs (macOS x2, Windows, Linux) that
  build, unpack and verify an archive, then attach it to a release.

Where these land in the GeoDa tree: `solver/` goes into the application bundle
(`Contents/Resources/spreg_engine/` on macOS, next to `GeoDa.exe` on Windows,
`/usr/share/geoda/spreg_engine/` on Linux) because it is code the app executes
and should be reviewed and versioned with the app. `manifest/` goes with it.
Nothing in the download is GeoDa code — the archive holds third-party Python
only, so fixing the protocol or the report never costs a re-download.

## Status

Prototype, and honest about it. What is actually verified on this machine
(macOS 14, arm64, spreg 1.9.1, Python 3.13.7):

* the engine archive builds, unpacks at an arbitrary path and runs there;
* the protocol selftest passes against the unpacked archive (`--selftest`),
  including a numerical anchor: OLS coefficients must match a direct
  least-squares solve to 1e-8;
* **all 33 registered models run** end to end (`tools/check_models.py`), from
  OLS to `GM_Combo_Hom_Regimes`, `Probit`, `NSLX` and `SKATER_reg`;
* a job written by the C++ example runs through the solver and comes back with
  a coefficient table;
* error paths behave: a protocol mismatch, an inconsistent array descriptor and
  a bad option value are each reported as a structured error with an
  actionable message.

Not verified here: the Windows and Linux archives (the CI matrix builds them,
but no one has run them yet), and — most importantly — **numerical parity with
GeoDa's own engine**, which needs GeoDa itself and is the first task of the
integration (see "Parity" below).

## Quick start

```bash
# needs uv on PATH (https://docs.astral.sh/uv) and a Python 3 to run the tools
# one engine archive for this machine, ~2 minutes cold, ~20 s warm
python3 tools/build_engine.py --outdir dist --manifest-entry dist/entry.json

# unpack it somewhere and run the protocol selftest with the engine's python
mkdir -p /tmp/engine && unzip -q dist/*.zip -d /tmp/engine
/tmp/engine/python/bin/python3 solver/solve.py --selftest

# every registered model, on synthetic data
/tmp/engine/python/bin/python3 tools/check_models.py

# a real data set, end to end
/tmp/engine/python/bin/python3 tools/make_demo_job.py --outdir /tmp/columbus \
    --model ML_Lag_Regimes --regime CRIME --run

# what the dialog can offer, generated from the engine itself
/tmp/engine/python/bin/python3 solver/solve.py --list-models
```

The C++ writer, if you want to see the format from the other side:

```bash
c++ -std=c++17 -o /tmp/write_job examples/write_job.cpp
/tmp/write_job /tmp/cppjob ML_Lag
/tmp/engine/python/bin/python3 solver/solve.py --job /tmp/cppjob
```

## Measured numbers

One machine, so treat them as orders of magnitude rather than promises.

| | |
|---|---|
| standalone CPython download | 24 MiB (python-build-standalone, fetched by `uv`) |
| engine archive, precompiled | **158.5 MB compressed, 474 MB unpacked** (12,728 files) |
| engine archive, `--no-bytecode` | 120.7 MB compressed, 376 MB unpacked (8,013 files) |
| build time | ~20 s warm, a couple of minutes cold (downloads ~100 MB of wheels) |
| install footprint in the user's data dir | one of the two above, per engine version |

Precompiled bytecode is the default: it makes the first estimation after an
install start in the same second as later ones instead of spending a few
seconds compiling numpy and friends. `--no-bytecode` trades 38 MB of download
for a slower first run; the choice is a one-line default in `build_engine.py`.

**Reproducibility.** The archive is built deterministically — sorted entries,
fixed timestamps, no build path in the payload — and the `--no-bytecode` variant
is byte-for-byte identical across independent builds (verified). With
precompiled bytecode, all but three of the 12,728 files are: three sklearn
`.pyc` files differ between runs because CPython's compiler marshals a
frozenset in a hash-seed-independent but run-dependent order. That is a
nicety, not a requirement: the manifest hash is produced by the same build that
produces the archive, so the digest always describes the bytes that ship, and
`--no-bytecode` is there if you want rebuilds to be verifiable end to end.
Note that `--build-dir` must stay fixed (it defaults to `<outdir>/.build`),
because the absolute path is baked into `_sysconfigdata` and into every `.pyc`;
the console scripts that would otherwise carry a build-path shebang are pruned,
so use `python3 -m pip` if you ever need pip inside an engine.

The dependency set cannot be trimmed much: libpysal hard-requires geopandas,
shapely, pyogrio, pyproj, requests, beautifulsoup4 and jinja2, and a real run
imports geopandas, shapely, pyproj, scikit-learn, pandas, scipy and bs4. The
interpreter itself is only about a quarter of the payload.

Runtime, from the earlier feasibility pass (KNN k=6, row-standardised), as a
warning about what *not* to put behind the new engine:

| n | OLS (+ diagnostics) | ML_Lag `LU` | ML_Lag `full` | ML_Lag `Ord` |
|---|---|---|---|---|
| 2,000 | 0.01 s | 0.4 s | | |
| 5,000 | 0.01 s | 2.8 s | 5.7 s | 28 s |
| 10,000 | 0.03 s | 12.7 s | 38.5 s | 241 s |
| 40,000 | 0.18 s | 479 s | | |

GeoDa's C++ engine answers these in well under a second because it uses the
characteristic-polynomial log-determinant approximation rather than
re-factorising. So keep the native path for OLS/ML lag/ML error and send only
the models GeoDa lacks to spreg — or accept the slowdown, but always expose
`method` and never default to `full` (at n=10,000 that is a dense 10,000²
matrix, about 800 MB).

## How GeoDa drives it

### 1. Install (or upgrade) the engine

1. Read `manifest/engines.json`, pick the entry for the running platform; the
   key is `macos-arm64`, `macos-x86_64`, `windows-x86_64`, `linux-x86_64`,
   `linux-aarch64`. Skip entries with `"status": "pending"`.
2. If the target directory `<user data>/GeoDa/engines/spreg-<spreg>-py<py>-<platform>/`
   already exists and contains a valid `engine.json` with a matching protocol,
   there is nothing to do.
3. Otherwise download the zip to a temporary file in the same parent directory
   (curl; `CURLOPT_FOLLOWLOCATION` on, because GitHub release assets redirect),
   with a progress dialog and a cancel button — `DialogTools/AutoUpdateDlg.cpp`
   already does exactly this and is the closest thing to copy.
4. Verify `sha256` against the manifest before touching it. A mismatch is a
   hard failure with the expected and actual digests shown.
5. Unpack with `wxZipInputStream` into `…-<platform>.tmp/`, then rename to
   `…-<platform>/`. Atomic, so an interrupted download never leaves a
   half-installed engine.
6. **Restore execute permissions.** wx does not preserve Unix modes, so after
   unpacking, `chmod +x` every path in the archive's `engine.json`
   `executables` list (that is why the list is in there). The archive itself
   stores mode 0755, so a command-line `unzip` needs no fixing up — this is
   only about the in-app path.
7. Probe: run `<engine>/python/bin/python3 <app>/…/spreg_engine/solver/solve.py --engine-info`
   with a short timeout. On failure, delete the directory, mark the engine
   unavailable and fall back to the built-in models — never leave the user
   with a dialog that cannot run anything.
8. Remember the path in the existing preferences, and offer "Remove the
   advanced engine" next to it. Old engine directories are deleted only after a
   newer one has validated.

Two conveniences worth having on day one: start the download in the background
as soon as the Regression dialog opens and the engine is missing (the built-in
models keep working meanwhile), and support **Installing from a file…** plus a
documented pre-seed directory (`<user data>/GeoDa/engines` is already searched)
for air-gapped and IT-managed machines.

### 2. Run a model

Build the job directory from the data the dialog already assembles —
`RegressionDlg::OnRunClick` already computes `undefs`, `valid_obs`, the
X matrix and a filtered `GalElement[]`; that block becomes the serializer, and
`examples/write_job.cpp` shows the two files it writes.

Then:

```
<engine>/python/bin/python3 <app>/…/spreg_engine/solver/solve.py --job <jobdir>
```

* GeoDa owns the job directory (system temp, `geoda-spreg-<pid>-<n>`), the
  timeout and the process lifetime; kill it on cancel. A killed process is
  detected by `result.json` being absent.
* The solver prints progress lines to stdout prefixed `[geoda-spreg]`, so a
  gauge can show "building weights", "estimating ML_Lag", "done in 0.3 s". It
  keeps everything spreg itself prints in `solver.log`, next to the job, so
  stdout stays parseable.
* On any failure `result.json` still exists with `status: "error"`, an
  `error.message` written for a user rather than a developer (spreg's own
  wording is usually good: *"There aren't enough observations for the given
  number of regimes and variables. Please check your regimes variable."*), and
  a full traceback in `solver.log`. Report the message in the dialog and offer
  the log directory.

### 3. Read the answer

`result.json` carries a coefficient table with names already rewritten to the
user's variables (`INC`, `W_INC`, `W_HOVAL`, `lambda`, and regime-prefixed names
such as `0_INC` when it is a regimes model), plus `fit`, `diagnostics`,
`spatial_parameters`, `impacts` where the estimator reports them, and
`observations` descriptors pointing into `result.bin` (predicted values,
residuals, prediction errors, region labels). Read it with json_spirit, the
same way `GdaJson.cpp` reads everything else, and `result.bin` with a plain
`std::ifstream`.

GeoDa renders its own report from those fields, so the existing report window,
*Save to File* and *Save to Table* keep working. `report.txt` holds spreg's own
`summary` text, which is a near neighbour of GeoDa's format (both descend from
Anselin's original SpaceStat layout) and is the right thing to show verbatim if
a structured field is missing, or as a first-cut display before the report
renderer is rewritten. Always print the engine and spreg version in the report
header: when a coefficient surprises someone, that is the first question.

### 4. Drive the dialog from the engine

`solve.py --list-models` prints the registry as JSON — ids, labels, families,
required data and every option with type, default, range and help text. Build
the model list and the option widgets from that instead of hard-coding them, and
the dialog cannot drift out of sync with the spreg version that is installed.

### 5. Use the user's own Python instead (optional)

For a machine with no internet but a wheelhouse, or a user who wants a specific
spreg build, the second mode is a venv off whatever Python is already there:

```
<their python> -m venv <user data>/GeoDa/engines/spreg-<ver>-venv
<venv>/bin/python -m pip install --only-binary=:all: --require-hashes \
    -r <app>/…/spreg_engine/solver/requirements.lock
```

Offer it as an advanced option, not the default. The failure modes are real and
they are the reason the downloaded engine exists:

* **Microsoft Store Python** is an App Execution Alias, not an interpreter, and
  behaves oddly from a GUI process; conda base environments have their own
  quirks. Validate candidates by actually running them (`-V`, then
  `import venv`), never by their path.
* **macOS `/usr/bin/python3`** is the Xcode CLT Python (3.9 today) and fails the
  ≥3.12 requirement; a Homebrew Python works but the user can upgrade or remove
  it later and silently break the venv.
* **Linux** distro Pythons are PEP 668 "externally managed": creating a venv is
  fine, installing into the system interpreter is not — never do that, and never
  reach for `--break-system-packages`.
* **Wheel coverage gaps**: Windows arm64 and musl/Alpine have spotty coverage.
  Detect and explain rather than compile from source.
* A corporate proxy or a broken pip configuration turns a five-second install
  into a support ticket; `requirements.lock` with hashes at least makes the
  result reproducible when it works.

## Parity with the built-in engine

Before any of this reaches users, prove that the two engines agree where they
overlap. Run the same data set and the same weights through both, and compare
coefficient by coefficient:

| Columbus (`HOVAL ~ INC + CRIME`, queen, row-standardised) | spreg | GeoDa C++ |
|---|---|---|
| OLS R² | 0.349514 | ? |
| OLS log-likelihood | −201.3677 | ? |
| ML lag ρ | 0.174847 | ? |
| ML error λ | 0.357823 | ? |

`tools/make_demo_job.py` writes exactly that job. Two conventions must be
matched deliberately, because they are places where the two engines can differ
without either being wrong: GeoDa's current ML path converts the weights to a
binary row-standardised W and ignores GWT values (the job's
`weights.transform` selects this), and the lag operator convention (W as stored
versus its transpose) has to be the same on both sides. Everything else follows
from spreg's `OLS` being ordinary least squares.

One output difference to expect, since it shapes the report renderer: spreg's
ML lag and ML error classes do not expose the LM / Moran / LR diagnostics as
result fields (only the classical models do), so those rows in GeoDa's current
lag and error reports have nothing behind them. The structured result simply
omits them, and the renderer must drop rows it has no value for rather than
printing blanks; `diagnostics` is empty for those models and 15 entries for
`OLS` on the same data.

## Adding a model, or moving to a new spreg

* **New model**: one entry in `MODELS` in `solver/spreg_models.py` (id, label,
  family, class, call style, options, required data) — then
  `tools/check_models.py --models <id>` until it is green. The registry is the
  only file that knows about spreg's API, and `--list-models` carries the change
  to the UI by itself. No protocol change, so no engine or app version bump.
* **New spreg**: edit `solver/requirements.in`, regenerate the lock
  (`uv pip compile --universal --generate-hashes`), rebuild, run
  `check_models.py`, and fix whatever spreg changed — which is exactly what the
  registry and the sweep exist for. Then bump the manifest and publish a new
  `spreg-engine-vN` release; the engine directory name carries the version, so
  old and new coexist.

## Deliberately out of scope for v1

The panel estimators (`PooledOLS`, `PanelFE`, `PanelRE`, `Panel_FE_Lag`,
`Panel_RE_Error`, `GM_KKP`, `ML_LagFE`/`RE`, …) and the SUR family (`SUR`,
`ThreeSLS`, `SURlagIV`, `SURerrorGM`, `SURerrorML`). They need stacked panel or
multi-equation data, and GeoDa's table has no time dimension. They can be added
later as new model ids without touching the protocol — the interesting part of
a panel UI is the data model, not the estimator.
