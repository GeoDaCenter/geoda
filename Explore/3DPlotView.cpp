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

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/dcbuffer.h>
#include <wx/menu.h>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../DialogTools/3DControlPan.h"
#include "../FramesManager.h"
#include "Geom3D.h"
#include "../GdaConst.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "3DPlotView.h"

BEGIN_EVENT_TABLE(C3DPlotCanvas, wxGLCanvas)
    EVT_SIZE(C3DPlotCanvas::OnSize)
    EVT_PAINT(C3DPlotCanvas::OnPaint)
    EVT_ERASE_BACKGROUND(C3DPlotCanvas::OnEraseBackground)
    EVT_MOUSE_EVENTS(C3DPlotCanvas::OnMouse)
END_EVENT_TABLE()

C3DPlotCanvas::C3DPlotCanvas(Project* project_s, C3DPlotFrame* t_frame, const wxGLAttributes& dispAttrs,
							 HLStateInt* highlight_state_s,
							 const std::vector<GdaVarTools::VarInfo>& v_info,
							 const std::vector<int>& col_ids,
							 wxWindow *parent,
							 wxWindowID id, const wxPoint& pos,
							 const wxSize& size, long style)
: wxGLCanvas(parent, dispAttrs, id, pos, size, style), ball(0),
project(project_s),
table_int(project_s->GetTableInt()),
num_obs(project_s->GetTableInt()->GetNumberRows()),
num_vars(v_info.size()),
num_time_vals(1),
highlight_state(highlight_state_s),
var_info(v_info),
data(v_info.size()),
data_undef(v_info.size()),
scaled_d(v_info.size()),
c3d_plot_frame(t_frame)
{
    wxLogMessage("Open C3DPlotCanvas.");
    
	m_context = new wxGLContext(this);
	selectable_fill_color = GdaConst::three_d_plot_default_point_colour;
	highlight_color = GdaConst::three_d_plot_default_highlight_colour;
	canvas_background_color=GdaConst::three_d_plot_default_background_colour;
	
	data_stats.resize(var_info.size());
	var_min.resize(var_info.size());
	var_max.resize(var_info.size());
	
	for (int v=0; v<num_vars; v++) {
		table_int->GetColData(col_ids[v], data[v]);
		table_int->GetColData(col_ids[v], scaled_d[v]);
		table_int->GetColUndefined(col_ids[v], data_undef[v]);
    }
   
    all_undefs.resize(num_obs, false);
	for (int v=0; v<num_vars; v++) {
		int data_times = data[v].shape()[0];
		for (int t=0; t<data_times; t++) {
			for (int i=0; i<num_obs; i++) {
                all_undefs[i] = all_undefs[i] || data_undef[v][t][i];
            }
        }
        
    }
    
	for (int v=0; v<num_vars; v++) {
		int data_times = data[v].shape()[0];
        
		data_stats[v].resize(data_times);
        
		for (int t=0; t<data_times; t++) {
            std::vector<double> temp_vec;
			for (int i=0; i<num_obs; i++) {
				temp_vec.push_back(data[v][t][i]);
			}
			data_stats[v][t].CalculateFromSample(temp_vec, all_undefs);
		}
	}
	
	c3d_plot_frame->ClearAllGroupDependencies();
	for (int i=0, sz=var_info.size(); i<sz; ++i) {
		c3d_plot_frame->AddGroupDependancy(var_info[i].name);
	}
	
	VarInfoAttributeChange();
	UpdateScaledData();
	
	xs = 0.1;
	xp = 0.0;
    ys = 0.1;
	yp = 0.0;
	zs = 0.1;
	zp = 0.0;
	
	m_bLButton = false;
	m_bRButton = false;
	m_brush = false;

	bb_min[0] = -1;
	bb_min[1] = -1;
	bb_min[2] = -1;
	bb_max[0] = 1;
	bb_max[1] = 1;
	bb_max[2] = 1;
	Vec3f b_min(bb_min);
	Vec3f b_max(bb_max);
	Vec3f ctr((bb_max[0]+bb_min[0])/2.0f, (bb_max[1]+bb_min[1])/2.0f,
			  (bb_max[2]+bb_min[2])/2.0f);
	
	float radius = norm(b_max - ctr);
	ball = new Arcball();
	ball->bounding_sphere(ctr, radius);

	m_x = false;
	m_y = false;
	m_z = false;
	m_d = true;
	b_select = false;
	isInit = false;
	
	highlight_state->registerObserver(this);
}

C3DPlotCanvas::~C3DPlotCanvas()
{
	if (ball) delete ball; ball = 0;	
	highlight_state->removeObserver(this);
    delete m_context;
    wxLogMessage("Close C3DPlotCanvas.");
}

void C3DPlotCanvas::AddTimeVariantOptionsToMenu(wxMenu* menu)
{
	if (!is_any_time_variant) return;
	wxMenu* menu1 = new wxMenu(wxEmptyString);
	{
		for (int i=0, iend=var_info.size(); i<iend; i++) {
			if (!var_info[i].is_time_variant) continue;
			wxString s;
			s << "Synchronize " << var_info[i].name << " with Time Control";
			wxMenuItem* mi =
				menu1->AppendCheckItem(GdaConst::ID_TIME_SYNC_VAR1+i, s, s);
			mi->Check(var_info[i].sync_with_global_time);
		}
	}
	
    menu->AppendSeparator();
	menu->Append(wxID_ANY, "Time Variable Options", menu1, "Time Variable Options");
}

void C3DPlotCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.

	for (int i=0, iend=var_info.size(); i<iend; i++) {
		if (var_info[i].is_time_variant) {
			GeneralWxUtils::CheckMenuItem(menu,
										  GdaConst::ID_TIME_SYNC_VAR1+i,
										  var_info[i].sync_with_global_time);
		}
	}
}


void C3DPlotCanvas::OnPaint( wxPaintEvent& event )
{
	if(!IsShown()) return;

    /* must always be here */
    wxPaintDC dc(this);

	wxGLCanvas::SetCurrent(*m_context);
    

    /* initialize OpenGL */
    if (isInit == false) {
        InitGL();
        isInit = true;
    }

	begin_redraw();
    RenderScene();
	end_redraw();

	if (bSelect) {
		double world11[3],world12[3], world22[3], world21[3];
		int pixel11[2], pixel12[2], pixel22[2], pixel21[2];

		int small_x=(select_start.x<select_end.x)? select_start.x:select_end.x;
		int large_x=(select_start.x>select_end.x)? select_start.x:select_end.x;
		int small_y=(select_start.y>select_end.y)? select_start.y:select_end.y;
		int large_y=(select_start.y<select_end.y)? select_start.y:select_end.y;

		pixel11[0] = small_x;
		pixel12[0] = small_x;
		pixel21[0] = large_x;
		pixel22[0] = large_x;

		pixel11[1] = small_y;
		pixel21[1] = small_y;
		pixel12[1] = large_y;
		pixel22[1] = large_y;

		unproject_pixel(pixel11, world11, 0.0);	
		unproject_pixel(pixel12, world12, 0.0);
		unproject_pixel(pixel22, world22, 0.0);
		unproject_pixel(pixel21, world21, 0.0);

		glLineWidth(2.0);
		glColor3f(0.75,0.75,0.75);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glBegin(GL_LINES);
			glVertex3dv(world11);
			glVertex3dv(world12);
			glVertex3dv(world12);
			glVertex3dv(world22);
			glVertex3dv(world22);
			glVertex3dv(world21);
			glVertex3dv(world21);
			glVertex3dv(world11);
		glEnd();
		glEnable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);
	}

    /* flush */
    glFlush();

    /* swap */
    SwapBuffers();
}

void C3DPlotCanvas::OnSize(wxSizeEvent& event)
{
    // this is also necessary to update the context on some platforms
    //wxGLCanvas::OnSize(event);
    
    // set GL viewport (not called by wxGLCanvas::OnSize on all platforms...)
    int w, h;
    GetClientSize(&w, &h);

    //if (GetContext()) {
        glViewport(0, 0, (GLint) w, (GLint) h);
    //}
}

void C3DPlotCanvas::OnEraseBackground(wxEraseEvent& event)
{
    // Do nothing, to avoid flashing on MSW
}

void C3DPlotCanvas::OnMouse( wxMouseEvent& event )
{
	wxPoint pt(event.GetPosition());
	wxClientDC dc(this);
	PrepareDC(dc);

	wxPoint point(event.GetLogicalPosition(dc));

	if (event.RightDown()) {
		m_bRButton = true;
		int where[2];
		where[0] = point.x;
		where[1] = point.y;
		last[0] = point.x;
		last[1] = point.y;
		ball->mouse_down(where,3);
	}
	
	if (event.RightUp()) {
		m_bRButton = false;
		int where[2];
		where[0] = point.x;
		where[1] = point.y;
		ball->mouse_up(where,3);
	}
	
	if (event.LeftDown()) {
		if ((event.CmdDown()) && this->m_d && this->b_select) {
			m_brush = true;
			last[0] = point.x;
			last[1] = point.y;
		} else {
			m_bLButton = true;
			int where[2];
			where[0] = point.x;
			where[1] = point.y;
			last[0] = point.x;
			last[1] = point.y;
			ball->mouse_down(where,1);
		}
	}
	
	if (event.LeftUp()) {
		if (bSelect && this->m_d ) {
			bSelect = false;
			SelectByRect();
		} else if (m_brush) {
			m_brush = false;
		} else {
			m_bLButton = false;
			int where[2];
			where[0] = point.x;
			where[1] = point.y;
			ball->mouse_up(where,1);
		}
	}
	
	if (event.Dragging()) {
		int where[2];
		where[0] = point.x;
		where[1] = point.y;
		if (m_brush) {
			float vp[4];
			glGetFloatv(GL_VIEWPORT, vp);
			float W=vp[2], H=vp[3];
			float diam = 2*(ball->radius);
			
			ball->apply_transform();
			
			int pix1[2], pix2[2], pix3[2];
			pix1[0] = (int)(W/2);
			pix1[1] = (int)(H/2);
			pix2[0] = (int)(W/2-1);
			pix2[1] = (int)(H/2);
			pix3[0] = (int)(W/2);
			pix3[1] = (int)(H/2-1);
			double world1[3], world2[3],world3[3];
			unproject_pixel(pix1, world1, 0.0);
			unproject_pixel(pix2, world2, 0.0);
			unproject_pixel(pix3, world3, 0.0);

			ball->unapply_transform();

			Vec3f w1(world1);
			Vec3f w2(world2);
			Vec3f w3(world3);

			Vec3f screen_x = w1-w2;
			unitize(screen_x);
			Vec3f screen_y = w3-w1;
			unitize(screen_y);

			Vec3f XX(1,0,0);
			Vec3f YY(0,1,0);
			Vec3f ZZ(0,0,1);

			xp += diam * (where[0] - last[0]) / W *(XX*screen_x);
			yp += diam * (where[0] - last[0]) / W *(YY*screen_x);
			zp += diam * (where[0] - last[0]) / W *(ZZ*screen_x); 

			xp += diam * (last[1] - where[1]) / H *(XX*screen_y);
			yp += diam * (last[1] - where[1]) / H *(YY*screen_y);
			zp += diam * (last[1] - where[1]) / H *(ZZ*screen_y);

			if (xp < -1.0) xp = -1.0;
			if (xp > 1.0) xp = 1.0;
			if (yp < -1.0) yp = -1.0;
			if (yp > 1.0) yp = 1.0;
			if (zp < -1.0) zp = -1.0;
			if (zp > 1.0) zp = 1.0;

			last[0] = where[0];
			last[1] = where[1];

			c3d_plot_frame->control->m_xp->SetValue((int)((xp+1)*10000));
			c3d_plot_frame->control->m_yp->SetValue((int)((yp+1)*10000));
			c3d_plot_frame->control->m_zp->SetValue((int)((zp+1)*10000));

			this->UpdateSelect();
		} else {
			bSelect = false;
			if (m_bLButton & m_bRButton) {
				ball->mouse_drag(where,last,2);
				last[0] = where[0];
				last[1] = where[1];
			} else if (m_bLButton) {
				ball->mouse_drag(where,last,1);
				last[0] = where[0];
				last[1] = where[1];
			} else if (m_bRButton) {
				ball->mouse_drag(where,last,3);
				last[0] = where[0];
				last[1] = where[1];
			}
		}
	}
	
	Refresh();
}

void C3DPlotCanvas::UpdateSelect()
{
	if (!b_select) return;
	
	int hl_size = highlight_state->GetHighlightSize();
	std::vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed = false;
	
	double minx = xp - xs;
	double maxx = xp + xs;
	double miny = yp - ys;
	double maxy = yp + ys;
	double minz = zp - zs;
	double maxz = zp + zs;
	
	int xt = var_info[0].time;
	int yt = var_info[1].time;
	int zt = var_info[2].time;
	
	for (int i=0; i<num_obs; i++) {
		bool contains = ((scaled_d[0][xt][i] >= minx) &&
						 (scaled_d[0][xt][i] <= maxx) &&
						 (scaled_d[1][yt][i] >= miny) &&
						 (scaled_d[1][yt][i] <= maxy) &&
						 (scaled_d[2][zt][i] >= minz) &&
						 (scaled_d[2][zt][i] <= maxz));
		if (contains) {
            if (!hs[i]) {
                hs[i] = true;
                selection_changed = true;
            }
		} else {
            if (hs[i]) {
                hs[i] = false;
                selection_changed = true;
            }
		}
	}

    if (selection_changed) {
		highlight_state->SetEventType(HLStateInt::delta);
		highlight_state->notifyObservers();
    }
}

void C3DPlotCanvas::SelectByRect()
{
	int hl_size = highlight_state->GetHighlightSize();
	std::vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed = false;
	
	double world11[3], world12[3], world22[3], world21[3];
	double world113[3], world123[3], world223[3], world213[3];
	
	int pixel11[2], pixel12[2], pixel22[2], pixel21[2];
	
    int small_x = (select_start.x < select_end.x)? select_start.x:select_end.x;
    int large_x = (select_start.x > select_end.x)? select_start.x:select_end.x;
    int small_y = (select_start.y < select_end.y)? select_start.y:select_end.y;
    int large_y = (select_start.y > select_end.y)? select_start.y:select_end.y;
	
	pixel11[0] = small_x;	pixel12[0] = small_x; 
	pixel21[0] = large_x;   pixel22[0] = large_x;
	pixel11[1] = small_y;   pixel21[1] = small_y;
	pixel12[1] = large_y;	pixel22[1] = large_y;
	
	ball->apply_transform();
	
	unproject_pixel(pixel11, world11, 0.0);	
	unproject_pixel(pixel12, world12, 0.0);
	unproject_pixel(pixel22, world22, 0.0);
    unproject_pixel(pixel21, world21, 0.0);
	
	unproject_pixel(pixel11, world113, 1.0);	
	unproject_pixel(pixel12, world123, 1.0);
	unproject_pixel(pixel22, world223, 1.0);
    unproject_pixel(pixel21, world213, 1.0);
	
	ball->unapply_transform();
	
	SPlane* plane;
	int i;
	
	bool *inside = new bool[num_obs*4];
	for(i=0; i<num_obs*4; i++) inside[i] = false;
	double *world1, *world2, *world3, *world4;
	for (int k=0; k<4; k++) {
		switch(k)
		{
			case 0: 
				world1 = world11;
				world2 = world12;
				world3 = world113;
				world4 = world123;
				break;
			case 1:
				world1 = world12;
				world2 = world22;
				world3 = world123;
				world4 = world223;
				break;
			case 2:
				world1 = world22;
				world2 = world21;
				world3 = world223;
				world4 = world213;
				break;
			case 3:
				world1 = world21;
				world2 = world11;
				world3 = world213;
				world4 = world113;
				break;
			default:
				break;
		}
		
		plane = new SPlane(world1, world2, world3);
		
		Vec3f a1(world1[0], world1[1], world1[2]);
		Vec3f a2(world2[0], world2[1], world2[2]);
		Vec3f a3(world3[0], world3[1], world3[2]);
		Vec3f a4(world4[0], world4[1], world4[2]);
		Vec3f l1 = a3 - a1;
		Vec3f l2 = a4 - a2;
		
		int xt = var_info[0].time;
		int yt = var_info[1].time;
		int zt = var_info[2].time;
		
		for (i=0; i<num_obs; i++) {
			Vec3f cor(scaled_d[0][xt][i], scaled_d[1][yt][i],
					  scaled_d[2][zt][i]);
			if (plane->isPositive(cor)) inside[k*num_obs+i] = true;
		}
        delete plane;
    }
	
	delete [] inside;
	
	for (i=0; i<num_obs; i++) {
		bool contains = (inside[i] && inside[num_obs+i] && inside[2*num_obs+i]
						 && inside[3*num_obs+i]);
		if (contains) {
            if (!hs[i]) {
                hs[i] = true;
                selection_changed = true;
            }
		} else {
            if (hs[i])  {
                hs[i] = false;
                selection_changed = true;
            }
		}
	}
    if (selection_changed) {
		highlight_state->SetEventType(HLStateInt::delta);
		highlight_state->notifyObservers();
    }
}

void C3DPlotCanvas::InitGL(void)
{
    //SetCurrent();

    //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearColor(((GLfloat) canvas_background_color.Red())/((GLfloat) 255.0),
				 ((GLfloat) canvas_background_color.Green())/((GLfloat) 255.0),
				 ((GLfloat) canvas_background_color.Blue())/((GLfloat) 255.0),
				 0.0f);
	
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);

    float ambient_light[4] = {1.0, 1.0, 1.0, 1.0};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (float *)ambient_light);

    const float light0_pos[4] = {0.0f, 0.5f, 1.0f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
    glEnable(GL_LIGHT0);

    float rgb[4] = {0.912f, 0.717f, 0.505f, 1.0f};
    float r_amb[4]; 
    float r_diff[4];
    float r_spec[4];

	for (int i=0; i<4; i++) {
		r_amb[i] = rgb[i]*0.1;
		r_diff[i] = rgb[i]*1.0;
		r_spec[i] = rgb[i]*0.3f;
	}

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, r_amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, r_diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, r_spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0);

    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
}

void C3DPlotCanvas::SetSelectableFillColor(wxColour color)
{
	selectable_fill_color = color;
	Refresh();
}

void C3DPlotCanvas::SetHighlightColor(wxColour color)
{
	highlight_color = color;
	Refresh();
}

void C3DPlotCanvas::SetCanvasBackgroundColor(wxColour color)
{
	canvas_background_color = color;
	InitGL();
	Refresh();
}

void C3DPlotCanvas::RenderScene()
{
	std::vector<bool>& hs = highlight_state->GetHighlight();
	
	int xt = var_info[0].time;
	int yt = var_info[1].time;
	int zt = var_info[2].time;
	
	GLUquadric* myQuad = 0;
	myQuad = gluNewQuadric();
	if (m_d) {
		//glColor3f(1.0, 1.0, 1.0);
		glColor3f(((GLfloat) selectable_fill_color.Red())/((GLfloat) 255.0),
				  ((GLfloat) selectable_fill_color.Green())/((GLfloat) 255.0),
				  ((GLfloat) selectable_fill_color.Blue())/((GLfloat) 255.0));
		for (int i=0; i<num_obs; i++) {
			if (all_undefs[i]) continue;
			if (hs[i]) continue;
			glPushMatrix();
			glTranslatef(scaled_d[0][xt][i], scaled_d[1][yt][i],
						 scaled_d[2][zt][i]);
			gluSphere(myQuad, 0.03, 5, 5); 	
			glPopMatrix();
		}
		//glColor3f(1.0, 1.0, 0.0);
		glColor3f(((GLfloat) highlight_color.Red())/((GLfloat) 255.0),
				  ((GLfloat) highlight_color.Green())/((GLfloat) 255.0),
				  ((GLfloat) highlight_color.Blue())/((GLfloat) 255.0));
		for (int i=0; i<num_obs; i++) {
			if (all_undefs[i]) continue;
			if (!hs[i]) continue;
			glPushMatrix();
			glTranslatef(scaled_d[0][xt][i], scaled_d[1][yt][i],
						 scaled_d[2][zt][i]);
			gluSphere(myQuad, 0.03, 5, 5); 	
			glPopMatrix();
		}
		
	}

	glDisable(GL_LIGHTING);
	if (m_x) {
		//glColor3f(0.75, 0.75, 0.75);
		glColor3f(((GLfloat) selectable_fill_color.Red())/((GLfloat) 255.0),
				  ((GLfloat) selectable_fill_color.Green())/((GLfloat) 255.0),
				  ((GLfloat) selectable_fill_color.Blue())/((GLfloat) 255.0));
		for(int i=0; i<num_obs; i++) {
			if (all_undefs[i]) continue;
			if (hs[i]) continue;
			glPushMatrix();
			glTranslatef(-1, scaled_d[1][yt][i], scaled_d[2][zt][i]);
			glRotatef(90, 0.0, 1.0, 0.0);	
			gluDisk(myQuad, 0, 0.02, 5, 3);
			glPopMatrix();
		}
		//glColor3f(1.0, 1.0, 0.0);
		glColor3f(((GLfloat) highlight_color.Red())/((GLfloat) 255.0),
				  ((GLfloat) highlight_color.Green())/((GLfloat) 255.0),
				  ((GLfloat) highlight_color.Blue())/((GLfloat) 255.0));
		for (int i=0; i<num_obs; i++) {
			if (all_undefs[i]) continue;
			if (!hs[i]) continue;
			glPushMatrix();
			glTranslatef(-1, scaled_d[1][yt][i], scaled_d[2][zt][i]);
			glRotatef(90, 0.0, 1.0, 0.0);
			gluDisk(myQuad, 0, 0.02, 5, 3);
			glPopMatrix();
		}
	}
	
	if (m_y) {
		//glColor3f(0.75, 0.75, 0.75);
		glColor3f(((GLfloat) selectable_fill_color.Red())/((GLfloat) 255.0),
				  ((GLfloat) selectable_fill_color.Green())/((GLfloat) 255.0),
				  ((GLfloat) selectable_fill_color.Blue())/((GLfloat) 255.0));
		for (int i=0; i<num_obs; i++) {
			if (all_undefs[i]) continue;
			if (hs[i]) continue;
			glPushMatrix();
			glTranslatef(scaled_d[0][xt][i], -1, scaled_d[2][zt][i]);
			glRotatef(90, 1.0, 0.0, 0.0); 
			gluDisk(myQuad, 0, 0.02, 5, 3);
			glPopMatrix();
		}
		//glColor3f(1.0, 1.0, 0.0);
		glColor3f(((GLfloat) highlight_color.Red())/((GLfloat) 255.0),
				  ((GLfloat) highlight_color.Green())/((GLfloat) 255.0),
				  ((GLfloat) highlight_color.Blue())/((GLfloat) 255.0));
		for (int i=0; i<num_obs; i++) {
			if (all_undefs[i]) continue;
			if (!hs[i]) continue;
			glPushMatrix();
			glTranslatef(scaled_d[0][xt][i], -1, scaled_d[2][zt][i]);
			glRotatef(90, 1.0, 0.0, 0.0); 
			gluDisk(myQuad, 0, 0.02, 5, 3);
			glPopMatrix();
		}
	}

	if (m_z) {
		//glColor3f(0.75, 0.75, 0.75);
		glColor3f(((GLfloat) selectable_fill_color.Red())/((GLfloat) 255.0),
				  ((GLfloat) selectable_fill_color.Green())/((GLfloat) 255.0),
				  ((GLfloat) selectable_fill_color.Blue())/((GLfloat) 255.0));
		for (int i=0; i<num_obs; i++) {
			if (all_undefs[i]) continue;
			if (hs[i]) continue;
			glPushMatrix();
			glTranslatef(scaled_d[0][xt][i], scaled_d[1][yt][i], -1);
			gluDisk(myQuad, 0, 0.02, 5, 3);
			glPopMatrix();
		}
		//glColor3f(1.0, 1.0, 0.0);
		glColor3f(((GLfloat) highlight_color.Red())/((GLfloat) 255.0),
				  ((GLfloat) highlight_color.Green())/((GLfloat) 255.0),
				  ((GLfloat) highlight_color.Blue())/((GLfloat) 255.0));
		for(int i=0; i<num_obs; i++) {
			if (all_undefs[i]) continue;
			if (!hs[i]) continue;
			glPushMatrix();
			glTranslatef(scaled_d[0][xt][i], scaled_d[1][yt][i], -1);
			gluDisk(myQuad, 0, 0.02, 5, 3);
			glPopMatrix();
		}
	}
	glEnable(GL_LIGHTING);

	if (b_select) {
		double minx = xp - xs;
		double maxx = xp + xs;
		double miny = yp - ys;
		double maxy = yp + ys;
		double minz = zp - zs;
		double maxz = zp + zs;
		glDisable(GL_LIGHTING);
		glLineWidth(2.0);
		glColor3f(1.0, 0.0, 0.0);
		glBegin(GL_LINES);
			glVertex3f(maxx,maxy,maxz);
			glVertex3f(maxx,miny,maxz);
			glVertex3f(maxx,miny,maxz);
			glVertex3f(minx,miny,maxz);
			glVertex3f(minx,miny,maxz);
			glVertex3f(minx,maxy,maxz);
			glVertex3f(minx,maxy,maxz);
			glVertex3f(maxx,maxy,maxz);
			glVertex3f(maxx,maxy,minz);
			glVertex3f(maxx,miny,minz);
			glVertex3f(maxx,miny,minz);
			glVertex3f(minx,miny,minz);
			glVertex3f(minx,miny,minz);
			glVertex3f(minx,maxy,minz);
			glVertex3f(minx,maxy,minz);
			glVertex3f(maxx,maxy,minz);
			glVertex3f(maxx,maxy,maxz);
			glVertex3f(maxx,maxy,minz);
			glVertex3f(maxx,miny,maxz);
			glVertex3f(maxx,miny,minz);
			glVertex3f(minx,miny,maxz);
			glVertex3f(minx,miny,minz);
			glVertex3f(minx,maxy,maxz);
			glVertex3f(minx,maxy,minz);			
		glEnd();
		glEnable(GL_LIGHTING);
	}

	glLineWidth(3.0);

	glColor3f((GLfloat) 0.95,(GLfloat) 0.0,(GLfloat) 0.95);
	glDisable(GL_LIGHTING);
	glBegin(GL_LINES);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) 1,(GLfloat) -1,(GLfloat) -1);

		glVertex3f((GLfloat) 1.2,(GLfloat) -0.9,(GLfloat) -1.0);
		glVertex3f((GLfloat) 1.1,(GLfloat) -0.8,(GLfloat) -1.0);
		glVertex3f((GLfloat) 1.2,(GLfloat) -0.8,(GLfloat) -1.0);
		glVertex3f((GLfloat) 1.1,(GLfloat) -0.9,(GLfloat) -1.0);
    glEnd();

	glColor3f((GLfloat) 0.0,(GLfloat) 0.95,(GLfloat) 0.0);
	glBegin(GL_LINES);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) -1,(GLfloat) 1,(GLfloat) -1);

		glVertex3f((GLfloat) -0.85,(GLfloat) 1.15,(GLfloat) -1.0);
		glVertex3f((GLfloat) -0.9,(GLfloat) 1.2,(GLfloat) -1.0);
		glVertex3f((GLfloat) -0.85,(GLfloat) 1.15,(GLfloat) -1.0);
		glVertex3f((GLfloat) -0.8,(GLfloat) 1.2,(GLfloat) -1.0);
		glVertex3f((GLfloat) -0.85,(GLfloat) 1.15,(GLfloat) -1.0);
		glVertex3f((GLfloat) -0.85,(GLfloat) 1.1,(GLfloat) -1.0);		
	glEnd();

	glColor3f((GLfloat) 0.0,(GLfloat) 0.95,(GLfloat) 0.95);
	glBegin(GL_LINES);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) 1);

		glVertex3f((GLfloat) -1.0,(GLfloat) -0.8,(GLfloat) 1.2);
		glVertex3f((GLfloat) -1.0,(GLfloat) -0.8,(GLfloat) 1.1);
		glVertex3f((GLfloat) -1.0,(GLfloat) -0.8,(GLfloat) 1.1);
		glVertex3f((GLfloat) -1.0,(GLfloat) -0.9,(GLfloat) 1.2);
		glVertex3f((GLfloat) -1.0,(GLfloat) -0.9,(GLfloat) 1.2);
		glVertex3f((GLfloat) -1.0,(GLfloat) -0.9,(GLfloat) 1.1);		
	glEnd();

	glLineWidth(1.0);
	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_LINES);
		glVertex3f((GLfloat) 1,(GLfloat) 1,(GLfloat) 1);
		glVertex3f((GLfloat) 1,(GLfloat) -1,(GLfloat) 1);
		glVertex3f((GLfloat) 1,(GLfloat) -1,(GLfloat) 1);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) 1);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) 1);
		glVertex3f((GLfloat) -1,(GLfloat) 1,(GLfloat) 1);
		glVertex3f((GLfloat) -1,(GLfloat) 1,(GLfloat) 1);
		glVertex3f((GLfloat) 1,(GLfloat) 1,(GLfloat) 1);
		glVertex3f((GLfloat) 1,(GLfloat) 1,(GLfloat) -1);
		glVertex3f((GLfloat) 1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) 1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) -1,(GLfloat) 1,(GLfloat) -1);
		glVertex3f((GLfloat) -1,(GLfloat) 1,(GLfloat) -1);
		glVertex3f((GLfloat) 1,(GLfloat) 1,(GLfloat) -1);
		glVertex3f((GLfloat) 1,(GLfloat) 1,(GLfloat) 1);
		glVertex3f((GLfloat) 1,(GLfloat) 1,(GLfloat) -1);
		glVertex3f((GLfloat) 1,(GLfloat) -1,(GLfloat) 1);
		glVertex3f((GLfloat) 1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) 1);
		glVertex3f((GLfloat) -1,(GLfloat) -1,(GLfloat) -1);
		glVertex3f((GLfloat) -1,(GLfloat) 1,(GLfloat) 1);
		glVertex3f((GLfloat) -1,(GLfloat) 1,(GLfloat) -1);
	glEnd();

	glEnable(GL_LIGHTING);
 
	gluDeleteQuadric(myQuad);
}

void C3DPlotCanvas::apply_camera()
{
    int w, h;
    GetClientSize(&w, &h);

    float aspect = (float)w / (float)h;

    Vec3 d_min(bb_min[0], bb_min[1], bb_min[2]);
    Vec3 d_max(bb_max[0], bb_max[1], bb_max[2]);

    Vec3 up(0, 1, 0);
    double fovy = 60.0;

    Vec3 at = (d_max+d_min)/2.0;

	double radius = sqrt((d_max[0]-at[0])*(d_max[0]-at[0])
						 +(d_max[1]-at[1])*(d_max[1]-at[1])
						 +(d_max[2]-at[2])*(d_max[2]-at[2]));
    double d = 3*radius / tan(fovy * M_PI/180.0);

    Vec3 from = at;

    from[2] += d;

    double znear = d/20;
    double zfar = 10*d;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluPerspective(fovy, aspect, znear, zfar);
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
    gluLookAt(from[0], from[1], from[2],
			  at[0], at[1], at[2],
			  up[0], up[1], up[2]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
}

void C3DPlotCanvas::end_redraw()
{
   ball->unapply_transform();
}

void C3DPlotCanvas::begin_redraw()
{
    apply_camera();
    ball->apply_transform();
}

wxString C3DPlotCanvas::GetCanvasTitle()
{
	wxString s("3D Plot: ");
	s << GetNameWithTime(0) << ", ";
	s << GetNameWithTime(1) << ", ";
	s << GetNameWithTime(2);
	return s;
}

wxString C3DPlotCanvas::GetNameWithTime(int var)
{
	if (var < 0 || var >= var_info.size()) return wxEmptyString;
	wxString s(var_info[var].name);
	if (var_info[var].is_time_variant) {
		s << " (" << project->GetTableInt()->GetTimeString(var_info[var].time);
		s << ")";
	}
	return s;
}

void C3DPlotCanvas::TimeChange()
{
	if (!is_any_sync_with_global_time) return;
	
	int cts = project->GetTimeState()->GetCurrTime();
	int ref_time = var_info[ref_var_index].time;
	int ref_time_min = var_info[ref_var_index].time_min;
	int ref_time_max = var_info[ref_var_index].time_max; 
	
	if ((cts == ref_time) ||
		(cts > ref_time_max && ref_time == ref_time_max) ||
		(cts < ref_time_min && ref_time == ref_time_min)) return;
	if (cts > ref_time_max) {
		ref_time = ref_time_max;
	} else if (cts < ref_time_min) {
		ref_time = ref_time_min;
	} else {
		ref_time = cts;
	}
	for (int v=0; v<num_vars; v++) {
		if (var_info[v].sync_with_global_time) {
			var_info[v].time = ref_time + var_info[v].ref_time_offset;
		}
	}
	SetCurrentTmStep(ref_time - ref_time_min);
	
	//invalidateBms();
	//PopulateCanvas();
	Refresh();
}

/** Update Secondary Attributes based on Primary Attributes.
 Update num_time_vals and ref_var_index based on Secondary Attributes. */
void C3DPlotCanvas::VarInfoAttributeChange()
{
	GdaVarTools::UpdateVarInfoSecondaryAttribs(var_info);
	
	is_any_time_variant = false;
	is_any_sync_with_global_time = false;
	for (int i=0; i<var_info.size(); i++) {
		if (var_info[i].is_time_variant) is_any_time_variant = true;
		if (var_info[i].sync_with_global_time) {
			is_any_sync_with_global_time = true;
		}
	}
	c3d_plot_frame->SetDependsOnNonSimpleGroups(is_any_time_variant);
	ref_var_index = -1;
	num_time_vals = 1;
	for (int i=0; i<var_info.size() && ref_var_index == -1; i++) {
		if (var_info[i].is_ref_variable) ref_var_index = i;
	}
	if (ref_var_index != -1) {
		num_time_vals = (var_info[ref_var_index].time_max -
						 var_info[ref_var_index].time_min) + 1;
	}
	
	//GdaVarTools::PrintVarInfoVector(var_info);
}

void C3DPlotCanvas::UpdateScaledData()
{
	for (int v=0; v<num_vars; v++) {
        
		int t_min = var_info[v].time_min;
		int t_max = var_info[v].time_max;
        
		double min = data_stats[v][t_min].min;
		double max = min;
        
		for (int t=t_min; t<=t_max; t++) {
			if (data_stats[v][t].min < min)
                min = data_stats[v][t].min;
			if (data_stats[v][t].max > max)
                max = data_stats[v][t].max;
		}
        
		double ctr = (min+max)/2.0;
		double scale = (max==min) ? 1.0 : 2.0/(max-min);
		
		for (int t=t_min; t<=t_max; t++) {
			for (int i=0; i<num_obs; i++) {
				scaled_d[v][t][i] = (data[v][t][i]-ctr)*scale;
			}
		}
	}
}

void C3DPlotCanvas::TimeSyncVariableToggle(int var_index)
{
	var_info[var_index].sync_with_global_time =
		!var_info[var_index].sync_with_global_time;
	VarInfoAttributeChange();
	Refresh();
}


/** Impelmentation of HighlightStateObserver interface function.  This
 is called by HighlightState when it notifies all observers
 that its state has changed. */
void C3DPlotCanvas::update(HLStateInt* o)
{
	HLStateInt::EventType type = highlight_state->GetEventType();
	if (type == HLStateInt::delta) {
			
		Refresh();
	} else {
		// type == HLStateInt::unhighlight_all
		// type == HLStateInt::invert
			
		Refresh();
	}
}


BEGIN_EVENT_TABLE(C3DPlotFrame, wxFrame)
	EVT_ACTIVATE(C3DPlotFrame::OnActivate)
    EVT_CLOSE(C3DPlotFrame::OnClose)
	EVT_MENU(XRCID("wxID_CLOSE"), C3DPlotFrame::OnMenuClose)
	EVT_CHAR_HOOK(TemplateFrame::OnKeyEvent)
END_EVENT_TABLE()


C3DPlotFrame::C3DPlotFrame(wxFrame *parent, Project* project,
						   const std::vector<GdaVarTools::VarInfo>& var_info,
						   const std::vector<int>& col_ids,
						   const wxString& title, const wxPoint& pos,
						   const wxSize& size, const long style)
	: TemplateFrame(parent, project, title, pos, size, style)
{
	m_splitter = new wxSplitterWindow(this);
    
	wxGLAttributes glAttributes; 
	//glAttributes.Defaults().RGBA().DoubleBuffer().Depth(16).Stencil(8).SampleBuffers(1).Samplers(4).EndList(); 
	glAttributes.PlatformDefaults().RGBA().DoubleBuffer().Depth(24).EndList();
	

	canvas = new C3DPlotCanvas(project, this, glAttributes,
							   project->GetHighlightState(),
							   var_info, col_ids,
							   m_splitter);
	
	control = new C3DControlPan(m_splitter, -1, wxDefaultPosition,
								wxDefaultSize, wxCAPTION|wxDEFAULT_DIALOG_STYLE);
	control->template_frame = this;
	m_splitter->SplitVertically(control, canvas, 70);
	UpdateTitle();

	Show(true);
}

C3DPlotFrame::~C3DPlotFrame()
{
}

void C3DPlotFrame::OnActivate(wxActivateEvent& event)
{
	if (event.GetActive()) {
		RegisterAsActive("C3DPlotFrame", GetTitle());
	}
}

void C3DPlotFrame::OnClose(wxCloseEvent& event)
{
	DeregisterAsActive();
	Destroy();
}

void C3DPlotFrame::OnMenuClose(wxCommandEvent& event)
{
	Close();
}

void C3DPlotFrame::MapMenus()
{
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Map Options Menus
	wxMenu* optMenu = wxXmlResource::Get()->
		LoadMenu("ID_3D_PLOT_VIEW_MENU_OPTIONS");
	canvas->AddTimeVariantOptionsToMenu(optMenu);
	canvas->SetCheckMarks(optMenu);
	GeneralWxUtils::ReplaceMenu(mb, "Options", optMenu);	
	UpdateOptionMenuItems();
}

void C3DPlotFrame::UpdateOptionMenuItems()
{
	TemplateFrame::UpdateOptionMenuItems(); // set common items first
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	int menu = mb->FindMenu("Options");
    if (menu == wxNOT_FOUND) {
	} else {
		canvas->SetCheckMarks(mb->GetMenu(menu));
	}
}

void C3DPlotFrame::OnHighlightColor(wxCommandEvent& event)
{
	wxColour new_color;
	if ( GetColorFromUser(this,
						  canvas->highlight_color,
						  new_color,
						  "Highlight Color") ) {
		canvas->SetHighlightColor(new_color);
	}
}

void C3DPlotFrame::OnCanvasBackgroundColor(wxCommandEvent& event)
{
	wxColour new_color;
	if ( GetColorFromUser(this,
						  canvas->canvas_background_color,
						  new_color,
						  "Background Color") ) {
		canvas->SetCanvasBackgroundColor(new_color);
	}
}

void C3DPlotFrame::OnSelectableFillColor(wxCommandEvent& event)
{
	wxColour new_color;
	if ( GetColorFromUser(this,
						  canvas->selectable_fill_color,
						  new_color,
						  "Fill Color") ) {
		canvas->SetSelectableFillColor(new_color);
	}	
}

/** Implementation of TimeStateObserver interface */
void C3DPlotFrame::update(TimeState* o)
{
	canvas->TimeChange();
	UpdateTitle();
}

void C3DPlotFrame::OnTimeSyncVariable(int var_index)
{
	if (!canvas) return;
	canvas->TimeSyncVariableToggle(var_index);
	UpdateOptionMenuItems();
}

void C3DPlotFrame::UpdateTitle()
{
	TemplateFrame::UpdateTitle();
	control->UpdateAxesLabels(canvas->GetNameWithTime(0),
							  canvas->GetNameWithTime(1),
							  canvas->GetNameWithTime(2));
}

