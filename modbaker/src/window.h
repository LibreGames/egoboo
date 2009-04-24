#ifndef window_h
#define window_h

#include <iostream>
#include <string>
using namespace std;

#include <guichan.hpp>
#include <guichan/sdl.hpp>
#include <guichan/opengl.hpp>
#include <guichan/opengl/openglsdlimageloader.hpp>


//---------------------------------------------------------------------
//-    Handles widget input
//---------------------------------------------------------------------
class c_widget_input_handler : public gcn::MouseListener
{
	public:
		c_widget_input_handler()
		{
		}

		void mousePressed (gcn::MouseEvent &mouseEvent)
		{
			mouseEvent.consume();
		}
};


//---------------------------------------------------------------------
//-    Input handler
//---------------------------------------------------------------------
class c_input_handler : public gcn::MouseListener
//, gcn::KeyListener
{
	private:
		bool gui_clicked;
		bool top;

	public:
		void init(bool p_top)
		{
			cout << "input handler created" << endl;
			this->top = p_top;
		}

		c_input_handler();

		void do_something(int);

		void mousePressed(gcn::MouseEvent&);
		void mouseMoved(gcn::MouseEvent&);

		bool get_gui_clicked()          { return this->gui_clicked; }
		void set_gui_clicked(bool guic) { this->gui_clicked = guic; }

		bool get_top()           { return this->top; }
		void set_top(bool p_top) { this->top = p_top; }
};


//---------------------------------------------------------------------
//-   A modbaker window
//---------------------------------------------------------------------
class c_mb_window : public gcn::Window
{
	public:
		c_mb_window(string);
};


class c_mb_image_button : public gcn::ImageButton
{
	public:
		c_mb_image_button(gcn::Image*);
		virtual void mousePressed(gcn::MouseEvent &);
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
			if (i <= this->number_of_elements)
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
		gcn::OpenGLSDLImageLoader* imageLoader;

		gcn::Container* top;

		gcn::ImageFont* font;

		// The palette tab
		c_mb_tabbedarea *t_palette;

		// The tabs
		c_mb_container* c_mesh;
		c_mb_container* c_texture;
		c_mb_list_model* obj_list_model;

		gcn::ListBox* obj_list_box;
		c_mb_container* c_object;

		// Info window
		c_mb_label* label_fps;
		c_mb_label* label_position;

		// The windows
		c_mb_window* w_palette;
		c_mb_window* w_info;

		// Append an icon to a widget
		void add_icon(c_mb_container*, string, int, int, int);

	public:
		c_window_manager();
		~c_window_manager();

		bool init();
		void set_fps(float);
		void set_position(float, float);

		// TODO: create setter and getter and make it private
		gcn::Gui* gui;
		SDL_Surface *screen;
		gcn::SDLInput* input;

		bool create_texture_window();
		bool destroy_texture_window();

		bool create_info_window();
		bool destroy_info_window();

		bool create_object_window();
		bool destroy_object_window();

		bool create_mesh_window();
		bool destroy_mesh_window();
};
#endif
