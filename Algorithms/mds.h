

#ifndef __GEODA_CENTER_ALG_MDS_H
#define __GEODA_CENTER_ALG_MDS_H

#include <stdlib.h>
#include <math.h> 
#include "DataUtils.h"

class AbstractMDS {
public:
    AbstractMDS(int dim);
    ~AbstractMDS();
    
    virtual double** fullmds(double** d, int n, int k, int dim);
    virtual double* pivotmds(double** input, double** result);
    virtual double** GetResult();
    
protected:
    int dim;
    double** result;
};

class FastMDS : public AbstractMDS {
public:
    FastMDS(double** distances, int n, int k, int dim);
    virtual ~FastMDS();
    
protected:
};

/*
class LandmarkMDS : public AbstractMDS {
    
};

class SMACOF : public AbstractMDS {
    
};
*/

#endif
