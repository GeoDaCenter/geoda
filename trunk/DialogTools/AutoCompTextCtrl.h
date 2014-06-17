


#ifndef __GEODA_CENTER_AUTOCOMPLETE_TEXT_CTRL_H__
#define __GEODA_CENTER_AUTOCOMPLETE_TEXT_CTRL_H__

#include <vector>
#include <string>
#include <algorithm>
#include <wx/textctrl.h>

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
	void SetAutoList(std::vector<std::string> &autoList) { 
		if (autoList.size() == 0) return;
		m_pList = autoList; 
		std::sort(m_pList.begin(), m_pList.end());
	}
	
private:
	bool m_bDoSelectAll;
	bool m_bIgnoreNextTextEdit;
	std::vector<std::string> m_pList;
	
	DECLARE_EVENT_TABLE()
};

#endif
