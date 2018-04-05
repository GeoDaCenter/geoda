#include <iostream>
#include <fstream>
#include <string>

#include <boost/unordered_map.hpp>
#include <wx/wx.h>

#include "arcgis_swm.h"
#include "../GenUtils.h"
#include "../Project.h"
#include "../DataViewer/TableInterface.h"

using namespace std;

GalElement* read()
{
    string input_file = "/Users/xun/Downloads/ohio.swm";
    string line;
    ifstream istream;
    
    istream.open(input_file.c_str(),ios::binary|ios::in);
    
    // first line
    // ID_VAR_NAME;ESRI_SRS\n
    getline(istream, line, '\n');
    
    // NO_OBS length=4
    uint32_t no_obs = 0;
    istream.read((char*)&no_obs, 4); // reads 4 bytes into
    
    // infile.seekp(243, ios::beg); // move 243 bytes into the file
    // infile.seekp(0,ios::end); // seek to the end of the file
    // infile.seekp(-10, ios::cur); // back up 10 bytes
    
    // ROW_STD length = 4
    uint32_t row_std = 0;
    istream.read((char*)&row_std, 4);
    
    GalElement* gal = new GalElement[no_obs];
    
    for (int i=0; i<no_obs; i++) {
        // origin length = 4
        uint32_t origin = 0;
        istream.read((char*)&origin, 4);
        
        // no_nghs length = 4
        uint32_t no_nghs = 0;
        istream.read((char*)&no_nghs, 4);
        
        uint32_t* n_ids = new uint32_t[no_nghs];
        istream.read ((char*)n_ids, sizeof (uint32_t) * no_nghs);
    
        double* n_w = new double[no_nghs];
        istream.read ((char*)n_w, sizeof (double) * no_nghs);
        
        double sum_w;
        istream.read((char*)&sum_w, 8);
        
        gal[i].SetSizeNbrs(no_nghs);
        for (int j=0; j<no_nghs; j++) {
            gal[i].SetNbr(j, n_ids[j], n_w[j]);
        }
    }
    
    istream.close();
    
    return gal;
}

wxString ReadIdFieldFromSwm(const wxString& fname)
{
#ifdef __WIN32__
    ifstream istream;
    istream.open(fname.wc_str(), ios::binary|ios::in);
#else
    ifstream istream;
    istream.open(GET_ENCODED_FILENAME(fname), ios::binary|ios::in);  // a text file
#endif
    
    if (!(istream.is_open() && istream.good())) {
        return wxEmptyString;
    }
    // first line
    string line;
    // ID_VAR_NAME;ESRI_SRS\n
    getline(istream, line, '\n');
    
    wxString first_line(line);
    int pos = first_line.First(';');
    wxString id_name = first_line.SubString(0, pos-1);
    
    istream.close();
    return id_name;
}

GalElement* ReadSwmAsGal(const wxString& fname, TableInterface* table_int)
{
#ifdef __WIN32__
    ifstream istream;
    istream.open(fname.wc_str(), ios::binary|ios::in);
#else
    ifstream istream;
    istream.open(GET_ENCODED_FILENAME(fname), ios::binary|ios::in);  // a text file
#endif
    
    if (!(istream.is_open() && istream.good())) {
        return 0;
    }
    // first line
    string line;
    // ID_VAR_NAME;ESRI_SRS\n
    getline(istream, line, '\n');
    
    wxString first_line(line);
    int pos = first_line.First(';');
    wxString id_name = first_line.SubString(0, pos-1);
    
    // NO_OBS length=4
    uint32_t no_obs = 0;
    istream.read((char*)&no_obs, 4); // reads 4 bytes into
    
    if (table_int != NULL && no_obs != table_int->GetNumberRows()) {
        wxString msg = "The number of observations specified in chosen ";
        msg << "weights file is " << no_obs << ", but the number in the ";
        msg << "current Table is " << table_int->GetNumberRows();
        msg << ", which is incompatible.";
        wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return 0;
    }
    if (table_int != NULL) {
        int col, tm;
        table_int->DbColNmToColAndTm(id_name, col, tm);
        if (col == wxNOT_FOUND) {
            wxString msg = "Specified key value field \"";
            msg << id_name << "\" on first line of weights file not found ";
            msg << "in currently loaded Table.";
            wxMessageDialog dlg(NULL, msg, "Error", wxOK | wxICON_ERROR);
            dlg.ShowModal();
            return 0;
        }
    }
    
    // ROW_STD length = 4
    uint32_t row_std = 0;
    istream.read((char*)&row_std, 4);
    
    boost::unordered_map<int, uint32_t> id_map;
    vector<vector<int> > nbr_ids(no_obs);
    vector<vector<double> > nbr_ws(no_obs);
    
    for (int i=0; i<no_obs; i++) {
        // origin length = 4
        uint32_t origin = 0;
        istream.read((char*)&origin, 4);
        
        id_map[i] = origin;
        
        // no_nghs length = 4
        uint32_t no_nghs = 0;
        istream.read((char*)&no_nghs, 4);
        
        uint32_t* n_ids = new uint32_t[no_nghs];
        istream.read ((char*)n_ids, sizeof (uint32_t) * no_nghs);
        
        double* n_w = new double[no_nghs];
        istream.read ((char*)n_w, sizeof (double) * no_nghs);
        
        double sum_w;
        istream.read((char*)&sum_w, 8);
        
        nbr_ids[i].resize(no_nghs);
        nbr_ws[i].resize(no_nghs);
        for (int j=0; j<no_nghs; j++) {
            nbr_ids[i][j] = n_ids[j];
            nbr_ws[i][j] = n_w[j];
        }
    }
    
    GalElement* gal = new GalElement[no_obs];
    for (int i=0; i<no_obs; i++) {
        int no_nghs = nbr_ids[i].size();
        gal[i].SetSizeNbrs(no_nghs);
        vector<int>& n_ids = nbr_ids[i];
        vector<double>& n_w = nbr_ws[i];
        for (int j=0; j<no_nghs; j++) {
            gal[i].SetNbr(j, id_map[n_ids[j]], n_w[j]);
        }
    }
    
    
    istream.close();
    
    return gal;
}
