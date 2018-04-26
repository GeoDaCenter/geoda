#include "hdbscan.h"


using namespace GeoDaClustering;

////////////////////////////////////////////////////////////////////////////////
//
// HDBSCAN
//
////////////////////////////////////////////////////////////////////////////////
HDBScan::HDBScan(int rows, int cols, double** _distances, double** _data, const vector<bool>& _undefs, GalElement* w, double* _controls, double _control_thres)
: FirstOrderSLKRedCap(rows, cols, _distances, _data, _undefs, w, _controls, _control_thres)
{
    /*
    rtree_pt_3d_t rtree;
    {
        vector<pt_3d> pts;
        {
            vector<pt_lonlat> ptll(nobs);
            for (int i=0; i<nobs; ++i) ptll[i] = pt_lonlat(x[i], y[i]);
            to_3d_centroids(ptll, pts);
        }
        fill_pt_rtree(rtree, pts);
    }
    
    for (rtree_pt_2d_t::const_query_iterator it = rtree.qbegin(bgi::intersects(rtree.bounds()));
         it != rtree.qend() ; ++it)
    {
        const pt_2d_val& v = *it;
        size_t obs = v.second;
        // each point "v" with index "obs"
        vector<pt_2d_val> q;
        rtree.query(bgi::nearest(v.first, k), std::back_inserter(q));
        GwtElement& e = Wp->gwt[obs];
        e.alloc(q.size());
        double local_bandwidth = 0;
        BOOST_FOREACH(pt_2d_val const& w, q) {
            if ( (kernel.IsEmpty() && w.second == v.second) ||
                (!kernel.IsEmpty() && !use_kernel_diagnals && w.second == v.second) )
                continue;
            GwtNeighbor neigh;
            neigh.nbx = w.second;
            double d = bg::distance(v.first, w.first);
            if (bandwidth_ ==0 && d > bandwidth) bandwidth = d;
            if (d > local_bandwidth) local_bandwidth = d;
            if (is_inverse) d = pow(d, power);
            neigh.weight =  d;
            e.Push(neigh);
            ++cnt;
        }
        if (adaptive_bandwidth && local_bandwidth > 0 && !kernel.IsEmpty()) {
            GwtNeighbor* nbrs = e.dt();
            for (int j=0; j<e.Size(); j++) {
                nbrs[j].weight = nbrs[j].weight / local_bandwidth;
            }
        }
    }
    
    // d_core(x_p), is the distance from x_p to its m_pts-nearest neighbor (incl. x_p)
    cores.resize(rows);
     */
}

HDBScan::~HDBScan()
{
    
}
