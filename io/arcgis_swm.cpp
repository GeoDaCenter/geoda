#include <iostream>
#include <fstream>
#include <string>

#include <boost/unordered_map.hpp>
#include <wx/wx.h>

#include "../GenUtils.h"
#include "../Project.h"
#include "../DataViewer/TableInterface.h"
#include "weights_interface.h"
#include "arcgis_swm.h"

using namespace std;

bool iequals(const string& a, const string& b)
{
    unsigned int sz = a.size();
    if (b.size() != sz)
        return false;
    for (unsigned int i = 0; i < sz; ++i)
        if (tolower(a[i]) != tolower(b[i]))
            return false;
    return true;
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
    
    if (id_name.find("VERSION")==0) {
        // new format: VERSION@10.1;UNIQUEID@FIELD_ID;
        id_name = line.substr(line.find(';')+1, line.size()-1);
        id_name = id_name.substr(0, id_name.find(';'));
        if (id_name.find("UNIQUEID") ==0 ){
            id_name = id_name.substr(id_name.find('@')+1, id_name.size()-1);
        }
    }
    
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
    // ID_VAR_NAME;ESRI_SRS\n
    string line;
    getline(istream, line, '\n');
    string id_name = line.substr(0, line.find(';'));
    
    int swmType = 0; // old
    bool fixed = false;
    
    if (id_name.find("VERSION")==0) {
        swmType = 1;
        // new format: VERSION@10.1;UNIQUEID@FIELD_ID;
        id_name = line.substr(line.find(';')+1, line.size()-1);
        id_name = id_name.substr(0, id_name.find(';'));
        if (id_name.find("UNIQUEID") ==0 ){
            id_name = id_name.substr(id_name.find('@')+1, id_name.size()-1);
        }
        int pos = line.find("FIXEDWEIGHTS@");
        if (pos > 0) {
            string fixed_w = line.substr(pos+13, 4);
            if (iequals(fixed_w, "True")) {
                fixed = true;
            }
        }
    }
    
    // NO_OBS length=4
    uint32_t no_obs = 0;
    istream.read((char*)&no_obs, 4); // reads 4 bytes into
   
    int num_obs_tbl = table_int->GetNumberRows();
    if (table_int != NULL && no_obs != num_obs_tbl) {
        throw WeightsMismatchObsException(no_obs);
    }
    std::vector<wxInt64> uids;
    boost::unordered_map<int, uint32_t> id_map;
    
    if (id_name != "Unknown" && table_int != NULL) {
        int col, tm;
        table_int->DbColNmToColAndTm(id_name, col, tm);
        if (col == wxNOT_FOUND) {
            throw WeightsIdNotFoundException(id_name.c_str());
        }
        table_int->GetColData(col, 0, uids);
        for (int i=0; i<uids.size(); i++) {
            id_map[uids[i]] = i;
        }
    } else {
        for (int i=0; i<no_obs; i++) {
            id_map[i] = i;
        }
    }
    
    // ROW_STD length = 4
    uint32_t row_std = 0;
    istream.read((char*)&row_std, 4);
    
    
    vector<vector<int> > nbr_ids(no_obs);
    vector<vector<double> > nbr_ws(no_obs);
    

    for (int i=0; i<no_obs; i++) {
        // origin length = 4
        uint32_t origin = 0;
        istream.read((char*)&origin, 4);
        int o_idx = id_map[origin];
        
        if ( id_map.find(o_idx) == id_map.end() ) {
            throw WeightsIntegerKeyNotFoundException(o_idx);
        }
        
        // no_nghs length = 4
        uint32_t no_nghs = 0;
        istream.read((char*)&no_nghs, 4);
        
        if (no_nghs > 0) {
            if (fixed) {
                uint32_t* n_ids = new uint32_t[no_nghs];
                istream.read ((char*)n_ids, sizeof (uint32_t) * no_nghs);
                
                double _w = 0;
                istream.read((char*)&_w, sizeof(double));
                
                double sum_w;
                istream.read((char*)&sum_w, sizeof(double)); // 8
                
                nbr_ids[o_idx].resize(no_nghs);
                nbr_ws[o_idx].resize(no_nghs);
                for (int j=0; j<no_nghs; j++) {
                    if ( id_map.find(n_ids[j]) == id_map.end() ) {
                        throw WeightsIntegerKeyNotFoundException(o_idx);
                    }
                    nbr_ids[o_idx][j] = id_map[ n_ids[j] ];
                    nbr_ws[o_idx][j] = _w;
                }
                delete[] n_ids;
                
            } else {
                uint32_t* n_ids = new uint32_t[no_nghs];
                istream.read ((char*)n_ids, sizeof (uint32_t) * no_nghs);
                
                double* n_w = new double[no_nghs];
                istream.read ((char*)n_w, sizeof (double) * no_nghs);
                
                double sum_w;
                istream.read((char*)&sum_w, 8);
                
                nbr_ids[o_idx].resize(no_nghs);
                nbr_ws[o_idx].resize(no_nghs);
                for (int j=0; j<no_nghs; j++) {
                    if ( id_map.find(n_ids[j]) == id_map.end() ) {
                        throw WeightsIntegerKeyNotFoundException(o_idx);
                    }
                    nbr_ids[ o_idx ][j] = id_map[ n_ids[j] ];
                    nbr_ws[ o_idx ][j] = n_w[j];
                }
                
                delete[] n_w;
                delete[] n_ids;
            }
        }
    }
    
    GalElement* gal = new GalElement[no_obs];
    for (int i=0; i<no_obs; i++) {
        int no_nghs = nbr_ids[i].size();
        gal[i].SetSizeNbrs(no_nghs);
        vector<int>& n_ids = nbr_ids[i];
        vector<double>& n_w = nbr_ws[i];
        for (int j=0; j<no_nghs; j++) {
            int nid = n_ids[j];
            gal[ i ].SetNbr(j, nid, n_w[j]);
        }
    }
    
    
    istream.close();
    
    return gal;
}
