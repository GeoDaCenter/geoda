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

#ifndef __GEODA_CENTER_CAT_CLASSIFICATION_H__
#define __GEODA_CENTER_CAT_CLASSIFICATION_H__

#include <vector>
#include <wx/brush.h>
#include <wx/colour.h>
#include <wx/pen.h>
#include <wx/string.h>
#include "../GenUtils.h"

struct CatClassifDef;
struct Category;
struct CategoryVec;
struct CatClassifData;
class TableInterface;

namespace CatClassification {
	
	const int max_num_categories = 20;
	
	enum CatClassifType { no_theme, hinge_15, hinge_30, quantile, percentile,
		stddev, excess_risk_theme, unique_values, natural_breaks,
		equal_intervals, lisa_categories, lisa_significance,
		getis_ord_categories, getis_ord_significance,
        local_geary_categories, local_geary_significance,custom };
	
	/** When CatClassifType != custom, BreakValsType is assumed to
	  be by_cat_classif_type.  Otherwise, if CatClassifType == custom,
	  BreakValsType cannot be by_cat_classif_type */
	enum BreakValsType { by_cat_classif_type,
		no_theme_break_vals, hinge_15_break_vals, hinge_30_break_vals,
		quantile_break_vals, percentile_break_vals, stddev_break_vals, 
		unique_values_break_vals, natural_breaks_break_vals,
		equal_intervals_break_vals, custom_break_vals };
	
	enum ColorScheme { sequential_color_scheme, diverging_color_scheme, qualitative_color_scheme, custom_color_scheme, unique_color_scheme };
	
	
	void CatLabelsFromBreaks(const std::vector<double>& breaks,
                             std::vector<wxString>& cat_labels,
							 const CatClassifType theme,
                             bool useScientifcNotation=false);
	
	void SetBreakPoints(std::vector<double>& breaks,
						std::vector<wxString>& cat_labels,
						const Gda::dbl_int_pair_vec_type& var,
                        const std::vector<bool>& var_undef,
						const CatClassifType theme, int num_cats,
                        bool useScientificNotation=false);
	
    void PopulateCatClassifData(const CatClassifDef& cat_def,
                                const std::vector<Gda::dbl_int_pair_vec_type>& var,
                                const std::vector<std::vector<bool> >& var_undef,
                                CatClassifData& cat_data, std::vector<bool>& cats_valid,
                                std::vector<wxString>& cats_error_message,
                                bool useSciNotation=false,
                                bool useUndefinedCategory=true);
		
	bool CorrectCatClassifFromTable(CatClassifDef& cc,
									TableInterface* table_int);
	
	void FindNaturalBreaks(int num_cats,
						   const Gda::dbl_int_pair_vec_type& var,
                           const std::vector<bool>& var_undef,
						   std::vector<double>& nat_breaks);
    
    void SetNaturalBreaksCats(int num_cats,
                              const std::vector<Gda::dbl_int_pair_vec_type>& var,
                              const std::vector<std::vector<bool> >& var_undef,
                              CatClassifData& cat_data, std::vector<bool>& cats_valid,
                              ColorScheme coltype=CatClassification::sequential_color_scheme);
	
	ColorScheme GetColSchmForType(CatClassifType theme_type);
	
	wxString CatClassifTypeToString(CatClassifType theme_type);
	
	void PickColorSet(std::vector<wxColour>& color_vec,
					  ColorScheme coltype, int num_color, bool reversed=false);
	
	void ChangeNumCats(int num_cats, CatClassifDef& cc);
	int ChangeBreakValue(int brk, double new_val, CatClassifDef& cc);
	void ChangeUnifDistMin(double new_unif_dist_min, CatClassifDef& cc);
	void ChangeUnifDistMax(double new_unif_dist_max, CatClassifDef& cc);
	void ApplyColorScheme(ColorScheme scheme, CatClassifDef& cc);
	void PrintCatClassifDef(const CatClassifDef& cc, wxString& str);
	wxString ColorToString(const wxColour& c);
	BreakValsType CatClassifTypeToBreakValsType(CatClassifType cct);
	CatClassifType BreakValsTypeToCatClassifType(BreakValsType bvt);
}

/**
 This is intended to represent both pre-defined category classifications 
 such as Hinge=1.5 and Standard Deviation categories, as well as user-defined
 categories.  When cat_classif_type != custom, breaks, names, colors,
 color_scheme, title and unique_id are ignored.  num_cats is only valid when
 the cat_classif_type is one of quantile, natural_breaks, equal_intervals
 or custom.
 
 colors is only valid when color_scheme is set to custom_color_scheme.
 
 unique_id is assigned only for custom cat_classif_type, and this is assigned
 at run-time (not when saved to disk).
 */
struct CatClassifDef {
	CatClassifDef();
	CatClassifDef& operator=(const CatClassifDef& s);
	bool operator==(const CatClassifDef& s) const;
	bool operator!=(const CatClassifDef& s) const;
	// If cat_classif_type != custom, then most fields below other
	// than num_cats are ignored.
	CatClassification::CatClassifType cat_classif_type;
	// breaks_type is only relevant when cat_classif_type == custom.
	// Cannot have breaks_type == by_cat_classif_type combined
	// with cat_classif_type == custom
	CatClassification::BreakValsType break_vals_type;
	int num_cats; // limit of 10
	bool automatic_labels; // update category labels automatically if true
	std::vector<double> breaks; // size: num_cats-1
	std::vector<wxString> names; // size: num_cats
	std::vector<wxColour> colors; // size: num_cats
	CatClassification::ColorScheme color_scheme; // one of four choices
	wxString title;
	wxString assoc_db_fld_name; // if empty, then uniform dist
	double uniform_dist_min; // used for uniform dist
	double uniform_dist_max; // used for uniform dist

	wxString ToStr() const;
};

struct Category {
	wxBrush brush;
	wxPen pen; // always derived from brush
	wxString label;
    // used for a special case in percentile like legend
    // e.g. 1% - 10% (34) 0.1 - 0.9
	wxString label_ext;
	int count;
	std::vector<int> ids;
	double min_val;
	double max_val;
	bool min_max_defined;
};

struct CategoryVec {
	std::vector<Category> cat_vec;
	std::vector<int> id_to_cat;
};

struct CatClassifData {
	std::vector<CategoryVec> categories;
	int curr_canvas_tm_step;
	int canvas_tm_steps; // total # canvas time steps
	
	// Note: Canvas Time Steps might not correspond to global time steps.
	// For views that display data from two or more variables such as
	// Scatter Plot, there may be fewer canvas time steps than global time
	// steps.
    void AppendUndefCategory(int time, int count);
	void CreateEmptyCategories(int num_canvas_tms, int num_obs);
	void CreateCategoriesAllCanvasTms(int num_cats, int num_canvas_tms,
									  int num_obs);
	void CreateCategoriesAtCanvasTm(int num_cats, int canvas_tm);
	void SetCategoryBrushesAllCanvasTms(std::vector<wxColour> colors);
	void SetCategoryBrushesAllCanvasTms(CatClassification::ColorScheme coltype,
										int ncolor, bool reversed);
	void SetCategoryBrushesAtCanvasTm(CatClassification::ColorScheme coltype,
									  int ncolor, bool reversed, int canvas_tm);
	int GetNumCategories(int canvas_tm);
	int GetNumObsInCategory(int canvas_tm, int cat);
	std::vector<int>& GetIdsRef(int canvas_tm, int cat);
	void SetCategoryColor(int canvas_tm, int cat, wxColour color);
	wxColour GetCategoryColor(int canvas_tm, int cat);
	wxBrush GetCategoryBrush(int canvas_tm, int cat);
	wxPen GetCategoryPen(int canvas_tm, int cat);
	void AppendIdToCategory(int canvas_tm, int cat, int id);
	void ClearAllCategoryIds();
	wxString GetCatLblWithCnt(int canvas_tm, int cat);
	wxString GetCategoryLabel(int canvas_tm, int cat);
	void SetCategoryLabel(int canvas_tm, int cat, const wxString& label);
	void SetCategoryLabelExt(int canvas_tm, int cat, const wxString& label);
	int GetCategoryCount(int canvas_tm, int cat);
	void SetCategoryCount(int canvas_tm, int cat, int count);
	void ResetCategoryMinMax(int canvas_tm, int cat);
	void ResetAllCategoryMinMax(int canvas_tm);
	void ResetAllCategoryMinMax();
	void UpdateCategoryMinMax(int canvas_tm, int cat, const double& val);
	void SetCategoryMinMax(int canvas_tm, int cat,
						   const double& min, const double& max);
	bool IsMinMaxDefined(int canvas_tm, int cat);
	bool IsCategoryEmpty(int canvas_tm, int cat);
	double GetCategoryMin(int canvas_tm, int cat);
	double GetCategoryMax(int canvas_tm, int cat);
	
	bool HasBreakVal(int canvas_tm, int cat_brk); // from 0 to num_cats-1
	double GetBreakVal(int canvas_tm, int cat_brk); // from 0 to num_cats-1
	
	int GetCurrentCanvasTmStep();
	void SetCurrentCanvasTmStep(int canvas_tm);
	int GetCanvasTmSteps();
    
    void ExchangeLabels(int from, int to);
};

#endif
