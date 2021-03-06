#include "egolib/Graphics/SDL/GraphicsSystemNew.hpp"

#include "egolib/Graphics/SDL/Display.hpp"

namespace Ego {
namespace SDL {

GraphicsSystemNew::GraphicsSystemNew()
{
    // (1) Get the video driver name.
    driverName = SDL_GetCurrentVideoDriver();
    // (2) Create the display list.
    int numVideoDisplays = SDL_GetNumVideoDisplays();
    if (numVideoDisplays < 0)
    {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "unable to get number of video displays");
    }
    for (int i = 0, n = numVideoDisplays; i < n; ++i)
    {
        displays.push_back(std::make_shared<Display>(this, i));
    }
}

GraphicsSystemNew::~GraphicsSystemNew()
{}

void GraphicsSystemNew::setCursorVisibility(bool visibility)
{
    int result = SDL_ShowCursor(visibility ? SDL_ENABLE : SDL_DISABLE);
    if (result < 0)
    {
        throw Id::EnvironmentErrorException(__FILE__, __LINE__, "SDL", std::string("SDL_ShowCursor(") + (visibility ? "SDL_ENABLE" : "SDL_DISABLE") + ") failed - reason `" + SDL_GetError() + "`");
    }
}

bool GraphicsSystemNew::getCursorVisibility() const
{
    int result = SDL_ShowCursor(SDL_QUERY);
    if (result < 0)
    {
        throw Id::EnvironmentErrorException(__FILE__, __LINE__, "SDL", std::string("SDL_GetShowCursor(SDL_Query) failed - reason `") + SDL_GetError() + "`");
    }
    return result == SDL_ENABLE;
}

void GraphicsSystemNew::update()
{
    // (1) Update the driver name.
    driverName = SDL_GetCurrentVideoDriver();
    // (2) Update the displays.
    /// @todo We need to detect if displays were added and removed.
    for (const auto& display : displays)
    {
        display->update();
    }
}

} // namespace SDL
} // namespace Ego
