/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 * 
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cfloat>
#include <iomanip>
#include <limits>
#include <math.h>
#include <sstream>
#include <boost/math/distributions/students_t.hpp>
#include <boost/thread.hpp>

#include <wx/dc.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <wx/regex.h>

#include "GdaConst.h"
#include "GenUtils.h"
#include "Explore/CatClassification.h"

int StringUtils::utf8_strlen(const std::string& str)
{
    int c,i,q;
    for (q=0, i=0; i < str.length(); i++, q++)
    {
        c = (unsigned char) str[i];
        if      (c>=0   && c<=127) i+=0;
        else if ((c & 0xE0) == 0xC0) i+=1;
        else if ((c & 0xF0) == 0xE0) i+=2;
        else if ((c & 0xF8) == 0xF0) i+=3;
        //else if (($c & 0xFC) == 0xF8) i+=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
        //else if (($c & 0xFE) == 0xFC) i+=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
        else return 0;//invalid utf8
    }
    return q;
}

void DbfFileUtils::SuggestDoubleParams(int length, int decimals,
                                       int* suggest_len, int* suggest_dec)
{
    // doubles have 52 bits for the mantissa, so we can allow at most
    // floor(log(2^52)) = 15 digits of precision.
    // We require that there length-2 >= decimals to allow for "x." . when
    // writing to disk, and when decimals = 15, require length >= 17 to
    // allow for "0." prefex. If length-2 == decimals, then negative numbers
    // are not allowed since there is not room for the "-0." prefix.
    if (GdaConst::max_dbf_double_len < length) {
        length = GdaConst::max_dbf_double_len;
    }
    if (length < 3) length = 3;
    if (decimals < 1) decimals = 1;
    if (decimals > 15) decimals = 15;
    if (length-2 < decimals) length = decimals + 2;
    
    *suggest_len = length;
    *suggest_dec = decimals;
}

double DbfFileUtils::GetMaxDouble(int length, int decimals,
                                  int* suggest_len, int* suggest_dec)
{
    // make sure that length and decimals have legal values
    SuggestDoubleParams(length, decimals, &length, &decimals);
    
    int len_inter = length - (1+decimals);
    //if (len_inter + decimals > 15) len_inter = 15-decimals;
    double r = 0;
    for (int i=0; i<len_inter+decimals; i++) r = r*10 + 9;
    for (int i=0; i<decimals; i++) r /= 10;
    
    if (suggest_len) *suggest_len = length;
    if (suggest_dec) *suggest_dec = decimals;
    return r;
}

wxString DbfFileUtils::GetMaxDoubleString(int length, int decimals)
{
    double x = GetMaxDouble(length, decimals, &length, &decimals);
    return wxString::Format("%.*f", decimals, x);
}

double DbfFileUtils::GetMinDouble(int length, int decimals,
                                  int* suggest_len, int* suggest_dec)
{
    SuggestDoubleParams(length, decimals, &length, &decimals);
    if (length-2 == decimals) return 0;
    if (suggest_len) *suggest_len = length;
    if (suggest_dec) *suggest_dec = decimals;
    return -DbfFileUtils::GetMaxDouble(length-1, decimals);
}

wxString DbfFileUtils::GetMinDoubleString(int length, int decimals)
{
    double x = GetMinDouble(length, decimals, &length, &decimals);
    if (length-2 == decimals) {
        wxString s("0.");
        for (int i=0; i<decimals; i++) s += "0";
        return s;
    }
    return wxString::Format("%.*f", decimals, x);
}

wxInt64 DbfFileUtils::GetMaxInt(int length)
{
    // We want to allow the user to enter a string of
    // all 9s for the largest value reported.  So, we must
    // limit the length of the string to be floor(log(2^63)) = 18
    if (length < 1) return 0;
    if (length > 18) length = 18;
    wxInt64 r=0;
    for (int i=0; i<length; i++) r = r*10 + 9;
    return r;
}

wxString DbfFileUtils::GetMaxIntString(int length)
{
    if (length < 19)
        return wxString::Format("%lld", GetMaxInt(length));
    else
        return "9223372036854775807"; // max value of int64
}

wxInt64 DbfFileUtils::GetMinInt(int length)
{
    // This is generally the -GetMaxInt(length-1), because we must
    // allow one character for the minus sign unless the length
    // is greater than 18;
    if (length > 19) length = 19;
    return -GetMaxInt(length-1);
}

wxString DbfFileUtils::GetMinIntString(int length)
{
    if (length < 19)
        return wxString::Format("%lld", GetMinInt(length));
    else
        return "-9223372036854775808"; // min value of int64
}

wxString Gda::DetectDateFormat(wxString s, std::vector<wxString>& date_items)
{
    // input s could be sth. like: %Y-%m-%d %H:%M:%S
    // 2015-1-11 13:57:24 %Y-%m-%d %H:%M:%S
    wxString YY = "([0-9]{4})";
    wxString yy = "([0-9]{2})";
    wxString MM = "([0-9]{1,2})";//"(0?[1-9]|1[0-2])";
    wxString DD = "([0-9]{1,2})";//"(0?[1-9]|[12][0-9]|3[01])";
    wxString ii = "([0-9]{1,2})";//"(00|[0-9]|1[0-9]|2[0-3])";
    wxString hh = "([0-9]{1,2})";//"(00|[0-9]|1[0-9]|2[0-3])";
    wxString mm = "([0-9]{1,2})";//"([0-9]|[0-5][0-9])";
    wxString ss = "([0-9]{1,2})"; //"([0-9]|[0-5][0-9])";
    wxString pp = "([AP]M)"; //"(AM | PM)";

    wxString pattern;
    wxString original_pattern;
    for (int i=0; i<GdaConst::gda_datetime_formats.size(); i++) {
        original_pattern = GdaConst::gda_datetime_formats[i];
        wxString select_pattern = original_pattern;
        select_pattern.Replace("%Y", YY);
        select_pattern.Replace("%y", yy);
        select_pattern.Replace("%m", MM);
        select_pattern.Replace("%d", DD);
        select_pattern.Replace("%I", ii);
        select_pattern.Replace("%H", hh);
        select_pattern.Replace("%M", mm);
        select_pattern.Replace("%S", ss);
        select_pattern.Replace("%p", pp);
      
        select_pattern = "^" + select_pattern + "$";
        
        wxRegEx regex(select_pattern);
        if (regex.IsValid()) {
            if (regex.Matches(s)) {
                if (regex.GetMatchCount()>0) {
                    pattern = select_pattern;
                    break;
                }
            }
        }
    }
   
    if (!pattern.IsEmpty()){
        wxString select_pattern = original_pattern;
        wxRegEx regex("(%[YymdIHMSp])");
        while (regex.Matches(select_pattern) ) {
            size_t start, len;
            regex.GetMatch(&start, &len, 0);
            date_items.push_back(regex.GetMatch(select_pattern, 1));
            select_pattern = select_pattern.Mid (start + len);
        }
    }
    return pattern;
}

// wxRegEx regex
// regex.Compile(pattern);
unsigned long long Gda::DateToNumber(wxString s_date, wxRegEx& regex, std::vector<wxString>& date_items)
{
    unsigned long long val = 0;
        
    if (regex.Matches(s_date)) {
        int n = (int)regex.GetMatchCount();
        wxString _year, _short_year, _month, _day, _hour, _minute, _second, _am_pm;
        long _l_year =0,  _l_short_year=0, _l_month=0, _l_day=0, _l_hour=0, _l_minute=0, _l_second=0;
        for (int i=1; i<n; i++) {
            if (date_items[i-1] == "%Y") {
                _year = regex.GetMatch(s_date, i);
                _year.ToLong(&_l_year);
            } else if (date_items[i-1] == "%y") {
                _short_year = regex.GetMatch(s_date, i);
                if( _short_year.ToLong(&_l_short_year)) {
                    _l_year = _l_short_year < 50 ? 2000 + _l_short_year : 1900 + _l_short_year;
                }
            } else if (date_items[i-1] == "%m") {
                _month = regex.GetMatch(s_date, i);
                _month.ToLong(&_l_month);
            } else if (date_items[i-1] == "%d") {
                _day = regex.GetMatch(s_date, i);
                _day.ToLong(&_l_day);
            } else if (date_items[i-1] == "%H" || date_items[i-1] == "%I") {
                _hour = regex.GetMatch(s_date, i);
                _hour.ToLong(&_l_hour);
            } else if (date_items[i-1] == "%M") {
                _minute = regex.GetMatch(s_date, i);
                _minute.ToLong(&_l_minute);
            } else if (date_items[i-1] == "%S") {
                _second = regex.GetMatch(s_date, i);
                _second.ToLong(&_l_second);
            } else if (date_items[i-1] == "%p") {
                _am_pm = regex.GetMatch(s_date, i);
            }
        }
        if (!_am_pm.IsEmpty()) {
            if (_am_pm.CmpNoCase("PM") == 0) {
                _l_hour += 12;
            }
        }
        val = _l_year * 10000000000 + _l_month * 100000000 + _l_day * 1000000 + _l_hour * 10000 + _l_minute * 100 + _l_second;
        
    }
    return val;
}

void GdaColorUtils::GetUnique20Colors(std::vector<wxColour>& colors)
{
    colors.clear();
    colors.push_back(wxColour(166,206,227));
    colors.push_back(wxColour(31,120,180));
    colors.push_back(wxColour(178,223,138));
    colors.push_back(wxColour(51,160,44));
    colors.push_back(wxColour(251,154,153));
    colors.push_back(wxColour(227,26,28));
    colors.push_back(wxColour(253,191,111));
    colors.push_back(wxColour(255,127,0));
    colors.push_back(wxColour(106,61,154));
    colors.push_back(wxColour(255,255,153));
    colors.push_back(wxColour(177,89,40));
    colors.push_back(wxColour(255,255,179));
    colors.push_back(wxColour(190,186,218));
    colors.push_back(wxColour(251,128,114));
    colors.push_back(wxColour(128,177,211));
    colors.push_back(wxColour(179,222,105));
    colors.push_back(wxColour(252,205,229));
    colors.push_back(wxColour(217,217,217));
    colors.push_back(wxColour(188,128,189));
    colors.push_back(wxColour(204,235,197));
};

void GdaColorUtils::GetLISAColors(std::vector<wxColour>& colors)
{
    colors.clear();
    colors.push_back(wxColour(240, 240, 240));
    colors.push_back(wxColour(255, 0, 0));
    colors.push_back(wxColour(0, 0, 255));
    colors.push_back(wxColour(150, 150, 255));
    colors.push_back(wxColour(255, 150, 150));
}

void GdaColorUtils::GetLISAColorLabels(std::vector<wxString>& labels)
{
    labels.clear();
    labels.push_back(GdaConst::gda_lbl_not_sig);
    labels.push_back(GdaConst::gda_lbl_highhigh);
    labels.push_back(GdaConst::gda_lbl_lowlow);
    labels.push_back(GdaConst::gda_lbl_lowhigh);
    labels.push_back(GdaConst::gda_lbl_highlow);
}

void GdaColorUtils::GetLocalGColors(std::vector<wxColour>& colors)
{
    colors.clear();
    colors.push_back(wxColour(240, 240, 240));
    colors.push_back(wxColour(255, 0, 0));
    colors.push_back(wxColour(0, 0, 255));
}
void GdaColorUtils::GetLocalGColorLabels(std::vector<wxString>& labels)
{
    labels.clear();
    labels.push_back(GdaConst::gda_lbl_not_sig);
    labels.push_back(GdaConst::gda_lbl_highhigh);
    labels.push_back(GdaConst::gda_lbl_lowlow);
}

void GdaColorUtils::GetLocalJoinCountColors(std::vector<wxColour>& colors)
{
    colors.clear();
    colors.push_back(wxColour(240, 240, 240));
    colors.push_back(wxColour(255, 0, 0));
}
void GdaColorUtils::GetLocalJoinCountColorLabels(std::vector<wxString>& labels)
{
    labels.clear();
    labels.push_back(GdaConst::gda_lbl_not_sig);
    labels.push_back(GdaConst::gda_lbl_highhigh);
}

void GdaColorUtils::GetLocalGearyColors(std::vector<wxColour>& colors)
{
    colors.clear();
    colors.push_back(wxColour(240, 240, 240));
    colors.push_back(wxColour(178,24,43));
    colors.push_back(wxColour(239,138,98));
    colors.push_back(wxColour(253,219,199));
    colors.push_back(wxColour(103,173,199));
}
void GdaColorUtils::GetLocalGearyColorLabels(std::vector<wxString>& labels)
{
    labels.clear();
    labels.push_back(GdaConst::gda_lbl_not_sig);
    labels.push_back(GdaConst::gda_lbl_highhigh);
    labels.push_back(GdaConst::gda_lbl_lowlow);
    labels.push_back(GdaConst::gda_lbl_otherpos);
    labels.push_back(GdaConst::gda_lbl_negative);
}

void GdaColorUtils::GetMultiLocalGearyColors(std::vector<wxColour>& colors)
{
    colors.clear();
    colors.push_back(wxColour(240, 240, 240));
    colors.push_back(wxColour(51,110,161));
}
void GdaColorUtils::GetMultiLocalGearyColorLabels(std::vector<wxString>& labels)
{
    labels.clear();
    labels.push_back(GdaConst::gda_lbl_not_sig);
    labels.push_back(GdaConst::gda_lbl_positive);
}

void GdaColorUtils::GetPercentileColors(std::vector<wxColour>& colors)
{
    colors.clear();
    CatClassification::PickColorSet(colors, CatClassification::diverging_color_scheme, 6, false);
    colors.insert(colors.begin(), wxColour(240, 240, 240));
}
void GdaColorUtils::GetPercentileColorLabels(std::vector<wxString>& labels)
{
    labels.clear();
    labels.push_back(GdaConst::gda_lbl_1p);
    labels.push_back(GdaConst::gda_lbl_1p_10p);
    labels.push_back(GdaConst::gda_lbl_10p_50p);
    labels.push_back(GdaConst::gda_lbl_50p_90p);
    labels.push_back(GdaConst::gda_lbl_90p_99p);
    labels.push_back(GdaConst::gda_lbl_99p);
}

void GdaColorUtils::GetBoxmapColors(std::vector<wxColour>& colors)
{
    colors.clear();
    CatClassification::PickColorSet(colors, CatClassification::diverging_color_scheme, 6, false);
    colors.insert(colors.begin(), wxColour(240, 240, 240));
}
void GdaColorUtils::GetBoxmapColorLabels(std::vector<wxString>& labels)
{
    labels.clear();
    labels.push_back(GdaConst::gda_lbl_loweroutlier);
    labels.push_back(GdaConst::gda_lbl_25p);
    labels.push_back(GdaConst::gda_lbl_25p_50p);
    labels.push_back(GdaConst::gda_lbl_50p_75p);
    labels.push_back(GdaConst::gda_lbl_75p);
    labels.push_back(GdaConst::gda_lbl_upperoutlier);
}

void GdaColorUtils::GetStddevColors(std::vector<wxColour>& colors)
{
    colors.clear();
    CatClassification::PickColorSet(colors, CatClassification::diverging_color_scheme, 6, false);
    colors.insert(colors.begin(), wxColour(240, 240, 240));
}
void GdaColorUtils::GetStddevColorLabels(std::vector<wxString>& labels)
{
    labels.clear();
    labels.push_back(GdaConst::gda_lbl_n2sigma);
    labels.push_back(GdaConst::gda_lbl_n2sigma_n1sigma);
    labels.push_back(GdaConst::gda_lbl_n1sigma);
    labels.push_back(GdaConst::gda_lbl_1sigma);
    labels.push_back(GdaConst::gda_lbl_1sigma_2sigma);
    labels.push_back(GdaConst::gda_lbl_2sigma);
}

void GdaColorUtils::GetQuantile2Colors(std::vector<wxColour>& colors)
{
    colors.clear();
    CatClassification::PickColorSet(colors, CatClassification::sequential_color_scheme, 2, false);
    colors.insert(colors.begin(), wxColour(240, 240, 240));
}
void GdaColorUtils::GetQuantile3Colors(std::vector<wxColour>& colors)
{
    colors.clear();
    CatClassification::PickColorSet(colors, CatClassification::sequential_color_scheme, 3, false);
    colors.insert(colors.begin(), wxColour(240, 240, 240));
    
}
void GdaColorUtils::GetQuantile4Colors(std::vector<wxColour>& colors)
{
    colors.clear();
    CatClassification::PickColorSet(colors, CatClassification::sequential_color_scheme, 4, false);
    colors.insert(colors.begin(), wxColour(240, 240, 240));
    
}
void GdaColorUtils::GetQuantile5Colors(std::vector<wxColour>& colors)
{
    colors.clear();
    CatClassification::PickColorSet(colors, CatClassification::sequential_color_scheme, 5, false);
    colors.insert(colors.begin(), wxColour(240, 240, 240));
    
}
void GdaColorUtils::GetQuantile6Colors(std::vector<wxColour>& colors)
{
    colors.clear();
    CatClassification::PickColorSet(colors, CatClassification::sequential_color_scheme, 6, false);
    colors.insert(colors.begin(), wxColour(240, 240, 240));
    
}
void GdaColorUtils::GetQuantile7Colors(std::vector<wxColour>& colors)
{
    colors.clear();
    CatClassification::PickColorSet(colors, CatClassification::sequential_color_scheme, 7, false);
    colors.insert(colors.begin(), wxColour(240, 240, 240));
    
}
void GdaColorUtils::GetQuantile8Colors(std::vector<wxColour>& colors)
{
    colors.clear();
    CatClassification::PickColorSet(colors, CatClassification::sequential_color_scheme, 8, false);
    colors.insert(colors.begin(), wxColour(240, 240, 240));
    
}
void GdaColorUtils::GetQuantile9Colors(std::vector<wxColour>& colors)
{
    colors.clear();
    CatClassification::PickColorSet(colors, CatClassification::sequential_color_scheme, 9, false);
    colors.insert(colors.begin(), wxColour(240, 240, 240));
}
void GdaColorUtils::GetQuantile10Colors(std::vector<wxColour>& colors)
{
    colors.clear();
    CatClassification::PickColorSet(colors, CatClassification::sequential_color_scheme, 10, false);
    colors.insert(colors.begin(), wxColour(240, 240, 240));
}

wxString GdaColorUtils::ToHexColorStr(const wxColour& c)
{
	return c.GetAsString(wxC2S_HTML_SYNTAX);
}

wxColour GdaColorUtils::ChangeBrightness(const wxColour& input_col,
										 int brightness)
{
	unsigned char r = input_col.Red(); 
	unsigned char g = input_col.Green();
	unsigned char b = input_col.Blue();
	unsigned char alpha = input_col.Alpha();
	wxColour::ChangeLightness(&r, &g, &b, brightness);
	return wxColour(r,g,b,alpha);
}

uint64_t Gda::ThomasWangHashUInt64(uint64_t key) {
	key = (~key) + (key << 21); // key = (key << 21) - key - 1;
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8); // key * 265
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4); // key * 21
	key = key ^ (key >> 28);
	key = key + (key << 31);
	return key;
}

double Gda::ThomasWangHashDouble(uint64_t key) {
	key = (~key) + (key << 21); // key = (key << 21) - key - 1;
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8); // key * 265
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4); // key * 21
	key = key ^ (key >> 28);
	key = key + (key << 31);
	return 5.42101086242752217E-20 * key;
}

double Gda::ThomasWangDouble(uint64_t& key) {
	key = (~key) + (key << 21); // key = (key << 21) - key - 1;
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8); // key * 265
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4); // key * 21
	key = key ^ (key >> 28);
	key = key + (key << 31);
	return 5.42101086242752217E-20 * key;
}

uint64_t Gda::factorial(unsigned int n)
{
    uint64_t r = 1;
    for(size_t i = n-1; i > 1; i--)
        r *= i;
    
    return r;
}

double Gda::combinatorial(unsigned int n, unsigned int k) {
   
    double r = 1;
    double s = 1;
    
    size_t i;
    int kk = k > n/2 ? k : n-k;
    
    for(i=n; i > kk; i--) r *= i;
    
    for(i=(n-kk); i>0; i--) s *= i;
    
    return r / s;
}

wxString Gda::CreateUUID(int nSize)
{
    if (nSize < 0 || nSize >= 38)
        nSize = 8;

    wxString letters = "abcdefghijklmnopqrstuvwxyz0123456789";
    
    srand ((unsigned int)time(NULL));
    
    wxString uid;
    while (uid.length() < nSize) {
        int iSecret = rand() % letters.size();
        uid += letters[iSecret];
    }
    return uid;
}

/** Use with std::sort for sorting in ascending order */
bool Gda::dbl_int_pair_cmp_less(const dbl_int_pair_type& ind1,
								  const dbl_int_pair_type& ind2)
{
	return ind1.first < ind2.first;
}

/** Use with std::sort for sorting in descending order */
bool Gda::dbl_int_pair_cmp_greater(const dbl_int_pair_type& ind1,
									 const dbl_int_pair_type& ind2)
{
	return ind1.first > ind2.first;
}

/** Use with std::sort for sorting in ascending order */
bool Gda::dbl_int_pair_cmp_second_less(const dbl_int_pair_type& ind1,
										 const dbl_int_pair_type& ind2)
{
	return ind1.second < ind2.second;
}

/** Use with std::sort for sorting in descending order */
bool Gda::dbl_int_pair_cmp_second_greater(const dbl_int_pair_type& ind1,
											const dbl_int_pair_type& ind2)
{
	return ind1.second > ind2.second;
}


void
HingeStats::
CalculateHingeStats(const std::vector<Gda::dbl_int_pair_type>& data)
{
	num_obs = (int)data.size();
	double N = num_obs;
	is_even_num_obs = (num_obs % 2) == 0;
	min_val = data[0].first;
	max_val = data[num_obs-1].first;
	Q2_ind = (N+1)/2.0 - 1;
	if (is_even_num_obs) {
		Q1_ind = (N+2)/4.0 - 1;
		Q3_ind = (3*N+2)/4.0 - 1;
	} else {
		Q1_ind = (N+3)/4.0 - 1;
		Q3_ind = (3*N+1)/4.0 - 1;
	}
	Q1 = (data[(int) floor(Q1_ind)].first +
		  data[(int) ceil(Q1_ind)].first)/2.0;
	Q2 = (data[(int) floor(Q2_ind)].first +
		  data[(int) ceil(Q2_ind)].first)/2.0;
	Q3 = (data[(int) floor(Q3_ind)].first +
		  data[(int) ceil(Q3_ind)].first)/2.0;
	IQR = Q3 - Q1;
	extreme_lower_val_15 = Q1 - 1.5*IQR;
	extreme_lower_val_30 = Q1 - 3.0*IQR;
	extreme_upper_val_15 = Q3 + 1.5*IQR;
	extreme_upper_val_30 = Q3 + 3.0*IQR;
	min_IQR_ind = -1;
	for (int i=0; i<num_obs; i++) {
		if (data[i].first < Q1) min_IQR_ind = i;
		else break;
	}
	if (min_IQR_ind < num_obs-1) min_IQR_ind++;
	max_IQR_ind = num_obs;
	for (int i=num_obs-1; i>=0; i--) {
		if (data[i].first > Q3) max_IQR_ind = i;
		else break;
	}
	if (max_IQR_ind > 0) max_IQR_ind--;
}

void
HingeStats::
CalculateHingeStats(const std::vector<Gda::dbl_int_pair_type>& data,
                    const std::vector<bool>& data_undef)
{
    num_obs = (int)data.size();
    double N = 0.0;
    std::vector<double> data_valid;
    
    bool has_init = false;
    for (size_t i =0; i<num_obs; i++) {
        int obs_idx = data[i].second;
        if (!data_undef[obs_idx]) {
            double val = data[i].first;
            data_valid.push_back(val); // sorted
            if (!has_init) {
                min_val = val;
                max_val = val;
                has_init = true;
            }
            if (val < min_val)
                min_val = val;
            if (val > max_val)
                max_val = val;
        }
    }
    
    N = data_valid.size();
    
    is_even_num_obs = (data_valid.size() % 2) == 0;
    
    Q2_ind = (N+1)/2.0 - 1;
    if (is_even_num_obs) {
        Q1_ind = (N+2)/4.0 - 1;
        Q3_ind = (3*N+2)/4.0 - 1;
    } else {
        Q1_ind = (N+3)/4.0 - 1;
        Q3_ind = (3*N+1)/4.0 - 1;
    }
    
    if (N == 0 || N < Q3_ind) return;
    
    Q1 = (data_valid[(int) floor(Q1_ind)] + data_valid[(int) ceil(Q1_ind)])/2.0;
    Q2 = (data_valid[(int) floor(Q2_ind)] + data_valid[(int) ceil(Q2_ind)])/2.0;
    Q3 = (data_valid[(int) floor(Q3_ind)] + data_valid[(int) ceil(Q3_ind)])/2.0;
    
    IQR = Q3 - Q1;
    
    extreme_lower_val_15 = Q1 - 1.5*IQR;
    extreme_lower_val_30 = Q1 - 3.0*IQR;
    extreme_upper_val_15 = Q3 + 1.5*IQR;
    extreme_upper_val_30 = Q3 + 3.0*IQR;
    
    min_IQR_ind = -1;
    for (int i=0; i<num_obs; i++) {
        if (data[i].first < Q1) {
            min_IQR_ind = i;
        }
        else
            break;
    }
    if (min_IQR_ind < num_obs-1) {
        min_IQR_ind++;
    }
    max_IQR_ind = num_obs;
    
    for (int i=num_obs-1; i>=0; i--) {
        if (data[i].first > Q3) {
            max_IQR_ind = i;
        }
        else
            break;
    }
    if (max_IQR_ind > 0)
        max_IQR_ind--;
}

// Assume input v is sorted.  If not, can sort
// with std::sort(v.begin(), v.end())
// Testing: for v = {15, 20, 35, 40, 50},
// percentile(1, v) = 15, percentile(10, v) = 15, percentile(11) = 15.25
// percentile(50, v) = 35, percentile(89, v) = 49.5,
// percentile(90, v) = 50, percentile(99, v) = 50
double Gda::percentile(double x, const std::vector<double>& v)
{
	int N = (int)v.size();
	double Nd = (double) N;
	double p_0 = (100.0/Nd) * (1.0-0.5);
	double p_Nm1 = (100.0/Nd) * (Nd-0.5);
    
    if (v.empty()) return 0;
    
	if (x <= p_0) return v[0];
	if (x >= p_Nm1) return v[N-1];
	
	for (int i=1; i<N; i++) {
		double p_i = (100.0/Nd) * ((((double) i)+1.0)-0.5);
		if (x == p_i) return v[i];
		if (x < p_i) {
			double p_im1 = (100.0/Nd) * ((((double) i))-0.5);
			return v[i-1] + Nd*((x-p_im1)/100.0)*(v[i]-v[i-1]);
		}
	}
	return v[N-1]; // execution should never get here
}

// Same assumptions as above
double Gda::percentile(double x, const Gda::dbl_int_pair_vec_type& v,
                       const std::vector<bool>& undefs)
{
    std::vector<double> valid_data;
    for (size_t i = 0; i<v.size(); i++ ) {
        double val = v[i].first;
        int ind = v[i].second;
        
        if (undefs[ind])
            continue;
        
        valid_data.push_back(val);
    }
    return percentile(x, valid_data);
}

// Same assumptions as above
double Gda::percentile(double x, const Gda::dbl_int_pair_vec_type& v)
{
	int N = (int)v.size();
	double Nd = (double) N;
	double p_0 = (100.0/Nd) * (1.0-0.5);
	double p_Nm1 = (100.0/Nd) * (Nd-0.5);
    
	if (x <= p_0)
        return v[0].first;
    
	if (x >= p_Nm1)
        return v[N-1].first;
	
	for (int i=1; i<N; i++) {
		double p_i = (100.0/Nd) * ((((double) i)+1.0)-0.5);
		if (x == p_i)
            return v[i].first;
		if (x < p_i) {
			double p_im1 = (100.0/Nd) * ((((double) i))-0.5);
			return v[i-1].first + Nd*((x-p_im1)/100.0)*(v[i].first-v[i-1].first);
		}
	}
	return v[N-1].first; // execution should never get here
}


SampleStatistics::SampleStatistics()
	 : sample_size(0), min(0), max(0), mean(0),
    var_with_bessel(0), var_without_bessel(0),
    sd_with_bessel(0), sd_without_bessel(0)
{
}

SampleStatistics::SampleStatistics(const std::vector<double>& data)
	: sample_size(0), min(0), max(0), mean(0),
	var_with_bessel(0), var_without_bessel(0),
	sd_with_bessel(0), sd_without_bessel(0)
{
	CalculateFromSample(data);
}

SampleStatistics::SampleStatistics(const std::vector<double>& data,
                                   const std::vector<bool>& undefs)
	: sample_size(0), min(0), max(0), mean(0),
	var_with_bessel(0), var_without_bessel(0),
	sd_with_bessel(0), sd_without_bessel(0)
{
    std::vector<double> valid_data;
    for (int i=0; i<data.size(); i++) {
        if (undefs[i] == false)
            valid_data.push_back(data[i]);
    }
	CalculateFromSample(valid_data);
}

SampleStatistics::SampleStatistics(const std::vector<double>& data,
                                   const std::vector<bool>& undefs1,
                                   const std::vector<bool>& undefs2)
	: sample_size(0), min(0), max(0), mean(0),
	var_with_bessel(0), var_without_bessel(0),
	sd_with_bessel(0), sd_without_bessel(0)
{
    std::vector<double> valid_data;
    for (int i=0; i<data.size(); i++) {
        if (undefs1[i] || undefs2[i])
            continue;
        valid_data.push_back(data[i]);
    }
	CalculateFromSample(valid_data);
}

void SampleStatistics::CalculateFromSample(const std::vector<double>& data,
                                           const std::vector<bool>& undefs)
{
    std::vector<double> valid_data;
    for (int i=0; i<data.size(); i++) {
        if (undefs[i] == false)
            valid_data.push_back(data[i]);
    }
    CalculateFromSample(valid_data);
}
void SampleStatistics::CalculateFromSample(const std::vector<double>& data)
{
	sample_size = (int)data.size();
	if (sample_size == 0) return;

	CalcMinMax(data, min, max);
	mean = CalcMean(data);
	
	double n = sample_size;
	double sum_squares = 0;
	for (int i=0; i<data.size(); i++) {
		sum_squares += data[i] * data[i];
	}
	
	var_without_bessel = (sum_squares/n) - (mean*mean);
	sd_without_bessel = sqrt(var_without_bessel);
	
	if (sample_size == 1) {
		var_with_bessel = var_without_bessel;
		sd_with_bessel = sd_without_bessel;
	} else {
		var_with_bessel = (n/(n-1)) * var_without_bessel;
		sd_with_bessel = sqrt(var_with_bessel);
	}
}

/** We assume that the data has been sorted in ascending order */
void
SampleStatistics::
CalculateFromSample(const std::vector<Gda::dbl_int_pair_type>& data_,
                    const std::vector<bool>& undefs)
{
    std::vector<double> data;
    for (int i=0; i<data_.size(); i++) {
        int id = data_[i].second;
        if (!undefs[id]) {
            data.push_back(data_[i].first);
        }
    }
    
	sample_size = (int)data.size();
	if (sample_size == 0) return;
	
	min = data[0];
	max = data[sample_size-1];
	mean = CalcMean(data);
	
	double n = sample_size;
	double sum_squares = 0;
	for (int i=0; i<data.size(); i++) {
		sum_squares += data[i] * data[i];
	}
	
	var_without_bessel = (sum_squares/n) - (mean*mean);
	sd_without_bessel = sqrt(var_without_bessel);
	
	if (sample_size == 1) {
		var_with_bessel = var_without_bessel;
		sd_with_bessel = sd_without_bessel;
	} else {
		var_with_bessel = (n/(n-1)) * var_without_bessel;
		sd_with_bessel = sqrt(var_with_bessel);
	}
}

std::string SampleStatistics::ToString()
{
    std::ostringstream ss;
	ss << "sample_size = " << sample_size << std::endl;
	ss << "min = " << min << std::endl;
	ss << "max = " << max << std::endl;
	ss << "mean = " << mean << std::endl;
	ss << "var_with_bessel = " << var_with_bessel << std::endl;
	ss << "var_without_bessel = " << var_without_bessel << std::endl;
	ss << "sd_with_bessel = " << sd_with_bessel << std::endl;
	ss << "sd_without_bessel = " << sd_without_bessel << std::endl;
	return ss.str();
}

double SampleStatistics::CalcMin(const std::vector<double>& data)
{
	double min = std::numeric_limits<double>::max();
	for (int i=0; i<data.size(); i++) {
		if ( data[i] < min ) min = data[i];
	}
	return min;
}

double SampleStatistics::CalcMax(const std::vector<double>& data)
{
	double max = -std::numeric_limits<double>::max();
	for (int i=0; i<data.size(); i++) {
		if ( data[i] > max ) max = data[i];
	}
	return max;
}

void SampleStatistics::CalcMinMax(const std::vector<double>& data,
								  double& min, double& max)
{
	if (data.size() == 0) return;
	min = data[0];
	max = data[0];
	for (int i=1; i<data.size(); i++) {
		if ( data[i] < min ) {
			min = data[i];
		} else if ( data[i] > max ) {
			max = data[i];
		}
	}
}


double SampleStatistics::CalcMean(const std::vector<double>& data)
{
	if (data.size() == 0) return 0;
	double total = 0;
	for (int i=0; i<data.size(); i++) {
		total += data[i];
	}
	return total / (double) data.size();
}

double SampleStatistics::CalcMean(const std::vector<Gda::dbl_int_pair_type>& data)
{
	if (data.size() == 0) return 0;
	double total = 0;
	for (int i=0; i<data.size(); i++) {
		total += data[i].first;
	}
	return total / (double) data.size();
}

SimpleLinearRegression::SimpleLinearRegression(const std::vector<double>& X,
											   const std::vector<double>& Y,
											   double meanX, double meanY,
											   double varX, double varY)
	: n(0), covariance(0), correlation(0), alpha(0), beta(0), r_squared(0),
	std_err_of_estimate(0), std_err_of_beta(0), std_err_of_alpha(0),
	t_score_alpha(0), t_score_beta(0), p_value_alpha(0), p_value_beta(0),
	valid(false), valid_correlation(false), valid_std_err(false),
	error_sum_squares(0)
{
	CalculateRegression(X, Y, meanX, meanY, varX, varY);
}

SimpleLinearRegression::SimpleLinearRegression(const std::vector<double>& X,
											   const std::vector<double>& Y,
                                               const std::vector<bool>& X_undef,
                                               const std::vector<bool>& Y_undef,
											   double meanX, double meanY,
											   double varX, double varY)
	: n(0), covariance(0), correlation(0), alpha(0), beta(0), r_squared(0),
	std_err_of_estimate(0), std_err_of_beta(0), std_err_of_alpha(0),
	t_score_alpha(0), t_score_beta(0), p_value_alpha(0), p_value_beta(0),
	valid(false), valid_correlation(false), valid_std_err(false),
	error_sum_squares(0)
{
    
    std::vector<double> X_valid;
    std::vector<double> Y_valid;
    
    for (int i=0; i<X.size(); i++) {
        if (X_undef[i] || Y_undef[i])
            continue;
        
        X_valid.push_back(X[i]);
        Y_valid.push_back(Y[i]);
    }
	CalculateRegression(X_valid, Y_valid, meanX, meanY, varX, varY);
}

void SimpleLinearRegression::CalculateRegression(const std::vector<double>& X,
												 const std::vector<double>& Y,
												 double meanX, double meanY,
												 double varX, double varY)
{
    n = (int)X.size();
	if (X.size() != Y.size() || X.size() < 2 )
        return;
	double expectXY = 0;
	for (int i=0; i<X.size(); i++) {
		expectXY += X[i]*Y[i];
	}
	expectXY /= (double) X.size();
	covariance = expectXY - meanX * meanY;
	if (varX > 4*DBL_MIN) {
		beta = covariance / varX;
		alpha = meanY - beta * meanX;
		valid = true;
	}
	double SS_tot = varY*Y.size();
	error_sum_squares = 0; // error_sum_squares = SS_err
	double err=0;
	for (int i=0; i<Y.size(); i++) {
		err = Y[i] - (alpha + beta * X[i]);
		error_sum_squares += err * err;
	}
	if (error_sum_squares < 16*DBL_MIN) {
		r_squared = 1;
	} else {
		r_squared = 1 - error_sum_squares / SS_tot;
	}
	
	if (Y.size()>2 && varX > 4*DBL_MIN) {
		// error_sum_squares/(n-k-1), k=1
		std_err_of_estimate = error_sum_squares/(Y.size()-2); 
		std_err_of_estimate = sqrt(std_err_of_estimate);
		std_err_of_beta = std_err_of_estimate/sqrt(X.size()*varX);
		double sum_x_squared = 0;
		for (int i=0; i<X.size(); i++) {
			sum_x_squared += X[i] * X[i];
		}
		std_err_of_alpha = std_err_of_beta * sqrt(sum_x_squared / X.size());
		
		if (std_err_of_alpha >= 16*DBL_MIN) {
			t_score_alpha = alpha / std_err_of_alpha;
		} else {
			t_score_alpha = 100;
		}
		if (std_err_of_beta >= 16*DBL_MIN) {
			t_score_beta = beta	/ std_err_of_beta;
		} else {
			t_score_beta = 100;
		}
		p_value_alpha = TScoreTo2SidedPValue(t_score_alpha, (int)X.size()-2);
		p_value_beta = TScoreTo2SidedPValue(t_score_beta, (int)X.size()-2);
		
		valid_std_err = true;
	}
	
	double d = sqrt(varX)*sqrt(varY);
	if (d > 4*DBL_MIN) {
		correlation = covariance / d;
		valid_correlation = true;
	}
}

double SimpleLinearRegression::TScoreTo2SidedPValue(double tscore, int df)
{
	using namespace boost::math;
	students_t dist(df);
	// Cumulative Distribution Function evaluated at tscore
	if ( tscore >= 0) {
		return 2*(1.0-cdf(dist, tscore));
	} else {
		return 2*cdf(dist,tscore);
	}

}

std::string SimpleLinearRegression::ToString()
{
    std::ostringstream ss;
	ss << "covariance = " << covariance << std::endl;
	ss << "correlation = " << correlation << std::endl;
	ss << "alpha = " << alpha << std::endl;
	ss << "beta = " << beta << std::endl;
	ss << "r_squared = " << r_squared << std::endl;
	ss << "valid = " << (valid ? "true" : "false") << std::endl;
	ss << "valid_correlation = " << (valid_correlation ? "true" : "false")
		<< std::endl;
	ss << "error_sum_squares = " << error_sum_squares << std::endl;
	return ss.str();
}

AxisScale::AxisScale()
: data_min(0), data_max(0), scale_min(0), scale_max(0),
scale_range(0), tic_inc(0), p(0)
{
}

AxisScale::AxisScale(double data_min_s, double data_max_s, int ticks_s,
                     int lbl_precision_s, bool lbl_prec_fixed_point_s)
: data_min(0), data_max(0), scale_min(0), scale_max(0),
scale_range(0), tic_inc(0), p(0), ticks(ticks_s),
lbl_precision(lbl_precision_s), lbl_prec_fixed_point(lbl_prec_fixed_point_s)
{
	CalculateScale(data_min_s, data_max_s, ticks_s);
}

AxisScale::AxisScale(const AxisScale& s)
: data_min(s.data_min), data_max(s.data_max),
	scale_min(s.scale_min), scale_max(s.scale_max),
	scale_range(s.scale_range), tic_inc(s.tic_inc), p(s.p),
	tics(s.tics), tics_str(s.tics_str), tics_str_show(s.tics_str_show),
	ticks(s.ticks), lbl_precision(s.lbl_precision),
    lbl_prec_fixed_point(s.lbl_prec_fixed_point)
{
}

AxisScale& AxisScale::operator=(const AxisScale& s)
{
	data_min = s.data_min;
	data_max = s.data_max;
	scale_min = s.scale_min;
	scale_max = s.scale_max;
	scale_range = s.scale_range;
	tic_inc = s.tic_inc;
	p = s.p;
	tics = s.tics;
	tics_str = s.tics_str;
	tics_str_show = s.tics_str_show;
	ticks = s.ticks;
    lbl_precision = s.lbl_precision;
    lbl_prec_fixed_point = s.lbl_prec_fixed_point;
	return *this;
}

void AxisScale::CalculateScale(double data_min_s, double data_max_s,
							   const int ticks)
{
	if (data_min_s <= data_max_s) {
		data_min = data_min_s;
		data_max = data_max_s;
	} else {
		data_min = data_max_s;
		data_max = data_min_s;	
	}
	
	double data_range = data_max - data_min;
	if ( data_range <= 2*DBL_MIN ) {
		scale_max = ceil((data_max + 0.05)*10)/10;
		scale_min = floor((data_min - 0.05)*10)/10;
		scale_range = scale_max - scale_min;
		p = 1;
		tic_inc = scale_range/2;
		tics.resize(3);
		tics_str.resize(3);
		tics[0] = scale_min;
		tics[1] = scale_min + tic_inc;
		tics[2] = scale_max;
	} else {
		p = floor(log10(data_range))-1;
		scale_max = ceil(data_max / pow((double)10,p)) * pow((double)10,p);
		scale_min = floor(data_min / pow((double)10,p)) * pow((double)10,p);
		scale_range = scale_max - scale_min;
		tic_inc = floor((scale_range / pow((double)10,p))/ticks)
			* pow((double)10,p);
		if (scale_min + tic_inc*(ticks+1) <= scale_max + 2*DBL_MIN) {
			tics.resize(ticks+2);
			tics_str.resize(ticks+2);
		} else {
			tics.resize(ticks+1);
			tics_str.resize(ticks+1);
		}
		for (int i=0; i<tics.size(); i++) {
			tics[i] = scale_min + i*tic_inc;
		}
	}
	tics_str_show.resize(tics_str.size());
	for (int i=0; i<tics.size(); i++) {
        tics_str[i] = GenUtils::DblToStr(tics[i], lbl_precision,
                                         lbl_prec_fixed_point);
		tics_str_show[i] = true;
	}
}

/** only display every other tic value */
void AxisScale::SkipEvenTics()
{
	for (int i=0; i<tics_str_show.size(); i++) tics_str_show[i] = (i%2 == 0);
}

void AxisScale::ShowAllTics()
{
	for (int i=0; i<tics_str_show.size(); i++) tics_str_show[i] = true;
}

std::string AxisScale::ToString()
{
    std::ostringstream ss;
	ss << "data_min = " << data_min << std::endl;
	ss << "data_max = " << data_max << std::endl;
	ss << "scale_min = " << scale_min << std::endl;
	ss << "scale_max = " << scale_max << std::endl;
	ss << "scale_range = " << scale_range << std::endl;
	ss << "p = " << p << std::endl;
	ss << "tic_inc = " << tic_inc << std::endl;
	for (int i=0; i<tics.size(); i++) {
		ss << "tics[" << i << "] = " << tics[i];
		ss << ",  tics_str[" << i << "] = " << tics_str[i] << std::endl;
	}
	ss << "Exiting AxisScale::CalculateScale" << std::endl;
	return ss.str();
}

wxString GenUtils::BoolToStr(bool b)
{
	return b ? "true" : "false";
}

bool GenUtils::StrToBool(const wxString& s)
{
	if (s.CmpNoCase("1") == 0) return true;
	if (s.CmpNoCase("true") == 0) return true;
	return false;
}

/** If input string has length < width, then prepends (or appends
 if pad_left=false) string with spaces so that total length is now width.
 If string length >= width, then returns original input string. */
wxString GenUtils::Pad(const wxString& s, int width, bool pad_left)
{
	if (s.length() >= width) return s;
	int pad_len = width - (int)s.length();
	wxString output;
	if (!pad_left) output << s;
	for (int i=0; i<pad_len; i++) output << " ";
	if (pad_left) output << s;
	return output;
}

wxString GenUtils::PadTrim(const wxString& s, int width, bool pad_left)
{
    if (s.length() > width) {
        int trim_w = width - 2; //"xxx..xxx"
        int second_w = trim_w / 2;
        int first_w = trim_w - second_w;
        wxString tmp = s.SubString(0, first_w-2);
        tmp << ".." << s.SubString(s.length() - second_w -1, s.length()-1);
        return tmp;
    }
    int pad_len = width - (int)s.length();
    wxString output;
    if (!pad_left) output << s;
    for (int i=0; i<pad_len; i++) output << " ";
    if (pad_left) output << s;
    return output;
}

wxString GenUtils::DblToStr(double x, int precision, bool fixed_point)
{
	std::stringstream ss;
    if (x < 10000000) {
        ss << std::fixed;
    }

     if (x == (int)x && fixed_point == false) {
         // The default should be that an integer is displayed as an integer
        ss << (int)x;
    } else {
        ss << std::setprecision(precision);
        ss << x;
    }

	return wxString(ss.str().c_str(), wxConvUTF8);
}

wxString GenUtils::IntToStr(int x, int precision)
{
    std::stringstream ss;
    
    if (x < 10000000) {
        ss << std::fixed;
    }
    ss << std::setprecision(precision);
    ss << x;

    return wxString(ss.str().c_str(), wxConvUTF8);
}

wxString GenUtils::PtToStr(const wxPoint& p)
{
	std::stringstream ss;
	ss << "(" << p.x << "," << p.y << ")";
	return wxString(ss.str().c_str(), wxConvUTF8);
}

wxString GenUtils::PtToStr(const wxRealPoint& p)
{
	std::stringstream ss;
	ss << std::setprecision(5);
	ss << "(" << p.x << "," << p.y << ")";
	return wxString(ss.str().c_str(), wxConvUTF8);
}

double GenUtils::Median(std::vector<double>& data)
{
    if (data.empty()) return 0;
    
    std::sort(data.begin(), data.end());

    int n = (int)data.size();
    if (n % 2 == 1) return data[n/2];

    return 0.5 * (data[n/2 -1] + data[n/2]);
}

double GenUtils::Median(double* data, int n, const std::vector<bool>& undefs)
{
    std::vector<double> valid_data;
    for (int i = 0; i < n; ++i) {
        if (!undefs[i]) {
            valid_data.push_back(data[i]);
        }
    }
    return Median(valid_data);
}

void GenUtils::DeviationFromMean(int nObs, double* data)
{
	if (nObs == 0) return;
	double sum = 0.0;
	for (int i=0, iend=nObs; i<iend; i++) sum += data[i];
	const double mean = sum / (double) nObs;
	for (int i=0, iend=nObs; i<iend; i++) data[i] -= mean;
}

void GenUtils::DeviationFromMean(int nObs, double* data, std::vector<bool>& undef)
{
	if (nObs == 0) return;
    
    int nValid = 0;
	double sum = 0.0;
    for (int i=0, iend=nObs; i<iend; i++) {
        if (undef[i])
            continue;
        sum += data[i];
        nValid += 1;
    }
	const double mean = sum / (double) nValid;
    for (int i=0, iend=nObs; i<iend; i++) {
        data[i] -= mean;
    }
}

void GenUtils::DeviationFromMean(std::vector<double>& data)
{
	if (data.size() == 0)
        return;
	double sum = 0.0;
    for (int i=0; i<data.size(); i++) {
        sum += data[i];
    }
	const double mean = sum / (double) data.size();
    for (int i=0; i<data.size(); i++) {
        data[i] -= mean;
    }
}

void GenUtils::DeviationFromMedian(std::vector<double>& data)
{
    if (data.size() == 0)
        return;
    double median = Median(data);
    for (int i=0; i<data.size(); i++) {
        data[i] -= median;
    }
}

void GenUtils::DeviationFromMedoid(std::vector<double>& data, double medoid_val)
{
    if (data.size() == 0)
        return;
    for (int i=0; i<data.size(); i++) {
        data[i] -= medoid_val;
    }
}

void GenUtils::DeviationFromMean(std::vector<double>& data, std::vector<bool>& undef)
{
    if (data.size() == 0) return;
    double sum = 0.0;
    int n = 0;
    for (int i=0; i<data.size(); i++) {
        if (undef[i]) continue;
        sum += data[i];
        n++;
    }
    const double mean = sum / n;
    for (int i=0; i<data.size(); i++) {
        if (undef[i]) continue;
        data[i] -= mean;
    }
}

void GenUtils::RangeAdjust(std::vector<double>& data)
{
    double min_val = DBL_MAX, max_val = DBL_MIN;
    for (size_t i=0; i<data.size(); ++i) {
        if (data[i] < min_val) min_val = data[i];
        else if (data[i] > max_val) max_val = data[i];
    }
    //  divide each variable by the range
    double range_val = max_val - min_val;
    if (range_val != 0) {
        for (size_t i=0; i<data.size(); ++i) {
            data[i] = data[i] /  range_val;
        }
    }
}

void GenUtils::RangeAdjust(std::vector<double>& data, std::vector<bool>& undef)
{
    double min_val = DBL_MAX, max_val = DBL_MIN;
    for (size_t i=0; i<data.size(); ++i) {
        if (undef[i]) continue;
        if (data[i] < min_val) min_val = data[i];
        else if (data[i] > max_val) max_val = data[i];
    }
    //  divide each variable by the range
    double range_val = max_val - min_val;
    if (range_val != 0) {
        for (size_t i=0; i<data.size(); ++i) {
            if (undef[i]) continue;
            data[i] = data[i] /  range_val;
        }
    }
}

void GenUtils::RangeStandardize(std::vector<double>& data)
{
    double min_val = DBL_MAX, max_val = DBL_MIN;
    for (size_t i=0; i<data.size(); ++i) {
        if (data[i] < min_val) min_val = data[i];
        else if (data[i] > max_val) max_val = data[i];
    }
    //  subtract the min from each variable and then divide by the range
    double range_val = max_val - min_val;
    if (range_val != 0) {
        for (size_t i=0; i<data.size(); ++i) {
            data[i] = (data[i] - min_val) /  range_val;
        }
    }
}

void GenUtils::RangeStandardize(std::vector<double>& data, std::vector<bool>& undef)
{
    double min_val = DBL_MAX, max_val = DBL_MIN;
    for (size_t i=0; i<data.size(); ++i) {
        if (undef[i]) continue;
        if (data[i] < min_val) min_val = data[i];
        else if (data[i] > max_val) max_val = data[i];
    }
    //  subtract the min from each variable and then divide by the range
    double range_val = max_val - min_val;
    if (range_val != 0) {
        for (size_t i=0; i<data.size(); ++i) {
            if (undef[i]) continue;
            data[i] = (data[i] - min_val) /  range_val;
        }
    }
}

void GenUtils::MeanAbsoluteDeviation(int nObs, double* data)
{
    if (nObs == 0) return;
    double sum = 0.0;
    for (int i=0, iend=nObs; i<iend; i++) sum += data[i];
    const double mean = sum / (double) nObs;
    double mad = 0.0;
    for (int i=0, iend=nObs; i<iend; i++) {
        mad += std::abs(data[i] - mean);
    }
    mad = mad / nObs;
    if (mad == 0) return;
    for (int i=0, iend=nObs; i<iend; i++) {
        data[i] = (data[i] - mean) / mad;
    }
}

void GenUtils::MeanAbsoluteDeviation(int nObs, double* data,
                                     std::vector<bool>& undef)
{
    if (nObs == 0) return;
    double nValid = 0;
    double sum = 0.0;
    for (int i=0, iend=nObs; i<iend; i++) {
        if (undef[i]) continue;
        sum += data[i];
        nValid += 1;
    }
    const double mean = sum / nValid;
    double mad = 0.0;
    for (int i=0, iend=nObs; i<iend; i++) {
        if (undef[i]) continue;
        mad += std::abs(data[i] - mean);
    }
    mad = mad / nValid;
    if (mad == 0) return;
    for (int i=0, iend=nObs; i<iend; i++) {
        if (undef[i]) continue;
        data[i] = (data[i] - mean) / mad;
    }
}
void GenUtils::MeanAbsoluteDeviation(std::vector<double>& data)
{
	if (data.size() == 0) return;
	double sum = 0.0;
    double nn = data.size();
	for (int i=0; i<data.size(); i++) sum += data[i];
    const double mean = sum / nn;
    double mad = 0.0;
    for (int i=0; i<data.size(); i++) {
        mad += std::abs(data[i] - mean);
    }
    mad = mad / nn;
    if (mad == 0) return;
    for (int i=0; i<data.size(); i++) {
        data[i] = (data[i] - mean) / mad;
    }
}
void GenUtils::MeanAbsoluteDeviation(std::vector<double>& data,
                                     std::vector<bool>& undef)
{
    if (data.size() == 0) return;
    double sum = 0.0;
    double nValid = 0;
    for (int i=0; i<data.size(); i++) {
        if (undef[i]) continue;
        sum += data[i];
        nValid += 1;
    }
    const double mean = sum / nValid;
    double mad = 0.0;
    for (int i=0; i<data.size(); i++) {
        if (undef[i]) continue;
        mad += std::abs(data[i] - mean);
    }
    mad = mad / nValid;
    if (mad == 0) return;
    for (int i=0; i<data.size(); i++) {
        if (undef[i]) continue;
        data[i] = (data[i] - mean) / mad;
    }
}

void GenUtils::Transformation(int trans_type,
                              std::vector<std::vector<double> >& data,
                              std::vector<std::vector<bool> >& undefs)
{
    if (trans_type < 1) {
        return;
    }
    for (size_t i=0; i<data.size(); i++) {
        if (trans_type == 1) {
            // demean
            DeviationFromMean(data[i], undefs[i]);
        } else if (trans_type == 2) {
            // standarize (z)
            StandardizeData(data[i], undefs[i]);
        } else if (trans_type == 3) {
            // MAD
            MeanAbsoluteDeviation(data[i], undefs[i]);
        }
    }
}

void GenUtils::rankify_fast(const std::vector<double>& x,
                                           std::vector<double>& Rank_X)
{
    size_t sz = x.size();

    std::vector<std::pair<double, size_t> > ordered_X(sz);
    for(size_t i = 0; i < sz; i++) {
        ordered_X[i].first = x[i];
        ordered_X[i].second = i;
    }
    std::sort(ordered_X.begin(), ordered_X.end());

    size_t rank = 1, n = 1, i = 0, j, idx;

    while (i < sz) {
        j = i;
        // get # of elements with euqal rank
        while (j < sz -1 && ordered_X[j].first == ordered_X[j+1].first) {
            j += 1;
        }
        n = j - i + 1;

        for (j=0; j<n; ++j) {
            // for each equal element use formula obtain index of T[i+j].first
            idx = ordered_X[i + j].second;
            Rank_X[idx] = rank + (n-1) * 0.5;
        }
        // increment rank and i
        rank += n;
        i += n;
    }
}

std::vector<double> GenUtils::rankify(const std::vector<double>& x)
{
    size_t N = x.size();
    // Rank Vector
    std::vector<double>  Rank_X(N);

    for(size_t i = 0; i < N; i++) {
        int r = 1, s = 1;

        // Count no of smaller elements
        // in 0 to i-1
        for(size_t j = 0; j < i; j++) {
            if (x[j] < x[i] ) r++;
            if (x[j] == x[i] ) s++;
        }

        // Count no of smaller elements
        // in i+1 to N-1
        for (size_t j = i+1; j < N; j++) {
            if (x[j] < x[i] ) r++;
            if (x[j] == x[i] ) s++;
        }

        // Use Fractional Rank formula
        // fractional_rank = r + (n-1)/2
        Rank_X[i] = r + (s-1) * 0.5;
    }

    // Return Rank Vector
    return Rank_X;
}

double GenUtils::RankCorrelation(std::vector<double>& x, std::vector<double>& y)
{
    // Get ranks of vector X y
    std::vector<double> rank_x(x.size(), 0),  rank_y(y.size(), 0);
    boost::thread_group threadPool;
    threadPool.add_thread(new boost::thread(&GenUtils::rankify_fast, x, boost::ref(rank_x)));
    threadPool.add_thread(new boost::thread(&GenUtils::rankify_fast, y, boost::ref(rank_y)));
    threadPool.join_all();

    double spearmans_r = Correlation(rank_x, rank_y);

    return spearmans_r;
}

double GenUtils::Correlation(std::vector<double>& x, std::vector<double>& y)
{
    int nObs = (int)x.size();
    double sum_x = 0;
    double sum_y = 0;
    for (int i=0; i<nObs; i++) {
        sum_x += x[i];
        sum_y += y[i];
    }
    double mean_x = sum_x / nObs;
    double mean_y = sum_y / nObs;
   
    double ss_x = 0;
    double ss_y = 0;
    double ss_xy = 0;
    double d_x = 0, d_y = 0;
    for (int i=0; i<nObs; i++) {
        d_x = x[i] - mean_x;
        d_y = y[i] - mean_y;
        ss_x += d_x * d_x;
        ss_y += d_y * d_y;
        ss_xy += d_x * d_y;
    }
    
    double r = pow(ss_x * ss_y, 0.5);
    r = ss_xy / r;
    return r;
}

double GenUtils::Sum(std::vector<double>& data)
{
    double sum = 0;
    int nObs = (int)data.size();
    for (int i=0; i<nObs; i++) sum += data[i];
    return sum;
}

double GenUtils::SumOfSquares(std::vector<double>& data)
{
    int nObs = (int)data.size();
    if (nObs <= 1)
        return 0;
    GenUtils::DeviationFromMean(data);
    double ssum = 0.0;
    for (int i=0; i<nObs; i++) {
        ssum += data[i] * data[i];
    }
    return ssum;
}

double GenUtils::SumOfSquaresMedian(std::vector<double>& data)
{
    int nObs = (int)data.size();
    if (nObs <= 1)
        return 0;
    GenUtils::DeviationFromMedian(data);
    double ssum = 0.0;
    for (int i=0; i<nObs; i++) {
        ssum += data[i] * data[i];
    }
    return ssum;
}

double GenUtils::SumOfManhattanMedian(std::vector<double>& data)
{
    int nObs = (int)data.size();
    if (nObs <= 1)
        return 0;
    GenUtils::DeviationFromMedian(data);
    double ssum = 0.0;
    for (int i=0; i<nObs; i++) {
        ssum += abs(data[i]);
    }
    return ssum;
}

double GenUtils::SumOfSquaresMedoid(std::vector<double>& data, double medoid_val)
{
    int nObs = (int)data.size();
    if (nObs <= 1)
        return 0;
    GenUtils::DeviationFromMedoid(data, medoid_val);
    double ssum = 0.0;
    for (int i=0; i<nObs; i++) {
        ssum += data[i] * data[i];
    }
    return ssum;
}

double GenUtils::SumOfManhattanMedoid(std::vector<double>& data, double medoid_val)
{
    int nObs = (int)data.size();
    if (nObs <= 1)
        return 0;
    GenUtils::DeviationFromMedoid(data, medoid_val);
    double ssum = 0.0;
    for (int i=0; i<nObs; i++) {
        ssum += abs(data[i]);
    }
    return ssum;
}

double GenUtils::GetVariance(std::vector<double>& data)
{
    if (data.size() <= 1) return 0;
    GenUtils::DeviationFromMean(data);
    double ssum = 0.0;
    for (int i=0; i<data.size(); i++) {
        ssum += data[i] * data[i];
    }
    return ssum / data.size();
}

bool GenUtils::StandardizeData(int nObs, double* data)
{
	if (nObs <= 1) return false;
	GenUtils::DeviationFromMean(nObs, data);
	double ssum = 0.0;
	for (int i=0, iend=nObs; i<iend; i++) ssum += data[i] * data[i];
	const double sd = sqrt(ssum / (double) (nObs-1.0));
	if (sd == 0) return false;
	for (int i=0, iend=nObs; i<iend; i++) data[i] /= sd;
	return true;
}

bool GenUtils::StandardizeData(int nObs, double* data, std::vector<bool>& undef)
{
	if (nObs <= 1) return false;
    
    int nValid = 0;
    for (int i=0; i<undef.size(); i++) {
        if (!undef[i])
            nValid += 1;
    }
    
	GenUtils::DeviationFromMean(nObs, data, undef);
	double ssum = 0.0;
    for (int i=0, iend=nObs; i<iend; i++) {
        if (undef[i])
            continue;
        ssum += data[i] * data[i];
    }
	const double sd = sqrt(ssum / (double) (nValid-1.0));
	if (sd == 0)
        return false;
    for (int i=0, iend=nObs; i<iend; i++) {
        data[i] /= sd;
    }
	return true;
}


bool GenUtils::StandardizeData(std::vector<double>& data)
{
	if (data.size() <= 1) return false;
	GenUtils::DeviationFromMean(data);
	double ssum = 0.0;
	for (int i=0; i<data.size(); i++) ssum += data[i] * data[i];
	const double sd = sqrt(ssum / (double) (data.size()-1.0));
	if (sd == 0) return false;
	for (int i=0; i<data.size(); i++) data[i] /= sd;
	return true;
}

bool GenUtils::StandardizeData(std::vector<double>& data, std::vector<bool>& undef)
{
    int nObs = (int)data.size();
    if (nObs <= 1) return false;
    
    int nValid = 0;
    for (int i=0; i<undef.size(); i++) {
        if (!undef[i])
            nValid += 1;
    }
    
    GenUtils::DeviationFromMean(data, undef);
    double ssum = 0.0;
    for (int i=0, iend=nObs; i<iend; i++) {
        if (undef[i])
            continue;
        ssum += data[i] * data[i];
    }
    const double sd = sqrt(ssum / (double) (nValid-1.0));
    if (sd == 0)
        return false;
    for (int i=0, iend=nObs; i<iend; i++) {
        data[i] /= sd;
    }
    return true;
}

wxString GenUtils::swapExtension(const wxString& fname, const wxString& ext)
{
	if (ext.IsEmpty()) return fname;
	wxString prefix = fname.BeforeLast('.');
	if (prefix.IsEmpty()) return fname + "." + ext;
	return prefix + "." + ext;
}

wxString GenUtils::GetFileDirectory(const wxString& path)
{
	int pos = path.Find('/',true);
	if (pos >= 0)
		return path.Left(pos) + '/';
	pos = path.Find('\\',true);
	if (pos >= 0)
		return path.Left(pos) + '\\';
	return wxEmptyString;
}

wxString GenUtils::GetFileName(const wxString& path)
{
	int pos = path.Find('/',true);
	if (pos >= 0)
		return path.Right(path.length() - pos - 1);
	pos = path.Find('\\',true);
	if (pos >= 0)
		return path.Right(path.length() - pos - 1);
	return wxEmptyString;
}

wxString GenUtils::GetFileNameNoExt(const wxString& path)
{
    wxString fname = GetFileName(path);
    int pos = fname.Find('.');
    if (pos >=0)
        return fname.SubString(0, pos-1);
    return fname;
}

wxString GenUtils::GetFileExt(const wxString& path)
{
	int pos = path.Find('.',true);
	if (pos >= 0)
		return path.Right(path.length() - pos - 1);
	return wxEmptyString;
}

wxString GenUtils::RestorePath(const wxString& proj_path, const wxString& path)
{
	wxFileName path_fn(path);
	if (path_fn.IsAbsolute()) return path;
	if (!path_fn.IsOk()) return path;
	wxFileName wd;
	wxFileName prj_path_fn(proj_path);
	if (prj_path_fn.GetExt().IsEmpty()) {
		wd.AssignDir(proj_path);
	} else {
		wd.AssignDir(prj_path_fn.GetPath());
	}
	if (!wd.IsOk() || !wd.IsDir() || !wd.IsAbsolute()) return path;
	if (path_fn.MakeAbsolute(wd.GetPath())) {
		if (path_fn.GetExt().IsEmpty()) {
			return path_fn.GetPath();
		}
		return path_fn.GetFullPath();
	}
	return path;
}

wxString GenUtils::SimplifyPath(const wxString& proj_path, const wxString& path)
{
	wxFileName wd;
        wxFileName proj_path_fn(proj_path); 
	if (proj_path_fn.GetExt().IsEmpty()) {
		wd.AssignDir(proj_path);
	} else {
		wd.AssignDir(proj_path_fn.GetPath());
	}
	return GenUtils::SimplifyPath(wd, path);
}

wxString GenUtils::SimplifyPath(const wxFileName& wd, const wxString& path)
{
    wxFileName path_fn(path);
	if (!wd.IsOk() || !wd.IsDir() || !wd.IsAbsolute() ||
		path_fn.IsRelative()) return path;
	wxFileName p;
	if (wxDirExists(path)) {
		p.AssignDir(path);
	} else {
		p.Assign(path);
	}
	if (p.GetVolume() != wd.GetVolume()) return path;
	wxArrayString p_dirs = p.GetDirs();
	wxArrayString wd_dirs = wd.GetDirs();
	if (p_dirs.size() < wd_dirs.size()) return path;
	for (int i=0; i<wd_dirs.size(); ++i) {
		if (p_dirs[i] != wd_dirs[i]) return path;
	}
	if (p.MakeRelativeTo(wd.GetPath())) {
		if (p.IsDir()) {
			return p.GetPath();
		}
		return p.GetFullPath();
	}
	return path;
}

void GenUtils::SplitLongPath(const wxString& path,
							 std::vector<wxString>& parts,
							 wxString& html_formatted,
							 int max_chars_per_part)
{
	if (max_chars_per_part < 15) max_chars_per_part = 15;
	parts.clear();
	html_formatted = "";
	if (path.size() <= max_chars_per_part) {
		parts.push_back(path);
		html_formatted = path;
		return;
	}
	wxFileName fn(path);
	wxArrayString dirs(fn.GetDirs());
	if (dirs.size() <= 1) {
		parts.push_back(path);
		html_formatted = path;
		return;
	}
	wxString sep = wxFileName::GetPathSeparator();
	// Note: always add sep char after dir added
	if (path.SubString(0,0) == sep) {
		parts.push_back(sep);
	} else if (fn.HasVolume()) {
		// We'll assume this is a Windows system since only other
		// supported are OSX and Linux and HasVolume should always
		// be false for these.
		parts.push_back(fn.GetVolume());
		parts[0] << wxFileName::GetVolumeSeparator(wxPATH_WIN);
		parts[0] << wxFileName::GetPathSeparator(wxPATH_WIN);
	} else {
		parts.push_back("");
	}
	size_t cp = 0; // current part
	for (size_t i=0; i<dirs.size(); ++i) {
		if (parts[cp].size() > max_chars_per_part) {
			++cp;
			parts.push_back("");
		}
		if ((parts[cp].size() + dirs[i].size() > max_chars_per_part) &&
			(parts[cp].size() > 0)) {
			++cp;
			parts.push_back("");
		}
		parts[cp] << dirs[i] << sep;
	}
	if (fn.HasName()) {
		wxString name = fn.GetName();
		name << "." << fn.GetExt();
		if ((parts[cp].size() + name.size() > max_chars_per_part) &&
			(parts[cp].size() > 0)) {
			++cp;
			parts.push_back("");
		}
		parts[cp] << name;
	}
	for (size_t i=0, last_part=parts.size()-1; i<=last_part; ++i) {
		html_formatted << parts[i];
		if (i < last_part) html_formatted << "<br />&nbsp;&nbsp;&nbsp;&nbsp;";
	}
}

/*
 Reverse
 Changes the order of bytes in the presentation of a 4 byte number.
  */
wxInt32 GenUtils::Reverse(const wxInt32 &val)
{
	union {
		wxInt32 v;
		char d[4];
	} chameleon;
	chameleon.v= val;
	char tmp = chameleon.d[0];
	chameleon.d[0] = chameleon.d[3];
	chameleon.d[3] = tmp;
	tmp = chameleon.d[1];
	chameleon.d[1] = chameleon.d[2];
	chameleon.d[2] = tmp;
	return chameleon.v;
}

long GenUtils::ReverseInt(const int &val)
{
	union {
		int v;
		char d[4];
	} chameleon;
	chameleon.v= val;
	char tmp = chameleon.d[0];
	chameleon.d[0] = chameleon.d[3];
	chameleon.d[3] = tmp;
	tmp = chameleon.d[1];
	chameleon.d[1] = chameleon.d[2];
	chameleon.d[2] = tmp;
	return chameleon.v;
}

void GenUtils::SkipTillNumber(std::istream &s)
{
	char ch;
	while (s >> ch) {
		if ((ch >= '0' && ch <= '9') || ch == '-' || ch == '+' || ch == '.')
			break;
	}
	if (s.good()) s.putback(ch);
}

// This is an implementation of ltoa
void GenUtils::longToString(const long d, char* Id, const int base) 
{
	int i = 0;
	long j = d;
	char rId[ GdaConst::ShpObjIdLen ];
	if (d == 0) {
		Id[0] = '0';
		Id[1] = '\0';
		return;
	}
	if (d < 0) j = -d;
	while (j != 0) {
		rId[i] = (j % base) + '0';
		j = j / base;
		i++;
	}
	j = i;
	if (d < 0) {
		Id[0] = '-';
		Id[i + 1] = '\0';
		while (i > 0) {
			Id[i] = rId[j - i];
			i--;
		}
		return;
	}
	
	Id[i] = '\0';
	while (i > 0) {
		Id[i - 1] = rId[j - i];
		i--;
	}
	return;
}

// Calculates Euclidean distance
double GenUtils::distance(const wxRealPoint& p1, const wxRealPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return sqrt(dx*dx + dy*dy);
}

double GenUtils::distance(const wxRealPoint& p1, const wxPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return sqrt(dx*dx + dy*dy);
}

double GenUtils::distance(const wxPoint& p1, const wxRealPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return sqrt(dx*dx + dy*dy);
}

double GenUtils::distance(const wxPoint& p1, const wxPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return sqrt(dx*dx + dy*dy);
}

// Calculates Euclidean distance
double GenUtils::distance_sqrd(const wxRealPoint& p1, const wxRealPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return dx*dx + dy*dy;
}

double GenUtils::distance_sqrd(const wxRealPoint& p1, const wxPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return dx*dx + dy*dy;
}

double GenUtils::distance_sqrd(const wxPoint& p1, const wxRealPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return dx*dx + dy*dy;
}

double GenUtils::distance_sqrd(const wxPoint& p1, const wxPoint& p2)
{
	double dx = p1.x - p2.x;
	double dy = p1.y - p2.y;
	return dx*dx + dy*dy;
}

// calculates distance from point p0 to an infinite line passing through
// points p1 and p2
double GenUtils::pointToLineDist(const wxPoint& p0, const wxPoint& p1,
								 const wxPoint& p2)
{
	double d_p1p2 = distance(p1, p2);
	if (d_p1p2 <= 16*DBL_MIN) return distance(p0, p1);
	return abs((p2.x-p1.x)*(p1.y-p0.y)-(p1.x-p0.x)*(p2.y-p1.y))/d_p1p2;
}

void GenUtils::strToInt64(const wxString& str, wxInt64 *val)
{
	char buf[1024];
	strcpy( buf, (const char*)str.mb_str(wxConvUTF8) );
	strToInt64(buf, val);
}

// Convert an ASCII string into a wxInt64 (or long long)
void GenUtils::strToInt64(const char *str, wxInt64 *val)
{
	wxInt64 total = 0;
	bool minus = 0;
 
	while (isspace(*str)) str++;
	if (*str == '+') {
		str++;
	} else if (*str == '-') {
		minus = true;
		str++;
	}
	while (isdigit(*str)) {
		total *= 10;
		total += (*str++ - '0');
	}
	*val = minus ? -total : total;
}

bool GenUtils::validInt(const wxString& str)
{
	char buf[1024];
	strcpy( buf, (const char*)str.mb_str(wxConvUTF8) );
	return validInt(buf);
}

// Checks that an ASCII string can be parsed to a valid integer.  At least
// one digit must been found.
bool GenUtils::validInt(const char* str)
{
	//LOG_MSG(wxString::Format("GenUtils::validInt(\"%s\"):", str));
	while (isspace(*str)) str++;
	if (*str == '+' || *str == '-') str++;
	const char* t = str;
	while (isdigit(*str)) str++;
	if (t == str) {
		// no digit found so return false
		//LOG_MSG("   no digit found");
		return false;
	}
	while (isspace(*str)) str++;
	// only return true if we are finally pointing at
	// the null terminating character.
	//LOG_MSG(wxString::Format("   final char is null: %d", *str == '\0'));
	return *str == '\0';
}

bool GenUtils::isEmptyOrSpaces(const wxString& str)
{
	char buf[1024];
	strcpy( buf, (const char*)str.mb_str(wxConvUTF8) );
	return isEmptyOrSpaces(buf);
}

// returns true if the string is either empty
// or has only space characters
bool GenUtils::isEmptyOrSpaces(const char *str)
{
	while (isspace(*str)) str++;
	// if the first not-space char is not the end of the string,
	// return false.
	return *str == '\0';
}

bool GenUtils::ExistsShpShxDbf(const wxFileName& fname, bool* shp_found,
							   bool* shx_found, bool* dbf_found)
{
	wxFileName shp(fname);
	shp.SetExt("shp");
	wxFileName shx(fname);
	shx.SetExt("shx");
	wxFileName dbf(fname);
	dbf.SetExt("dbf");
	if (shp_found) *shp_found = shp.FileExists();
	if (shx_found) *shx_found = shx.FileExists();
	if (dbf_found) *dbf_found = dbf.FileExists();
	return shp.FileExists() && shx.FileExists() && dbf.FileExists();
}

wxString GenUtils::FindLongestSubString(const std::vector<wxString> strings,
										bool cs)
{
	int n  = (int)strings.size();
	if (n == 0) return "";
	std::vector<wxString> strs(strings);
	if (!cs) for (int i=0; i<n; i++) strs[i].MakeLower();
	wxString ref_str = strs[0];
	for (int i=0; i<n; ++i) {
		if (strs[i].length() < ref_str.length()) ref_str = strs[i];
	}
	int len = (int)ref_str.length();
	if (len == 0) return "";
	// iterate over all possible substrings in ref_str starting from first
	// position in ref_str, and starting with full ref_str.  Reduce length
	// of substring to search each iteration.
	for (int cur_len=len; cur_len > 0; --cur_len) {
		for (int cur_pos=0; cur_pos <= len-cur_len; ++cur_pos) {
			wxString ss = ref_str.substr(cur_pos, cur_len);
			bool all_match = true; // substring found everywhere currently
			for (int i=0; i<n && all_match; i++) {
				if (strs[i].find(ss) == wxString::npos) all_match = false;
			}
			if (all_match) {
				// common substring found.  Return unmodified (case-preserved)
				// substring from first string
				return strings[0].substr(strs[0].find(ss), cur_len);
			}
		}
	}
	return ""; // no substring match, return empty string.
}


bool GenUtils::less_vectors(const std::vector<int>& a,const std::vector<int>& b) {
    return a.size() > b.size();
}

bool GenUtils::smaller_pair(const std::pair<int, int>& a,
                            const std::pair<int, int>& b) {
    return a.second > b.second;
}

wxString GenUtils::WrapText(wxWindow *win, const wxString& text, int widthMax)
{
	class HardBreakWrapper : public wxTextWrapper
	{
		public:
		HardBreakWrapper(wxWindow *win, const wxString& text, int widthMax) {
			Wrap(win, text, widthMax);
		}
		wxString const& GetWrapped() const { return m_wrapped; }
		protected:
		virtual void OnOutputLine(const wxString& line) {
			m_wrapped += line;
		}
		virtual void OnNewLine() {
			m_wrapped += '\n';
		}
		private:
		wxString m_wrapped;
	};
	HardBreakWrapper wrapper(win, text, widthMax);
	return wrapper.GetWrapped();
}

wxString GenUtils::GetExeDir()
{
	wxString exePath = wxStandardPaths::Get().GetExecutablePath();
	wxFileName exeFile(exePath);
	wxString exeDir = exeFile.GetPathWithSep();
	return exeDir;
}

wxString GenUtils::GetWebPluginsDir()
{
	wxString exePath = wxStandardPaths::Get().GetExecutablePath();
	wxFileName exeFile(exePath);
	wxString exeDir = exeFile.GetPathWithSep();
    exeDir << "web_plugins" << wxFileName::GetPathSeparator();
    
	return exeDir;
}

wxString GenUtils::GetResourceDir()
{
	wxString exePath = wxStandardPaths::Get().GetExecutablePath();
	wxFileName exeFile(exePath);
	wxString exeDir = exeFile.GetPathWithSep();
    exeDir << "../Resources" << wxFileName::GetPathSeparator();
    
	return exeDir;
}

wxString GenUtils::GetSamplesDir()
{
#ifdef __WXOSX__
    return GetResourceDir() + "web_plugins/";
#else
    return GetWebPluginsDir();
#endif
}

wxString GenUtils::GetLoggerPath()
{
#ifdef __WXOSX__
    return GetResourceDir() + "logger.txt";
#elif __linux__
    return GetWebPluginsDir() + "logger.txt";
#else
    wxString confDir = wxStandardPaths::Get().GetUserConfigDir();
    // Windows: AppData\Roaming\GeoDa
    wxString geodaUserDir = confDir + wxFileName::GetPathSeparator() + "GeoDa";
    if (wxDirExists(geodaUserDir) == false) {
        wxFileName::Mkdir(geodaUserDir);
    }
    return geodaUserDir + wxFileName::GetPathSeparator() + "logger.txt";
#endif
}

wxString GenUtils::GetUserSamplesDir()
{
    // this function will be only called in linux env
    wxString confDir = wxStandardPaths::Get().GetUserConfigDir();
    // Unix: ~ (the home directory)
    wxString geodaUserDir = confDir + wxFileName::GetPathSeparator() + ".geoda";
    if (wxDirExists(geodaUserDir) == false) {
        wxFileName::Mkdir(geodaUserDir);
    }
    wxString webDir = geodaUserDir + wxFileName::GetPathSeparator() + "web_plugins";
    if (wxDirExists(webDir) == false) {
        wxFileName::Mkdir(webDir);
    }
    return webDir + wxFileName::GetPathSeparator();
}

wxString GenUtils::GetBasemapDir()
{
#ifdef __linux__
    wxString confDir = wxStandardPaths::Get().GetUserConfigDir();
    // Unix: ~ (the home directory)
    wxString geodaUserDir = confDir + wxFileName::GetPathSeparator() + ".geoda";
    if (wxDirExists(geodaUserDir) == false) {
        wxFileName::Mkdir(geodaUserDir);
    }
    wxString basemapDir = geodaUserDir + wxFileName::GetPathSeparator() + "basemap_cache";
    if (wxDirExists(basemapDir) == false) {
        wxFileName::Mkdir(basemapDir);
    }
    return basemapDir;
#elif __WXMAC__
    return GetExeDir() + "../Resources/basemap_cache";
#else
    wxString confDir = wxStandardPaths::Get().GetUserConfigDir();
    // Windows: AppData\Roaming\GeoDa
    wxString geodaUserDir = confDir + wxFileName::GetPathSeparator() + "GeoDa";
    if (wxDirExists(geodaUserDir) == false) {
        wxFileName::Mkdir(geodaUserDir);
    }
    wxString basemapDir = geodaUserDir + wxFileName::GetPathSeparator() + "basemap_cache";
    if (wxDirExists(basemapDir) == false) {
        wxFileName::Mkdir(basemapDir);
    }
    return basemapDir;
#endif
}

wxString GenUtils::GetCachePath()
{
#ifdef __linux__
    wxString confDir = wxStandardPaths::Get().GetUserConfigDir();
    // Unix: ~ (the home directory)
    wxString geodaUserDir = confDir + wxFileName::GetPathSeparator() + ".geoda";
    if (wxDirExists(geodaUserDir) == false) {
        wxFileName::Mkdir(geodaUserDir);
    }
    wxString cachePath = geodaUserDir + wxFileName::GetPathSeparator() + "cache.sqlite";
    if (wxFileExists(cachePath) == false) {
        wxString origCachePath = GetExeDir() + "cache.sqlite";
        wxCopyFile(origCachePath, cachePath);
    }
    return cachePath;
#elif __WXMAC__
    return GetExeDir() + "../Resources/cache.sqlite";
#else
    wxString confDir = wxStandardPaths::Get().GetUserConfigDir();
    // Windows: AppData\Roaming\GeoDa
    wxString geodaUserDir = confDir + wxFileName::GetPathSeparator() + "GeoDa";
    if (wxDirExists(geodaUserDir) == false) {
        wxFileName::Mkdir(geodaUserDir);
    }
    wxString cachePath = geodaUserDir + wxFileName::GetPathSeparator() + "cache.sqlite";
    if (wxFileExists(cachePath) == false) {
        wxString origCachePath = GetExeDir() + "cache.sqlite";
        wxCopyFile(origCachePath, cachePath);
    }
    return cachePath;
#endif
}

wxString GenUtils::GetLangConfigPath()
{
#ifdef __linux__
    wxString search_path = GetExeDir() + wxFileName::GetPathSeparator() +  "lang";
#elif __WXMAC__
    wxString search_path = GetExeDir() + "/../Resources/lang";
#else
    wxString confDir = wxStandardPaths::Get().GetUserConfigDir();
    // Windows: AppData\Roaming\GeoDa
    wxString geodaUserDir = confDir + wxFileName::GetPathSeparator() + "GeoDa";
    if (wxDirExists(geodaUserDir) == false) {
        wxFileName::Mkdir(geodaUserDir);
    }
    wxString search_path = geodaUserDir + wxFileName::GetPathSeparator() + "lang";
    if (wxDirExists(search_path) == false) {
        wxFileName::Mkdir(search_path);
    }
    wxString langPath = search_path + wxFileName::GetPathSeparator() + "config.ini";
    if (wxFileExists(langPath) == false) {
        wxString origLangPath = GetExeDir() + "lang" + wxFileName::GetPathSeparator() + "config.ini";
        wxCopyFile(origLangPath, langPath);
    }
#endif
    
    return search_path;
}

wxString GenUtils::GetLangSearchPath()
{
#ifdef __WXMAC__
    wxString search_path = GetExeDir() + "/../Resources/lang";
#else
    wxString search_path = GetExeDir() + wxFileName::GetPathSeparator() +  "lang";
#endif
    
    return search_path;
}
