#ifndef controls_h
#define controls_h

#include <guichan.hpp>
#include <guichan/sdl.hpp>
#include <guichan/opengl.hpp>
#include <guichan/opengl/openglsdlimageloader.hpp>

//---------------------------------------------------------------------
//-
//---------------------------------------------------------------------
class GameKeyListener : public gcn::MouseListener, gcn::KeyListener
{
	public:
		void mousePressed(gcn::MouseEvent& event)
		{
			bool gui_clicked;
			gui_clicked = true;

			if (!event.isConsumed())
			{
				// Key event was not used by the GUI.
				gui_clicked = false;
			}

			handle_mouse_events(event, gui_clicked);
		}

		void handle_mouse_events(gcn::MouseEvent&, bool);
		void handle_key_events(gcn::KeyEvent&, bool);
};


//---------------------------------------------------------------------
//-   Handle GUI events
//---------------------------------------------------------------------
void GameKeyListener::handle_mouse_events(gcn::MouseEvent& mouse_event, bool gui_clicked)
{
	cout << "Mouse event" << endl;
}


//---------------------------------------------------------------------
//-   Handle game events
//---------------------------------------------------------------------
void GameKeyListener::handle_key_events(gcn::KeyEvent &key_event, bool gui_clicked)
{
	cout << "Key event" << endl;
}

//GameKeyListener* gameKeyListener;
//top->addKeyListener(gameKeyListener);

#endif
