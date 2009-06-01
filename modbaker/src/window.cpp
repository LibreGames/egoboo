//#include "global.h"

#include "global.h"
#include "window.h"

#include <iostream>
#include <string>
#include <sstream>
using namespace std;


//---------------------------------------------------------------------
//-   Functions for the modbaker image loader
//---------------------------------------------------------------------
c_mb_image_loader::c_mb_image_loader()
{
}


gcn::Image* c_mb_image_loader::load_part(string p_filename, int p_y, int p_x, int p_width, int p_height)
{
	SDL_Surface *loadedSurface = loadSDLSurface(p_filename);
	SDL_Surface *s_part;

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


c_mb_label::c_mb_label(string p_caption)
{
	this->setCaption(p_caption);

	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


c_mb_textfield::c_mb_textfield()
{
	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


c_mb_scroll_area::c_mb_scroll_area()
{
	this->setBackgroundColor(gcn::Color(255, 255, 255, 230));
	this->setBaseColor(gcn::Color(255, 255, 255, 230));
}


c_mb_image_button::c_mb_image_button(gcn::Image* image)
{
	this->setImage(image);
	this->tooltip = "";

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


//---------------------------------------------------------------------
//-   A modbaker image button
//---------------------------------------------------------------------
void c_mb_image_button::mousePressed(gcn::MouseEvent &event)
{
}


void c_mb_image_button::mouseMoved(gcn::MouseEvent &event)
{
	if (g_renderer->get_wm()->get_tooltip() == this->tooltip)
		return;

	g_renderer->get_wm()->set_tooltip(this->tooltip);
}


//---------------------------------------------------------------------
//-   c_window_manager constructor
//---------------------------------------------------------------------
c_window_manager::c_window_manager(c_renderer* p_renderer)
{
	try
	{
		this->selected_object    = -1;
		this->selected_tile_flag = -1;
		this->selected_tile_type = -1;

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
		top->setFocusable(true);
		top->addMouseListener(g_input_handler);
		top->addKeyListener(g_input_handler);
		gui->setTop(top);
	}
	catch (gcn::Exception e)
	{
		cout << "Guichan exception: " << endl;
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
	// The texture and object window get created on module load
	this->create_mesh_window();
	this->create_info_window();
	this->create_load_window();
	this->create_save_window();

	this->create_tile_flag_window();
	this->create_tile_type_window();
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
	delete this->w_save;
	delete this->w_load;

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

	if (this->label_fps == NULL)
		return;

	this->label_fps->setCaption(tmp_str.str());
}


//---------------------------------------------------------------------
//-   Update the FPS in the info window
//---------------------------------------------------------------------
void c_window_manager::set_position(float p_x, float p_y)
{
	ostringstream tmp_str;
	tmp_str << "Pos: " << p_x << ", " << p_y;

	if (this->label_position == NULL)
		return;

	this->label_position->setCaption(tmp_str.str());
}


//---------------------------------------------------------------------
//-   Add an icon to a container
//---------------------------------------------------------------------
void c_window_manager::add_icon(c_mb_container* p_container, string p_filename, int p_x, int p_y, int p_id, string p_tooltip)
{
	try {
		c_mb_image_button* icon;
		gcn::Image* image;

		image = gcn::Image::load(p_filename);
		icon = new c_mb_image_button(image);
		icon->setSize(32, 32);
		icon->set_tooltip(p_tooltip);

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
void c_window_manager::add_texture_set(c_mb_container* p_container, string p_filename, int p_set, int p_x, int p_y, int p_size)
{
	c_mb_image_button* icon;
	gcn::Image* image;

	c_mb_image_loader* ilo;
	ilo = new c_mb_image_loader();

	ostringstream tmp_str;

	int t_y, t_x;

	try {
		for (t_y=0; t_y<8; t_y++)
		{
			for (t_x=0; t_x<8; t_x++)
			{

				tmp_str.str("");
				tmp_str.clear();

				tmp_str << "tex;" << p_set << ";" << t_y << ";" << t_x;

				image = ilo->load_part(p_filename, (t_y * p_size), (t_x * p_size), p_size, p_size);
				icon = new c_mb_image_button(image);
				icon->setSize(p_size, p_size);
				icon->setId(tmp_str.str());
				icon->set_tooltip("Select texture");

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
		this->c_mesh = new c_mb_container();
		this->c_mesh->setWidth(128);
		this->c_mesh->setHeight(320);

		// Add the icons
		this->add_icon(this->c_mesh, "data/window-texture.png",   0,  0, WINDOW_TEXTURE_TOGGLE, "Textures");
		this->add_icon(this->c_mesh, "data/window-object.png",   32,  0, WINDOW_OBJECT_TOGGLE,  "Objects");
		this->add_icon(this->c_mesh, "data/window-info.png",     64,  0, WINDOW_INFO_TOGGLE,    "Info");
		this->add_icon(this->c_mesh, "data/system-log-out.png",  96,  0, ACTION_EXIT, "Quit");

		this->add_icon(this->c_mesh, "data/window-tile-flag.png",  0,  32, WINDOW_TILE_FLAG_TOGGLE, "Tile flags");
		this->add_icon(this->c_mesh, "data/window-tile-type.png", 32,  32, WINDOW_TILE_TYPE_TOGGLE, "Tile types");

		this->add_icon(this->c_mesh, "data/document-new.png",   0, 64, ACTION_MESH_NEW, "New mesh");
		this->add_icon(this->c_mesh, "data/document-open.png", 32, 64, WINDOW_LOAD_SHOW, "Load mesh");
		this->add_icon(this->c_mesh, "data/document-save.png", 64, 64, WINDOW_SAVE_SHOW, "Save mesh");


		this->add_icon(this->c_mesh, "data/list-add.png",       0, 256, ACTION_TILE_FLAG_PAINT, "Set tile flag");
		this->add_icon(this->c_mesh, "data/list-add.png",      32, 256, ACTION_TILE_TYPE_PAINT, "Set tile type");


		this->add_icon(this->c_mesh, "data/list-add.png",       0, 96, ACTION_VERTEX_INC, "Incr. height");
		this->add_icon(this->c_mesh, "data/list-remove.png",   64, 96, ACTION_VERTEX_DEC, "Decr. height");

		this->add_icon(this->c_mesh, "data/tile_flags.png",       96,  64, ACTION_TOGGLE_TILEFLAGS, "Show tile flags");

		this->add_icon(this->c_mesh, "data/go-up.png",         32,  96, ACTION_VERTEX_UP, "Vertex up");
		this->add_icon(this->c_mesh, "data/go-previous.png",    0, 128, ACTION_VERTEX_LEFT, "Vertex left");
		this->add_icon(this->c_mesh, "data/go-next.png",       64, 128, ACTION_VERTEX_RIGHT, "Vertex right");
		this->add_icon(this->c_mesh, "data/go-down.png",       32, 160, ACTION_VERTEX_DOWN, "Vertex down");
		this->add_icon(this->c_mesh, "data/edit-clear.png",    32, 128, ACTION_SELECTION_CLEAR, "Clear sel.");

		this->add_icon(this->c_mesh, "data/weld-vertices.png",    64, 160, ACTION_WELD_VERTICES, "Weld sel.");

		this->add_icon(this->c_mesh, "data/selection-tile.png",    0, 192, ACTION_SELMODE_TILE,   "Tile select");
		this->add_icon(this->c_mesh, "data/selection-vertex.png", 32, 192, ACTION_SELMODE_VERTEX, "Vertex select");
		this->add_icon(this->c_mesh, "data/selection-object.png", 64, 192, ACTION_SELMODE_OBJECT, "Object select");

		this->label_tooltip = new c_mb_label("Tooltip");
		this->c_mesh->add(this->label_tooltip);
		this->label_tooltip->setPosition(0, 300);
		this->label_tooltip->setSize(130, 20);

		this->w_palette = new c_mb_window("Palette");
		this->w_palette->setDimension(gcn::Rectangle(0, 0, 132, 340));
		this->w_palette->setPosition(0, 0);

		// Create the palette container
		this->w_palette->add(c_mesh);    // Add container to the window
		this->top->add(this->w_palette); // Add window to the GUI

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
bool c_window_manager::create_texture_window(string p_modname)
{
	// TODO: Error: Tile0 does not get changed on module change
	string file;
	ostringstream tmp_str;
	int tab;
	int size;

	try
	{
		if (g_config == NULL)
			cout << "Warning: Config is NULL!" << endl;

		// The window
		this->w_texture = new c_mb_window("Texture");
		this->top->add(this->w_texture);
		this->w_texture->setPosition(50, 0);
		this->w_texture->setVisible(false);

		// The tabbed area
		this->t_texture = new c_mb_tabbedarea();
		this->t_texture->setSize(256, 300);

		c_mb_tab* tmp_tab;
		c_mb_container* tmp_container;

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

			file = g_config->get_egoboo_path() + "modules/" + p_modname + "/gamedat/tile" + tmp_str.str() + ".bmp";
			this->add_texture_set(tmp_container, file, tab, 0, 0, size);
			this->t_texture->addTab(tmp_tab, tmp_container);

			this->w_texture->add(t_texture);
			// The tabbed area is ~40 pixels high
			this->w_texture->setDimension(gcn::Rectangle(100, 100, 258, 298));
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
bool c_window_manager::create_object_window(string p_modname)
{
	try
	{
		// The object window
		this->obj_list_model = new c_mb_list_model();
		this->obj_list_box   = new c_mb_list_box(this->obj_list_model);

		unsigned int i;

		if (g_object_manager == NULL)
			return false;

		// Append all objects to the area
		for (i=0; i<g_object_manager->get_objects_size(); i++)
		{
			this->obj_list_model->add_element(g_object_manager->get_object(i)->get_name());
		}

		this->obj_list_box->adjustSize();

		this->sc_object = new c_mb_scroll_area();
		this->sc_object->setSize(128, 180);

		this->sc_object->setContent(this->obj_list_box);

		w_object = new c_mb_window("Object");
		w_object->setSize(130, 200);
		w_object->setPosition(100, 0);
		w_object->setVisible(false);

		// Create the palette container
		this->w_object->add(sc_object); // Add container to the window
		this->top->add(this->w_object); // Add window to the GUI

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
//-   Create the tile flag window
//---------------------------------------------------------------------
bool c_window_manager::create_tile_flag_window()
{
	try
	{
		// The object window
		this->lm_tile_flag = new c_mb_list_model();
		this->lb_tile_flag = new c_mb_list_box(this->lm_tile_flag);

		this->lm_tile_flag->add_element("Draw 1st");
		this->lm_tile_flag->add_element("Draw 2nd (Normal)");
		this->lm_tile_flag->add_element("Reflection");
		this->lm_tile_flag->add_element("Animated");
		this->lm_tile_flag->add_element("Water");
		this->lm_tile_flag->add_element("Wall");
		this->lm_tile_flag->add_element("Impassable");
		this->lm_tile_flag->add_element("Damage");
		this->lm_tile_flag->add_element("Ice / Slippy");

		this->lb_tile_flag->setSize(160, 175);

		this->sc_tile_flag = new c_mb_scroll_area();
		this->sc_tile_flag->setSize(168, 180);

		this->sc_tile_flag->setContent(this->lb_tile_flag);

		w_tile_flag = new c_mb_window("Tile flags");
		w_tile_flag->setSize(170, 200);
		w_tile_flag->setPosition(100, 0);
		w_tile_flag->setVisible(false);

		// Create the palette container
		this->w_tile_flag->add(sc_tile_flag); // Add container to the window
		this->top->add(this->w_tile_flag);    // Add window to the GUI

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
//-   Create the tile type window
//---------------------------------------------------------------------
bool c_window_manager::create_tile_type_window()
{
	try
	{
		// The object window
		this->lm_tile_type = new c_mb_list_model();
		this->lb_tile_type = new c_mb_list_box(this->lm_tile_type);

		this->lm_tile_type->add_element("Two Faced Ground");
		this->lm_tile_type->add_element("Two Faced Ground");
		this->lm_tile_type->add_element("Four Faced Ground");
		this->lm_tile_type->add_element("Eight Faced Ground");
		this->lm_tile_type->add_element("Ten Face Pillar");
		this->lm_tile_type->add_element("Eighteen Faced Pillar");
		this->lm_tile_type->add_element("Blank");
		this->lm_tile_type->add_element("Blank");
		this->lm_tile_type->add_element("Six Faced Wall (WE)");
		this->lm_tile_type->add_element("Six Faced Wall (NS)");
		this->lm_tile_type->add_element("Blank");
		this->lm_tile_type->add_element("Blank");
		this->lm_tile_type->add_element("Eight Faced Wall (W)");
		this->lm_tile_type->add_element("Eight Faced Wall (N)");
		this->lm_tile_type->add_element("Eight Faced Wall (E)");
		this->lm_tile_type->add_element("Eight Faced Wall (S)");
		this->lm_tile_type->add_element("Ten Faced Wall (WS)");
		this->lm_tile_type->add_element("Ten Faced Wall (NW)");
		this->lm_tile_type->add_element("Ten Faced Wall (NE)");
		this->lm_tile_type->add_element("Ten Faced Wall (ES)");
		this->lm_tile_type->add_element("Twelve Faced Wall (WSE)");
		this->lm_tile_type->add_element("Twelve Faced Wall (NWS)");
		this->lm_tile_type->add_element("Twelve Faced Wall (ENW)");
		this->lm_tile_type->add_element("Twelve Faced Wall (SEN)");
		this->lm_tile_type->add_element("Twelve Faced Stair (WE)");
		this->lm_tile_type->add_element("Twelve Faced Stair (NS)");

		this->lb_tile_type->setSize(280, 178);

		this->sc_tile_type = new c_mb_scroll_area();
		this->sc_tile_type->setSize(298, 180);

		this->sc_tile_type->setContent(this->lb_tile_type);

		w_tile_type = new c_mb_window("Tile flags");
		w_tile_type->setSize(300, 200);
		w_tile_type->setPosition(100, 50);
		w_tile_type->setVisible(false);

		// Create the palette container
		this->w_tile_type->add(sc_tile_type); // Add container to the window
		this->top->add(this->w_tile_type);    // Add window to the GUI

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
	try
	{
		this->label_fps = new c_mb_label("FPS: ???");
		this->label_fps->setPosition(0, 0);
		this->label_fps->setSize(145, 20);

		this->label_position = new c_mb_label("Position: ???");
		this->label_position->setPosition(0, 20);
		this->label_position->setSize(145, 20);

		this->w_info = new c_mb_window("Info");
		this->w_info->add(this->label_fps);
		this->w_info->add(this->label_position);
		this->top->add(this->w_info);
		this->w_info->setDimension(gcn::Rectangle(500, 0, 150, 70));

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
//-   Create the window for loading
//---------------------------------------------------------------------
bool c_window_manager::create_load_window()
{
	try
	{
		gcn::Label* tmp_label;
		tmp_label = new c_mb_label("Filename:");
		tmp_label->setPosition(0, 0);

		tf_load = new c_mb_textfield();
		tf_load->setText("name.mod");
		tf_load->setPosition(0, 20);
		tf_load->setSize(100, 20);

		c_mb_button* button_ok;
		button_ok = new c_mb_button();
		button_ok->setCaption("OK");
		button_ok->setPosition(0, 40);
		button_ok->setSize(100, 20);
		button_ok->set_type(BUTTON_LOAD_OK);

		c_mb_button* button_cancel;
		button_cancel = new c_mb_button();
		button_cancel->setCaption("Cancel");
		button_cancel->setPosition(0, 60);
		button_cancel->setSize(100, 20);
		button_cancel->set_type(BUTTON_LOAD_CANCEL);

		this->w_load = new c_mb_window("Load file");
		this->w_load->add(tmp_label);
		this->w_load->add(tf_load);
		this->w_load->add(button_ok);
		this->w_load->add(button_cancel);
		this->top->add(this->w_load);
		this->w_load->setDimension(gcn::Rectangle(0, 0, 200, 100));
		this->w_load->setPosition(250, 0);
		this->w_load->setVisible(false);

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
//-   Create the window for saving
//---------------------------------------------------------------------
bool c_window_manager::create_save_window()
{
	try
	{
		gcn::Label* tmp_label;
		tmp_label = new c_mb_label("Save file:");
		tmp_label->setPosition(0, 0);

		tf_save = new c_mb_textfield();
		tf_save->setText("name.mpd");
		tf_save->setPosition(0, 20);
		tf_save->setSize(100, 20);

		c_mb_button* button_ok;
		button_ok = new c_mb_button();
		button_ok->setCaption("OK");
		button_ok->setPosition(0, 40);
		button_ok->setSize(100, 20);
		button_ok->set_type(BUTTON_SAVE_OK);

		c_mb_button* button_cancel;
		button_cancel = new c_mb_button();
		button_cancel->setCaption("Cancel");
		button_cancel->setPosition(0, 60);
		button_cancel->setSize(100, 20);
		button_cancel->set_type(BUTTON_SAVE_CANCEL);

		this->w_save = new c_mb_window("Save file");
		this->w_save->add(tmp_label);
		this->w_save->add(tf_save);
		this->w_save->add(button_ok);
		this->w_save->add(button_cancel);
		this->top->add(this->w_save);
		this->w_save->setDimension(gcn::Rectangle(0, 0, 200, 100));
		this->w_save->setPosition(250, 0);
		this->w_save->setVisible(false);

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


void c_window_manager::set_load_visibility(bool p_visibility)
{
	this->w_load->setVisible(p_visibility);
}


void c_window_manager::toggle_load_visibility()
{
	if (this->w_load->isVisible())
		this->w_load->setVisible(false);
	else
		this->w_load->setVisible(true);
}


void c_window_manager::set_save_visibility(bool p_visibility)
{
	this->w_save->setVisible(p_visibility);
}


void c_window_manager::toggle_save_visibility()
{
	if (this->w_save->isVisible())
		this->w_save->setVisible(false);
	else
		this->w_save->setVisible(true);
}


void c_window_manager::set_tile_flag_visibility(bool p_visibility)
{
	this->w_tile_flag->setVisible(p_visibility);
}


void c_window_manager::toggle_tile_flag_visibility()
{
	if (this->w_tile_flag->isVisible())
		this->w_tile_flag->setVisible(false);
	else
		this->w_tile_flag->setVisible(true);
}


void c_window_manager::set_tile_type_visibility(bool p_visibility)
{
	this->w_tile_type->setVisible(p_visibility);
}


void c_window_manager::toggle_tile_type_visibility()
{
	if (this->w_tile_type->isVisible())
		this->w_tile_type->setVisible(false);
	else
		this->w_tile_type->setVisible(true);
}


//---------------------------------------------------------------------
//-   Set the visibility of the texture window
//---------------------------------------------------------------------
bool c_window_manager::set_texture_visibility(bool p_visibility)
{
	if (w_texture == NULL)
	{
		cout << "WARNING: No texture window defined!" << endl;
		return false;
	}

	this->w_texture->setVisible(p_visibility);
	return true;
}


//---------------------------------------------------------------------
//-   Toggle the visibility of the texture window
//---------------------------------------------------------------------
bool c_window_manager::toggle_texture_visibility()
{
	if (w_texture == NULL)
	{
		cout << "WARNING: No texture window defined!" << endl;
		return false;
	}

	if (this->w_texture->isVisible())
		this->w_texture->setVisible(false);
	else
		this->w_texture->setVisible(true);

	return true;
}


//---------------------------------------------------------------------
//-   Set the visibility of the object window
//---------------------------------------------------------------------
bool c_window_manager::set_object_visibility(bool p_visibility)
{
	if (w_object == NULL)
	{
		cout << "WARNING: No object window defined!" << endl;
		return false;
	}

	this->w_object->setVisible(p_visibility);
	return true;
}


//---------------------------------------------------------------------
//-   Toggle the visibility of the object window
//---------------------------------------------------------------------
bool c_window_manager::toggle_object_visibility()
{
	if (w_object == NULL)
	{
		cout << "WARNING: No object window defined!" << endl;
		return false;
	}

	if (this->w_object->isVisible())
		this->w_object->setVisible(false);
	else
		this->w_object->setVisible(true);

	return true;
}


//---------------------------------------------------------------------
//-   Set the visibility of the info window
//---------------------------------------------------------------------
bool c_window_manager::set_info_visibility(bool p_visibility)
{
	if (w_info == NULL)
	{
		cout << "WARNING: No info window defined!" << endl;
		return false;
	}

	this->w_info->setVisible(p_visibility);
	return true;
}


//---------------------------------------------------------------------
//-   Toggle the visibility of the texture window
//---------------------------------------------------------------------
bool c_window_manager::toggle_info_visibility()
{
	if (w_info == NULL)
	{
		cout << "WARNING: No info window defined!" << endl;
		return false;
	}

	if (this->w_info->isVisible())
		this->w_info->setVisible(false);
	else
		this->w_info->setVisible(true);

	return true;
}


void c_mb_button::set_type(int p_type)
{
	this->type = p_type;
}

int c_mb_button::get_type()
{
	return this->type;
}


void c_mb_button::mousePressed(gcn::MouseEvent &event)
{
	switch (this->type)
	{
		case BUTTON_SAVE_OK:
			g_input_handler->do_something(ACTION_MESH_SAVE);
			break;

		case BUTTON_LOAD_OK:
			g_input_handler->do_something(ACTION_MESH_LOAD);
			break;

		case BUTTON_SAVE_CANCEL:
			g_input_handler->do_something(WINDOW_SAVE_HIDE);
			break;

		case BUTTON_LOAD_CANCEL:
			g_input_handler->do_something(WINDOW_LOAD_HIDE);
			break;
	}
}


int c_window_manager::get_selected_object()
{
	return this->obj_list_box->getSelected();
}


int c_window_manager::get_selected_tile_flag()
{
	return g_mesh->get_meshfx(this->lb_tile_flag->getSelected());
}


int c_window_manager::get_selected_tile_type()
{
	return g_mesh->get_meshfx(this->lb_tile_type->getSelected());
}


void c_window_manager::set_selected_object(int p_object)
{
	this->selected_object = p_object;
}


void c_window_manager::set_selected_tile_flag(int p_tile_flag)
{
	this->selected_tile_flag = p_tile_flag;
}


void c_window_manager::set_selected_tile_type(int p_tile_type)
{
	this->selected_tile_type = p_tile_type;
}
