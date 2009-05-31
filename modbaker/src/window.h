#ifndef window_h
#define window_h

#include <iostream>
#include <string>
using namespace std;

#include <guichan.hpp>
#include <guichan/sdl.hpp>
#include <guichan/opengl.hpp>
#include <guichan/opengl/openglsdlimageloader.hpp>
#include <guichan/opengl/openglimage.hpp>

// Action types for the input system
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
	WINDOW_SAVE_SHOW        = 26,
	WINDOW_SAVE_HIDE        = 27,
	WINDOW_LOAD_SHOW        = 28,
	WINDOW_LOAD_HIDE        = 29,
	ACTION_SET_TEXTURE      = 30,
	ACTION_SELMODE_VERTEX   = 31,
	ACTION_SELMODE_TILE     = 32,
	ACTION_SELMODE_OBJECT   = 33,
	ACTION_QUIT             = 34,
	ACTION_WELD_VERTICES    = 35,
	ACTION_SHOW_TILEFLAGS   = 36,
	ACTION_HIDE_TILEFLAGS   = 37,
	ACTION_TOGGLE_TILEFLAGS = 38
};


enum
{
	BUTTON_SAVE_OK     = 0,
	BUTTON_LOAD_OK     = 1,
	BUTTON_SAVE_CANCEL = 2,
	BUTTON_LOAD_CANCEL = 3
};

//---------------------------------------------------------------------
//-    Image loader
//---------------------------------------------------------------------
class c_mb_image_loader : public gcn::OpenGLSDLImageLoader
{
	public:
		c_mb_image_loader();
		gcn::Image* load_part(string, int, int, int, int);
};


//---------------------------------------------------------------------
//-    Handles widget input
//---------------------------------------------------------------------
class c_widget_input_handler : public gcn::MouseListener
{
	public:
		c_widget_input_handler() {}

		void mousePressed (gcn::MouseEvent &mouseEvent)
		{
			mouseEvent.consume();
		}

		void mouseMoved (gcn::MouseEvent &mouseEvent)
		{
			mouseEvent.consume();
		}
};


//---------------------------------------------------------------------
//-    Input handler
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
//-   A modbaker window
//---------------------------------------------------------------------
class c_mb_window : public gcn::Window
{
	public:
		c_mb_window(string);
};


class c_mb_tab : public gcn::Tab
{
	public:
		c_mb_tab(string);
};


class c_mb_tabbedarea : public gcn::TabbedArea
{
	public:
		c_mb_tabbedarea();
};


class c_mb_scroll_area : public gcn::ScrollArea
{
	public:
		c_mb_scroll_area();
};


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


class c_mb_textfield : public gcn::TextField
{
	public:
		c_mb_textfield();
};


class c_mb_label : public gcn::Label
{
	public:
		c_mb_label(string);
};


class c_mb_container : public gcn::Container
{
	public:
		c_mb_container();
};

class c_renderer;

//---------------------------------------------------------------------
//-   The list box model
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
//-   The window manager class
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
		c_mb_list_model* obj_list_model;

		// Object picker
		gcn::ListBox* obj_list_box;
		c_mb_container* c_object;
		c_mb_scroll_area* sc_object;

		// Info window
		c_mb_label* label_fps;
		c_mb_label* label_position;

		// The tooltip
		c_mb_label* label_tooltip;

		// The windows
		c_mb_window* w_palette;
		c_mb_window* w_object;
		c_mb_window* w_info;
		c_mb_window* w_save;
		c_mb_window* w_load;
		c_mb_window* w_texture;

		// Append an icon to a widget
		void add_icon(c_mb_container*, string, int, int, int, string);
		void add_texture_set(c_mb_container*, string, int, int, int, int);

	public:
		c_window_manager(c_renderer*);
		~c_window_manager();

		void set_fps(float);
		void set_position(float, float);

		void set_load_visibility(bool);
		void toggle_load_visibility();

		void set_save_visibility(bool);
		void toggle_save_visibility();

		// TODO: Merge those functions
		// Provide a c_window as parameter (with a get_window(string) function)
		bool set_texture_visibility(bool);
		bool toggle_texture_visibility();

		bool set_object_visibility(bool);
		bool toggle_object_visibility();

		bool set_info_visibility(bool);
		bool toggle_info_visibility();

		// TODO: Make private
		c_mb_textfield* tf_load;
		c_mb_textfield* tf_save;

		gcn::SDLInput* get_input() { return this->input; }

		void set_tooltip(string p_tooltip)
		{
			this->label_tooltip->setCaption(p_tooltip);
		}

		string get_tooltip()
		{
			return this->label_tooltip->getCaption();
		}

		gcn::Gui* get_gui();
		void set_gui(gcn::Gui*);

		bool create_texture_window(string);
		void destroy_texture_window()
		{
			if (this->w_texture != NULL)
				delete w_texture;
		}

		bool create_info_window();
		bool destroy_info_window();

		bool create_save_window();
		bool create_load_window();

		bool create_object_window(string);
		void destroy_object_window()
		{
			if (this->w_object != NULL)
				delete w_object;
		}

		bool create_mesh_window();
		bool destroy_mesh_window();
};
#endif
