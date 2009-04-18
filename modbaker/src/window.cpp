//#include "global.h"

#include "window.h"

#include <iostream>
#include <string>
#include <sstream>
using namespace std;


//---------------------------------------------------------------------
//-
//---------------------------------------------------------------------
c_modbaker_window::c_modbaker_window(string caption)
{
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
	this->setMovable(true);
	this->setCaption(caption);
}


//---------------------------------------------------------------------
//-
//---------------------------------------------------------------------
c_window_manager::c_window_manager()
{
}


//---------------------------------------------------------------------
//-
//---------------------------------------------------------------------
c_window_manager::~c_window_manager()
{
	delete this->w_texture;
	delete this->w_info;
	delete this->w_object;
//	delete this->w_toolbar;

	delete this->font;
	delete this->top;
	delete this->gui;

	delete this->graphics;
	delete this->imageLoader;
}


void c_window_manager::set_fps(float p_fps)
{
	ostringstream tmp_fps;
	tmp_fps << "FPS: " << p_fps;

	this->label_fps->setCaption(tmp_fps.str());
}


//---------------------------------------------------------------------
//-
//---------------------------------------------------------------------
bool c_window_manager::init()
{
	// Init the image loader
	imageLoader = new gcn::OpenGLSDLImageLoader();
	gcn::Image::setImageLoader(imageLoader);

	// Init the graphics
	graphics = new gcn::OpenGLGraphics(screen->w, screen->h);
	graphics->setTargetPlane(screen->w, screen->h);

	// Load the font
//	string pfname = g_config.get_egoboo_path() + "basicdat/" + g_config.get_font_file();
//	font = new gcn::contrib::OGLFTFont(pfname.c_str(), g_config.get_font_size());
	font = new gcn::ImageFont("rpgfont.png", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-+/():;%&``*#=[]'");
	gcn::Widget::setGlobalFont(font);

	// Setup the GUI
	input = new gcn::SDLInput();


	gui = new gcn::Gui();
	gui->setInput(input);
	gui->setGraphics(graphics);

	top = new gcn::Container();
	top->setDimension(gcn::Rectangle(0, 0, screen->w, screen->h));
	top->setOpaque(false);
	gui->setTop(top);


	// Create the windows
	this->create_texture_window();
	this->create_info_window();
	this->create_object_window();
//	this->create_toolbar();
	return true;
}


//---------------------------------------------------------------------
//-
//---------------------------------------------------------------------
bool c_window_manager::create_texture_window()
{
	try
	{
		this->label2 = new gcn::Label("Test 2");
		this->label2->setPosition(0, 0);

		this->w_texture = new c_modbaker_window("Textures");
		this->w_texture->add(this->label2);
		this->top->add(this->w_texture);
		this->w_texture->setDimension(gcn::Rectangle(0, 0, 150, 50));

		return true;
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
	return false;
}


//---------------------------------------------------------------------
//-
//---------------------------------------------------------------------
bool c_window_manager::create_info_window()
{
	try
	{
		// The info window
		this->label_fps = new gcn::Label("FPS: ???");
		this->label_fps->setPosition(0, 0);

		this->w_info = new c_modbaker_window("Info");
		this->w_info->add(this->label_fps);
		this->top->add(this->w_info);
		this->w_info->setDimension(gcn::Rectangle(0, 0, 100, 50));
		this->w_info->setPosition(500, 0);

		return true;
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
	return false;
}


//---------------------------------------------------------------------
//-
//---------------------------------------------------------------------
bool c_window_manager::create_object_window()
{
	try
	{
		// The object window
		this->obj_list_model = new c_modbaker_list_model();
		this->obj_list_box = new gcn::ListBox(this->obj_list_model);

		this->obj_list_model->add_element("test.obj");
		this->obj_list_model->add_element("test2.obj");
		this->obj_list_model->add_element("test3.obj");
		this->obj_list_model->add_element("advent.obj");

		this->w_object = new c_modbaker_window("Objects");
		this->w_object->add(this->obj_list_box);
		this->top->add(this->w_object);
		this->w_object->setDimension(gcn::Rectangle(0, 0, 200, 50));
		this->w_object->setPosition(0, 300);

		return true;
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
	return false;
}


//---------------------------------------------------------------------
//-
//---------------------------------------------------------------------
bool c_window_manager::create_toolbar()
{
	return false;
}
