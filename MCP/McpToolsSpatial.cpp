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

#include "MCP/McpToolsSpatial.h"
#include "MCP/McpTools.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <wx/wx.h>
#include <wx/mstream.h>
#include <wx/base64.h>

#include "GdaJson.h"
#include "GeoDa.h"
#include "TemplateFrame.h"
#include "TemplateCanvas.h"
#include "Project.h"
#include "GdaConst.h"
#include "GenUtils.h"
#include "VarTools.h"
#include "SpatialIndAlgs.h"
#include "GdaException.h"
#include "DataViewer/TableInterface.h"
#include "DataViewer/DataSource.h"
#include "ShapeOperations/OGRDataAdapter.h"
#include "VarCalc/WeightsManInterface.h"
#include "VarCalc/WeightsMetaInfo.h"
#include "ShapeOperations/WeightsManager.h"
#include "ShapeOperations/GalWeight.h"
#include "ShapeOperations/GwtWeight.h"
#include "ShapeOperations/GeodaWeight.h"
#include "ShapeOperations/PolysToContigWeights.h"
#include "ShapeOperations/Randik.h"
#include "Explore/AbstractCoordinator.h"
#include "Explore/LisaCoordinator.h"
#include "Explore/LocalGearyCoordinator.h"
#include "Explore/GStatCoordinator.h"
#include "Explore/MapNewView.h"
#include "Explore/HistogramView.h"
#include "Explore/BoxNewPlotView.h"
#include "Explore/ScatterNewPlotView.h"
#include "Explore/LisaMapNewView.h"
#include "Explore/LisaScatterPlotView.h"
#include "Explore/ScatterPlotMatView.h"
#include "Explore/PCPNewView.h"
#include "Explore/LineChartView.h"
#include "Explore/CorrelogramView.h"
#include "Explore/DistancePlotView.h"
#include "Explore/3DPlotView.h"
#include "Explore/CatClassification.h"
#include "Regression/DiagnosticReport.h"
#include "Algorithms/redcap.h"
#include "Algorithms/azp.h"
#include "Algorithms/cluster.h"
#include "Algorithms/DataUtils.h"
#include "Algorithms/dbscan.h"
#include "Algorithms/hdbscan.h"
#include "Algorithms/spectral.h"
#include "Algorithms/pam.h"
#include "Algorithms/mds.h"
#include "Algorithms/pca.h"
#include "Algorithms/spatial_kmeans.h"
#include "Weights/DistUtils.h"

// Declared in Regression/smile2.cpp (no public header).
bool classicalRegression(GalElement *g, int num_obs, double *Y, int dim,
                         double **X, int expl, DiagnosticReport *dr,
                         bool InclConstant, bool m_moranz, wxGauge* gauge,
                         bool do_white_test);

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

    json_spirit::Value Arr(const std::vector<json_spirit::Value>& vals)
    {
        return json_spirit::Value(json_spirit::Array(vals));
    }

    // ---------------------------------------------------------------------
    // Parameter getters. All return the default when the key is absent.
    // ---------------------------------------------------------------------
    wxString GetStr(const json_spirit::Object& params, const char* name,
                    const wxString& def = wxString())
    {
        json_spirit::Value v;
        if (GdaJson::findValue(params, v, name) &&
            v.type() == json_spirit::str_type) {
            return wxString::FromUTF8(v.get_str().c_str());
        }
        return def;
    }

    int GetInt(const json_spirit::Object& params, const char* name, int def)
    {
        json_spirit::Value v;
        if (GdaJson::findValue(params, v, name) &&
            v.type() == json_spirit::int_type) {
            return v.get_int();
        }
        return def;
    }

    double GetDouble(const json_spirit::Object& params, const char* name,
                     double def)
    {
        json_spirit::Value v;
        if (GdaJson::findValue(params, v, name) &&
            (v.type() == json_spirit::int_type ||
             v.type() == json_spirit::real_type)) {
            return v.get_real();
        }
        return def;
    }

    bool GetBool(const json_spirit::Object& params, const char* name, bool def)
    {
        json_spirit::Value v;
        if (GdaJson::findValue(params, v, name) &&
            v.type() == json_spirit::bool_type) {
            return v.get_bool();
        }
        return def;
    }

    std::vector<wxString> GetStrArray(const json_spirit::Object& params,
                                      const char* name)
    {
        std::vector<wxString> out;
        json_spirit::Value v;
        if (GdaJson::findValue(params, v, name) &&
            v.type() == json_spirit::array_type) {
            const json_spirit::Array& arr = v.get_array();
            for (size_t i = 0; i < arr.size(); ++i) {
                if (arr[i].type() == json_spirit::str_type) {
                    out.push_back(
                        wxString::FromUTF8(arr[i].get_str().c_str()));
                }
            }
        }
        return out;
    }

    // ---------------------------------------------------------------------
    // Image capture. Renders a TemplateFrame's canvas to a base64-encoded
    // PNG so window tools can return a snapshot to the MCP client. Mirrors
    // TemplateFrame::ExportImage (TemplateFrame.cpp) but writes to memory.
    // Must run on the main thread (wx/GL state) -- window tools register
    // with run_on_worker == false.
    // ---------------------------------------------------------------------
    std::string RenderFrameToPNGBase64(TemplateFrame* frame)
    {
        if (!frame || !frame->template_canvas) {
            return "";
        }
        TemplateCanvas* canvas = frame->template_canvas;

        // The frame was just constructed; the event loop has not run, so the
        // canvas is still 0x0 until laid out. Lay it out now (falling back
        // to a default window size) so the render target has real pixels.
        frame->Layout();
        int w = canvas->GetClientSize().GetWidth();
        int h = canvas->GetClientSize().GetHeight();
        if (w <= 0 || h <= 0) {
            wxSize fs = frame->GetClientSize();
            if (fs.GetWidth() <= 0 || fs.GetHeight() <= 0) {
                frame->SetClientSize(800, 600);
            }
            frame->Layout();
            w = canvas->GetClientSize().GetWidth();
            h = canvas->GetClientSize().GetHeight();
            if (w <= 0 || h <= 0) {
                return "";
            }
        }
        // Render at exactly the canvas's client size: RenderToDC() resizes the
        // canvas's layer bitmaps to whatever it is handed and restores them
        // only on the next idle event, while OnPaint() blits GetClientSize()
        // from layer2_bm. Rendering at any other size leaves layer2_bm smaller
        // than the paint rect and trips wxBitmap::GetSubBitmap. The client size
        // is already in logical units, so do not scale it down again.
        wxBitmap canvas_bm;
        canvas_bm.CreateScaled(w, h, 32, 1.0);
        wxMemoryDC canvas_dc(canvas_bm);
        canvas_dc.SetBackground(*wxWHITE_BRUSH);
        canvas_dc.Clear();
        canvas->RenderToDC(canvas_dc, w, h);

        wxImage img = canvas_bm.ConvertToImage();
        if (!img.IsOk()) {
            return "";
        }
        wxMemoryOutputStream os;
        if (!img.SaveFile(os, wxBITMAP_TYPE_PNG)) {
            return "";
        }
        wxStreamBuffer* buf = os.GetOutputStreamBuffer();
        size_t len = buf ? buf->GetBufferSize() : 0;
        const void* data = buf ? (const void*)buf->GetBufferStart() : NULL;
        if (!data || len == 0) {
            return "";
        }
        return wxBase64Encode(data, len).ToStdString();
    }

    // Build a tool result that carries an image snapshot as an MCP image
    // content block alongside the usual JSON summary. McpServer.cpp passes
    // the "content" array through verbatim when present.
    json_spirit::Value ImageResult(const std::string& base64_png,
                                   const json_spirit::Value& summary)
    {
        std::vector<json_spirit::Pair> img;
        img.push_back(P("type", json_spirit::Value("image")));
        img.push_back(P("data", json_spirit::Value(base64_png)));
        img.push_back(P("mimeType", json_spirit::Value("image/png")));

        std::vector<json_spirit::Pair> txt;
        txt.push_back(P("type", json_spirit::Value("text")));
        txt.push_back(P("text",
            json_spirit::Value(wxString::FromUTF8(
                json_spirit::write(summary).c_str()).ToStdString())));

        json_spirit::Array content;
        content.push_back(Obj(img));
        content.push_back(Obj(txt));

        std::vector<json_spirit::Pair> r;
        r.push_back(P("content", json_spirit::Value(content)));
        return Obj(r);
    }

    // ---------------------------------------------------------------------
    // Project / table / column validation
    // ---------------------------------------------------------------------
    Project* RequireProject(const McpToolContext& ctx)
    {
        if (!ctx.project) {
            throw McpError(-32602, "No project is open. Open a data set first.");
        }
        return ctx.project;
    }

    TableInterface* RequireTable(const McpToolContext& ctx)
    {
        Project* project = RequireProject(ctx);
        TableInterface* table = project->GetTableInt();
        if (!table) {
            throw McpError(-32602, "The open project has no table.");
        }
        return table;
    }

    int RequireColumn(TableInterface* table, const wxString& name)
    {
        int col = table->FindColId(name);
        if (col < 0) {
            throw McpError(-32602,
                "Unknown column: " + name.ToStdString());
        }
        return col;
    }

    // Ask the user which variable an analysis should use, when the request did
    // not say. Only numeric columns are offered -- that is what the analysis
    // tools accept. Returns an empty string when the client cannot be asked or
    // the user declined, and the caller then raises the "missing parameter"
    // error it raised before.
    wxString AskForColumn(const McpToolContext& ctx, TableInterface* table)
    {
        if (!ctx.elicit || !ctx.elicit->Available()) return wxString();

        std::vector<std::string> options;
        int ncols = table->GetNumberCols();
        for (int c = 0; c < ncols; ++c) {
            GdaConst::FieldType type = table->GetColType(c, 0);
            if (type == GdaConst::double_type ||
                type == GdaConst::long64_type) {
                options.push_back(table->GetColName(c).ToStdString());
            }
        }
        if (options.empty()) return wxString();

        std::string answer;
        if (!ctx.elicit->Ask("Which variable should be used?", "column",
                             "Variable", options, "", answer)) {
            return wxString();
        }
        return wxString::FromUTF8(answer.c_str());
    }

    // Validate that the given columns' field names can be written to the
    // target format. The GUI export path fixes illegal or duplicated names
    // with a modal FieldNameCorrectionDlg; from a worker thread that dialog
    // cannot be shown, so reject with a clear error instead.
    void ValidateExportFieldNames(TableInterface* table,
                                  const std::vector<int>& col_ids,
                                  GdaConst::DataSourceType ds_type,
                                  const wxString& format)
    {
        std::map<wxString, bool> seen_names;
        int time_steps = table->GetTimeSteps();
        for (size_t c = 0; c < col_ids.size(); ++c) {
            int id = col_ids[c];
            std::vector<wxString> names;
            if (table->IsColTimeVariant(id)) {
                for (int t = 0; t < time_steps; ++t) {
                    wxString fname = table->GetColName(id, t);
                    if (!fname.IsEmpty()) names.push_back(fname);
                }
            } else {
                wxString fname = table->GetColName(id);
                if (!fname.IsEmpty()) names.push_back(fname);
            }
            for (size_t k = 0; k < names.size(); ++k) {
                wxString fname = names[k];
                std::map<GdaConst::DataSourceType, int>::const_iterator lit =
                    GdaConst::datasrc_field_lens.find(ds_type);
                if (lit != GdaConst::datasrc_field_lens.end() &&
                    lit->second > 0 && fname.length() > lit->second) {
                    wxString msg = wxString::Format(
                        "Field name \"%s\" is longer than %d characters, which "
                        "%s cannot store. Rename the column first.",
                        fname, lit->second, format);
                    throw McpError(-32602, msg.ToStdString());
                }
                std::map<GdaConst::DataSourceType, wxString>::const_iterator rit =
                    GdaConst::datasrc_field_regex.find(ds_type);
                if (rit != GdaConst::datasrc_field_regex.end() &&
                    !rit->second.IsEmpty()) {
                    wxRegEx regex;
                    regex.Compile(rit->second);
                    if (!regex.Matches(fname)) {
                        wxString msg = wxString::Format(
                            "Field name \"%s\" is not valid for %s. Rename the "
                            "field first.",
                            fname, format);
                        throw McpError(-32602, msg.ToStdString());
                    }
                }
                wxString key = fname.Upper();
                if (seen_names.find(key) != seen_names.end()) {
                    wxString msg = wxString::Format(
                        "Duplicate field name \"%s\" cannot be exported to %s. "
                        "Rename one of the fields first.",
                        fname, format);
                    throw McpError(-32602, msg.ToStdString());
                }
                seen_names[key] = true;
            }
        }
    }

    // ---------------------------------------------------------------------
    // Weights helpers
    // ---------------------------------------------------------------------
    boost::uuids::uuid ParseUuid(const wxString& id)
    {
        try {
            boost::uuids::string_generator gen;
            return gen(id.ToStdString());
        } catch (...) {
            throw McpError(-32602,
                "Invalid weights id: " + id.ToStdString());
        }
    }

    WeightsManInterface* RequireWeightsMan(const McpToolContext& ctx)
    {
        Project* project = RequireProject(ctx);
        WeightsManInterface* wmi = project->GetWManInt();
        if (!wmi) {
            throw McpError(-32602, "Weights manager is not available.");
        }
        return wmi;
    }

    boost::uuids::uuid RequireWeights(const McpToolContext& ctx,
                                      const wxString& id)
    {
        WeightsManInterface* wmi = RequireWeightsMan(ctx);
        boost::uuids::uuid uid = ParseUuid(id);
        if (!wmi->WeightsExists(uid)) {
            throw McpError(-32602,
                "Unknown weights id: " + id.ToStdString());
        }
        return uid;
    }

    GalWeight* GetGalWeight(const McpToolContext& ctx, const wxString& id)
    {
        boost::uuids::uuid uid = RequireWeights(ctx, id);
        GalWeight* gw = ((WeightsNewManager*)ctx.project->GetWManInt())
                            ->GetGal(uid);
        if (!gw) {
            throw McpError(-32602,
                "Weights id is not a spatial weights object: " +
                id.ToStdString());
        }
        return gw;
    }

    // Convert a GwtWeight to a GalWeight (binary adjacency). The caller owns
    // the returned GalWeight.
    GalWeight* GwtToGal(const GwtWeight* gwt_w, int num_obs)
    {
        GalWeight* gal_w = new GalWeight();
        gal_w->num_obs = num_obs;
        gal_w->gal = new GalElement[num_obs];
        for (int i = 0; i < num_obs; ++i) {
            const GwtElement& ge = gwt_w->gwt[i];
            gal_w->gal[i].SetSizeNbrs(ge.Size());
            for (long j = 0; j < ge.Size(); ++j) {
                gal_w->gal[i].SetNbr(j, ge.elt(j).nbx);
            }
        }
        return gal_w;
    }

    // Register a GalWeight with the weights manager and return its uuid.
    boost::uuids::uuid RegisterGalWeight(Project* project, GalWeight* w,
                                        WeightsMetaInfo::WeightTypeEnum type,
                                        const wxString& id_field)
    {
        WeightsManInterface* wmi = project->GetWManInt();
        w->GetNbrStats();
        WeightsMetaInfo wmi_info;
        wmi_info.num_obs = w->GetNumObs();
        wmi_info.id_var = id_field;
        wmi_info.SetSymmetric(w->is_symmetric);
        wmi_info.SetMinNumNbrs(w->GetMinNumNbrs());
        wmi_info.SetMaxNumNbrs(w->GetMaxNumNbrs());
        wmi_info.SetMeanNumNbrs(w->GetMeanNumNbrs());
        wmi_info.SetMedianNumNbrs(w->GetMedianNumNbrs());
        wmi_info.SetSparsity(w->GetSparsity());
        wmi_info.SetDensity(w->GetDensity());
        wmi_info.SetWeightsType(type);
        WeightsMetaInfo e(wmi_info);
        boost::uuids::uuid uid = wmi->RequestWeights(e);
        if (!uid.is_nil()) {
            // RequestWeights always creates a fresh entry (with no gal_weight
            // attached), so AssociateGal must be called to attach the weights.
            bool success = ((WeightsNewManager*)wmi)->AssociateGal(uid, w);
            if (success) wmi->MakeDefault(uid);
        }
        return uid;
    }

    // ---------------------------------------------------------------------
    // Column data extraction
    // ---------------------------------------------------------------------
    // Build a GdaVarTools::VarInfo for a single column (time 0).
    GdaVarTools::VarInfo BuildVarInfo(TableInterface* table,
                                      const wxString& name)
    {
        int col = RequireColumn(table, name);
        GdaVarTools::VarInfo v;
        v.time = 0;
        v.name = name;
        v.is_time_variant = table->IsColTimeVariant(name);
        table->GetMinMaxVals(col, v.min, v.max);
        v.sync_with_global_time = v.is_time_variant;
        v.fixed_scale = true;
        return v;
    }

    // Extract a numeric column into data/undefs. Throws for non-numeric
    // columns.
    void GetNumericColumn(TableInterface* table, const wxString& name,
                          std::vector<double>& data,
                          std::vector<bool>& undefs)
    {
        int col = RequireColumn(table, name);
        GdaConst::FieldType type = table->GetColType(col, 0);
        if (type == GdaConst::string_type || type == GdaConst::date_type ||
            type == GdaConst::time_type ||
            type == GdaConst::datetime_type) {
            throw McpError(-32602,
                "Column is not numeric: " + name.ToStdString());
        }
        table->GetColData(col, 0, data, undefs);
    }

    // ---------------------------------------------------------------------
    // Global spatial statistics (no existing coordinator)
    // ---------------------------------------------------------------------
    // Compute the global Moran's I with the standard formula:
    //   I = (n / S0) * sum_ij w_ij z_i z_j / sum_i z_i^2
    // where z = x - xbar and S0 = sum of all weights. This is the regression
    // slope of Wz on z (through origin) when the weights are row-standardized,
    // and reduces to the general form for unstandardized weights.
    // Returns {I, expected, p_value, permutations}.
    void GlobalMoran(const std::vector<double>& x,
                     const std::vector<bool>& undefs,
                     const GalWeight* gw, int permutations,
                     double& I, double& expected, double& p_value)
    {
        int n = (int)x.size();
        double sum_x = 0.0;
        int valid = 0;
        for (int i = 0; i < n; ++i) {
            if (undefs[i]) continue;
            sum_x += x[i];
            ++valid;
        }
        if (valid < 2) {
            throw McpError(-32602, "Not enough valid observations.");
        }
        double xbar = sum_x / valid;

        // Centered values and S0 (sum of all weights).
        std::vector<double> z(n, 0.0);
        double S0 = 0.0;
        for (int i = 0; i < n; ++i) {
            if (undefs[i]) continue;
            z[i] = x[i] - xbar;
            const GalElement& ge = gw->gal[i];
            for (long j = 0; j < ge.Size(); ++j) {
                long nb = ge[j];
                if (nb >= 0 && nb < n && !undefs[nb]) S0 += 1.0;
            }
        }
        if (S0 <= 0) {
            throw McpError(-32602, "Weights matrix has no connections.");
        }

        double num = 0.0, den = 0.0;
        for (int i = 0; i < n; ++i) {
            if (undefs[i]) continue;
            den += z[i] * z[i];
            const GalElement& ge = gw->gal[i];
            for (long j = 0; j < ge.Size(); ++j) {
                long nb = ge[j];
                if (nb >= 0 && nb < n && !undefs[nb]) num += z[i] * z[nb];
            }
        }
        if (den <= 0) {
            throw McpError(-32602, "Variable has zero variance.");
        }
        I = (n / S0) * num / den;
        expected = -1.0 / (valid - 1);

        // Permutation test (denominator is invariant under permutation).
        int count = 0;
        Randik rnd;
        std::vector<int> perm(n);
        std::vector<long> rands(n);
        for (int p = 0; p < permutations; ++p) {
            rnd.Perm(undefs, n, &perm[0], &rands[0]);
            double num_p = 0.0;
            for (int i = 0; i < n; ++i) {
                if (undefs[i]) continue;
                const GalElement& ge = gw->gal[i];
                for (long j = 0; j < ge.Size(); ++j) {
                    long nb = ge[j];
                    if (nb >= 0 && nb < n && !undefs[nb])
                        num_p += z[perm[i]] * z[perm[nb]];
                }
            }
            double I_p = (n / S0) * num_p / den;
            if (I_p >= I) ++count;
        }
        p_value = (double)(count + 1) / (double)(permutations + 1);
    }

    // Compute the global Geary's C with the standard formula:
    //   C = (n-1) / (2*S0) * sum_ij w_ij (x_i - x_j)^2 / sum_i (x_i - xbar)^2
    // where S0 = sum of all weights.
    void GlobalGeary(const std::vector<double>& x,
                     const std::vector<bool>& undefs,
                     const GalWeight* gw, int permutations,
                     double& C, double& expected, double& p_value)
    {
        int n = (int)x.size();
        double sum_x = 0.0;
        int valid = 0;
        for (int i = 0; i < n; ++i) {
            if (undefs[i]) continue;
            sum_x += x[i];
            ++valid;
        }
        if (valid < 2) {
            throw McpError(-32602, "Not enough valid observations.");
        }
        double xbar = sum_x / valid;

        double S0 = 0.0, sum_diff = 0.0, sum_var = 0.0;
        for (int i = 0; i < n; ++i) {
            if (undefs[i]) continue;
            double d = x[i] - xbar;
            sum_var += d * d;
            const GalElement& ge = gw->gal[i];
            for (long j = 0; j < ge.Size(); ++j) {
                long nb = ge[j];
                if (nb >= 0 && nb < n && !undefs[nb]) {
                    S0 += 1.0;
                    double dd = x[i] - x[nb];
                    sum_diff += dd * dd;
                }
            }
        }
        if (S0 <= 0 || sum_var <= 0) {
            throw McpError(-32602, "Not enough valid observations.");
        }
        C = ((double)(valid - 1) / (2.0 * S0)) * sum_diff / sum_var;
        expected = 1.0;

        // Geary's C < 1 indicates positive spatial autocorrelation, C > 1
        // negative. Use a two-sided pseudo-p-value: count both tails and take
        // 2 * min(count_le, count_ge), clamped to [0, 1].
        int count_le = 0, count_ge = 0;
        Randik rnd;
        std::vector<int> perm(n);
        std::vector<long> rands(n);
        for (int p = 0; p < permutations; ++p) {
            rnd.Perm(undefs, n, &perm[0], &rands[0]);
            double sum_diff_p = 0.0;
            for (int i = 0; i < n; ++i) {
                if (undefs[i]) continue;
                const GalElement& ge = gw->gal[i];
                for (long j = 0; j < ge.Size(); ++j) {
                    long nb = ge[j];
                    if (nb >= 0 && nb < n && !undefs[nb]) {
                        double d = x[perm[i]] - x[perm[nb]];
                        sum_diff_p += d * d;
                    }
                }
            }
            double C_p = ((double)(valid - 1) / (2.0 * S0)) * sum_diff_p / sum_var;
            if (C_p <= C) ++count_le;
            if (C_p >= C) ++count_ge;
        }
        double p_le = (double)(count_le + 1) / (double)(permutations + 1);
        double p_ge = (double)(count_ge + 1) / (double)(permutations + 1);
        p_value = 2.0 * (p_le < p_ge ? p_le : p_ge);
        if (p_value > 1.0) p_value = 1.0;
    }

    // Compute the global Getis-Ord General G statistic:
    //   G = sum_ij w_ij x_i x_j / sum_ij x_i x_j   (i != j)
    // The denominator is (sum x)^2 - sum x^2 over valid observations.
    void GlobalGeneralG(const std::vector<double>& x,
                        const std::vector<bool>& undefs,
                        const GalWeight* gw, int permutations,
                        double& G, double& expected, double& p_value)
    {
        int n = (int)x.size();
        double sum_x = 0.0, sum_x2 = 0.0, S0 = 0.0, sum_xy = 0.0;
        int valid = 0;
        for (int i = 0; i < n; ++i) {
            if (undefs[i]) continue;
            ++valid;
            sum_x += x[i];
            sum_x2 += x[i] * x[i];
            const GalElement& ge = gw->gal[i];
            for (long j = 0; j < ge.Size(); ++j) {
                long nb = ge[j];
                if (nb >= 0 && nb < n && !undefs[nb]) {
                    S0 += 1.0;
                    sum_xy += x[i] * x[nb];
                }
            }
        }
        double denom = sum_x * sum_x - sum_x2;  // sum over i != j of x_i x_j
        if (valid < 2 || S0 <= 0 || denom <= 0) {
            throw McpError(-32602, "Not enough valid observations.");
        }
        G = sum_xy / denom;
        expected = S0 / ((double)valid * (valid - 1));

        int count = 0;
        Randik rnd;
        std::vector<int> perm(n);
        std::vector<long> rands(n);
        for (int p = 0; p < permutations; ++p) {
            rnd.Perm(undefs, n, &perm[0], &rands[0]);
            double sum_xy_p = 0.0;
            for (int i = 0; i < n; ++i) {
                if (undefs[i]) continue;
                const GalElement& ge = gw->gal[i];
                for (long j = 0; j < ge.Size(); ++j) {
                    long nb = ge[j];
                    if (nb >= 0 && nb < n && !undefs[nb])
                        sum_xy_p += x[perm[i]] * x[perm[nb]];
                }
            }
            double G_p = sum_xy_p / denom;
            if (G_p >= G) ++count;
        }
        p_value = (double)(count + 1) / (double)(permutations + 1);
    }

    // ---------------------------------------------------------------------
    // Clustering helpers
    // ---------------------------------------------------------------------
    // Compute the full distance matrix (row-wise) for the given data.
    double** ComputeDistances(double** data, int rows, int ncols,
                              char dist)
    {
        int** mask = new int*[rows];
        for (int i = 0; i < rows; ++i) {
            mask[i] = new int[ncols];
            for (int c = 0; c < ncols; ++c) mask[i][c] = 1;
        }
        double* weight = new double[ncols];
        for (int c = 0; c < ncols; ++c) weight[c] = 1.0;
        double** ragged = distancematrix(rows, ncols, data, mask, weight,
                                         dist, 0);
        double** full = DataUtils::fullRaggedMatrix(ragged, rows, rows, false);
        for (int i = 1; i < rows; ++i) free(ragged[i]);
        free(ragged);
        for (int i = 0; i < rows; ++i) delete[] mask[i];
        delete[] mask;
        delete[] weight;
        return full;
    }

    void FreeDataMatrix(double** data, int rows)
    {
        if (!data) return;
        for (int i = 0; i < rows; ++i) delete[] data[i];
        delete[] data;
    }

    void FreeDistMatrix(double** dist, int rows)
    {
        if (!dist) return;
        for (int i = 0; i < rows; ++i) delete[] dist[i];
        delete[] dist;
    }

    // Serialize a cluster assignment vector (0-based) as 1-based labels.
    json_spirit::Value ClusterResult(const std::vector<int>& clusters)
    {
        std::vector<json_spirit::Value> arr;
        arr.reserve(clusters.size());
        for (size_t i = 0; i < clusters.size(); ++i) {
            arr.push_back(json_spirit::Value(clusters[i] + 1));
        }
        return Arr(arr);
    }
}

// =========================================================================
// project
// =========================================================================
json_spirit::Value McpProjectStatus(const McpToolContext& ctx,
                                    const json_spirit::Object& params)
{
    if (!ctx.project) {
        std::vector<json_spirit::Pair> r;
        r.push_back(P("open", json_spirit::Value(false)));
        return Obj(r);
    }
    Project* project = ctx.project;
    std::vector<json_spirit::Pair> r;
    r.push_back(P("open", json_spirit::Value(true)));
    r.push_back(P("title",
        json_spirit::Value(project->GetProjectTitle().ToStdString())));
    wxString path;
    if (project->GetProjectConf()) {
        path = project->GetProjectConf()->GetFilePath();
    }
    r.push_back(P("path", json_spirit::Value(path.ToStdString())));
    r.push_back(P("num_records", json_spirit::Value(project->GetNumRecords())));
    TableInterface* table = project->GetTableInt();
    if (table) {
        r.push_back(P("num_columns", json_spirit::Value(table->GetNumberCols())));
    }
    return Obj(r);
}

// =========================================================================
// table
// =========================================================================
json_spirit::Value McpTableListColumns(const McpToolContext& ctx,
                                      const json_spirit::Object& params)
{
    TableInterface* table = RequireTable(ctx);
    int ncols = table->GetNumberCols();
    std::vector<json_spirit::Value> cols;
    for (int c = 0; c < ncols; ++c) {
        wxString name = table->GetColName(c);
        GdaConst::FieldType type = table->GetColType(c, 0);
        std::vector<json_spirit::Pair> col;
        col.push_back(P("name", json_spirit::Value(name.ToStdString())));
        col.push_back(P("type",
            json_spirit::Value(GdaConst::FieldTypeToStr(type).ToStdString())));
        col.push_back(P("time_variant",
            json_spirit::Value(table->IsColTimeVariant(c))));
        cols.push_back(Obj(col));
    }
    std::vector<json_spirit::Pair> r;
    r.push_back(P("columns", Arr(cols)));
    return Obj(r);
}

json_spirit::Value McpTableGetColumn(const McpToolContext& ctx,
                                    const json_spirit::Object& params)
{
    TableInterface* table = RequireTable(ctx);
    wxString name = GetStr(params, "column");
    if (name.IsEmpty()) {
        throw McpError(-32602, "Missing required parameter: column");
    }
    int col = RequireColumn(table, name);
    GdaConst::FieldType type = table->GetColType(col, 0);
    std::vector<json_spirit::Pair> r;
    r.push_back(P("name", json_spirit::Value(name.ToStdString())));
    r.push_back(P("type",
        json_spirit::Value(GdaConst::FieldTypeToStr(type).ToStdString())));
    r.push_back(P("num_rows", json_spirit::Value(table->GetNumberRows())));

    if (type == GdaConst::string_type) {
        std::vector<wxString> data;
        std::vector<bool> undefs;
        table->GetColData(col, 0, data, undefs);
        std::vector<json_spirit::Value> arr;
        for (size_t i = 0; i < data.size(); ++i) {
            if (undefs[i]) {
                arr.push_back(json_spirit::Value());
            } else {
                arr.push_back(json_spirit::Value(data[i].ToStdString()));
            }
        }
        r.push_back(P("data", Arr(arr)));
    } else if (type == GdaConst::long64_type) {
        std::vector<wxInt64> data;
        std::vector<bool> undefs;
        table->GetColData(col, 0, data, undefs);
        std::vector<json_spirit::Value> arr;
        for (size_t i = 0; i < data.size(); ++i) {
            if (undefs[i]) {
                arr.push_back(json_spirit::Value());
            } else {
                arr.push_back(json_spirit::Value((int)data[i]));
            }
        }
        r.push_back(P("data", Arr(arr)));
    } else {
        std::vector<double> data;
        std::vector<bool> undefs;
        table->GetColData(col, 0, data, undefs);
        std::vector<json_spirit::Value> arr;
        for (size_t i = 0; i < data.size(); ++i) {
            if (undefs[i]) {
                arr.push_back(json_spirit::Value());
            } else {
                arr.push_back(json_spirit::Value(data[i]));
            }
        }
        r.push_back(P("data", Arr(arr)));
    }
    return Obj(r);
}

json_spirit::Value McpTableUnivariateStats(const McpToolContext& ctx,
                                          const json_spirit::Object& params)
{
    TableInterface* table = RequireTable(ctx);
    wxString name = GetStr(params, "column");
    if (name.IsEmpty()) {
        throw McpError(-32602, "Missing required parameter: column");
    }
    int col = RequireColumn(table, name);
    GdaConst::FieldType type = table->GetColType(col, 0);
    if (type == GdaConst::string_type || type == GdaConst::date_type ||
        type == GdaConst::time_type || type == GdaConst::datetime_type) {
        throw McpError(-32602,
            "Column is not numeric: " + name.ToStdString());
    }

    std::vector<double> data;
    std::vector<bool> undefs;
    table->GetColData(col, 0, data, undefs);

    std::vector<double> vals;
    for (size_t i = 0; i < data.size(); ++i) {
        if (!undefs[i]) vals.push_back(data[i]);
    }
    int n = (int)vals.size();
    if (n == 0) {
        throw McpError(-32602,
            "Column has no valid observations: " + name.ToStdString());
    }

    std::sort(vals.begin(), vals.end());
    double sum = 0.0;
    for (int i = 0; i < n; ++i) sum += vals[i];
    double mean = sum / n;
    double median = (n % 2 == 1) ? vals[n / 2]
                                 : 0.5 * (vals[n / 2 - 1] + vals[n / 2]);
    double q1 = (n % 2 == 1) ? vals[n / 4]
                             : 0.5 * (vals[n / 4 - 1] + vals[n / 4]);
    double q3 = (n % 2 == 1) ? vals[3 * n / 4]
                             : 0.5 * (vals[3 * n / 4 - 1] + vals[3 * n / 4]);
    double variance = 0.0;
    for (int i = 0; i < n; ++i) {
        double d = vals[i] - mean;
        variance += d * d;
    }
    variance /= (n - 1);
    double std_dev = sqrt(variance);
    double skewness = 0.0, kurtosis = 0.0;
    if (std_dev > 0) {
        for (int i = 0; i < n; ++i) {
            double z = (vals[i] - mean) / std_dev;
            skewness += z * z * z;
            kurtosis += z * z * z * z;
        }
        skewness /= n;
        kurtosis = kurtosis / n - 3.0;
    }

    std::vector<json_spirit::Pair> r;
    r.push_back(P("name", json_spirit::Value(name.ToStdString())));
    r.push_back(P("count", json_spirit::Value(n)));
    r.push_back(P("missing", json_spirit::Value((int)undefs.size() - n)));
    r.push_back(P("mean", json_spirit::Value(mean)));
    r.push_back(P("median", json_spirit::Value(median)));
    r.push_back(P("std_dev", json_spirit::Value(std_dev)));
    r.push_back(P("variance", json_spirit::Value(variance)));
    r.push_back(P("min", json_spirit::Value(vals[0])));
    r.push_back(P("max", json_spirit::Value(vals[n - 1])));
    r.push_back(P("sum", json_spirit::Value(sum)));
    r.push_back(P("q1", json_spirit::Value(q1)));
    r.push_back(P("q3", json_spirit::Value(q3)));
    r.push_back(P("iqr", json_spirit::Value(q3 - q1)));
    r.push_back(P("skewness", json_spirit::Value(skewness)));
    r.push_back(P("kurtosis", json_spirit::Value(kurtosis)));
    return Obj(r);
}

// =========================================================================
// file/export (scripted data export)
// =========================================================================
// Headless counterpart of File->Export Data. Writes the open data set
// (geometry + table) to a file via OGR. This runs on a worker thread, so it
// must not touch wx GUI code: field names are validated here instead of
// popping the FieldNameCorrectionDlg, and OGR geometry conversion is driven
// directly (see ExportDataDlg::CreateOGRLayer for the GUI equivalent).
json_spirit::Value McpFileExport(const McpToolContext& ctx,
                                 const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    wxString out_path = GetStr(params, "path");
    if (out_path.IsEmpty()) {
        throw McpError(-32602, "Missing required parameter: path");
    }
    // Resolve the OGR format name from the explicit parameter or the file
    // extension (e.g. GeoJSON, ESRI Shapefile, GeoPackage, CSV).
    wxString format = GetStr(params, "format");
    if (format.IsEmpty()) {
        wxFileName fname(out_path);
        format = IDataSource::GetDataTypeNameByExt(fname.GetExt());
    }
    if (format.IsEmpty()) {
        throw McpError(-32602,
            "Cannot determine export format. Use a known file extension "
            "(e.g. .geojson, .gpkg, .shp, .csv) or pass format explicitly.");
    }
    GdaConst::DataSourceType ds_type = IDataSource::FindDataSourceType(format);
    if (ds_type == GdaConst::ds_unknown) {
        throw McpError(-32602,
            "Unsupported export format: " + format.ToStdString());
    }
    bool is_table = IDataSource::IsTableOnly(ds_type);

    // Export every observation (all rows, no selection).
    int num_obs = project->main_data.records.size();
    if (num_obs == 0) num_obs = project->GetNumRecords();
    if (num_obs == 0) {
        throw McpError(-32602, "Export failed: the current data set is empty.");
    }
    std::vector<int> selected_rows;
    for (int i = 0; i < num_obs; ++i) selected_rows.push_back(i);

    // Convert the in-memory geometry records to GdaShapes. Mirrors
    // ExportDataDlg::CreateOGRLayer; table-only data keeps an empty list.
    std::vector<GdaShape*> geometries;
    Shapefile::ShapeType shape_type = Shapefile::NULL_SHAPE;
    if (project->main_data.header.shape_type == Shapefile::POINT_TYP) {
        Shapefile::PointContents* pc;
        for (int i = 0; i < num_obs; ++i) {
            pc = (Shapefile::PointContents*)
                project->main_data.records[i].contents_p;
            if (pc->x == 0 && pc->y == 0 &&
                (pc->x < project->main_data.header.bbox_x_min ||
                 pc->x > project->main_data.header.bbox_x_max) &&
                (pc->y < project->main_data.header.bbox_y_min ||
                 pc->y > project->main_data.header.bbox_y_max)) {
                geometries.push_back(new GdaPoint());
            } else {
                geometries.push_back(
                    new GdaPoint(wxRealPoint(pc->x, pc->y)));
            }
        }
        shape_type = Shapefile::POINT_TYP;
    } else if (project->main_data.header.shape_type == Shapefile::POLYGON) {
        Shapefile::PolygonContents* pc;
        for (int i = 0; i < num_obs; ++i) {
            pc = (Shapefile::PolygonContents*)
                project->main_data.records[i].contents_p;
            geometries.push_back(new GdaPolygon(pc));
        }
        shape_type = Shapefile::POLYGON;
    }

    // Validate field names up front. The GUI export path fixes illegal or
    // duplicated names with a modal FieldNameCorrectionDlg; from a worker
    // thread that dialog cannot be shown, so reject with a clear error.
    wxString cpgEncoding = project->GetCpgEncode();
    std::vector<int> all_col_ids;
    for (int id = 0; id < table->GetNumberCols(); ++id) {
        all_col_ids.push_back(id);
    }
    ValidateExportFieldNames(table, all_col_ids, ds_type, format);

    // Convert GdaShapes to OGR geometries and reproject if a CRS was given.
    OGRDataAdapter& ogr_adapter = OGRDataAdapter::GetInstance();
    std::vector<OGRGeometry*> ogr_geometries;
    OGRwkbGeometryType geom_type = wkbNone;
    OGRSpatialReference* spatial_ref = project->GetSpatialReference();
    OGRSpatialReference new_ref;
    if (is_table) {
        spatial_ref = NULL;  // table-only output, avoid creating a .prj
    } else {
        geom_type = ogr_adapter.MakeOGRGeometries(geometries, shape_type,
                                                  ogr_geometries,
                                                  selected_rows);
        wxString str_crs = GetStr(params, "crs");
        bool valid_input_crs = false;
        if (!str_crs.IsEmpty()) {
            if (new_ref.importFromProj4(str_crs.c_str()) != OGRERR_NONE) {
                wxString msg = wxString::Format("Invalid crs value: %s",
                                                str_crs);
                throw McpError(-32602, msg.ToStdString());
            }
            valid_input_crs = true;
        }
        if (out_path.EndsWith(".json") || out_path.EndsWith(".geojson")) {
            // for GeoJSON, force transform to EPSG4326 automatically so the
            // output can be dropped into a web map (e.g. kepler.gl)
            new_ref.importFromEPSG(4326);
            valid_input_crs = true;
        }
        if (ogr_geometries.size() > 0 && valid_input_crs) {
            if (spatial_ref && spatial_ref->IsSame(&new_ref) == false) {
                OGRCoordinateTransformation *poCT;
#ifdef __PROJ6__
                spatial_ref->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
                new_ref.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
#endif
                poCT = OGRCreateCoordinateTransformation(spatial_ref, &new_ref);
                if (!poCT) {
                    // Happens when PROJ data (proj.db) is unavailable, e.g. a
                    // bare binary run outside its bundle. A NULL transform
                    // would segfault inside transform(), so fail cleanly.
                    throw McpError(-32000,
                        "Cannot create the coordinate transformation to "
                        "EPSG:4326 (PROJ database not available).");
                }
                for (size_t i = 0; i < ogr_geometries.size(); ++i) {
                    ogr_geometries[i]->transform(poCT);
                }
                OGRCoordinateTransformation::DestroyCT(poCT);
            }
            spatial_ref = &new_ref;
        }
    }

    wxString layer_name = GetStr(params, "layer_id");
    if (layer_name.IsEmpty()) layer_name = table->GetTableName();
    if (layer_name.IsEmpty()) layer_name = "geoda_export";

    OGRLayerProxy* new_layer = NULL;
    try {
        new_layer = ogr_adapter.ExportDataSource(
            format, out_path, layer_name, geom_type, ogr_geometries, table,
            selected_rows, spatial_ref, false, cpgEncoding,
            true /* skip_field_name_dialog: names validated above */);
    } catch (GdaException& e) {
        for (size_t i = 0; i < ogr_geometries.size(); ++i) {
            OGRGeometryFactory::destroyGeometry(ogr_geometries[i]);
        }
        for (size_t i = 0; i < geometries.size(); ++i) delete geometries[i];
        throw McpError(-32000,
                       std::string("Export failed: ") + e.what());
    }
    if (new_layer == NULL) {
        for (size_t i = 0; i < ogr_geometries.size(); ++i) {
            OGRGeometryFactory::destroyGeometry(ogr_geometries[i]);
        }
        for (size_t i = 0; i < geometries.size(); ++i) delete geometries[i];
        throw McpError(-32602, "Export failed: unable to create the data source.");
    }

    // Wait for the background AddFeatures thread (ExportDataDlg::CreateOGRLayer).
    int prog_n_max = selected_rows.size();
    while (new_layer->export_progress < prog_n_max) {
        wxMilliSleep(100);
        if (new_layer->stop_exporting) {
            throw McpError(-32000, "Export cancelled.");
        }
        if (new_layer->export_progress == -1) {
            wxString msg = wxString::Format("Export to %s failed.\n\n%s",
                                            out_path, new_layer->error_message);
            throw McpError(-32000, msg.ToStdString());
        }
    }
    ogr_adapter.StopExport();  // deletes new_layer and its OGR geometries
    for (size_t i = 0; i < geometries.size(); ++i) delete geometries[i];

    std::vector<json_spirit::Pair> r;
    r.push_back(P("success", json_spirit::Value(true)));
    r.push_back(P("path", json_spirit::Value(out_path.ToStdString())));
    r.push_back(P("format", json_spirit::Value(format.ToStdString())));
    r.push_back(P("num_features", json_spirit::Value((int)selected_rows.size())));
    return Obj(r);
}

// =========================================================================
// table/export (scripted column-subset export)
// =========================================================================
// Export a selection of columns (plus geometry when the data is spatial) to
// a file via OGR. Unlike file/export this writes only the requested columns,
// so e.g. an ID, a LISA cluster label and centroid coordinates can be saved
// without dragging along every column of the table. Runs on the worker
// thread: no wx GUI objects, field names are pre-validated (see
// ValidateExportFieldNames). The OGR datasource is built by hand so that the
// layer contains exactly the selected fields (ExportDataSource writes the
// whole table).
json_spirit::Value McpTableExport(const McpToolContext& ctx,
                                  const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = RequireTable(ctx);
    wxString out_path = GetStr(params, "path");
    if (out_path.IsEmpty()) {
        throw McpError(-32602, "Missing required parameter: path");
    }
    // Resolve the OGR format name from the explicit parameter or the file
    // extension.
    wxString format = GetStr(params, "format");
    if (format.IsEmpty()) {
        wxFileName fname(out_path);
        format = IDataSource::GetDataTypeNameByExt(fname.GetExt());
    }
    if (format.IsEmpty()) {
        throw McpError(-32602,
            "Cannot determine export format. Use a known file extension "
            "(e.g. .geojson, .gpkg, .shp, .csv) or pass format explicitly.");
    }
    GdaConst::DataSourceType ds_type = IDataSource::FindDataSourceType(format);
    if (ds_type == GdaConst::ds_unknown) {
        throw McpError(-32602,
            "Unsupported export format: " + format.ToStdString());
    }
    bool is_table = IDataSource::IsTableOnly(ds_type);
    bool include_geometry = GetBool(params, "include_geometry", true);

    // Select the columns to export: an explicit list, or all columns.
    std::vector<wxString> requested = GetStrArray(params, "columns");
    std::vector<int> col_ids;
    if (requested.empty()) {
        for (int id = 0; id < table->GetNumberCols(); ++id) {
            col_ids.push_back(id);
        }
    } else {
        for (size_t i = 0; i < requested.size(); ++i) {
            col_ids.push_back(RequireColumn(table, requested[i]));
        }
    }
    if (col_ids.empty()) {
        throw McpError(-32602, "Export failed: no columns to export.");
    }
    ValidateExportFieldNames(table, col_ids, ds_type, format);

    // Export every observation (all rows, no selection).
    int num_obs = project->main_data.records.size();
    if (num_obs == 0) num_obs = project->GetNumRecords();
    if (num_obs == 0) {
        throw McpError(-32602, "Export failed: the current data set is empty.");
    }
    std::vector<int> selected_rows;
    for (int i = 0; i < num_obs; ++i) selected_rows.push_back(i);

    // Convert the in-memory geometry records to GdaShapes. Mirrors
    // ExportDataDlg::CreateOGRLayer; table-only data keeps an empty list.
    std::vector<GdaShape*> geometries;
    Shapefile::ShapeType shape_type = Shapefile::NULL_SHAPE;
    if (!is_table && include_geometry) {
        if (project->main_data.header.shape_type == Shapefile::POINT_TYP) {
            Shapefile::PointContents* pc;
            for (int i = 0; i < num_obs; ++i) {
                pc = (Shapefile::PointContents*)
                    project->main_data.records[i].contents_p;
                if (pc->x == 0 && pc->y == 0 &&
                    (pc->x < project->main_data.header.bbox_x_min ||
                     pc->x > project->main_data.header.bbox_x_max) &&
                    (pc->y < project->main_data.header.bbox_y_min ||
                     pc->y > project->main_data.header.bbox_y_max)) {
                    geometries.push_back(new GdaPoint());
                } else {
                    geometries.push_back(
                        new GdaPoint(wxRealPoint(pc->x, pc->y)));
                }
            }
            shape_type = Shapefile::POINT_TYP;
        } else if (project->main_data.header.shape_type ==
                   Shapefile::POLYGON) {
            Shapefile::PolygonContents* pc;
            for (int i = 0; i < num_obs; ++i) {
                pc = (Shapefile::PolygonContents*)
                    project->main_data.records[i].contents_p;
                geometries.push_back(new GdaPolygon(pc));
            }
            shape_type = Shapefile::POLYGON;
        }
    }

    // Convert GdaShapes to OGR geometries and reproject if a CRS was given.
    OGRDataAdapter& ogr_adapter = OGRDataAdapter::GetInstance();
    std::vector<OGRGeometry*> ogr_geometries;
    OGRwkbGeometryType geom_type = wkbNone;
    OGRSpatialReference* spatial_ref = NULL;
    OGRSpatialReference new_ref;
    if (!is_table && include_geometry) {
        geom_type = ogr_adapter.MakeOGRGeometries(geometries, shape_type,
                                                  ogr_geometries,
                                                  selected_rows);
        spatial_ref = project->GetSpatialReference();
        wxString str_crs = GetStr(params, "crs");
        bool valid_input_crs = false;
        if (!str_crs.IsEmpty()) {
            if (new_ref.importFromProj4(str_crs.c_str()) != OGRERR_NONE) {
                wxString msg = wxString::Format("Invalid crs value: %s",
                                                str_crs);
                throw McpError(-32602, msg.ToStdString());
            }
            valid_input_crs = true;
        }
        if (out_path.EndsWith(".json") || out_path.EndsWith(".geojson")) {
            // for GeoJSON, force transform to EPSG4326 automatically so the
            // output can be dropped into a web map (e.g. kepler.gl)
            new_ref.importFromEPSG(4326);
            valid_input_crs = true;
        }
        if (ogr_geometries.size() > 0 && valid_input_crs) {
            if (spatial_ref && spatial_ref->IsSame(&new_ref) == false) {
                OGRCoordinateTransformation *poCT;
#ifdef __PROJ6__
                spatial_ref->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
                new_ref.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
#endif
                poCT = OGRCreateCoordinateTransformation(spatial_ref, &new_ref);
                if (!poCT) {
                    // Happens when PROJ data (proj.db) is unavailable, e.g. a
                    // bare binary run outside its bundle. A NULL transform
                    // would segfault inside transform(), so fail cleanly.
                    throw McpError(-32000,
                        "Cannot create the coordinate transformation to "
                        "EPSG:4326 (PROJ database not available).");
                }
                for (size_t i = 0; i < ogr_geometries.size(); ++i) {
                    ogr_geometries[i]->transform(poCT);
                }
                OGRCoordinateTransformation::DestroyCT(poCT);
            }
            spatial_ref = &new_ref;
        }
    }

    // Read the selected column values once (time step 0). Mirrors the typed
    // reads in OGRLayerProxy::AddFeatures.
    std::vector<std::vector<double> > d_vals(col_ids.size());
    std::vector<std::vector<wxInt64> > l_vals(col_ids.size());
    std::vector<std::vector<wxString> > s_vals(col_ids.size());
    std::vector<std::vector<unsigned long long> > t_vals(col_ids.size());
    std::vector<std::vector<bool> > col_undefs(col_ids.size());
    for (size_t c = 0; c < col_ids.size(); ++c) {
        GdaConst::FieldType ftype = table->GetColType(col_ids[c], 0);
        if (ftype == GdaConst::double_type) {
            table->GetColData(col_ids[c], 0, d_vals[c], col_undefs[c]);
        } else if (ftype == GdaConst::long64_type) {
            table->GetColData(col_ids[c], 0, l_vals[c], col_undefs[c]);
        } else if (ftype == GdaConst::date_type ||
                   ftype == GdaConst::time_type ||
                   ftype == GdaConst::datetime_type) {
            table->GetColData(col_ids[c], 0, t_vals[c], col_undefs[c]);
        } else {
            // others are treated as string_type
            table->GetColData(col_ids[c], 0, s_vals[c], col_undefs[c]);
            if (ds_type == GdaConst::ds_csv) {
                for (size_t m = 0; m < s_vals[c].size(); ++m) {
                    col_undefs[c][m] = false;  // no undefs in csv file
                    if (s_vals[c][m].IsEmpty()) s_vals[c][m] = " ";
                }
            }
        }
    }

    // OGR drivers generally refuse to overwrite an existing file, so remove
    // the target first (a missing file is fine). Shapefile keeps a set of
    // sidecar files that must go too.
    wxRemoveFile(out_path);
    if (ds_type == GdaConst::ds_shapefile) {
        wxString base = out_path.BeforeLast('.');
        wxArrayString exts;
        exts.Add("shp"); exts.Add("shx"); exts.Add("dbf"); exts.Add("prj");
        exts.Add("cpg"); exts.Add("sbn"); exts.Add("sbx"); exts.Add("qix");
        for (size_t i = 0; i < exts.size(); ++i) {
            wxString fn;
            fn << base << "." << exts[i];
            if (fn != out_path) wxRemoveFile(fn);
        }
    }

    wxString layer_name = GetStr(params, "layer_id");
    if (layer_name.IsEmpty()) layer_name = table->GetTableName();
    if (layer_name.IsEmpty()) layer_name = "geoda_export";

    GDALDataset* ds = NULL;
    try {
        GDALDriver* driver = GetGDALDriverManager()->GetDriverByName(
            format.ToStdString().c_str());
        if (!driver) {
            throw McpError(-32000,
                "Export failed: no OGR driver for format: " +
                format.ToStdString());
        }
        ds = driver->Create(out_path.ToStdString().c_str(),
                            0, 0, 0, GDT_Unknown, NULL);
        if (!ds) {
            wxString msg = wxString::Format(
                "Export failed: cannot create %s.\n\n%s",
                out_path, CPLGetLastErrorMsg());
            throw McpError(-32000, msg.ToStdString());
        }

        char** papszLCO = NULL;
        papszLCO = CSLAddString(papszLCO, "OVERWRITE=yes");
        papszLCO = CSLAddString(papszLCO, "LAUNDER=no");
        if (ds_type == GdaConst::ds_csv && GdaConst::gda_create_csvt) {
            papszLCO = CSLAddString(papszLCO, "CREATE_CSVT=YES");
        }
        OGRLayer* ogr_layer = ds->CreateLayer(layer_name.mb_str(), spatial_ref,
                                              geom_type, papszLCO);
        CSLDestroy(papszLCO);
        if (!ogr_layer) {
            wxString msg = wxString::Format(
                "Export failed: cannot create layer \"%s\".\n\n%s",
                layer_name, CPLGetLastErrorMsg());
            throw McpError(-32000, msg.ToStdString());
        }

        // Create one field per selected column, mirroring
        // OGRDatasourceProxy::CreateLayer's field type mapping.
        OGRFeatureDefn* feat_def = ogr_layer->GetLayerDefn();
        std::vector<OGRFieldType> field_types;
        for (size_t c = 0; c < col_ids.size(); ++c) {
            int id = col_ids[c];
            wxString fname = table->GetColName(id);
            GdaConst::FieldType ftype = table->GetColType(id, 0);
            OGRFieldType oft = OFTString;
            if (ftype == GdaConst::long64_type) {
                oft = OFTInteger64;
            } else if (ftype == GdaConst::double_type) {
                oft = OFTReal;
            } else if (ftype == GdaConst::date_type) {
                oft = OFTDate;
            } else if (ftype == GdaConst::time_type) {
                oft = OFTTime;
            } else if (ftype == GdaConst::datetime_type) {
                oft = OFTDateTime;
            }
            OGRFieldDefn oField(fname.utf8_str(), oft);
            oField.SetWidth(table->GetColLength(id, 0));
            int dec = table->GetColDecimals(id, 0);
            if (dec > 0) oField.SetPrecision(dec);
            if (ogr_layer->CreateField(&oField, false) != OGRERR_NONE) {
                wxString msg = wxString::Format(
                    "Export failed: cannot create field \"%s\".\n\n%s",
                    fname, CPLGetLastErrorMsg());
                throw McpError(-32000, msg.ToStdString());
            }
            field_types.push_back(oft);
        }

        // Write the features. SetGeometry copies, so ogr_geometries stay
        // owned by this function and are destroyed after the datasource
        // closes.
        for (int row = 0; row < num_obs; ++row) {
            OGRFeature* feat = OGRFeature::CreateFeature(feat_def);
            if (include_geometry && row < (int)ogr_geometries.size() &&
                ogr_geometries[row] != NULL) {
                feat->SetGeometry(ogr_geometries[row]);
            }
            for (size_t c = 0; c < col_ids.size(); ++c) {
                if (row < (int)col_undefs[c].size() && col_undefs[c][row]) {
                    feat->SetFieldNull((int)c);
                    continue;
                }
                GdaConst::FieldType ftype = table->GetColType(col_ids[c], 0);
                if (ftype == GdaConst::double_type &&
                    row < (int)d_vals[c].size()) {
                    feat->SetField((int)c, d_vals[c][row]);
                } else if (ftype == GdaConst::long64_type &&
                           row < (int)l_vals[c].size()) {
                    feat->SetField((int)c, (GIntBig)l_vals[c][row]);
                } else if ((ftype == GdaConst::date_type ||
                            ftype == GdaConst::time_type ||
                            ftype == GdaConst::datetime_type) &&
                           row < (int)t_vals[c].size()) {
                    unsigned long long v = t_vals[c][row];
                    int year = v / 10000000000;
                    int month = (v % 10000000000) / 100000000;
                    int day = (v % 100000000) / 1000000;
                    int hour = (v % 1000000) / 10000;
                    int minute = (v % 10000) / 100;
                    int second = v % 100;
                    feat->SetField((int)c, year, month, day, hour, minute,
                                   second);
                } else if (row < (int)s_vals[c].size()) {
                    feat->SetField((int)c, s_vals[c][row].c_str());
                }
            }
            if (ogr_layer->CreateFeature(feat) != OGRERR_NONE) {
                OGRFeature::DestroyFeature(feat);
                wxString msg = wxString::Format(
                    "Export failed at feature %d.\n\n%s",
                    row, CPLGetLastErrorMsg());
                throw McpError(-32000, msg.ToStdString());
            }
            OGRFeature::DestroyFeature(feat);
        }
    } catch (McpError&) {
        if (ds) GDALClose(ds);
        for (size_t i = 0; i < ogr_geometries.size(); ++i) {
            OGRGeometryFactory::destroyGeometry(ogr_geometries[i]);
        }
        for (size_t i = 0; i < geometries.size(); ++i) delete geometries[i];
        throw;
    }
    GDALClose(ds);
    for (size_t i = 0; i < ogr_geometries.size(); ++i) {
        OGRGeometryFactory::destroyGeometry(ogr_geometries[i]);
    }
    for (size_t i = 0; i < geometries.size(); ++i) delete geometries[i];

    json_spirit::Array cols_out;
    for (size_t c = 0; c < col_ids.size(); ++c) {
        cols_out.push_back(json_spirit::Value(
            table->GetColName(col_ids[c]).ToStdString()));
    }
    std::vector<json_spirit::Pair> r;
    r.push_back(P("success", json_spirit::Value(true)));
    r.push_back(P("path", json_spirit::Value(out_path.ToStdString())));
    r.push_back(P("format", json_spirit::Value(format.ToStdString())));
    r.push_back(P("num_features", json_spirit::Value((int)selected_rows.size())));
    r.push_back(P("num_columns", json_spirit::Value((int)col_ids.size())));
    r.push_back(P("columns", json_spirit::Value(cols_out)));
    return Obj(r);
}

// =========================================================================
// weights
// =========================================================================
json_spirit::Value McpWeightsCreate(const McpToolContext& ctx,
                                    const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    int num_obs = project->GetNumRecords();

    wxString type = GetStr(params, "type", "queen");
    int k = GetInt(params, "k", 6);
    double distance_threshold = GetDouble(params, "distance_threshold", 0.0);
    wxString kernel = GetStr(params, "kernel", "triangular");
    int order = GetInt(params, "order", 1);
    bool is_arc = GetBool(params, "is_arc", false);
    bool is_mile = GetBool(params, "is_mile", false);

    GalWeight* gal_w = NULL;
    GwtWeight* gwt_w = NULL;
    WeightsMetaInfo::WeightTypeEnum wtype = WeightsMetaInfo::WT_custom;

    if (type == "queen" || type == "rook") {
        bool is_queen = (type == "queen");
        GalElement* gal = PolysToContigWeights(project->main_data, is_queen);
        if (!gal) {
            throw McpError(-32602,
                "Failed to build contiguity weights. The data may not be "
                "polygon-based.");
        }
        gal_w = new GalWeight();
        gal_w->num_obs = num_obs;
        gal_w->gal = gal;
        gal_w->is_symmetric = true;
        wtype = is_queen ? WeightsMetaInfo::WT_queen
                         : WeightsMetaInfo::WT_rook;
    } else if (type == "knn") {
        std::vector<double> x, y;
        project->GetCentroids(x, y);
        if (x.size() != (size_t)num_obs) {
            throw McpError(-32602,
                "Centroids are not available for this data set.");
        }
        gwt_w = SpatialIndAlgs::knn_build(x, y, k, is_arc, is_mile);
        if (!gwt_w) {
            throw McpError(-32602, "Failed to build k-nearest weights.");
        }
        gal_w = GwtToGal(gwt_w, num_obs);
        wtype = WeightsMetaInfo::WT_knn;
    } else if (type == "distance" || type == "threshold") {
        if (distance_threshold <= 0) {
            throw McpError(-32602,
                "distance_threshold must be positive for distance weights.");
        }
        std::vector<double> x, y;
        project->GetCentroids(x, y);
        if (x.size() != (size_t)num_obs) {
            throw McpError(-32602,
                "Centroids are not available for this data set.");
        }
        gwt_w = SpatialIndAlgs::thresh_build(x, y, distance_threshold, 1.0,
                                             is_arc, is_mile);
        if (!gwt_w) {
            throw McpError(-32602, "Failed to build distance weights.");
        }
        gal_w = GwtToGal(gwt_w, num_obs);
        wtype = WeightsMetaInfo::WT_threshold;
    } else if (type == "kernel") {
        std::vector<double> x, y;
        project->GetCentroids(x, y);
        if (x.size() != (size_t)num_obs) {
            throw McpError(-32602,
                "Centroids are not available for this data set.");
        }
        double bandwidth = GetDouble(params, "bandwidth", 0.0);
        bool adaptive = GetBool(params, "adaptive_bandwidth", false);
        gwt_w = SpatialIndAlgs::knn_build(x, y, k, is_arc, is_mile, true, 1.0,
                                         kernel, bandwidth, adaptive);
        if (!gwt_w) {
            throw McpError(-32602, "Failed to build kernel weights.");
        }
        gal_w = GwtToGal(gwt_w, num_obs);
        wtype = WeightsMetaInfo::WT_kernel;
    } else {
        throw McpError(-32602,
            "Unknown weights type: " + type.ToStdString() +
            " (expected queen, rook, knn, distance, or kernel)");
    }

    if (gwt_w) {
        delete gwt_w;
        gwt_w = NULL;
    }

    boost::uuids::uuid uid = RegisterGalWeight(project, gal_w, wtype, "");
    if (uid.is_nil()) {
        delete gal_w;
        throw McpError(-32602, "Failed to register weights with the manager.");
    }

    std::vector<json_spirit::Pair> r;
    r.push_back(P("weights",
        json_spirit::Value(boost::uuids::to_string(uid))));
    r.push_back(P("type", json_spirit::Value(type.ToStdString())));
    r.push_back(P("num_obs", json_spirit::Value(num_obs)));
    r.push_back(P("min_nbrs", json_spirit::Value(gal_w->GetMinNumNbrs())));
    r.push_back(P("max_nbrs", json_spirit::Value(gal_w->GetMaxNumNbrs())));
    r.push_back(P("mean_nbrs", json_spirit::Value(gal_w->GetMeanNumNbrs())));
    r.push_back(P("median_nbrs", json_spirit::Value(gal_w->GetMedianNumNbrs())));
    r.push_back(P("sparsity", json_spirit::Value(gal_w->GetSparsity())));
    r.push_back(P("density", json_spirit::Value(gal_w->GetDensity())));
    return Obj(r);
}

json_spirit::Value McpWeightsList(const McpToolContext& ctx,
                                  const json_spirit::Object& params)
{
    WeightsManInterface* wmi = RequireWeightsMan(ctx);
    std::list<boost::uuids::uuid> ids = wmi->GetIds();
    std::vector<json_spirit::Value> arr;
    for (std::list<boost::uuids::uuid>::iterator it = ids.begin();
         it != ids.end(); ++it) {
        WeightsMetaInfo info = wmi->GetMetaInfo(*it);
        std::vector<json_spirit::Pair> w;
        w.push_back(P("weights",
            json_spirit::Value(boost::uuids::to_string(*it))));
        w.push_back(P("type",
            json_spirit::Value(info.TypeToStr().ToStdString())));
        w.push_back(P("num_obs", json_spirit::Value(info.num_obs)));
        w.push_back(P("min_nbrs", json_spirit::Value(info.min_nbrs)));
        w.push_back(P("max_nbrs", json_spirit::Value(info.max_nbrs)));
        w.push_back(P("mean_nbrs", json_spirit::Value(info.mean_nbrs)));
        w.push_back(P("median_nbrs", json_spirit::Value(info.median_nbrs)));
        w.push_back(P("sparsity", json_spirit::Value(info.sparsity_val)));
        w.push_back(P("density", json_spirit::Value(info.density_val)));
        arr.push_back(Obj(w));
    }
    std::vector<json_spirit::Pair> r;
    r.push_back(P("weights", Arr(arr)));
    return Obj(r);
}

json_spirit::Value McpWeightsDescribe(const McpToolContext& ctx,
                                     const json_spirit::Object& params)
{
    wxString id = GetStr(params, "weights");
    if (id.IsEmpty()) {
        throw McpError(-32602, "Missing required parameter: weights");
    }
    WeightsManInterface* wmi = RequireWeightsMan(ctx);
    boost::uuids::uuid uid = RequireWeights(ctx, id);
    WeightsMetaInfo info = wmi->GetMetaInfo(uid);
    std::vector<json_spirit::Pair> r;
    r.push_back(P("weights", json_spirit::Value(id.ToStdString())));
    r.push_back(P("type", json_spirit::Value(info.TypeToStr().ToStdString())));
    r.push_back(P("num_obs", json_spirit::Value(info.num_obs)));
    r.push_back(P("min_nbrs", json_spirit::Value(info.min_nbrs)));
    r.push_back(P("max_nbrs", json_spirit::Value(info.max_nbrs)));
    r.push_back(P("mean_nbrs", json_spirit::Value(info.mean_nbrs)));
    r.push_back(P("median_nbrs", json_spirit::Value(info.median_nbrs)));
    r.push_back(P("sparsity", json_spirit::Value(info.sparsity_val)));
    r.push_back(P("density", json_spirit::Value(info.density_val)));
    r.push_back(P("symmetric", json_spirit::Value(
        info.sym_type == WeightsMetaInfo::SYM_symmetric)));
    return Obj(r);
}

// =========================================================================
// lisa
// =========================================================================
json_spirit::Value McpLisaLocalMoran(const McpToolContext& ctx,
                                     const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    wxString column = GetStr(params, "column");
    wxString weights = GetStr(params, "weights");
    if (column.IsEmpty()) column = AskForColumn(ctx, table);
    if (column.IsEmpty() || weights.IsEmpty()) {
        throw McpError(-32602,
            "Missing required parameters: column, weights");
    }
    int permutations = GetInt(params, "permutations", 999);
    double cutoff = GetDouble(params, "significance_cutoff", 0.05);
    bool row_standardize = GetBool(params, "row_standardize", true);
    wxString lisa_type_s = GetStr(params, "lisa_type", "univariate");
    wxString second_column = GetStr(params, "second_column");

    boost::uuids::uuid uid = RequireWeights(ctx, weights);

    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    var_info.push_back(BuildVarInfo(table, column));
    col_ids.push_back(RequireColumn(table, column));
    if (lisa_type_s == "bivariate" || lisa_type_s == "differential") {
        if (second_column.IsEmpty()) {
            throw McpError(-32602,
                "second_column is required for bivariate/differential LISA");
        }
        var_info.push_back(BuildVarInfo(table, second_column));
        col_ids.push_back(RequireColumn(table, second_column));
    }
    GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);

    LisaCoordinator::LisaType lisa_type = LisaCoordinator::univariate;
    if (lisa_type_s == "bivariate") lisa_type = LisaCoordinator::bivariate;
    else if (lisa_type_s == "eb_rate_standardized")
        lisa_type = LisaCoordinator::eb_rate_standardized;
    else if (lisa_type_s == "differential")
        lisa_type = LisaCoordinator::differential;

    LisaCoordinator coord(uid, project, var_info, col_ids, lisa_type, true,
                          row_standardize, false);
    coord.SetNumPermutations(permutations);
    coord.SetSignificanceCutoff(cutoff);
    coord.CalcPseudoP();

    int n = project->GetNumRecords();
    std::vector<json_spirit::Value> moran, lags, sig, clusters;
    double* moran_p = coord.local_moran_vecs[0];
    double* lags_p = coord.lags_vecs[0];
    double* sig_p = coord.GetLocalSignificanceValues(0);
    int* clus_p = coord.GetClusterIndicators(0);
    for (int i = 0; i < n; ++i) {
        moran.push_back(json_spirit::Value(moran_p[i]));
        lags.push_back(json_spirit::Value(lags_p[i]));
        sig.push_back(json_spirit::Value(sig_p[i]));
        clusters.push_back(json_spirit::Value(clus_p[i]));
    }
    std::vector<json_spirit::Pair> r;
    r.push_back(P("local_moran", Arr(moran)));
    r.push_back(P("lags", Arr(lags)));
    r.push_back(P("significance", Arr(sig)));
    r.push_back(P("cluster", Arr(clusters)));
    r.push_back(P("permutations", json_spirit::Value(permutations)));
    r.push_back(P("significance_cutoff", json_spirit::Value(cutoff)));
    return Obj(r);
}

json_spirit::Value McpLisaLocalGeary(const McpToolContext& ctx,
                                     const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    wxString column = GetStr(params, "column");
    wxString weights = GetStr(params, "weights");
    if (column.IsEmpty()) column = AskForColumn(ctx, table);
    if (column.IsEmpty() || weights.IsEmpty()) {
        throw McpError(-32602,
            "Missing required parameters: column, weights");
    }
    int permutations = GetInt(params, "permutations", 999);
    double cutoff = GetDouble(params, "significance_cutoff", 0.05);
    bool row_standardize = GetBool(params, "row_standardize", true);
    wxString lisa_type_s = GetStr(params, "lisa_type", "univariate");
    wxString second_column = GetStr(params, "second_column");

    boost::uuids::uuid uid = RequireWeights(ctx, weights);

    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    var_info.push_back(BuildVarInfo(table, column));
    col_ids.push_back(RequireColumn(table, column));
    if (lisa_type_s == "bivariate" || lisa_type_s == "differential") {
        if (second_column.IsEmpty()) {
            throw McpError(-32602,
                "second_column is required for bivariate/differential LISA");
        }
        var_info.push_back(BuildVarInfo(table, second_column));
        col_ids.push_back(RequireColumn(table, second_column));
    }
    GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);

    LocalGearyCoordinator::LocalGearyType ltype =
        LocalGearyCoordinator::univariate;
    if (lisa_type_s == "bivariate") ltype = LocalGearyCoordinator::bivariate;
    else if (lisa_type_s == "eb_rate_standardized")
        ltype = LocalGearyCoordinator::eb_rate_standardized;
    else if (lisa_type_s == "differential")
        ltype = LocalGearyCoordinator::differential;
    else if (lisa_type_s == "multivariate")
        ltype = LocalGearyCoordinator::multivariate;

    LocalGearyCoordinator coord(uid, project, var_info, col_ids, ltype, true,
                               row_standardize);
    coord.permutations = permutations;
    coord.significance_cutoff = cutoff;
    coord.CalcPseudoP();

    int n = project->GetNumRecords();
    std::vector<json_spirit::Value> geary, lags, sig, clusters;
    double* geary_p = coord.local_geary_vecs[0];
    double* lags_p = coord.lags_vecs[0];
    double* sig_p = coord.sig_local_geary_vecs[0];
    int* clus_p = coord.cluster_vecs[0];
    for (int i = 0; i < n; ++i) {
        geary.push_back(json_spirit::Value(geary_p[i]));
        lags.push_back(json_spirit::Value(lags_p[i]));
        sig.push_back(json_spirit::Value(sig_p[i]));
        clusters.push_back(json_spirit::Value(clus_p[i]));
    }
    std::vector<json_spirit::Pair> r;
    r.push_back(P("local_geary", Arr(geary)));
    r.push_back(P("lags", Arr(lags)));
    r.push_back(P("significance", Arr(sig)));
    r.push_back(P("cluster", Arr(clusters)));
    r.push_back(P("permutations", json_spirit::Value(permutations)));
    r.push_back(P("significance_cutoff", json_spirit::Value(cutoff)));
    return Obj(r);
}

json_spirit::Value McpLisaLocalG(const McpToolContext& ctx,
                                 const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    wxString column = GetStr(params, "column");
    wxString weights = GetStr(params, "weights");
    if (column.IsEmpty()) column = AskForColumn(ctx, table);
    if (column.IsEmpty() || weights.IsEmpty()) {
        throw McpError(-32602,
            "Missing required parameters: column, weights");
    }
    int permutations = GetInt(params, "permutations", 999);
    double cutoff = GetDouble(params, "significance_cutoff", 0.05);
    bool row_standardize = GetBool(params, "row_standardize", true);
    bool gstar = GetBool(params, "gstar", false);

    boost::uuids::uuid uid = RequireWeights(ctx, weights);

    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    var_info.push_back(BuildVarInfo(table, column));
    col_ids.push_back(RequireColumn(table, column));
    GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);

    GStatCoordinator coord(uid, project, var_info, col_ids, row_standardize);
    coord.permutations = permutations;
    coord.significance_cutoff = cutoff;
    coord.CalcPseudoP();

    int n = project->GetNumRecords();
    std::vector<json_spirit::Value> g_vals, z_vals, p_vals, clusters;
    double* g_p = gstar ? coord.G_star_vecs[0] : coord.G_vecs[0];
    double* z_p = gstar ? coord.z_star_vecs[0] : coord.z_vecs[0];
    double* p_p = gstar ? coord.pseudo_p_star_vecs[0]
                        : coord.pseudo_p_vecs[0];
    std::vector<wxInt64> c_val;
    coord.FillClusterCats(0, !gstar, true, c_val);
    for (int i = 0; i < n; ++i) {
        g_vals.push_back(json_spirit::Value(g_p[i]));
        z_vals.push_back(json_spirit::Value(z_p[i]));
        p_vals.push_back(json_spirit::Value(p_p[i]));
        clusters.push_back(json_spirit::Value((int)c_val[i]));
    }
    std::vector<json_spirit::Pair> r;
    r.push_back(P("g", Arr(g_vals)));
    r.push_back(P("z", Arr(z_vals)));
    r.push_back(P("pseudo_p", Arr(p_vals)));
    r.push_back(P("cluster", Arr(clusters)));
    r.push_back(P("gstar", json_spirit::Value(gstar)));
    r.push_back(P("permutations", json_spirit::Value(permutations)));
    r.push_back(P("significance_cutoff", json_spirit::Value(cutoff)));
    return Obj(r);
}

// =========================================================================
// global
// =========================================================================
json_spirit::Value McpGlobalMoran(const McpToolContext& ctx,
                                  const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    wxString column = GetStr(params, "column");
    wxString weights = GetStr(params, "weights");
    if (column.IsEmpty()) column = AskForColumn(ctx, table);
    if (column.IsEmpty() || weights.IsEmpty()) {
        throw McpError(-32602,
            "Missing required parameters: column, weights");
    }
    int permutations = GetInt(params, "permutations", 999);
    GalWeight* gw = GetGalWeight(ctx, weights);

    std::vector<double> x;
    std::vector<bool> undefs;
    GetNumericColumn(table, column, x, undefs);

    double I, expected, p_value;
    GlobalMoran(x, undefs, gw, permutations, I, expected, p_value);

    std::vector<json_spirit::Pair> r;
    r.push_back(P("moran_i", json_spirit::Value(I)));
    r.push_back(P("expected", json_spirit::Value(expected)));
    r.push_back(P("p_value", json_spirit::Value(p_value)));
    r.push_back(P("permutations", json_spirit::Value(permutations)));
    return Obj(r);
}

json_spirit::Value McpGlobalGeary(const McpToolContext& ctx,
                                  const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    wxString column = GetStr(params, "column");
    wxString weights = GetStr(params, "weights");
    if (column.IsEmpty()) column = AskForColumn(ctx, table);
    if (column.IsEmpty() || weights.IsEmpty()) {
        throw McpError(-32602,
            "Missing required parameters: column, weights");
    }
    int permutations = GetInt(params, "permutations", 999);
    GalWeight* gw = GetGalWeight(ctx, weights);

    std::vector<double> x;
    std::vector<bool> undefs;
    GetNumericColumn(table, column, x, undefs);

    double C, expected, p_value;
    GlobalGeary(x, undefs, gw, permutations, C, expected, p_value);

    std::vector<json_spirit::Pair> r;
    r.push_back(P("geary_c", json_spirit::Value(C)));
    r.push_back(P("expected", json_spirit::Value(expected)));
    r.push_back(P("p_value", json_spirit::Value(p_value)));
    r.push_back(P("permutations", json_spirit::Value(permutations)));
    return Obj(r);
}

json_spirit::Value McpGlobalGeneralG(const McpToolContext& ctx,
                                     const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    wxString column = GetStr(params, "column");
    wxString weights = GetStr(params, "weights");
    if (column.IsEmpty()) column = AskForColumn(ctx, table);
    if (column.IsEmpty() || weights.IsEmpty()) {
        throw McpError(-32602,
            "Missing required parameters: column, weights");
    }
    int permutations = GetInt(params, "permutations", 999);
    GalWeight* gw = GetGalWeight(ctx, weights);

    std::vector<double> x;
    std::vector<bool> undefs;
    GetNumericColumn(table, column, x, undefs);

    double G, expected, p_value;
    GlobalGeneralG(x, undefs, gw, permutations, G, expected, p_value);

    std::vector<json_spirit::Pair> r;
    r.push_back(P("general_g", json_spirit::Value(G)));
    r.push_back(P("expected", json_spirit::Value(expected)));
    r.push_back(P("p_value", json_spirit::Value(p_value)));
    r.push_back(P("permutations", json_spirit::Value(permutations)));
    return Obj(r);
}

// =========================================================================
// cluster
// =========================================================================
// Shared helper: extract the selected columns into a row-wise data matrix.
// Returns the number of rows and columns, and the undefs mask.
struct ClusterData
{
    int rows;
    int ncols;
    double** data;
    std::vector<bool> undefs;
};

ClusterData GetClusterData(const McpToolContext& ctx,
                           const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    std::vector<wxString> columns = GetStrArray(params, "columns");
    if (columns.empty()) {
        throw McpError(-32602, "Missing required parameter: columns");
    }
    int rows = project->GetNumRecords();
    int ncols = (int)columns.size();
    std::vector<std::vector<double> > col_data(ncols);
    std::vector<std::vector<bool> > col_undefs(ncols);
    for (int c = 0; c < ncols; ++c) {
        GetNumericColumn(table, columns[c], col_data[c], col_undefs[c]);
    }
    ClusterData cd;
    cd.rows = rows;
    cd.ncols = ncols;
    cd.undefs.assign(rows, false);
    for (int i = 0; i < rows; ++i) {
        for (int c = 0; c < ncols; ++c) {
            if (col_undefs[c][i]) cd.undefs[i] = true;
        }
    }
    cd.data = new double*[rows];
    for (int i = 0; i < rows; ++i) {
        cd.data[i] = new double[ncols];
        for (int c = 0; c < ncols; ++c) {
            cd.data[i][c] = col_data[c][i];
        }
    }
    return cd;
}

void FreeClusterData(ClusterData& cd)
{
    FreeDataMatrix(cd.data, cd.rows);
}

json_spirit::Value McpClusterSkater(const McpToolContext& ctx,
                                    const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    wxString weights = GetStr(params, "weights");
    if (weights.IsEmpty()) {
        throw McpError(-32602, "Missing required parameter: weights");
    }
    int k = GetInt(params, "k", 3);
    GalWeight* gw = GetGalWeight(ctx, weights);

    ClusterData cd = GetClusterData(ctx, params);
    double** dist = ComputeDistances(cd.data, cd.rows, cd.ncols, 'e');

    double* bound_vals = 0;
    double min_bound = 0;
    SpanningTreeClustering::Skater skater(cd.rows, cd.ncols, dist, cd.data,
                                          cd.undefs, gw->gal, bound_vals,
                                          min_bound);
    skater.Partitioning(k);
    std::vector<std::vector<int> >& regions = skater.GetRegions();

    std::vector<int> clusters(cd.rows, 0);
    for (size_t r = 0; r < regions.size(); ++r) {
        for (size_t j = 0; j < regions[r].size(); ++j) {
            clusters[regions[r][j]] = (int)r;
        }
    }
    FreeDistMatrix(dist, cd.rows);
    FreeClusterData(cd);

    std::vector<json_spirit::Pair> r;
    r.push_back(P("clusters", ClusterResult(clusters)));
    r.push_back(P("k", json_spirit::Value(k)));
    return Obj(r);
}

json_spirit::Value McpClusterRedcap(const McpToolContext& ctx,
                                   const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    wxString weights = GetStr(params, "weights");
    if (weights.IsEmpty()) {
        throw McpError(-32602, "Missing required parameter: weights");
    }
    int k = GetInt(params, "k", 3);
    wxString method = GetStr(params, "method", "firstorder");
    GalWeight* gw = GetGalWeight(ctx, weights);

    ClusterData cd = GetClusterData(ctx, params);
    double** dist = ComputeDistances(cd.data, cd.rows, cd.ncols, 'e');

    double* bound_vals = 0;
    double min_bound = 0;
    SpanningTreeClustering::AbstractClusterFactory* redcap = NULL;
    if (method == "firstorder") {
        redcap = new SpanningTreeClustering::FirstOrderSLKRedCap(
            cd.rows, cd.ncols, dist, cd.data, cd.undefs, gw->gal, bound_vals,
            min_bound);
    } else if (method == "fullorder_ward") {
        redcap = new SpanningTreeClustering::FullOrderWardRedCap(
            cd.rows, cd.ncols, dist, cd.data, cd.undefs, gw->gal, bound_vals,
            min_bound);
    } else if (method == "fullorder_alk") {
        redcap = new SpanningTreeClustering::FullOrderALKRedCap(
            cd.rows, cd.ncols, dist, cd.data, cd.undefs, gw->gal, bound_vals,
            min_bound);
    } else if (method == "fullorder_clk") {
        redcap = new SpanningTreeClustering::FullOrderCLKRedCap(
            cd.rows, cd.ncols, dist, cd.data, cd.undefs, gw->gal, bound_vals,
            min_bound);
    } else if (method == "fullorder_slk") {
        redcap = new SpanningTreeClustering::FullOrderSLKRedCap(
            cd.rows, cd.ncols, dist, cd.data, cd.undefs, gw->gal, bound_vals,
            min_bound);
    } else {
        FreeDistMatrix(dist, cd.rows);
        FreeClusterData(cd);
        throw McpError(-32602,
            "Unknown redcap method: " + method.ToStdString());
    }
    redcap->Partitioning(k);
    std::vector<std::vector<int> >& regions = redcap->GetRegions();
    std::vector<int> clusters(cd.rows, 0);
    for (size_t r = 0; r < regions.size(); ++r) {
        for (size_t j = 0; j < regions[r].size(); ++j) {
            clusters[regions[r][j]] = (int)r;
        }
    }
    delete redcap;
    FreeDistMatrix(dist, cd.rows);
    FreeClusterData(cd);

    std::vector<json_spirit::Pair> r;
    r.push_back(P("clusters", ClusterResult(clusters)));
    r.push_back(P("k", json_spirit::Value(k)));
    r.push_back(P("method", json_spirit::Value(method.ToStdString())));
    return Obj(r);
}

json_spirit::Value McpClusterSchc(const McpToolContext& ctx,
                                  const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    wxString weights = GetStr(params, "weights");
    if (weights.IsEmpty()) {
        throw McpError(-32602, "Missing required parameter: weights");
    }
    int k = GetInt(params, "k", 3);
    wxString method = GetStr(params, "method", "singlelink");
    GalWeight* gw = GetGalWeight(ctx, weights);

    ClusterData cd = GetClusterData(ctx, params);
    double** dist = ComputeDistances(cd.data, cd.rows, cd.ncols, 'e');

    double* bound_vals = 0;
    double min_bound = 0;
    SpanningTreeClustering::AbstractClusterFactory* redcap = NULL;
    if (method == "singlelink") {
        redcap = new SpanningTreeClustering::FullOrderSLKRedCap(
            cd.rows, cd.ncols, dist, cd.data, cd.undefs, gw->gal, bound_vals,
            min_bound);
    } else if (method == "completelink") {
        redcap = new SpanningTreeClustering::FullOrderCLKRedCap(
            cd.rows, cd.ncols, dist, cd.data, cd.undefs, gw->gal, bound_vals,
            min_bound);
    } else if (method == "averagelink") {
        redcap = new SpanningTreeClustering::FullOrderALKRedCap(
            cd.rows, cd.ncols, dist, cd.data, cd.undefs, gw->gal, bound_vals,
            min_bound);
    } else if (method == "ward") {
        redcap = new SpanningTreeClustering::FullOrderWardRedCap(
            cd.rows, cd.ncols, dist, cd.data, cd.undefs, gw->gal, bound_vals,
            min_bound);
    } else {
        FreeDistMatrix(dist, cd.rows);
        FreeClusterData(cd);
        throw McpError(-32602,
            "Unknown schc method: " + method.ToStdString());
    }
    redcap->Partitioning(k);
    std::vector<std::vector<int> >& regions = redcap->GetRegions();
    std::vector<int> clusters(cd.rows, 0);
    for (size_t r = 0; r < regions.size(); ++r) {
        for (size_t j = 0; j < regions[r].size(); ++j) {
            clusters[regions[r][j]] = (int)r;
        }
    }
    delete redcap;
    FreeDistMatrix(dist, cd.rows);
    FreeClusterData(cd);

    std::vector<json_spirit::Pair> r;
    r.push_back(P("clusters", ClusterResult(clusters)));
    r.push_back(P("k", json_spirit::Value(k)));
    r.push_back(P("method", json_spirit::Value(method.ToStdString())));
    return Obj(r);
}

json_spirit::Value McpClusterMaxp(const McpToolContext& ctx,
                                  const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    wxString weights = GetStr(params, "weights");
    if (weights.IsEmpty()) {
        throw McpError(-32602, "Missing required parameter: weights");
    }
    double min_bound = GetDouble(params, "min_bound", 0.0);
    wxString bound_variable = GetStr(params, "bound_variable");
    GalWeight* gw = GetGalWeight(ctx, weights);

    ClusterData cd = GetClusterData(ctx, params);
    double** dist = ComputeDistances(cd.data, cd.rows, cd.ncols, 'e');
    RawDistMatrix dm(dist);

    std::vector<ZoneControl> controls;
    if (!bound_variable.IsEmpty() && min_bound > 0) {
        TableInterface* table = project->GetTableInt();
        std::vector<double> bv;
        std::vector<bool> undefs;
        GetNumericColumn(table, bound_variable, bv, undefs);
        ZoneControl zc(bv);
        zc.AddControl(ZoneControl::SUM, ZoneControl::MORE_THAN, min_bound);
        controls.push_back(zc);
    }

    int iterations = GetInt(params, "iterations", 1);
    MaxpRegion maxp(iterations, gw->gal, cd.data, &dm, cd.rows, cd.ncols,
                    controls, 1, std::vector<int>(), 123456789);
    std::vector<int> clusters = maxp.GetResults();
    for (size_t i = 0; i < clusters.size(); ++i) clusters[i] -= 1;

    FreeDistMatrix(dist, cd.rows);
    FreeClusterData(cd);

    std::vector<json_spirit::Pair> r;
    r.push_back(P("clusters", ClusterResult(clusters)));
    r.push_back(P("min_bound", json_spirit::Value(min_bound)));
    return Obj(r);
}

json_spirit::Value McpClusterAzp(const McpToolContext& ctx,
                                 const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    wxString weights = GetStr(params, "weights");
    if (weights.IsEmpty()) {
        throw McpError(-32602, "Missing required parameter: weights");
    }
    int k = GetInt(params, "k", 3);
    wxString method = GetStr(params, "method", "greedy");
    GalWeight* gw = GetGalWeight(ctx, weights);

    ClusterData cd = GetClusterData(ctx, params);
    double** dist = ComputeDistances(cd.data, cd.rows, cd.ncols, 'e');
    RawDistMatrix dm(dist);

    std::vector<ZoneControl> controls;
    std::vector<int> clusters;
    if (method == "greedy") {
        AZP azp(k, gw->gal, cd.data, &dm, cd.rows, cd.ncols, controls);
        clusters = azp.GetResults();
    } else if (method == "sa") {
        AZPSA azp(k, gw->gal, cd.data, &dm, cd.rows, cd.ncols, controls);
        clusters = azp.GetResults();
    } else if (method == "tabu") {
        AZPTabu azp(k, gw->gal, cd.data, &dm, cd.rows, cd.ncols, controls);
        clusters = azp.GetResults();
    } else {
        FreeDistMatrix(dist, cd.rows);
        FreeClusterData(cd);
        throw McpError(-32602,
            "Unknown azp method: " + method.ToStdString());
    }
    for (size_t i = 0; i < clusters.size(); ++i) clusters[i] -= 1;

    FreeDistMatrix(dist, cd.rows);
    FreeClusterData(cd);

    std::vector<json_spirit::Pair> r;
    r.push_back(P("clusters", ClusterResult(clusters)));
    r.push_back(P("k", json_spirit::Value(k)));
    r.push_back(P("method", json_spirit::Value(method.ToStdString())));
    return Obj(r);
}

json_spirit::Value McpClusterSpatialKmeans(const McpToolContext& ctx,
                                          const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    wxString weights = GetStr(params, "weights");
    if (weights.IsEmpty()) {
        throw McpError(-32602, "Missing required parameter: weights");
    }
    int k = GetInt(params, "k", 3);
    GalWeight* gw = GetGalWeight(ctx, weights);

    ClusterData cd = GetClusterData(ctx, params);

    // Run k-means first to get an initial clustering. 'b' is kmeans++ seeding,
    // 'a' is random initialization.
    char init_method = (GetStr(params, "init") == "kmeans++") ? 'b' : 'a';
    int* clusterid = new int[cd.rows];
    double error = 0.0;
    int ifound = 0;
    int** mask = new int*[cd.rows];
    for (int i = 0; i < cd.rows; ++i) {
        mask[i] = new int[cd.ncols];
        for (int c = 0; c < cd.ncols; ++c) mask[i][c] = 1;
    }
    double* weight = new double[cd.ncols];
    for (int c = 0; c < cd.ncols; ++c) weight[c] = 1.0;
    kcluster(k, cd.rows, cd.ncols, cd.data, mask, weight, 0, 10, 100,
             init_method, 'e', clusterid, &error, &ifound, 0, 0, 1, 1);
    for (int i = 0; i < cd.rows; ++i) delete[] mask[i];
    delete[] mask;
    delete[] weight;

    std::vector<std::vector<int> > clusters(k);
    for (int i = 0; i < cd.rows; ++i) {
        int c = clusterid[i];
        if (c >= 0 && c < k) clusters[c].push_back(i);
    }
    delete[] clusterid;

    SpatialKMeans skm(cd.rows, clusters, gw);
    skm.Run();
    std::vector<std::vector<int> > result = skm.GetClusters();

    std::vector<int> labels(cd.rows, 0);
    for (size_t r = 0; r < result.size(); ++r) {
        for (size_t j = 0; j < result[r].size(); ++j) {
            labels[result[r][j]] = (int)r;
        }
    }
    FreeClusterData(cd);

    std::vector<json_spirit::Pair> r;
    r.push_back(P("clusters", ClusterResult(labels)));
    r.push_back(P("k", json_spirit::Value(k)));
    return Obj(r);
}

json_spirit::Value McpClusterDbscan(const McpToolContext& ctx,
                                   const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    int minpts = GetInt(params, "minpts", 4);
    double eps = GetDouble(params, "eps", 0.0);

    ClusterData cd = GetClusterData(ctx, params);
    if (eps <= 0) {
        // Estimate eps from the data (max 1-NN distance), matching the
        // desktop DBSCAN dialog behavior.
        Gda::DistUtils dist_util(cd.data, cd.rows, cd.ncols,
                                 ANNuse_euclidean_dist);
        eps = dist_util.GetMinThreshold();
    }
    DBSCAN dbscan(minpts, (float)eps, (const double**)cd.data, cd.rows,
                  cd.ncols, ANNuse_euclidean_dist);
    std::vector<int> clusters = dbscan.getResults();
    // DBSCAN returns -1 for noise; convert to 0-based labels.
    std::vector<int> labels(cd.rows, 0);
    int next = 0;
    std::map<int, int> remap;
    for (int i = 0; i < cd.rows; ++i) {
        if (clusters[i] < 0) {
            labels[i] = 0;
        } else {
            if (remap.find(clusters[i]) == remap.end()) {
                remap[clusters[i]] = ++next;
            }
            labels[i] = remap[clusters[i]];
        }
    }
    FreeClusterData(cd);

    std::vector<json_spirit::Value> arr;
    for (int i = 0; i < cd.rows; ++i) {
        arr.push_back(json_spirit::Value(labels[i]));
    }
    std::vector<json_spirit::Pair> r;
    r.push_back(P("clusters", Arr(arr)));
    r.push_back(P("minpts", json_spirit::Value(minpts)));
    r.push_back(P("eps", json_spirit::Value(eps)));
    return Obj(r);
}

json_spirit::Value McpClusterHdbscan(const McpToolContext& ctx,
                                     const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    int minpts = GetInt(params, "minpts", 4);

    ClusterData cd = GetClusterData(ctx, params);
    double** dist = ComputeDistances(cd.data, cd.rows, cd.ncols, 'e');
    RawDistMatrix dm(dist);

    std::vector<double> core_dist = Gda::HDBScan::ComputeCoreDistance(
        cd.data, cd.rows, cd.ncols, minpts, 'e');
    // The HDBScan constructor runs the full clustering pipeline.
    Gda::HDBScan hdb(minpts, minpts, 1.0, 0, false, cd.rows, cd.ncols, &dm,
                     core_dist, cd.undefs);
    std::vector<std::vector<int> > regions = hdb.GetRegions();

    // HDBSCAN convention: 0 = noise, 1+ = cluster id.
    std::vector<int> labels(cd.rows, 0);
    for (size_t r = 0; r < regions.size(); ++r) {
        for (size_t j = 0; j < regions[r].size(); ++j) {
            labels[regions[r][j]] = (int)r + 1;
        }
    }
    FreeDistMatrix(dist, cd.rows);
    FreeClusterData(cd);

    std::vector<json_spirit::Value> arr;
    for (int i = 0; i < cd.rows; ++i) {
        arr.push_back(json_spirit::Value(labels[i]));
    }
    std::vector<json_spirit::Pair> r;
    r.push_back(P("clusters", Arr(arr)));
    r.push_back(P("minpts", json_spirit::Value(minpts)));
    return Obj(r);
}

json_spirit::Value McpClusterSpectral(const McpToolContext& ctx,
                                     const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    wxString weights = GetStr(params, "weights");
    if (weights.IsEmpty()) {
        throw McpError(-32602, "Missing required parameter: weights");
    }
    int k = GetInt(params, "k", 3);
    GalWeight* gw = GetGalWeight(ctx, weights);

    ClusterData cd = GetClusterData(ctx, params);
    Spectral spectral;
    spectral.set_data(cd.data, cd.rows, cd.ncols);
    spectral.set_centers(k);
    spectral.set_knn(6);
    spectral.set_kmeans_npass(10);
    spectral.set_kmeans_maxiter(300);
    spectral.cluster(0);
    const std::vector<wxInt64>& assignments = spectral.get_assignments();

    std::vector<int> labels(cd.rows, 0);
    for (int i = 0; i < cd.rows; ++i) {
        labels[i] = (int)assignments[i];
    }
    FreeClusterData(cd);

    std::vector<json_spirit::Pair> r;
    r.push_back(P("clusters", ClusterResult(labels)));
    r.push_back(P("k", json_spirit::Value(k)));
    return Obj(r);
}

json_spirit::Value McpClusterPam(const McpToolContext& ctx,
                                 const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    int k = GetInt(params, "k", 3);

    ClusterData cd = GetClusterData(ctx, params);
    double** dist = ComputeDistances(cd.data, cd.rows, cd.ncols, 'e');
    RawDistMatrix dm(dist);

    LAB init(&dm, 123456789);
    FastPAM pam(cd.rows, &dm, &init, k, 100, 1.0);
    pam.run();
    std::vector<int> clusters = pam.getResults();

    FreeDistMatrix(dist, cd.rows);
    FreeClusterData(cd);

    std::vector<json_spirit::Pair> r;
    r.push_back(P("clusters", ClusterResult(clusters)));
    r.push_back(P("k", json_spirit::Value(k)));
    return Obj(r);
}

json_spirit::Value McpClusterMds(const McpToolContext& ctx,
                                 const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    ClusterData cd = GetClusterData(ctx, params);
    double** dist = ComputeDistances(cd.data, cd.rows, cd.ncols, 'e');

    // Build a full distance matrix as vector<vector<double>>.
    std::vector<std::vector<double> > dmat(cd.rows,
                                           std::vector<double>(cd.rows, 0.0));
    for (int i = 0; i < cd.rows; ++i) {
        for (int j = 0; j < cd.rows; ++j) {
            dmat[i][j] = dist[i][j];
        }
    }
    FastMDS mds(dmat, 2, 100);
    std::vector<std::vector<double> >& result = mds.GetResult();

    std::vector<json_spirit::Value> coords;
    for (int i = 0; i < cd.rows; ++i) {
        std::vector<json_spirit::Value> pt;
        for (int j = 0; j < 2; ++j) {
            pt.push_back(json_spirit::Value(result[j][i]));
        }
        coords.push_back(Arr(pt));
    }
    FreeDistMatrix(dist, cd.rows);
    FreeClusterData(cd);

    std::vector<json_spirit::Pair> r;
    r.push_back(P("coordinates", Arr(coords)));
    r.push_back(P("dimensions", json_spirit::Value(2)));
    return Obj(r);
}

json_spirit::Value McpClusterPca(const McpToolContext& ctx,
                                 const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    ClusterData cd = GetClusterData(ctx, params);

    Pca pca(cd.data, cd.rows, cd.ncols);
    pca.Calculate();
    std::vector<float> sd = pca.sd();
    std::vector<float> prop = pca.prop_of_var();
    std::vector<float> cum = pca.cum_prop();
    std::vector<float> scores = pca.scores();
    std::vector<unsigned int> eliminated = pca.eliminated_columns();

    std::vector<json_spirit::Value> sd_arr, prop_arr, cum_arr;
    for (size_t i = 0; i < sd.size(); ++i) {
        sd_arr.push_back(json_spirit::Value(sd[i]));
        prop_arr.push_back(json_spirit::Value(prop[i]));
        cum_arr.push_back(json_spirit::Value(cum[i]));
    }
    std::vector<json_spirit::Value> score_rows;
    for (int i = 0; i < cd.rows; ++i) {
        std::vector<json_spirit::Value> row;
        for (int c = 0; c < cd.ncols; ++c) {
            row.push_back(json_spirit::Value(scores[i * cd.ncols + c]));
        }
        score_rows.push_back(Arr(row));
    }
    std::vector<json_spirit::Value> elim_arr;
    for (size_t i = 0; i < eliminated.size(); ++i) {
        elim_arr.push_back(json_spirit::Value((int)eliminated[i]));
    }
    FreeClusterData(cd);

    std::vector<json_spirit::Pair> r;
    r.push_back(P("standard_deviation", Arr(sd_arr)));
    r.push_back(P("proportion_of_variance", Arr(prop_arr)));
    r.push_back(P("cumulative_proportion", Arr(cum_arr)));
    r.push_back(P("scores", Arr(score_rows)));
    r.push_back(P("eliminated_columns", Arr(elim_arr)));
    r.push_back(P("kaiser", json_spirit::Value((int)pca.kaiser())));
    r.push_back(P("thresh95", json_spirit::Value((int)pca.thresh95())));
    return Obj(r);
}

json_spirit::Value McpClusterKmeans(const McpToolContext& ctx,
                                    const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    int k = GetInt(params, "k", 3);
    if (k < 1) {
        throw McpError(-32602, "k must be at least 1");
    }

    ClusterData cd = GetClusterData(ctx, params);
    if (cd.rows < k) {
        FreeClusterData(cd);
        throw McpError(-32602, "k is larger than the number of observations");
    }

    int* clusterid = new int[cd.rows];
    double error = 0.0;
    int ifound = 0;
    int** mask = new int*[cd.rows];
    for (int i = 0; i < cd.rows; ++i) {
        mask[i] = new int[cd.ncols];
        for (int c = 0; c < cd.ncols; ++c) mask[i][c] = 1;
    }
    double* weight = new double[cd.ncols];
    for (int c = 0; c < cd.ncols; ++c) weight[c] = 1.0;
    kcluster(k, cd.rows, cd.ncols, cd.data, mask, weight, 0, 10, 100, 'a',
             'e', clusterid, &error, &ifound, 0, 0, 1, 1);
    for (int i = 0; i < cd.rows; ++i) delete[] mask[i];
    delete[] mask;
    delete[] weight;

    std::vector<int> labels(cd.rows, 0);
    for (int i = 0; i < cd.rows; ++i) {
        if (clusterid[i] >= 0 && clusterid[i] < k) labels[i] = clusterid[i];
    }
    delete[] clusterid;
    FreeClusterData(cd);

    std::vector<json_spirit::Pair> r;
    r.push_back(P("clusters", ClusterResult(labels)));
    r.push_back(P("k", json_spirit::Value(k)));
    r.push_back(P("error", json_spirit::Value(error)));
    return Obj(r);
}

json_spirit::Value McpClusterKmedians(const McpToolContext& ctx,
                                      const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    int k = GetInt(params, "k", 3);
    if (k < 1) {
        throw McpError(-32602, "k must be at least 1");
    }

    ClusterData cd = GetClusterData(ctx, params);
    if (cd.rows < k) {
        FreeClusterData(cd);
        throw McpError(-32602, "k is larger than the number of observations");
    }

    // k-medians uses the Manhattan (cityblock) distance matrix with
    // k-medoids.
    double** dist = ComputeDistances(cd.data, cd.rows, cd.ncols, 'b');
    int* clusterid = new int[cd.rows];
    double error = 0.0;
    int ifound = 0;
    kmedoids(k, cd.rows, dist, 10, 100, clusterid, &error, &ifound, 0, 0, 1,
             1);

    std::vector<int> labels(cd.rows, 0);
    for (int i = 0; i < cd.rows; ++i) {
        if (clusterid[i] >= 0 && clusterid[i] < k) labels[i] = clusterid[i];
    }
    delete[] clusterid;
    FreeDistMatrix(dist, cd.rows);
    FreeClusterData(cd);

    std::vector<json_spirit::Pair> r;
    r.push_back(P("clusters", ClusterResult(labels)));
    r.push_back(P("k", json_spirit::Value(k)));
    r.push_back(P("error", json_spirit::Value(error)));
    return Obj(r);
}

json_spirit::Value McpClusterHierarchical(const McpToolContext& ctx,
                                          const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    int k = GetInt(params, "k", 3);
    wxString method = GetStr(params, "method", "average");
    if (k < 1) {
        throw McpError(-32602, "k must be at least 1");
    }

    ClusterData cd = GetClusterData(ctx, params);
    if (cd.rows < k) {
        FreeClusterData(cd);
        throw McpError(-32602, "k is larger than the number of observations");
    }

    int** mask = new int*[cd.rows];
    for (int i = 0; i < cd.rows; ++i) {
        mask[i] = new int[cd.ncols];
        for (int c = 0; c < cd.ncols; ++c) mask[i][c] = 1;
    }
    double* weight = new double[cd.ncols];
    for (int c = 0; c < cd.ncols; ++c) weight[c] = 1.0;

    char m = 'a';  // average linkage
    if (method == "single") m = 's';
    else if (method == "complete") m = 'c';
    else if (method == "ward") m = 'a';  // ward not supported; fall back

    double** distmatrix = 0;
    GdaNode* tree = treecluster(cd.rows, cd.ncols, cd.data, mask, weight, 0,
                                'e', m, distmatrix);
    if (!tree) {
        for (int i = 0; i < cd.rows; ++i) delete[] mask[i];
        delete[] mask;
        delete[] weight;
        FreeClusterData(cd);
        throw McpError(-32602, "Hierarchical clustering failed");
    }

    int* clusterid = new int[cd.rows];
    cuttree(cd.rows, tree, k, clusterid);
    free(tree);

    std::vector<int> labels(cd.rows, 0);
    for (int i = 0; i < cd.rows; ++i) {
        if (clusterid[i] >= 0 && clusterid[i] < k) labels[i] = clusterid[i];
    }
    delete[] clusterid;
    for (int i = 0; i < cd.rows; ++i) delete[] mask[i];
    delete[] mask;
    delete[] weight;
    FreeClusterData(cd);

    std::vector<json_spirit::Pair> r;
    r.push_back(P("clusters", ClusterResult(labels)));
    r.push_back(P("k", json_spirit::Value(k)));
    r.push_back(P("method", json_spirit::Value(method.ToStdString())));
    return Obj(r);
}

// =========================================================================
// regress
// =========================================================================
json_spirit::Value McpRegressClassic(const McpToolContext& ctx,
                                     const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    wxString dep = GetStr(params, "dependent");
    if (dep.IsEmpty()) {
        throw McpError(-32602, "Missing required parameter: dependent");
    }
    std::vector<wxString> indep = GetStrArray(params, "independent");
    if (indep.empty()) {
        throw McpError(-32602, "Missing required parameter: independent");
    }
    bool include_constant = GetBool(params, "include_constant", true);

    int n = project->GetNumRecords();
    int dep_col = RequireColumn(table, dep);
    std::vector<double> y;
    std::vector<bool> y_undef;
    table->GetColData(dep_col, 0, y, y_undef);

    int nX = (int)indep.size();
    std::vector<std::vector<double> > x(nX);
    std::vector<std::vector<bool> > x_undef(nX);
    for (int c = 0; c < nX; ++c) {
        int col = RequireColumn(table, indep[c]);
        table->GetColData(col, 0, x[c], x_undef[c]);
    }

    // Complete-case analysis: drop rows with any undefined value.
    std::vector<int> valid_rows;
    for (int i = 0; i < n; ++i) {
        if (y_undef[i]) continue;
        bool ok = true;
        for (int c = 0; c < nX; ++c) {
            if (x_undef[c][i]) { ok = false; break; }
        }
        if (ok) valid_rows.push_back(i);
    }
    int m = (int)valid_rows.size();
    if (m < nX + 2) {
        throw McpError(-32602,
            "Not enough valid observations for regression");
    }

    double* Y = new double[m];
    double** X = new double*[nX];
    for (int c = 0; c < nX; ++c) X[c] = new double[m];
    for (int r = 0; r < m; ++r) {
        int i = valid_rows[r];
        Y[r] = y[i];
        for (int c = 0; c < nX; ++c) X[c][r] = x[c][i];
    }

    DiagnosticReport dr(m, nX, include_constant, false, 0);
    bool ok = classicalRegression(NULL, m, Y, m, X, nX, &dr,
                                   include_constant, false, NULL, false);

    std::vector<json_spirit::Value> coeffs, sterrs, zvals, probs, names;
    int ncoef = include_constant ? nX + 1 : nX;
    for (int i = 0; i < ncoef; ++i) {
        names.push_back(json_spirit::Value(
            (i == 0 && include_constant)
                ? "CONSTANT"
                : indep[i - (include_constant ? 1 : 0)].ToStdString()));
        coeffs.push_back(json_spirit::Value(dr.GetCoefficient(i)));
        sterrs.push_back(json_spirit::Value(dr.GetStdError(i)));
        zvals.push_back(json_spirit::Value(dr.GetZValue(i)));
        probs.push_back(json_spirit::Value(dr.GetProbability(i)));
    }

    for (int c = 0; c < nX; ++c) delete[] X[c];
    delete[] X;
    delete[] Y;

    std::vector<json_spirit::Pair> r;
    r.push_back(P("ok", json_spirit::Value(ok)));
    r.push_back(P("dependent", json_spirit::Value(dep.ToStdString())));
    r.push_back(P("num_obs", json_spirit::Value(m)));
    r.push_back(P("num_vars", json_spirit::Value(ncoef)));
    r.push_back(P("include_constant", json_spirit::Value(include_constant)));
    r.push_back(P("names", Arr(names)));
    r.push_back(P("coefficients", Arr(coeffs)));
    r.push_back(P("std_errors", Arr(sterrs)));
    r.push_back(P("t_stats", Arr(zvals)));
    r.push_back(P("p_values", Arr(probs)));
    r.push_back(P("r_squared", json_spirit::Value(dr.GetR2())));
    r.push_back(P("r_squared_adjusted", json_spirit::Value(dr.GetR2_adjust())));
    r.push_back(P("log_likelihood", json_spirit::Value(dr.GetLIK())));
    r.push_back(P("aic", json_spirit::Value(dr.GetAIC())));
    r.push_back(P("f_stat", json_spirit::Value(dr.GetFtest())));
    r.push_back(P("f_stat_prob", json_spirit::Value(dr.GetFtestProb())));
    return Obj(r);
}

// =========================================================================
// window
// =========================================================================
json_spirit::Value McpWindowCreateMap(const McpToolContext& ctx,
                                     const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    wxString column = GetStr(params, "column");
    if (column.IsEmpty()) {
        throw McpError(-32602, "Missing required parameter: column");
    }
    wxString theme_s = GetStr(params, "theme", "quantile");
    int num_categories = GetInt(params, "num_categories", 5);
    wxString weights = GetStr(params, "weights");
    wxString smoothing_s = GetStr(params, "smoothing", "no_smoothing");

    boost::uuids::uuid weights_id = boost::uuids::nil_uuid();
    if (!weights.IsEmpty()) {
        weights_id = RequireWeights(ctx, weights);
    }

    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    var_info.push_back(BuildVarInfo(table, column));
    col_ids.push_back(RequireColumn(table, column));
    GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);

    CatClassification::CatClassifType theme = CatClassification::quantile;
    if (theme_s == "no_theme") theme = CatClassification::no_theme;
    else if (theme_s == "hinge_15") theme = CatClassification::hinge_15;
    else if (theme_s == "hinge_30") theme = CatClassification::hinge_30;
    else if (theme_s == "quantile") theme = CatClassification::quantile;
    else if (theme_s == "percentile") theme = CatClassification::percentile;
    else if (theme_s == "stddev") theme = CatClassification::stddev;
    else if (theme_s == "excess_risk")
        theme = CatClassification::excess_risk_theme;
    else if (theme_s == "unique_values")
        theme = CatClassification::unique_values;
    else if (theme_s == "natural_breaks")
        theme = CatClassification::natural_breaks;
    else if (theme_s == "equal_intervals")
        theme = CatClassification::equal_intervals;

    MapCanvas::SmoothingType smoothing = MapCanvas::no_smoothing;
    if (smoothing_s == "raw_rate") smoothing = MapCanvas::raw_rate;
    else if (smoothing_s == "excess_risk") smoothing = MapCanvas::excess_risk;
    else if (smoothing_s == "empirical_bayes")
        smoothing = MapCanvas::empirical_bayes;
    else if (smoothing_s == "spatial_rate")
        smoothing = MapCanvas::spatial_rate;
    else if (smoothing_s == "spatial_empirical_bayes")
        smoothing = MapCanvas::spatial_empirical_bayes;

    MapFrame* nf = new MapFrame(GdaFrame::GetGdaFrame(), project, var_info,
                                col_ids, theme, smoothing, num_categories,
                                weights_id);
    nf->UpdateTitle();

    std::vector<json_spirit::Pair> r;
    r.push_back(P("created", json_spirit::Value(true)));
    r.push_back(P("column", json_spirit::Value(column.ToStdString())));
    r.push_back(P("theme", json_spirit::Value(theme_s.ToStdString())));
    r.push_back(P("num_categories", json_spirit::Value(num_categories)));
    json_spirit::Value summary = Obj(r);
    if (GetBool(params, "return_image", false)) {
        std::string b64 = RenderFrameToPNGBase64(nf);
        if (!b64.empty()) {
            return ImageResult(b64, summary);
        }
    }
    return summary;
}

json_spirit::Value McpWindowCreatePlot(const McpToolContext& ctx,
                                      const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    wxString plot_type = GetStr(params, "plot_type");
    if (plot_type.IsEmpty()) {
        throw McpError(-32602, "Missing required parameter: plot_type");
    }
    std::vector<wxString> columns = GetStrArray(params, "columns");
    if (columns.empty()) {
        throw McpError(-32602, "Missing required parameter: columns");
    }
    wxString title = GetStr(params, "title");

    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    for (size_t i = 0; i < columns.size(); ++i) {
        var_info.push_back(BuildVarInfo(table, columns[i]));
        col_ids.push_back(RequireColumn(table, columns[i]));
    }
    GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);

    TemplateFrame* nf = NULL;
    if (plot_type == "histogram") {
        HistogramFrame* hf = new HistogramFrame(GdaFrame::GetGdaFrame(),
                                                project, var_info, col_ids,
                                                title);
        nf = hf;
        int bins = GetInt(params, "bins", 0);
        if (bins > 0) {
            HistogramCanvas* hc =
                dynamic_cast<HistogramCanvas*>(hf->template_canvas);
            if (hc) hc->SetNumIntervals(bins);
        }
    } else if (plot_type == "boxplot") {
        nf = new BoxPlotFrame(GdaFrame::GetGdaFrame(), project,
                              var_info, col_ids, title);
    } else if (plot_type == "scatter") {
        nf = new ScatterNewPlotFrame(GdaFrame::GetGdaFrame(), project,
                                     var_info, col_ids, false, title);
    } else {
        throw McpError(-32602,
            "Unknown plot_type: " + plot_type.ToStdString() +
            " (expected histogram, boxplot, or scatter)");
    }
    nf->UpdateTitle();

    std::vector<json_spirit::Pair> r;
    r.push_back(P("created", json_spirit::Value(true)));
    r.push_back(P("plot_type", json_spirit::Value(plot_type.ToStdString())));
    json_spirit::Value summary = Obj(r);
    if (GetBool(params, "return_image", false)) {
        std::string b64 = RenderFrameToPNGBase64(nf);
        if (!b64.empty()) {
            return ImageResult(b64, summary);
        }
    }
    return summary;
}

json_spirit::Value McpWindowCreateLisaMap(const McpToolContext& ctx,
                                          const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    wxString column = GetStr(params, "column");
    wxString weights = GetStr(params, "weights");
    if (column.IsEmpty() || weights.IsEmpty()) {
        throw McpError(-32602,
            "Missing required parameters: column, weights");
    }
    wxString map_type = GetStr(params, "map_type", "cluster");
    int permutations = GetInt(params, "permutations", 999);
    double cutoff = GetDouble(params, "significance_cutoff", 0.05);
    bool row_standardize = GetBool(params, "row_standardize", true);
    wxString lisa_type_s = GetStr(params, "lisa_type", "univariate");
    wxString second_column = GetStr(params, "second_column");

    boost::uuids::uuid uid = RequireWeights(ctx, weights);

    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    var_info.push_back(BuildVarInfo(table, column));
    col_ids.push_back(RequireColumn(table, column));
    if (lisa_type_s == "bivariate" || lisa_type_s == "differential") {
        if (second_column.IsEmpty()) {
            throw McpError(-32602,
                "second_column is required for bivariate/differential LISA");
        }
        var_info.push_back(BuildVarInfo(table, second_column));
        col_ids.push_back(RequireColumn(table, second_column));
    }
    GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);

    LisaCoordinator::LisaType lisa_type = LisaCoordinator::univariate;
    if (lisa_type_s == "bivariate") lisa_type = LisaCoordinator::bivariate;
    else if (lisa_type_s == "eb_rate_standardized")
        lisa_type = LisaCoordinator::eb_rate_standardized;
    else if (lisa_type_s == "differential")
        lisa_type = LisaCoordinator::differential;

    // The LisaMapFrame takes ownership of the coordinator.
    LisaCoordinator* lc = new LisaCoordinator(uid, project, var_info, col_ids,
                                              lisa_type, true, row_standardize,
                                              false);
    lc->SetNumPermutations(permutations);
    lc->SetSignificanceCutoff(cutoff);
    lc->CalcPseudoP();

    bool is_cluster = (map_type == "cluster");
    bool is_bi = (lisa_type == LisaCoordinator::bivariate);
    bool is_eb = (lisa_type == LisaCoordinator::eb_rate_standardized);
    LisaMapFrame* nf = new LisaMapFrame(GdaFrame::GetGdaFrame(), project, lc,
                                        is_cluster, is_bi, is_eb);
    nf->UpdateTitle();

    std::vector<json_spirit::Pair> r;
    r.push_back(P("created", json_spirit::Value(true)));
    r.push_back(P("map_type", json_spirit::Value(map_type.ToStdString())));
    r.push_back(P("column", json_spirit::Value(column.ToStdString())));
    r.push_back(P("lisa_type", json_spirit::Value(lisa_type_s.ToStdString())));
    json_spirit::Value summary = Obj(r);
    if (GetBool(params, "return_image", false)) {
        std::string b64 = RenderFrameToPNGBase64(nf);
        if (!b64.empty()) {
            return ImageResult(b64, summary);
        }
    }
    return summary;
}

json_spirit::Value McpWindowCreateScatterPlotMatrix(const McpToolContext& ctx,
                                                    const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    ScatterPlotMatFrame* nf = new ScatterPlotMatFrame(
        GdaFrame::GetGdaFrame(), project, _("Scatter Plot Matrix"),
        wxDefaultPosition, GdaConst::scatterplot_default_size);
    nf->UpdateTitle();

    std::vector<json_spirit::Pair> r;
    r.push_back(P("created", json_spirit::Value(true)));
    r.push_back(P("plot_type", json_spirit::Value("scatterplot_matrix")));
    return Obj(r);
}

json_spirit::Value McpWindowCreateBubbleChart(const McpToolContext& ctx,
                                              const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    std::vector<wxString> columns = GetStrArray(params, "columns");
    if (columns.size() < 3) {
        throw McpError(-32602,
            "Bubble chart requires at least 3 columns (x, y, size)");
    }
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    for (size_t i = 0; i < columns.size(); ++i) {
        var_info.push_back(BuildVarInfo(table, columns[i]));
        col_ids.push_back(RequireColumn(table, columns[i]));
    }
    GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);

    ScatterNewPlotFrame* nf = new ScatterNewPlotFrame(
        GdaFrame::GetGdaFrame(), project, var_info, col_ids, true,
        _("Bubble Chart"), wxDefaultPosition,
        GdaConst::bubble_chart_default_size);
    nf->UpdateTitle();

    std::vector<json_spirit::Pair> r;
    r.push_back(P("created", json_spirit::Value(true)));
    r.push_back(P("plot_type", json_spirit::Value("bubble_chart")));
    return Obj(r);
}

json_spirit::Value McpWindowCreate3DScatter(const McpToolContext& ctx,
                                            const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    std::vector<wxString> columns = GetStrArray(params, "columns");
    if (columns.size() < 3) {
        throw McpError(-32602,
            "3D scatter requires at least 3 columns (x, y, z)");
    }
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    for (size_t i = 0; i < columns.size(); ++i) {
        var_info.push_back(BuildVarInfo(table, columns[i]));
        col_ids.push_back(RequireColumn(table, columns[i]));
    }
    GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);

    C3DPlotFrame* nf = new C3DPlotFrame(
        GdaFrame::GetGdaFrame(), project, var_info, col_ids, _("3D Plot"),
        std::vector<wxString>(),
        std::vector<std::pair<wxString, double> >(), wxDefaultPosition,
        GdaConst::three_d_default_size, wxDEFAULT_FRAME_STYLE);
    nf->UpdateTitle();

    std::vector<json_spirit::Pair> r;
    r.push_back(P("created", json_spirit::Value(true)));
    r.push_back(P("plot_type", json_spirit::Value("3d_scatter")));
    return Obj(r);
}

json_spirit::Value McpWindowCreatePcp(const McpToolContext& ctx,
                                     const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    std::vector<wxString> columns = GetStrArray(params, "columns");
    if (columns.empty()) {
        throw McpError(-32602, "Missing required parameter: columns");
    }
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    for (size_t i = 0; i < columns.size(); ++i) {
        var_info.push_back(BuildVarInfo(table, columns[i]));
        col_ids.push_back(RequireColumn(table, columns[i]));
    }
    GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);

    PCPFrame* nf = new PCPFrame(GdaFrame::GetGdaFrame(), project, var_info,
                                col_ids);
    nf->UpdateTitle();

    std::vector<json_spirit::Pair> r;
    r.push_back(P("created", json_spirit::Value(true)));
    r.push_back(P("plot_type", json_spirit::Value("pcp")));
    return Obj(r);
}

json_spirit::Value McpWindowCreateLineChart(const McpToolContext& ctx,
                                           const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    LineChartFrame* nf = new LineChartFrame(
        GdaFrame::GetGdaFrame(), project, _("Average Comparison Chart"),
        wxDefaultPosition, GdaConst::line_chart_default_size);
    nf->UpdateTitle();

    std::vector<json_spirit::Pair> r;
    r.push_back(P("created", json_spirit::Value(true)));
    r.push_back(P("plot_type", json_spirit::Value("line_chart")));
    return Obj(r);
}

json_spirit::Value McpWindowCreateCorrelogram(const McpToolContext& ctx,
                                              const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    CorrelogramFrame* nf = new CorrelogramFrame(GdaFrame::GetGdaFrame(),
                                                project);
    nf->UpdateTitle();

    std::vector<json_spirit::Pair> r;
    r.push_back(P("created", json_spirit::Value(true)));
    r.push_back(P("plot_type", json_spirit::Value("correlogram")));
    return Obj(r);
}

json_spirit::Value McpWindowCreateDistancePlot(const McpToolContext& ctx,
                                               const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    wxString x_name = GetStr(params, "x_column");
    wxString y_name = GetStr(params, "y_column");
    if (x_name.IsEmpty() || y_name.IsEmpty()) {
        throw McpError(-32602,
            "Missing required parameters: x_column, y_column");
    }
    int x_col = RequireColumn(table, x_name);
    int y_col = RequireColumn(table, y_name);
    std::vector<double> x, y;
    std::vector<bool> x_undef, y_undef;
    table->GetColData(x_col, 0, x, x_undef);
    table->GetColData(y_col, 0, y, y_undef);

    double x_min = 0, x_max = 0, y_min = 0, y_max = 0;
    table->GetMinMaxVals(x_col, 0, x_min, x_max);
    table->GetMinMaxVals(y_col, 0, y_min, y_max);

    DistancePlotFrame* nf = new DistancePlotFrame(
        GdaFrame::GetGdaFrame(), project, x, y, x_undef, y_undef, x_min,
        x_max, y_min, y_max, x_name, y_name, _("Distance Plot"));
    nf->UpdateTitle();

    std::vector<json_spirit::Pair> r;
    r.push_back(P("created", json_spirit::Value(true)));
    r.push_back(P("plot_type", json_spirit::Value("distance_plot")));
    return Obj(r);
}

json_spirit::Value McpWindowCreateMoranScatterplot(const McpToolContext& ctx,
                                                   const json_spirit::Object& params)
{
    Project* project = RequireProject(ctx);
    TableInterface* table = project->GetTableInt();
    if (!table) {
        throw McpError(-32602, "The open project has no table.");
    }
    wxString column = GetStr(params, "column");
    wxString weights = GetStr(params, "weights");
    if (column.IsEmpty() || weights.IsEmpty()) {
        throw McpError(-32602,
            "Missing required parameters: column, weights");
    }
    boost::uuids::uuid uid = RequireWeights(ctx, weights);

    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    var_info.push_back(BuildVarInfo(table, column));
    col_ids.push_back(RequireColumn(table, column));
    GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);

    // The LisaScatterPlotFrame takes ownership of the coordinator.
    LisaCoordinator* lc = new LisaCoordinator(uid, project, var_info, col_ids,
                                              LisaCoordinator::univariate,
                                              false);
    LisaScatterPlotFrame* nf = new LisaScatterPlotFrame(
        GdaFrame::GetGdaFrame(), project, lc);
    nf->UpdateTitle();

    std::vector<json_spirit::Pair> r;
    r.push_back(P("created", json_spirit::Value(true)));
    r.push_back(P("plot_type", json_spirit::Value("moran_scatterplot")));
    return Obj(r);
}
