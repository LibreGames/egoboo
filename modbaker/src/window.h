/// @file
/// @brief GUI system
/// @details Definition of the GUI system

#ifndef window_h
#define window_h

#include <iostream>
#include <string>
using namespace std;

#include "mesh.h"
#include "core/modmenu.h"

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
///   Action types for the input system
//---------------------------------------------------------------------
enum
{
	ACTION_MESH_NEW         =  0,
	ACTION_MESH_LOAD        =  1,
	ACTION_MESH_SAVE        =  2,
	ACTION_VERTEX_UP        =  3,
	ACTION_VERTEX_LEFT      =  4,
	ACTION_VERTEX_RIGHT     =  5,
	ACTION_VERTEX_DOWN      =  6,
	ACTION_VERTEX_INC       =  7,
	ACTION_VERTEX_DEC       =  8,
	ACTION_SELECTION_CLEAR  =  9,
	ACTION_EXIT             = 10,
	SCROLL_UP               = 11,
	SCROLL_LEFT             = 12,
	SCROLL_RIGHT            = 13,
	SCROLL_DOWN             = 14,
	SCROLL_INC              = 15,
	SCROLL_DEC              = 16,
	SCROLL_X_STOP           = 17,
	SCROLL_Y_STOP           = 18,
	SCROLL_Z_STOP           = 19,
	ACTION_MODIFIER_ON      = 20,
	ACTION_MODIFIER_OFF     = 21,
	ACTION_PAINT_TEXTURE    = 22,
	WINDOW_TEXTURE_TOGGLE   = 23,
	WINDOW_OBJECT_TOGGLE    = 24,
	WINDOW_INFO_TOGGLE      = 25,
	WINDOW_MESH_SAVE_SHOW   = 26,
	WINDOW_MESH_SAVE_HIDE   = 27,
	WINDOW_MESH_LOAD_SHOW   = 28,
	WINDOW_MESH_LOAD_HIDE   = 29,
	ACTION_SET_TEXTURE      = 30,
	ACTION_SELMODE_VERTEX   = 31,
	ACTION_SELMODE_TILE     = 32,
	ACTION_SELMODE_OBJECT   = 33,
	ACTION_QUIT             = 34,
	ACTION_WELD_VERTICES    = 35,
	ACTION_SHOW_TILEFLAGS   = 36,
	ACTION_HIDE_TILEFLAGS   = 37,
	ACTION_TOGGLE_TILEFLAGS = 38,
	WINDOW_TILE_FLAG_SHOW   = 39,
	WINDOW_TILE_FLAG_HIDE   = 40,
	WINDOW_TILE_FLAG_TOGGLE = 41,
	WINDOW_TILE_TYPE_SHOW   = 42,
	WINDOW_TILE_TYPE_HIDE   = 43,
	WINDOW_TILE_TYPE_TOGGLE = 44,
	ACTION_TILE_FLAG_PAINT  = 45,
	ACTION_TILE_TYPE_PAINT  = 46,
	ACTION_OBJECT_REMOVE    = 47,
	ACTION_MAKE_WALL        = 48,
	ACTION_MAKE_FLOOR       = 49,
	WINDOW_MESH_NEW_SHOW    = 50,
	WINDOW_MESH_NEW_HIDE    = 51,
	WINDOW_MESH_NEW_TOGGLE  = 52,
	WINDOW_MOD_MENU_SHOW    = 53,
	WINDOW_MOD_MENU_HIDE    = 54,
	WINDOW_MOD_MENU_TOGGLE  = 55,
	ACTION_SAVE_MOD_MENU    = 56,
	ACTION_LOAD_MOD_MENU    = 57
};


//---------------------------------------------------------------------
///   GI Button types
//---------------------------------------------------------------------
enum
{
	BUTTON_MESH_SAVE_OK     = 0,
	BUTTON_MESH_SAVE_CANCEL = 2,
	BUTTON_MESH_LOAD_OK     = 1,
	BUTTON_MESH_LOAD_CANCEL = 3,
	BUTTON_MESH_NEW_OK      = 4,
	BUTTON_MESH_NEW_CANCEL  = 5,
	BUTTON_MOD_MENU_OK      = 6,
	BUTTON_MOD_MENU_CANCEL  = 7
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
///   Definition of the general input handler
//---------------------------------------------------------------------
class c_input_handler : public gcn::MouseListener, public gcn::KeyListener
{
	private:
		bool window_input;

	public:
		c_input_handler();

		void do_something(int);

		void mousePressed(gcn::MouseEvent&);
		void mouseMoved(gcn::MouseEvent&);
		void keyPressed(gcn::KeyEvent&);
		void keyReleased(gcn::KeyEvent&);
};


//---------------------------------------------------------------------
///   Definition of a modbaker window
//---------------------------------------------------------------------
class c_mb_window : public gcn::Window
{
	public:
		c_mb_window(string);
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
//---------------------------------------------------------------------
class c_mb_button : public gcn::Button
{
	private:
		int type;

	public:
		c_mb_button();
		void set_type(int);
		int  get_type();

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
///   Definition of a modbaker GUI container
//---------------------------------------------------------------------
class c_mb_container : public gcn::Container
{
	public:
		c_mb_container();
};

class c_renderer;


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
///   Definition of the window for editing menu.txt
//---------------------------------------------------------------------
class c_mod_txt_window : public c_mb_window
{
	private:
		c_mb_label*     label_name;
		c_mb_textfield* tf_name;
		c_mb_label*     label_ref;
		c_mb_textfield* tf_ref;
		c_mb_label*     label_req_idsz;
		c_mb_textfield* tf_req_idsz;
		c_mb_label*     label_num_imp;
		c_mb_textfield* tf_num_imp;
		c_mb_label*     label_export;
		c_mb_textfield* tf_export;
		c_mb_label*     label_pla_min;
		c_mb_textfield* tf_pla_min;
		c_mb_label*     label_pla_max;
		c_mb_textfield* tf_pla_max;
		c_mb_label*     label_respawn;
		c_mb_textfield* tf_respawn;
		c_mb_label*     label_not_used;
		c_mb_textfield* tf_not_used;
		c_mb_label*     label_rating;
		c_mb_textfield* tf_rating;
		// Elements for the summary
		c_mb_label*     label_summary;
		c_mb_textfield* tf_summary1;
		c_mb_textfield* tf_summary2;
		c_mb_textfield* tf_summary3;
		c_mb_textfield* tf_summary4;
		c_mb_textfield* tf_summary5;
		c_mb_textfield* tf_summary6;
		c_mb_textfield* tf_summary7;
		c_mb_textfield* tf_summary8;
		// The buttons
		c_mb_button* button_ok;
		c_mb_button* button_cancel;

	public:
		c_mod_txt_window();

		c_menu_txt* get_menu_txt();
		void set_menu_txt(c_menu_txt*);
};


//---------------------------------------------------------------------
///   Definition of the window manager
//---------------------------------------------------------------------
class c_window_manager
{
	private:
		c_input_handler m_input_handler;
		// GUI
		gcn::OpenGLGraphics* graphics;
		c_mb_image_loader* imageLoader;

		gcn::Container* top;
		gcn::ImageFont* font;

		gcn::Gui* gui;
		gcn::SDLInput* input;

		// The tabs
		c_mb_tabbedarea* t_texture;

		// Object list
		c_mb_container* c_mesh;

		// Object picker
		c_mb_list_box*    obj_list_box;
		c_mb_list_model*  obj_list_model;
		c_mb_container*   c_object;
		c_mb_scroll_area* sc_object;

		// Tile types
		c_mb_list_box*    lb_tile_type;
		c_mb_list_model*  lm_tile_type;
		c_mb_container*   c_tile_type;
		c_mb_scroll_area* sc_tile_type;

		// Tile flags
		c_mb_list_box*    lb_tile_flag;
		c_mb_list_model*  lm_tile_flag;
		c_mb_container*   c_tile_flag;
		c_mb_scroll_area* sc_tile_flag;

		// Info window
		c_mb_label* label_fps;
		c_mb_label* label_position;

		// The tooltip
		c_mb_label* label_tooltip;

		// The windows
		c_mb_window* w_palette;
		c_mb_window* w_object;
		c_mb_window* w_tile_type;
		c_mb_window* w_tile_flag;
		c_mb_window* w_info;
		c_mb_window* w_mesh_save;
		c_mb_window* w_mesh_load;
		c_mb_window* w_mesh_new;
		c_mod_txt_window* w_mod_menu;
		c_mb_window* w_texture;
		c_mb_window* w_spawn;
		c_mb_window* w_minimap;
		c_mb_window* w_config;

		// input fields
		c_mb_textfield* tf_name;   // New mesh: module name
		c_mb_textfield* tf_size_x; // New mesh: x size
		c_mb_textfield* tf_size_y; // New mesh: y size


		int selected_object;
		int selected_tile_flag;
		int selected_tile_type;

		// Append an icon to a widget
		void add_icon(c_mb_container*, string, int, int, int, string);
		void add_texture_set(c_mb_container*, string, int, int, int, int);

	public:
		c_window_manager(c_renderer*);
		~c_window_manager();

		/// Set the text for the "name" textfield
		/// \param p_name new name
		void            set_tf_name(c_mb_textfield* p_name)   { this->tf_name = p_name; }
		/// Get the text for the "name" textfield
		/// \return name
		c_mb_textfield* get_tf_name()                         { return this->tf_name; }

		/// Set the text for the "x size" textfield
		/// \param p_size_x new x size
		void            set_tf_size_x(c_mb_textfield* p_size_x) { this->tf_size_x = p_size_x; }
		/// Get the text for the "x size" textfield
		/// \return x size
		c_mb_textfield* get_tf_size_x()                         { return this->tf_size_x; }

		/// Set the text for the "y size" textfield
		/// \param p_size_y new y size
		void            set_tf_size_y(c_mb_textfield* p_size_y) { this->tf_size_x = p_size_y; }
		/// Get the text for the "y size" textfield
		/// \return y size
		c_mb_textfield* get_tf_size_y()                         { return this->tf_size_y; }

		void set_fps(float);
		void set_position(float, float);

		bool set_visibility(string, bool);
		bool toggle_visibility(string);

		c_mb_window* get_window(string);

		int get_selected_object();
		int get_selected_tile_flag();
		int get_selected_tile_type();

		void set_selected_object(int);
		void set_selected_tile_flag(int);
		void set_selected_tile_type(int);

		/// \todo make the text fields private
		c_mb_textfield* tf_load; ///< "Load mesh" textfield
		c_mb_textfield* tf_save; ///< "Save mesh" textfield


		/// Get the input object
		/// \return input object
		gcn::SDLInput* get_input() { return this->input; }


		/// Set the current tooltip
		/// \param p_tooltip new tooltip
		void set_tooltip(string p_tooltip)
		{
			this->label_tooltip->setCaption(p_tooltip);
		}


		/// Get the current tooltip
		/// \return tooltip
		string get_tooltip()
		{
			return this->label_tooltip->getCaption();
		}


		gcn::Gui* get_gui();
		void set_gui(gcn::Gui*);

		bool create_texture_window(string);
		void destroy_texture_window();

		bool create_info_window();
//		bool destroy_info_window();

		bool create_tile_flag_window();
//		bool destroy_tile_flag_window();

		bool create_tile_type_window();
//		bool destroy_tile_type_window();

		bool create_mesh_save_window();
//		bool destroy_mesh_save_window();
		bool create_mesh_load_window();
//		bool destroy_mesh_load_window();
		bool create_mesh_new_window();
//		bool destroy_mesh_new_window();
		bool create_mod_menu_window();
//		bool destroy_mod_menu_window();

		bool create_object_window(string);
		void destroy_object_window();

		/// Get the menu.txt object
		/// \return menu.txt object
		c_menu_txt* get_menu_txt() { return this->w_mod_menu->get_menu_txt(); }
		/// Set the menu.txt object
		/// \param p_menu_txt new menu.txt object
		void set_menu_txt(c_menu_txt* p_menu_txt) { this->w_mod_menu->set_menu_txt(p_menu_txt); }

		bool create_mesh_window();
//		bool destroy_mesh_window();
};
#endif
