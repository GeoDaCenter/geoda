# Spatial Data Analysis with the GeoDa MCP Server

A workflow guide following **Luc Anselin, "Exploring Spatial Data with GeoDa:
A Workbook"** (Center for Spatial Data Science, University of Chicago). It
maps each part of the workbook onto the MCP tools of the GeoDa MCP server so
the exercises can be reproduced against the open project.

The workbook's running example is the **natregimes** sample dataset (3,085
U.S. counties; homicide rates and socioeconomic covariates).

> This document is also served by the MCP server as the
> `spatial-analysis-workbook` prompt (`prompts/get`). An MCP client can load
> the full guide, or pass `section` to load one part:
> `overview, maps, weights, global, local, rates, multivariate, clustering`.

## FIRST: Always load this skill

Loading this skill's content is the **first step of every task** — before any
tool call and before any other action. Read it with
`skill/get {uri: "skill://spatial-analysis-workbook"}` (what `initialize` points
you at), as the resource `resources/read {uri:
"skill://spatial-analysis-workbook"}`, or as the prompt
`prompts/get {name: "spatial-analysis-workbook"}` — all three serve this text.
Then follow it exactly: do NOT call `table/*`, `weights/*`, `global/*`,
`space/*`, `window/*`, or any other tool before you have read it. Every
workbook exercise maps to a numbered step in the sections below.

If `project/status` reports that nothing is open, open the data set yourself
with `file/open {path}` — an absolute path, `~` and `${VAR}` expanded, e.g.
`~/Downloads/natregimes/natregimes.shp`. A project already open is left alone:
call `file/close` first to switch data sets.

Some tools ask the user directly. If a required numeric variable is missing from
a `lisa/*` or `global/*` call, the server raises a question card (MCP
elicitation) listing the project's numeric columns and continues with whatever
the user picks — so you do not have to ask first. If you already asked and were
told, pass the column and no card appears.

---

## Thorough univariate spatial analysis workflow

For a request like "run a spatial analysis on variable X" (e.g. `HR60`), run
the workbook's full univariate pipeline below in order. Use the weights id
from step 4 in every later step, and interpret each result before moving on:

1. **Examine the variable** — `table/univariate_stats {column: HR60}`. Report
   count, missing, mean, median, std_dev, min, max, skewness, kurtosis, and
   note the distribution shape (e.g. `HR60` is right-skewed with a long tail).
2. **Distribution plots** — `explore/histogram {columns: [HR60]}` and
   `explore/boxplot {columns: [HR60]}`. Read them for skew and outliers
   (whisker length, extreme values).
3. **Map the variable** — `window/create_map {column: HR60, theme: quantile}`
   (or `map/quantile {column: HR60}`). Describe where the high and low values
   are located (e.g. `HR60` is concentrated in the South).
4. **Spatial weights** — `weights/create {type: queen}`. Use the returned
   weights id in every step below.
5. **Global Moran's I** — `space/global_moran {column: HR60, weights,
   permutations: 999}`. Report `moran_i`, `expected`, `p_value`. `I > E[I]`
   with a small p-value means significant positive spatial autocorrelation.
6. **Local Moran's I** — `space/lisa_univariate {column: HR60, weights,
   permutations: 999, significance_cutoff: 0.05}`. Report the HH/LL/LH/HL
   cluster counts and how many observations are significant.
7. **LISA Cluster Map** — `window/create_lisa_map {column: HR60, weights,
   map_type: cluster}`. Live GeoDa map coloring HH (hot spots), LL (cold
   spots), LH/HL (spatial outliers).
8. **LISA Significance Map** — `window/create_lisa_map {column: HR60,
   weights, map_type: significance}`. Live map coloring the significant
   locations at the chosen cutoff.

**Conclusion:** summarize the distribution shape, the spatial pattern from
the quantile map, the global Moran's I result, the dominant LISA cluster
types, and where the significant hot and cold spots are located.

---

## 0. Overview and data orientation

Before any analysis, confirm a dataset is open and understand its variables.

- `file/open {path}` — open a local data set (shapefile, GeoJSON, GeoPackage,
  or a `.gda` project) in the running app; `file/close` closes it again. A
  format without geometry opens as a table-only project.
- `project/status` — confirm a dataset is open; rows, columns, and the list
  of column names.
- `table/list_columns` — see the available variables and their types.
- `table/get_column {column}` — inspect a variable's raw values.

**Workbook practice:** load natregimes and list the columns. `HR60`, `HC60`
and `RD60` are used throughout the exercises.

## 1. Visualizing spatial data

The workbook's first part builds choropleth maps and exploratory plots to
uncover the spatial distribution of a variable. Windows created here are live
GeoDa windows on the desktop, linked to one another.

- `window/create_map {column, theme, num_categories}`
  - `theme`: `quantile`, `natural_breaks`, `equal_intervals`, `unique_values`,
    `stddev`, `percentile`, or `no_theme` (default `quantile`, 5 classes)
  - **Exercise:** quantile map of `HR60`; then `natural_breaks` and `stddev`
    maps and compare the spatial patterns they reveal.
- `window/create_plot {plot_type, columns}`
  - `plot_type`: `histogram` (1 column), `boxplot` (1 column), or `scatter`
    (2 columns)
  - **Exercise:** histogram and box plot of `HR60` (skewed? outliers?), then
    scatter of `HR60` vs `HC60`.

**Interpretation:** quantile maps give equal counts per class; natural breaks
minimize within-class variance; standard deviation maps show deviations from
the mean in units of the standard deviation.

## 2. Spatial weights

Every spatial statistic in the workbook needs a spatial weights matrix
defining the neighborhood structure. Create a weights matrix, then list and
describe it.

- `weights/create {type, ...}`
  - `type = queen | rook` — first-order contiguity (`order = N` for higher
    order neighbors)
  - `type = knn` — k nearest neighbors (`k`, default 6)
  - `type = distance` — fixed-distance band (`distance_threshold`;
    `is_arc`/`is_mile` for great-circle distance)
  - `type = kernel` — distance-based kernel weights (`kernel`: `triangular`,
    `uniform`, `epanechnikov`, `quartic`, `gaussian`)
  - Returns a weights id (uuid) used by all other tools.
- `weights/list` — list the registered weights matrices.
- `weights/describe {weights}` — neighbor count distribution, density, and
  connectivity. Use this to check for islands (neighborless units) and to
  compare specifications.

**Workbook practice:** create first-order queen contiguity weights and
k-nearest-neighbor weights (k=6) for natregimes; describe both and compare
their neighbor-count distributions.

## 3. Global spatial autocorrelation

Global statistics summarize whether the variable is spatially autocorrelated
across the whole study area, using the weights matrix.

- `global/moran {column, weights, permutations}` — Global Moran's I plus the
  pseudo p-value and the mean/std of the permutation distribution.
  `E[I] = -1/(n-1)`; `I > E[I]` indicates positive spatial autocorrelation.
- `global/geary {column, weights, permutations}` — Global Geary's C.
  `C` in (0,2); `C < 1` indicates positive autocorrelation, `C > 1` negative.
- `global/general_g {column, weights, permutations}` — Global Getis-Ord
  General G, a high/low concentration statistic.

All three take `permutations` (default 999) for the pseudo p-value.

**Workbook practice:** Global Moran's I for `HR60` with queen weights; report
the statistic and its pseudo p-value and state whether the homicide rate is
spatially autocorrelated.

## 4. Local spatial autocorrelation (LISA)

Local statistics assess where clusters form: they give a statistic and pseudo
p-value per observation, plus a cluster category. Run the full LISA workflow
in order:

1. **Univariate statistics** — `table/univariate_stats {column: HR60}`.
   Confirm the distribution (mean, median, skewness) before testing for
   spatial autocorrelation.
2. **Spatial weights** — `weights/create {type: queen}`. Use the returned
   weights id in every step below.
3. **Global Moran's I** — `space/global_moran {column: HR60, weights}`.
   Establish whether the variable is globally autocorrelated.
4. **Local Moran's I** — `space/lisa_univariate {column: HR60, weights,
   permutations: 999, significance_cutoff: 0.05}`. Cluster codes: `1=HH`,
   `2=LL`, `3=LH`, `4=HL`, `5=neighborless`, `6=undefined`.
5. **LISA Cluster Map** — `window/create_lisa_map {column: HR60, weights,
   map_type: cluster}`. A live GeoDa map window coloring HH/LL/LH/HL.
6. **LISA Significance Map** — `window/create_lisa_map {column: HR60,
   weights, map_type: significance}`. A live GeoDa map window coloring
   significant locations at the chosen cutoff.

**Workbook practice:** run steps 1-6 on `HR60` with queen weights; count the
HH/LL/LH/HL categories from step 4 and identify significant locations at
p < 0.05 from the significance map.

**Interpretation:** HH are high-value clusters (hot spots), LL are low-value
clusters (cold spots), LH/HL are spatial outliers (a low value surrounded by
high values, and vice versa).

**Variants:** `lisa/local_geary` and `lisa/local_g` (or `space/local_g_star`
for G*) provide alternative local statistics with the same cluster coding.
`lisa_type: bivariate` (with `second_column`) tests the local association
between two variables.

## 5. Rates and rate smoothing

The workbook smooths crude rates (e.g., homicide rates) to stabilize them,
especially for small populations. Rate maps are produced through the map
window tool, which carries the smoothing options.

- `window/create_map {column, weights, smoothing}` — `smoothing`: `raw_rate`,
  `excess_risk`, `empirical_bayes`, `spatial_rate`, or
  `spatial_empirical_bayes` (default `no_smoothing`). `weights` is required
  for the spatial methods.

**Interpretation:** `empirical_bayes` shrinks crude rates toward the global
mean in proportion to their variance; `spatial_rate` smooths with a spatial
moving average; `spatial_empirical_bayes` combines both. Compare the
raw-rate map with an EB-smoothed map of `HR60` to see how extreme rates in
small-population counties are moderated.

## 6. Multivariate exploration and dimension reduction

The workbook pairs maps with multivariate plots, and uses dimension reduction
to summarize many variables.

- `window/create_plot {plot_type: scatter, columns: [x, y]}` — bivariate
  scatter plot (e.g., `HR60` vs `HC60`).
- `window/create_plot {plot_type: boxplot, columns: [v]}` — box plot for
  outlier detection.
- `cluster/pca {columns}` — Principal Component Analysis: loadings, explained
  variance per component, and component scores for every observation. Use
  the first two components as a low-dimensional summary before clustering.

**Workbook practice:** PCA on the natregimes socioeconomic variables; report
the share of variance explained by the first two components.

## 7. Clustering and regionalization

The workbook closes with clustering: standard attribute clustering, and
spatially constrained clustering (regionalization) that forces clusters to be
spatially contiguous via the weights matrix.

**Standard (attribute) clustering:**

- `cluster/pam {columns, k}` — Partitioning Around Medoids
- `cluster/dbscan {columns, minpts, eps}` — density-based; noise
  observations are labeled `0` and clusters are `1+`. If `eps` is omitted it
  is estimated from the data (max 1-nearest-neighbor distance).
- `cluster/hdbscan {columns, minpts}` — hierarchical density-based
- `cluster/spectral {columns, weights, k}` — spectral clustering on the
  graph defined by the weights
- `cluster/spatial_kmeans {columns, weights, k, init}` — k-means with a
  spatial penalty (`init`: `kmeans++` or `random`)

**Spatially constrained (regionalization, all require `weights`):**

- `cluster/skater {columns, weights, k, boundary}` — minimum-spanning-tree
  regionalization (optional `boundary` variable for a minimum-bound
  constraint)
- `cluster/redcap {columns, weights, k, method}` — `method`: `firstorder`,
  `fullorder`, `singlelink`, `avglink`, `completelink`
- `cluster/schc {columns, weights, k, method}` — `method`: `singlelink`,
  `completelink`, `avglink`
- `cluster/maxp {columns, weights, bound_variable, min_bound}` — Max-p
  regionalization with a bound constraint
- `cluster/azp {columns, weights, k, method}` — `method`: `greedy`, `tabu`,
  or `sa`

**Dimension reduction for visualization:**

- `cluster/mds {columns}` — multidimensional scaling coordinates

**Workbook practice:** run SKATER (k=4) on `HR60`/`HC60` with queen weights
and compare the regions with the LISA clusters; then run K-means/PAM on the
same variables and contrast the spatially unconstrained solution.
