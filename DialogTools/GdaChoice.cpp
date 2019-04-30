
#include "GdaChoice.h"

IMPLEMENT_DYNAMIC_CLASS(GdaChoice, wxChoice)

GdaChoice::GdaChoice(wxWindow *parent, wxWindowID id, const wxPoint &pos,
                       const wxSize &size, int n, const wxString choices[],
                       long style, const wxValidator &validator,
                       const wxString &name)
: wxChoice(parent, id, pos, size, n, choices, style, validator, name)
{
    table_int = NULL;
}

GdaChoice::~GdaChoice()
{
}

void GdaChoice::Clear()
{
    wxChoice::Clear();
    var_items.Clear();
    name_to_nm.clear();
    name_to_tm_id.clear();
}

void GdaChoice::GdaInitContent(TableInterface *table_int, int data_type) {
    this->table_int = table_int;
    std::vector<int> col_id_map;
    if (data_type == (SHOW_INTEGER | SHOW_STRING | SHOW_FLOAT)) {
        table_int->FillColIdMap(col_id_map);
    } else if (data_type == (SHOW_INTEGER | SHOW_FLOAT)) {
        table_int->FillNumericColIdMap(col_id_map);
    } else if (data_type == SHOW_INTEGER) {
        table_int->FillIntegerColIdMap(col_id_map);
    } else if (data_type == SHOW_STRING) {
        table_int->FillStringColIdMap(col_id_map);
    } else if (data_type == (SHOW_INTEGER | SHOW_STRING)) {
        table_int->FillStringAndIntegerColIdMap(col_id_map);
    }

    for (size_t i=0; i<col_id_map.size(); i++) {
        int id = col_id_map[i];
        wxString name = table_int->GetColName(id);
        if (table_int->IsColTimeVariant(id)) {
            for (size_t t=0; t<table_int->GetColTimeSteps(id); t++) {
                wxString nm = name;
                nm << " (" << table_int->GetTimeString(t) << ")";
                name_to_nm[nm] = id;
                name_to_tm_id[nm] = t;
                var_items.Add(nm);
            }
        } else {
            name_to_nm[name] = id;
            name_to_tm_id[name] = 0;
            var_items.Add(name);
        }
    }
    for (size_t i=0; i<var_items.size(); ++i) {
        this->Append(var_items[i]);
    }
}

wxString GdaChoice::GdaGetSelection(int& col_id, int& tm_id)
{
    if (table_int == NULL) return wxEmptyString;
    if (var_items.IsEmpty()) return wxEmptyString;

    int sel_id = this->GetSelection();
    if (sel_id == wxNOT_FOUND) return wxEmptyString;

    if (sel_id >= var_items.size()) return wxEmptyString;

    wxString sel_str = var_items[sel_id];
    if (name_to_nm.find(sel_str) == name_to_nm.end() ||
        name_to_tm_id.find(sel_str) == name_to_tm_id.end())
        return wxEmptyString;
    col_id = name_to_nm[sel_str];
    tm_id = name_to_tm_id[sel_str];

    return table_int->GetColName(col_id, tm_id);
}

GdaConst::FieldType GdaChoice::GdaGetSelectionFieldType(unsigned int pos)
{
    GdaConst::FieldType ftype = GdaConst::unknown_type;
    if (table_int == NULL) return ftype;
    if (var_items.IsEmpty()) return ftype;

    int sel_id = this->GetSelection();
    if (sel_id == wxNOT_FOUND) return ftype;

    if (sel_id >= var_items.size()) return ftype;

    wxString sel_str = var_items[sel_id];
    if (name_to_nm.find(sel_str) == name_to_nm.end() ||
        name_to_tm_id.find(sel_str) == name_to_tm_id.end())
        return ftype;
    int col_id = name_to_nm[sel_str];
    int tm_id = name_to_tm_id[sel_str];

    return table_int->GetColType(col_id, tm_id);
}

bool GdaChoice::GdaCheckDuplicate(unsigned int pos)
{
    if (table_int == NULL) return true;
    if (var_items.IsEmpty()) return true;
    if (pos >= var_items.size()) return true;

    wxString sel_str = var_items[pos];
    if (name_to_nm.find(sel_str) == name_to_nm.end() ||
        name_to_tm_id.find(sel_str) == name_to_tm_id.end())
        return true;

    int col_id = name_to_nm[sel_str];
    int tm_id = name_to_tm_id[sel_str];
    std::vector<wxString> values;
    table_int->GetColData(col_id, tm_id, values);

    std::map<wxString, bool> dup_dict;
    for (size_t i=0; i<values.size(); ++i) {
        if (dup_dict.find(values[i]) != dup_dict.end()) {
            return true;
        }
        dup_dict[values[i]] = true;
    }
    return false;
}

std::map<wxString, std::vector<int> > GdaChoice::GdaGetUniqueValues(unsigned int pos)
{
    std::map<wxString, std::vector<int> > results;

    if (table_int == NULL) return results;
    if (var_items.IsEmpty()) return results;
    if (pos >= var_items.size()) return results;

    wxString sel_str = var_items[pos];
    if (name_to_nm.find(sel_str) == name_to_nm.end() ||
        name_to_tm_id.find(sel_str) == name_to_tm_id.end())
        return results;

    int col_id = name_to_nm[sel_str];
    int tm_id = name_to_tm_id[sel_str];
    std::vector<wxString> values;
    table_int->GetColData(col_id, tm_id, values);

    for (size_t i=0; i<values.size(); ++i) {
        results[values[i]].push_back(i);
    }
    return results;
}


