#include "CharacterWindow.hpp"
#include "game/GUI/Label.hpp"
#include "game/GUI/Image.hpp"
#include "game/Entities/_Include.hpp"

namespace Ego
{
namespace GUI
{

static const int LINE_SPACING_OFFSET = 5; //To make space between lines less

CharacterWindow::CharacterWindow(const std::shared_ptr<Object> &object) : InternalWindow(object->getName()),
    _character(object)
{
    setSize(320, 256);

    // draw the character's main icon
    std::shared_ptr<Image> characterIcon = std::make_shared<Image>(TextureManager::get().get_valid_ptr(_character->getProfile()->getIcon(_character->skin)));
    characterIcon->setPosition(5, 32);
    characterIcon->setSize(32, 32);
    addComponent(characterIcon);

    std::stringstream buffer;

    if(_character->isAlive())
    {
        //Level
        buffer << std::to_string(_character->getExperienceLevel());
        switch(_character->getExperienceLevel())
        {
            case 1:
                buffer << "st";
            break;

            case 2:
                buffer << "nd";
            break;

            case 3:
                buffer << "rd";
            break;

            default:
                buffer << "th";
            break;
        }
        buffer << " level ";

        //Gender
        if     (_character->getGender() == GENDER_MALE)   buffer << "male ";
        else if(_character->getGender() == GENDER_FEMALE) buffer << "female ";        
    }
    else
    {
        buffer << "Dead ";
    }

    //Class
    buffer << _character->getProfile()->getClassName();

    std::shared_ptr<Label> classLevelLabel = std::make_shared<Label>(buffer.str());
    classLevelLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    classLevelLabel->setPosition(characterIcon->getX() + characterIcon->getWidth() + 5, characterIcon->getY());
    addComponent(classLevelLabel);

    //Attribute Labels
    int maxWidth = 0;

    std::shared_ptr<Label> attributeLabel = std::make_shared<Label>("ATTRIBUTES");
    attributeLabel->setPosition(characterIcon->getX(), characterIcon->getY() + characterIcon->getHeight() + 5);
    attributeLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(attributeLabel);

    //Might = increases Life, melee damage, carry capacity, block skill, reduces movement penality for heavy armor, throw distance, brute weapon damage
    std::shared_ptr<Label> strengthLabel = std::make_shared<Label>("Strength: ");
    strengthLabel->setPosition(attributeLabel->getX(), attributeLabel->getY() + attributeLabel->getHeight()-LINE_SPACING_OFFSET);
    strengthLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(strengthLabel);
    maxWidth = std::max(maxWidth, strengthLabel->getWidth()+10);

    //Quickness = increases movement speed, attack speed, jump height, ranged aim, agile weapon damage, ranged weapon damage
    std::shared_ptr<Label> dexterityLabel = std::make_shared<Label>("Dexterity: ");
    dexterityLabel->setPosition(strengthLabel->getX(), strengthLabel->getY() + strengthLabel->getHeight()-LINE_SPACING_OFFSET);
    dexterityLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(dexterityLabel);
    maxWidth = std::max(maxWidth, dexterityLabel->getWidth()+10);

    //Spell Power = mana flow, improves spell aim, increases damage with spells
    std::shared_ptr<Label> wisdomLabel = std::make_shared<Label>("Wisdom: ");
    wisdomLabel->setPosition(dexterityLabel->getX(), dexterityLabel->getY() + dexterityLabel->getHeight()-LINE_SPACING_OFFSET);
    wisdomLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(wisdomLabel);
    maxWidth = std::max(maxWidth, wisdomLabel->getWidth()+10);

    //Intellect = increases Max Mana, identify items, detect traps, increases XP gain
    std::shared_ptr<Label> intelligenceLabel = std::make_shared<Label>("Intellect: ");
    intelligenceLabel->setPosition(wisdomLabel->getX(), wisdomLabel->getY() + wisdomLabel->getHeight()-LINE_SPACING_OFFSET);
    intelligenceLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(intelligenceLabel);
    maxWidth = std::max(maxWidth, intelligenceLabel->getWidth()+10);

    //Now attribute Values
    std::shared_ptr<Label> strengthValue = std::make_shared<Label>(std::to_string(_character->getAttribute(Ego::Attribute::MIGHT)));
    strengthValue->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    strengthValue->setPosition(strengthLabel->getX() + maxWidth, strengthLabel->getY());
    addComponent(strengthValue);

    std::shared_ptr<Label> dexterityValue = std::make_shared<Label>(std::to_string(_character->getAttribute(Ego::Attribute::AGILITY)));
    dexterityValue->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    dexterityValue->setPosition(dexterityLabel->getX() + maxWidth, dexterityLabel->getY());
    addComponent(dexterityValue);

    std::shared_ptr<Label> wisdomValue = std::make_shared<Label>(std::to_string(_character->getAttribute(Ego::Attribute::WISDOM)));
    wisdomValue->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    wisdomValue->setPosition(wisdomLabel->getX() + maxWidth, wisdomLabel->getY());
    addComponent(wisdomValue);

    std::shared_ptr<Label> intelligenceValue = std::make_shared<Label>(std::to_string(_character->getAttribute(Ego::Attribute::INTELLECT)));
    intelligenceValue->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    intelligenceValue->setPosition(intelligenceLabel->getX() + maxWidth, intelligenceLabel->getY());
    addComponent(intelligenceValue);

    //Defences
    std::shared_ptr<Label> defenceLabel = std::make_shared<Label>("DEFENCES");
    defenceLabel->setPosition(getX() + getWidth()/2, attributeLabel->getY());
    defenceLabel->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    addComponent(defenceLabel);    

    int yPos = defenceLabel->getY() + defenceLabel->getHeight() - LINE_SPACING_OFFSET;
    for(int type = 0; type < DAMAGE_COUNT; ++type) {
        yPos += addResistanceLabel(defenceLabel->getX(), yPos, static_cast<DamageType>(type));
    }
}

int CharacterWindow::addResistanceLabel(const int x, const int y, const DamageType type)
{
    //Enum to string
    std::string damageName;
    switch(type)
    {
        case DAMAGE_POKE:  damageName = "Poke"; break;
        case DAMAGE_SLASH: damageName = "Slash"; break;
        case DAMAGE_CRUSH: damageName = "Crush"; break;
        case DAMAGE_FIRE:  damageName = "Fire"; break;
        case DAMAGE_ZAP:   damageName = "Zap"; break;
        case DAMAGE_ICE:   damageName = "Ice"; break;
        case DAMAGE_EVIL:  damageName = "Evil"; break;
        case DAMAGE_HOLY:  damageName = "Holy"; break;
        default: throw Ego::Core::UnhandledSwitchCaseException(__FILE__, __LINE__);
    }

    //Label
    std::shared_ptr<Label> label = std::make_shared<Label>(damageName + ":");
    label->setPosition(x, y);
    label->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    label->setColor(Ego::Math::Colour4f(DamageType_getColour(type), 1.0f));
    addComponent(label);

    //Value
    std::shared_ptr<Label> value = std::make_shared<Label>(std::to_string(static_cast<int>(_character->getRawDamageResistance(type))));
    value->setPosition(label->getX() + 50, label->getY());
    value->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    value->setColor(Ego::Math::Colour4f(DamageType_getColour(type), 1.0f));
    addComponent(value);

    //Percent
    std::shared_ptr<Label> percent = std::make_shared<Label>("(" + std::to_string(static_cast<int>(_character->getDamageReduction(type)*100)) + "%)");
    percent->setPosition(label->getX() + 100, label->getY());
    percent->setFont(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME));
    percent->setColor(Ego::Math::Colour4f(DamageType_getColour(type), 1.0f));
    addComponent(percent);

    return label->getHeight()-LINE_SPACING_OFFSET;
}

} //GUI
} //Ego