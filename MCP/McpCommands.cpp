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

#include "MCP/McpCommands.h"
#include "MCP/McpTools.h"
#include "MCP/McpToolsSpatial.h"

namespace
{
    // ---------------------------------------------------------------------
    // JSON helpers
    // ---------------------------------------------------------------------
    json_spirit::Pair P(const wxString& name, const json_spirit::Value& v)
    {
        return json_spirit::Pair(name.ToStdString(), v);
    }

    json_spirit::Value Obj(const std::vector<json_spirit::Pair>& pairs)
    {
        return json_spirit::Value(json_spirit::Object(pairs));
    }

    // ---------------------------------------------------------------------
    // JSON Schema property helpers
    // ---------------------------------------------------------------------
    json_spirit::Value StrProp(const char* desc)
    {
        std::vector<json_spirit::Pair> p;
        p.push_back(P("type", json_spirit::Value("string")));
        p.push_back(P("description", json_spirit::Value(desc)));
        return Obj(p);
    }

    json_spirit::Value IntProp(const char* desc)
    {
        std::vector<json_spirit::Pair> p;
        p.push_back(P("type", json_spirit::Value("integer")));
        p.push_back(P("description", json_spirit::Value(desc)));
        return Obj(p);
    }

    json_spirit::Value NumProp(const char* desc)
    {
        std::vector<json_spirit::Pair> p;
        p.push_back(P("type", json_spirit::Value("number")));
        p.push_back(P("description", json_spirit::Value(desc)));
        return Obj(p);
    }

    json_spirit::Value BoolProp(const char* desc)
    {
        std::vector<json_spirit::Pair> p;
        p.push_back(P("type", json_spirit::Value("boolean")));
        p.push_back(P("description", json_spirit::Value(desc)));
        return Obj(p);
    }

    json_spirit::Value ArrProp(const char* desc, const char* item_type)
    {
        std::vector<json_spirit::Pair> items;
        items.push_back(P("type", json_spirit::Value(item_type)));
        std::vector<json_spirit::Pair> p;
        p.push_back(P("type", json_spirit::Value("array")));
        p.push_back(P("description", json_spirit::Value(desc)));
        p.push_back(P("items", Obj(items)));
        return Obj(p);
    }

    // As above, plus JSON Schema minItems/maxItems so a client can tell how
    // many elements the array takes (e.g. a scatter plot takes exactly two).
    // Pass 0 to leave a bound unconstrained.
    json_spirit::Value ArrProp(const char* desc, const char* item_type,
                               int min_items, int max_items)
    {
        json_spirit::Object o = ArrProp(desc, item_type).get_obj();
        if (min_items > 0) {
            o.push_back(P("minItems", json_spirit::Value(min_items)));
        }
        if (max_items > 0) {
            o.push_back(P("maxItems", json_spirit::Value(max_items)));
        }
        return json_spirit::Value(o);
    }

    // String property restricted to a fixed set of allowed values, emitted as
    // a JSON Schema "enum" so a client can enumerate the choices instead of
    // parsing the description.
    json_spirit::Value EnumProp(const char* desc,
                                const std::vector<const char*>& values)
    {
        std::vector<json_spirit::Pair> p;
        p.push_back(P("type", json_spirit::Value("string")));
        p.push_back(P("description", json_spirit::Value(desc)));
        std::vector<json_spirit::Value> vals;
        for (size_t i = 0; i < values.size(); ++i) {
            vals.push_back(json_spirit::Value(values[i]));
        }
        p.push_back(P("enum", json_spirit::Value(json_spirit::Array(vals))));
        return Obj(p);
    }

    // Add a JSON Schema "default" to a property. Use the same value the tool
    // handler applies when the argument is omitted, so a client can pre-select
    // it. The overloads mirror the *Prop helpers above.
    json_spirit::Value Def(const json_spirit::Value& prop,
                           const json_spirit::Value& def)
    {
        json_spirit::Object o = prop.get_obj();
        o.push_back(P("default", def));
        return json_spirit::Value(o);
    }

    json_spirit::Value Def(const json_spirit::Value& prop, const char* def)
    {
        return Def(prop, json_spirit::Value(def));
    }

    json_spirit::Value Def(const json_spirit::Value& prop, int def)
    {
        return Def(prop, json_spirit::Value(def));
    }

    json_spirit::Value Def(const json_spirit::Value& prop, double def)
    {
        return Def(prop, json_spirit::Value(def));
    }

    json_spirit::Value Def(const json_spirit::Value& prop, bool def)
    {
        return Def(prop, json_spirit::Value(def));
    }

    json_spirit::Value Schema(const std::vector<json_spirit::Pair>& props,
                              const std::vector<const char*>& required)
    {
        std::vector<json_spirit::Pair> s;
        s.push_back(P("type", json_spirit::Value("object")));
        s.push_back(P("properties", Obj(props)));
        if (!required.empty()) {
            std::vector<json_spirit::Value> req;
            for (size_t i = 0; i < required.size(); ++i) {
                req.push_back(json_spirit::Value(required[i]));
            }
            s.push_back(P("required",
                          json_spirit::Value(json_spirit::Array(req))));
        }
        return Obj(s);
    }

    json_spirit::Value NoParams()
    {
        return Schema(std::vector<json_spirit::Pair>(),
                      std::vector<const char*>());
    }

    // ---------------------------------------------------------------------
    // Fixed-parameter injection
    // ---------------------------------------------------------------------
    json_spirit::Pair Fixed(const char* key, const char* value)
    {
        return json_spirit::Pair(key, json_spirit::Value(value));
    }

    json_spirit::Pair Fixed(const char* key, int value)
    {
        return json_spirit::Pair(key, json_spirit::Value(value));
    }

    json_spirit::Pair Fixed(const char* key, bool value)
    {
        return json_spirit::Pair(key, json_spirit::Value(value));
    }

    // Call a tool handler with a fixed parameter merged into the request
    // arguments. Used by menu-action wrappers that pin a parameter (e.g.
    // map/quantile pins theme=quantile) while passing the rest through.
    json_spirit::Value CallWithFixed(const McpToolContext& ctx,
                                     const json_spirit::Object& params,
                                     McpToolHandler handler,
                                     const json_spirit::Pair& fixed)
    {
        json_spirit::Object merged = params;
        bool found = false;
        for (size_t j = 0; j < merged.size(); ++j) {
            if (merged[j].name_ == fixed.name_) {
                merged[j].value_ = fixed.value_;
                found = true;
                break;
            }
        }
        if (!found) merged.push_back(fixed);
        return handler(ctx, merged);
    }

    // ---------------------------------------------------------------------
    // Menu-action wrapper handlers
    // ---------------------------------------------------------------------
    // Space menu
    json_spirit::Value McpSpaceLisaUnivariate(const McpToolContext& ctx,
                                              const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpLisaLocalMoran,
                             Fixed("lisa_type", "univariate"));
    }

    json_spirit::Value McpSpaceLisaBivariate(const McpToolContext& ctx,
                                             const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpLisaLocalMoran,
                             Fixed("lisa_type", "bivariate"));
    }

    json_spirit::Value McpSpaceLisaDifferential(const McpToolContext& ctx,
                                                const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpLisaLocalMoran,
                             Fixed("lisa_type", "differential"));
    }

    json_spirit::Value McpSpaceLisaEb(const McpToolContext& ctx,
                                      const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpLisaLocalMoran,
                             Fixed("lisa_type", "eb_rate_standardized"));
    }

    json_spirit::Value McpSpaceLocalGStar(const McpToolContext& ctx,
                                          const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpLisaLocalG, Fixed("gstar", true));
    }

    json_spirit::Value McpSpaceLocalGearyMultivariate(
        const McpToolContext& ctx, const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpLisaLocalGeary,
                             Fixed("lisa_type", "multivariate"));
    }

    // Map menu
    json_spirit::Value McpMapQuantile(const McpToolContext& ctx,
                                      const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpWindowCreateMap,
                             Fixed("theme", "quantile"));
    }

    json_spirit::Value McpMapNaturalBreaks(const McpToolContext& ctx,
                                           const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpWindowCreateMap,
                             Fixed("theme", "natural_breaks"));
    }

    json_spirit::Value McpMapEqualIntervals(const McpToolContext& ctx,
                                            const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpWindowCreateMap,
                             Fixed("theme", "equal_intervals"));
    }

    json_spirit::Value McpMapPercentile(const McpToolContext& ctx,
                                        const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpWindowCreateMap,
                             Fixed("theme", "percentile"));
    }

    json_spirit::Value McpMapStddev(const McpToolContext& ctx,
                                    const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpWindowCreateMap,
                             Fixed("theme", "stddev"));
    }

    json_spirit::Value McpMapUniqueValues(const McpToolContext& ctx,
                                          const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpWindowCreateMap,
                             Fixed("theme", "unique_values"));
    }

    json_spirit::Value McpMapRatesRaw(const McpToolContext& ctx,
                                      const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpWindowCreateMap,
                             Fixed("smoothing", "raw_rate"));
    }

    json_spirit::Value McpMapRatesExcessRisk(const McpToolContext& ctx,
                                             const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpWindowCreateMap,
                             Fixed("smoothing", "excess_risk"));
    }

    json_spirit::Value McpMapRatesEb(const McpToolContext& ctx,
                                     const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpWindowCreateMap,
                             Fixed("smoothing", "empirical_bayes"));
    }

    json_spirit::Value McpMapRatesSpatial(const McpToolContext& ctx,
                                          const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpWindowCreateMap,
                             Fixed("smoothing", "spatial_rate"));
    }

    json_spirit::Value McpMapRatesSpatialEb(const McpToolContext& ctx,
                                            const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpWindowCreateMap,
                             Fixed("smoothing", "spatial_empirical_bayes"));
    }

    // Explore menu
    json_spirit::Value McpExploreHistogram(const McpToolContext& ctx,
                                           const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpWindowCreatePlot,
                             Fixed("plot_type", "histogram"));
    }

    json_spirit::Value McpExploreBoxplot(const McpToolContext& ctx,
                                         const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpWindowCreatePlot,
                             Fixed("plot_type", "boxplot"));
    }

    json_spirit::Value McpExploreScatterplot(const McpToolContext& ctx,
                                            const json_spirit::Object& params)
    {
        return CallWithFixed(ctx, params, McpWindowCreatePlot,
                             Fixed("plot_type", "scatter"));
    }

    // ---------------------------------------------------------------------
    // Requires-GUI handler for menu actions that cannot be parameterized
    // ---------------------------------------------------------------------
    json_spirit::Value McpRequiresGui(const McpToolContext& ctx,
                                      const json_spirit::Object& params)
    {
        throw McpError(-32602,
            "This action requires the GeoDa GUI; use the menu instead.");
    }

    // ---------------------------------------------------------------------
    // Registration helper
    // ---------------------------------------------------------------------
    // `elicit` marks tools that may ask the user for a parameter the request
    // left out; it defaults to false so only the tools that do opt in.
    void Add(McpTools& tools, const char* id, const char* label,
             const char* menu, const char* desc,
             const json_spirit::Value& schema, bool worker,
             McpToolHandler h, bool elicit = false)
    {
        tools.AddTool(id, label, menu, desc, schema, worker, h, elicit);
    }
}

// ---------------------------------------------------------------------------
// RegisterCommands
// ---------------------------------------------------------------------------
void RegisterCommands(McpTools& tools)
{
    // =====================================================================
    // skill (first: the client is told to read this before anything else)
    // =====================================================================
    Add(tools, "skill/list", "List Skills", "Skill",
        "List the skills this server ships, as skill:// URIs. Currently the "
        "spatial analysis workbook; read it with skill/get.",
        NoParams(), false, McpSkillList);

    Add(tools, "skill/get", "Read Skill", "Skill",
        "Return the text of a skill from skill/list -- the workbook that maps "
        "spatial analysis tasks onto these tools. Read it before any analysis; "
        "resources/read serves the same text to clients that read resources.",
        Schema(std::vector<json_spirit::Pair>{
                   P("uri", StrProp("Skill URI, e.g. "
                                    "skill://spatial-analysis-workbook"))},
               {"uri"}),
        false, McpSkillGet);

    // =====================================================================
    // file
    // =====================================================================
    // Opening creates the table and map frames, so this is a main-thread tool
    // (run_on_worker == false) like the window tools.
    Add(tools, "file/open", "Open Data", "File",
        "Open a data set or a .gda project file from the local disk in the "
        "running GeoDa, replacing nothing: any project already open must be "
        "closed with file/close first. Reads the formats GeoDa reads "
        "(shapefile, GeoJSON, GeoPackage, KML, CSV, ...); a format without "
        "geometry opens as a table-only project.",
        Schema(std::vector<json_spirit::Pair>{
                   P("path", StrProp("Absolute path to the data set or .gda "
                                     "project file"))},
               {"path"}),
        false, McpFileOpen);

    Add(tools, "file/close", "Close Project", "File",
        "Close the open project and its windows. Refuses when the project has "
        "unsaved changes unless force is true, since nothing here can save "
        "them (file/save is a GUI action).",
        Schema(std::vector<json_spirit::Pair>{
                   P("force", Def(BoolProp("Discard unsaved changes"), false))},
               {}),
        false, McpFileClose);

    // =====================================================================
    // project
    // =====================================================================
    Add(tools, "project/status", "Project Status", "Project",
        "Report whether a data set is open, its title, path, and dimensions.",
        NoParams(), false, McpProjectStatus);

    // =====================================================================
    // table
    // =====================================================================
    Add(tools, "table/list_columns", "List Columns", "Table",
        "List the columns of the open table with their types.",
        NoParams(), false, McpTableListColumns);

    Add(tools, "table/get_column", "Get Column", "Table",
        "Return the raw values of a column.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Column name"))},
               {"column"}),
        false, McpTableGetColumn);

    Add(tools, "table/univariate_stats", "Univariate Statistics", "Table",
        "Descriptive statistics for a numeric column: count, mean, median, "
        "std dev, min, max, quartiles, skewness, kurtosis.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name"))},
               {"column"}),
        false, McpTableUnivariateStats);

    // =====================================================================
    // weights
    // =====================================================================
    Add(tools, "weights/create", "Create Weights", "Weights",
        "Create a spatial weights matrix and return its id (uuid). Types: "
        "queen, rook, knn, distance, kernel.",
        Schema(std::vector<json_spirit::Pair>{
                   P("type",
                     Def(EnumProp("Spatial weights type",
                                  {"queen", "rook", "knn", "distance",
                                   "kernel"}), "queen")),
                   P("k", Def(IntProp("Number of nearest neighbors (knn)"), 6)),
                   P("distance_threshold",
                     NumProp("Distance threshold (distance)")),
                   P("kernel",
                     Def(EnumProp("Kernel function",
                                  {"triangular", "uniform", "epanechnikov",
                                   "quartic", "gaussian"}), "triangular")),
                   P("order",
                     Def(IntProp("Contiguity order (queen/rook)"), 1)),
                   P("is_arc",
                     Def(BoolProp("Use great-circle distance"), false)),
                   P("is_mile", Def(BoolProp("Distance in miles"), false))},
               {}),
        true, McpWeightsCreate);

    Add(tools, "weights/list", "List Weights", "Weights",
        "List the registered weights matrices with their ids and types.",
        NoParams(), false, McpWeightsList);

    Add(tools, "weights/describe", "Describe Weights", "Weights",
        "Neighbor count distribution, density, and connectivity of a "
        "weights matrix.",
        Schema(std::vector<json_spirit::Pair>{
                   P("weights", StrProp("Weights id (uuid)"))},
               {"weights"}),
        false, McpWeightsDescribe);

    // =====================================================================
    // global
    // =====================================================================
    Add(tools, "global/moran", "Global Moran's I", "Space",
        "Global Moran's I with pseudo p-value from a permutation test.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("permutations", Def(IntProp("Number of permutations"), 999))},
               {"column", "weights"}),
        true, McpGlobalMoran, true);

    Add(tools, "global/geary", "Global Geary's C", "Space",
        "Global Geary's C with pseudo p-value from a permutation test.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("permutations", Def(IntProp("Number of permutations"), 999))},
               {"column", "weights"}),
        true, McpGlobalGeary, true);

    Add(tools, "global/general_g", "Global Getis-Ord General G", "Space",
        "Global Getis-Ord General G, a high/low concentration statistic.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("permutations", Def(IntProp("Number of permutations"), 999))},
               {"column", "weights"}),
        true, McpGlobalGeneralG, true);

    // =====================================================================
    // lisa
    // =====================================================================
    Add(tools, "lisa/local_moran", "Local Moran's I", "Space",
        "Local Moran's I with pseudo p-values and cluster categories "
        "(1=HH, 2=LL, 3=LH, 4=HL, 5=neighborless, 6=undefined).",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("permutations", Def(IntProp("Number of permutations"), 999)),
                   P("significance_cutoff",
                     Def(NumProp("Significance level"), 0.05)),
                   P("row_standardize", Def(BoolProp("Row-standardize"), true)),
                   P("lisa_type",
                     Def(EnumProp("LISA type",
                                  {"univariate", "bivariate", "differential",
                                   "eb_rate_standardized"}), "univariate")),
                   P("second_column",
                     StrProp("Second column for bivariate/differential"))},
               {"column", "weights"}),
        true, McpLisaLocalMoran, true);

    Add(tools, "lisa/local_geary", "Local Geary", "Space",
        "Local Geary statistic with pseudo p-values and cluster categories.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("permutations", Def(IntProp("Number of permutations"), 999)),
                   P("significance_cutoff",
                     Def(NumProp("Significance level"), 0.05)),
                   P("row_standardize", Def(BoolProp("Row-standardize"), true)),
                   P("lisa_type",
                     Def(EnumProp("LISA type",
                                  {"univariate", "bivariate", "differential",
                                   "eb_rate_standardized", "multivariate"}),
                         "univariate")),
                   P("second_column",
                     StrProp("Second column for bivariate/differential"))},
               {"column", "weights"}),
        true, McpLisaLocalGeary, true);

    Add(tools, "lisa/local_g", "Local Getis-Ord G", "Space",
        "Local Getis-Ord G (or G* with gstar=true) with pseudo p-values.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("permutations", Def(IntProp("Number of permutations"), 999)),
                   P("significance_cutoff",
                     Def(NumProp("Significance level"), 0.05)),
                   P("gstar", BoolProp("Use G* (include self in neighborhood)"))},
               {"column", "weights"}),
        true, McpLisaLocalG, true);

    // =====================================================================
    // cluster
    // =====================================================================
    Add(tools, "cluster/skater", "SKATER", "Cluster",
        "Minimum-spanning-tree regionalization (spatially constrained).",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Numeric column names", "string", 1, 0)),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("k", Def(IntProp("Number of regions"), 3)),
                   P("boundary", StrProp("Boundary variable (optional)"))},
               {"columns", "weights"}),
        true, McpClusterSkater);

    Add(tools, "cluster/redcap", "REDCAP", "Cluster",
        "Spatially constrained hierarchical clustering (regionalization).",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Numeric column names", "string", 1, 0)),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("k", Def(IntProp("Number of regions"), 3)),
                   P("method",
                     Def(EnumProp("REDCAP method",
                                  {"firstorder", "fullorder_ward",
                                   "fullorder_alk", "fullorder_clk",
                                   "singlelink", "avglink", "completelink"}),
                         "firstorder"))},
               {"columns", "weights"}),
        true, McpClusterRedcap);

    Add(tools, "cluster/schc", "SCHC", "Cluster",
        "Spatially constrained hierarchical clustering.",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Numeric column names", "string", 1, 0)),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("k", Def(IntProp("Number of regions"), 3)),
                   P("method",
                     Def(EnumProp("SCHC method",
                                  {"singlelink", "avglink", "completelink"}),
                         "singlelink"))},
               {"columns", "weights"}),
        true, McpClusterSchc);

    Add(tools, "cluster/maxp", "Max-p", "Cluster",
        "Max-p regionalization with a bound constraint.",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Numeric column names", "string", 1, 0)),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("bound_variable", StrProp("Bound variable")),
                   P("min_bound", NumProp("Minimum bound value"))},
               {"columns", "weights", "bound_variable", "min_bound"}),
        true, McpClusterMaxp);

    Add(tools, "cluster/azp", "AZP", "Cluster",
        "Automatic zoning procedure (regionalization).",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Numeric column names", "string", 1, 0)),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("k", Def(IntProp("Number of regions"), 3)),
                   P("method",
                     Def(EnumProp("AZP method", {"greedy", "tabu", "sa"}),
                         "greedy"))},
               {"columns", "weights"}),
        true, McpClusterAzp);

    Add(tools, "cluster/spatial_kmeans", "Spatial K-Means", "Cluster",
        "K-means with a spatial penalty via the weights matrix.",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Numeric column names", "string", 1, 0)),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("k", Def(IntProp("Number of clusters"), 3)),
                   P("init",
                     Def(EnumProp("Cluster initialization",
                                  {"kmeans++", "random"}), "random"))},
               {"columns", "weights"}),
        true, McpClusterSpatialKmeans);

    Add(tools, "cluster/dbscan", "DBSCAN", "Cluster",
        "Density-based clustering. Noise observations are labeled 0.",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Numeric column names", "string", 1, 0)),
                   P("minpts", Def(IntProp("Min points"), 4)),
                   P("eps", NumProp("Epsilon (estimated if omitted)"))},
               {"columns"}),
        true, McpClusterDbscan);

    Add(tools, "cluster/hdbscan", "HDBSCAN", "Cluster",
        "Hierarchical density-based clustering. Noise is labeled 0.",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Numeric column names", "string", 1, 0)),
                   P("minpts", Def(IntProp("Min points"), 4))},
               {"columns"}),
        true, McpClusterHdbscan);

    Add(tools, "cluster/spectral", "Spectral Clustering", "Cluster",
        "Spectral clustering on the graph defined by the weights.",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Numeric column names", "string", 1, 0)),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("k", Def(IntProp("Number of clusters"), 3))},
               {"columns", "weights"}),
        true, McpClusterSpectral);

    Add(tools, "cluster/pam", "PAM", "Cluster",
        "Partitioning Around Medoids.",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Numeric column names", "string", 1, 0)),
                   P("k", Def(IntProp("Number of clusters"), 3))},
               {"columns"}),
        true, McpClusterPam);

    Add(tools, "cluster/kmeans", "K-Means", "Cluster",
        "Standard k-means clustering (Euclidean distance).",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Numeric column names", "string", 1, 0)),
                   P("k", Def(IntProp("Number of clusters"), 3))},
               {"columns"}),
        true, McpClusterKmeans);

    Add(tools, "cluster/kmedians", "K-Medians", "Cluster",
        "K-medians clustering (Manhattan distance, medoid-based).",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Numeric column names", "string", 1, 0)),
                   P("k", Def(IntProp("Number of clusters"), 3))},
               {"columns"}),
        true, McpClusterKmedians);

    Add(tools, "cluster/hierarchical", "Hierarchical Clustering", "Cluster",
        "Agglomerative hierarchical clustering (single, complete, average, "
        "or ward linkage).",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Numeric column names", "string", 1, 0)),
                   P("k", Def(IntProp("Number of clusters"), 3)),
                   P("method",
                     Def(EnumProp("Linkage method",
                                  {"single", "complete", "average", "ward"}),
                         "average"))},
               {"columns"}),
        true, McpClusterHierarchical);

    Add(tools, "cluster/mds", "Multidimensional Scaling", "Cluster",
        "MDS coordinates for the observations (2 dimensions).",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Numeric column names", "string", 1, 0))},
               {"columns"}),
        true, McpClusterMds);

    Add(tools, "cluster/pca", "Principal Component Analysis", "Cluster",
        "PCA loadings, explained variance, and component scores.",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Numeric column names", "string", 1, 0))},
               {"columns"}),
        true, McpClusterPca);

    Add(tools, "cluster/tsne", "t-SNE", "Cluster",
        "t-distributed stochastic neighbor embedding.",
        NoParams(), true, McpRequiresGui);

    // =====================================================================
    // regress
    // =====================================================================
    Add(tools, "regress/classic", "Classic Regression", "Regress",
        "Ordinary least squares regression of a dependent variable on "
        "independent variables.",
        Schema(std::vector<json_spirit::Pair>{
                   P("dependent", StrProp("Dependent variable")),
                   P("independent",
                     ArrProp("Independent variables", "string", 1, 0)),
                   P("include_constant",
                     Def(BoolProp("Include a constant term"), true))},
               {"dependent", "independent"}),
        true, McpRegressClassic);

    // =====================================================================
    // window
    // =====================================================================
    Add(tools, "window/create_map", "Create Map", "Window",
        "Create a choropleth map window with a classification theme and "
        "optional rate smoothing.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("theme",
                     Def(EnumProp("Classification theme",
                                  {"quantile", "natural_breaks",
                                   "equal_intervals", "percentile", "stddev",
                                   "unique_values", "no_theme"}), "quantile")),
                   P("num_categories", Def(IntProp("Number of classes"), 5)),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("smoothing",
                     Def(EnumProp("Rate smoothing",
                                  {"no_smoothing", "raw_rate", "excess_risk",
                                   "empirical_bayes", "spatial_rate",
                                   "spatial_empirical_bayes"}),
                         "no_smoothing")),
                   P("return_image",
                     Def(BoolProp("Return a PNG snapshot of the map as an MCP "
                                  "image content block"), false))},
               {"column"}),
        false, McpWindowCreateMap);

    Add(tools, "window/create_plot", "Create Plot", "Window",
        "Create a histogram, box plot, or scatter plot window.",
        Schema(std::vector<json_spirit::Pair>{
                   P("plot_type",
                     EnumProp("Plot type", {"histogram", "boxplot", "scatter"})),
                   P("columns", ArrProp("Column names", "string", 1, 0)),
                   P("bins",
                     IntProp("Number of histogram bins (histogram only; "
                             "uses GeoDa's default when omitted)")),
                   P("title", StrProp("Window title (optional)")),
                   P("return_image",
                     Def(BoolProp("Return a PNG snapshot of the plot as an MCP "
                                  "image content block"), false))},
               {"plot_type", "columns"}),
        false, McpWindowCreatePlot);

    Add(tools, "window/create_lisa_map", "Create LISA Map", "Window",
        "Create a live LISA Cluster Map or LISA Significance Map window.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("map_type",
                     Def(EnumProp("Map type", {"cluster", "significance"}),
                         "cluster")),
                   P("permutations", Def(IntProp("Number of permutations"), 999)),
                   P("significance_cutoff",
                     Def(NumProp("Significance level"), 0.05)),
                   P("row_standardize", Def(BoolProp("Row-standardize"), true)),
                   P("lisa_type",
                     Def(EnumProp("LISA type",
                                  {"univariate", "bivariate", "differential",
                                   "eb_rate_standardized"}), "univariate")),
                   P("second_column",
                     StrProp("Second column for bivariate/differential")),
                   P("return_image",
                     Def(BoolProp("Return a PNG snapshot of the map as an MCP "
                                  "image content block"), false))},
               {"column", "weights"}),
        false, McpWindowCreateLisaMap);

    Add(tools, "window/create_scatterplot_matrix", "Scatter Plot Matrix",
        "Window",
        "Create a scatter plot matrix window.",
        NoParams(), false, McpWindowCreateScatterPlotMatrix);

    Add(tools, "window/create_bubble_chart", "Bubble Chart", "Window",
        "Create a bubble chart window (x, y, size, color).",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns",
                     ArrProp("At least 3 column names (x, y, size)", "string", 3, 0))},
               {"columns"}),
        false, McpWindowCreateBubbleChart);

    Add(tools, "window/create_3d_scatter", "3D Scatter", "Window",
        "Create a 3D scatter plot window (x, y, z).",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns",
                     ArrProp("At least 3 column names (x, y, z)", "string", 3, 0))},
               {"columns"}),
        false, McpWindowCreate3DScatter);

    Add(tools, "window/create_pcp", "Parallel Coordinate Plot", "Window",
        "Create a parallel coordinate plot window.",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Column names", "string", 1, 0))},
               {"columns"}),
        false, McpWindowCreatePcp);

    Add(tools, "window/create_line_chart", "Line Chart", "Window",
        "Create an average comparison line chart window.",
        NoParams(), false, McpWindowCreateLineChart);

    Add(tools, "window/create_correlogram", "Correlogram", "Window",
        "Create a spatial correlogram window.",
        NoParams(), false, McpWindowCreateCorrelogram);

    Add(tools, "window/create_distance_plot", "Distance Plot", "Window",
        "Create a distance plot window for two columns.",
        Schema(std::vector<json_spirit::Pair>{
                   P("x_column", StrProp("X column name")),
                   P("y_column", StrProp("Y column name"))},
               {"x_column", "y_column"}),
        false, McpWindowCreateDistancePlot);

    Add(tools, "window/create_moran_scatterplot", "Moran Scatterplot",
        "Window",
        "Create a Moran scatter plot window for a column and weights.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)"))},
               {"column", "weights"}),
        false, McpWindowCreateMoranScatterplot);

    // =====================================================================
    // Space menu actions
    // =====================================================================
    Add(tools, "space/global_moran", "Global Moran's I", "Space",
        "Global Moran's I for a column with the given weights.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("permutations", Def(IntProp("Number of permutations"), 999))},
               {"column", "weights"}),
        true, McpGlobalMoran);

    Add(tools, "space/lisa_univariate", "Univariate Local Moran", "Space",
        "Univariate Local Moran's I with pseudo p-values and clusters.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("permutations", Def(IntProp("Number of permutations"), 999)),
                   P("significance_cutoff",
                     Def(NumProp("Significance level"), 0.05))},
               {"column", "weights"}),
        true, McpSpaceLisaUnivariate);

    Add(tools, "space/lisa_bivariate", "Bivariate Local Moran", "Space",
        "Bivariate Local Moran's I (requires second_column).",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("second_column", StrProp("Second column")),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("permutations", Def(IntProp("Number of permutations"), 999)),
                   P("significance_cutoff",
                     Def(NumProp("Significance level"), 0.05))},
               {"column", "second_column", "weights"}),
        true, McpSpaceLisaBivariate);

    Add(tools, "space/lisa_differential", "Differential Local Moran", "Space",
        "Differential Local Moran's I (requires second_column).",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("second_column", StrProp("Second column")),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("permutations", Def(IntProp("Number of permutations"), 999)),
                   P("significance_cutoff",
                     Def(NumProp("Significance level"), 0.05))},
               {"column", "second_column", "weights"}),
        true, McpSpaceLisaDifferential);

    Add(tools, "space/lisa_eb", "EB Rate Local Moran", "Space",
        "Empirical Bayes rate-standardized Local Moran's I.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("permutations", Def(IntProp("Number of permutations"), 999)),
                   P("significance_cutoff",
                     Def(NumProp("Significance level"), 0.05))},
               {"column", "weights"}),
        true, McpSpaceLisaEb);

    Add(tools, "space/local_g", "Local Getis-Ord G", "Space",
        "Local Getis-Ord G statistic.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("permutations", Def(IntProp("Number of permutations"), 999)),
                   P("significance_cutoff",
                     Def(NumProp("Significance level"), 0.05))},
               {"column", "weights"}),
        true, McpLisaLocalG);

    Add(tools, "space/local_g_star", "Local Getis-Ord G*", "Space",
        "Local Getis-Ord G* statistic (includes self in neighborhood).",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("permutations", Def(IntProp("Number of permutations"), 999)),
                   P("significance_cutoff",
                     Def(NumProp("Significance level"), 0.05))},
               {"column", "weights"}),
        true, McpSpaceLocalGStar);

    Add(tools, "space/local_geary", "Local Geary", "Space",
        "Univariate Local Geary statistic.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("permutations", Def(IntProp("Number of permutations"), 999)),
                   P("significance_cutoff",
                     Def(NumProp("Significance level"), 0.05))},
               {"column", "weights"}),
        true, McpLisaLocalGeary);

    Add(tools, "space/local_geary_multivariate", "Multivariate Local Geary",
        "Space",
        "Multivariate Local Geary statistic (requires second_column).",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("second_column", StrProp("Second column")),
                   P("weights", StrProp("Weights id (uuid)")),
                   P("permutations", Def(IntProp("Number of permutations"), 999)),
                   P("significance_cutoff",
                     Def(NumProp("Significance level"), 0.05))},
               {"column", "second_column", "weights"}),
        true, McpSpaceLocalGearyMultivariate);

    Add(tools, "space/moran_scatterplot", "Moran Scatterplot", "Space",
        "Create a Moran scatter plot window.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)"))},
               {"column", "weights"}),
        false, McpWindowCreateMoranScatterplot);

    Add(tools, "space/correlogram", "Correlogram", "Space",
        "Create a spatial correlogram window.",
        NoParams(), false, McpWindowCreateCorrelogram);

    Add(tools, "space/distance_plot", "Distance Plot", "Space",
        "Create a distance plot window for two columns.",
        Schema(std::vector<json_spirit::Pair>{
                   P("x_column", StrProp("X column name")),
                   P("y_column", StrProp("Y column name"))},
               {"x_column", "y_column"}),
        false, McpWindowCreateDistancePlot);

    // =====================================================================
    // Map menu actions
    // =====================================================================
    Add(tools, "map/quantile", "Quantile Map", "Map",
        "Create a quantile choropleth map.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("num_categories", Def(IntProp("Number of classes"), 5))},
               {"column"}),
        false, McpMapQuantile);

    Add(tools, "map/natural_breaks", "Natural Breaks Map", "Map",
        "Create a natural breaks (Jenks) choropleth map.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("num_categories", Def(IntProp("Number of classes"), 5))},
               {"column"}),
        false, McpMapNaturalBreaks);

    Add(tools, "map/equal_intervals", "Equal Intervals Map", "Map",
        "Create an equal intervals choropleth map.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("num_categories", Def(IntProp("Number of classes"), 5))},
               {"column"}),
        false, McpMapEqualIntervals);

    Add(tools, "map/percentile", "Percentile Map", "Map",
        "Create a percentile choropleth map.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("num_categories", Def(IntProp("Number of classes"), 5))},
               {"column"}),
        false, McpMapPercentile);

    Add(tools, "map/stddev", "Standard Deviation Map", "Map",
        "Create a standard deviation choropleth map.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name"))},
               {"column"}),
        false, McpMapStddev);

    Add(tools, "map/unique_values", "Unique Values Map", "Map",
        "Create a unique values choropleth map.",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Column name"))},
               {"column"}),
        false, McpMapUniqueValues);

    Add(tools, "map/rates_raw", "Raw Rate Map", "Map",
        "Create a raw rate map (requires weights).",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)"))},
               {"column", "weights"}),
        false, McpMapRatesRaw);

    Add(tools, "map/rates_excess_risk", "Excess Risk Map", "Map",
        "Create an excess risk map (requires weights).",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)"))},
               {"column", "weights"}),
        false, McpMapRatesExcessRisk);

    Add(tools, "map/rates_eb", "Empirical Bayes Map", "Map",
        "Create an empirical Bayes smoothed rate map (requires weights).",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)"))},
               {"column", "weights"}),
        false, McpMapRatesEb);

    Add(tools, "map/rates_spatial", "Spatial Rate Map", "Map",
        "Create a spatial rate smoothed map (requires weights).",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)"))},
               {"column", "weights"}),
        false, McpMapRatesSpatial);

    Add(tools, "map/rates_spatial_eb", "Spatial EB Rate Map", "Map",
        "Create a spatial empirical Bayes smoothed rate map (requires "
        "weights).",
        Schema(std::vector<json_spirit::Pair>{
                   P("column", StrProp("Numeric column name")),
                   P("weights", StrProp("Weights id (uuid)"))},
               {"column", "weights"}),
        false, McpMapRatesSpatialEb);

    // =====================================================================
    // Explore menu actions
    // =====================================================================
    Add(tools, "explore/histogram", "Histogram", "Explore",
        "Create a histogram window.",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("One column name", "string", 1, 1)),
                   P("bins",
                     IntProp("Number of histogram bins (uses GeoDa's default "
                             "when omitted)"))},
               {"columns"}),
        false, McpExploreHistogram);

    Add(tools, "explore/boxplot", "Box Plot", "Explore",
        "Create a box plot window.",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("One column name", "string", 1, 1))},
               {"columns"}),
        false, McpExploreBoxplot);

    Add(tools, "explore/scatterplot", "Scatter Plot", "Explore",
        "Create a scatter plot window.",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Two column names (x, y)", "string", 2, 2))},
               {"columns"}),
        false, McpExploreScatterplot);

    Add(tools, "explore/scatterplot_matrix", "Scatter Plot Matrix", "Explore",
        "Create a scatter plot matrix window.",
        NoParams(), false, McpWindowCreateScatterPlotMatrix);

    Add(tools, "explore/bubble_chart", "Bubble Chart", "Explore",
        "Create a bubble chart window.",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns",
                     ArrProp("At least 3 column names (x, y, size)", "string", 3, 0))},
               {"columns"}),
        false, McpWindowCreateBubbleChart);

    Add(tools, "explore/3d_scatter", "3D Scatter", "Explore",
        "Create a 3D scatter plot window.",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns",
                     ArrProp("At least 3 column names (x, y, z)", "string", 3, 0))},
               {"columns"}),
        false, McpWindowCreate3DScatter);

    Add(tools, "explore/pcp", "Parallel Coordinate Plot", "Explore",
        "Create a parallel coordinate plot window.",
        Schema(std::vector<json_spirit::Pair>{
                   P("columns", ArrProp("Column names", "string", 1, 0))},
               {"columns"}),
        false, McpWindowCreatePcp);

    Add(tools, "explore/line_chart", "Line Chart", "Explore",
        "Create an average comparison line chart window.",
        NoParams(), false, McpWindowCreateLineChart);

    // =====================================================================
    // File / Table / Tools / Time / Help menu actions (require the GUI)
    // =====================================================================
    Add(tools, "file/save", "Save Project", "File",
        "Save the current project. Requires the GeoDa GUI.",
        NoParams(), false, McpRequiresGui);

    Add(tools, "file/save_as", "Save Project As", "File",
        "Save the current project under a new name. Requires the GeoDa GUI.",
        NoParams(), false, McpRequiresGui);

    Add(tools, "file/export", "Export Data", "File",
        "Export the open data set to a file via OGR. Geometry data is "
        "included when the format supports it (e.g. GeoJSON, GeoPackage, "
        "Shapefile, CSV, KML). For GeoJSON the output is automatically "
        "reprojected to EPSG:4326.",
        Schema(std::vector<json_spirit::Pair>{
                   P("path",
                     StrProp("Output file path (extension selects the "
                             "format, e.g. .geojson, .gpkg, .shp, .csv)")),
                   P("format",
                     StrProp("OGR format name, overrides the extension, e.g. "
                             "\"GeoJSON\", \"ESRI Shapefile\", \"GeoPackage\"")),
                   P("crs",
                     StrProp("Optional PROJ string to reproject to (GeoJSON "
                             "always uses EPSG:4326)")),
                   P("layer_id",
                     StrProp("Optional layer name (defaults to the table "
                             "name)"))},
               {"path"}),
        true, McpFileExport);

    Add(tools, "table/export", "Export Table Columns", "Table",
        "Export a subset of the table columns (with geometry when the data "
        "is spatial) to a file via OGR. Unlike file/export, only the "
        "requested columns are written, so e.g. an ID, a cluster label and "
        "centroid coordinates can be saved without the whole table. Geometry "
        "data is included when the format supports it. For GeoJSON the "
        "output is automatically reprojected to EPSG:4326.",
        Schema(std::vector<json_spirit::Pair>{
                   P("path",
                     StrProp("Output file path (extension selects the "
                             "format, e.g. .geojson, .gpkg, .shp, .csv)")),
                   P("columns",
                     ArrProp("Column names to export; defaults to all "
                             "columns", "string")),
                   P("include_geometry",
                     Def(BoolProp("Include geometry when the format supports "
                                  "it"), true)),
                   P("format",
                     StrProp("OGR format name, overrides the extension, e.g. "
                             "\"GeoJSON\", \"ESRI Shapefile\", \"GeoPackage\"")),
                   P("crs",
                     StrProp("Optional PROJ string to reproject to (GeoJSON "
                             "always uses EPSG:4326)")),
                   P("layer_id",
                     StrProp("Optional layer name (defaults to the table "
                             "name)"))},
               {"path"}),
        true, McpTableExport);

    Add(tools, "table/aggregation", "Aggregation", "Table",
        "Aggregate the table. Requires the GeoDa GUI.",
        NoParams(), false, McpRequiresGui);

    Add(tools, "table/merge", "Merge", "Table",
        "Merge tables. Requires the GeoDa GUI.",
        NoParams(), false, McpRequiresGui);

    Add(tools, "table/field_calc", "Field Calculation", "Table",
        "Calculate a new field. Requires the GeoDa GUI.",
        NoParams(), false, McpRequiresGui);

    Add(tools, "table/select", "Select", "Table",
        "Select observations. Requires the GeoDa GUI.",
        NoParams(), false, McpRequiresGui);

    Add(tools, "table/delete", "Delete", "Table",
        "Delete observations. Requires the GeoDa GUI.",
        NoParams(), false, McpRequiresGui);

    Add(tools, "tools/weights_manager", "Weights Manager", "Tools",
        "Open the weights manager. Requires the GeoDa GUI.",
        NoParams(), false, McpRequiresGui);

    Add(tools, "tools/table", "Table", "Tools",
        "Open the table window. Requires the GeoDa GUI.",
        NoParams(), false, McpRequiresGui);

    Add(tools, "time/chooser", "Time Chooser", "Time",
        "Open the time chooser. Requires the GeoDa GUI.",
        NoParams(), false, McpRequiresGui);

    Add(tools, "help/about", "About GeoDa", "Help",
        "Show the About dialog. Requires the GeoDa GUI.",
        NoParams(), false, McpRequiresGui);

    Add(tools, "help/help", "Help", "Help",
        "Open the help. Requires the GeoDa GUI.",
        NoParams(), false, McpRequiresGui);
}
