/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
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

#include <assert.h>
#include <sstream>
#include <string>
#include <vector>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include "logger.h"
#include "FramesManager.h"
#include "DataViewer/DbfGridTableBase.h"
#include "DataViewer/TableState.h"
#include "DialogTools/SaveToTableDlg.h"
#include "Explore/CatClassification.h"
#include "Explore/CatClassifManager.h"
#include "Generic/MyShape.h"
#include "ShapeOperations/DbfFile.h"
#include "ShapeOperations/GalWeight.h"
#include "ShapeOperations/ShapeUtils.h"
#include "ShapeOperations/VoronoiUtils.h"
#include "ShapeOperations/WeightsManager.h"
#include "Project.h"

i_array_type Project::shared_category_scratch; // used by TemplateCanvas
int Project::next_project_id = 0;

Project::Project(int num_records_s)
: project_id(next_project_id++),
grid_base(0), map_aspect_ratio(0),
table_only_project(false), is_project_valid(false),
num_records(num_records_s),
regression_dlg(0),
default_v1_time(0), default_v2_time(0), default_v3_time(0), default_v4_time(0),
default_v1_name(""), default_v2_name(""), default_v3_name(""),
default_v4_name(""), allow_enable_save(true),
shp_file_needs_first_save(false), mean_centers(0), centroids(0),
point_duplicates_initialized(false),
point_dups_warn_prev_displayed(false),
voronoi_rook_nbr_gal(0)
{
	frames_manager = new FramesManager;
	highlight_state = new HighlightState;
	highlight_state->SetSize(num_records);
	cat_classif_manager = new CatClassifManager;
	w_manager = new WeightsManager(num_records);
	table_state = new TableState;
}

void Project::Init(DbfGridTableBase* grid_base_s)
{
	int num_obs = grid_base_s->GetNumberRows();
	shared_category_scratch.resize(
		boost::extents[CatClassification::max_num_classes][num_obs]);
	grid_base = grid_base_s;
	table_only_project = true;
	is_project_valid = true;
	allow_enable_save = wxFileExists(grid_base->GetDbfFileName().GetFullPath());
}

void Project::Init(DbfGridTableBase* grid_base_s, wxFileName shp_fname)
{
	int num_obs = grid_base_s->GetNumberRows();
	shared_category_scratch.resize(
		boost::extents[CatClassification::max_num_classes][num_obs]);	
	grid_base = grid_base_s;
	table_only_project = false;
	is_project_valid = true;
	OpenShpFile(shp_fname);
	allow_enable_save = wxFileExists(grid_base->GetDbfFileName().GetFullPath());
}

Project::~Project()
{
	LOG_MSG("Entering Project::~Project");
	if (cat_classif_manager) delete cat_classif_manager; cat_classif_manager=0;
	if (w_manager) delete w_manager; w_manager = 0;
	for (int i=0, iend=mean_centers.size(); i<iend; i++) delete mean_centers[i];
	for (int i=0, iend=centroids.size(); i<iend; i++) delete centroids[i];
	if (voronoi_rook_nbr_gal) delete [] voronoi_rook_nbr_gal;
	//NOTE: the wxGrid instance in NewTableViewerFrame has
	// ownership and is therefore responsible for deleting the
	// grid_base when it closes.
	//if (grid_base) delete grid_base; grid_base = 0;
	LOG_MSG("Exiting Project::~Project");
}

bool Project::OpenShpFile(wxFileName shp_fname_s)
{
	LOG_MSG("Entering Project::OpenShpFile");
	using namespace std;
	using namespace Shapefile;
	
	shp_fname = shp_fname_s;
	wxFileName m_shx_fname(shp_fname);
	m_shx_fname.SetExt("shx");
	std::string m_shx_str(m_shx_fname.GetFullPath().mb_str());
	wxFileName m_shp_fname(shp_fname);
	m_shp_fname.SetExt("shp");
	std::string m_shp_str(m_shp_fname.GetFullPath().mb_str());
	
	Shapefile::populateIndex(m_shx_str, index_data);
	//std::ostringstream index_strm;
	//Shapefile::printIndex(index_data, index_strm);
	//wxString msg1(index_strm.str().c_str(), wxConvUTF8);
	//LOG_MSG(msg1);
	
	Shapefile::populateMain(index_data, m_shp_str, main_data);

	
    //std::ostringstream main_strm;
	//Shapefile::printMain(main_data, main_strm);
	//wxString msg2(main_strm.str().c_str(), wxConvUTF8);
	//LOG_MSG(msg2);
	
    int shp_num_recs = Shapefile::calcNumIndexHeaderRecords(index_data.header);
    LOG(shp_num_recs);
	LOG(main_data.records.size());

	map_aspect_ratio = 1;
	//map_aspect_ratio = ShapeUtils::CalcAspectRatio(index_data.header);
	//if (map_aspect_ratio == 0) return false;

	//if (main_data.header.shape_type == POLY_LINE ||
	//	main_data.header.shape_type == POLYGON ) {
		//polyCentroids.resize(main_data.records.size());
		//polyMeanCenters.resize(main_data.records.size());
	//}

//	if (main_data.header.shape_type == POINT ) {
//		for (int i=0, iend=main_data.records.size(); i<iend; i++) {
//			Shapefile::MainRecord& rec = main_data.records[i];
//			Shapefile::PointContents* pc_p =
//			dynamic_cast<Shapefile::PointContents*>(rec.contents_p);
//			assert(pc_p);
//		}
//	} else if (main_data.header.shape_type == POLYGON ) {
		//polyLists.resize(main_data.records.size());
//		for (int i=0, iend=main_data.records.size(); i<iend; i++) {
//			Shapefile::MainRecord& rec = main_data.records[i];
//			Shapefile::PolygonContents* pc_p =
//			dynamic_cast<Shapefile::PolygonContents*>(rec.contents_p);
//			assert(pc_p);
//			std::list<polygon_2d>* l_p = new std::list<polygon_2d>;
			//ShapeUtils::polygonContentsToPolyList(*pc_p, polyLists[i]);
			//polyCentroids[i] = ShapeUtils::centroid(polyLists[i]);
			//polyMeanCenters[i] = ShapeUtils::meanCenter(polyLists[i]);
//		}
//	}

	//LOG_MSG("polyList created:");
	//for (int i=0; i<polyLists.size(); i++) {
	//	std::ostringstream s;
	//	ShapeUtils::printPolyList(polyLists[i], s, 1);
	//	wxString msg(s.str().c_str(), wxConvUTF8);
	//	LOG_MSG(msg);
	//}
	
	//for (int i=0; i<polyLists.size(); i++) {
	//	std::ostringstream s1;
	//	s1 << dsv(polyCentroids[i]);
	//	wxString pt1(s1.str().c_str(), wxConvUTF8);
	//	std::ostringstream s2;
	//	s2 << dsv(polyMeanCenters[i]);
	//	wxString pt2(s2.str().c_str(), wxConvUTF8);		
	//	wxString msg("polyCentroids[");
	//	msg << i << "] = " << pt1 << ",  polyMeanCenters[";
	//	msg << i << "] = " << pt2;
	//	LOG_MSG(msg);
	//}
	
	// NOTE: clean this up
		 
	return true;
}


wxString Project::GetMainDir()
{
	if (table_only_project || shp_fname.GetPathWithSep().IsEmpty() ) {
		return grid_base->GetDbfFileName().GetPathWithSep();
	} else {
		return shp_fname.GetPathWithSep();
	}
}

wxString Project::GetMainName()
{
	if (table_only_project || shp_fname.GetName().IsEmpty()) {
		return grid_base->GetDbfFileName().GetName();
	} else {
		return shp_fname.GetName();
	}
}

wxString Project::GetFullShpName()
{
	if (table_only_project || shp_fname.GetFullPath().IsEmpty()) {
		return grid_base->GetDbfFileName().GetFullPath();
	} else {
		return shp_fname.GetFullPath();
	}
}

void Project::CreateShapefileFromPoints(const std::vector<double> x,
										const std::vector<double> y)
{
	if (!IsTableOnlyProject()) return;
	
	shp_fname = wxFileName();
	ShapeUtils::populatePointShpFile(x, y, index_data, main_data);
	map_aspect_ratio = 1;
	
	table_only_project = false;
	allow_enable_save = false;
	shp_file_needs_first_save = true;
}

void Project::SaveVoronoiToShapefile()
{
	GetVoronoiPolygons();
	// generate a list of list of duplicates.  Or, better yet, have a map of
	// lists where the key is the id, and the value is a list of all ids
	// in the same set.
	// If duplicates exist, then give option to save duplicate sets to Table
	if (IsPointDuplicates()) DisplayPointDupsWarning();
	if (voronoi_polygons.size() != GetNumRecords()) return;
	
	wxFileDialog dlg(NULL, "New Shapefile Name", wxEmptyString, wxEmptyString,
					 "SHP files (*.shp)|*.shp", wxFD_SAVE);
	if (dlg.ShowModal() != wxID_OK) return;
	
	bool space_time = grid_base->IsTimeVariant();
	
	wxFileName new_fname = dlg.GetPath();
	wxString new_main_dir = new_fname.GetPathWithSep();
	wxString new_main_name = new_fname.GetName();
	wxString new_sp_dbf = new_main_dir + new_main_name + ".dbf";
	wxString new_tm_dbf = new_main_dir + new_main_name + "_time.dbf";
	wxString new_shp = new_main_dir + new_main_name + ".shp";
	wxString new_shx = new_main_dir + new_main_name + ".shx";
	
	// Prompt for overwrite permissions
	std::vector<wxString> overwrite_list;
	if (wxFileExists(new_sp_dbf)) {
		overwrite_list.push_back(new_sp_dbf);
	}
	if (space_time && wxFileExists(new_tm_dbf)) {
		overwrite_list.push_back(new_tm_dbf);
	}
	if (wxFileExists(new_shp)) {
		overwrite_list.push_back(new_shp);
	}
	if (wxFileExists(new_shx)) {
		overwrite_list.push_back(new_shx);
	}
	
	// Prompt for overwrite permission
	if (overwrite_list.size() > 0) {
		wxString msg((overwrite_list.size() > 1) ? "Files " : "File ");
		for (int i=0; i<overwrite_list.size(); i++) {
			msg << overwrite_list[i];
			if (i < overwrite_list.size()-1) msg << ", ";
		}
		msg << " already exist";
		msg << ((overwrite_list.size() > 1) ? "." : "s.");
		msg << " Ok to overwrite?";
		wxMessageDialog dlg (NULL, msg, "Overwrite?",
							 wxYES_NO | wxCANCEL | wxNO_DEFAULT);
		if (dlg.ShowModal() != wxID_YES) return;
	}
	
	Shapefile::Main main;
	Shapefile::Index index;
	ShapeUtils::convertMyPolygonsToShpFile(voronoi_polygons,
										   voronoi_bb_xmin, voronoi_bb_ymin,
										   voronoi_bb_xmax, voronoi_bb_ymax,
										   index, main);

	std::string err_msg;
	if (!Shapefile::writePolygonIndexFile(new_shx.ToStdString(),
										  index, err_msg)) {
		wxMessageDialog dlg (NULL, err_msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	if (!Shapefile::writePolygonMainFile(new_shp.ToStdString(),
										 main, err_msg)) {
		wxMessageDialog dlg (NULL, err_msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	if (grid_base->IsTimeVariant()) {
		if (!SaveTableSpaceTimeNew(new_sp_dbf, new_tm_dbf)) return;
	} else {
		if (!SaveTableSpaceNew(new_sp_dbf)) return;
	}
	{
		wxMessageDialog dlg(NULL, "Saved to new Shapefile successfully.",
							"Success", wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
	}
}

/**
 mean_centers == true: save mean centers
 mean_centers == false: save centroids
 */
void Project::SaveCentersToShapefile(bool mean_centers)
{	
	wxFileDialog dlg(NULL, "New Shapefile Name", wxEmptyString, wxEmptyString,
					 "SHP files (*.shp)|*.shp", wxFD_SAVE);
	if (dlg.ShowModal() != wxID_OK) return;
	
	bool space_time = grid_base->IsTimeVariant();
	
	wxFileName new_fname = dlg.GetPath();
	wxString new_main_dir = new_fname.GetPathWithSep();
	wxString new_main_name = new_fname.GetName();
	wxString new_sp_dbf = new_main_dir + new_main_name + ".dbf";
	wxString new_tm_dbf = new_main_dir + new_main_name + "_time.dbf";
	wxString new_shp = new_main_dir + new_main_name + ".shp";
	wxString new_shx = new_main_dir + new_main_name + ".shx";
	
	// Prompt for overwrite permissions
	std::vector<wxString> overwrite_list;
	if (wxFileExists(new_sp_dbf)) {
		overwrite_list.push_back(new_sp_dbf);
	}
	if (space_time && wxFileExists(new_tm_dbf)) {
		overwrite_list.push_back(new_tm_dbf);
	}
	if (wxFileExists(new_shp)) {
		overwrite_list.push_back(new_shp);
	}
	if (wxFileExists(new_shx)) {
		overwrite_list.push_back(new_shx);
	}
	
	// Prompt for overwrite permission
	if (overwrite_list.size() > 0) {
		wxString msg((overwrite_list.size() > 1) ? "Files " : "File ");
		for (int i=0; i<overwrite_list.size(); i++) {
			msg << overwrite_list[i];
			if (i < overwrite_list.size()-1) msg << ", ";
		}
		msg << " already exist";
		msg << ((overwrite_list.size() > 1) ? "." : "s.");
		msg << " Ok to overwrite?";
		wxMessageDialog dlg (NULL, msg, "Overwrite?",
							 wxYES_NO | wxCANCEL | wxNO_DEFAULT);
		if (dlg.ShowModal() != wxID_YES) return;
	}
	
	std::vector<double> x;
	std::vector<double> y;
	if (mean_centers) {
		GetMeanCenters(x, y);
	} else {
		GetCentroids(x, y);
	}
	Shapefile::Main main;
	Shapefile::Index index;
	ShapeUtils::populatePointShpFile(x, y, index, main);
		
	std::string err_msg;
	if (!Shapefile::writePointIndexFile(new_shx.ToStdString(),
										  index, err_msg)) {
		wxMessageDialog dlg (NULL, err_msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	if (!Shapefile::writePointMainFile(new_shp.ToStdString(),
										 main, err_msg)) {
		wxMessageDialog dlg (NULL, err_msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
	if (grid_base->IsTimeVariant()) {
		if (!SaveTableSpaceTimeNew(new_sp_dbf, new_tm_dbf)) return;
	} else {
		if (!SaveTableSpaceNew(new_sp_dbf)) return;
	}
	{
		wxMessageDialog dlg(NULL, "Saved to new Shapefile successfully.",
							"Success", wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
	}
}


bool Project::IsPointDuplicates()
{
	if (!point_duplicates_initialized) {
		std::vector<double> x;
		std::vector<double> y;
		if (!GetCenters(x, y)) return false;
		GeoDa::VoronoiUtils::FindPointDuplicates(x, y, point_duplicates);
	}
	return point_duplicates.size() > 0;
}

void Project::DisplayPointDupsWarning()
{
	if (point_dups_warn_prev_displayed) return;
	wxString msg("Duplicate Thiessen polygons exist due "
				 "to duplicate or near-duplicate map points. "
				 " Press OK to save duplicate polygon ids "
				 "to Table.");
	wxMessageDialog dlg(NULL, msg, "Duplicate Thiessen "
						"Polygons Found",
						wxOK | wxCANCEL | wxICON_INFORMATION);
	if (dlg.ShowModal() == wxID_OK) SaveVoronoiDupsToTable();
	point_dups_warn_prev_displayed = true;
}

void Project::GetVoronoiRookNeighborMap(std::vector<std::set<int> >& nbr_map)
{
	IsPointDuplicates();
	std::vector<double> x;
	std::vector<double> y;
	GetCenters(x, y);
	GeoDa::VoronoiUtils::PointsToContiguity(x, y, false, nbr_map);
}

GalElement* Project::GetVoronoiRookNeighborGal()
{
	if (!voronoi_rook_nbr_gal) {
		std::vector<std::set<int> > nbr_map;
		GetVoronoiRookNeighborMap(nbr_map);
		voronoi_rook_nbr_gal = GeoDa::VoronoiUtils::NeighborMapToGal(nbr_map);
	}
	return voronoi_rook_nbr_gal;
}

bool Project::SaveTableSpaceNew(const wxString& new_sp_dbf)
{
	if (!GetGridBase()) return false;
	DbfFileHeader backup_header = grid_base->orig_header;
	
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	grid_base->orig_header.year = timeinfo->tm_year+1900;
	grid_base->orig_header.month = timeinfo->tm_mon+1;
	grid_base->orig_header.day = timeinfo->tm_mday;
	
	wxString err_msg;
	bool success = grid_base->WriteToDbf(new_sp_dbf, err_msg);
	if (!success) {
		wxMessageDialog dlg (NULL, err_msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		LOG_MSG(err_msg);
	} else {
		LOG_MSG("Table saved successfully");
	}
	grid_base->orig_header = backup_header;
	
	return success;
}

bool Project::SaveTableSpaceTimeNew(const wxString& new_sp_dbf,
									const wxString& new_tm_dbf)
{
	if (!GetGridBase()) return false;
	DbfFileHeader backup_sp_header = grid_base->orig_header;
	DbfFileHeader backup_tm_header = grid_base->orig_header_tm;
	
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	grid_base->orig_header.year = timeinfo->tm_year+1900;
	grid_base->orig_header.month = timeinfo->tm_mon+1;
	grid_base->orig_header.day = timeinfo->tm_mday;
	grid_base->orig_header_tm.year = timeinfo->tm_year+1900;
	grid_base->orig_header_tm.month = timeinfo->tm_mon+1;
	grid_base->orig_header_tm.day = timeinfo->tm_mday;
	
	wxString err_msg;
	bool success = grid_base->WriteToSpaceTimeDbf(new_sp_dbf,
												  new_tm_dbf, err_msg);
	if (!success) {
		wxMessageDialog dlg (NULL, err_msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		LOG_MSG(err_msg);
		return false;
	} else {
		LOG_MSG("Space-Time tables saved successfully");
	}
	grid_base->orig_header = backup_sp_header;
	grid_base->orig_header_tm = backup_tm_header;
	
	return success;
}

void Project::SaveVoronoiDupsToTable()
{
	if (!IsPointDuplicates()) return;
	std::vector<SaveToTableEntry> data(1);
	std::vector<wxInt64> dup_ids(num_records, -1);
	std::vector<bool> undefined(num_records, true);
	for (std::list<std::list<int> >::iterator dups_iter
		 = point_duplicates.begin();
		 dups_iter != point_duplicates.end(); dups_iter++) {
		int head_id = *(dups_iter->begin());
		for (std::list<int>::iterator iter=dups_iter->begin();
			 iter != dups_iter->end(); iter++) {
			undefined[*iter] = false;
			dup_ids[*iter] = head_id+1;
		}			
	}
	data[0].l_val = &dup_ids;
	data[0].undefined = &undefined;
	data[0].label = "Duplicate IDs";
	data[0].field_default = "DUP_IDS";
	data[0].type = GeoDaConst::long64_type;
	
	wxString title("Save Duplicate Thiessen Polygon Ids");
	SaveToTableDlg dlg(GetGridBase(), NULL, data, title,
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();	
}

void Project::AddNeighborsToSelection()
{
	if (!GetWManager() || (GetWManager() && !GetWManager()->GetCurrWeight())) {
		return;
	}
	LOG_MSG("Entering Project::AddNeighborsToSelection");
	GalWeight* gal_weights = 0;

	GeoDaWeight* w = GetWManager()->GetCurrWeight();
	if (!w) {
		LOG_MSG("Warning: no current weight matrix found");
		return;
	}
	if (w->weight_type != GeoDaWeight::gal_type) {
		LOG_MSG("Error: Only GAL type weights are currently supported. "
				"Other weight types are internally converted to GAL.");
		return;
	} else {
		gal_weights = (GalWeight*) w;
	}
	
	// go through the list of all objects in current selection
	// for each selected object and add each of its neighbor to
	// the list so long as it isn't already selected.	
	
	HighlightState& hs = *highlight_state;
	std::vector<bool>& h = hs.GetHighlight();
	std::vector<int>& nh = hs.GetNewlyHighlighted();
	std::vector<int>& nuh = hs.GetNewlyUnhighlighted();
	int nh_cnt = 0;
	std::vector<bool> add_elem(gal_weights->num_obs, false);
	
	for (int i=0; i<gal_weights->num_obs; i++) {
		if (h[i]) {
			GalElement& e = gal_weights->gal[i];
			for (int j=0, jend=e.Size(); j<jend; j++) {
				int obs = e.elt(j);
				if (!h[obs] && !add_elem[obs]) {
					add_elem[obs] = true;
					nh[nh_cnt++] = obs;
				}
			}
		}
	}
	
	if (nh_cnt > 0) {
		hs.SetEventType(HighlightState::delta);
		hs.SetTotalNewlyHighlighted(nh_cnt);
		hs.SetTotalNewlyUnhighlighted(0);
		hs.notifyObservers();
	} else {
		LOG_MSG("No elements to add to current selection");
	}
	LOG_MSG("Exiting Project::AddNeighborsToSelection");
}

void Project::AddMeanCenters()
{
	LOG_MSG("In Project::AddMeanCenters");
	
	if (!grid_base || main_data.records.size() == 0) return;
	GetMeanCenters();
	if (mean_centers.size() != num_records) return;

	std::vector<double> x(num_records);
	std::vector<double> y(num_records);
	for (int i=0; i<num_records; i++) {
		x[i] = mean_centers[i]->center_o.x;
		y[i] = mean_centers[i]->center_o.y;
	}
	
	std::vector<SaveToTableEntry> data(2);
	data[0].d_val = &x;
	data[0].label = "X-Coordinates";
	data[0].field_default = "XMCTR";
	data[0].type = GeoDaConst::double_type;
	
	data[1].d_val = &y;
	data[1].label = "Y-Coordinates";
	data[1].field_default = "YMCTR";
	data[1].type = GeoDaConst::double_type;	
	
	SaveToTableDlg dlg(grid_base, NULL, data,
					   "Add Mean Centers to Table",
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
}

void Project::AddCentroids()
{
	LOG_MSG("In Project::AddCentroids");
	
	if (!grid_base || main_data.records.size() == 0) return;	
	GetCentroids();
	if (centroids.size() != num_records) return;
	
	std::vector<double> x(num_records);
	std::vector<double> y(num_records);
	for (int i=0; i<num_records; i++) {
		x[i] = centroids[i]->center_o.x;
		y[i] = centroids[i]->center_o.y;
	}
	
	std::vector<SaveToTableEntry> data(2);
	data[0].d_val = &x;
	data[0].label = "X-Coordinates";
	data[0].field_default = "XCNTRD";
	data[0].type = GeoDaConst::double_type;
	
	data[1].d_val = &y;
	data[1].label = "Y-Coordinates";
	data[1].field_default = "YCNTRD";
	data[1].type = GeoDaConst::double_type;	
	
	SaveToTableDlg dlg(grid_base, NULL, data,
					   "Add Centroids to Table",
					   wxDefaultPosition, wxSize(400,400));
	dlg.ShowModal();
}
	
bool Project::GetCenters(std::vector<double>& x, std::vector<double>& y)
{
	if (!main_data.records.size() == num_records) return false;
	x.resize(num_records);
	y.resize(num_records);
	
	const std::vector<MyPoint*>& pts = GetCentroids();
	for (int i=0; i<num_records; i++) {
		x[i] = pts[i]->center_o.x;
		y[i] = pts[i]->center_o.y;
	}
	return true;
}

const std::vector<MyPoint*>& Project::GetMeanCenters()
{
	int num_obs = main_data.records.size();
	if (mean_centers.size() == 0 && num_obs > 0) {
		if (main_data.header.shape_type == Shapefile::POINT) {
			mean_centers.resize(num_obs);
			Shapefile::PointContents* pc;
			for (int i=0; i<num_obs; i++) {
				pc = (Shapefile::PointContents*)
					main_data.records[i].contents_p;
				mean_centers[i] = new MyPoint(wxRealPoint(pc->x, pc->y));
			}
		} else if (main_data.header.shape_type == Shapefile::POLYGON) {
			mean_centers.resize(num_obs);
			Shapefile::PolygonContents* pc;
			for (int i=0; i<num_obs; i++) {
				pc = (Shapefile::PolygonContents*)
					main_data.records[i].contents_p;
				MyPolygon poly(pc);
				mean_centers[i] =
					new MyPoint(MyShapeAlgs::calculateMeanCenter(&poly));
			}
		}
	}
	return mean_centers;
}

const std::vector<MyPoint*>& Project::GetCentroids()
{
	int num_obs = main_data.records.size();
	if (centroids.size() == 0 && num_obs > 0) {
		if (main_data.header.shape_type == Shapefile::POINT) {
			centroids.resize(num_obs);
			Shapefile::PointContents* pc;
			for (int i=0; i<num_obs; i++) {
				pc = (Shapefile::PointContents*)
				main_data.records[i].contents_p;
				centroids[i] = new MyPoint(wxRealPoint(pc->x, pc->y));
			}
		} else if (main_data.header.shape_type == Shapefile::POLYGON) {
			centroids.resize(num_obs);
			Shapefile::PolygonContents* pc;
			for (int i=0; i<num_obs; i++) {
				pc = (Shapefile::PolygonContents*)
					main_data.records[i].contents_p;
				MyPolygon poly(pc);
				centroids[i] =
					new MyPoint(MyShapeAlgs::calculateCentroid(&poly));
			}
		}
	}
	return centroids;	
}

void Project::GetMeanCenters(std::vector<double>& x, std::vector<double>& y)
{
	GetMeanCenters();
	int num_obs = mean_centers.size();
	if (x.size() < num_obs) x.resize(num_obs);
	if (y.size() < num_obs) y.resize(num_obs);
	for (int i=0; i<num_obs; i++) {
		x[i] = mean_centers[i]->center_o.x;
		y[i] = mean_centers[i]->center_o.y;
	}
}

void Project::GetCentroids(std::vector<double>& x, std::vector<double>& y)
{
	GetCentroids();
	int num_obs = centroids.size();
	if (x.size() < num_obs) x.resize(num_obs);
	if (y.size() < num_obs) y.resize(num_obs);
	for (int i=0; i<num_obs; i++) {
		x[i] = centroids[i]->center_o.x;
		y[i] = centroids[i]->center_o.y;
	}
}

const std::vector<MyPolygon*>& Project::GetVoronoiPolygons()
{
	if (voronoi_polygons.size() == num_records) {
		return voronoi_polygons;
	} else {
		for (int i=0; i<voronoi_polygons.size(); i++) {
			delete voronoi_polygons[i];
		}
		voronoi_polygons.clear();
	}
	
	std::vector<double> x(num_records);
	std::vector<double> y(num_records);
	GetCenters(x,y);
	
	GeoDa::VoronoiUtils::MakePolygons(x, y, voronoi_polygons,
									  voronoi_bb_xmin, voronoi_bb_ymin,
									  voronoi_bb_xmax, voronoi_bb_ymax);
	for (int i=0, iend=voronoi_polygons.size(); i<iend; i++) {
		voronoi_polygons[i]->setPen(*wxBLACK_PEN);
		voronoi_polygons[i]->setBrush(*wxTRANSPARENT_BRUSH);
	}
	
	return voronoi_polygons;
}


