#include <string>
#include <vector>

bool CreateRookWeights(std::string in_file, std::string out_file, int order=1, bool include_lower_order=false);

bool CreateQueenWeights(std::string in_file, std::string out_file, int order=1, bool include_lower_order=false);

bool CreateKNNWeights(std::string in_file, std::string out_file, int k, bool is_arc=false, bool is_mile=true);

bool CreateDistanceWeights(std::string in_file, std::string out_file, double threshold, bool is_arc=false, bool is_mile=true);

bool LISA(std::string in_w_file, std::vector<double> var_1, std::vector<double> var_2, std::vector<double>& localMoran, std::vector<double>& sigLocalMoran, std::vector<int>& sigFlag, std::vector<int>& clusterFlag, int lisa_type=0, int numPermutations=599);

bool 
LocalGeary(
    std::string in_w_file, 
    std::vector<std::vector<double> >& data, 
    std::vector<double>& localGeary, 
    std::vector<double>& sigLocalGeary, 
    std::vector<int>& sigFlag, 
    std::vector<int>& clusterFlag, 
    int numPermutations=599
);

bool 
Hinge15(
    int num_obs,
    const std::vector<double>& data, 
    int num_categories, 
    bool useScientificNotation,
    std::vector<double>& breaks // return results
);

bool 
Hinge30(
    int num_obs,
    const std::vector<double>& data, 
    int num_categories, 
    bool useScientificNotation,
    std::vector<double>& breaks // return results
);

std::string 
PCA(
    std::vector<float>& x,
    std::vector<std::string>& x_names,
    int nrows,
    int ncols,
    int is_corr,
    int is_center,
    int is_scale
);
