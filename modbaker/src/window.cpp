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
	this->w_object  = NULL;
	this->w_texture = NULL;

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
		font = new gcn::ImageFont("data/rpgfont.png", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-+/():;%&``*#=[]'_");
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
	this->create_mesh_load_window();
	this->create_mesh_save_window();
	this->create_mesh_new_window();
	this->create_mod_menu_window();

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
	delete this->w_mesh_save;
	delete this->w_mesh_load;
	delete this->w_mesh_new;
	delete this->w_mod_menu;

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
//-   Create the main window
//---------------------------------------------------------------------
bool c_window_manager::create_mesh_window()
{
	try
	{
		this->c_mesh = new c_mb_container();
		this->c_mesh->setWidth(128);
		this->c_mesh->setHeight(320);

		// Add the icons
		this->add_icon(this->c_mesh, "data/icons/window-texture.png",      0,   0, WINDOW_TEXTURE_TOGGLE, "Textures");
		this->add_icon(this->c_mesh, "data/icons/window-object.png",      32,   0, WINDOW_OBJECT_TOGGLE,  "Objects");
		this->add_icon(this->c_mesh, "data/icons/window-info.png",        64,   0, WINDOW_INFO_TOGGLE,    "Info");
		this->add_icon(this->c_mesh, "data/icons/system-log-out.png",     96,   0, ACTION_EXIT,           "Quit");

		this->add_icon(this->c_mesh, "data/icons/window-tile-flag.png",    0,  32, WINDOW_TILE_FLAG_TOGGLE, "Tile flags");
		this->add_icon(this->c_mesh, "data/icons/window-tile-type.png",   32,  32, WINDOW_TILE_TYPE_TOGGLE, "Tile types");
		this->add_icon(this->c_mesh, "data/icons/window-module-menu.png", 64,  32, WINDOW_MOD_MENU_SHOW,    "Mod menu");

		this->add_icon(this->c_mesh, "data/icons/document-new.png",      0,  64, WINDOW_MESH_NEW_SHOW,  "New mesh");
		this->add_icon(this->c_mesh, "data/icons/document-open.png",    32,  64, WINDOW_MESH_LOAD_SHOW, "Load mesh");
		this->add_icon(this->c_mesh, "data/icons/document-save.png",    64,  64, WINDOW_MESH_SAVE_SHOW, "Save mesh");

		this->add_icon(this->c_mesh, "data/icons/list-add.png",          0,  96, ACTION_VERTEX_INC, "Incr. height");
		this->add_icon(this->c_mesh, "data/icons/list-remove.png",      64,  96, ACTION_VERTEX_DEC, "Decr. height");

		this->add_icon(this->c_mesh, "data/icons/tile_flags.png",       96,  64, ACTION_TOGGLE_TILEFLAGS, "Show tile flags");

		this->add_icon(this->c_mesh, "data/icons/go-up.png",            32,  96, ACTION_VERTEX_UP,       "Vertex up");
		this->add_icon(this->c_mesh, "data/icons/go-previous.png",       0, 128, ACTION_VERTEX_LEFT,     "Vertex left");
		this->add_icon(this->c_mesh, "data/icons/go-next.png",          64, 128, ACTION_VERTEX_RIGHT,    "Vertex right");
		this->add_icon(this->c_mesh, "data/icons/go-down.png",          32, 160, ACTION_VERTEX_DOWN,     "Vertex down");
		this->add_icon(this->c_mesh, "data/icons/edit-clear.png",       32, 128, ACTION_SELECTION_CLEAR, "Clear sel.");

		this->add_icon(this->c_mesh, "data/icons/weld-vertices.png",    64, 160, ACTION_WELD_VERTICES, "Weld sel.");

		this->add_icon(this->c_mesh, "data/icons/selection-tile.png",    0, 192, ACTION_SELMODE_TILE,   "Tile select");
		this->add_icon(this->c_mesh, "data/icons/selection-vertex.png", 32, 192, ACTION_SELMODE_VERTEX, "Vertex select");
		this->add_icon(this->c_mesh, "data/icons/selection-object.png", 64, 192, ACTION_SELMODE_OBJECT, "Object select");

		this->add_icon(this->c_mesh, "data/icons/tile-type-wall.png",    0, 224, ACTION_MAKE_WALL, "Make wall");
		this->add_icon(this->c_mesh, "data/icons/tile-type-floor.png",  32, 224, ACTION_MAKE_FLOOR, "Make floor");

		this->add_icon(this->c_mesh, "data/icons/list-add.png",          0, 256, ACTION_TILE_FLAG_PAINT, "Set tile flag");
		this->add_icon(this->c_mesh, "data/icons/list-add.png",         32, 256, ACTION_TILE_TYPE_PAINT, "Set tile type");

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
	string obj_showname;

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
			obj_showname = g_object_manager->get_object(i)->get_name();

			if (g_object_manager->get_object(i)->get_gor())
				obj_showname += " (GOR)";

			this->obj_list_model->add_element(obj_showname);
		}

		this->obj_list_box->setSize(380, 170);

		this->sc_object = new c_mb_scroll_area();
		this->sc_object->setSize(398, 180);

		this->sc_object->setContent(this->obj_list_box);

		w_object = new c_mb_window("Object");
		w_object->setSize(400, 200);
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
//-   Window definition for the info window
//---------------------------------------------------------------------
c_mod_txt_window::c_mod_txt_window() : c_mb_window("Module menu")
{
		// The module display name
		this->label_name = new c_mb_label("Name");
		this->label_name->setPosition(0, 0);
		this->label_name->setSize(225, 20);

		this->tf_name = new c_mb_textfield();
		this->tf_name->setText("My new");
		this->tf_name->setPosition(225, 0);
		this->tf_name->setSize(225, 20);

		// Reference module
		this->label_ref = new c_mb_label("Reference module");
		this->label_ref->setPosition(0, 20);
		this->label_ref->setSize(225, 20);

		this->tf_ref = new c_mb_textfield();
		this->tf_ref->setText("NONE");
		this->tf_ref->setPosition(225, 20);
		this->tf_ref->setSize(225, 20);

		// Required reference (IDSZ)
		this->label_req_idsz = new c_mb_label("Required IDSZ");
		this->label_req_idsz->setPosition(0, 40);
		this->label_req_idsz->setSize(225, 20);

		this->tf_req_idsz = new c_mb_textfield();
		this->tf_req_idsz->setText("[NONE]");
		this->tf_req_idsz->setPosition(225, 40);
		this->tf_req_idsz->setSize(225, 20);

		// Number of imports (1 to 4)
		this->label_num_imp = new c_mb_label("Number of imports (1-4)");
		this->label_num_imp->setPosition(0, 60);
		this->label_num_imp->setSize(225, 20);

		this->tf_num_imp = new c_mb_textfield();
		this->tf_num_imp->setText("1");
		this->tf_num_imp->setPosition(225, 60);
		this->tf_num_imp->setSize(225, 20);

		// Allow exporting
		this->label_export = new c_mb_label("Allow exporting?");
		this->label_export->setPosition(0, 80);
		this->label_export->setSize(225, 20);

		this->tf_export = new c_mb_textfield();
		this->tf_export->setText("TRUE");
		this->tf_export->setPosition(225, 80);
		this->tf_export->setSize(225, 20);

		// Minimum number of players
		this->label_pla_min = new c_mb_label("Min. players (1-4)");
		this->label_pla_min->setPosition(0, 100);
		this->label_pla_min->setSize(225, 20);

		this->tf_pla_min = new c_mb_textfield();
		this->tf_pla_min->setText("1");
		this->tf_pla_min->setPosition(225, 100);
		this->tf_pla_min->setSize(225, 20);

		// Maximum number of players
		this->label_pla_max = new c_mb_label("Max. players (1-4)");
		this->label_pla_max->setPosition(0, 120);
		this->label_pla_max->setSize(225, 20);

		this->tf_pla_max = new c_mb_textfield();
		this->tf_pla_max->setText("1");
		this->tf_pla_max->setPosition(225, 120);
		this->tf_pla_max->setSize(225, 20);

		// Allow respawning?
		this->label_respawn = new c_mb_label("Allow respawning?");
		this->label_respawn->setPosition(0, 140);
		this->label_respawn->setSize(225, 20);

		this->tf_respawn = new c_mb_textfield();
		this->tf_respawn->setText("TRUE");
		this->tf_respawn->setPosition(225, 140);
		this->tf_respawn->setSize(225, 20);

		// NOT USED
		this->label_not_used = new c_mb_label("NOT USED");
		this->label_not_used->setPosition(0, 160);
		this->label_not_used->setSize(225, 20);

		this->tf_not_used = new c_mb_textfield();
		this->tf_not_used->setText("FALSE");
		this->tf_not_used->setPosition(225, 160);
		this->tf_not_used->setSize(225, 20);

		// Level rating
		this->label_rating = new c_mb_label("Level rating");
		this->label_rating->setPosition(0, 180);
		this->label_rating->setSize(225, 20);

		this->tf_rating = new c_mb_textfield();
		this->tf_rating->setText("***");
		this->tf_rating->setPosition(225, 180);
		this->tf_rating->setSize(225, 20);

		// Module summary
		this->label_summary = new c_mb_label("Module summary");
		this->label_summary->setPosition(0, 200);
		this->label_summary->setSize(450, 20);

		this->tf_summary1 = new c_mb_textfield();
		this->tf_summary1->setText("");
		this->tf_summary1->setPosition(0, 220);
		this->tf_summary1->setSize(450, 20);

		this->tf_summary2 = new c_mb_textfield();
		this->tf_summary2->setText("");
		this->tf_summary2->setPosition(0, 240);
		this->tf_summary2->setSize(450, 20);

		this->tf_summary3 = new c_mb_textfield();
		this->tf_summary3->setText("");
		this->tf_summary3->setPosition(0, 260);
		this->tf_summary3->setSize(450, 20);

		this->tf_summary4 = new c_mb_textfield();
		this->tf_summary4->setText("");
		this->tf_summary4->setPosition(0, 280);
		this->tf_summary4->setSize(450, 20);

		this->tf_summary5 = new c_mb_textfield();
		this->tf_summary5->setText("");
		this->tf_summary5->setPosition(0, 300);
		this->tf_summary5->setSize(450, 20);

		this->tf_summary6 = new c_mb_textfield();
		this->tf_summary6->setText("");
		this->tf_summary6->setPosition(0, 320);
		this->tf_summary6->setSize(450, 20);

		this->tf_summary7 = new c_mb_textfield();
		this->tf_summary7->setText("");
		this->tf_summary7->setPosition(0, 340);
		this->tf_summary7->setSize(450, 20);

		this->tf_summary8 = new c_mb_textfield();
		this->tf_summary8->setText("");
		this->tf_summary8->setPosition(0, 360);
		this->tf_summary8->setSize(450, 20);


		this->button_ok = new c_mb_button();
		this->button_ok->setCaption("OK");
		this->button_ok->setPosition(0, 380);
		this->button_ok->setSize(225, 20);
		this->button_ok->set_type(BUTTON_MOD_MENU_OK);

		this->button_cancel = new c_mb_button();
		this->button_cancel->setCaption("Cancel");
		this->button_cancel->setPosition(225, 380);
		this->button_cancel->setSize(225, 20);
		this->button_cancel->set_type(BUTTON_MOD_MENU_CANCEL);

		this->add(this->label_name);
		this->add(this->tf_name);
		this->add(this->label_ref);
		this->add(this->tf_ref);
		this->add(this->label_req_idsz);
		this->add(this->tf_req_idsz);
		this->add(this->label_num_imp);
		this->add(this->tf_num_imp);
		this->add(this->label_export);
		this->add(this->tf_export);
		this->add(this->label_pla_min);
		this->add(this->tf_pla_min);
		this->add(this->label_pla_max);
		this->add(this->tf_pla_max);
		this->add(this->label_respawn);
		this->add(this->tf_respawn);
		this->add(this->label_not_used);
		this->add(this->tf_not_used);
		this->add(this->label_rating);
		this->add(this->tf_rating);
		this->add(this->label_summary);
		this->add(this->tf_summary1);
		this->add(this->tf_summary2);
		this->add(this->tf_summary3);
		this->add(this->tf_summary4);
		this->add(this->tf_summary5);
		this->add(this->tf_summary6);
		this->add(this->tf_summary7);
		this->add(this->tf_summary8);
		this->add(this->button_ok);
		this->add(this->button_cancel);
		this->setSize(450, 420);
		this->setPosition(250, 0);
		this->setVisible(false);
}


//---------------------------------------------------------------------
//-   Return the text of the input boxes
//---------------------------------------------------------------------
c_menu_txt* c_mod_txt_window::get_menu_txt() {
	c_menu_txt *tmp_menu_txt;
	tmp_menu_txt = new c_menu_txt();

	tmp_menu_txt->set_name(add_underscores(this->tf_name->getText()));
	tmp_menu_txt->set_reference_module(this->tf_ref->getText());
	tmp_menu_txt->set_required_idsz(this->tf_req_idsz->getText());
	tmp_menu_txt->set_number_of_imports(translate_into_uint(this->tf_num_imp->getText()));
	tmp_menu_txt->set_allow_export(translate_into_bool(this->tf_export->getText()));
	tmp_menu_txt->set_min_players(translate_into_uint(this->tf_pla_min->getText()));
	tmp_menu_txt->set_max_players(translate_into_uint(this->tf_pla_max->getText()));
	tmp_menu_txt->set_allow_respawn(translate_into_bool(this->tf_respawn->getText()));
	tmp_menu_txt->set_not_used(translate_into_bool(this->tf_not_used->getText()));
	tmp_menu_txt->set_rating(this->tf_rating->getText());

	vector<string> tmp_summary;
	tmp_summary.push_back(add_underscores(this->tf_summary1->getText()));
	tmp_summary.push_back(add_underscores(this->tf_summary2->getText()));
	tmp_summary.push_back(add_underscores(this->tf_summary3->getText()));
	tmp_summary.push_back(add_underscores(this->tf_summary4->getText()));
	tmp_summary.push_back(add_underscores(this->tf_summary5->getText()));
	tmp_summary.push_back(add_underscores(this->tf_summary6->getText()));
	tmp_summary.push_back(add_underscores(this->tf_summary7->getText()));
	tmp_summary.push_back(add_underscores(this->tf_summary8->getText()));
	tmp_menu_txt->set_summary(tmp_summary);

	return tmp_menu_txt;
}


//---------------------------------------------------------------------
//-   Set the text of the input boxes based on a c_menu_txt object
//---------------------------------------------------------------------
void c_mod_txt_window::set_menu_txt(c_menu_txt* p_menu_txt)
{
	if (p_menu_txt == NULL)
	{
		cout << "WARNING: menu_txt is NULL" << endl;
		return;
	}

	this->tf_name->setText(remove_underscores(p_menu_txt->get_name()));
	this->tf_ref->setText(p_menu_txt->get_reference_module());
	this->tf_req_idsz->setText(p_menu_txt->get_required_idsz());
	this->tf_num_imp->setText(translate_into_string(p_menu_txt->get_number_of_imports()));
	this->tf_export->setText(translate_into_egobool(p_menu_txt->get_allow_export()));
	this->tf_pla_min->setText(translate_into_string(p_menu_txt->get_min_players()));
	this->tf_pla_max->setText(translate_into_string(p_menu_txt->get_max_players()));
	this->tf_respawn->setText(translate_into_egobool(p_menu_txt->get_allow_respawn()));
	this->tf_not_used->setText(translate_into_egobool(p_menu_txt->get_not_used()));
	this->tf_rating->setText(p_menu_txt->get_rating());

	this->tf_summary1->setText(remove_underscores(p_menu_txt->get_summary()[0]));
	this->tf_summary2->setText(remove_underscores(p_menu_txt->get_summary()[1]));
	this->tf_summary3->setText(remove_underscores(p_menu_txt->get_summary()[2]));
	this->tf_summary4->setText(remove_underscores(p_menu_txt->get_summary()[3]));
	this->tf_summary5->setText(remove_underscores(p_menu_txt->get_summary()[4]));
	this->tf_summary6->setText(remove_underscores(p_menu_txt->get_summary()[5]));
	this->tf_summary7->setText(remove_underscores(p_menu_txt->get_summary()[6]));
	this->tf_summary8->setText(remove_underscores(p_menu_txt->get_summary()[7]));
}


//---------------------------------------------------------------------
//-   Create the window for the module menu
//---------------------------------------------------------------------
// TODO: Merge all create window functions into one function
bool c_window_manager::create_mod_menu_window()
{
	try
	{
		this->w_mod_menu = new c_mod_txt_window();

		this->top->add(this->w_mod_menu);

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
bool c_window_manager::create_mesh_load_window()
{
	try
	{
		gcn::Label* tmp_label;
		tmp_label = new c_mb_label("Filename:");
		tmp_label->setPosition(0, 0);

		tf_load = new c_mb_textfield();
		tf_load->setText("name.mod");
		tf_load->setPosition(0, 0);
		tf_load->setSize(100, 20);

		c_mb_button* button_ok;
		button_ok = new c_mb_button();
		button_ok->setCaption("OK");
		button_ok->setPosition(0, 20);
		button_ok->setSize(100, 20);
		button_ok->set_type(BUTTON_MESH_LOAD_OK);

		c_mb_button* button_cancel;
		button_cancel = new c_mb_button();
		button_cancel->setCaption("Cancel");
		button_cancel->setPosition(0, 40);
		button_cancel->setSize(100, 20);
		button_cancel->set_type(BUTTON_MESH_LOAD_CANCEL);

		this->w_mesh_load = new c_mb_window("Load file");
		this->w_mesh_load->add(tmp_label);
		this->w_mesh_load->add(tf_load);
		this->w_mesh_load->add(button_ok);
		this->w_mesh_load->add(button_cancel);
		this->top->add(this->w_mesh_load);
		this->w_mesh_load->setDimension(gcn::Rectangle(0, 0, 100, 80));
		this->w_mesh_load->setPosition(250, 0);
		this->w_mesh_load->setVisible(false);

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
bool c_window_manager::create_mesh_save_window()
{
	try
	{
		gcn::Label* tmp_label;
		tmp_label = new c_mb_label("Save file:");
		tmp_label->setPosition(0, 0);

		tf_save = new c_mb_textfield();
		tf_save->setText("name.mpd");
		tf_save->setPosition(0, 0);
		tf_save->setSize(100, 20);

		c_mb_button* button_ok;
		button_ok = new c_mb_button();
		button_ok->setCaption("OK");
		button_ok->setPosition(0, 20);
		button_ok->setSize(100, 20);
		button_ok->set_type(BUTTON_MESH_SAVE_OK);

		c_mb_button* button_cancel;
		button_cancel = new c_mb_button();
		button_cancel->setCaption("Cancel");
		button_cancel->setPosition(0, 40);
		button_cancel->setSize(100, 20);
		button_cancel->set_type(BUTTON_MESH_SAVE_CANCEL);

		this->w_mesh_save = new c_mb_window("Save file");
		this->w_mesh_save->add(tmp_label);
		this->w_mesh_save->add(tf_save);
		this->w_mesh_save->add(button_ok);
		this->w_mesh_save->add(button_cancel);
		this->top->add(this->w_mesh_save);
		this->w_mesh_save->setDimension(gcn::Rectangle(0, 0, 100, 80));
		this->w_mesh_save->setPosition(250, 0);
		this->w_mesh_save->setVisible(false);

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
//-   Create the window for creating a new mesh
//---------------------------------------------------------------------
bool c_window_manager::create_mesh_new_window()
{
	try
	{
		c_mb_label* label_meshname;
		c_mb_label* label_meshsize;
		c_mb_label* label_meshsize_x;

		label_meshname = new c_mb_label("Module name");
		label_meshname->setPosition(0, 0);
		label_meshname->setSize(200, 20);

		this->tf_name = new c_mb_textfield();
		this->tf_name->setText("level.mpd");
		this->tf_name->setPosition(200, 0);
		this->tf_name->setSize(100, 20);

		// Mesh size
		label_meshsize = new c_mb_label("Mesh size (width x height)");
		label_meshsize->setPosition(0, 20);
		label_meshsize->setSize(300, 20);

		label_meshsize_x = new c_mb_label("x");
		label_meshsize_x->setPosition(40, 40);
		label_meshsize_x->setSize(10, 20);

		this->tf_size_x = new c_mb_textfield();
		this->tf_size_x->setText("3");
		this->tf_size_x->setPosition(0, 40);
		this->tf_size_x->setSize(35, 20);

		this->tf_size_y = new c_mb_textfield();
		this->tf_size_y->setText("3");
		this->tf_size_y->setPosition(50, 40);
		this->tf_size_y->setSize(35, 20);

		c_mb_button* button_ok;
		button_ok = new c_mb_button();
		button_ok->setCaption("OK");
		button_ok->setPosition(100, 60);
		button_ok->setSize(100, 20);
		button_ok->set_type(BUTTON_MESH_NEW_OK);

		c_mb_button* button_cancel;
		button_cancel = new c_mb_button();
		button_cancel->setCaption("Cancel");
		button_cancel->setPosition(200, 60);
		button_cancel->setSize(100, 20);
		button_cancel->set_type(BUTTON_MESH_NEW_CANCEL);

		this->w_mesh_new = new c_mb_window("New mesh");
		this->w_mesh_new->add(label_meshname);
		this->w_mesh_new->add(label_meshsize);
		this->w_mesh_new->add(label_meshsize_x);
		this->w_mesh_new->add(this->tf_name);
		this->w_mesh_new->add(this->tf_size_x);
		this->w_mesh_new->add(this->tf_size_y);
		this->w_mesh_new->add(button_ok);
		this->w_mesh_new->add(button_cancel);
		this->top->add(this->w_mesh_new);
		this->w_mesh_new->setDimension(gcn::Rectangle(0, 0, 300, 100));
		this->w_mesh_new->setPosition(250, 0);
		this->w_mesh_new->setVisible(false);

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
//-   Return the GUI object
//---------------------------------------------------------------------
gcn::Gui* c_window_manager::get_gui()
{
	return this->gui;
}


//---------------------------------------------------------------------
//-   Set the GUI object
//---------------------------------------------------------------------
void c_window_manager::set_gui(gcn::Gui* p_gui)
{
	this->gui = p_gui;
}


//---------------------------------------------------------------------
//-   Set the visibility of a window
//---------------------------------------------------------------------
bool c_window_manager::set_visibility(string p_window_name, bool p_visibility)
{
	c_mb_window* window;
	window = get_window(p_window_name);

	if (window == NULL)
	{
		cout << "WARNING: No window '" << p_window_name << "' defined!" << endl;
		return false;
	}

	window->setVisible(p_visibility);
	return true;
}


//---------------------------------------------------------------------
//-   Toggle the visibility of a window
//---------------------------------------------------------------------
bool c_window_manager::toggle_visibility(string p_window_name)
{
	c_mb_window* window;
	window = get_window(p_window_name);

	if (window == NULL)
	{
		cout << "WARNING: No window '" << p_window_name << "' defined!" << endl;
		return false;
	}

	if (window->isVisible())
		window->setVisible(false);
	else
		window->setVisible(true);

	return true;
}


//---------------------------------------------------------------------
//-   Setter & getter for the button type
//---------------------------------------------------------------------
void c_mb_button::set_type(int p_type)
{
	this->type = p_type;
}

int c_mb_button::get_type()
{
	return this->type;
}


//---------------------------------------------------------------------
//-   Mouse input handling
//---------------------------------------------------------------------
void c_mb_button::mousePressed(gcn::MouseEvent &event)
{
	switch (this->type)
	{
		case BUTTON_MESH_SAVE_OK:
			g_input_handler->do_something(ACTION_MESH_SAVE);
			g_input_handler->do_something(WINDOW_MESH_SAVE_HIDE);
			break;

		case BUTTON_MESH_LOAD_OK:
			g_input_handler->do_something(ACTION_MESH_LOAD);
			g_input_handler->do_something(WINDOW_MESH_LOAD_HIDE);
			break;

		case BUTTON_MESH_NEW_OK:
			vect2 new_size;
			new_size.x = translate_into_uint(g_renderer->get_wm()->get_tf_size_x()->getText());
			new_size.y = translate_into_uint(g_renderer->get_wm()->get_tf_size_y()->getText());
			g_mesh->set_size(new_size);

			g_input_handler->do_something(ACTION_MESH_NEW);
			g_input_handler->do_something(WINDOW_MESH_NEW_HIDE);
			break;

		case BUTTON_MOD_MENU_OK:
			g_input_handler->do_something(ACTION_SAVE_MOD_MENU);
			g_input_handler->do_something(WINDOW_MOD_MENU_HIDE);
			break;

		case BUTTON_MESH_SAVE_CANCEL:
			g_input_handler->do_something(WINDOW_MESH_SAVE_HIDE);
			break;

		case BUTTON_MESH_LOAD_CANCEL:
			g_input_handler->do_something(WINDOW_MESH_LOAD_HIDE);
			break;

		case BUTTON_MESH_NEW_CANCEL:
			g_input_handler->do_something(WINDOW_MESH_NEW_HIDE);
			break;

		case BUTTON_MOD_MENU_CANCEL:
			g_input_handler->do_something(WINDOW_MOD_MENU_HIDE);
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
	return this->lb_tile_type->getSelected();
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


//---------------------------------------------------------------------
//-   Return a window object based on a string
//---------------------------------------------------------------------
c_mb_window* c_window_manager::get_window(string p_window)
{
	if (p_window == "texture")
		return this->w_texture;

	if (p_window == "info")
		return this->w_info;

	if (p_window == "object")
		return this->w_object;

	if (p_window == "tile_type")
		return this->w_tile_type;

	if (p_window == "tile_flag")
		return this->w_tile_flag;

	if (p_window == "minimap")
		return this->w_minimap;

	if (p_window == "config")
		return this->w_config;

	if (p_window == "palette")
		return this->w_palette;

	if (p_window == "mesh_save")
		return this->w_mesh_save;

	if (p_window == "mesh_load")
		return this->w_mesh_load;

	if (p_window == "mesh_new")
		return this->w_mesh_new;

	if (p_window == "spawn")
		return this->w_spawn;

	if (p_window == "mod_menu")
		return this->w_mod_menu;

	return NULL;
}
