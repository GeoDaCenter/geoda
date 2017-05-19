#include <string>
#include <vector>
#include <set>
#include <utility>
#include <algorithm>
#include <sstream>
#include <stdio.h>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/tokenzr.h>

#include "../ShapeOperations/GwtWeight.h"
#include "../ShapeOperations/GalWeight.h"
#include "../ShapeOperations/PolysToContigWeights.h"
#include "../ShapeOperations/VoronoiUtils.h"
#include "../Explore/LisaCoordinator.h"
#include "../Explore/LocalGearyCoordinator.h"
#include "../Explore/CatClassification.h"
#include "../SpatialIndAlgs.h"
#include "../GenUtils.h"
#include "../pca.h"

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

///////////////////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////////////
    // 0 univariate lisa
    // 1 bivariate lisa
    // 2 EB lisa
    // 3 diff lisa
bool LISA(std::string in_w_file, std::vector<double> var_1, std::vector<double> var_2, std::vector<double>& localMoran, std::vector<double>& sigLocalMoran, std::vector<int>& sigFlag, std::vector<int>& clusterFlag, int lisa_type, int numPermutations)
{
    wxString w_path(in_w_file);
    int num_obs = var_1.size();

    LisaCoordinator* lc = new LisaCoordinator(w_path, num_obs, var_1, var_2, lisa_type, numPermutations);
    for (int i=0; i<num_obs; i++) {
        localMoran[i] = lc->local_moran_vecs[0][i];
        sigLocalMoran[i] = lc->sig_local_moran_vecs[0][i];
        sigFlag[i] = lc->sig_cat_vecs[0][i];
        clusterFlag[i] = lc->cluster_vecs[0][i];
    }
    delete lc;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////////////
bool LocalGeary(std::string in_w_file, std::vector<std::vector<double> >& data, std::vector<double>& localGeary, std::vector<double>& sigLocalGeary, std::vector<int>& sigFlag, std::vector<int>& clusterFlag, int numPermutations)
{
    wxString w_path(in_w_file);
    int num_obs = data[0].size();

    LocalGearyCoordinator* lc = new LocalGearyCoordinator(w_path, num_obs, data, numPermutations);
    for (int i=0; i<num_obs; i++) {
        localGeary[i] = lc->local_geary_vecs[0][i];
        sigLocalGeary[i] = lc->sig_local_geary_vecs[0][i];
        sigFlag[i] = lc->sig_cat_vecs[0][i];
        clusterFlag[i] = lc->cluster_vecs[0][i];
    }
    delete lc;
}

///////////////////////////////////////////////////////////////////////////////
//
// std::vector<int> clusters
///////////////////////////////////////////////////////////////////////////////
bool 
Hinge1530(
    int type,
    int num_obs,
    const std::vector<double>& data, 
    int num_categories, 
    bool useScientificNotation,
    std::vector<double>& breaks // return results
) {
        
    CatClassification::CatClassifType cat_type = CatClassification::hinge_15;

    if (type == 1) {
        cat_type = CatClassification::hinge_30;
    }

    int n_tms = 1; // we don't support time-grouped variable for now

    CatClassifData cat_data;

    CatClassifDef cat_classif_def;

    std::vector<bool> map_valid(n_tms, true);

    std::vector<wxString> map_error_message(n_tms);

    std::vector<std::vector<std::pair<double, int> > > cat_var_sorted(n_tms);

    std::vector<std::vector<bool> > cat_var_undef(n_tms);

    for (int t=0; t<n_tms; t++) {
        for (int i=0; i<num_obs; i++) {
            double val = data[i];
            cat_var_sorted[t].push_back(std::make_pair(val, i));
            cat_var_undef[t].push_back(false);
        }

        std::sort( cat_var_sorted[t].begin(), 
            cat_var_sorted[t].end(), 
            Gda::dbl_int_pair_cmp_less);
    }

    CatClassification::ChangeNumCats(num_categories, cat_classif_def);

    cat_classif_def.color_scheme = CatClassification::GetColSchmForType(cat_type);

    cat_classif_def.cat_classif_type = cat_type;

    cat_data.CreateCategoriesAllCanvasTms(num_categories, n_tms, num_obs);

    // Update Categories based on num_cats
    CatClassification::PopulateCatClassifData(cat_classif_def,
                                              cat_var_sorted,
                                              cat_var_undef,
                                              cat_data,
                                              map_valid,
                                              map_error_message,
                                              useScientificNotation);    

    //int cnc = cat_data.GetNumCategories(cat_data.GetCurrentCanvasTmStep());
    //CatClassification::ChangeNumCats(cnc, cat_classif_def);
    
    // get results
    const std::vector<Category>& cat_vec = cat_data.categories[0].cat_vec;

    breaks.resize(cat_vec.size());

    for (int i=0; i<cat_vec.size(); i++) {
        //breaks[i] = cat_vec[i].max_val;        
        breaks[i] = cat_data.categories[0].cat_vec[i].max_val;
#ifdef DEBUG
    printf("breaks[%d]: %f\n", i, breaks[i]);
#endif
    }

    return true;
}
bool 
Hinge15(
    int num_obs,
    const std::vector<double>& data, 
    int num_categories, 
    bool useScientificNotation,
    std::vector<double>& breaks // return results
) {
    return Hinge1530(0, num_obs, data, num_categories, useScientificNotation, breaks);
}


bool 
Hinge30(
    int num_obs,
    const std::vector<double>& data, 
    int num_categories, 
    bool useScientificNotation,
    std::vector<double>& breaks // return results
) {
    return Hinge1530(1, num_obs, data, num_categories, useScientificNotation, breaks);
}


///////////////////////////////////////////////////////////////////////////////
//
// std::vector<int> clusters
///////////////////////////////////////////////////////////////////////////////
std::string 
PCA(
    std::vector<float>& x,
    std::vector<std::string>& x_names,
    int _nrows,
    int _ncols,
    int _is_corr,
    int _is_center,
    int _is_scale
) {
    Pca pca;

    bool is_corr = _is_corr == 0 ? false : true;
    bool is_center = _is_center == 0 ? false : true;
    bool is_scale = _is_scale == 0 ? false : true;
    
    int rtn = pca.Calculate(x, _nrows, _ncols, is_corr, is_center, is_scale);

    if ( 0 != rtn ) {
        // error of PCA
        return "";
    }

    vector<float> sd = pca.sd();
    vector<float> prop_of_var = pca.prop_of_var();
    vector<float> cum_prop = pca.cum_prop();
    vector<float> scores = pca.scores();
    
    vector<unsigned int> el_cols = pca.eliminated_columns();
    
    float kaiser = pca.kaiser();
    float thresh95 = pca.thresh95();
    
    unsigned int ncols = pca.ncols();
    unsigned int nrows = pca.nrows();
   
    int max_sel_name_len = 0;
 
    wxString method = pca.method();
    
    wxString pca_log;
    //pca_log << "\n\nPCA method: " << method;
    
    pca_log << "\n\nStandard deviation:\n";
    for (int i=0; i<sd.size();i++) pca_log << sd[i] << " ";

    pca_log << "\n\nProportion of variance:\n";
    for (int i=0; i<prop_of_var.size();i++) pca_log << prop_of_var[i] << " ";
    
    pca_log << "\n\nCumulative proportion:\n";
    for (int i=0; i<cum_prop.size();i++) pca_log << cum_prop[i] << " ";
    
    pca_log << "\n\nKaiser criterion: " << kaiser;
    pca_log << "\n\n95% threshold criterion: " << thresh95;
    
    pca_log << "\n\nEigenvalues:\n";
    std::stringstream ss;
    ss << pca.eigen_values;
    pca_log << ss.str();
    
    //pca_log << pca.eigen_values;
    pca_log << "\n\nEigenvectors:\n";
    
    std::stringstream ss1;
    ss1 << pca.eigen_vectors;
    wxString loadings =  ss1.str();
    wxStringTokenizer tokenizer(loadings, "\n");
    wxArrayString items;
    bool header = false;
    while ( tokenizer.HasMoreTokens() )
    {
        wxString token = tokenizer.GetNextToken();
        // process token here
        items.Add(token);
  
        if (header == false) {
            pca_log << wxString::Format("%-*s", max_sel_name_len+4, "");
            int n_len = token.length();
            int pos = 0;
            bool start = false;
            int  sub_len = 0;
            int pc_idx = 1;
            
            while (pos < n_len){
                if ( start && sub_len > 0 && (token[pos] == ' ' || pos == n_len-1) ) {
                    // end of a number
                    pca_log << wxString::Format("%*s%d", sub_len-1, "PC", pc_idx++);
                    sub_len = 1;
                    start = false;
                } else {
                    if (!start && token[pos] != ' ') {
                        start = true;
                    }
                    sub_len += 1;
                }
                pos += 1;
            }
            header = true;
            pca_log << "\n";
        }
    }
    
    for (int k=0; k<items.size();k++) {
        pca_log << wxString::Format("%-*s", max_sel_name_len+4, wxString(x_names[k])) << items[k] << "\n";
    }
    
    unsigned int row_lim;
    unsigned int col_lim; 
    
    if (scores.size() != nrows * ncols) {
        row_lim = (nrows < ncols)? nrows : ncols,
        col_lim = (ncols < nrows)? ncols : nrows;
    } else {
        row_lim = nrows;
        col_lim = ncols;
    }
    
    return string(pca_log.mb_str());
}


