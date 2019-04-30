


#ifndef __GEODA_CENTER_GDACHOICE_H__
#define __GEODA_CENTER_GDACHOICE_H__

#include <vector>
#include <string>
#include <algorithm>
#include <wx/choice.h>

#include "../DataViewer/TableInterface.h"


class GdaChoice : public wxChoice
{
    int data_type;
    wxArrayString var_items;
    std::map<wxString, int> name_to_nm;
    std::map<wxString, int> name_to_tm_id;
    TableInterface* table_int;

public:
    GdaChoice(){}
    GdaChoice(wxWindow *parent,
               wxWindowID id,
               const wxPoint &pos=wxDefaultPosition,
               const wxSize &size=wxDefaultSize,
               int n=0,
               const wxString choices[]=NULL,
               long style=0,
               const wxValidator &validator=wxDefaultValidator,
               const wxString &name=wxChoiceNameStr);
    virtual ~GdaChoice();

    // customized functions:
    // style for filling content in wxListBox
    enum DataType {
        SHOW_INTEGER = 1, //0b00000001
        SHOW_STRING = 2, //0b00000010
        SHOW_FLOAT = 4
    };

    virtual void Clear();

    // init content in wxListBox
    void GdaInitContent(TableInterface* table_int, int data_type);

    // get selections and related values from wxListBox
    wxString GdaGetSelection(int& col_id, int& tm_id);

    // check if selected has unique values
    bool GdaCheckDuplicate(unsigned int pos);

    std::map<wxString, std::vector<int> > GdaGetUniqueValues(unsigned int pos);

    GdaConst::FieldType GdaGetSelectionFieldType(unsigned int pos);

    DECLARE_DYNAMIC_CLASS(GdaChoice)
};

#endif
