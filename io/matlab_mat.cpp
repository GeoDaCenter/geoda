#include <vector>
#include <iostream>
#include <cstdlib>

#include "../GenUtils.h"
#include "../DataViewer/TableInterface.h"
#include "MatfileReader.h"
#include "weights_interface.h"
#include "matlab_mat.h"

wxString ReadIdFieldFromMat(const wxString& fname)
{
    return "ogc_fid";
}

GalElement* ReadMatAsGal(const wxString& fname, TableInterface* table_int)
{
#ifdef __WIN32__
    std::ifstream istream;
    istream.open(fname.wc_str(), std::ios::binary|std::ios::in);
#else
    std::ifstream istream;
    istream.open(GET_ENCODED_FILENAME(fname), std::ios::binary|std::ios::in);  // a text file
#endif
    
    if (!(istream.is_open() && istream.good())) {
        return 0;
    }
    MatfileReader mmr(istream);
    mmr.parseHeader();
    //cout << mmr.descriptiveText() << std::endl;
    //cout << mmr.subsysDataOffset() << std::endl;
    //cout << mmr.version() << std::endl;
    //cout << mmr.endianIndicator() << std::endl;
    mmr.gotoData();
    if (mmr.parseDataElement()  ==  false) {
        throw WeightsNotValidException();
    }
    std::vector<DataElement*> des = mmr.dataElements();
    // cout << des[0]->dataType() << std::endl;
    DataElement* de = des[0];
    CompressedDataElement* cde;
    if (des[0]->dataType() == miCOMPRESSED) {
        cde = dynamic_cast<CompressedDataElement*>(des[0]);
        assert(cde);
        // fstream fo("compressed.dat", ios_base::out | ios_base::binary);
        // assert(fo);
        // fo.write(cde->decompressedData(), cde->decompressedSize());
        de = cde->reparse();
    }
    
    // get row & col #
    MatrixDataElement* mde = (MatrixDataElement*)de;
    DimensionsArray* da = mde->dimensionsArray();
    std::vector<int32_t>& dim = da->dimensions();
    if (dim.size() != 2) {
        throw WeightsNotValidException();
    }
    int n_rows = dim[0];
    int n_cols = dim[1];
    if (n_rows != n_cols) {
        throw WeightsNotValidException();
    }
  
    int num_obs = table_int->GetNumberRows();
    if (n_rows != num_obs) {
        throw WeightsMismatchObsException(n_rows);
    }
   
    // prepare output
    GalElement* gal = new GalElement[num_obs];

    // get weights matrix
    std::vector<double> data(num_obs * num_obs);
    NumericArray<double>* sde = (NumericArray<double>*)mde;
    DataElement* real = sde->real();
    EDataType dt = real->dataType();
    if (dt == miSINGLE) {
        // float
        FlatDataElement<float>* flatdata = ( FlatDataElement<float>*)real;
        std::vector<float>& _data = flatdata->data();
        for (int i=0; i< data.size(); i++) {
            data[i] = _data[i];
        }
    } else if (dt == miDOUBLE) {
        // double
        FlatDataElement<double>* flatdata = ( FlatDataElement<double>*)real;
        std::vector<double>& _data = flatdata->data();
        for (int i=0; i< data.size(); i++) {
            data[i] = _data[i];
        }
    } else {
        // INT8 UINT8 INT16 UINT16 INT32 UINT32
        FlatDataElement<uint8_t>* flatdata = ( FlatDataElement<uint8_t>*)real;
        std::vector<uint8_t>& _data = flatdata->data();
        for (int i=0; i< data.size(); i++) {
            data[i] = _data[i];
        }
    }
    
    for (int i=0; i<num_obs; i++) {
        // row
        int row = i * num_obs;
        int no_nbrs = 0;
        for (int j=0; j<num_obs; j++) {
            // col
            float w = data[j + row];
            if ( w != 0) {
                no_nbrs += 1;
            }
        }
        gal[i].SetSizeNbrs(no_nbrs);
        int nbr_idx = 0;
        for (int j=0; j<num_obs; j++) {
            float w = data[j + row];
            if ( w != 0) {
                gal[i].SetNbr(nbr_idx++, j, w);
            }
        }
    }
    
    return gal;
}

