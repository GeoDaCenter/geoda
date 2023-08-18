#ifndef __GEODA_CENTER_MATLAB_MAT_H__
#define __GEODA_CENTER_MATLAB_MAT_H__
#include <fstream>
#include <exception>

#include "../ShapeOperations/GalWeight.h"

wxString ReadIdFieldFromMat(const wxString& fname);

GalElement* ReadMatAsGal(const wxString& fname, TableInterface* table_int);

#endif /* _MATFILEREADER_H_ */
