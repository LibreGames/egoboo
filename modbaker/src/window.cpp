//#include "global.h"

#include "global.h"
#include "window.h"

#include <iostream>
#include <string>
#include <sstream>
using namespace std;


c_mb_image_loader::c_mb_image_loader()
{
}


gcn::Image* c_mb_image_loader::load_part(string p_filename, int p_y, int p_x, int p_width, int p_height)
{
	SDL_Surface *loadedSurface = loadSDLSurface(p_filename);
	SDL_Surface *s_part;

	// TODO: Are width and height in the right oder?
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


//---------------------------------------------------------------------
//-   Set default actions for the widgets
//---------------------------------------------------------------------
c_mb_window::c_mb_window(string caption)
{
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
	this->setMovable(true);
	this->setCaption(caption);

	// Catch all events withing the gui
	c_widget_input_handler *wil;
	wil = new c_widget_input_handler();

	this->addMouseListener(wil);
}


c_mb_label::c_mb_label(string p_caption)
{
	this->setCaption(p_caption);

	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


c_mb_tab::c_mb_tab(string p_caption)
{
	this->setCaption(p_caption);

	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


c_mb_tabbedarea::c_mb_tabbedarea()
{
	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


c_mb_textfield::c_mb_textfield()
{
	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


c_mb_image_button::c_mb_image_button(gcn::Image* image)
{
	this->setImage(image);

	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


c_mb_button::c_mb_button()
{
	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


c_mb_container::c_mb_container()
{
	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


void c_mb_image_button::mousePressed(gcn::MouseEvent &event)
{
}

//---------------------------------------------------------------------
//-   c_window_manager constructor
//---------------------------------------------------------------------
c_window_manager::c_window_manager(c_renderer* p_renderer)
{
	try
	{
		// Init the image loader
		imageLoader = new c_mb_image_loader();
		gcn::Image::setImageLoader(imageLoader);

		// Init the graphics
		graphics = new gcn::OpenGLGraphics(p_renderer->get_screen()->w, p_renderer->get_screen()->h);
		graphics->setTargetPlane(p_renderer->get_screen()->w, p_renderer->get_screen()->h);

		// Load the font
	//	string pfname = g_config.get_egoboo_path() + "basicdat/" + g_config.get_font_file();
	//	font = new gcn::contrib::OGLFTFont(pfname.c_str(), g_config.get_font_size());
		font = new gcn::ImageFont("data/rpgfont.png", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-+/():;%&``*#=[]'");
		gcn::Widget::setGlobalFont(font);

		// Setup the GUI
		input = new gcn::SDLInput();

		g_input_handler = new c_input_handler();

		gui = new gcn::Gui();
		gui->setInput(input);
		gui->setGraphics(graphics);

		// Create the top container
		top = new gcn::Container();
		top->setDimension(gcn::Rectangle(0, 0, p_renderer->get_screen()->w, p_renderer->get_screen()->h));
		top->setOpaque(false);
		top->addMouseListener(g_input_handler);
		top->addKeyListener(g_input_handler);
		gui->setTop(top);

		// Create the palette container
		this->t_palette = new c_mb_tabbedarea();
		this->t_palette->setSize(130, 320);

		this->w_palette = new c_mb_window("Palette");
		this->top->add(this->w_palette);
		this->w_palette->setDimension(gcn::Rectangle(0, 0, 132, 320));
		this->w_palette->setPosition(0, 0);
		this->w_palette->add(t_palette);
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
	// Create the windows
	this->create_mesh_window();
	this->create_texture_window();
	this->create_info_window();
	this->create_object_window();
	this->create_filename_window();
}


//---------------------------------------------------------------------
//-   c_window_manager destructor
//---------------------------------------------------------------------
c_window_manager::~c_window_manager()
{
	// TODO: Add missing objects
	delete this->c_mesh;
	delete this->c_object;
	delete this->w_info;
	delete this->w_palette;

	delete this->font;
	delete this->top;
	delete this->gui;

	delete this->graphics;
	delete this->imageLoader;
}


//---------------------------------------------------------------------
//-   Update the FPS in the info window
//---------------------------------------------------------------------
void c_window_manager::set_fps(float p_fps)
{
	ostringstream tmp_str;
	tmp_str << "FPS: " << p_fps;

//	this->label_fps->setCaption(tmp_str.str());
}


//---------------------------------------------------------------------
//-   Update the FPS in the info window
//---------------------------------------------------------------------
void c_window_manager::set_position(float p_x, float p_y)
{
	ostringstream tmp_str;
	tmp_str << "Pos: " << p_x << ", " << p_y;

//	this->label_position->setCaption(tmp_str.str());
}


//---------------------------------------------------------------------
//-   Add an icon to a container
//---------------------------------------------------------------------
void c_window_manager::add_icon(c_mb_container* p_container, string p_filename, int p_x, int p_y, int p_id)
{
	try {
		c_mb_image_button* icon;
		gcn::Image* image;

		image = gcn::Image::load(p_filename);
		icon = new c_mb_image_button(image);
		icon->setSize(32, 32);

		ostringstream tmp_id;
		tmp_id << p_id;

		icon->setId(tmp_id.str());

		p_container->add(icon, p_x, p_y);
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
//-   Add a set of textures
//---------------------------------------------------------------------
void c_window_manager::add_texture_set(c_mb_container* p_container, string p_filename, int p_x, int p_y, int p_size)
{
	c_mb_image_button* icon;
	gcn::Image* image;

	c_mb_image_loader* ilo;
	ilo = new c_mb_image_loader();

	int t_y, t_x;

	try {
		for (t_y=0; t_y<8; t_y++)
		{
			for (t_x=0; t_x<8; t_x++)
			{
				image = ilo->load_part(p_filename, (t_y * p_size), (t_x * p_size), p_size, p_size);
				icon = new c_mb_image_button(image);
				icon->setSize(p_size, p_size);

				p_container->add(icon, (p_x + (t_x * p_size)), (p_y + (t_y * p_size)));
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
//-   Create the mesh window
//---------------------------------------------------------------------
bool c_window_manager::create_mesh_window()
{
	try
	{
		c_mb_tab* tab;
		tab = new c_mb_tab("Mesh");

		this->c_mesh = new c_mb_container();
		this->c_mesh->setSize(128, 200);

		// Add the icons
		this->add_icon(this->c_mesh, "data/document-new.png",   0,  0, ACTION_MESH_NEW);
		this->add_icon(this->c_mesh, "data/document-open.png", 32,  0, WINDOW_LOAD_SHOW);
		this->add_icon(this->c_mesh, "data/document-save.png", 64,  0, WINDOW_SAVE_SHOW);

		this->add_icon(this->c_mesh, "data/list-add.png",       0, 32, ACTION_VERTEX_INC);
		this->add_icon(this->c_mesh, "data/list-remove.png",   64, 32, ACTION_VERTEX_DEC);

		this->add_icon(this->c_mesh, "data/go-up.png",         32, 32, ACTION_VERTEX_UP);
		this->add_icon(this->c_mesh, "data/go-previous.png",    0, 64, ACTION_VERTEX_LEFT);
		this->add_icon(this->c_mesh, "data/go-next.png",       64, 64, ACTION_VERTEX_RIGHT);
		this->add_icon(this->c_mesh, "data/go-down.png",       32, 96, ACTION_VERTEX_DOWN);

		this->add_icon(this->c_mesh, "data/edit-clear.png",    64, 128, ACTION_SELECTION_CLEAR);
		this->add_icon(this->c_mesh, "data/applications-graphics.png", 32, 128, WINDOW_TEXTURE_TOGGLE);

		this->t_palette->addTab(tab, this->c_mesh);

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
//-   Create the texture tab
//---------------------------------------------------------------------
bool c_window_manager::create_texture_window()
{
	string file;
	ostringstream tmp_str;
	int tab;
	int size;

	c_mb_tab*       tmp_tab;
	c_mb_container* tmp_container;

	try
	{
		if (g_config == NULL)
			cout << "Warning: Config is NULL!" << endl;

		// The window
		this->w_texture = new c_mb_window("Textures");
		this->top->add(this->w_texture);
		this->w_texture->setPosition(50, 0);
		this->w_texture->setVisible(false);

		// The tabbed area
		this->t_texture = new c_mb_tabbedarea();
		this->t_texture->setSize(256, 300);

		// Create the tabs
		for (tab=0; tab<=2; tab++)
		{
			tmp_str.str("");
			tmp_str.clear();

			tmp_str << tab;

			size = 32;
			if (tab >= 1)
				size *= 2;

			tmp_tab = new c_mb_tab("Tile " + tmp_str.str());
			tmp_container = new c_mb_container();
			tmp_container->setSize(256, 256);

			file = g_config->get_egoboo_path() + "modules/" + g_mesh->modname + "/gamedat/tile" + tmp_str.str() + ".bmp";
			this->add_texture_set(tmp_container, file, 0, 0, size);
			this->t_texture->addTab(tmp_tab, tmp_container);

			this->w_texture->add(t_texture);
			// The tabbed area is ~40 pixels high
			this->w_texture->setDimension(gcn::Rectangle(0, 0, 258, 298));
			tmp_tab       = NULL;
			tmp_container = NULL;
		}

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
//-   Create the object tab
//---------------------------------------------------------------------
bool c_window_manager::create_object_window()
{
	try
	{
		// The object window
		this->obj_list_model = new c_mb_list_model();
		this->obj_list_box = new gcn::ListBox(this->obj_list_model);

		this->obj_list_model->add_element("test.obj");
		this->obj_list_model->add_element("test2.obj");
		this->obj_list_model->add_element("test3.obj");
		this->obj_list_model->add_element("advent.obj");

		this->obj_list_box->setDimension(gcn::Rectangle(0, 0, 128, 200));

		c_mb_tab* tab;
		tab = new c_mb_tab("Objects");

		this->c_object = new c_mb_container();
		this->c_object->add(this->obj_list_box);
		this->c_object->setSize(128, 200);
		this->t_palette->addTab(tab, this->c_object);

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
//-   Create the info window
//---------------------------------------------------------------------
bool c_window_manager::create_info_window()
{
	return false;
	try
	{
		this->label_fps = new c_mb_label("FPS: ???");
		this->label_fps->setPosition(0, 0);

		this->label_position = new c_mb_label("Position: ???");
//		this->label_position->setPosition(50, 0);

		this->w_info = new c_mb_window("Info");
		this->w_info->add(this->label_fps);
		this->w_info->add(this->label_position);
		this->top->add(this->w_info);
		this->w_info->setDimension(gcn::Rectangle(0, 0, 100, 100));
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
//-   Create the window for saving and loading
//---------------------------------------------------------------------
bool c_window_manager::create_filename_window()
{
	try
	{
		gcn::Label* tmp_label;
		tmp_label = new c_mb_label("Filename:");
		tmp_label->setPosition(0, 0);

		c_mb_textfield* textfield;
		textfield = new c_mb_textfield();
		textfield->setText("level.mpd");
		textfield->setPosition(0, 20);
		textfield->setSize(100, 20);

		c_mb_button* button;
		button = new c_mb_button();
		button->setCaption("OK");
		button->setPosition(0, 40);
		button->setSize(100, 20);

		this->w_filename = new c_mb_window("Load / save file");
		this->w_filename->add(tmp_label);
		this->w_filename->add(textfield);
		this->w_filename->add(button);
		this->top->add(this->w_filename);
		this->w_filename->setDimension(gcn::Rectangle(0, 0, 200, 100));
		this->w_filename->setPosition(250, 0);
		this->w_filename->setVisible(false);

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


gcn::Gui* c_window_manager::get_gui()
{
	return this->gui;
}


void c_window_manager::set_gui(gcn::Gui* p_gui)
{
	this->gui = p_gui;
}


void c_window_manager::set_filename_visibility(bool p_visibility)
{
	this->w_filename->setVisible(p_visibility);
}


void c_window_manager::toggle_filename_visibility()
{
	if (this->w_filename->isVisible())
		this->w_filename->setVisible(false);
	else
		this->w_filename->setVisible(true);
}


void c_window_manager::set_texture_visibility(bool p_visibility)
{
	this->w_texture->setVisible(p_visibility);
}


void c_window_manager::toggle_texture_visibility()
{
	if (this->w_texture->isVisible())
		this->w_texture->setVisible(false);
	else
		this->w_texture->setVisible(true);
}
