#include "wxTranslationHelper.h"
#include <wx/dir.h>
#include <wx/config.h>
#include <wx/fileconf.h>
#include <wx/filename.h>

wxTranslationHelper::wxTranslationHelper(wxApp & app, 
										 const wxString & search_path,
										 bool use_native_config)
: m_App(app), m_SearchPath(search_path), 
m_ConfigPath(wxEmptyString), m_Locale(NULL), m_UseNativeConfig(use_native_config)
{	
	if(search_path.IsEmpty()) {
		m_SearchPath = wxPathOnly(m_App.argv[0]);
	}
}

wxTranslationHelper::~wxTranslationHelper()
{
	Save();
	if(m_Locale)
	{
		wxDELETE(m_Locale);
	}
}

wxLocale * wxTranslationHelper::GetLocale()
{
	return m_Locale;
}

const wxString & wxTranslationHelper::GetSearchPath()
{
	return m_SearchPath;
}

void wxTranslationHelper::SetSearchPath(wxString & value)
{
	m_SearchPath = value;
	if(m_SearchPath.IsEmpty()) {
		m_SearchPath = wxPathOnly(m_App.argv[0]);
	}
}

const wxString & wxTranslationHelper::GetConfigPath()
{
	return m_ConfigPath;
}

void wxTranslationHelper::SetConfigPath(wxString & value)
{
	m_ConfigPath = value;
}

bool wxTranslationHelper::Load()
{
	wxConfigBase * config;
	if(m_UseNativeConfig) {
		config = new wxConfig(m_App.GetAppName());
	} else {
		config = new wxFileConfig(m_App.GetAppName(), wxEmptyString, m_ConfigPath);
	}
	long language;
	config->SetPath("Translation");
	if(!config->Read("Language", &language, wxLANGUAGE_UNKNOWN)) {
		language = wxLANGUAGE_UNKNOWN;
	}
	delete config;
    
	if(language == wxLANGUAGE_UNKNOWN) {
		return false;
	}
    
	wxArrayString names;
	wxArrayLong identifiers;
	GetInstalledLanguages(names, identifiers);
	
    //AskUserForLanguage(names, identifiers);
    
    for(size_t i = 0; i < identifiers.Count(); i++) {
		if(identifiers[i] == language) {
            if(m_Locale) {
                wxDELETE(m_Locale);
            }
			m_Locale = new wxLocale;
			m_Locale->Init(identifiers[i]);
			m_Locale->AddCatalogLookupPathPrefix(m_SearchPath);
			m_Locale->AddCatalog(m_App.GetAppName());
			return true;
		}
	}	
	return false;
}

void wxTranslationHelper::Save(bool bReset)
{
	wxConfigBase * config;
	if(m_UseNativeConfig) {
		config = new wxConfig(m_App.GetAppName());
	} else {
		config = new wxFileConfig(m_App.GetAppName(), wxEmptyString, m_ConfigPath);
	}
	long language = wxLANGUAGE_UNKNOWN;
	if(!bReset) {
		if(m_Locale) {
			language = m_Locale->GetLanguage();
		}
	}	
	config->DeleteEntry("Translation");
	config->SetPath("Translation");
	config->Write("Language", language);
	config->Flush();
	delete config;
}

void wxTranslationHelper::GetInstalledLanguages(wxArrayString & names, 
												wxArrayLong & identifiers)
{
	names.Clear();
	identifiers.Clear();	
	
	const wxLanguageInfo * langinfo;	
	wxString name = wxLocale::GetLanguageName(wxLANGUAGE_DEFAULT);

	if(!name.IsEmpty()) {
		names.Add("Default");
		identifiers.Add(wxLANGUAGE_DEFAULT);		
	}
	
	if(!wxDir::Exists(m_SearchPath)) {
		wxLogTrace(".mo", "Directory %s DOES NOT EXIST !!!", m_SearchPath.GetData());
		return;
	}
    
    wxString filespec = "*";
    
    wxString appname = m_App.GetAppName();
    wxString filename;
	wxDir dir(m_SearchPath);
    bool cont = dir.GetFirst(&filename, filespec, wxDIR_DIRS);
    
    while ( cont ) {
		langinfo = wxLocale::FindLanguageInfo(filename);
		
		if(langinfo != NULL) {
            // search for mo file
            wxString mo_file = dir.GetName() + wxFileName::GetPathSeparator() + filename + wxFileName::GetPathSeparator() + appname + ".mo";
			if(wxFileExists(mo_file)) {
				names.Add(langinfo->Description);
				identifiers.Add(langinfo->Language);
			}
		}
        
        cont = dir.GetNext(&filename);
	}
}

bool wxTranslationHelper::AskUserForLanguage(wxArrayString & names, 
											 wxArrayLong & identifiers)
{
	wxCHECK_MSG(names.Count() == identifiers.Count(), false, 
		"Array of language names and identifiers should have the same size.");
    
	long index = wxGetSingleChoiceIndex(_("Select the language"), _("Language"), names);
    
	if(index != -1) {
		if(m_Locale) {
			wxDELETE(m_Locale);
		}
		m_Locale = new wxLocale;
		m_Locale->Init(identifiers[index]);
		m_Locale->AddCatalogLookupPathPrefix(m_SearchPath);
		m_Locale->AddCatalog(m_App.GetAppName());
        //Catalog Name = m_App.GetAppName().GetData());
        Save();
		return true;
	}
	return false;
}
