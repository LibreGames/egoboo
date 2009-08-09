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
/// @brief Widget definitions
/// @details Definition of the GUI widgets

#ifndef ui_widgets_h
#define ui_widgets_h

#include <iostream>
#include <string>
using namespace std;

//#include "../core/modmenu.h"
//#include "../SDL_extensions.h"

#include <guichan.hpp>
#include <guichan/sdl.hpp>
#include <guichan/opengl.hpp>
#include <guichan/opengl/openglsdlimageloader.hpp>
#include <guichan/opengl/openglimage.hpp>


//---------------------------------------------------------------------
///   Types for the list box
//---------------------------------------------------------------------
enum list_boxes
{
	LIST_BOX_OBJECT,    ///< Object list box
	LIST_BOX_TILE_FLAG, ///< Tile flag list box
	LIST_BOX_TILE_TYPE  ///< Tile type selection list box
};


//---------------------------------------------------------------------
///   the general action listener for all widgets
//---------------------------------------------------------------------
class c_mb_action_listener : public gcn::ActionListener
{
	private:
	protected:
	public:
		c_mb_action_listener();

		virtual void action(const gcn::ActionEvent&) = 0;
};


//---------------------------------------------------------------------
///   Definition of the widget input handler
//---------------------------------------------------------------------
class c_widget_input_handler : public gcn::MouseListener
{
	public:
		c_widget_input_handler() {}

		/// Gets called when the user clicks on a widget
		/// \param mouseEvent mouse event
		void mousePressed (gcn::MouseEvent &mouseEvent)
		{
			mouseEvent.consume();
		}

		/// Gets called when the mouse got moved on a widget
		/// \param mouseEvent mouse event
		void mouseMoved (gcn::MouseEvent &mouseEvent)
		{
			mouseEvent.consume();
		}
};


//---------------------------------------------------------------------
///    Definition of the image loader
//---------------------------------------------------------------------
class c_mb_image_loader : public gcn::OpenGLSDLImageLoader
{
	public:
		c_mb_image_loader();
		gcn::Image* load_part(string, int, int, int, int);
};


//---------------------------------------------------------------------
///   Definition of a modbaker GUI container
//---------------------------------------------------------------------
class c_mb_container : public gcn::Container
{
	private:
		c_mb_action_listener *m_action_listener;
	public:
		c_mb_container();

		void set_action_listener(c_mb_action_listener*);

		// Add a widget to the container
		void add_texture_set(string, int, int, int, int);
};


//---------------------------------------------------------------------
///   Definition of a modbaker GUI tab
//---------------------------------------------------------------------
class c_mb_tab : public gcn::Tab
{
	public:
		c_mb_tab(string);
};


//---------------------------------------------------------------------
///   Definition of a modbaker GUI tabbed area
//---------------------------------------------------------------------
class c_mb_tabbedarea : public gcn::TabbedArea
{
	public:
		c_mb_tabbedarea();
};


//---------------------------------------------------------------------
///   Definition of a modbaker GUI scroll area
//---------------------------------------------------------------------
class c_mb_scroll_area : public gcn::ScrollArea
{
	public:
		c_mb_scroll_area();
};


//---------------------------------------------------------------------
///   Definition of a modbaker GUI image button
//---------------------------------------------------------------------
class c_mb_image_button : public gcn::ImageButton
{
	private:
		string tooltip;

	public:
		c_mb_image_button(gcn::Image*);

		virtual void mousePressed(gcn::MouseEvent &);
		virtual void mouseMoved(gcn::MouseEvent &);

		void set_tooltip(string p_tooltip) { this->tooltip = p_tooltip; }
		string get_tooltip()               { return this->tooltip; }
};


//---------------------------------------------------------------------
///   Definition of a modbaker GUI button
///   \todo remove parent
//---------------------------------------------------------------------
class c_mb_button : public gcn::Button
{
	private:
		int type;
		c_mb_container *m_parent;

	public:
		c_mb_button();

		virtual void mousePressed(gcn::MouseEvent &);
};


//---------------------------------------------------------------------
///   Definition of a modbaker GUI text field
//---------------------------------------------------------------------
class c_mb_textfield : public gcn::TextField
{
	public:
		c_mb_textfield();
};


//---------------------------------------------------------------------
///   Definition of a modbaker GUI label
//---------------------------------------------------------------------
class c_mb_label : public gcn::Label
{
	public:
		c_mb_label(string);
};


//---------------------------------------------------------------------
///   Definition of a modbaker GUI list model
//---------------------------------------------------------------------
class c_mb_list_model : public gcn::ListModel
{
	private:
		int number_of_elements;
		vector <string> elements;

	public:
		c_mb_list_model()
		{
			this->number_of_elements = 0;
		}

		void add_element(string p_name)
		{
			this->elements.push_back(p_name);
			this->number_of_elements++;
		}

//		bool remove_element(int);

		std::string getElementAt(int i)
		{
			if (i < this->number_of_elements)
			{
				return elements[i];
			}

			return "error";
		}

		int getNumberOfElements()
		{
			return elements.size();
		}
};


//---------------------------------------------------------------------
///   Definition of a modbaker GUI list box
//---------------------------------------------------------------------
class c_mb_list_box : public gcn::ListBox
{
	public:
		c_mb_list_box(c_mb_list_model *model)
		{
			this->setListModel(model);
		}
};


//---------------------------------------------------------------------
///   Definition of a modbaker window
//---------------------------------------------------------------------
class c_mb_window : public gcn::Window
{
	private:
	public:
		c_mb_window(string);
		c_mb_window(c_mb_action_listener*, string);

		void add_icon(c_mb_action_listener*, string, int, int, int, string);
};
#endif
