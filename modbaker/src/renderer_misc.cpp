//---------------------------------------------------------------------
// ModBaker - a module editor for Egoboo
//---------------------------------------------------------------------
// Copyright (C) 2009 Tobias Gall
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------
#include "global.h"
#include "renderer.h"
#include "ogl_debug.h"

#include <iostream>
#include <math.h>

//---------------------------------------------------------------------
//-   Constructur: Set the basic camera values
//---------------------------------------------------------------------
c_camera::c_camera()
{
	m_movex  = 0.0f;
	m_movey  = 0.0f;
	m_movez  = 0.0f;

	m_factor = SCROLLFACTOR_SLOW;

	this->reset();
}


void c_camera::reset()
{
	m_pos.x = 0.0f;
	m_pos.y = 0.0f;
	m_pos.z = 0.0f;
	m_pos.z = -3000.0f; // Move 3000 units up

	m_trackpos.x = m_pos.x;
	m_trackpos.y = m_pos.y;
	m_trackpos.z = m_pos.z - 1.0f;
}


void c_camera::move()
{
	if (g_renderer->m_fps > 0)
	{
		m_pos.x -= (m_movex / g_renderer->m_fps) * m_factor;
		m_pos.y -= (m_movey / g_renderer->m_fps) * m_factor;
		m_pos.z -= (m_movez / g_renderer->m_fps) * m_factor;
	}

	this->make_matrix();
}


//---------------------------------------------------------------------
//-   The view matrix (for the camera view)
//---------------------------------------------------------------------
GLmatrix c_camera::make_view_matrix(
	const vect3 from,     // camera location
	const vect3 at,       // camera look-at target
	const vect3 world_up, // world's up, usually 0, 0, 1
	const float roll)     // clockwise roll around viewing direction, in radians
{
	GLmatrix view;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// Move the camera
	glTranslatef(m_pos.x, m_pos.y, m_pos.z);
//	glScalef(-1, 1, 1);

	// Load the current matrix stack to view
	glGetFloatv(GL_MODELVIEW_MATRIX, view.v);

	glPopMatrix();

	return view;
}


//---------------------------------------------------------------------
//-   The projection matrix (for the mesh)
//---------------------------------------------------------------------
GLmatrix c_camera::make_projection_matrix(
	const float near_plane, // distance to near clipping plane
	const float far_plane,  // distance to far clipping plane
	const float fov)        // field of view angle, in radians
{
	GLmatrix proj;
	GLint viewport[4];
	float ratio;

	// use OpenGl to create our projection matrix
	glGetIntegerv(GL_VIEWPORT, viewport);

	ratio = (float)(viewport[2] - viewport[0] ) / (float)(viewport[3] - viewport[1]);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	gluPerspective(fov, ratio, near_plane, far_plane);
	glGetFloatv(GL_PROJECTION_MATRIX, proj.v);

	glPopMatrix();

	return proj;
}


//---------------------------------------------------------------------
//-   Set up the projection (world) matrix
//---------------------------------------------------------------------
void c_camera::_cam_frustum_jitter_fov(
	GLdouble nearval,
	GLdouble farval,
	GLdouble fov,
	GLdouble xoff,
	GLdouble yoff)
{
	GLfloat  scale;
	GLint    viewport[4];
	GLdouble hprime, wprime;
	GLdouble left, right, top, bottom;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glLoadIdentity();

	glGetIntegerv(GL_VIEWPORT, viewport);

	hprime = 2.0f * nearval * tan(fov * 0.5f);
	scale  = hprime / viewport[3];
	wprime = scale * viewport[2];

	xoff *= 0.25;
	yoff *= 0.25;

	left   = -wprime / 2.0f;
	right  =  wprime / 2.0f;
	top    = -hprime / 2.0f;
	bottom =  hprime / 2.0f;

	glFrustum(
		left   - xoff * scale,
		right  - xoff * scale,
		top    - yoff * scale,
		bottom - yoff * scale,
		nearval, farval
	);

	glGetFloatv(GL_PROJECTION_MATRIX, m_projection.v);

	glPopMatrix();
}


//---------------------------------------------------------------------
//-   This function sets cam->m_view to the camera's location and rotation
//---------------------------------------------------------------------
void c_camera::make_matrix()
{
	vect3 worldup;
	float dither_x, dither_y;

	worldup.x = 0;
	worldup.y = 1.0f;
	worldup.z = 0;

	m_view = this->make_view_matrix(m_pos, m_trackpos, worldup, 0);

	dither_x = 2.0f * (float) rand() / (float) RAND_MAX - 1.0f;
	dither_y = 2.0f * (float) rand() / (float) RAND_MAX - 1.0f;

//	this->_cam_frustum_jitter_fov(10.0f, 20000.0f, DEG_TO_RAD*FOV, dither_x, dither_y);
	this->_cam_frustum_jitter_fov(10.0f, 20000.0f, DEG_TO_RAD*FOV, 1.0, 1.0);

	g_frustum.CalculateFrustum(m_projection_big.v, m_view.v);
}


//---------------------------------------------------------------------
//-   Build the renderlist
//---------------------------------------------------------------------
bool c_renderlist::build()
{
	if (g_mesh == NULL)
	{
		cout << "WARNING: No mesh found" << endl;
		return false;
	}

	Uint32 fan;

	// Reset the values
	m_num_totl  = 0;
	m_num_shine = 0;
	m_num_reflc = 0;
	m_num_norm  = 0;
	m_num_watr  = 0;

	for (fan = 0; fan < g_mesh->mi->tile_count; fan++)
	{
		g_mesh->mem->tilelst[fan].inrenderlist = false;

		if (m_num_totl < MAXMESHRENDER)
		{
			bool is_shine, is_noref, is_norml, is_water;

			g_mesh->mem->tilelst[fan].inrenderlist = true;

//			is_shine = mesh_has_all_bits(mm->tilelst, fan, MPDFX_SHINY);
//			is_noref = mesh_has_all_bits(mm->tilelst, fan, MPDFX_NOREFLECT);
//			is_norml = !is_shine;
//			is_water = mesh_has_some_bits(mm->tilelst, fan, MPDFX_WATER);

			is_norml = true; // TODO: Added by xenom
			is_noref = false;
			is_shine = false;
			is_water = false;

			// Put each tile in basic list
			m_totl[m_num_totl] = fan;
			m_num_totl++;
//			mesh_fan_add_renderlist(mm->tilelst, fan);

			// Put each tile
			if (!is_noref)
			{
				m_reflc[m_num_reflc] = fan;
				m_num_reflc++;
			}

			if (is_shine)
			{
				m_shine[m_num_shine] = fan;
				m_num_shine++;
			}

			if (is_norml)
			{
				m_norm[m_num_norm] = fan;
				m_num_norm++;
			}

			if (is_water)
			{
				// precalculate the "mode" variable so that we don't waste time rendering the waves
				int tx, ty;

				ty = fan / g_mesh->mi->tiles_x;
				tx = fan % g_mesh->mi->tiles_x;

				m_watr_mode[m_num_watr] = ((ty & 1) << 1) + (tx & 1);
				m_watr[m_num_watr]      = fan;

				m_num_watr++;
			}
		}
	}

	return true;
}
