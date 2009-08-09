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

/// @file
/// @brief helper functions for the GUI system
/// @details Implementation of the GUI system helper functions

#include <iostream>
#include <string>
using namespace std;

#include "../utility.h"
#include "ui_widgets.h"


//---------------------------------------------------------------------
///   c_mb_window constructor
///   \param p_caption caption of the window
//---------------------------------------------------------------------
c_mb_window::c_mb_window(string p_caption)
{
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
	this->setMovable(true);
	this->setCaption(p_caption);

	// Catch all events withing the gui
	c_widget_input_handler *wil;
	wil = new c_widget_input_handler();

	this->addMouseListener(wil);
}


//---------------------------------------------------------------------
///   c_mb_container constructor
//---------------------------------------------------------------------
c_mb_container::c_mb_container()
{
	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
	this->m_action_listener = NULL;
}


//---------------------------------------------------------------------
///   Set the action listener
///   \param p_ali action listener
//---------------------------------------------------------------------
void c_mb_container::set_action_listener(c_mb_action_listener *p_ali)
{
	this->m_action_listener = p_ali;
}


//---------------------------------------------------------------------
///   c_mb_tab constructor
///   \param p_caption caption of the tab
//---------------------------------------------------------------------
c_mb_tab::c_mb_tab(string p_caption)
{
	this->setCaption(p_caption);

	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


//---------------------------------------------------------------------
///   c_mb_tabbedarea constructor
//---------------------------------------------------------------------
c_mb_tabbedarea::c_mb_tabbedarea()
{
	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


//---------------------------------------------------------------------
///   c_mb_label constructor
///   \param p_caption caption of the label
//---------------------------------------------------------------------
c_mb_label::c_mb_label(string p_caption)
{
	this->setCaption(p_caption);

	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


//---------------------------------------------------------------------
///   c_mb_textfield constructor
//---------------------------------------------------------------------
c_mb_textfield::c_mb_textfield()
{
	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


//---------------------------------------------------------------------
///   c_mb_scroll_area constructor
//---------------------------------------------------------------------
c_mb_scroll_area::c_mb_scroll_area()
{
	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


//---------------------------------------------------------------------
///   c_mb_image_button constructor
///   \param image image object for the image button
//---------------------------------------------------------------------
c_mb_image_button::c_mb_image_button(gcn::Image* image)
{
	this->setImage(image);
	this->tooltip = "";

	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


//---------------------------------------------------------------------
///   c_mb_button constructor
//---------------------------------------------------------------------
c_mb_button::c_mb_button()
{
	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


//---------------------------------------------------------------------
///   Gets called when the mouse got pressed withing a button
///   \param event mouse event
//---------------------------------------------------------------------
void c_mb_button::mousePressed(gcn::MouseEvent &event)
{
	distributeActionEvent();
}


//---------------------------------------------------------------------
///   Gets called when a mouse button got pressed withing the image button
///   \param event mouse event
//---------------------------------------------------------------------
void c_mb_image_button::mousePressed(gcn::MouseEvent &event)
{
	distributeActionEvent();
}


//---------------------------------------------------------------------
///   Gets called when the mouse got moved withing the image button
///   \param event mouse event
///   \todo re-implement the tooltip
//---------------------------------------------------------------------
void c_mb_image_button::mouseMoved(gcn::MouseEvent &event)
{
/*	if (g_renderer->get_wm()->get_tooltip() == this->tooltip)
		return;

	g_renderer->get_wm()->set_tooltip(this->tooltip);
*/
}


//---------------------------------------------------------------------
///   Add an icon to a container
///   \param p_filename filename of the icon
///   \param p_x x position of the icon
///   \param p_y y position of the icon
///   \param p_id event ID of the icon
///   \param p_tooltip tooltip for the icon
//---------------------------------------------------------------------
void c_mb_window::add_icon(c_mb_action_listener *p_ali, string p_filename, int p_x, int p_y, int p_id, string p_tooltip)
{
	try {
		c_mb_image_button* icon;
		gcn::Image* image;

		image = gcn::Image::load(p_filename);
		icon = new c_mb_image_button(image);
		icon->setSize(32, 32);
		icon->set_tooltip(p_tooltip);

		icon->setActionEventId(translate_into_string(p_id));
		icon->addActionListener(p_ali);

		ostringstream tmp_id;
		tmp_id << p_id;

		icon->setId(tmp_id.str());

		this->add(icon, p_x, p_y);
	}
	catch (gcn::Exception e)
	{
		cout << e.getMessage() << endl;
	}
	catch (std::exception e)
	{
		cout << "Std exception: " << e.what() << endl;
	}
	catch (...)
	{
		cout << "Unknown exception" << endl;
	}
}


//---------------------------------------------------------------------
///   Add a texture selection area
///   \param p_filename filename of the texture set to load
///   \param p_set texture set to load
///   \param p_x x position of the selection area
///   \param p_y y position of the selection area
///   \param p_size site of each tile
//---------------------------------------------------------------------
void c_mb_container::add_texture_set(string p_filename, int p_set, int p_x, int p_y, int p_size)
{
	if (this->m_action_listener == NULL)
		cout << "WARNING: Action listener is NULL!" << endl;

	c_mb_image_button* icon;
	gcn::Image* image;

	c_mb_image_loader* ilo;
	ilo = new c_mb_image_loader();

	ostringstream tmp_str;

	int texture_id, cnt_y, cnt_x, tile_factor;

	tile_factor = 1;

	// When we use big textures, we only have the half amount of textures
	if (p_size == 64)
		tile_factor = 2;

	try {
		for (cnt_y=0; cnt_y<(8 / tile_factor); cnt_y++)
		{
			for (cnt_x=0; cnt_x<(8 / tile_factor); cnt_x++)
			{
				tmp_str.str("");
				tmp_str.clear();

				texture_id = (p_set * (64)) + (cnt_y * 8 * tile_factor) + (cnt_x * tile_factor);

				tmp_str << "tex;" << p_size << ";" << texture_id;

				image = ilo->load_part(p_filename, (cnt_y * p_size), (cnt_x * p_size), p_size, p_size);
				icon = new c_mb_image_button(image);
				icon->setSize(p_size, p_size);

				icon->setActionEventId(tmp_str.str());
				icon->addActionListener(this->m_action_listener);

				icon->set_tooltip("Select texture");

				this->add(icon, (p_x + (cnt_x * p_size)), (p_y + (cnt_y * p_size)));
				icon = NULL;
			}
		}
	}
	catch (gcn::Exception e)
	{
		cout << e.getMessage() << endl;
	}
	catch (std::exception e)
	{
		cout << "Std exception: " << e.what() << endl;
	}
	catch (...)
	{
		cout << "Unknown exception" << endl;
	}
}


//---------------------------------------------------------------------
///   c_mb_action_listener constructor
//---------------------------------------------------------------------
c_mb_action_listener::c_mb_action_listener()
{
	cout << "Setting up the action listener" << endl;
}


//---------------------------------------------------------------------
///   c_mb_image_loader constructor
//---------------------------------------------------------------------
c_mb_image_loader::c_mb_image_loader()
{
}


//---------------------------------------------------------------------
///   Load only a part of an image
///   \param p_filename filename of the image to load
///   \param p_y y starting position
///   \param p_x x starting position
///   \param p_width width of the extract
///   \param p_height height of the extract
///   \return image containing the extract
//---------------------------------------------------------------------
gcn::Image* c_mb_image_loader::load_part(string p_filename, int p_y, int p_x, int p_width, int p_height)
{
	SDL_Surface *loadedSurface = loadSDLSurface(p_filename);
	SDL_Surface *s_part;

	glEnable(GL_TEXTURE_2D);

	// Create a new surface
	s_part = SDL_CreateRGBSurface(SDL_SWSURFACE, p_width, p_height,
		32, rmask, gmask, bmask, amask);


	SDL_Rect rect;

	rect.x = p_x;
	rect.y = p_y;
	rect.w = p_width;
	rect.h = p_height;

	if (loadedSurface == NULL)
	{
		throw GCN_EXCEPTION(
		std::string("Unable to load image file: ") + p_filename);
	}

	if(SDL_BlitSurface(loadedSurface, &rect, s_part, NULL) < 0)
	{
		cout << "Error: " << SDL_GetError() << endl;
	}

	SDL_Surface *surface = convertToStandardFormat(s_part);
	SDL_FreeSurface(loadedSurface);
	SDL_FreeSurface(s_part);

	if (surface == NULL)
	{
		throw GCN_EXCEPTION(
			std::string("Not enough memory to load: ") + p_filename);
	}

	gcn::OpenGLImage *image = new gcn::OpenGLImage((unsigned int*)surface->pixels,
		surface->w,
		surface->h,
		true);
	SDL_FreeSurface(surface);

	return image;
}
