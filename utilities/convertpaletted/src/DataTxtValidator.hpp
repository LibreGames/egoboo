#pragma once

#include "Tool.hpp"

namespace Tools {

using namespace Standard;

/**
 * @brief Validate <c>data.txt</c> files.
 */
struct DataTxtValidator : public Editor::Tool {

public:
    /**
     * @brief Construct this tool.
     */
    DataTxtValidator();

    /**
     * @brief Destruct this tool.
     */
    virtual ~DataTxtValidator();

    /** @copydoc Tool::run */
    void run(const Vector<SharedPtr<CommandLine::Option>>& arguments) override;

    /** @copydoc Tool:getHelp */
    const String& getHelp() const override;

private:
    void validate(const String& pathname);

}; // struct DataTxtValidator

struct DataTxtValidatorFactory : Editor::ToolFactory {
    Editor::Tool *create() noexcept override {
        try {
            return new DataTxtValidator();
        } catch (...) {
            return nullptr;
        }
    }
}; // struct DataTxtValidatorFactory


} // namespace Tools
