#pragma once

#include "game/GUI/Component.hpp"

namespace Ego {
namespace GUI {
/// A container is a component which may contain other components.
class Container : public Component {
public:
    class ContainerIterator : public Id::NonCopyable {
    public:

        inline std::vector<std::shared_ptr<Component>>::const_iterator cbegin() const {
            return _components.cbegin();
        }

        inline std::vector<std::shared_ptr<Component>>::const_iterator cend() const {
            return _components.cend();
        }

        inline std::vector<std::shared_ptr<Component>>::iterator begin() {
            return _components.begin();
        }

        inline std::vector<std::shared_ptr<Component>>::iterator end() {
            return _components.end();
        }

        inline std::vector<std::shared_ptr<Component>>::const_reverse_iterator crbegin() const {
            return _components.crbegin();
        }

        inline std::vector<std::shared_ptr<Component>>::const_reverse_iterator crend() const {
            return _components.crend();
        }

        inline std::vector<std::shared_ptr<Component>>::reverse_iterator rbegin() {
            return _components.rbegin();
        }

        inline std::vector<std::shared_ptr<Component>>::reverse_iterator rend() {
            return _components.rend();
        }

        ~ContainerIterator() {
        }

        // Copy constructor
        ContainerIterator(const ContainerIterator &other) : NonCopyable(),
            _components(other._components) {
        }


    private:
        ContainerIterator(Container &container) :
            _components(container._components) {
        }

        //Copy of the vector contained in the Container (for thread safety)
        std::vector<std::shared_ptr<Component>> _components;

        friend class Container;
    };
    /// Construct this container.
    Container();
    /// Destruct this container.
    ~Container();
    /// Add a component to this container.
    /// @param component the component
    /// @throw Id::InvalidArgumentException @a component is a null pointer
    virtual void addComponent(const std::shared_ptr<Component>& component);
    /// Remove a component from this container.
    /// @param component the component
    /// @throw Id::InvalidArgumentException @a component is a null pointer
    virtual void removeComponent(const std::shared_ptr<Component>& component);

public:
    /// @copydoc InputListener::notifyMouseMoved
    virtual bool notifyMouseMoved(const Events::MouseMovedEventArgs& e) override;
    /// @copydoc InputListener::notifyKeyboardKeyPressed
    virtual bool notifyKeyboardKeyPressed(const Events::KeyboardKeyPressedEventArgs& ee) override;
    /// @copydoc InputListener::notifyMouseButtonReleased
    virtual bool notifyMouseButtonReleased(const Events::MouseButtonReleasedEventArgs& e) override;
    /// @copydoc InputListener::notifyMouseButtonPressed
    virtual bool notifyMouseButtonPressed(const Events::MouseButtonPressedEventArgs& e) override;
    /// @copydoc InputListener::notifyMouseWheelTurned
    virtual bool notifyMouseWheelTurned(const Events::MouseWheelTurnedEventArgs& e) override;

    /// @brief Bring a GUI component to the front of this container, so that it is drawn on top of all others and consumes input events first.
    /// @param component the component
    void bringComponentToFront(std::shared_ptr<Component> component);

    /**
    * @brief
    *  Clears and removes all GUI components from this container. GUI Components are not
    *  destroyed (i.e they can still exist in another container). This method is thread-safe.
    */
    virtual void clearComponents();
    void setComponentList(const std::vector<std::shared_ptr<Component>> &list);


    /// @brief Return a thread-safe iterator to this container.
    /// @return the iterator
    inline ContainerIterator iterator() { return ContainerIterator(*this); }

    /**
    * @return
    *   Number of GUI components currently contained within this container
    **/
    size_t getComponentCount() const;

public:
    /// @brief Renders all GUI components contained inside this container
    virtual void drawAll(DrawingContext& drawingContext);

protected:
    virtual void drawContainer(DrawingContext& drawingContext) = 0;

private:
    /// @brief The component list.
    std::vector<std::shared_ptr<Component>> _components;
    /// @brief The mutex.
    std::mutex _mutex;
};
} // namespace GUI
} // namespace Ego