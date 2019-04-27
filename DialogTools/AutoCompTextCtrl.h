


#ifndef __GEODA_CENTER_AUTOCOMPLETE_TEXT_CTRL_H__
#define __GEODA_CENTER_AUTOCOMPLETE_TEXT_CTRL_H__

#include <vector>
#include <string>
#include <algorithm>
#include <wx/textctrl.h>
#include <wx/listbox.h>

#include "../DataViewer/TableInterface.h"

class AutoTextCtrl: public wxTextCtrl {
	DECLARE_DYNAMIC_CLASS(AutoTextCtrl) 
	
public:
	AutoTextCtrl(){}
	AutoTextCtrl(wxWindow* parent, wxWindowID id, 
			   const wxString& value = "", 
			   const wxPoint& pos = wxDefaultPosition, 
			   const wxSize& size = wxDefaultSize, 
			   long style = 0, 
			   const wxValidator& validator = wxDefaultValidator, 
			   const wxString& name = wxTextCtrlNameStr);
	
	void TextEntered( wxCommandEvent &event );
	void DoIdle( wxIdleEvent &event );
	void DoKeyDown( wxKeyEvent &event );
	void SetAutoList(std::vector<wxString> &autoList) {
		if (autoList.size() == 0) return;
		m_pList = autoList; 
		std::sort(m_pList.begin(), m_pList.end());
	}
	
private:
	bool m_bDoSelectAll;
	bool m_bIgnoreNextTextEdit;
	std::vector<wxString> m_pList;
	
	DECLARE_EVENT_TABLE()
};

class GdaListBox : public wxListBox
{
    int data_type;
    wxArrayString var_items;
    std::map<wxString, int> name_to_nm;
    std::map<wxString, int> name_to_tm_id;

public:
    enum DataType {
        SHOW_ALL = 0,
        SHOW_INTEGER = 1, //0b00000001
        SHOW_STRING = 2, //0b00000010
        SHOW_NUMERIC = 4,
        SHOW_STRING_INTEGER = 8
    };
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

    void InitContent(TableInterface* table_int, int data_type);

    DECLARE_DYNAMIC_CLASS(GdaListBox)
};

#endif
