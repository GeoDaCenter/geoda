"""spreg model registry and result extraction for the GeoDa solver.

This module is the single source of truth for

  * which spreg estimators GeoDa can ask for (model ids, labels, families);
  * what data each one needs (weights / regimes / endogenous / instruments /
    coordinates);
  * which options each one accepts, their types, defaults and the spreg keyword
    they map onto;
  * how to turn a spreg result object into the protocol result structure.

GeoDa never hard-codes any of this: ``solve.py --list-models`` dumps it as JSON
so the C++ side can build its model list and option widgets from the engine that
is actually installed.  Adding a model is a change to this file only.

Deliberately excluded from protocol v1: the panel estimators (``PooledOLS``,
``PanelFE``, ``PanelRE``, ``Panel_FE_Lag``/``_Error``, ``Panel_RE_Lag``/``_Error``,
``GM_KKP``, ``ML_LagFE``/``RE``, ...) and the SUR family (``SUR``, ``ThreeSLS``,
``SURlagIV``, ``SURerrorGM``, ``SURerrorML``).  They need stacked panel or
multi-equation data, which GeoDa's table has no model for.  Also excluded are
the clustering-based ``TSLS_Endog_Regimes`` / ``GM_Lag_Endog_Regimes``, whose
constructor builds regimes from an n_clusters argument instead of taking a
regime variable - a different UI concept, to be added in a later revision.
"""

from __future__ import annotations

import math
import re
from typing import Any, Dict, List, Optional, Tuple

# spreg's internal coefficient names: var_<i>, W_var_<i>, W_dep_var, lambda,
# optionally prefixed by a regime id ("0_var_1") or by "_Global_".

# --------------------------------------------------------------------------
# option specs
# --------------------------------------------------------------------------
# type:  bool | int | float | str | enum | intlist | boolist
# arg:   the spreg keyword the value is passed as (defaults to the option name)
# enum values are validated; enum may carry "map" to translate to spreg values
# (e.g. "auto" -> None).

OPT = {
    "robust": {"type": "enum", "values": ["none", "white", "het"], "default": "none",
               "map": {"none": None}, "help": "robust standard errors"},
    "slx_lags": {"type": "int", "default": 0, "min": 0, "max": 3,
                 "help": "spatial lags of X (1 = spatial Durbin / SLX)"},
    "slx_vars": {"type": "str", "default": "all", "help": "'all' or a list of X names"},
    "sig2n_k": {"type": "bool", "default": True, "help": "sigma^2 = u'u/(n-k)"},
    "nonspat_diag": {"type": "bool", "default": True, "help": "Jarque-Bera, BP, KB"},
    "spat_diag": {"type": "bool", "default": True, "help": "LM tests for spatial dependence"},
    "moran": {"type": "bool", "default": True, "help": "Moran's I on the residuals"},
    "white_test": {"type": "bool", "default": False, "help": "White heteroskedasticity test"},
    "method": {"type": "enum", "values": ["LU", "full", "ord"], "default": "LU",
               "help": "log-Jacobian method: LU (sparse), full (dense), ord (eigenvalues)"},
    "epsilon": {"type": "float", "default": 1e-7, "help": "convergence tolerance"},
    "spat_impacts": {"type": "enum", "values": ["none", "simple", "full", "all"],
                     "default": "simple", "map": {"none": None},
                     "help": "direct/indirect/total impacts"},
    "constant_regi": {"type": "enum", "values": ["many", "one"], "default": "many",
                      "help": "one constant per regime, or one overall"},
    "cols2regi": {"type": "str", "default": "all",
                  "help": "'all' or a list of booleans, one per X variable"},
    "regime_lag_sep": {"type": "bool", "default": False, "help": "regime-specific lag coefficient"},
    "regime_err_sep": {"type": "bool", "default": True, "help": "separate error process per regime"},
    "cores": {"type": "bool", "default": False, "help": "use all cores"},
    "max_iter": {"type": "int", "default": 1, "min": 1},
    "step1c": {"type": "bool", "default": False},
    "w_lags": {"type": "int", "default": 1, "min": 1, "max": 3,
               "help": "number of lags used to build the instruments"},
    "lag_q": {"type": "bool", "default": True, "help": "also lag the additional instruments"},
    "optim": {"type": "enum", "values": ["newton", "bfgs", "ncg"], "default": "newton",
              "help": "optimizer"},
    "maxiter": {"type": "int", "default": 100, "min": 1, "help": "optimizer iterations"},
    "scalem": {"type": "enum", "values": ["phimean", "xmean"], "default": "phimean",
               "help": "scale of the marginal effects"},
    "predflag": {"type": "bool", "default": True, "help": "print the prediction table"},
    "bstart": {"type": "bool", "default": False, "help": "use the OLS estimates as a start"},
    "distance_metric": {"type": "enum", "values": ["Euclidean", "Arc"], "default": "Euclidean"},
    "leafsize": {"type": "int", "default": 30, "min": 1},
    "var_flag": {"type": "enum", "values": ["analytic", "numeric"], "default": "analytic",
                 "map": {"analytic": 1, "numeric": 0}},
    "verbose": {"type": "bool", "default": False},
    "n_clusters": {"type": "int", "default": 5, "min": 2},
    "quorum": {"type": "int", "default": -1},
    "model_family": {"type": "enum", "values": ["spreg", "statsmodels"], "default": "spreg",
                     "help": "regression used to score the regions"},
}

# --------------------------------------------------------------------------
# the registry
# --------------------------------------------------------------------------
# call styles, i.e. how the estimator is invoked:
#   standard        cls(y, x, w=w, **kw)
#   regimes         cls(y, x, regimes, w=w, **kw)
#   endog           cls(y, x, yend, q, w=w, **kw)
#   endog_regimes_a cls(y, x, yend, q, regimes, w=w, **kw)
#   endog_regimes_b cls(y, x, regimes, yend, q, w=w, **kw)
#   nslx            cls(y, x, coords, **kw)
#   skater          cls().fit(n_clusters=..., W=w, data=x, **kw)

def _m(model_id, label, family, cls, style="standard", options=(), requires=None, notes=""):
    req = {"weights": True, "regimes": False, "endog": False, "instruments": False, "coords": False}
    if requires:
        req.update(requires)
    return {
        "id": model_id,
        "label": label,
        "family": family,
        "class": cls,
        "style": style,
        "options": {name: dict(OPT[name]) for name in options},
        "requires": req,
        "notes": notes,
    }


_REGI = ("constant_regi", "cols2regi")

MODELS: List[Dict[str, Any]] = [
    # ---- classical -------------------------------------------------------
    _m("OLS", "Classical (OLS)", "classical", "spreg.ols:OLS",
       options=("robust", "slx_lags", "sig2n_k", "nonspat_diag", "spat_diag", "moran", "white_test"),
       notes="weights are required by spreg even for a purely non-spatial OLS"),
    _m("OLS_Regimes", "Classical (OLS), by regime", "classical", "spreg.ols_regimes:OLS_Regimes",
       style="regimes",
       options=("robust", "slx_lags", "sig2n_k", "nonspat_diag", "spat_diag", "moran",
                "white_test") + _REGI,
       requires={"regimes": True}),
    # ---- ML --------------------------------------------------------------
    _m("ML_Lag", "Spatial lag (ML)", "ml_lag", "spreg.ml_lag:ML_Lag",
       options=("method", "epsilon", "slx_lags", "spat_impacts", "spat_diag"),
       notes="slx_lags=1 gives the spatial Durbin model"),
    _m("ML_Error", "Spatial error (ML)", "ml_error", "spreg.ml_error:ML_Error",
       options=("method", "epsilon", "slx_lags")),
    _m("ML_Lag_Regimes", "Spatial lag (ML), by regime", "ml_lag", "spreg.ml_lag_regimes:ML_Lag_Regimes",
       style="regimes",
       options=("method", "epsilon", "slx_lags", "regime_lag_sep", "cores", "spat_diag") + _REGI,
       requires={"regimes": True}),
    _m("ML_Error_Regimes", "Spatial error (ML), by regime", "ml_error",
       "spreg.ml_error_regimes:ML_Error_Regimes", style="regimes",
       options=("method", "epsilon", "slx_lags", "regime_err_sep", "regime_lag_sep", "cores")
               + _REGI,
       requires={"regimes": True}),
    # ---- GMM / IV, no endogenous variables -------------------------------
    _m("GM_Error", "Spatial error (GMM)", "gmm_error", "spreg.error_sp:GM_Error"),
    _m("GM_Error_Het", "Spatial error (GMM, heteroskedastic)", "gmm_error",
       "spreg.error_sp_het:GM_Error_Het", options=("max_iter", "epsilon", "step1c")),
    _m("GM_Error_Hom", "Spatial error (GMM, homoskedastic)", "gmm_error",
       "spreg.error_sp_hom:GM_Error_Hom", options=("max_iter", "epsilon")),
    _m("GMM_Error", "Spatial error (GMM, automatic variant)", "gmm_error",
       "spreg.error_sp:GMM_Error", options=("slx_lags",)),
    # ---- GMM / IV with endogenous variables ------------------------------
    _m("GM_Endog_Error", "Spatial error (GMM), endogenous X", "gmm_error",
       "spreg.error_sp:GM_Endog_Error", style="endog", options=("slx_lags",),
       requires={"endog": True, "instruments": True}),
    _m("GM_Endog_Error_Het", "Spatial error (GMM, het), endogenous X", "gmm_error",
       "spreg.error_sp_het:GM_Endog_Error_Het", style="endog",
       options=("max_iter", "epsilon", "step1c", "slx_lags"),
       requires={"endog": True, "instruments": True}),
    _m("GM_Endog_Error_Hom", "Spatial error (GMM, hom), endogenous X", "gmm_error",
       "spreg.error_sp_hom:GM_Endog_Error_Hom", style="endog",
       options=("max_iter", "epsilon", "slx_lags"),
       requires={"endog": True, "instruments": True}),
    _m("GM_Combo", "Spatial lag and error (GMM, SARAR)", "gmm_combo",
       "spreg.error_sp:GM_Combo", style="endog",
       options=("w_lags", "lag_q", "slx_lags"),
       requires={"endog": False, "instruments": False},
       notes="endogenous X and instruments are optional; W_X is used as instruments"),
    _m("GM_Combo_Het", "Spatial lag and error (GMM, het, SARAR)", "gmm_combo",
       "spreg.error_sp_het:GM_Combo_Het", style="endog",
       options=("w_lags", "lag_q", "slx_lags", "max_iter", "epsilon", "step1c")),
    _m("GM_Combo_Hom", "Spatial lag and error (GMM, hom, SARAR)", "gmm_combo",
       "spreg.error_sp_hom:GM_Combo_Hom", style="endog",
       options=("w_lags", "lag_q", "slx_lags", "max_iter", "epsilon")),
    _m("GM_Lag", "Spatial lag (GS2SLS / spatial 2SLS)", "gm_lag",
       "spreg.twosls_sp:GM_Lag", style="endog",
       options=("w_lags", "lag_q", "slx_lags", "robust", "sig2n_k", "spat_diag"),
       requires={"endog": False, "instruments": False},
       notes="endogenous X and instruments are optional; W_X is used as instruments"),
    _m("TSLS", "Two stage least squares (2SLS)", "tsls", "spreg.twosls:TSLS",
       style="endog",
       options=("robust", "slx_lags", "sig2n_k", "spat_diag", "nonspat_diag"),
       requires={"endog": True, "instruments": True}),
    # ---- regimes variants of GMM / IV ------------------------------------
    _m("GM_Error_Regimes", "Spatial error (GMM), by regime", "gmm_error",
       "spreg.error_sp_regimes:GM_Error_Regimes", style="regimes",
       options=_REGI + ("regime_err_sep", "regime_lag_sep", "slx_lags"),
       requires={"regimes": True}),
    _m("GM_Error_Het_Regimes", "Spatial error (GMM, het), by regime", "gmm_error",
       "spreg.error_sp_het_regimes:GM_Error_Het_Regimes", style="regimes",
       options=_REGI + ("regime_err_sep", "regime_lag_sep", "slx_lags", "max_iter", "epsilon",
                        "step1c"),
       requires={"regimes": True}),
    _m("GM_Error_Hom_Regimes", "Spatial error (GMM, hom), by regime", "gmm_error",
       "spreg.error_sp_hom_regimes:GM_Error_Hom_Regimes", style="regimes",
       options=_REGI + ("regime_err_sep", "regime_lag_sep", "slx_lags"),
       requires={"regimes": True}),
    _m("GM_Endog_Error_Regimes", "Spatial error (GMM), endogenous X, by regime", "gmm_error",
       "spreg.error_sp_regimes:GM_Endog_Error_Regimes", style="endog_regimes_a",
       options=_REGI + ("regime_err_sep", "regime_lag_sep", "slx_lags", "cores"),
       requires={"regimes": True, "endog": True, "instruments": True}),
    _m("GM_Endog_Error_Het_Regimes", "Spatial error (GMM, het), endogenous X, by regime",
       "gmm_error", "spreg.error_sp_het_regimes:GM_Endog_Error_Het_Regimes",
       style="endog_regimes_a",
       options=_REGI + ("regime_err_sep", "regime_lag_sep", "slx_lags", "max_iter", "epsilon",
                        "step1c"),
       requires={"regimes": True, "endog": True, "instruments": True}),
    _m("GM_Endog_Error_Hom_Regimes", "Spatial error (GMM, hom), endogenous X, by regime",
       "gmm_error", "spreg.error_sp_hom_regimes:GM_Endog_Error_Hom_Regimes",
       style="endog_regimes_a",
       options=_REGI + ("regime_err_sep", "regime_lag_sep", "slx_lags"),
       requires={"regimes": True, "endog": True, "instruments": True}),
    _m("GM_Combo_Regimes", "Spatial lag and error (GMM, SARAR), by regime", "gmm_combo",
       "spreg.error_sp_regimes:GM_Combo_Regimes", style="endog_regimes_b",
       options=_REGI + ("regime_err_sep", "regime_lag_sep", "slx_lags", "w_lags", "lag_q", "cores"),
       requires={"regimes": True}),
    _m("GM_Combo_Het_Regimes", "Spatial lag and error (GMM, het, SARAR), by regime", "gmm_combo",
       "spreg.error_sp_het_regimes:GM_Combo_Het_Regimes", style="endog_regimes_b",
       options=_REGI + ("regime_err_sep", "slx_lags", "w_lags", "lag_q", "max_iter", "epsilon"),
       requires={"regimes": True}),
    _m("GM_Combo_Hom_Regimes", "Spatial lag and error (GMM, hom, SARAR), by regime", "gmm_combo",
       "spreg.error_sp_hom_regimes:GM_Combo_Hom_Regimes", style="endog_regimes_b",
       options=_REGI + ("regime_err_sep", "slx_lags", "w_lags", "lag_q", "cores", "max_iter"),
       requires={"regimes": True}),
    _m("GMM_Error_Regimes", "Spatial error (GMM, automatic), by regime", "gmm_error",
       "spreg.error_sp_regimes:GMM_Error_Regimes", style="endog_regimes_c",
       options=_REGI + ("regime_err_sep", "regime_lag_sep"),
       requires={"regimes": True}),
    _m("GM_Lag_Regimes", "Spatial lag (GS2SLS), by regime", "gm_lag",
       "spreg.twosls_sp_regimes:GM_Lag_Regimes", style="endog_regimes_b",
       options=_REGI + ("slx_lags", "w_lags", "lag_q", "robust", "sig2n_k"),
       requires={"regimes": True}),
    _m("TSLS_Regimes", "Two stage least squares, by regime", "tsls",
       "spreg.twosls_regimes:TSLS_Regimes", style="endog_regimes_a",
       options=_REGI + ("slx_lags", "sig2n_k", "spat_diag"),
       requires={"regimes": True, "endog": True, "instruments": True}),
    # ---- binary dependent variable ---------------------------------------
    _m("Probit", "Probit (binary dependent variable)", "probit", "spreg.probit:Probit",
       options=("slx_lags", "optim", "maxiter", "scalem", "predflag"),
       notes="y must contain only 0 and 1"),
    # ---- non-linear SLX --------------------------------------------------
    _m("NSLX", "Non-linear SLX (distance decay)", "nslx", "spreg.nslx:NSLX", style="nslx",
       options=("distance_metric", "leafsize", "var_flag", "verbose"),
       requires={"coords": True, "weights": False},
       notes="uses coordinates; W and its transform are ignored"),
    # ---- regionalization -------------------------------------------------
    _m("SKATER_reg", "SKATER with regression-based regions", "skater",
       "spreg.skater_reg:Skater_reg", style="skater",
       options=("n_clusters", "quorum", "model_family"),
       notes="clusters the observations, not a regression on regions"),
]

MODELS_BY_ID = {m["id"]: m for m in MODELS}


def list_models() -> List[Dict[str, Any]]:
    """The registry as plain JSON-able data (protocol `--list-models`)."""
    return MODELS


def get_model(model_id: str) -> Dict[str, Any]:
    try:
        return MODELS_BY_ID[model_id]
    except KeyError:
        raise KeyError(
            "unknown model %r; known models: %s" % (model_id, ", ".join(sorted(MODELS_BY_ID)))
        )


# --------------------------------------------------------------------------
# options
# --------------------------------------------------------------------------

def _coerce(name: str, spec: Dict[str, Any], value: Any):
    t = spec["type"]
    if t == "bool":
        if not isinstance(value, bool):
            raise ValueError("option %s must be true/false" % name)
        return value
    if t == "int":
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise ValueError("option %s must be an integer" % name)
        v = int(value)
        if "min" in spec and v < spec["min"]:
            raise ValueError("option %s must be >= %d" % (name, spec["min"]))
        if "max" in spec and v > spec["max"]:
            raise ValueError("option %s must be <= %d" % (name, spec["max"]))
        return v
    if t == "float":
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise ValueError("option %s must be a number" % name)
        return float(value)
    if t == "enum":
        if value not in spec["values"]:
            raise ValueError("option %s must be one of %s" % (name, "/".join(spec["values"])))
        return spec.get("map", {}).get(value, value)
    if t in ("str", "intlist", "boolist"):
        if isinstance(value, (list, str)):
            return value
        raise ValueError("option %s must be a string or list" % name)
    raise ValueError("option %s has an unsupported type" % name)


def validate_options(entry: Dict[str, Any], options: Dict[str, Any]) -> Tuple[Dict[str, Any], List[str]]:
    """Return (kwargs for the estimator, warnings).  Raises on bad input."""
    specs = entry["options"]
    unknown = sorted(set(options) - set(specs))
    if unknown:
        raise ValueError(
            "model %s does not accept option(s) %s; it accepts %s"
            % (entry["id"], ", ".join(unknown), ", ".join(sorted(specs)) or "no options")
        )
    kwargs: Dict[str, Any] = {}
    warnings: List[str] = []
    for name, spec in specs.items():
        if name in options:
            value = _coerce(name, spec, options[name])
        else:
            value = spec["default"]
            if spec["type"] == "enum":                # defaults go through the same map
                value = spec.get("map", {}).get(value, value)
        if value is None:
            continue                          # "map" turned it into None: leave spreg's default
        arg = spec.get("arg", name)
        if arg == "slx_vars" and isinstance(value, str) and value.strip().lower() == "all":
            continue
        if arg == "cols2regi" and isinstance(value, str) and value.strip().lower() == "all":
            continue
        kwargs[arg] = value
    return kwargs, warnings


# --------------------------------------------------------------------------
# diagnostics
# --------------------------------------------------------------------------
# (attribute, label, group, shape)
#   stat_p    : tuple (statistic, p)                     -> df = 1
#   moran     : tuple (I, z, p)
#   df_stat_p : dict with 'df' plus a statistic and a p-value under varying keys
#   impacts   : dict {kind: (direct, indirect, total)}

DIAGNOSTICS = [
    ("lm_error", "LM (error)", "spatial dependence", "stat_p"),
    ("lm_lag", "LM (lag)", "spatial dependence", "stat_p"),
    ("rlm_error", "Robust LM (error)", "spatial dependence", "stat_p"),
    ("rlm_lag", "Robust LM (lag)", "spatial dependence", "stat_p"),
    ("rlm_durlag", "Robust LM (lag, Durbin)", "spatial dependence", "stat_p"),
    ("lm_sarma", "LM (SARMA)", "spatial dependence", "stat_p"),
    ("lm_wx", "LM (WX)", "spatial dependence", "stat_p"),
    ("rlm_wx", "Robust LM (WX)", "spatial dependence", "stat_p"),
    ("lm_spdurbin", "LM (spatial Durbin)", "spatial dependence", "stat_p"),
    ("lm_slxerr", "LM (SLX error)", "spatial dependence", "stat_p"),
    ("moran_res", "Moran's I (residuals)", "spatial dependence", "moran"),
    ("ak_test", "Anselin-Kelejian test", "spatial dependence", "stat_p"),
    ("white", "White test", "heteroskedasticity", "df_stat_p"),
    ("breusch_pagan", "Breusch-Pagan", "heteroskedasticity", "df_stat_p"),
    ("koenker_bassett", "Koenker-Bassett", "heteroskedasticity", "df_stat_p"),
    ("jarque_bera", "Jarque-Bera", "normality", "df_stat_p"),
    ("f_stat", "F statistic", "fit", "stat_p"),
    ("lr", "Likelihood ratio test", "spatial dependence", "stat_p"),
    ("llr", "Log-likelihood ratio", "fit", "stat_p"),
    ("chow", "Chow test", "regimes", "chow"),
    ("wald", "Wald test", "regimes", "chow"),
]


def _py(value: Any) -> Any:
    """Convert numpy scalars/arrays to plain Python, recursively, JSON-safe."""
    if value is None:
        return None
    if isinstance(value, bool):
        return value
    if isinstance(value, (int, str)):
        return value
    if isinstance(value, float):
        return None if (math.isnan(value) or math.isinf(value)) else value
    if hasattr(value, "shape"):                      # numpy scalar or array
        try:
            flat = value.ravel().tolist() if hasattr(value, "ravel") else [value.item()]
        except Exception:
            return None
        if len(flat) == 1:
            return _py(flat[0])
        return [_py(v) for v in flat]
    if isinstance(value, (list, tuple)):
        return [_py(v) for v in value]
    if isinstance(value, dict):
        return {str(k): _py(v) for k, v in value.items()}
    return str(value)


def _scalar(value: Any) -> Optional[float]:
    v = _py(value)
    if isinstance(v, (int, float)) and not isinstance(v, bool):
        return float(v)
    return None


def collect_diagnostics(model_obj: Any) -> Tuple[List[Dict[str, Any]], List[str]]:
    """Probe the estimator for the diagnostics it produced."""
    out: List[Dict[str, Any]] = []
    warnings: List[str] = []
    for attr, label, group, kind in DIAGNOSTICS:
        if not hasattr(model_obj, attr):
            continue
        value = getattr(model_obj, attr, None)
        if value is None:
            continue
        try:
            if kind == "stat_p":
                stat, p = _scalar(value[0]), _scalar(value[1])
                out.append({"group": group, "name": attr, "label": label,
                            "stat": stat, "df": 1, "p": p})
            elif kind == "moran":
                out.append({"group": group, "name": attr, "label": label,
                            "stat": _scalar(value[0]), "df": 1, "p": _scalar(value[-1])})
            elif kind == "df_stat_p":
                if not isinstance(value, dict):
                    continue
                stat = None
                for key, val in value.items():
                    if key not in ("df", "pvalue", "p", "prob"):
                        stat = _scalar(val)
                        if stat is not None:
                            break
                p = _scalar(value.get("pvalue", value.get("p", value.get("prob"))))
                out.append({"group": group, "name": attr, "label": label,
                            "stat": stat, "df": _scalar(value.get("df")), "p": p})
            elif kind == "chow":
                stat = None
                for key in ("chow", "wald", "value", "stat", "test"):
                    if hasattr(value, key):
                        stat = _scalar(getattr(value, key))
                        if stat is not None:
                            break
                p = _scalar(getattr(value, "pvalue", None)) if hasattr(value, "pvalue") else None
                df = _scalar(getattr(value, "df", None)) if hasattr(value, "df") else None
                if stat is not None:
                    out.append({"group": group, "name": attr, "label": label,
                                "stat": stat, "df": df, "p": p})
        except Exception as exc:                      # never let a probe break a run
            warnings.append("could not read diagnostic %s (%s)" % (attr, type(exc).__name__))
    return out, warnings


def extract_impacts(model_obj: Any) -> List[Dict[str, Any]]:
    """Direct / indirect / total multipliers, when the estimator reports them."""
    mult = getattr(model_obj, "sp_multipliers", None)
    if not isinstance(mult, dict):
        return []
    out = []
    for kind, values in mult.items():
        values = _py(values)
        if isinstance(values, list) and len(values) == 3:
            out.append({"kind": str(kind), "direct": values[0],
                        "indirect": values[1], "total": values[2]})
    return out


# --------------------------------------------------------------------------
# coefficients and fit statistics
# --------------------------------------------------------------------------
FIT_FIELDS = [
    ("logll", "logll"), ("aic", "aic"), ("schwarz", "schwarz"), ("sig2", "sigma2"),
    ("r2", "r2"), ("ar2", "adj_r2"), ("pr2", "pr2"), ("pr2_e", "pr2_e"),
    ("mean_y", "mean_y"), ("std_y", "std_y"), ("utu", "utu"), ("sig2n_k", "sigma2_ml"),
]


def _split_regime_prefix(raw: str) -> Tuple[str, str]:
    """Separate the regime a coefficient belongs to from the coefficient.

    ``0_INC`` -> ``("0_", "INC")``; ``_Global_W_HOVAL`` -> ``("_Global_",
    "W_HOVAL")``, spreg's mark for a coefficient shared by every regime.
    """
    if raw.startswith("_Global_"):
        return "_Global_", raw[len("_Global_"):]
    match = re.match(r"^(\d+_)(.+)$", raw)
    if match:
        return match.group(1), match.group(2)
    return "", raw


def _rewrite_name(raw: str, x_names: List[str], y_name: str,
                  yend_names: Optional[List[str]] = None) -> Tuple[str, str]:
    """Map spreg's names onto the user's, and say what each row is.

    Two sets of names come out of spreg depending on whether it was told what
    the variables are called: ``var_1`` / ``W_dep_var`` / ``endogenous_1`` when
    it was not, and the user's own names - ``INC``, ``W_HOVAL``, ``CRIME`` -
    when it was, which is what our solver asks for.  Both are recognised here,
    and the regime prefix is carried through untouched.
    """
    prefix, core = _split_regime_prefix(raw)
    name = core
    role = "exog"

    lag_names = {"W_dep_var", "W_" + y_name} if y_name else {"W_dep_var"}
    if core in lag_names:
        name = "W_" + y_name if y_name else core
        role = "lag"
    elif core == "lambda":
        role = "error"
    elif yend_names and core in yend_names:
        role = "endog"                       # an instrumented regressor
    elif core.startswith("W_var_"):
        role = "slx"
        idx = _index(core[len("W_var_"):])
        if idx and 1 <= idx <= len(x_names):
            name = "W_" + x_names[idx - 1]
    elif core.startswith("W_") and core[len("W_"):] in x_names:
        role = "slx"                         # the spatial lag of a covariate
    elif core.startswith("var_"):
        idx = _index(core[len("var_"):])
        if idx and 1 <= idx <= len(x_names):
            name = x_names[idx - 1]
    return prefix + name, role


def _index(text: str) -> Optional[int]:
    try:
        return int(text)
    except ValueError:
        return None


def extract_coefficients(model_obj: Any, x_names: List[str], y_name: str,
                         yend_names: Optional[List[str]] = None
                         ) -> Tuple[Dict[str, Any], List[str]]:
    """The coefficient table, from spreg's uniform ``output`` DataFrame."""
    warnings: List[str] = []
    table = getattr(model_obj, "output", None)
    if table is not None and hasattr(table, "to_dict"):
        cols = {c: table[c].tolist() for c in table.columns}
        pairs = [_rewrite_name(str(v), x_names, y_name, yend_names)
                 for v in cols["var_names"]]
        return {
            "names": [p[0] for p in pairs],
            "roles": [p[1] for p in pairs],
            "estimate": [_scalar(v) for v in cols["coefficients"]],
            "std_err": [_scalar(v) for v in cols.get("std_err", [])],
            "z": [_scalar(v) for v in cols.get("zt_stat", [])],
            "p": [_scalar(v) for v in cols.get("prob", [])],
        }, warnings

    # fall back to the raw arrays if spreg ever stops exposing `output`
    betas = _py(getattr(model_obj, "betas", None))
    if not isinstance(betas, list):
        raise RuntimeError("the estimator returned no coefficient table")
    warnings.append("coefficient table rebuilt from `betas`: standard errors unavailable")
    raw_names = [str(n) for n in (getattr(model_obj, "name_x", []) or [])]
    pairs = [_rewrite_name(n, x_names, y_name, yend_names) for n in raw_names]
    names = [p[0] for p in pairs]
    roles = [p[1] for p in pairs]
    while len(names) < len(betas):
        names.append("var_%d" % (len(names) + 1))
        roles.append("exog")
    return {"names": names[:len(betas)], "roles": roles[:len(betas)],
            "estimate": betas, "std_err": [], "z": [], "p": []}, warnings


def extract_fit(model_obj: Any) -> Dict[str, Any]:
    fit: Dict[str, Any] = {}
    for attr, key in FIT_FIELDS:
        if hasattr(model_obj, attr):
            value = _scalar(getattr(model_obj, attr))
            if value is not None:
                fit[key] = value
    return fit


def extract_clusters(model_obj: Any, ctx: Dict[str, Any]) -> Tuple[Dict[str, Any], List[str]]:
    """Regionalization results (SKATER_reg) - region id per observation."""
    labels = getattr(model_obj, "current_labels_", None)
    if labels is None:
        raise RuntimeError("the estimator returned no region labels")
    labels = [int(v) for v in _py(labels)]
    sizes: Dict[int, int] = {}
    for value in labels:
        sizes[value] = sizes.get(value, 0) + 1
    return {
        "clusters": {
            "n": len(labels),
            "n_regions": len(sizes),
            "sizes": [sizes[k] for k in sorted(sizes)],
            "label_array": "region",
        }
    }, []


def extract_result(model_obj: Any, entry: Dict[str, Any], ctx: Dict[str, Any]) -> Dict[str, Any]:
    """Turn a spreg result into the protocol's result structure."""
    x_names: List[str] = ctx.get("x_names", [])
    y_name: str = ctx.get("y_name", "")

    if entry["family"] == "skater":                      # not a regression: regions
        body, warnings = extract_clusters(model_obj, ctx)
        body["model"] = {"requested": entry["id"], "class": entry["class"].replace(":", "."),
                         "family": entry["family"], "title": entry["label"],
                         "n": int(ctx.get("n", 0)), "k": 0}
        body["warnings"] = warnings
        return body

    coefficients, warnings = extract_coefficients(model_obj, x_names, y_name,
                                                  ctx.get("yend_names"))
    diagnostics, dwarn = collect_diagnostics(model_obj)
    warnings += dwarn

    # std_err / z / p may be shorter than the coefficient vector (GM_Combo reports
    # `lambda` without a standard error); pad so the arrays stay aligned.
    n_rows = len(coefficients["estimate"])
    for key in ("std_err", "z", "p"):
        values = coefficients[key]
        if values and len(values) < n_rows:
            warnings.append(
                "%s reports no %s for %d coefficient(s); see the report text"
                % (entry["id"], {"std_err": "standard error", "z": "z statistic",
                                 "p": "p value"}[key], n_rows - len(values)))
            coefficients[key] = values + [None] * (n_rows - len(values))

    spatial = []
    for name, role, est, se, z, p in zip(coefficients["names"], coefficients["roles"],
                                        coefficients["estimate"], coefficients["std_err"] or [None] * n_rows,
                                        coefficients["z"] or [None] * n_rows,
                                        coefficients["p"] or [None] * n_rows):
        if role in ("lag", "error"):
            spatial.append({"name": name, "role": role, "estimate": est,
                            "std_err": se, "z": z, "p": p})

    title = getattr(model_obj, "title", None) or entry["label"]
    result = {
        "model": {
            "requested": entry["id"],
            "class": entry["class"].replace(":", "."),
            "family": entry["family"],
            "title": str(title),
            "n": int(getattr(model_obj, "n", ctx.get("n", 0)) or 0),
            "k": int(getattr(model_obj, "k", 0) or 0),
        },
        "coefficients": coefficients,
        "spatial_parameters": spatial,
        "fit": extract_fit(model_obj),
        "diagnostics": diagnostics,
        "warnings": warnings,
    }
    impacts = extract_impacts(model_obj)
    if impacts:
        result["impacts"] = impacts
    return result
