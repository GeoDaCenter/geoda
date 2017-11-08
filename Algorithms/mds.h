

#ifndef __GEODA_CENTER_ALG_MDS_H
#define __GEODA_CENTER_ALG_MDS_H

#include <stdlib.h>
#include <math.h> 
#include "DataUtils.h"

class AbstractMDS {
public:
    AbstractMDS(int n, int dim);
    ~AbstractMDS();
    
    virtual void fullmds(vector<vector<double> >& d, int dim, int maxiter=100);
    virtual vector<double> pivotmds(vector<vector<double> >& input, vector<vector<double> >& result);
    virtual vector<vector<double> >& GetResult();
    
protected:
    int n;
    int dim;
    vector<vector<double> > result;
};

class FastMDS : public AbstractMDS {
public:
    FastMDS(vector<vector<double> >& distances, int dim, int maxiter);
    virtual ~FastMDS();
   
    
protected:
    vector<vector<double> > classicalScaling(vector<vector<double> >& d, int dim, int maxiter);
    vector<double> lmds(vector<vector<double> >& P, vector<vector<double> >& result, int maxiter);
};

/*
class LandmarkMDS : public AbstractMDS {
    
};

class SMACOF : public AbstractMDS {
    
};
*/

#endif
