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

#ifndef __GEODA_CENTER_GEODA_H__
#define __GEODA_CENTER_GEODA_H__

#include <string>
#include <list>
#include <wx/app.h>
#include <wx/clipbrd.h>
#include <wx/colordlg.h>
#include <wx/docview.h>
#include <wx/filename.h>
#include <wx/string.h>
#include <wx/timer.h>
#include <wx/toolbar.h>

// Forward Declarations
class GalWeight;
class ProgressDlg;
class Project;
class CatClassifFrame;

/** Main appilcation class. */
class MyApp: public wxApp
{
public:
	//MyApp(void);
	virtual bool OnInit(void);
	virtual int OnExit(void);
	virtual void OnFatalException(void);
	//void OnTimer(wxTimerEvent& event);
private:
	//static wxTimer* timer;
	//DECLARE_EVENT_TABLE()
};

DECLARE_APP(MyApp)

/** Main toolbar frame. */
class MyFrame: public wxFrame
{
public:
	MyFrame(const wxString& title,
			const wxPoint& pos, const wxSize& size, long style);
	virtual ~MyFrame();
	
	void EnableTool(const wxString& id_str, bool enable);
	void EnableTool(int xrc_id, bool enable);
	GalWeight* GetGal();

	void OnKeyEvent(wxKeyEvent& event);
	void OnToolOpenNewTable(wxCommandEvent& event);
	void OnOpenNewTable();
	/** Opens a new SHP file and initializes many global variables. */
	void OnOpenShapefile(wxCommandEvent& event);
	/** open columbus shapefile for testing */
	void OpenColumbusTest();
	void OnOpenTableOnly(wxCommandEvent& event);
	void OnOpenSpTmShapefile(wxCommandEvent& event);
	void OnOpenSpTmTableOnly(wxCommandEvent& event);
	void OpenProject(bool table_only, bool space_time);
	bool OnCloseMap(bool ignore_unsaved_changes = false);
	void OnClose(wxCloseEvent& event);
	void OnMenuClose(wxCommandEvent& event);
	void OnCloseAll(wxCommandEvent& event);
	void OnQuit(wxCommandEvent& WXUNUSED(event));

	void OnSelectWithRect(wxCommandEvent& event);
	void OnSelectWithCircle(wxCommandEvent& event);
	void OnSelectWithLine(wxCommandEvent& event);
	void OnSelectionMode(wxCommandEvent& event);
	void OnFitToWindowMode(wxCommandEvent& event);
	void OnFixedAspectRatioMode(wxCommandEvent& event);
	void OnZoomMode(wxCommandEvent& event);
	void OnZoomIn(wxCommandEvent& event);
	void OnZoomOut(wxCommandEvent& event);
	void OnPanMode(wxCommandEvent& event);
	void OnPrintCanvasState(wxCommandEvent& event);
	
	void OnSaveCanvasImageAs(wxCommandEvent& event);
	void OnSaveSelectedToColumn(wxCommandEvent& event);
	void OnCanvasBackgroundColor(wxCommandEvent& event);
	void OnLegendBackgroundColor(wxCommandEvent& event);
	void OnSelectableFillColor(wxCommandEvent& event);
	void OnSelectableOutlineColor(wxCommandEvent& event);
	void OnSelectableOutlineVisible(wxCommandEvent& event);
	void OnHighlightColor(wxCommandEvent& event);
	
	void OnSetDefaultVariableSettings(wxCommandEvent& WXUNUSED(event));
	void OnCopyImageToClipboard(wxCommandEvent& event);
	void OnCopyLegendToClipboard(wxCommandEvent& event);
	
	void OnToolsWeightsOpen(wxCommandEvent& event);  
	void OnToolsWeightsCreate(wxCommandEvent& event);
	void OnConnectivityHistView(wxCommandEvent& event);

	void OnMapChoices(wxCommandEvent& event);
	void OnOpenChoices(wxCommandEvent& event);
	
	void OnShapePointsFromASCII(wxCommandEvent& event);
	void OnShapePolygonsFromGrid(wxCommandEvent& event);
	void OnShapePolygonsFromBoundary(wxCommandEvent& event);
	void OnShapeToBoundary(wxCommandEvent& event);

	void OnShowTimeChooser(wxCommandEvent& event);
	void OnShowDataMovie(wxCommandEvent& event);
	void OnShowCatClassif(wxCommandEvent& event);
	CatClassifFrame* GetCatClassifFrame();
	void OnSpaceTimeTool(wxCommandEvent& event);
	void OnMoveSelectedToTop(wxCommandEvent& event);
	void OnClearSelection(wxCommandEvent& event);
	void OnRangeSelection(wxCommandEvent& event);
	void OnFieldCalculation(wxCommandEvent& event);
	void OnAddCol(wxCommandEvent& event);
	void OnDeleteCol(wxCommandEvent& event);
	void OnEditFieldProperties(wxCommandEvent& event);
	void OnMergeTableData(wxCommandEvent& event);
	void OnSaveProject(wxCommandEvent& event);
	void OnSaveAsProject(wxCommandEvent& event);
	void OnExportToCsvFile(wxCommandEvent& event);
	bool SaveTableSpace();
	bool SaveTableSpaceTime();
	void OnGeneratePointShpFile(wxCommandEvent& event);
	
	void OnRegressionClassic(wxCommandEvent& event);

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
	void OnExploreScatterplot(wxCommandEvent& event);
	void OnExploreScatterNewPlot(wxCommandEvent& event);
	void OnExploreBubbleChart(wxCommandEvent& event);
	void OnExploreTestMap(wxCommandEvent& event);
	void OnExploreTestTable(wxCommandEvent& event);
	void OnExploreBox(wxCommandEvent& event);
	void OnExploreNewBox(wxCommandEvent& event);
	void OnExplorePCP(wxCommandEvent& event);
	void OnExplore3DP(wxCommandEvent& event);

	void OnMoranMenuChoices(wxCommandEvent& event);
	void OnOpenMSPL(wxCommandEvent& event);
	void OnOpenGMoran(wxCommandEvent& event);
	void OnOpenMoranEB(wxCommandEvent& event);
	void OnLisaMenuChoices(wxCommandEvent& event);
	void OnOpenUniLisa(wxCommandEvent& event);
	void OnOpenMultiLisa(wxCommandEvent& event);
	void OnOpenLisaEB(wxCommandEvent& event);
	void OnOpenGetisOrd(wxCommandEvent& event);

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
	void OnOpenQuantile(wxCommandEvent& event);
	void OnQuantile(wxCommandEvent& event);
	void OnOpenPercentile(wxCommandEvent& event);
	void OnPercentile(wxCommandEvent& event);
	void OnOpenHinge15(wxCommandEvent& event);
	void OnHinge15(wxCommandEvent& event);
	void OnOpenHinge30(wxCommandEvent& event);
	void OnHinge30(wxCommandEvent& event);
	void OnOpenStddev(wxCommandEvent& event);
	void OnStddev(wxCommandEvent& event);
	void OnOpenNaturalBreaks(wxCommandEvent& event);
	void OnNaturalBreaks(wxCommandEvent& event);
	void OnOpenEqualIntervals(wxCommandEvent& event);
	void OnEqualIntervals(wxCommandEvent& event);
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
	void OnCondHorizQuantile(wxCommandEvent& event);
	void OnCondHorizPercentile(wxCommandEvent& event);
	void OnCondHorizHinge15(wxCommandEvent& event);
	void OnCondHorizHinge30(wxCommandEvent& event);
	void OnCondHorizStddev(wxCommandEvent& event);
	void OnCondHorizNaturalBreaks(wxCommandEvent& event);
	void OnCondHorizEqualIntervals(wxCommandEvent& event);
	void OnCondHorizUniqueValues(wxCommandEvent& event);
	
	void OnCondVertThemelessMap(wxCommandEvent& event);
	void OnCondVertQuantile(wxCommandEvent& event);
	void OnCondVertPercentile(wxCommandEvent& event);
	void OnCondVertHinge15(wxCommandEvent& event);
	void OnCondVertHinge30(wxCommandEvent& event);
	void OnCondVertStddev(wxCommandEvent& event);
	void OnCondVertNaturalBreaks(wxCommandEvent& event);
	void OnCondVertEqualIntervals(wxCommandEvent& event);
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
	void OnAddNeighborsToSelection(wxCommandEvent& event);
	
	void OnAddMeanCenters(wxCommandEvent& event);
	void OnAddCentroids(wxCommandEvent& event);
	void OnDisplayMeanCenters(wxCommandEvent& event);
	void OnDisplayCentroids(wxCommandEvent& event);
	void OnDisplayVoronoiDiagram(wxCommandEvent& event);
	void OnSaveVoronoiToShapefile(wxCommandEvent& event);
	void OnSaveMeanCntrsToShapefile(wxCommandEvent& event);
	void OnSaveCentroidsToShapefile(wxCommandEvent& event);
	void OnSaveVoronoiDupsToTable(wxCommandEvent& event);
	
	// ScatterPlot and PCP specific callbacks
	void OnViewStandardizedData(wxCommandEvent& event);
	void OnViewOriginalData(wxCommandEvent& event);
	// ScatterPlot specific callbacks
	void OnViewRegressionSelectedExcluded(wxCommandEvent& event);
	void OnViewRegressionSelected(wxCommandEvent& event);
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

	void DisplayRegression(const wxString dump);

	void UpdateToolbarAndMenus();
	void SetMenusToDefault();

	static Project* project_p;
	static Project* GetProject() { return projectOpen ? project_p : 0; }
		
	// Getter/Setter methods
	static bool IsProjectOpen();
	static void SetProjectOpen(bool open);
	static MyFrame* theFrame;
	
	static std::list<wxToolBar*> toolbar_list;
private:
	static bool projectOpen;
	DECLARE_EVENT_TABLE()
};

#endif

