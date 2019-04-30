


#ifndef __GEODA_CENTER_GDALISTBOX_H__
#define __GEODA_CENTER_GDALISTBOX_H__

#include <vector>
#include <string>
#include <algorithm>
#include <wx/listbox.h>

#include "../DataViewer/TableInterface.h"


class GdaListBox : public wxListBox
{
    int data_type;
    wxArrayString var_items;
    std::map<wxString, int> name_to_nm;
    std::map<wxString, int> name_to_tm_id;
    TableInterface* table_int;

public:
    GdaListBox(){}
    GdaListBox(wxWindow *parent,
               wxWindowID id,
               const wxPoint &pos=wxDefaultPosition,
               const wxSize &size=wxDefaultSize,
               int n=0,
               const wxString choices[]=NULL,
               long style=0,
               const wxValidator &validator=wxDefaultValidator,
               const wxString &name=wxListBoxNameStr);
    virtual ~GdaListBox();

    virtual void Clear();
    
    // customized functions:
    // style for filling content in wxListBox
    enum DataType {
        SHOW_INTEGER = 1, //0b00000001
        SHOW_STRING = 2, //0b00000010
        SHOW_FLOAT = 4
    };

    // init content in wxListBox
    void GdaInitContent(TableInterface* table_int, int data_type);

    // get selections and related values from wxListBox
    void GdaGetSelections(std::vector<int>& col_ids, std::vector<int>& tm_ids);

    // get all items and related values from wxListBox
    void GdaGetAllItems(std::vector<int>& col_ids, std::vector<int>& tm_ids);

    // append item with column idx and time idx to wxListBox
    void GdaAppend(const wxString& item, const int& col_id, const int& tm_id);

    // get selected string with column idx and time idx
    wxString GdaGetString(const int& i, int& col_id, int& tm_id);

    // delete selected item and column idx/time idx
    void Delete(unsigned int pos);

    DECLARE_DYNAMIC_CLASS(GdaListBox)
};

#endif
