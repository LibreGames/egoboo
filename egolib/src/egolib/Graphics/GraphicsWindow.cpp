#include "egolib/Graphics/GraphicsWindow.hpp"

namespace Ego {

GraphicsWindow::GraphicsWindow(const WindowProperties& windowProperties) : window(nullptr)
{
    uint32_t flags_sdl = windowProperties.upload();
    window = SDL_CreateWindow("SDL 2.x Window",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              320, 240, flags_sdl);
    if (nullptr == window)
    {
        Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__,
                                         "unable to create SDL window: ",
                                         SDL_GetError(), Log::EndOfEntry);
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "unable to create SDL window");
    }
    displayIndex = SDL_GetWindowDisplayIndex(window);
    if (displayIndex < 0)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
        Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__,
                                         "unable to create SDL window: ",
                                         SDL_GetError(), Log::EndOfEntry);
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "unable to create SDL window");
    }
}

GraphicsWindow::~GraphicsWindow()
{
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}

std::string GraphicsWindow::getTitle() const
{
    return SDL_GetWindowTitle(window);
}

void GraphicsWindow::setTitle(const std::string& title)
{
    SDL_SetWindowTitle(window, title.c_str());
}

Size2i GraphicsWindow::getSize() const
{
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    return Size2i(width, height);
}

void GraphicsWindow::setSize(const Size2i& size)
{
    SDL_SetWindowSize(window, size.width(), size.height());
}

Point2i GraphicsWindow::getPosition() const
{
    int x, y;
    SDL_GetWindowPosition(window, &x, &y);
    return Point2i(x, y);
}

void GraphicsWindow::setPosition(const Point2i& position)
{
    SDL_SetWindowPosition(window, position.x(), position.y());
}

void GraphicsWindow::center()
{
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void GraphicsWindow::setIcon(SDL_Surface *icon)
{
#if !defined(ID_OSX)
    SDL_SetWindowIcon(window, icon);
#endif
}

Size2i GraphicsWindow::getDrawableSize() const
{
    int width, height;
#if SDL_VERSION_ATLEAST(2, 0, 1)
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_OPENGL)
    {
        SDL_GL_GetDrawableSize(window, &width, &height);
    }
    else
    {
        SDL_GetWindowSize(window, &width, &height);
    }
#else
    SDL_GetWindowSize(window, &width, &height);
#endif
    return Size2i(width, height);
}

void GraphicsWindow::update()
{
    int newDisplayIndex = SDL_GetWindowDisplayIndex(window);
    if (newDisplayIndex < 0)
    {
        Log::Entry e(Log::Level::Warning, __FILE__, __LINE__);
        e << "unable to get display index" << Log::EndOfEntry;
        Log::get() << e;
    }
    displayIndex = newDisplayIndex;
    SDL_Event event;
    int result;
    SDL_PumpEvents();
    // Process window events.
    result = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_WINDOWEVENT, SDL_WINDOWEVENT);
    while (result == 1)
    {
        switch (event.window.event)
        {
            case SDL_WINDOWEVENT_ENTER:
                this->MouseEntered(Events::WindowEventArgs(Events::WindowEventArgs::Kind::MouseEntered));
                break;
            case SDL_WINDOWEVENT_LEAVE:
                this->MouseLeft(Events::WindowEventArgs(Events::WindowEventArgs::Kind::MouseLeft));
                break;
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                this->KeyboardFocusReceived(Events::WindowEventArgs(Events::WindowEventArgs::Kind::KeyboardFocusReceived));
                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                this->KeyboardFocusLost(Events::WindowEventArgs(Events::WindowEventArgs::Kind::KeyboardFocusLost));
                break;
            case SDL_WINDOWEVENT_RESIZED:
                this->Resized(Events::WindowEventArgs(Events::WindowEventArgs::Kind::Resized));
                break;
            case SDL_WINDOWEVENT_SHOWN:
                this->Shown(Events::WindowEventArgs(Events::WindowEventArgs::Kind::Shown));
                break;
            case SDL_WINDOWEVENT_HIDDEN:
                this->Hidden(Events::WindowEventArgs(Events::WindowEventArgs::Kind::Hidden));
                break;
            case SDL_WINDOWEVENT_EXPOSED:
                break;
        }
        result = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_WINDOWEVENT, SDL_WINDOWEVENT);
    }
    if (result < 0)
    {
        /* @todo Emit a warning. */
    }
}

int GraphicsWindow::getDisplayIndex() const
{
    return displayIndex;
}

void GraphicsWindow::setGrabEnabled(bool enabled)
{
    SDL_SetWindowGrab(window, enabled ? SDL_TRUE : SDL_FALSE);
}

bool GraphicsWindow::isGrabEnabled() const
{
    return SDL_TRUE == SDL_GetWindowGrab(window);
}

WindowProperties GraphicsWindow::getProperties() const
{
    WindowProperties windowProperties;
    windowProperties.download(SDL_GetWindowFlags(window));
    return windowProperties;
}

SDL_Window *GraphicsWindow::get()
{
    return window;
}

} // namespace Ego
