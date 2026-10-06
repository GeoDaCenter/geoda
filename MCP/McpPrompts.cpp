/**
 * GeoDa TM, Copyright (C) 2011-2025 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 *
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MCP/McpPrompts.h"
#include "GdaJson.h"
#include "MCP/McpTools.h"  // McpError

namespace
{
    json_spirit::Pair P(const wxString& name, const json_spirit::Value& v)
    {
        return json_spirit::Pair(name.ToStdString(), v);
    }

    json_spirit::Value Obj(const std::vector<json_spirit::Pair>& pairs)
    {
        return json_spirit::Value(json_spirit::Object(pairs));
    }

    // Section keys accepted by the spatial-analysis-workbook prompt's
    // "section" argument, with a short label shown in prompts/list.
    const char* kSectionKeys[] = {
        "workflow", "overview", "maps", "weights", "global", "local",
        "rates", "multivariate", "clustering"
    };
    const char* kSectionLabels[] = {
        "Thorough univariate spatial analysis workflow",
        "Overview and data orientation",
        "Visualizing spatial data (choropleth maps and plots)",
        "Spatial weights",
        "Global spatial autocorrelation",
        "Local spatial autocorrelation (LISA)",
        "Rates and rate smoothing",
        "Multivariate exploration and dimension reduction",
        "Clustering and regionalization"
    };
    const int kNumSections =
        sizeof(kSectionKeys) / sizeof(kSectionKeys[0]);

    wxString JoinSectionKeys()
    {
        wxString s;
        for (int i = 0; i < kNumSections; ++i) {
            if (i > 0) s += ", ";
            s += kSectionKeys[i];
        }
        return s;
    }
}

McpPrompts::McpPrompts()
{
}

McpPrompts::~McpPrompts()
{
}

json_spirit::Value McpPrompts::GetPromptsList() const
{
    std::vector<json_spirit::Pair> arg;
    arg.push_back(P("name",
        json_spirit::Value("section")));
    arg.push_back(P("description",
        json_spirit::Value(
            "Section of the workbook guide to return. Omit for the full "
            "guide. One of: " + JoinSectionKeys().ToStdString())));
    arg.push_back(P("required", json_spirit::Value(false)));

    json_spirit::Array args;
    args.push_back(Obj(arg));

    std::vector<json_spirit::Pair> prompt;
    prompt.push_back(P("name", json_spirit::Value("spatial-analysis-workbook")));
    prompt.push_back(P("description",
        json_spirit::Value(
            "Spatial data analysis workflow following Luc Anselin's "
            "\"Exploring Spatial Data with GeoDa: A Workbook\" (Center for "
            "Spatial Data Science). Maps each workbook part to the GeoDa MCP "
            "tools: maps, weights, global autocorrelation, LISA, rates, and "
            "clustering. Pass 'section' to load one part, or omit it for the "
            "full guide.")));
    prompt.push_back(P("arguments", json_spirit::Value(args)));

    json_spirit::Array prompts;
    prompts.push_back(Obj(prompt));

    std::vector<json_spirit::Pair> result;
    result.push_back(P("prompts", json_spirit::Value(prompts)));
    return Obj(result);
}

json_spirit::Value McpPrompts::GetPrompt(const json_spirit::Object& params) const
{
    wxString name = GdaJson::getStrValFromObj(params, "name");
    if (name != "spatial-analysis-workbook") {
        throw McpError(-32602, "Unknown prompt: " + name.ToStdString());
    }

    wxString section;
    json_spirit::Value args_val;
    if (GdaJson::findValue(params, args_val, "arguments") &&
        args_val.type() == json_spirit::obj_type) {
        section = GdaJson::getStrValFromObj(args_val.get_obj(), "section");
    }

    std::string text;
    if (section.IsEmpty()) {
        text = GuideText();
    } else {
        text = SectionText(section);
        if (text.empty()) {
            throw McpError(-32602,
                "Unknown section: " + section.ToStdString() +
                " (expected one of: " + JoinSectionKeys().ToStdString() + ")");
        }
    }

    // MCP message content is an array of content blocks, matching how
    // tools/call results are wrapped.
    json_spirit::Pair content_type("type", json_spirit::Value("text"));
    json_spirit::Pair content_text("text",
        json_spirit::Value(wxString::FromUTF8(text.c_str()).ToStdString()));
    std::vector<json_spirit::Pair> content;
    content.push_back(content_type);
    content.push_back(content_text);
    json_spirit::Array content_blocks;
    content_blocks.push_back(Obj(content));

    json_spirit::Pair msg_role("role", json_spirit::Value("user"));
    json_spirit::Pair msg_content("content",
        json_spirit::Value(content_blocks));
    std::vector<json_spirit::Pair> msg;
    msg.push_back(msg_role);
    msg.push_back(msg_content);
    json_spirit::Array messages;
    messages.push_back(Obj(msg));

    std::vector<json_spirit::Pair> result;
    result.push_back(P("description",
        json_spirit::Value(
            "Workbook-guided spatial data analysis with the GeoDa MCP server")));
    result.push_back(P("messages", json_spirit::Value(messages)));
    return Obj(result);
}

std::string McpPrompts::SectionText(const wxString& section)
{
    if (section == "workflow") {
        return
"## Thorough univariate spatial analysis workflow\n"
"\n"
"For a request like \"run a spatial analysis on variable X\" (e.g. HR60), "
"run the workbook's full univariate pipeline below in order. Use the weights "
"id from step 4 in every later step, and interpret each result before moving "
"on:\n"
"\n"
"1. Examine the variable -- table/univariate_stats {column: HR60}. Report "
"count, missing, mean, median, std_dev, min, max, skewness, kurtosis, and "
"note the distribution shape (e.g. HR60 is right-skewed with a long tail).\n"
"2. Distribution plots -- explore/histogram {columns: [HR60]} and "
"explore/boxplot {columns: [HR60]}. Read them for skew and outliers "
"(whisker length, extreme values).\n"
"3. Map the variable -- window/create_map {column: HR60, theme: quantile} "
"(or map/quantile {column: HR60}). Describe where the high and low values "
"are located (e.g. HR60 is concentrated in the South).\n"
"4. Spatial weights -- weights/create {type: queen}. Use the returned "
"weights id in every step below.\n"
"5. Global Moran's I -- space/global_moran {column: HR60, weights, "
"permutations: 999}. Report moran_i, expected, p_value. I > E[I] with a "
"small p-value means significant positive spatial autocorrelation.\n"
"6. Local Moran's I -- space/lisa_univariate {column: HR60, weights, "
"permutations: 999, significance_cutoff: 0.05}. Report the HH/LL/LH/HL "
"cluster counts and how many observations are significant.\n"
"7. LISA Cluster Map -- window/create_lisa_map {column: HR60, weights, "
"map_type: cluster}. Live GeoDa map coloring HH (hot spots), LL (cold "
"spots), LH/HL (spatial outliers).\n"
"8. LISA Significance Map -- window/create_lisa_map {column: HR60, weights, "
"map_type: significance}. Live map coloring the significant locations at "
"the chosen cutoff.\n"
"\n"
"Conclusion: summarize the distribution shape, the spatial pattern from the "
"quantile map, the global Moran's I result, the dominant LISA cluster "
"types, and where the significant hot and cold spots are located.\n";
    }
    if (section == "overview") {
        return
"## 0. Overview and data orientation\n"
"\n"
"Before any analysis, confirm a dataset is open and understand its "
"variables. The workbook's running example uses the natregimes sample "
"dataset (3,085 U.S. counties; homicide rates and socioeconomic "
"covariates).\n"
"\n"
"- project/status   Confirm a dataset is open; rows, columns, and the list "
"of column names.\n"
"- table/list_columns  See the available variables and their types.\n"
"- table/get_column {column}  Inspect a variable's raw values.\n"
"\n"
"Workbook practice: load natregimes and list the columns. HR60, HC60 and "
"RD60 are used throughout the exercises.\n";
    }
    if (section == "maps") {
        return
"## 1. Visualizing spatial data\n"
"\n"
"The workbook's first part builds choropleth maps and exploratory plots "
"to uncover the spatial distribution of a variable. Windows created here "
"are live GeoDa windows on the desktop, linked to one another.\n"
"\n"
"- window/create_map {column, theme, num_categories}\n"
"    theme: quantile, natural_breaks, equal_intervals, unique_values,\n"
"           stddev, percentile, or no_theme (default quantile, 5 classes)\n"
"    Exercise: quantile map of HR60; then natural_breaks and stddev maps\n"
"    and compare the spatial patterns they reveal.\n"
"- window/create_plot {plot_type, columns}\n"
"    plot_type: histogram (1 column), boxplot (1 column), or\n"
"               scatter (2 columns)\n"
"    Exercise: histogram and box plot of HR60 (skewed? outliers?), then\n"
"    scatter of HR60 vs HC60.\n"
"\n"
"Interpretation: quantile maps give equal counts per class; natural breaks "
"minimize within-class variance; standard deviation maps show deviations "
"from the mean in units of the standard deviation.\n";
    }
    if (section == "weights") {
        return
"## 2. Spatial weights\n"
"\n"
"Every spatial statistic in the workbook needs a spatial weights matrix "
"defining the neighborhood structure. Create a weights matrix, then list "
"and describe it.\n"
"\n"
"- weights/create {type, ...}\n"
"    type = queen | rook   first-order contiguity (order = N for higher\n"
"                          order neighbors)\n"
"    type = knn            k nearest neighbors (k, default 6)\n"
"    type = distance       fixed-distance band (distance_threshold;\n"
"                          is_arc/is_mile for great-circle distance)\n"
"    type = kernel         distance-based kernel weights (kernel:\n"
"                          triangular, uniform, epanechnikov, quartic,\n"
"                          gaussian)\n"
"    Returns a weights id (uuid) used by all other tools.\n"
"- weights/list   List the registered weights matrices.\n"
"- weights/describe {weights}  Neighbor count distribution, density, and\n"
"    connectivity. Use this to check for islands (neighborless units) and\n"
"    to compare specifications.\n"
"\n"
"Workbook practice: create first-order queen contiguity weights and "
"k-nearest-neighbor weights (k=6) for natregimes; describe both and "
"compare their neighbor-count distributions.\n";
    }
    if (section == "global") {
        return
"## 3. Global spatial autocorrelation\n"
"\n"
"Global statistics summarize whether the variable is spatially "
"autocorrelated across the whole study area, using the weights matrix.\n"
"\n"
"- global/moran {column, weights, permutations}\n"
"    Global Moran's I plus the pseudo p-value and the mean/std of the\n"
"    permutation distribution. E[I] = -1/(n-1); I > E[I] indicates\n"
"    positive spatial autocorrelation.\n"
"- global/geary {column, weights, permutations}\n"
"    Global Geary's C. C in (0,2); C < 1 indicates positive\n"
"    autocorrelation, C > 1 negative.\n"
"- global/general_g {column, weights, permutations}\n"
"    Global Getis-Ord General G, a high/low concentration statistic.\n"
"\n"
"All three take permutations (default 999) for the pseudo p-value.\n"
"\n"
"Workbook practice: Global Moran's I for HR60 with queen weights; report "
"the statistic and its pseudo p-value and state whether the homicide "
"rate is spatially autocorrelated.\n";
    }
    if (section == "local") {
        return
"## 4. Local spatial autocorrelation (LISA)\n"
"\n"
"Local statistics assess where clusters form: they give a statistic and "
"pseudo p-value per observation, plus a cluster category. Run the full "
"LISA workflow in order:\n"
"\n"
"1. table/univariate_stats {column: HR60}\n"
"    Confirm the distribution (mean, median, skewness) before testing for\n"
"    spatial autocorrelation.\n"
"2. weights/create {type: queen}\n"
"    Use the returned weights id in every step below.\n"
"3. space/global_moran {column: HR60, weights}\n"
"    Establish whether the variable is globally autocorrelated.\n"
"4. space/lisa_univariate {column: HR60, weights, permutations: 999,\n"
"                          significance_cutoff: 0.05}\n"
"    Cluster codes: 1=HH, 2=LL, 3=LH, 4=HL, 5=neighborless, 6=undefined.\n"
"5. window/create_lisa_map {column: HR60, weights, map_type: cluster}\n"
"    A live GeoDa map window coloring HH/LL/LH/HL.\n"
"6. window/create_lisa_map {column: HR60, weights,\n"
"                           map_type: significance}\n"
"    A live GeoDa map window coloring significant locations.\n"
"\n"
"Workbook practice: run steps 1-6 on HR60 with queen weights; count the "
"HH/LL/LH/HL categories from step 4 and identify significant locations at "
"p < 0.05 from the significance map.\n"
"\n"
"Interpretation: HH are high-value clusters (hot spots), LL are low-value "
"clusters (cold spots), LH/HL are spatial outliers (a low value surrounded "
"by high values, and vice versa).\n"
"\n"
"Variants: lisa/local_geary and lisa/local_g (or space/local_g_star for G*) "
"provide alternative local statistics with the same cluster coding. "
"lisa_type: bivariate (with second_column) tests the local association "
"between two variables.\n";
    }
    if (section == "rates") {
        return
"## 5. Rates and rate smoothing\n"
"\n"
"The workbook smooths crude rates (e.g., homicide rates) to stabilize "
"them, especially for small populations. Rate maps are produced through "
"the map window tool, which carries the smoothing options.\n"
"\n"
"- window/create_map {column, weights, smoothing}\n"
"    smoothing: raw_rate, excess_risk, empirical_bayes, spatial_rate, or\n"
"    spatial_empirical_bayes (default no_smoothing). weights is required\n"
"    for the spatial methods.\n"
"\n"
"Interpretation: empirical_bayes shrinks crude rates toward the global "
"mean in proportion to their variance; spatial_rate smooths with a "
"spatial moving average; spatial_empirical_bayes combines both. Compare "
"the raw-rate map with an EB-smoothed map of HR60 to see how extreme "
"rates in small-population counties are moderated.\n";
    }
    if (section == "multivariate") {
        return
"## 6. Multivariate exploration and dimension reduction\n"
"\n"
"The workbook pairs maps with multivariate plots, and uses dimension "
"reduction to summarize many variables.\n"
"\n"
"- window/create_plot {plot_type: scatter, columns: [x, y]}\n"
"    Bivariate scatter plot (e.g., HR60 vs HC60).\n"
"- window/create_plot {plot_type: boxplot, columns: [v]}\n"
"    Box plot for outlier detection.\n"
"- cluster/pca {columns}\n"
"    Principal Component Analysis: loadings, explained variance per\n"
"    component, and component scores for every observation. Use the first\n"
"    two components as a low-dimensional summary before clustering.\n"
"\n"
"Workbook practice: PCA on the natregimes socioeconomic variables; report "
"the share of variance explained by the first two components.\n";
    }
    if (section == "clustering") {
        return
"## 7. Clustering and regionalization\n"
"\n"
"The workbook closes with clustering: standard attribute clustering, and "
"spatially constrained clustering (regionalization) that forces clusters "
"to be spatially contiguous via the weights matrix.\n"
"\n"
"Standard (attribute) clustering:\n"
"- cluster/pam {columns, k}             Partitioning Around Medoids\n"
"- cluster/dbscan {columns, minpts, eps}\n"
"    Density-based; noise observations are labeled 0 and clusters are 1+.\n"
"    If eps is omitted it is estimated from the data (max 1-nearest-neighbor\n"
"    distance).\n"
"- cluster/hdbscan {columns, minpts}    Hierarchical density-based\n"
"- cluster/spectral {columns, weights, k}\n"
"    Spectral clustering on the graph defined by the weights.\n"
"- cluster/spatial_kmeans {columns, weights, k, init}\n"
"    K-means with a spatial penalty (init: kmeans++ or random).\n"
"\n"
"Spatially constrained (regionalization, all require weights):\n"
"- cluster/skater {columns, weights, k, boundary}\n"
"    Minimum-spanning-tree regionalization (optional boundary variable\n"
"    for a minimum-bound constraint).\n"
"- cluster/redcap {columns, weights, k, method}\n"
"    method: firstorder, fullorder, singlelink, avglink, completelink.\n"
"- cluster/schc {columns, weights, k, method}\n"
"    method: singlelink, completelink, avglink.\n"
"- cluster/maxp {columns, weights, bound_variable, min_bound}\n"
"    Max-p regionalization with a bound constraint.\n"
"- cluster/azp {columns, weights, k, method}\n"
"    method: greedy, tabu, or sa.\n"
"\n"
"Dimension reduction for visualization:\n"
"- cluster/mds {columns}   Multidimensional scaling coordinates\n"
"\n"
"Workbook practice: run SKATER (k=4) on HR60/HC60 with queen weights and "
"compare the regions with the LISA clusters; then run K-means/PAM on the "
"same variables and contrast the spatially unconstrained solution.\n";
    }
    return "";
}

std::string McpPrompts::GuideText()
{
    std::string s;
    s +=
"# Spatial Data Analysis with the GeoDa MCP Server\n"
"\n"
"This guide follows Luc Anselin, \"Exploring Spatial Data with GeoDa: A "
"Workbook\" (Center for Spatial Data Science, University of Chicago). It "
"maps each part of the workbook onto the MCP tools of this server so the "
"exercises can be reproduced against the open project. The workbook's "
"running example is the natregimes sample dataset (3,085 U.S. counties).\n"
"\n"
"## FIRST: Always load the skill content\n"
"\n"
"Loading the relevant skill content is the first step of every task, before "
"any tool call and before any other action:\n"
"- Over MCP: list skills with `resources/list`, then read this workbook "
"skill with `resources/read {uri: \"skill://spatial-analysis-workbook\"}` and "
"follow it exactly.\n"
"- From this prompt: the same content is served by `prompts/get`; read the "
"full guide before using any tool.\n"
"Do NOT call `table/*`, `weights/*`, `global/*`, `space/*`, `window/*`, or "
"any other tool before you have read the skill. The workbook skill is the "
"authoritative step-by-step guide for every exercise.\n"
"\n"
"Some tools ask the user directly. If a required numeric variable is missing "
"from a `lisa/*` or `global/*` call, the server raises a question card (MCP "
"elicitation) listing the project's numeric columns and continues with "
"whatever the user picks -- so you do not have to ask first. If you already "
"asked and were told, pass the column and no card appears.\n"
"\n"
"You are acting as a spatial analyst. Work through the parts in order, "
"create the weights matrix early (every spatial statistic needs it), and "
"report statistics with the interpretation guidance in each section.\n"
"\n";
    s += SectionText("workflow");
    s += "\n";
    s += SectionText("overview");
    s += "\n";
    s += SectionText("maps");
    s += "\n";
    s += SectionText("weights");
    s += "\n";
    s += SectionText("global");
    s += "\n";
    s += SectionText("local");
    s += "\n";
    s += SectionText("rates");
    s += "\n";
    s += SectionText("multivariate");
    s += "\n";
    s += SectionText("clustering");
    return s;
}
