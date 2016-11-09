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



#include <fstream>
#include <wx/wx.h>
#include <wx/filedlg.h>
#include <wx/dir.h>
#include <wx/filefn.h> 
#include <wx/msgdlg.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
//#include <wx/utils.h>

#include <curl/curl.h>

#include "../Explore/LisaMapNewView.h"
#include "../Explore/HistogramView.h"
#include "../Explore/MapNewView.h"
#include "../Explore/ScatterNewPlotView.h"
#include "../Explore/ScatterPlotMatView.h"
#include "../GdaCartoDB.h"
#include "../FramesManager.h"
#include "../Project.h"

#include "PublishDlg.h"


BEGIN_EVENT_TABLE( PublishDlg, wxDialog )
    EVT_BUTTON( wxID_OK, PublishDlg::OnOkClick )
END_EVENT_TABLE()

PublishDlg::PublishDlg(wxWindow* parent, Project* _p,
                     wxWindowID id,
                     const wxString& title,
                     const wxPoint& pos,
                     const wxSize& size )
{
    wxLogMessage("Open PublishDlg.");
    p = _p;
    
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_GEODA_PUBLISH_DLG");
    FindWindow(XRCID("wxID_OK"))->Enable(true);
	m_txt_uname = XRCCTRL(*this, "IDC_GEODA_USERNAME",wxTextCtrl);
	m_txt_key = XRCCTRL(*this, "IDC_GEODA_KEY",wxTextCtrl);
	m_txt_title = XRCCTRL(*this, "IDC_GEODA_PUBLISH_TITLE",wxTextCtrl);
	m_txt_description = XRCCTRL(*this, "IDC_GEODA_PUBLISH_DESCRIPTION",wxTextCtrl);
    
    m_txt_uname->SetValue("lixun910");
    m_txt_uname->Enable(false);
    
    m_txt_key->SetValue("asdjk23989234kasdlfj29");
    m_txt_key->Enable(false);
    
    SetParent(parent);
	SetPosition(pos);
	Centre();
}


void PublishDlg::OnOkClick( wxCommandEvent& event )
{
    wxLogMessage("In PublishDlg::OnOkClick()");
    wxString title(m_txt_title->GetValue());
    wxString description(m_txt_description->GetValue() );
    
    
    GeoDaWebProxy geodaweb;
    geodaweb.Publish(p, title, description);
    
	EndDialog(wxID_OK);
}


using namespace std;

GeoDaWebProxy::GeoDaWebProxy()
{
    api_key = CartoDBProxy::GetInstance().GetKey();
    user_name = CartoDBProxy::GetInstance().GetUserName();
}


GeoDaWebProxy::GeoDaWebProxy(const string& _user_name, const string& _api_key)
{
    user_name = _user_name;
    api_key = _api_key;
    
}

GeoDaWebProxy::~GeoDaWebProxy() {
    
}


void GeoDaWebProxy::Publish(Project* p, wxString& title, wxString& description)
{
	if (p == NULL)
		return;
	
	ostringstream ss;
	
	// table_name
	ss << buildParameter("table_name", p->layername);

    ss << "&" << buildParameter("title", title);
    
    ss << "&" << buildParameter("content", description);
    
	
	// maps & plots
	FramesManager* fm = p->GetFramesManager();
	list<FramesManagerObserver*> observers(fm->getCopyObservers());
	list<FramesManagerObserver*>::iterator it;
	for (it=observers.begin(); it != observers.end(); ++it) {
		if (LisaMapFrame* w = dynamic_cast<LisaMapFrame*>(*it)) {
			vector<int> clusters;
			w->GetVizInfo(clusters);
			if (!clusters.empty()) {
				ss << "&" << buildParameter("lisa", clusters);
			}
			continue;
		}
		if (MapFrame* w = dynamic_cast<MapFrame*>(*it)) {
            wxString shape_type;
            wxString field_name;
            std::vector<wxString> clrs;
            std::vector<double> bins;
            
            
            w->GetVizInfo(shape_type, field_name, clrs, bins);
            
            ss << "&map={\"map_type\":\"" << shape_type.mb_str() << "\",";
            ss << "\"legend_field\":\"" << field_name << "\"";
            
            if (!clrs.empty()) {
                
                ss << ",\"colors\":[";
                for (size_t i=0; i< clrs.size(); i++) {
                    ss << "\"" << clrs[i] << "\"";
                    if (i < clrs.size() -1) ss << ",";
                }
                ss << "]";
            }
            if (!bins.empty()) {
                ss.setf(std::ios_base::fixed);
                ss << ",\"bins\":[";
                for (size_t i=0; i< bins.size(); i++) {
                    ss << bins[i] ;
                    if (i < clrs.size() -1) ss << ",";
                }
                ss << "]";
            }
            ss << "}";
			continue;
		} 
		
		if (HistogramFrame* w = dynamic_cast<HistogramFrame*>(*it)) {
			wxString col_name;
			int num_bins = 0;
			w->GetVizInfo(col_name, num_bins);
			if (!col_name.empty() && num_bins>0) {
				wxString val;
				val << "[\"" << col_name << "\"," << num_bins << "]";
				ss << "&" << buildParameter("histogram", val);
			}
			continue;
		} 
		if (ScatterPlotMatFrame* w = dynamic_cast<ScatterPlotMatFrame*>(*it)) {
			vector<wxString> vars;
			w->GetVizInfo(vars);
			if (!vars.empty()) {
				ss << "&" << buildParameter("scattermatrix", vars);
			}
		    continue;
			
		}
		if (ScatterNewPlotFrame* w = dynamic_cast<ScatterNewPlotFrame*>(*it)) {
			wxString x;
			wxString y;
			w->GetVizInfo(x, y);
			if (!x.empty() && !y.empty()) {
				wxString val;
				val << "[\"" << x << "\",\"" << y << "\"]";
				ss << "&" << buildParameter("scatterplot", val);
			}
			continue;
		} 
	}
	
	// submit request
	string parameter = ss.str();
	
	string returnUrl = doPost(parameter);
	
	// launch browser with return url
	wxString published_url(returnUrl);
	wxLaunchDefaultBrowser(published_url);
}


void GeoDaWebProxy::SetKey(const string& key) {
    api_key = key;
}

void GeoDaWebProxy::SetUserName(const string& name) {
    user_name = name;
}

string GeoDaWebProxy::buildParameter(map<wxString, vector<int> >& val)
{
	ostringstream par;
	map<wxString, vector<int> >::iterator it;
	
	par << "{" ;
	for (it=val.begin(); it != val.end(); ++it) {
		wxString clr( it->first);
		vector<int>& ids = it->second;
		
		par << "\"" << clr.mb_str() << "\": [";
		for (size_t i=0; i< ids.size(); i++) {
			par << ids[i];
            if (i < ids.size() -1) par << ",";
		}
		par << "],";
	}
	par << "}" ;
	
	return par.str();
}

string GeoDaWebProxy::buildParameter(const char* key, string& val)
{
	ostringstream par;
	par << key << "=" << val;
	return par.str();
}


string GeoDaWebProxy::buildParameter(const char* key, wxString& val)
{
	ostringstream par;
	par << key << "=" << val.mb_str();
	return par.str();
}

string GeoDaWebProxy::buildParameter(const char* key, vector<int>& val)
{
	ostringstream par;
	par << key << "=" << "[";
	for (size_t i=0, n=val.size(); i<n; i++) {
		par << val[i];
        if (i < val.size() -1) par << ",";
	}
	par << "]";
	return par.str();
}

string GeoDaWebProxy::buildParameter(const char* key, vector<wxString>& val)
{
	ostringstream par;
	par << key << "=" << "[";
	for (size_t i=0, n=val.size(); i<n; i++) {
		par << "\"" << val[i] << "\"";
        if (i < val.size() -1) par << ",";
	}
	par << "]";
	return par.str();
}
									 
string GeoDaWebProxy::buildBaseUrl()
{
    ostringstream url;
    url << "https://webpool.csf.asu.edu/xun/myapp/geoda_publish/";
    //url << "http://127.0.0.1:8000/myapp/geoda_publish/";
    return url.str();
}

size_t write_data(char *ptr, size_t size, size_t nmemb, void *userdata) {
    std::ostringstream *stream = (std::ostringstream*)userdata;
    size_t count = size * nmemb;
    stream->write(ptr, count);
    return count;
}


void GeoDaWebProxy::doGet(string& parameter)
{
    CURL* curl;
    CURLcode res;
    
    curl = curl_easy_init();
    if (curl) {
        string url = buildBaseUrl() + "?api_key=" + api_key +"&" + parameter;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        
        // Grab image 
        res = curl_easy_perform(curl);
        if( res ) {
            printf("Cannot connect carto.com!\n");
        } 
        
        int res_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
        if (!((res_code == 200 || res_code == 201) && res != CURLE_ABORTED_BY_CALLBACK))
        {
            printf("!!! Response code: %d\n", res_code);
			// Clean up the resources 
			curl_easy_cleanup(curl);
            return;
        }
		// Clean up the resources 
		curl_easy_cleanup(curl);
    }
    
}

string GeoDaWebProxy::doPost(const string& _parameter)
{
    CURL* curl;
    CURLcode res;

	string parameter = _parameter;

    //curl_global_init(CURL_GLOBAL_ALL);
    ostringstream out;
	
    curl = curl_easy_init();
    if (curl) {
        string url = buildBaseUrl();
        parameter = "api_key=" + api_key + "&" + parameter;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, parameter.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		 
        res = curl_easy_perform(curl);
        if( res ) {
            printf("Cannot connect carto.com!\n");
        } 
        
        int res_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);

        if (!((res_code == 200 || res_code == 201) && res != CURLE_ABORTED_BY_CALLBACK))
        {
            printf("!!! Response code: %d\n", res_code);
        }
		// Clean up the resources 
		curl_easy_cleanup(curl);
    }

    //curl_global_cleanup();
    return out.str();
}
