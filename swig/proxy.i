%module geoda

%include "std_string.i"

%{
#include "proxy.h"
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
%}

/**
 * is_arc: default value false -- using Euclidean distance
 *
 */

bool CreateRookWeights(std::string in_file, std::string out_file, int order=1, bool include_lower_order=false);

bool CreateQueenWeights(std::string in_file, std::string out_file, int order=1, bool include_lower_order=false);

bool CreateKNNWeights(std::string in_file, std::string out_file, int k, bool is_arc=false, bool is_mile=true);

bool CreateDistanceWeights(std::string in_file, std::string out_file, double threshold, bool is_arc=false, bool is_mile=true);
