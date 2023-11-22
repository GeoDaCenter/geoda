

#ifndef __GEODA_CENTER_ALG_MDS_H
#define __GEODA_CENTER_ALG_MDS_H

#include <stdlib.h>
#include <math.h> 
#include "DataUtils.h"

class AbstractMDS {
public:
    AbstractMDS(int n, int dim);
    ~AbstractMDS();
    
    virtual void fullmds(std::vector<std::vector<double> >& d, int dim, int maxiter=100);
    virtual std::vector<double> pivotmds(std::vector<std::vector<double> >& input, std::vector<std::vector<double> >& result);
    virtual std::vector<std::vector<double> >& GetResult();
    
protected:
    int n;
    int dim;
    std::vector<std::vector<double> > result;
};

class FastMDS : public AbstractMDS {
public:
    FastMDS(std::vector<std::vector<double> >& distances, int dim, int maxiter);
    virtual ~FastMDS();
   
    
protected:
    std::vector<std::vector<double> > classicalScaling(std::vector<std::vector<double> >& d, int dim, int maxiter);
    std::vector<double> lmds(std::vector<std::vector<double> >& P, std::vector<std::vector<double> >& result, int maxiter);
};

/*
class LandmarkMDS : public AbstractMDS {
    
};

class SMACOF : public AbstractMDS {
    
};
*/

#endif
