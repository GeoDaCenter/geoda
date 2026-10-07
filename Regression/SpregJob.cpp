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

#include "SpregJob.h"
#include "SpregEngine.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/utils.h>

#include <json_spirit/json_spirit.h>

#include "../ShapeOperations/GalWeight.h"

using namespace SpregEngine;

namespace {

const char* kFloat64 = "f8";
const char* kInt64 = "i8";
const char* kInt32 = "i4";

std::string ToUtf8(const wxString& text) { return text.utf8_string(); }

const json_spirit::Value* FindMember(const json_spirit::Object& obj,
									 const char* name)
{
	for (json_spirit::Object::const_iterator it = obj.begin(); it != obj.end(); ++it) {
		if (it->name_ == name) return &it->value_;
	}
	return NULL;
}

wxString AsString(const json_spirit::Value* value)
{
	if (!value || value->type() != json_spirit::str_type) return wxEmptyString;
	return wxString::FromUTF8(value->get_str().c_str());
}

double AsDouble(const json_spirit::Value* value, double fallback)
{
	if (!value) return fallback;
	if (value->type() == json_spirit::int_type) {
		return static_cast<double>(value->get_int64());
	}
	if (value->is_uint64()) return static_cast<double>(value->get_uint64());
	if (value->type() == json_spirit::real_type) return value->get_real();
	return fallback;
}

bool AsBool(const json_spirit::Value* value, bool fallback)
{
	if (!value || value->type() != json_spirit::bool_type) return fallback;
	return value->get_bool();
}

/** A number that may be absent: JSON null becomes NaN, which prints as blank. */
double AsMaybeDouble(const json_spirit::Value& value)
{
	if (value.type() == json_spirit::int_type) {
		return static_cast<double>(value.get_int64());
	}
	if (value.is_uint64()) return static_cast<double>(value.get_uint64());
	if (value.type() == json_spirit::real_type) return value.get_real();
	return std::numeric_limits<double>::quiet_NaN();
}

} // namespace

namespace SpregJob {

// ---------------------------------------------------------------------------
// writing the job
// ---------------------------------------------------------------------------

Writer::Writer(const wxString& job_dir)
	: dir_(job_dir), offset_(0), have_y_(false), have_x_(false), have_w_(false),
	  n_(0), k_(0)
{
}

bool Writer::AddFloatArray(const wxString& name, const std::vector<double>& values,
						   int rows, int cols, wxString& err)
{
	const wxString path = dir_ + wxFileName::GetPathSeparator() + "data.bin";
	wxFile file(path, wxFile::write_append);      // creates it when it is new
	if (!file.IsOpened()) {
		err = wxString::Format(_("Could not write %s"), path);
		return false;
	}
	Array array;
	array.name = name;
	array.dtype = kFloat64;
	array.rows = rows;
	array.cols = cols;
	array.offset = offset_;
	array.nbytes = static_cast<long long>(values.size()) * sizeof(double);
	if (array.nbytes > 0 && file.Write(&values[0], values.size() * sizeof(double))
		!= values.size() * sizeof(double)) {
		err = wxString::Format(_("Could not write %s"), path);
		return false;
	}
	file.Close();
	offset_ += array.nbytes;
	arrays_.push_back(array);
	return true;
}

bool Writer::AddIntArray(const wxString& name, const std::vector<int>& values,
						 int rows, int cols, wxString& err)
{
	const wxString path = dir_ + wxFileName::GetPathSeparator() + "data.bin";
	wxFile file(path, wxFile::write_append);      // creates it when it is new
	if (!file.IsOpened()) {
		err = wxString::Format(_("Could not write %s"), path);
		return false;
	}
	Array array;
	array.name = name;
	array.dtype = kInt32;
	array.rows = rows;
	array.cols = cols;
	array.offset = offset_;
	array.nbytes = static_cast<long long>(values.size()) * sizeof(int);
	if (array.nbytes > 0 && file.Write(&values[0], values.size() * sizeof(int))
		!= values.size() * sizeof(int)) {
		err = wxString::Format(_("Could not write %s"), path);
		return false;
	}
	file.Close();
	offset_ += array.nbytes;
	arrays_.push_back(array);
	return true;
}

bool Writer::AddY(const std::vector<double>& y, wxString& err)
{
	n_ = static_cast<int>(y.size());
	have_y_ = AddFloatArray("y", y, n_, 1, err);
	return have_y_;
}

bool Writer::AddX(const std::vector<std::vector<double> >& x, wxString& err)
{
	if (x.empty()) {
		err = _("There are no covariates to regress on.");
		return false;
	}
	k_ = static_cast<int>(x.size());
	const int n = static_cast<int>(x[0].size());
	// row major, as the protocol says: n rows, k columns, so that element
	// (i, j) sits at i*k + j and numpy's reshape finds what it expects
	std::vector<double> flat;
	flat.reserve(static_cast<size_t>(n) * k_);
	for (int row = 0; row < n; ++row) {
		for (int col = 0; col < k_; ++col) flat.push_back(x[col][row]);
	}
	have_x_ = AddFloatArray("x", flat, n, k_, err);
	return have_x_;
}

bool Writer::AddWeights(const GalElement* gal, int n, wxString& err)
{
	if (!gal) {
		err = _("No spatial weights matrix was given.");
		return false;
	}
	// GeoDa's GalElement[] is already a CSR structure: a neighbour list and a
	// parallel weight list per observation
	std::vector<int> indptr(n + 1, 0);
	std::vector<int> indices;
	std::vector<double> values;
	for (int i = 0; i < n; ++i) {
		const std::vector<long>& nbrs = gal[i].GetNbrs();
		const std::vector<double>& weights = gal[i].GetNbrWeights();
		for (size_t j = 0; j < nbrs.size(); ++j) {
			indices.push_back(static_cast<int>(nbrs[j]));
			values.push_back(j < weights.size() ? weights[j] : 1.0);
		}
		indptr[i + 1] = static_cast<int>(indices.size());
	}
	return AddWeights(indptr, indices, values, n, err);
}

bool Writer::AddWeights(const std::vector<int>& indptr,
						const std::vector<int>& indices,
						const std::vector<double>& values, int n, wxString& err)
{
	if (static_cast<int>(indptr.size()) != n + 1) {
		err = _("The spatial weights matrix does not match the number of observations.");
		return false;
	}
	// indptr goes out as 64 bit: the protocol says i8
	std::vector<long long> indptr64(indptr.begin(), indptr.end());
	{
		const wxString path = dir_ + wxFileName::GetPathSeparator() + "data.bin";
		wxFile file(path, wxFile::write_append);
		if (!file.IsOpened()) {
			err = wxString::Format(_("Could not write %s"), path);
			return false;
		}
		Array array;
		array.name = "w_indptr";
		array.dtype = kInt64;
		array.rows = n + 1;
		array.cols = 1;
		array.offset = offset_;
		array.nbytes = static_cast<long long>(indptr64.size()) * sizeof(long long);
		if (file.Write(&indptr64[0], static_cast<size_t>(array.nbytes))
			!= static_cast<size_t>(array.nbytes)) {
			err = wxString::Format(_("Could not write %s"), path);
			return false;
		}
		file.Close();
		offset_ += array.nbytes;
		arrays_.push_back(array);
	}
	have_w_ = AddIntArray("w_indices", indices, static_cast<int>(indices.size()), 1, err)
		&& AddFloatArray("w_data", values, static_cast<int>(values.size()), 1, err);
	return have_w_;
}

bool Writer::AddRegimes(const std::vector<int>& regimes, wxString& err)
{

	return AddIntArray("regime", regimes, static_cast<int>(regimes.size()), 1, err);
}

bool Writer::AddEndogenous(const std::vector<std::vector<double> >& yend, wxString& err)
{
	const int n = static_cast<int>(yend[0].size());
	std::vector<double> flat;
	for (int row = 0; row < n; ++row) {
		for (size_t col = 0; col < yend.size(); ++col) flat.push_back(yend[col][row]);
	}
	return AddFloatArray("yend", flat, n, static_cast<int>(yend.size()), err);
}

bool Writer::AddInstruments(const std::vector<std::vector<double> >& q, wxString& err)
{
	const int n = static_cast<int>(q[0].size());
	std::vector<double> flat;
	for (int row = 0; row < n; ++row) {
		for (size_t col = 0; col < q.size(); ++col) flat.push_back(q[col][row]);
	}
	return AddFloatArray("q", flat, n, static_cast<int>(q.size()), err);
}

namespace {

json_spirit::Array NamesToArray(const std::vector<wxString>& names)
{
	json_spirit::Array array;
	for (size_t i = 0; i < names.size(); ++i) {
		array.push_back(json_spirit::Value(ToUtf8(names[i])));
	}
	return array;
}

json_spirit::Value OptionsToObject(const std::map<wxString, wxString>& options)
{
	json_spirit::Object object;
	for (std::map<wxString, wxString>::const_iterator it = options.begin();
		 it != options.end(); ++it) {
		const wxString& value = it->second;
		// The registry says what type each option has, but the value here is
		// text, so json_spirit has to be told: booleans and numbers go out as
		// such, everything else - enum names, "all", model ids - as a string.
		// ToCDouble also understands exponents ("1e-07" is what epsilon's
		// default looks like), which a hand written digit check does not.
		if (value == "true" || value == "false") {
			object.push_back(json_spirit::Pair(ToUtf8(it->first), value == "true"));
		} else {
			double number = 0;
			if (!value.IsEmpty() && value.ToCDouble(&number)) {
				object.push_back(json_spirit::Pair(ToUtf8(it->first),
												   json_spirit::Value(number)));
			} else {
				object.push_back(json_spirit::Pair(ToUtf8(it->first),
												   json_spirit::Value(ToUtf8(value))));
			}
		}
	}
	return json_spirit::Value(object);
}

} // namespace

bool Writer::Write(const wxString& model,
				   const std::map<wxString, wxString>& options,
				   const wxString& y_name, const std::vector<wxString>& x_names,
				   const wxString& weights_name, const wxString& regimes_name,
				   const std::vector<wxString>& yend_names,
				   const std::vector<wxString>& q_names, wxString& err)
{
	if (!have_y_) { err = _("There is no dependent variable."); return false; }
	if (!have_x_) { err = _("There are no covariates."); return false; }

	json_spirit::Object root;
	root.push_back(json_spirit::Pair("protocol", 1));
	root.push_back(json_spirit::Pair("job_id", "geoda"));
	root.push_back(json_spirit::Pair("created", ""));
	{
		json_spirit::Object app;
		app.push_back(json_spirit::Pair("name", "GeoDa"));
		app.push_back(json_spirit::Pair("version", ""));
		app.push_back(json_spirit::Pair("platform", ToUtf8(PlatformKey())));
		root.push_back(json_spirit::Pair("app", app));
	}
	root.push_back(json_spirit::Pair("model", ToUtf8(model)));
	root.push_back(json_spirit::Pair("options", OptionsToObject(options)));

	{
		json_spirit::Object data;
		data.push_back(json_spirit::Pair("n", n_));
		{
			json_spirit::Object y;
			y.push_back(json_spirit::Pair("name", ToUtf8(y_name)));
			y.push_back(json_spirit::Pair("array", "y"));
			data.push_back(json_spirit::Pair("y", y));
		}
		{
			json_spirit::Object x;
			x.push_back(json_spirit::Pair("names", NamesToArray(x_names)));
			x.push_back(json_spirit::Pair("array", "x"));
			data.push_back(json_spirit::Pair("x", x));
		}
		data.push_back(json_spirit::Pair("constant", true));
		if (!regimes_name.IsEmpty()) {
			json_spirit::Object regimes;
			regimes.push_back(json_spirit::Pair("name", ToUtf8(regimes_name)));
			regimes.push_back(json_spirit::Pair("array", "regime"));
			data.push_back(json_spirit::Pair("regimes", regimes));
		}
		if (!yend_names.empty()) {
			json_spirit::Object yend;
			yend.push_back(json_spirit::Pair("names", NamesToArray(yend_names)));
			yend.push_back(json_spirit::Pair("array", "yend"));
			data.push_back(json_spirit::Pair("endogenous", yend));
		}
		if (!q_names.empty()) {
			json_spirit::Object q;
			q.push_back(json_spirit::Pair("names", NamesToArray(q_names)));
			q.push_back(json_spirit::Pair("array", "q"));
			data.push_back(json_spirit::Pair("instruments", q));
		}
		root.push_back(json_spirit::Pair("data", data));
	}

	if (have_w_) {
		json_spirit::Object weights;
		weights.push_back(json_spirit::Pair("format", "csr"));
		weights.push_back(json_spirit::Pair("n", n_));
		weights.push_back(json_spirit::Pair("indptr", "w_indptr"));
		weights.push_back(json_spirit::Pair("indices", "w_indices"));
		weights.push_back(json_spirit::Pair("data", "w_data"));
		weights.push_back(json_spirit::Pair("transform", "r"));
		weights.push_back(json_spirit::Pair("name", ToUtf8(weights_name)));
		root.push_back(json_spirit::Pair("weights", weights));
	}

	{
		json_spirit::Object outputs;
		outputs.push_back(json_spirit::Pair("observations", true));
		outputs.push_back(json_spirit::Pair("report", true));
		root.push_back(json_spirit::Pair("outputs", outputs));
	}

	{
		json_spirit::Object arrays;
		for (size_t i = 0; i < arrays_.size(); ++i) {
			json_spirit::Object entry;
			entry.push_back(json_spirit::Pair("dtype", ToUtf8(arrays_[i].dtype)));
			json_spirit::Array shape;
			shape.push_back(arrays_[i].rows);
			shape.push_back(arrays_[i].cols);
			entry.push_back(json_spirit::Pair("shape", shape));
			// boost::int64_t is json_spirit's own 64 bit type; a bare long long
			// is ambiguous between its int and its int64 constructors on gcc
			entry.push_back(json_spirit::Pair("offset",
											  static_cast<boost::int64_t>(arrays_[i].offset)));
			entry.push_back(json_spirit::Pair("nbytes",
											  static_cast<boost::int64_t>(arrays_[i].nbytes)));
			arrays.push_back(json_spirit::Pair(ToUtf8(arrays_[i].name), entry));
		}
		root.push_back(json_spirit::Pair("arrays", arrays));
	}

	const wxString path = dir_ + wxFileName::GetPathSeparator() + "job.json";
	// 17 digits, so that the option values survive the round trip unchanged
	const std::string text = json_spirit::write(json_spirit::Value(root),
												json_spirit::pretty_print, 17);
	wxFile file(path, wxFile::write);
	if (!file.IsOpened() || file.Write(text.c_str(), text.size()) != text.size()) {
		err = wxString::Format(_("Could not write %s"), path);
		return false;
	}
	file.Close();
	return true;
}

// ---------------------------------------------------------------------------
// reading the result
// ---------------------------------------------------------------------------

bool Result::Read(const wxString& job_dir, wxString& err)
{
	const wxString sep = wxFileName::GetPathSeparator();
	const wxString path = job_dir + sep + "result.json";
	wxFile file(path);
	if (!file.IsOpened()) {
		err = wxString::Format(_("The engine left no result behind (%s)."), path);
		return false;
	}
	const wxFileOffset length = file.Length();
	std::string text;
	text.resize(length > 0 ? static_cast<size_t>(length) : 0);
	const wxFileOffset got = text.empty() ? 0 : file.Read(&text[0], text.size());
	file.Close();
	text.resize(got > 0 ? static_cast<size_t>(got) : 0);

	json_spirit::Value value;
	if (!json_spirit::read(text, value) || value.type() != json_spirit::obj_type) {
		err = wxString::Format(_("%s is not readable."), path);
		return false;
	}
	const json_spirit::Object& root = value.get_obj();

	const wxString status = AsString(FindMember(root, "status"));
	{
		const json_spirit::Value* engine = FindMember(root, "engine");
		if (engine && engine->type() == json_spirit::obj_type) {
			spreg_version = AsString(FindMember(engine->get_obj(), "spreg"));
		}
	}
	if (status != "ok") {
		const json_spirit::Value* failure = FindMember(root, "error");
		if (failure && failure->type() == json_spirit::obj_type) {
			error_message = AsString(FindMember(failure->get_obj(), "message"));
		}
		if (error_message.IsEmpty()) error_message = _("The engine reported a failure.");
		ok = false;
		return true;                    // a reported failure is still a result
	}

	const json_spirit::Value* model = FindMember(root, "model");
	if (model && model->type() == json_spirit::obj_type) {
		const json_spirit::Object& m = model->get_obj();
		requested_model = AsString(FindMember(m, "requested"));
		class_name = AsString(FindMember(m, "class"));
		title = AsString(FindMember(m, "title"));
		n = static_cast<int>(AsDouble(FindMember(m, "n"), 0));
		k = static_cast<int>(AsDouble(FindMember(m, "k"), 0));
	}

	const json_spirit::Value* coefficients = FindMember(root, "coefficients");
	if (coefficients && coefficients->type() == json_spirit::obj_type) {
		const json_spirit::Object& c = coefficients->get_obj();
		const json_spirit::Value* names = FindMember(c, "names");
		const json_spirit::Value* roles = FindMember(c, "roles");
		if (names && names->type() == json_spirit::array_type) {
			const json_spirit::Array& array = names->get_array();
			for (size_t i = 0; i < array.size(); ++i) {
				this->names.push_back(AsString(&array[i]));
			}
		}
		if (roles && roles->type() == json_spirit::array_type) {
			const json_spirit::Array& array = roles->get_array();
			for (size_t i = 0; i < array.size(); ++i) {
				this->roles.push_back(AsString(&array[i]));
			}
		}
		const char* keys[] = { "estimate", "std_err", "z", "p" };
		std::vector<double>* targets[] = { &estimate, &std_err, &z, &p };
		for (int i = 0; i < 4; ++i) {
			const json_spirit::Value* values = FindMember(c, keys[i]);
			if (!values || values->type() != json_spirit::array_type) continue;
			const json_spirit::Array& array = values->get_array();
			for (size_t j = 0; j < array.size(); ++j) {
				targets[i]->push_back(AsMaybeDouble(array[j]));
			}
		}
	}
	for (size_t i = 0; i < names.size(); ++i) {
		if (names[i].StartsWith("_Global_") || names[i].StartsWith("_global")) {
			has_global_rows = true;
		}
	}

	const json_spirit::Value* fit = FindMember(root, "fit");
	if (fit && fit->type() == json_spirit::obj_type) {
		const json_spirit::Object& f = fit->get_obj();
		for (json_spirit::Object::const_iterator it = f.begin(); it != f.end(); ++it) {
			this->fit[wxString::FromUTF8(it->name_.c_str())] =
				AsMaybeDouble(it->value_);
		}
	}

	const json_spirit::Value* diagnostics = FindMember(root, "diagnostics");
	if (diagnostics && diagnostics->type() == json_spirit::array_type) {
		const json_spirit::Array& array = diagnostics->get_array();
		for (size_t i = 0; i < array.size(); ++i) {
			if (array[i].type() != json_spirit::obj_type) continue;
			const json_spirit::Object& d = array[i].get_obj();
			Diagnostic item;
			item.group = AsString(FindMember(d, "group"));
			item.label = AsString(FindMember(d, "label"));
			item.statistic = AsDouble(FindMember(d, "stat"), 0);
			item.df = AsDouble(FindMember(d, "df"), 0);
			item.p = AsDouble(FindMember(d, "p"), 0);
			this->diagnostics.push_back(item);
		}
	}

	const json_spirit::Value* warnings = FindMember(root, "warnings");
	if (warnings && warnings->type() == json_spirit::array_type) {
		const json_spirit::Array& array = warnings->get_array();
		for (size_t i = 0; i < array.size(); ++i) {
			this->warnings.push_back(AsString(&array[i]));
		}
	}

	// per observation numbers, out of result.bin
	const json_spirit::Value* observations = FindMember(root, "observations");
	if (observations && observations->type() == json_spirit::obj_type) {
		const json_spirit::Object& o = observations->get_obj();
		const json_spirit::Value* arrays = FindMember(o, "arrays");
		if (arrays && arrays->type() == json_spirit::obj_type) {
			const json_spirit::Object& table = arrays->get_obj();
			const wxString bin_path = job_dir + sep + "result.bin";
			wxFile bin(bin_path);
			if (bin.IsOpened()) {
				const wxFileOffset bin_length = bin.Length();
				std::vector<char> payload(
					bin_length > 0 ? static_cast<size_t>(bin_length) : 0);
				if (!payload.empty()) {
					if (bin.Read(&payload[0], payload.size()) < 0) payload.clear();
				}
				bin.Close();
				const struct { const char* key; std::vector<double>* target; } wanted[] = {
					{ "yhat", &yhat }, { "resid", &resid }, { "pred_err", &pred_err }
				};
				for (int w = 0; w < 3; ++w) {
					const json_spirit::Value* entry = FindMember(table, wanted[w].key);
					if (!entry || entry->type() != json_spirit::obj_type) continue;
					const json_spirit::Object& e = entry->get_obj();
					const long long offset =
						static_cast<long long>(AsDouble(FindMember(e, "offset"), 0));
					const long long nbytes =
						static_cast<long long>(AsDouble(FindMember(e, "nbytes"), 0));
					if (offset < 0 || nbytes <= 0
						|| offset + nbytes > static_cast<long long>(payload.size())) {
						continue;
					}
					const size_t count = static_cast<size_t>(nbytes / sizeof(double));
					// memcpy, not a cast: the payload is a character buffer and
					// has no reason to be aligned for doubles
					std::vector<double> values(count);
					std::memcpy(&values[0], &payload[static_cast<size_t>(offset)],
								count * sizeof(double));
					*wanted[w].target = values;
				}
			}
		}
	}

	ok = true;
	return true;
}

// ---------------------------------------------------------------------------
// the model registry
// ---------------------------------------------------------------------------

wxString ModelListCachePath(const wxString& engine_dir)
{
	return engine_dir + wxFileName::GetPathSeparator() + "model_list.json";
}

namespace {

/** Turns the solver's registry dump into the list the dialog offers. */
bool ParseModels(const std::string& text, std::vector<ModelOption>& models,
				 wxString& err)
{
	models.clear();
	json_spirit::Value value;
	if (!json_spirit::read(text, value) || value.type() != json_spirit::obj_type) {
		err = _("The engine's model list could not be read.");
		return false;
	}
	const json_spirit::Value* list = FindMember(value.get_obj(), "models");
	if (!list) {
		err = _("The engine's model list is empty.");
		return false;
	}

	const json_spirit::Array& array = list->get_array();
	for (size_t i = 0; i < array.size(); ++i) {
		if (array[i].type() != json_spirit::obj_type) continue;
		const json_spirit::Object& m = array[i].get_obj();
		ModelOption option;
		option.id = AsString(FindMember(m, "id"));
		option.label = AsString(FindMember(m, "label"));
		option.family = AsString(FindMember(m, "family"));
		const json_spirit::Value* requires = FindMember(m, "requires");
		if (requires && requires->type() == json_spirit::obj_type) {
			const json_spirit::Object& r = requires->get_obj();
			option.needs_weights = AsBool(FindMember(r, "weights"), true);
			option.needs_regimes = AsBool(FindMember(r, "regimes"), false);
			option.needs_endog = AsBool(FindMember(r, "endog"), false);
			option.needs_instruments = AsBool(FindMember(r, "instruments"), false);
			option.needs_coords = AsBool(FindMember(r, "coords"), false);
		}
		const json_spirit::Value* options = FindMember(m, "options");
		if (options && options->type() == json_spirit::obj_type) {
			const json_spirit::Object& o = options->get_obj();
			for (json_spirit::Object::const_iterator it = o.begin();
				 it != o.end(); ++it) {
				const json_spirit::Object& spec = it->value_.get_obj();
				const json_spirit::Value* def = FindMember(spec, "default");
				wxString value_text;
				if (def) {
					if (def->type() == json_spirit::bool_type) {
						value_text = def->get_bool() ? "true" : "false";
					} else if (def->type() == json_spirit::str_type) {
						value_text = wxString::FromUTF8(def->get_str().c_str());
					} else {
						value_text << AsDouble(def, 0);
					}
				}
				option.defaults[wxString::FromUTF8(it->name_.c_str())] = value_text;
			}
		}
		if (!option.id.IsEmpty()) models.push_back(option);
	}
	return true;
}

} // namespace

bool ListModels(const wxString& engine_dir, std::vector<ModelOption>& models,
				wxString& err, bool allow_cache)
{
	models.clear();

	if (allow_cache) {
		wxFile cache(ModelListCachePath(engine_dir));
		if (cache.IsOpened()) {
			const wxFileOffset length = cache.Length();
			std::string text;
			text.resize(length > 0 ? static_cast<size_t>(length) : 0);
			const wxFileOffset got = text.empty() ? 0 : cache.Read(&text[0], text.size());
			cache.Close();
			text.resize(got > 0 ? static_cast<size_t>(got) : 0);
			if (!text.empty() && ParseModels(text, models, err) && !models.empty()) {
				return true;
			}
			err.Clear();               // an unreadable cache is not an error: ask
		}
	}

	wxString text;
	if (!RunSolverCommand(engine_dir, "--list-models", text, err, 120)) return false;
	if (!ParseModels(text.utf8_string(), models, err)) return false;

	// keep it for next time; a failure here is not worth bothering anyone about
	wxFile cache(ModelListCachePath(engine_dir), wxFile::write);
	if (cache.IsOpened()) {
		const std::string utf8 = text.utf8_string();
		cache.Write(utf8.c_str(), utf8.size());
		cache.Close();
	}
	return true;
}

} // namespace SpregJob
