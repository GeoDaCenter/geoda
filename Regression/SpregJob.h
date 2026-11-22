/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
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

#ifndef __GEODA_CENTER_SPREG_JOB_H__
#define __GEODA_CENTER_SPREG_JOB_H__

#include <map>
#include <string>
#include <vector>
#include <wx/string.h>

class GalElement;

/**
 * The job protocol of spreg_engine/PROTOCOL.md, from the C++ side: writing the
 * directory the solver reads, and reading the answer it writes back.
 *
 * The dialog does not know anything about spreg's classes or its arguments -
 * that is what spreg_engine/solver/spreg_models.py is for.  Here we only
 * marshal GeoDa's data into the format on one side and read the numbers out of
 * the result on the other.
 */
namespace SpregJob {

/** Writes data.bin and job.json into one directory. */
class Writer {
public:
	explicit Writer(const wxString& job_dir);

	/** The observation index of the dependent variable, n x 1. */
	bool AddY(const std::vector<double>& y, wxString& err);
	/** The covariates, n by k, no constant (the solver adds it). */
	bool AddX(const std::vector<std::vector<double> >& x, wxString& err);
	/** The weights, as the CSR GeoDa's GalElement[] already is. */
	bool AddWeights(const GalElement* gal, int n, wxString& err);
	/** The same, straight from three arrays, for callers that have no GAL. */
	bool AddWeights(const std::vector<int>& indptr,
					const std::vector<int>& indices,
					const std::vector<double>& values, int n, wxString& err);
	/** A regime membership variable, one id per observation (a "regimes" model). */
	bool AddRegimes(const std::vector<int>& regimes, wxString& err);
	/** The two coordinate columns, n by 2, for the models that build W from them. */
	bool AddCoords(const std::vector<double>& xs, const std::vector<double>& ys,
				   wxString& err);
	/** Endogenous variables and their instruments, n by q, for the IV models. */
	bool AddEndogenous(const std::vector<std::vector<double> >& yend, wxString& err);
	bool AddInstruments(const std::vector<std::vector<double> >& q, wxString& err);

	/**
	 * Writes job.json.  `model` is an id from the solver's registry, `options`
	 * are the option names that registry declares.  The names travel with the
	 * data so that the result comes back with GeoDa's variable names on it.
	 */
	bool Write(const wxString& model,
			   const std::map<wxString, wxString>& options,
			   const wxString& y_name, const std::vector<wxString>& x_names,
			   const wxString& weights_name, const wxString& regimes_name,
			   const std::vector<wxString>& yend_names,
			   const std::vector<wxString>& q_names,
			   const std::vector<wxString>& coords_names, wxString& err);

	const wxString& path() const { return dir_; }

private:
	bool AddFloatArray(const wxString& name, const std::vector<double>& values,
					   int rows, int cols, wxString& err);
	bool AddIntArray(const wxString& name, const std::vector<int>& values,
					 int rows, int cols, wxString& err);
	struct Array { wxString name, dtype; int rows, cols; long long offset, nbytes; };

	wxString dir_;
	long long offset_;
	std::vector<Array> arrays_;
	bool have_y_, have_x_, have_w_;
	int n_;
	int k_;
};

/** What the solver wrote back, in the terms the report needs. */
struct Result {
	bool ok;
	int n, k;
	wxString requested_model, class_name, title, error_message;
	wxString spreg_version;
	std::vector<wxString> names, roles;
	std::vector<double> estimate, std_err, z, p;   // nulls become NaN
	std::map<wxString, double> fit;
	struct Diagnostic {
		wxString group, label;
		double statistic, df, p;
		Diagnostic() : statistic(0), df(0), p(0) {}
	};
	std::vector<Diagnostic> diagnostics;
	std::vector<double> yhat, resid, pred_err;     // empty when not reported
	// regionalization results (SKATER): one region id per observation, and how
	// many observations each region has
	std::vector<double> region;
	int n_regions;
	std::vector<double> region_sizes;
	// "regimes" models: one group of coefficients per regime, as the solver
	// named them ("0_INC", "1_INC", and "_Global_..." for the shared ones)
	bool has_global_rows;
	std::vector<wxString> warnings;

	Result() : ok(false), n(0), k(0), has_global_rows(false), n_regions(0) {}

	/** Reads result.json (and result.bin) from a job directory. */
	bool Read(const wxString& job_dir, wxString& err);
};

/** One choice a model offers, as the registry describes it. */
struct OptionSpec {
	wxString name;                            // the keyword the engine knows it by
	wxString label;                           // what to call it in the dialog, if it has a name
	wxString type;                             // "bool", "int", "float", "enum", "str"
	wxString help;
	std::vector<wxString> values;              // for "enum"
	wxString default_value;                    // as text, whatever the type
	double min_value, max_value;
	bool has_range;
	OptionSpec() : min_value(0), max_value(0), has_range(false) {}
};

/** A model the solver offers, from `solve.py --list-models`. */
struct ModelOption {
	wxString id, label, family;
	bool needs_weights, needs_regimes, needs_endog, needs_instruments, needs_coords;
	bool needs_binary_y;
	// in the order the engine's registry declares them - the estimation options
	// first and the tests of the chosen model after them, which is the order the
	// dialog shows them in and the order a map would have thrown away
	std::vector<OptionSpec> options;
	ModelOption() : needs_weights(true), needs_regimes(false), needs_endog(false),
		needs_instruments(false), needs_coords(false), needs_binary_y(false) {}
};

/**
 * The models the engine offers.
 *
 * Starting the engine costs a second or two, so its answer is kept in the
 * engine directory (model_list.json) and read from there from then on; the
 * cache lives and dies with the engine.  With allow_cache false it always asks
 * the engine, which is what warming the cache after an installation does.
 */
bool ListModels(const wxString& engine_dir, std::vector<ModelOption>& models,
				wxString& err, bool allow_cache = true);

/** Where that answer is kept, i.e. <engine_dir>/model_list.json. */
wxString ModelListCachePath(const wxString& engine_dir);

} // namespace SpregJob

#endif
