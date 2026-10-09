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
* **`.github/workflows/spreg_engine.yml`** — four build jobs (macOS x2, Windows,
  Linux) that build an archive, unpack it and verify it with the interpreter it
  contains, then attach the archives to a release; a parity job compares the
  published engine against GeoDa's own.

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

### 0. What an engine carries

The archive holds the interpreter and the wheel set in `solver/requirements.lock`,
which is the same for every platform except where a wheel does not exist: no
release of `pyogrio` has a `win_arm64` wheel, and `build_engine.py` installs with
`--only-binary`, so `windows-arm64` is built with `--skip pyogrio`.  That is safe
because the solver never reads a file with it - the job carries its data as
arrays and spreg only reaches for geopandas' file layer when a *user* asks it to
read one, which the engine is never asked to do.  An engine built that way names
what it left out in its own `engine.json`, so it can be asked.

### 1. Install (or upgrade) the engine

1. Read `manifest/engines.json`, pick the entry for the running platform; the
   key is `macos-arm64`, `macos-x86_64`, `windows-x86_64`, `windows-arm64`,
   `linux-x86_64`, `linux-aarch64`. Skip entries with `"status": "pending"`.
   Which of these exist follows what GeoDa itself ships: there is an arm64
   installer for both macOS and Windows, so both get an engine, while GeoDa's
   Linux builds are x86_64 only and `linux-aarch64` stays a placeholder. A
   platform with no entry is not an error - the dialog says there is no engine
   published for it yet and the application falls back to its own models.
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

### Which tests a model has

The tests come from the estimators, and spreg does not give every model the same
set - its notebooks (5, 9, 13, 15) are the reference for this, and the registry
follows them:

| model | tests |
|-------|-------|
| `OLS`, `OLS_Regimes` | Jarque-Bera, Breusch-Pagan, Koenker-Bassett and the F statistic (`nonspat_diag`); LM tests for lag, error and their robust forms, SARMA, spatial Durbin and WX (`spat_diag`); Moran's I (`moran`); the White test (`white_test`, which spreg computes only when asked) |
| `ML_Lag`, `ML_Error` | the likelihood ratio test of rho or lambda = 0 - spreg's ML classes carry no test of their own, so the solver forms it with `spreg.diagnostics.likratiotest` against an OLS on the same data, as notebooks 13 and 15 do |
| `GM_Lag`, `GM_Lag_Regimes`, `TSLS`, `TSLS_Regimes` | the Anselin-Kelejian test on the residual of the IV/GMM fit (`spat_diag`) |
| the GMM error and SARAR families, `Probit`, `NSLX`, `SKATER_reg` | none: spreg offers no post-estimation tests for them, so their options do not claim any |

A model that has no tests has no test options, and the report leaves the section
out rather than printing it empty. `diagnostics` is empty for those models and 16
entries for `OLS` on the same data.

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

## The application side

Where the pieces live in a built GeoDa:

| | |
|---|---|
| `Regression/SpregEngine.{h,cpp}` | discovery, manifest, download, checksum, unpack, install, remove, run |
| `Regression/SpregSha256.h` | SHA-256, header only, no new dependency |
| `DialogTools/SpregEngineDlg.{h,cpp}` | the **Install Spreg** dialog: what is being downloaded, how far it has got, and what to do when it fails |
| the **Install Spreg** button | in the Regression dialog, under the models it unlocks |
| the installed engine | `<user data>/GeoDa/engines/` — macOS `~/Library/Application Support/GeoDa/engines`, Windows `%APPDATA%\GeoDa\engines`, Linux `~/.geoda/engines` |
| the shipped solver and manifest | macOS `GeoDa.app/Contents/Resources/spreg_engine/`, elsewhere `<exe dir>/spreg_engine/` |

Decisions worth knowing:

* The engine is installed from the Regression dialog, not from a menu of its
  own: it only means anything next to the models it provides, and a separate
  "engine manager" invited questions about what it was for. The button and its
  status line are added to the dialog's Models box in code rather than in
  `dialogs.xrc`, because regenerating `rc/GdaAppResources.cpp` with a different
  `wxrc` version rewrites thirteen thousand lines of it.  Once an engine is
  installed the button disappears and the line reads *spreg 1.9.1 ready - the
  advanced models are available*; that is where the controls of the coming
  models will be enabled.
* The model list comes out of the engine, and starting the engine costs a
  second or two, so the Regression dialog fetches it *after* it is on screen
  rather than while it is being built, and the answer is kept in the engine
  directory (`model_list.json`) from then on.  Installing an engine fills that
  cache, so the dialog opens in about fifty milliseconds and the list is there
  a moment later; measured: 49 ms to build the dialog, 78 ms to fill the list
  from the cache, against about 1.6 s when it has to ask the engine.
* The variables some of the engine's models need are collected where they are
  needed: choosing a model with regimes reveals a picker for the regime
  variable, choosing one with endogenous variables reveals the two lists for them
  and for their instruments, and choosing one that builds its weights from
  coordinates - which then needs no GeoDa weights at all - reveals a pair of
  coordinate pickers.  Which row appears is decided by the model registry, not by
  a list written here.
* *Save to Table* works for the engine's models too: predicted values,
  residuals and (where the model reports them) prediction errors, as
  `SPR_PREDIC`, `SPR_RESIDU` and `SPR_PRDERR` - `SPR_` because a shapefile field
  name is ten characters.
* One build-system wrinkle, for whoever works on the solver: the CMake build
  copies `solver/` into the application only after the application itself
  relinks, so an edit to the Python alone does not reach the running
  application - touch a source file or copy the directory by hand.  The make
  based release builds always copy.
* Installing is the only thing the dialog does.  There is no uninstall and no
  install-from-file in the interface: those are administrator jobs, and the
  README describes them - delete `<user data>/GeoDa/engines/spreg-*` to remove
  an engine, and for an offline or pre-seeded installation unpack the archive
  into that directory yourself, or point `GEODA_SPREG_ENGINES` at a directory
  you have prepared.
* The engine archives have to be published before the button can work: the
  manifest points at a release asset under `GeoDaCenter/software`, and until
  the release exists the download answers 404 - which the dialog now says in so
  many words, including the address it tried.
* `spreg_engine.yml`'s release job publishes them there, so a tag is all it
  takes: `SPREG_ENGINE` is built by the four build jobs, and on `spreg-engine-v<N>`
  the archives are attached to a release of that name in **GeoDaCenter/software**
  and the manifest entries are printed for pasting in.  Writing to that repository
  needs a token of its own, because the workflow token may only write the
  repository the workflow runs in: keep a fine-grained token with
  *Contents: read and write* on `GeoDaCenter/software` as the
  `SOFTWARE_RELEASE_TOKEN` secret of this repository.  Without it the job stops
  before it publishes anything and says so.
* `GEODA_SPREG_ENGINES` overrides the engine directory. That is for
  administrators who prepare machines, multi-user installs, and the headless
  test below; it is also the documented pre-seed path for air-gapped sites.
* The engine never touches the system: it lives in the user's own directory and
  the same dialog removes it. GeoDa's own engine and installer are untouched.
* Installs are atomic. The archive is unpacked into `<engine>.tmp-<pid>` and
  renamed into place, the previous engine is kept until the new one is in
  place, and a failed download or a mismatched checksum leaves everything as it
  was.
* An archive that tries to write outside its own directory (a `../` entry, an
  absolute path) is refused, and the part of it that was extracted is deleted.
* GeoDa waits for the file the solver writes rather than for its exit code:
  `wx` only delivers exit codes through the event loop, which is not running
  while a dialog is blocked waiting.
* `Regression/` and `DialogTools/` are compiled by glob on macOS and Linux, so
  no makefile changes were needed there; MSVC (three projects) and Xcode (both
  projects) keep explicit lists and were updated. When the CMake build lands,
  its `GEODA_RUNTIME_FILES`/`copy_directory` block needs the same two
  directories, next to the existing `web_plugins` copy.

### Tests, without building the application

```bash
tools/test_sha256.cpp                  # digest vectors, cross-checked with shasum
tools/run_engine_manager_test.sh       # verify / unpack / install / discover / upgrade / remove
tools/run_parity_test.sh [engine-dir]  # GeoDa's own engine against spreg, same data, same weights
tools/syntax_check.py <sources>        # type-check against the flags CMake used
```

`run_parity_test.sh` answers the question the whole exercise rests on: do the two
engines agree?  It builds only the two engines - the C++ one from
`Regression/`, the other through the solver - and runs OLS, ML lag and ML error
over the same grid with the same weights.  They agree to 1e-8 or better on every
coefficient and on the log-likelihood; `.github/workflows/spreg_engine.yml` runs the same
comparison on every engine it builds.  One trap is written into the test: the
C++ engine *absorbs* the arrays it is handed, so each model gets its own copy -
sharing one set of arrays made the second and third models disagree wildly, which
is how a first version of it failed.

`run_engine_manager_test.sh` is the other one that matters: it links only
`SpregEngine.cpp` and a small harness, and runs the real installer against a
real archive in a scratch directory (build one first with
`tools/build_engine.py --outdir dist`) — including a corrupted download, an archive
with a `../` entry, installing twice over itself, and removing something that is
not an engine.

### Still to come

Rendering the structured result into GeoDa's report, *Save to Table* and *Save
to File*; building the job directory from `RegressionDlg`'s existing data
preparation; the model list and option widgets from `solve.py --list-models`;
and the parity harness against the built-in engine.

## Deliberately out of scope for v1

The panel estimators (`PooledOLS`, `PanelFE`, `PanelRE`, `Panel_FE_Lag`,
`Panel_RE_Error`, `GM_KKP`, `ML_LagFE`/`RE`, …) and the SUR family (`SUR`,
`ThreeSLS`, `SURlagIV`, `SURerrorGM`, `SURerrorML`). They need stacked panel or
multi-equation data, and GeoDa's table has no time dimension. They can be added
later as new model ids without touching the protocol — the interesting part of
a panel UI is the data model, not the estimator.
