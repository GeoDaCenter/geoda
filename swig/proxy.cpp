#include <string>
#include <vector>
#include <set>
#include <stdio.h>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../ShapeOperations/GwtWeight.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/PolysToContigWeights.h"
#include "../ShapeOperations/VoronoiUtils.h"
#include "../Explore/LisaCoordinator.h"
#include "../SpatialIndAlgs.h"

#include "proxy.h"

using namespace std;

wxString GetLayerName(string shp_filename)
{
    wxString m_shp_str(shp_filename);

    wxFileName m_shp_fname(m_shp_str);

    return m_shp_fname.GetName();
}

int OpenShapeFile(string in_file, Shapefile::Main& main_data, Shapefile::Index& index_data)
{
    wxString m_shp_str(in_file);

    wxFileName m_shx_fname(m_shp_str);
    m_shx_fname.SetExt("shx");
    wxString m_shx_str = m_shx_fname.GetFullPath();

#ifdef DEBUG
    printf("shx file name: %s\n", m_shx_str.mb_str().data());
#endif

    bool success = false;
    success = Shapefile::populateIndex(m_shx_str, index_data);
#ifdef DEBUG
    printf("populateIndex: %d\n", success);
#endif
    if (success == false)
        return success;

    success = Shapefile::populateMain(index_data, m_shp_str, main_data);
#ifdef DEBUG
    printf("populateMain: %d\n", success);
#endif
    if (success == false)
        return success;

    if (index_data.header.shape_type == Shapefile::POLYGON_Z) {
        index_data.header.shape_type = Shapefile::POLYGON;
    } else if (index_data.header.shape_type == Shapefile::POLYGON_M) {  
        index_data.header.shape_type = Shapefile::POLYGON;
    } else if (index_data.header.shape_type == Shapefile::POINT_Z) {
        index_data.header.shape_type = Shapefile::POINT_TYP;
    } else if (index_data.header.shape_type == Shapefile::POINT_M) {
        index_data.header.shape_type = Shapefile::POINT_TYP;
    } else if (index_data.header.shape_type == Shapefile::MULTI_POINT) {
        return Shapefile::MULTI_POINT;
    } else if (index_data.header.shape_type == Shapefile::POLY_LINE_Z) {
        index_data.header.shape_type = Shapefile::POLY_LINE;
    } else if (index_data.header.shape_type == Shapefile::POLY_LINE_M) {
        index_data.header.shape_type = Shapefile::POLY_LINE;
    }

    return success;
}

bool CreateCentroids(Shapefile::Main& main_data, std::vector<double>& XX, std::vector<double>& YY)
{
    int num_obs = main_data.records.size();
    XX.resize(num_obs);
    YY.resize(num_obs);

    if (main_data.header.shape_type == Shapefile::POINT_TYP) {
        Shapefile::PointContents* pc;
        for (int i=0; i<num_obs; i++) {
            pc = (Shapefile::PointContents*)main_data.records[i].contents_p;
            if (pc->shape_type == 0) {
                XX[i] = 0;
                YY[i] = 0;
            } else {
                XX[i] = pc->x;
                YY[i] = pc->y;
            }
        }
    } else if (main_data.header.shape_type == Shapefile::POLYGON) {
        Shapefile::PolygonContents* pc;
        for (int i=0; i<num_obs; i++) {
            pc = (Shapefile::PolygonContents*)main_data.records[i].contents_p;
            GdaPolygon poly(pc);
            if (poly.isNull()) {
                XX[i] = 0;
                YY[i] = 0;
            } else {
                wxRealPoint rp(GdaShapeAlgs::calculateCentroid(&poly));
                XX[i] = rp.x;
                YY[i] = rp.y;
            }
        }
    } else {
        // line data are not supported.
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Contiguity Weights (Queen/Rook)
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void GetVoronoiRookNeighborMap(Shapefile::Main& main_data, std::vector<std::set<int> >& nbr_map)
{
#ifdef DEBUG
    printf("Project::GetVoronoiRookNeighborMap()");
#endif

    std::vector<double> x;
    std::vector<double> y;
    CreateCentroids(main_data, x, y);
    Gda::VoronoiUtils::PointsToContiguity(x, y, false, nbr_map);
}

void GetVoronoiQueenNeighborMap(Shapefile::Main& main_data, std::vector<std::set<int> >& nbr_map)
{
#ifdef DEBUG
    printf("Project::GetVoronoiQueenNeighborMap()");
#endif

    std::vector<double> x;
    std::vector<double> y;
    CreateCentroids(main_data, x, y);
    Gda::VoronoiUtils::PointsToContiguity(x, y, true, nbr_map);
}

bool CreateContiguityWeights(string in_file, string out_file, bool is_rook, int order, bool include_lower_order)
{
    Shapefile::Main main_data;
    Shapefile::Index index_data;
    bool success = OpenShapeFile(in_file, main_data, index_data);
#ifdef DEBUG
    printf("OpenShapeFile: %d\n", success);
#endif
    if (!success)
        return false;

    int n_obs = Shapefile::calcNumIndexHeaderRecords(index_data.header);
#ifdef DEBUG
    printf("number of observations: %d\n", n_obs);
#endif

    wxString layer_name = GetLayerName(in_file);
    double precision_threshold = 0.0;

    GalElement *gal = NULL;

    if (main_data.header.shape_type == Shapefile::POINT_TYP) {
        std::vector<std::set<int> > nbr_map;
        if (is_rook) {
            GetVoronoiRookNeighborMap(main_data, nbr_map);
        } else {
            GetVoronoiQueenNeighborMap(main_data, nbr_map);
        }
        gal = Gda::VoronoiUtils::NeighborMapToGal(nbr_map); 
    } else {
        gal  = PolysToContigWeights(main_data, !is_rook, precision_threshold);
    }       

#ifdef DEBUG
    printf("PolysToContigWeights: %d\n", gal);
#endif
    if (gal == NULL)
        return false;
   
    if (order > 1)  
        Gda::MakeHigherOrdContiguity(order, n_obs, gal, include_lower_order);
   
    // output: save weights file 
    wxString ofn(out_file);
    wxString idd = "ogc_fid"; // should be the same with database
    vector<wxInt64> id_vec;
    for (int i=0; i<n_obs; i++) id_vec.push_back(i+1); // POLY_ID starts with 1

    bool flag = Gda::SaveGal(gal, layer_name, ofn, idd, id_vec);
#ifdef DEBUG
    printf("SaveGal: %d\n", flag);
#endif

    delete[] gal; 
    return flag;
}

bool CreateQueenWeights(string in_file, string out_file, int order, bool include_lower_order)
{
    bool is_rook = false;
    return CreateContiguityWeights(in_file, out_file, is_rook, order, include_lower_order);
}

bool CreateRookWeights(string in_file, string out_file, int order, bool include_lower_order)
{
    bool is_rook = true;
    return CreateContiguityWeights(in_file, out_file, is_rook, order, include_lower_order);
}

bool Test(string in_file, string out_file, int order)
{
    wxString inf(in_file);
    wxString ofn(out_file);

    wxString layer_name = "test";
    wxString idd = "F_ID";
    vector<wxInt64> id_vec;


    int num_obs = 10;

    vector<double> m_XCOO;
    vector<double> m_YCOO;
    
    for (int i=0; i<num_obs; i++) {
        m_XCOO.push_back( (double) i );
        m_YCOO.push_back( (double) i );
        id_vec.push_back( i );
    }

    double th = 1.5;
    bool is_arc = false;
    bool is_mi = false;

    GwtWeight* w = 0;
    w = SpatialIndAlgs::thresh_build(m_XCOO, m_YCOO, th, is_arc, is_mi);


    bool flag = false;
    flag = Gda::SaveGwt(w->gwt, layer_name, ofn, idd, id_vec);
    
    if (w) delete w;

    return flag;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// KNN Weights
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CreateKNNWeights(std::string in_file, std::string out_file, int k, bool is_arc, bool is_mile)
{
    Shapefile::Main main_data;
    Shapefile::Index index_data;
    bool success = OpenShapeFile(in_file, main_data, index_data);
#ifdef DEBUG
    printf("OpenShapeFile: %d\n", success);
#endif
    if (!success)
        return false;

    int n_obs = Shapefile::calcNumIndexHeaderRecords(index_data.header);
#ifdef DEBUG
    printf("number of observations: %d\n", n_obs);
#endif

    std::vector<double> XX;
    std::vector<double> YY;
    success = CreateCentroids(main_data, XX, YY);
#ifdef DEBUG
    printf("CreateCentroids: %d\n", success);
    printf("length of XX/YY: %d\n", XX.size());
#endif
    if (!success)
        return false;

    wxString layer_name = GetLayerName(in_file);
    GwtWeight* w = 0;
    w = SpatialIndAlgs::knn_build(XX, YY, k, is_arc, is_mile);
#ifdef DEBUG
    printf("SpatialIndAlgs::knn_build(): %d\n", w);
#endif

    if (w == NULL)
        return false;

    // output: save weights file 
    wxString ofn(out_file);
    wxString idd = "ogc_fid"; // should be the same with database
    vector<wxInt64> id_vec;
    for (int i=0; i<n_obs; i++) id_vec.push_back(i+1); // POLY_ID starts with 1

    bool flag = false;
    flag = Gda::SaveGwt(w->gwt, layer_name, ofn, idd, id_vec);

    delete w;

    return flag;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Distance based Weights
//
///////////////////////////////////////////////////////////////////////////////////////////////////

bool CreateDistanceWeights(std::string in_file, std::string out_file, double threshold, bool is_arc, bool is_mile)
{
    Shapefile::Main main_data;
    Shapefile::Index index_data;
    bool success = OpenShapeFile(in_file, main_data, index_data);
#ifdef DEBUG
    printf("OpenShapeFile: %d\n", success);
#endif
    if (!success)
        return false;

    int n_obs = Shapefile::calcNumIndexHeaderRecords(index_data.header);
#ifdef DEBUG
    printf("number of observations: %d\n", n_obs);
#endif

    std::vector<double> XX;
    std::vector<double> YY;
    success = CreateCentroids(main_data, XX, YY);
#ifdef DEBUG
    printf("CreateCentroids: %d\n", success);
    printf("length of XX/YY: %d\n", XX.size());
#endif
    if (!success)
        return false;

    wxString layer_name = GetLayerName(in_file);
    GwtWeight* w = 0;
    w = SpatialIndAlgs::thresh_build(XX, YY, threshold, is_arc, is_mile);
#ifdef DEBUG
    printf("SpatialIndAlgs::knn_build(): %d\n", w);
#endif

    if (w == NULL)
        return false;

    // output: save weights file 
    wxString ofn(out_file);
    wxString idd = "ogc_fid"; // should be the same with database
    vector<wxInt64> id_vec;
    for (int i=0; i<n_obs; i++) id_vec.push_back(i+1); // POLY_ID starts with 1

    bool flag = false;
    flag = Gda::SaveGwt(w->gwt, layer_name, ofn, idd, id_vec);

    delete w;

    return flag;
}

    // 0 univariate lisa
    // 1 bivariate lisa
    // 2 EB lisa
    // 3 diff lisa
bool LISA(std::string in_w_file, std::vector<double> var_1, std::vector<double> var_2, std::vector<double>& localMoran, std::vector<double>& sigLocalMoran, std::vector<int>& sigFlag, std::vector<int>& clusterFlag, int lisa_type, int numPermutations)
{
    wxString w_path(in_w_file);
    int num_obs = var_1.size();

    LisaCoordinator* lc = new LisaCoordinator(w_path, num_obs, var_1, var_2, lisa_type, numPermutations);
#ifdef DEBUG
    printf("LISA result(): \n");
    for (int i=0; i<num_obs; i++) {
        double a = lc->cluster_vecs[0][i];
        //double b = lc->sig_local_moran_vecs[0][i];
        //printf("%f,%f\n", a, b);
    }
#endif
    delete lc;
    return false;
}

