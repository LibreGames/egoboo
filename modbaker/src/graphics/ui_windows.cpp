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
/// @brief Window implementation
/// @details Implementation of the GUI windows

#include <iostream>
#include <string>
using namespace std;

#include "../utility.h"
#include "ui_windows.h"



//---------------------------------------------------------------------
///   Window definition for the menu.txt window
//---------------------------------------------------------------------
c_mod_txt_window::c_mod_txt_window(c_mb_action_listener* p_ali) : c_mb_window("Module menu")
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
		this->button_ok->setActionEventId("mod_menu_ok");
		this->button_ok->addActionListener(p_ali);

		this->button_cancel = new c_mb_button();
		this->button_cancel->setCaption("Cancel");
		this->button_cancel->setPosition(225, 380);
		this->button_cancel->setSize(225, 20);
		this->button_cancel->setActionEventId("mod_menu_cancel");
		this->button_cancel->addActionListener(p_ali);

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
///   Window definition for the texture window
///   \param p_ali an action listener
///   \param p_dirname directory name to load the textures from
//---------------------------------------------------------------------
c_texture_window::c_texture_window(c_mb_action_listener* p_ali, string p_dirname) : c_mb_window("Texture")
{
	this->current_size = 32;
	this->t_texture = NULL;

	this->set_action_listener(p_ali);

	try
	{
		// The window
		this->setPosition(50, 0);
		this->setVisible(false);

		this->b_texsize = new c_mb_button();
		this->b_texsize->setCaption("Large");
		this->b_texsize->setPosition(0, 290);
		this->b_texsize->setSize(128, 20);
		this->b_texsize->setActionEventId("texture_large");
		this->b_texsize->addActionListener(p_ali);

		this->b_reload = new c_mb_button();
		this->b_reload->setCaption("Reload");
		this->b_reload->setPosition(128, 290);
		this->b_reload->setSize(128, 20);
		this->b_reload->setActionEventId("reload_textures");
		this->b_reload->addActionListener(p_ali);

		// Build the tabbed area
		this->build_tabbed_area(p_dirname);

//		this->t_texture->

		this->add(this->b_texsize);
		this->add(this->b_reload);
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
///   Set the action listener
///   \param p_ali action listener for the image buttons
//---------------------------------------------------------------------
void c_texture_window::set_action_listener(c_mb_action_listener *p_ali)
{
	this->m_action_listener = p_ali;
}


//---------------------------------------------------------------------
///   Reload the texture tabbed area
///   \param p_ali action listener for the image buttons
///   \param p_dirname directory to load the tiles from
//---------------------------------------------------------------------
void c_texture_window::build_tabbed_area(string p_dirname)
{
	string file;
	ostringstream tmp_str;
	int tab;

	c_mb_tab* tmp_tab;
	c_mb_container* tmp_container;

	if (this->t_texture != NULL)
		delete this->t_texture;

	try
	{
		// The tabbed area
		this->t_texture = new c_mb_tabbedarea();
		this->t_texture->setSize(256, 283);

		// Create the tabs
		for (tab=0; tab<=3; tab++)
		{
			tmp_str.str("");
			tmp_str.clear();

			tmp_str << tab;

			tmp_tab = new c_mb_tab("Tile " + tmp_str.str());
			tmp_container = new c_mb_container();
			tmp_container->setSize(256, 256);

			file = p_dirname + "tile" + tmp_str.str() + ".bmp";
			tmp_container->set_action_listener(this->m_action_listener);
			tmp_container->add_texture_set(file, tab, 0, 0, this->current_size);
			this->t_texture->addTab(tmp_tab, tmp_container);

			this->add(this->t_texture);
			// The tabbed area is ~40 pixels high
			this->setDimension(gcn::Rectangle(100, 100, 258, 330));
			tmp_tab       = NULL;
			tmp_container = NULL;
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
///   Toggle the texture size (button caption, action ID, size, ...)
//---------------------------------------------------------------------
void c_texture_window::toggle_texsize()
{
	if (this->current_size == 64)
	{
		this->b_texsize->setCaption("Small");
		this->b_texsize->setActionEventId("texture_small");
		this->current_size = 32;
	}
	else
	{
		this->b_texsize->setCaption("Large");
		this->b_texsize->setActionEventId("texture_large");
		this->current_size = 64;
	}
}


//---------------------------------------------------------------------
///   Window definition for the  window
///   \param p_ali an action listener
//---------------------------------------------------------------------
c_mesh_load_window::c_mesh_load_window(c_mb_action_listener* p_ali) : c_mb_window("Load module")
{
	gcn::Label* tmp_label;

	try
	{
		tmp_label = new c_mb_label("Filename:");
		tmp_label->setPosition(0, 0);

		tf_load = new c_mb_textfield();
		tf_load->setText("name.mod");
		tf_load->setPosition(0, 0);
		tf_load->setSize(100, 20);

		button_ok = new c_mb_button();
		button_ok->setCaption("OK");
		button_ok->setPosition(0, 20);
		button_ok->setSize(100, 20);
		button_ok->setActionEventId("load_mesh_ok");
		button_ok->addActionListener(p_ali);

		button_cancel = new c_mb_button();
		button_cancel->setCaption("Cancel");
		button_cancel->setPosition(0, 40);
		button_cancel->setSize(100, 20);
		button_cancel->setActionEventId("load_mesh_cancel");
		button_cancel->addActionListener(p_ali);

		this->add(tmp_label);
		this->add(tf_load);
		this->add(button_ok);
		this->add(button_cancel);
		this->setDimension(gcn::Rectangle(0, 0, 100, 80));
		this->setPosition(250, 0);
		this->setVisible(false);
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
///   Window definition for the  window
///   \param p_ali an action listener
//---------------------------------------------------------------------
c_info_window::c_info_window(c_mb_action_listener* p_ali) : c_mb_window("Info")
{
	try
	{
		this->label_fps = new c_mb_label("FPS: ???");
		this->label_fps->setPosition(0, 0);
		this->label_fps->setSize(145, 20);

		this->label_position_x = new c_mb_label("X Pos: ???");
		this->label_position_x->setPosition(0, 20);
		this->label_position_x->setSize(145, 20);

		this->label_position_y = new c_mb_label("Y Pos: ???");
		this->label_position_y->setPosition(0, 40);
		this->label_position_y->setSize(145, 20);

		this->add(this->label_fps);
		this->add(this->label_position_x);
		this->add(this->label_position_y);
		this->setDimension(gcn::Rectangle(500, 0, 150, 100));
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
///   Window definition for the  window
///   \param p_ali an action listener
//---------------------------------------------------------------------
c_tile_type_window::c_tile_type_window(c_mb_action_listener* p_ali) : c_mb_window("Tile type")
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


		this->setSize(300, 200);
		this->setPosition(100, 50);
		this->setVisible(false);

		this->add(sc_tile_type); // Add container to the window
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
///   Window definition for the  window
///   \param p_ali an action listener
//---------------------------------------------------------------------
c_tile_flag_window::c_tile_flag_window(c_mb_action_listener* p_ali) : c_mb_window("Tile flag")
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


		this->setSize(170, 200);
		this->setPosition(100, 0);
		this->setVisible(false);

		this->add(sc_tile_flag); // Add container to the window
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
///   Window definition for the  window
///   \param p_ali an action listener
//---------------------------------------------------------------------
c_object_window::c_object_window(c_mb_action_listener* p_ali) : c_mb_window("Object")
{
	try
	{
		// The object window
		this->obj_list_model = new c_mb_list_model();
		this->obj_list_box   = new c_mb_list_box(this->obj_list_model);

		this->obj_list_model->add_element("NONE");

		this->obj_list_box->setSize(380, 170);

		this->sc_object = new c_mb_scroll_area();
		this->sc_object->setSize(398, 180);

		this->sc_object->setContent(this->obj_list_box);

		this->setSize(400, 200);
		this->setPosition(100, 0);
		this->setVisible(false);

		// Create the palette container
		this->add(sc_object); // Add container to the window

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
///   Window definition for the  window
///   \param p_ali an action listener
//---------------------------------------------------------------------
c_palette_window::c_palette_window(c_mb_action_listener* p_ali) : c_mb_window("Palette")
{
	try
	{
		// Add the icons
		this->add_icon(p_ali, "data/icons/window-texture.png",      0,   0, WINDOW_TEXTURE_TOGGLE, "Textures");
		this->add_icon(p_ali, "data/icons/window-object.png",      32,   0, WINDOW_OBJECT_TOGGLE,  "Objects");
		this->add_icon(p_ali, "data/icons/window-info.png",        64,   0, WINDOW_INFO_TOGGLE,    "Info");
		this->add_icon(p_ali, "data/icons/system-log-out.png",     96,   0, ACTION_EXIT,           "Quit");

		this->add_icon(p_ali, "data/icons/window-tile-flag.png",    0,  32, WINDOW_TILE_FLAG_TOGGLE, "Tile flags");
		this->add_icon(p_ali, "data/icons/window-tile-type.png",   32,  32, WINDOW_TILE_TYPE_TOGGLE, "Tile types");
		this->add_icon(p_ali, "data/icons/window-module-menu.png", 64,  32, WINDOW_MOD_MENU_SHOW,    "Mod menu");

		this->add_icon(p_ali, "data/icons/document-new.png",      0,  64, WINDOW_MESH_NEW_SHOW,  "New mesh");
		this->add_icon(p_ali, "data/icons/document-open.png",    32,  64, WINDOW_MESH_LOAD_SHOW, "Load mesh");
		this->add_icon(p_ali, "data/icons/document-save.png",    64,  64, WINDOW_MESH_SAVE_SHOW, "Save mesh");

		this->add_icon(p_ali, "data/icons/list-add.png",          0,  96, ACTION_VERTEX_INC, "Incr. height");
		this->add_icon(p_ali, "data/icons/list-remove.png",      64,  96, ACTION_VERTEX_DEC, "Decr. height");

		this->add_icon(p_ali, "data/icons/tile_flags.png",       96,  64, ACTION_TOGGLE_TILEFLAGS, "Show tile flags");

		this->add_icon(p_ali, "data/icons/go-up.png",            32,  96, ACTION_VERTEX_UP,       "Vertex up");
		this->add_icon(p_ali, "data/icons/go-previous.png",       0, 128, ACTION_VERTEX_LEFT,     "Vertex left");
		this->add_icon(p_ali, "data/icons/go-next.png",          64, 128, ACTION_VERTEX_RIGHT,    "Vertex right");
		this->add_icon(p_ali, "data/icons/go-down.png",          32, 160, ACTION_VERTEX_DOWN,     "Vertex down");
		this->add_icon(p_ali, "data/icons/edit-clear.png",       32, 128, ACTION_SELECTION_CLEAR, "Clear sel.");

		this->add_icon(p_ali, "data/icons/weld-vertices.png",    64, 160, ACTION_WELD_VERTICES, "Weld sel.");

		this->add_icon(p_ali, "data/icons/selection-tile.png",    0, 192, ACTION_SELMODE_TILE,   "Tile select");
		this->add_icon(p_ali, "data/icons/selection-vertex.png", 32, 192, ACTION_SELMODE_VERTEX, "Vertex select");
		this->add_icon(p_ali, "data/icons/selection-object.png", 64, 192, ACTION_SELMODE_OBJECT, "Object select");

		this->add_icon(p_ali, "data/icons/tile-type-wall.png",    0, 224, ACTION_MAKE_WALL, "Make wall");
		this->add_icon(p_ali, "data/icons/tile-type-floor.png",  32, 224, ACTION_MAKE_FLOOR, "Make floor");

		this->add_icon(p_ali, "data/icons/list-add.png",          0, 256, ACTION_TILE_FLAG_PAINT, "Set tile flag");
		this->add_icon(p_ali, "data/icons/list-add.png",         32, 256, ACTION_TILE_TYPE_PAINT, "Set tile type");


		this->label_tooltip = new c_mb_label("Tooltip");
		this->add(this->label_tooltip);
		this->label_tooltip->setPosition(0, 300);
		this->label_tooltip->setSize(130, 20);

		this->setDimension(gcn::Rectangle(0, 0, 132, 340));
		this->setPosition(0, 0);
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
///   Window definition for the  window
///   \param p_ali an action listener
//---------------------------------------------------------------------
c_mesh_save_window::c_mesh_save_window(c_mb_action_listener* p_ali) : c_mb_window("Save mesh")
{
	gcn::Label* tmp_label;
	c_mb_button* button_ok;
	c_mb_button* button_cancel;

	try
	{
		tmp_label = new c_mb_label("Save file:");
		tmp_label->setPosition(0, 0);

		this->tf_save = new c_mb_textfield();
		this->tf_save->setText("name.mpd");
		this->tf_save->setPosition(0, 0);
		this->tf_save->setSize(100, 20);

		button_ok = new c_mb_button();
		button_ok->setCaption("OK");
		button_ok->setPosition(0, 20);
		button_ok->setSize(100, 20);
		button_ok->setActionEventId("save_mesh_ok");
		button_ok->addActionListener(p_ali);

		button_cancel = new c_mb_button();
		button_cancel->setCaption("Cancel");
		button_cancel->setPosition(0, 40);
		button_cancel->setSize(100, 20);
		button_cancel->setActionEventId("save_mesh_cancel");
		button_cancel->addActionListener(p_ali);

		this->add(tmp_label);
		this->add(tf_save);
		this->add(button_ok);
		this->add(button_cancel);
		this->setDimension(gcn::Rectangle(0, 0, 100, 80));
		this->setPosition(250, 0);
		this->setVisible(false);
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
///   Window definition for the  window
///   \param p_ali an action listener
//---------------------------------------------------------------------
c_mesh_new_window::c_mesh_new_window(c_mb_action_listener* p_ali) : c_mb_window("New mesh")
{
	c_mb_label* label_meshname;
	c_mb_label* label_meshsize;
	c_mb_label* label_meshsize_x;

	c_mb_button* button_ok;
	c_mb_button* button_cancel;

	try
	{

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

		button_ok = new c_mb_button();
		button_ok->setCaption("OK");
		button_ok->setPosition(100, 60);
		button_ok->setSize(100, 20);
		button_ok->setActionEventId("new_mesh_ok");
		button_ok->addActionListener(p_ali);

		button_cancel = new c_mb_button();
		button_cancel->setCaption("Cancel");
		button_cancel->setPosition(200, 60);
		button_cancel->setSize(100, 20);
		button_cancel->setActionEventId("new_mesh_cancel");
		button_cancel->addActionListener(p_ali);

		this->add(label_meshname);
		this->add(label_meshsize);
		this->add(label_meshsize_x);
		this->add(this->tf_name);
		this->add(this->tf_size_x);
		this->add(this->tf_size_y);
		this->add(button_ok);
		this->add(button_cancel);
		this->setDimension(gcn::Rectangle(0, 0, 300, 100));
		this->setPosition(250, 0);
		this->setVisible(false);
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
///   Get the module.txt window
///   \return window object
//---------------------------------------------------------------------
c_menu_txt* c_mod_txt_window::get_menu_txt_content() {
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
///   Set the text of the input boxes based on a c_menu_txt object
///   \param p_menu_txt Content of the menu.txt window
//---------------------------------------------------------------------
void c_mod_txt_window::set_menu_txt_content(c_menu_txt* p_menu_txt)
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
///   c_info_window destructor
//---------------------------------------------------------------------
c_info_window::~c_info_window()
{
}


//---------------------------------------------------------------------
///   c_mesh_new_window destructor
//---------------------------------------------------------------------
c_mesh_new_window::~c_mesh_new_window()
{
}


//---------------------------------------------------------------------
///   c_mesh_load_window destructor
//---------------------------------------------------------------------
c_mesh_load_window::~c_mesh_load_window()
{
}


//---------------------------------------------------------------------
///   c_mesh_save_window destructor
//---------------------------------------------------------------------
c_mesh_save_window::~c_mesh_save_window()
{
}


//---------------------------------------------------------------------
///   c_palette_window destructor
//---------------------------------------------------------------------
c_palette_window::~c_palette_window()
{
}


//---------------------------------------------------------------------
///   c_tile_flag_window destructor
//---------------------------------------------------------------------
c_tile_flag_window::~c_tile_flag_window()
{
}


//---------------------------------------------------------------------
///   c_tile_type_window destructor
//---------------------------------------------------------------------
c_tile_type_window::~c_tile_type_window()
{
}


//---------------------------------------------------------------------
///   c_texture_window destructor
//---------------------------------------------------------------------
c_texture_window::~c_texture_window()
{
}


//---------------------------------------------------------------------
///   c_object_window destructor
//---------------------------------------------------------------------
c_object_window::~c_object_window()
{
}
