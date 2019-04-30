
#include "GdaListBox.h"

IMPLEMENT_DYNAMIC_CLASS(GdaListBox, wxListBox)

GdaListBox::GdaListBox(wxWindow *parent, wxWindowID id, const wxPoint &pos,
                       const wxSize &size, int n, const wxString choices[],
                       long style, const wxValidator &validator,
                       const wxString &name)
: wxListBox(parent, id, pos, size, n, choices, style, validator, name)
{
    table_int = NULL;
}

GdaListBox::~GdaListBox()
{
}

void GdaListBox::Clear()
{
    wxListBox::Clear();
    var_items.Clear();
    name_to_nm.clear();
    name_to_tm_id.clear();
}

void GdaListBox::GdaInitContent(TableInterface *table_int, int data_type) {
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
    if (!var_items.IsEmpty()) {
        this->InsertItems(var_items, 0);
    }
}

void GdaListBox::GdaGetSelections(std::vector<int>& col_ids,
                                  std::vector<int>& tm_ids)
{
    if (var_items.IsEmpty()) return;

    wxArrayInt sel_ids;
    int n_sel = this->GetSelections(sel_ids);
    if (n_sel == 0) return;

    for (size_t i=0; i<sel_ids.size(); ++i) {
        int sel_id = sel_ids[i];
        if (sel_id < 0 || sel_id >= var_items.size())
            continue;
        wxString sel_str = var_items[sel_id];
        if (name_to_nm.find(sel_str) == name_to_nm.end() ||
            name_to_tm_id.find(sel_str) == name_to_tm_id.end())
            continue;
        int col_idx = name_to_nm[sel_str];
        int tm_idx = name_to_tm_id[sel_str];
        col_ids.push_back(col_idx);
        tm_ids.push_back(tm_idx);
    }
}

void GdaListBox::GdaGetAllItems(std::vector<int>& col_ids,
                                std::vector<int>& tm_ids)
{
    if (var_items.IsEmpty()) return;

    for (size_t i=0; i<var_items.size(); ++i) {
        wxString sel_str = var_items[i];
        if (name_to_nm.find(sel_str) == name_to_nm.end() ||
            name_to_tm_id.find(sel_str) == name_to_tm_id.end())
            continue;
        int col_idx = name_to_nm[sel_str];
        int tm_idx = name_to_tm_id[sel_str];
        col_ids.push_back(col_idx);
        tm_ids.push_back(tm_idx);
    }
}

void GdaListBox::GdaAppend(const wxString& item, const int& col_id,
                           const int& tm_id)
{
    for (size_t i=0; i<var_items.size(); ++i) {
        if (var_items[i].CmpNoCase(item) == 0) {
            return;
        }
    }
    var_items.push_back(item);
    name_to_nm[item] = col_id;
    name_to_tm_id[item] = tm_id;
    this->Append(item);
}

wxString GdaListBox::GdaGetString(const int& i, int& col_id, int& tm_id)
{
    if (i <0 || i >= var_items.size()) return wxEmptyString;

    wxString sel_str = var_items[i];

    if (name_to_nm.find(sel_str) == name_to_nm.end() ||
        name_to_tm_id.find(sel_str) == name_to_tm_id.end())
        return wxEmptyString;

    col_id = name_to_nm[sel_str];
    tm_id = name_to_tm_id[sel_str];

    return sel_str;
}

void GdaListBox::Delete(unsigned int i)
{
    if (i >= var_items.size()) return;

    wxListBox::Delete(i);

    wxString sel_str = var_items[i];
    if (name_to_nm.find(sel_str) != name_to_nm.end()) {
        name_to_nm.erase(name_to_nm.find(sel_str));
    }
    if (name_to_tm_id.find(sel_str) != name_to_tm_id.end()) {
        name_to_tm_id.erase(name_to_tm_id.find(sel_str));
    }
    var_items.erase(var_items.begin() + i);
}
