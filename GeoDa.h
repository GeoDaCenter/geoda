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

#ifndef __GEODA_CENTER_GEODA_H__
#define __GEODA_CENTER_GEODA_H__

#include <list>
#include <string>
#include <vector>
#include <boost/uuid/uuid.hpp>
#include <wx/arrstr.h>
#include <wx/app.h>
#include <wx/clipbrd.h>
#include <wx/colordlg.h>
#include <wx/cmdline.h>
#include <wx/docview.h>
#include <wx/filename.h>
#include <wx/ipc.h>
#include <wx/string.h>
#include <wx/snglinst.h>
#include <wx/timer.h>
#include <wx/toolbar.h>
#include <wx/xrc/xh_auitoolb.h>

// Forward Declarations
class ProgressDlg;
class Project;
class CatClassifFrame;
class GdaApp;
class GdaFrame;
class GdaServer;
class GdaClient;
class GdaConnection;
class LineChartFrame;

/** Main appilcation class. */
class GdaApp: public wxApp
{
public:
	GdaApp();
	virtual ~GdaApp();
	virtual bool OnInit(void);
	virtual int OnExit(void);
	virtual void OnFatalException(void);
	virtual void OnInitCmdLine(wxCmdLineParser& parser);
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
	virtual void MacOpenFiles(const wxArrayString& fileNames);

	static const wxCmdLineEntryDesc globalCmdLineDesc[];
    
private:
	wxString cmd_line_proj_file_name;
	wxSingleInstanceChecker* checker;
	GdaServer* server;
};

DECLARE_APP(GdaApp)

/** Main toolbar frame. */
class GdaFrame: public wxFrame
{
public:
	GdaFrame(const wxString& title,
			const wxPoint& pos, const wxSize& size, long style);
	virtual ~GdaFrame();
	
    
	void EnableTool(const wxString& id_str, bool enable);
	void EnableTool(int xrc_id, bool enable);
	boost::uuids::uuid GetWeightsId(const wxString& caption = "Choose Weights");

    void OnSize(wxSizeEvent& event);
	void OnKeyEvent(wxKeyEvent& event);
	void OnToolOpenNewTable(wxCommandEvent& event);
	void OnOpenNewTable();
	bool OnCloseProject(bool ignore_unsaved_changes = false);
	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);
	void OnCloseProjectEvt(wxCommandEvent& event);
	void OnQuit(wxCommandEvent& WXUNUSED(event));
	
	void NewProjectFromFile(const wxString& full_file_path);
	void OnNewProject(wxCommandEvent& event);
	void OnNewProjectFromShp(wxCommandEvent& event);
	void OnNewProjectFromSqlite(wxCommandEvent& event);
	void OnNewProjectFromCsv(wxCommandEvent& event);
	void OnNewProjectFromDbf(wxCommandEvent& event);
	void OnNewProjectFromGdb(wxCommandEvent& event);
	void OnNewProjectFromJson(wxCommandEvent& event);
	void OnNewProjectFromGml(wxCommandEvent& event);
	void OnNewProjectFromKml(wxCommandEvent& event);
	void OnNewProjectFromMapinfo(wxCommandEvent& event);
	void OnNewProjectFromXls(wxCommandEvent& event);
	void OpenProject(const wxString& full_proj_path);
	void OnOpenProject(wxCommandEvent& event);
	void OnSaveProject(wxCommandEvent& event);
	void OnSaveAsProject(wxCommandEvent& event);
	
	void OnShowProjectInfo(wxCommandEvent& event);
	
	void OnHtmlEntry(int entry);
	void OnHtmlEntry0(wxCommandEvent& event);
	void OnHtmlEntry1(wxCommandEvent& event);
	void OnHtmlEntry2(wxCommandEvent& event);
	void OnHtmlEntry3(wxCommandEvent& event);
	void OnHtmlEntry4(wxCommandEvent& event);
	void OnHtmlEntry5(wxCommandEvent& event);
	void OnHtmlEntry6(wxCommandEvent& event);
	void OnHtmlEntry7(wxCommandEvent& event);
	void OnHtmlEntry8(wxCommandEvent& event);
	void OnHtmlEntry9(wxCommandEvent& event);
	
	void OnSelectWithRect(wxCommandEvent& event);
	void OnSelectWithCircle(wxCommandEvent& event);
	void OnSelectWithLine(wxCommandEvent& event);
	void OnSelectionMode(wxCommandEvent& event);
	void OnFitToWindowMode(wxCommandEvent& event);
	void OnFixedAspectRatioMode(wxCommandEvent& event);
	void OnZoomMode(wxCommandEvent& event);
	void OnPanMode(wxCommandEvent& event);
	void OnPrintCanvasState(wxCommandEvent& event);
	
    void OnChangeMapTransparency(wxCommandEvent& event);
    void OnCleanBasemap(wxCommandEvent& event);
    void OnSetNoBasemap(wxCommandEvent& event);
    void OnSetBasemap1(wxCommandEvent& event);
    void OnSetBasemap2(wxCommandEvent& event);
    void OnSetBasemap3(wxCommandEvent& event);
    void OnSetBasemap4(wxCommandEvent& event);
    void OnSetBasemap5(wxCommandEvent& event);
    void OnSetBasemap6(wxCommandEvent& event);
    void OnSetBasemap7(wxCommandEvent& event);
    void OnSetBasemap8(wxCommandEvent& event);
    void OnBasemapConfig(wxCommandEvent& event);
    
	void OnSaveCanvasImageAs(wxCommandEvent& event);
	void OnSaveSelectedToColumn(wxCommandEvent& event);
	void OnCanvasBackgroundColor(wxCommandEvent& event);
	void OnLegendUseScientificNotation(wxCommandEvent& event);
	void OnLegendBackgroundColor(wxCommandEvent& event);
	void OnSelectableFillColor(wxCommandEvent& event);
	void OnSelectableOutlineColor(wxCommandEvent& event);
	void OnSelectableOutlineVisible(wxCommandEvent& event);
	void OnHighlightColor(wxCommandEvent& event);
	
	void OnCopyImageToClipboard(wxCommandEvent& event);
	void OnCopyLegendToClipboard(wxCommandEvent& event);
	
	void OnToolsWeightsManager(wxCommandEvent& event);
	void OnToolsWeightsCreate(wxCommandEvent& event);
	void OnConnectivityHistView(wxCommandEvent& event);
	void OnConnectivityMapView(wxCommandEvent& event);
	void ShowConnectivityMapView(boost::uuids::uuid weights_id);
	
	void OnMapChoices(wxCommandEvent& event);
	
	void OnShapePointsFromASCII(wxCommandEvent& event);
	void OnShapePolygonsFromGrid(wxCommandEvent& event);
	void OnShapePolygonsFromBoundary(wxCommandEvent& event);
	void OnShapeToBoundary(wxCommandEvent& event);

	void OnShowTimeChooser(wxCommandEvent& event);
	void OnShowDataMovie(wxCommandEvent& event);
	void OnShowCatClassif(wxCommandEvent& event);
	CatClassifFrame* GetCatClassifFrame(bool useScientificNotation=false);
	void OnVarGroupingEditor(wxCommandEvent& event);
	void OnTimeEditor(wxCommandEvent& event);
	void OnMoveSelectedToTop(wxCommandEvent& event);
	void OnInvertSelection(wxCommandEvent& event);
	void OnClearSelection(wxCommandEvent& event);
	void OnRangeSelection(wxCommandEvent& event);
	void OnFieldCalculation(wxCommandEvent& event);
	void OnCalculator(wxCommandEvent& event);
	void OnAddCol(wxCommandEvent& event);
	void OnDeleteCol(wxCommandEvent& event);
	void OnEditFieldProperties(wxCommandEvent& event);
	void OnChangeFieldType(wxCommandEvent& event);
	void OnMergeTableData(wxCommandEvent& event);
	void OnExportToCsvFile(wxCommandEvent& event); // not used currently
	void OnExportToOGR(wxCommandEvent& event);
	void OnExportSelectedToOGR(wxCommandEvent& event);
	void OnGeneratePointShpFile(wxCommandEvent& event);
	
	void OnRegressionClassic(wxCommandEvent& event);
	
	void OnPublish(wxCommandEvent& event);

	void OnCondPlotChoices(wxCommandEvent& event);
	void OnShowConditionalMapView(wxCommandEvent& event);
	void OnShowConditionalScatterView(wxCommandEvent& event);
	void OnShowConditionalHistView(wxCommandEvent& event);
	
	void OnShowCartogramNewView(wxCommandEvent& event );
	void OnCartogramImprove1(wxCommandEvent& event);
	void OnCartogramImprove2(wxCommandEvent& event);
	void OnCartogramImprove3(wxCommandEvent& event);
	void OnCartogramImprove4(wxCommandEvent& event);
	void OnCartogramImprove5(wxCommandEvent& event);
	void OnCartogramImprove6(wxCommandEvent& event);

	void OnExploreHist(wxCommandEvent& event);
	void OnExploreScatterNewPlot(wxCommandEvent& event);
	void OnExploreBubbleChart(wxCommandEvent& event);
	void OnExploreScatterPlotMat(wxCommandEvent& event);
	void OnExploreTestMap(wxCommandEvent& event);
	void OnExploreBox(wxCommandEvent& event);
	void OnExploreNewBox(wxCommandEvent& event);
	void OnExplorePCP(wxCommandEvent& event);
	void OnExplore3DP(wxCommandEvent& event);
	void OnExploreLineChart(wxCommandEvent& event);
	void OnExploreCovScatterPlot(wxCommandEvent& event);
	void OnExploreCorrelogram(wxCommandEvent& event);
	
	void OnMoranMenuChoices(wxCommandEvent& event);
	void OnOpenMSPL(wxCommandEvent& event);
	void OnOpenGMoran(wxCommandEvent& event);
    void OnOpenDiffMoran(wxCommandEvent& event);
	void OnOpenMoranEB(wxCommandEvent& event);
	void OnLisaMenuChoices(wxCommandEvent& event);
	void OnGetisMenuChoices(wxCommandEvent& event);
	void OnOpenUniLisa(wxCommandEvent& event);
	void OnOpenMultiLisa(wxCommandEvent& event);
	void OnOpenLisaEB(wxCommandEvent& event);
	void OnOpenGetisOrd(wxCommandEvent& event);
	void OnOpenGetisOrdStar(wxCommandEvent& event);

	void OnNewCustomCatClassifA(wxCommandEvent& event);
	void OnNewCustomCatClassifB(wxCommandEvent& event);
	void OnNewCustomCatClassifC(wxCommandEvent& event);
	void OnCCClassifA(int cc_menu_num);
	void OnCCClassifB(int cc_menu_num);
	void OnCCClassifC(int cc_menu_num);
	void OnCCClassifA0(wxCommandEvent& e);
	void OnCCClassifA1(wxCommandEvent& e);
	void OnCCClassifA2(wxCommandEvent& e);
	void OnCCClassifA3(wxCommandEvent& e);
	void OnCCClassifA4(wxCommandEvent& e);
	void OnCCClassifA5(wxCommandEvent& e);
	void OnCCClassifA6(wxCommandEvent& e);
	void OnCCClassifA7(wxCommandEvent& e);
	void OnCCClassifA8(wxCommandEvent& e);
	void OnCCClassifA9(wxCommandEvent& e);
	void OnCCClassifA10(wxCommandEvent& e);
	void OnCCClassifA11(wxCommandEvent& e);
	void OnCCClassifA12(wxCommandEvent& e);
	void OnCCClassifA13(wxCommandEvent& e);
	void OnCCClassifA14(wxCommandEvent& e);
	void OnCCClassifA15(wxCommandEvent& e);
	void OnCCClassifA16(wxCommandEvent& e);
	void OnCCClassifA17(wxCommandEvent& e);
	void OnCCClassifA18(wxCommandEvent& e);
	void OnCCClassifA19(wxCommandEvent& e);
	void OnCCClassifA20(wxCommandEvent& e);
	void OnCCClassifA21(wxCommandEvent& e);
	void OnCCClassifA22(wxCommandEvent& e);
	void OnCCClassifA23(wxCommandEvent& e);
	void OnCCClassifA24(wxCommandEvent& e);
	void OnCCClassifA25(wxCommandEvent& e);
	void OnCCClassifA26(wxCommandEvent& e);
	void OnCCClassifA27(wxCommandEvent& e);
	void OnCCClassifA28(wxCommandEvent& e);
	void OnCCClassifA29(wxCommandEvent& e);
	void OnCCClassifB0(wxCommandEvent& e);
	void OnCCClassifB1(wxCommandEvent& e);
	void OnCCClassifB2(wxCommandEvent& e);
	void OnCCClassifB3(wxCommandEvent& e);
	void OnCCClassifB4(wxCommandEvent& e);
	void OnCCClassifB5(wxCommandEvent& e);
	void OnCCClassifB6(wxCommandEvent& e);
	void OnCCClassifB7(wxCommandEvent& e);
	void OnCCClassifB8(wxCommandEvent& e);
	void OnCCClassifB9(wxCommandEvent& e);
	void OnCCClassifB10(wxCommandEvent& e);
	void OnCCClassifB11(wxCommandEvent& e);
	void OnCCClassifB12(wxCommandEvent& e);
	void OnCCClassifB13(wxCommandEvent& e);
	void OnCCClassifB14(wxCommandEvent& e);
	void OnCCClassifB15(wxCommandEvent& e);
	void OnCCClassifB16(wxCommandEvent& e);
	void OnCCClassifB17(wxCommandEvent& e);
	void OnCCClassifB18(wxCommandEvent& e);
	void OnCCClassifB19(wxCommandEvent& e);
	void OnCCClassifB20(wxCommandEvent& e);
	void OnCCClassifB21(wxCommandEvent& e);
	void OnCCClassifB22(wxCommandEvent& e);
	void OnCCClassifB23(wxCommandEvent& e);
	void OnCCClassifB24(wxCommandEvent& e);
	void OnCCClassifB25(wxCommandEvent& e);
	void OnCCClassifB26(wxCommandEvent& e);
	void OnCCClassifB27(wxCommandEvent& e);
	void OnCCClassifB28(wxCommandEvent& e);
	void OnCCClassifB29(wxCommandEvent& e);
	void OnCCClassifC0(wxCommandEvent& e);
	void OnCCClassifC1(wxCommandEvent& e);
	void OnCCClassifC2(wxCommandEvent& e);
	void OnCCClassifC3(wxCommandEvent& e);
	void OnCCClassifC4(wxCommandEvent& e);
	void OnCCClassifC5(wxCommandEvent& e);
	void OnCCClassifC6(wxCommandEvent& e);
	void OnCCClassifC7(wxCommandEvent& e);
	void OnCCClassifC8(wxCommandEvent& e);
	void OnCCClassifC9(wxCommandEvent& e);
	void OnCCClassifC10(wxCommandEvent& e);
	void OnCCClassifC11(wxCommandEvent& e);
	void OnCCClassifC12(wxCommandEvent& e);
	void OnCCClassifC13(wxCommandEvent& e);
	void OnCCClassifC14(wxCommandEvent& e);
	void OnCCClassifC15(wxCommandEvent& e);
	void OnCCClassifC16(wxCommandEvent& e);
	void OnCCClassifC17(wxCommandEvent& e);
	void OnCCClassifC18(wxCommandEvent& e);
	void OnCCClassifC19(wxCommandEvent& e);
	void OnCCClassifC20(wxCommandEvent& e);
	void OnCCClassifC21(wxCommandEvent& e);
	void OnCCClassifC22(wxCommandEvent& e);
	void OnCCClassifC23(wxCommandEvent& e);
	void OnCCClassifC24(wxCommandEvent& e);
	void OnCCClassifC25(wxCommandEvent& e);
	void OnCCClassifC26(wxCommandEvent& e);
	void OnCCClassifC27(wxCommandEvent& e);
	void OnCCClassifC28(wxCommandEvent& e);
	void OnCCClassifC29(wxCommandEvent& e);
	
	void OnOpenThemelessMap(wxCommandEvent& event);
	void OnThemelessMap(wxCommandEvent& event);
	
	void OnOpenQuantile1(wxCommandEvent& event);
	void OnOpenQuantile2(wxCommandEvent& event);
	void OnOpenQuantile3(wxCommandEvent& event);
	void OnOpenQuantile4(wxCommandEvent& event);
	void OnOpenQuantile5(wxCommandEvent& event);
	void OnOpenQuantile6(wxCommandEvent& event);
	void OnOpenQuantile7(wxCommandEvent& event);
	void OnOpenQuantile8(wxCommandEvent& event);
	void OnOpenQuantile9(wxCommandEvent& event);
	void OnOpenQuantile10(wxCommandEvent& event);
	void OpenQuantile(int num_cats);
	void OnQuantile1(wxCommandEvent& event);
	void OnQuantile2(wxCommandEvent& event);
	void OnQuantile3(wxCommandEvent& event);
	void OnQuantile4(wxCommandEvent& event);
	void OnQuantile5(wxCommandEvent& event);
	void OnQuantile6(wxCommandEvent& event);
	void OnQuantile7(wxCommandEvent& event);
	void OnQuantile8(wxCommandEvent& event);
	void OnQuantile9(wxCommandEvent& event);
	void OnQuantile10(wxCommandEvent& event);
	void ChangeToQuantile(int num_cats);

	void OnOpenPercentile(wxCommandEvent& event);
	void OnPercentile(wxCommandEvent& event);

	void OnOpenHinge15(wxCommandEvent& event);
	void OnHinge15(wxCommandEvent& event);

	void OnOpenHinge30(wxCommandEvent& event);
	void OnHinge30(wxCommandEvent& event);

	void OnOpenStddev(wxCommandEvent& event);
	void OnStddev(wxCommandEvent& event);

	void OnOpenNaturalBreaks1(wxCommandEvent& event);
	void OnOpenNaturalBreaks2(wxCommandEvent& event);
	void OnOpenNaturalBreaks3(wxCommandEvent& event);
	void OnOpenNaturalBreaks4(wxCommandEvent& event);
	void OnOpenNaturalBreaks5(wxCommandEvent& event);
	void OnOpenNaturalBreaks6(wxCommandEvent& event);
	void OnOpenNaturalBreaks7(wxCommandEvent& event);
	void OnOpenNaturalBreaks8(wxCommandEvent& event);
	void OnOpenNaturalBreaks9(wxCommandEvent& event);
	void OnOpenNaturalBreaks10(wxCommandEvent& event);
	void OpenNaturalBreaks(int num_cats);
	void OnNaturalBreaks1(wxCommandEvent& event);
	void OnNaturalBreaks2(wxCommandEvent& event);
	void OnNaturalBreaks3(wxCommandEvent& event);
	void OnNaturalBreaks4(wxCommandEvent& event);
	void OnNaturalBreaks5(wxCommandEvent& event);
	void OnNaturalBreaks6(wxCommandEvent& event);
	void OnNaturalBreaks7(wxCommandEvent& event);
	void OnNaturalBreaks8(wxCommandEvent& event);
	void OnNaturalBreaks9(wxCommandEvent& event);
	void OnNaturalBreaks10(wxCommandEvent& event);
	void ChangeToNaturalBreaks(int num_cats);
	
	void OnOpenEqualIntervals1(wxCommandEvent& event);
	void OnOpenEqualIntervals2(wxCommandEvent& event);
	void OnOpenEqualIntervals3(wxCommandEvent& event);
	void OnOpenEqualIntervals4(wxCommandEvent& event);
	void OnOpenEqualIntervals5(wxCommandEvent& event);
	void OnOpenEqualIntervals6(wxCommandEvent& event);
	void OnOpenEqualIntervals7(wxCommandEvent& event);
	void OnOpenEqualIntervals8(wxCommandEvent& event);
	void OnOpenEqualIntervals9(wxCommandEvent& event);
	void OnOpenEqualIntervals10(wxCommandEvent& event);
	void OpenEqualIntervals(int num_cats);
	void OnEqualIntervals1(wxCommandEvent& event);
	void OnEqualIntervals2(wxCommandEvent& event);
	void OnEqualIntervals3(wxCommandEvent& event);
	void OnEqualIntervals4(wxCommandEvent& event);
	void OnEqualIntervals5(wxCommandEvent& event);
	void OnEqualIntervals6(wxCommandEvent& event);
	void OnEqualIntervals7(wxCommandEvent& event);
	void OnEqualIntervals8(wxCommandEvent& event);
	void OnEqualIntervals9(wxCommandEvent& event);
	void OnEqualIntervals10(wxCommandEvent& event);
	void ChangeToEqualIntervals(int num_cats);

	void OnOpenUniqueValues(wxCommandEvent& event);
	void OnUniqueValues(wxCommandEvent& event);

	void OnSaveCategories(wxCommandEvent& event);
	
	void OnOpenRawrate(wxCommandEvent& event);
	void OnRawrate(wxCommandEvent& event);

	void OnOpenExcessrisk(wxCommandEvent& event);
	void OnExcessrisk(wxCommandEvent& event);

	void OnOpenEmpiricalBayes(wxCommandEvent& event);
	void OnEmpiricalBayes(wxCommandEvent& event);

	void OnOpenSpatialRate(wxCommandEvent& event);
	void OnSpatialRate(wxCommandEvent& event);

	void OnOpenSpatialEmpiricalBayes(wxCommandEvent& event);
	void OnSpatialEmpiricalBayes(wxCommandEvent& event);
	
	void OnCondHorizThemelessMap(wxCommandEvent& event);
	
	void OnCondHorizQuant1(wxCommandEvent& event);
	void OnCondHorizQuant2(wxCommandEvent& event);
	void OnCondHorizQuant3(wxCommandEvent& event);
	void OnCondHorizQuant4(wxCommandEvent& event);
	void OnCondHorizQuant5(wxCommandEvent& event);
	void OnCondHorizQuant6(wxCommandEvent& event);
	void OnCondHorizQuant7(wxCommandEvent& event);
	void OnCondHorizQuant8(wxCommandEvent& event);
	void OnCondHorizQuant9(wxCommandEvent& event);
	void OnCondHorizQuant10(wxCommandEvent& event);
	void ChangeToCondHorizQuant(int num_cats);
	
	void OnCondHorizPercentile(wxCommandEvent& event);
	void OnCondHorizHinge15(wxCommandEvent& event);
	void OnCondHorizHinge30(wxCommandEvent& event);
	void OnCondHorizStddev(wxCommandEvent& event);
	
	void OnCondHorizNatBrks1(wxCommandEvent& event);
	void OnCondHorizNatBrks2(wxCommandEvent& event);
	void OnCondHorizNatBrks3(wxCommandEvent& event);
	void OnCondHorizNatBrks4(wxCommandEvent& event);
	void OnCondHorizNatBrks5(wxCommandEvent& event);
	void OnCondHorizNatBrks6(wxCommandEvent& event);
	void OnCondHorizNatBrks7(wxCommandEvent& event);
	void OnCondHorizNatBrks8(wxCommandEvent& event);
	void OnCondHorizNatBrks9(wxCommandEvent& event);
	void OnCondHorizNatBrks10(wxCommandEvent& event);
	void ChangeToCondHorizNatBrks(int num_cats);
	
	void OnCondHorizEquInts1(wxCommandEvent& event);
	void OnCondHorizEquInts2(wxCommandEvent& event);
	void OnCondHorizEquInts3(wxCommandEvent& event);
	void OnCondHorizEquInts4(wxCommandEvent& event);
	void OnCondHorizEquInts5(wxCommandEvent& event);
	void OnCondHorizEquInts6(wxCommandEvent& event);
	void OnCondHorizEquInts7(wxCommandEvent& event);
	void OnCondHorizEquInts8(wxCommandEvent& event);
	void OnCondHorizEquInts9(wxCommandEvent& event);
	void OnCondHorizEquInts10(wxCommandEvent& event);
	void ChangeToCondHorizEquInts(int num_cats);
	
	void OnCondHorizUniqueValues(wxCommandEvent& event);
	
	void OnCondVertThemelessMap(wxCommandEvent& event);

	void OnCondVertQuant1(wxCommandEvent& event);
	void OnCondVertQuant2(wxCommandEvent& event);
	void OnCondVertQuant3(wxCommandEvent& event);
	void OnCondVertQuant4(wxCommandEvent& event);
	void OnCondVertQuant5(wxCommandEvent& event);
	void OnCondVertQuant6(wxCommandEvent& event);
	void OnCondVertQuant7(wxCommandEvent& event);
	void OnCondVertQuant8(wxCommandEvent& event);
	void OnCondVertQuant9(wxCommandEvent& event);
	void OnCondVertQuant10(wxCommandEvent& event);
	void ChangeToCondVertQuant(int num_cats);	
	
	void OnCondVertPercentile(wxCommandEvent& event);
	void OnCondVertHinge15(wxCommandEvent& event);
	void OnCondVertHinge30(wxCommandEvent& event);
	void OnCondVertStddev(wxCommandEvent& event);
	
	void OnCondVertNatBrks1(wxCommandEvent& event);
	void OnCondVertNatBrks2(wxCommandEvent& event);
	void OnCondVertNatBrks3(wxCommandEvent& event);
	void OnCondVertNatBrks4(wxCommandEvent& event);
	void OnCondVertNatBrks5(wxCommandEvent& event);
	void OnCondVertNatBrks6(wxCommandEvent& event);
	void OnCondVertNatBrks7(wxCommandEvent& event);
	void OnCondVertNatBrks8(wxCommandEvent& event);
	void OnCondVertNatBrks9(wxCommandEvent& event);
	void OnCondVertNatBrks10(wxCommandEvent& event);
	void ChangeToCondVertNatBrks(int num_cats);
	
	void OnCondVertEquInts1(wxCommandEvent& event);
	void OnCondVertEquInts2(wxCommandEvent& event);
	void OnCondVertEquInts3(wxCommandEvent& event);
	void OnCondVertEquInts4(wxCommandEvent& event);
	void OnCondVertEquInts5(wxCommandEvent& event);
	void OnCondVertEquInts6(wxCommandEvent& event);
	void OnCondVertEquInts7(wxCommandEvent& event);
	void OnCondVertEquInts8(wxCommandEvent& event);
	void OnCondVertEquInts9(wxCommandEvent& event);
	void OnCondVertEquInts10(wxCommandEvent& event);
	void ChangeToCondVertEquInts(int num_cats);
	
	void OnCondVertUniqueValues(wxCommandEvent& event);
		
	void OnSaveResults(wxCommandEvent& event);
	
	void OnHistogramIntervals(wxCommandEvent& event);
	void OnSaveConnectivityToTable(wxCommandEvent& event);
	void OnSelectIsolates(wxCommandEvent& event);
	
	void OnRan99Per(wxCommandEvent& event);
	void OnRan199Per(wxCommandEvent& event);
	void OnRan499Per(wxCommandEvent& event);
	void OnRan999Per(wxCommandEvent& event);
	void OnRanOtherPer(wxCommandEvent& event);
	
	void OnUseSpecifiedSeed(wxCommandEvent& event);
	void OnSpecifySeedDlg(wxCommandEvent& event);
	
	void OnSaveMoranI(wxCommandEvent& event);
	
	void OnSigFilter05(wxCommandEvent& event);
	void OnSigFilter01(wxCommandEvent& event);
	void OnSigFilter001(wxCommandEvent& event);
	void OnSigFilter0001(wxCommandEvent& event);
	
	void OnSaveGetisOrd(wxCommandEvent& event);
	void OnSaveLisa(wxCommandEvent& event);
	
	void OnSelectCores(wxCommandEvent& event);
	void OnSelectNeighborsOfCores(wxCommandEvent& event);
	void OnSelectCoresAndNeighbors(wxCommandEvent& event);
	
	void OnAddMeanCenters(wxCommandEvent& event);
	void OnAddCentroids(wxCommandEvent& event);
	void OnDisplayMeanCenters(wxCommandEvent& event);
	void OnDisplayCentroids(wxCommandEvent& event);
	void OnDisplayVoronoiDiagram(wxCommandEvent& event);
	void OnExportVoronoi(wxCommandEvent& event);
	void OnExportMeanCntrs(wxCommandEvent& event);
	void OnExportCentroids(wxCommandEvent& event);
	void OnSaveVoronoiDupsToTable(wxCommandEvent& event);
	
	// ScatterPlot and PCP specific callbacks
	void OnViewStandardizedData(wxCommandEvent& event);
	void OnViewOriginalData(wxCommandEvent& event);
	// ScatterPlot specific callbacks
	void OnViewLinearSmoother(wxCommandEvent& event);
	void OnViewLowessSmoother(wxCommandEvent& event);
	void OnEditLowessParams(wxCommandEvent& event);
	void OnEditVariables(wxCommandEvent& event);
	void OnViewRegimesRegression(wxCommandEvent& event);
	void OnViewRegressionSelectedExcluded(wxCommandEvent& event);
	void OnViewRegressionSelected(wxCommandEvent& event);
	void OnCompareRegimes(wxCommandEvent& event);
	void OnCompareTimePeriods(wxCommandEvent& event);
	void OnCompareRegAndTmPer(wxCommandEvent& event);
	void OnDisplayStatistics(wxCommandEvent& event);
	void OnShowAxesThroughOrigin(wxCommandEvent& event);
	// BoxPlot and Histogram specific callback
	void OnShowAxes(wxCommandEvent& event);
	// Conditional Scatter Plot
	void OnDisplayAxesScaleValues(wxCommandEvent& event);
	void OnDisplaySlopeValues(wxCommandEvent& event);
	
	void OnTimeSyncVariable(int var_index);
	void OnTimeSyncVariable1(wxCommandEvent& event);
	void OnTimeSyncVariable2(wxCommandEvent& event);
	void OnTimeSyncVariable3(wxCommandEvent& event);
	void OnTimeSyncVariable4(wxCommandEvent& event);
	
	void OnFixedScaleVariable(int var_index);
	void OnFixedScaleVariable1(wxCommandEvent& event);
	void OnFixedScaleVariable2(wxCommandEvent& event);
	void OnFixedScaleVariable3(wxCommandEvent& event);
	void OnFixedScaleVariable4(wxCommandEvent& event);
	
	void OnPlotsPerView(int plots_per_view);
	void OnPlotsPerView1(wxCommandEvent& event);
	void OnPlotsPerView2(wxCommandEvent& event);
	void OnPlotsPerView3(wxCommandEvent& event);
	void OnPlotsPerView4(wxCommandEvent& event);
	void OnPlotsPerView5(wxCommandEvent& event);
	void OnPlotsPerView6(wxCommandEvent& event);
	void OnPlotsPerView7(wxCommandEvent& event);
	void OnPlotsPerView8(wxCommandEvent& event);
	void OnPlotsPerView9(wxCommandEvent& event);
	void OnPlotsPerView10(wxCommandEvent& event);
	void OnPlotsPerViewOther(wxCommandEvent& event);
	void OnPlotsPerViewAll(wxCommandEvent& event);
	
	void OnDisplayStatusBar(wxCommandEvent& event);
	
	void OnHelpAbout(wxCommandEvent& event);
	void OnCheckUpdates(wxCommandEvent& event);
	void OnCheckTestMode(wxCommandEvent& event);
    
    void OnTableSetLocale(wxCommandEvent& event);
    void OnEncodingUTF8(wxCommandEvent& event);
	void OnEncodingUTF16(wxCommandEvent& event);
	void OnEncodingWindows1250(wxCommandEvent& event);
	void OnEncodingWindows1251(wxCommandEvent& event);
	void OnEncodingWindows1254(wxCommandEvent& event);
	void OnEncodingWindows1255(wxCommandEvent& event);
	void OnEncodingWindows1256(wxCommandEvent& event);
	void OnEncodingWindows1258(wxCommandEvent& event);
	void OnEncodingCP852(wxCommandEvent& event);
	void OnEncodingCP866(wxCommandEvent& event);
	void OnEncodingISO8859_1(wxCommandEvent& event);
	void OnEncodingISO8859_2(wxCommandEvent& event);
	void OnEncodingISO8859_3(wxCommandEvent& event);
	void OnEncodingISO8859_5(wxCommandEvent& event);
	void OnEncodingISO8859_7(wxCommandEvent& event);
	void OnEncodingISO8859_8(wxCommandEvent& event);
	void OnEncodingISO8859_9(wxCommandEvent& event);
	void OnEncodingISO8859_10(wxCommandEvent& event);
	void OnEncodingISO8859_15(wxCommandEvent& event);
	void OnEncodingGB2312(wxCommandEvent& event);
	void OnEncodingBIG5(wxCommandEvent& event);
	void OnEncodingKOI8_R(wxCommandEvent& event);
	void OnEncodingSHIFT_JIS(wxCommandEvent& event);
	void OnEncodingEUC_JP(wxCommandEvent& event);
	void OnEncodingEUC_KR(wxCommandEvent& event);
    void SetEncodingCheckmarks(wxFontEncoding e);
    void SetBasemapCheckmarks(int idx);
    
	void DisplayRegression(const wxString dump);

	void UpdateToolbarAndMenus();
	void SetMenusToDefault();

	static Project* GetProject() { return projectOpen ? project_p : 0; }
	static GdaFrame* GetGdaFrame() { return gda_frame; }
	static bool IsProjectOpen();
	
	struct MenuItem {
		MenuItem(const wxString& t, const wxString& u) :menu_title(t), url(u){};
		wxString menu_title; wxString url; };
	static std::vector<MenuItem> htmlMenuItems;
	static bool GetHtmlMenuItems();
	
	// GetHtmlMenuItems helper functions
	static bool GetHtmlMenuItemsJson();
	static bool GetHtmlMenuItemsSqlite();
	static int sqlite3_GetHtmlMenuItemsCB(void *data, int argc,
										  char **argv, char **azColName);
	
    void CheckUpdate();

private:
	static void SetProjectOpen(bool open);
    
    bool hasUpdate;

	static GdaFrame* gda_frame;
	static Project* project_p;
	static bool projectOpen;
	static std::list<wxAuiToolBar*> toolbar_list; // not currently used
	
	DECLARE_EVENT_TABLE()
};

class GdaServer : public wxServer {
public:
	virtual wxConnectionBase* OnAcceptConnection(const wxString& topic);
};

class GdaClient : public wxClient {
public:
	GdaClient() {}
	virtual wxConnectionBase* OnMakeConnection();
};

class GdaConnection : public wxConnection {
public:
	GdaConnection() {}
	virtual ~GdaConnection() {}
	
	virtual bool OnExec(const wxString &topic, const wxString &data);
};

/** This helper class is a workaround for an issue that is currently unique
 to LineChartCanvas, but might apply to other views in the future.
 Several menu items in LineChartCanvas result in an action that deletes
 the current LineChartCanvas from the parent Frame.  This timer class
 decouples the this action from the original wxWidgets menu popup callback
 by calling the indended callback again after a 100 ms delay. */
class LineChartEventDelay : public wxTimer
{
public:
	LineChartEventDelay();
	LineChartEventDelay(LineChartFrame* lc_frame, const wxString& cb_name);
	virtual ~LineChartEventDelay();
	
	LineChartFrame* lc_frame;
	wxString cb_name;
	virtual void Notify();
};

#endif

