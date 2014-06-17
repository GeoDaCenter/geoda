
#include "AutoCompTextCtrl.h"

IMPLEMENT_DYNAMIC_CLASS(AutoTextCtrl, wxTextCtrl) 

BEGIN_EVENT_TABLE(AutoTextCtrl, wxTextCtrl)
EVT_TEXT(wxID_ANY, AutoTextCtrl::TextEntered) 
EVT_IDLE(AutoTextCtrl::DoIdle)
EVT_KEY_DOWN(AutoTextCtrl::DoKeyDown) 
END_EVENT_TABLE()


AutoTextCtrl:: AutoTextCtrl (wxWindow* parent, wxWindowID id, 
							 const wxString& value, 
						 const wxPoint& pos, const wxSize& size, 
						 long style, const wxValidator& validator, 
						 const wxString& name)
: wxTextCtrl(parent, id, value, pos, size, style, validator, name)
{
	m_bIgnoreNextTextEdit = false;
}


void 
AutoTextCtrl::DoIdle( wxIdleEvent &event )
{
	m_bIgnoreNextTextEdit = false;
	event.Skip();
}


void 
AutoTextCtrl::TextEntered( wxCommandEvent &event )
{
	std::string sVal;
	std::string szListName;
	long iPoint;
	
	if(m_bIgnoreNextTextEdit) {
		m_bIgnoreNextTextEdit = false;
		event.Skip();
		return;
	}
	
	iPoint = GetInsertionPoint();
	
	// First, if the cursor is at the end of the text
	if(GetInsertionPoint() != GetLastPosition()) {
		event.Skip();
		return;
	}
	
	// Second, if the text so far is the prefix of a class in the catalog
	sVal = GetValue();
	
	// Get the first matching value from the sorted list
	for (size_t i=0; i < m_pList.size(); i++) {
		if (m_pList[i].find( sVal ) ==0 ){
			szListName = m_pList[i];
			break;
		}
	}
	
	if (!szListName.empty() && sVal.length() < szListName.length()) {
	//if(szListName && strncmp(szVal, szClassName, strlen(szVal)) == 0 &&
    //   strlen(szVal) < strlen(szClassName)) {
		// Third, write the entire name of the first matching class 
		//    into the text string
		SetValue(wxString(szListName));
		
		// Fourth, select the text from the current cursor position to 
		//    the end of the text
		SetSelection(iPoint, -1);
	}
}


void 
AutoTextCtrl::DoKeyDown( wxKeyEvent &event )
{
	int kc = event.GetKeyCode();
	
	// This is necessary to ignore edits after a BACKSPACE or DELETE
	//   key (points when auto-complete should be momentarily turned off)
	if(kc == WXK_DELETE  ||  kc == WXK_NUMPAD_DELETE || kc == WXK_BACK || 
	   kc == WXK_CLEAR)
		m_bIgnoreNextTextEdit = true;
	
	event.Skip();
}

