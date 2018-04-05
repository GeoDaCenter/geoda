#include <fstream>

#include "../ShapeOperations/GalWeight.h"

using namespace std;

GalElement* read();

wxString ReadIdFieldFromSwm(const wxString& fname);

GalElement* ReadSwmAsGal(const wxString& fname, TableInterface* table_int);
