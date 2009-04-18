#ifndef window_h
#define window_h

#include <guichan.hpp>
#include <guichan/sdl.hpp>
#include <guichan/opengl.hpp>
#include <guichan/opengl/openglsdlimageloader.hpp>


#include <string>

using namespace std;


//---------------------------------------------------------------------
//-   A modbaker window
//---------------------------------------------------------------------
class c_modbaker_window : public gcn::Window
{
	public:
		c_modbaker_window(string);
};


//---------------------------------------------------------------------
//-   The list box model
//---------------------------------------------------------------------
class c_modbaker_list_model : public gcn::ListModel
{
	private:
		int number_of_elements;
		vector <string> elements;

	public:
		bool add_element(string p_name)
		{
			this->elements.push_back(p_name);
			this->number_of_elements++;
			return true;
		}

		bool remove_element(int);

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
		// GUI
		gcn::OpenGLGraphics* graphics;
		gcn::OpenGLSDLImageLoader* imageLoader;

		gcn::Container* top;

		gcn::ImageFont* font;

		// Texture window
		c_modbaker_window* w_texture;
		gcn::Label* label2;

		// Info window
		gcn::Label* label_fps;
		c_modbaker_window* w_info;

		// Object window
		c_modbaker_list_model* obj_list_model;
		gcn::ListBox* obj_list_box;
		c_modbaker_window* w_object;

		// Other windows
		c_modbaker_window* w_toolbar;

	public:
		c_window_manager();
		~c_window_manager();

		bool init();
		void set_fps(float);

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

		bool create_toolbar();
		bool destroy_toolbar();
};
#endif
