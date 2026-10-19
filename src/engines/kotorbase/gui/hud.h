/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * xoreos is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * xoreos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xoreos. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Base in-game HUD for KotOR games.
 */

#ifndef ENGINES_KOTORBASE_GUI_HUD_H
#define ENGINES_KOTORBASE_GUI_HUD_H

#include <map>
#include <vector>

#include "src/engines/kotorbase/types.h"
#include "src/engines/kotorbase/actionmenu.h"

#include "src/engines/kotorbase/gui/gui.h"
#include "src/engines/kotorbase/gui/floatingtext.h"
#include "src/engines/kotorbase/gui/selectioncircle.h"

namespace Engines {

namespace KotORBase {

class Module;
class Inventory;
class Creature;
class Object;

class HUD : public GUI {
public:
	HUD(Module &module, ::Engines::Console *console = 0);

	// Minimap

	virtual void setMinimap(const Common::UString &map, int northAxis,
	                        float worldPt1X, float worldPt1Y, float worldPt2X, float worldPt2Y,
	                        float mapPt1X, float mapPt1Y, float mapPt2X, float mapPt2Y);

	virtual void setPosition(float x, float y);
	virtual void setRotation(float angle);
	virtual void updateMinimapExplored(const std::vector<bool> &explored);
	virtual void updateMinimapMapPins(const std::vector<MapPin> &pins);

	// Return to hideout

	virtual void setReturnStrref(uint32_t id);
	virtual void setReturnQueryStrref(uint32_t id);
	virtual void setReturnEnabled(bool);

	// Container

	virtual void showContainer(Inventory &inv);

	// Party management

	virtual void setPartyLeader(Creature *creature);
	virtual void setPartyMember1(Creature *creature);
	virtual void setPartyMember2(Creature *creature);

	// Selection handling

	Object *getHoveredObject() const;
	Object *getTargetObject() const;

	void setHoveredObject(Object *object);
	void setTargetObject(Object *object);

	void resetSelection();
	void updateSelection();
	void hideSelection();

	void updateGrenadeReticle(int screenX, int screenY);
	void hideGrenadeReticle();

	virtual void notifyJournalUpdated();

	void showFloatingText(Object *object, const Common::UString &text, float duration = 6.0f);

	virtual void update(float dt);

protected:
	Module &_module;

	void reset();
	void init();
	virtual void callbackActive(Widget &widget);

	/** Handle a click on the action menus' widgets. True if @p tag is one of them. */
	bool handleActionMenuWidget(const Common::UString &tag);

private:
	// Hovered object

	Object *_hoveredObject { nullptr };
	std::unique_ptr<SelectionCircle> _hoveredCircle;

	// Target object

	Object *_targetObject { nullptr };
	std::unique_ptr<SelectionCircle> _targetCircle;
	std::unique_ptr<SelectionCircle> _grenadeReticle;
	bool _targetDirty { false };

	// Widgets for showing target information

	Odyssey::WidgetLabel       *_targetName { nullptr };
	Odyssey::WidgetLabel       *_targetNameBackground { nullptr };
	Odyssey::WidgetProgressbar *_targetHealth { nullptr };
	Odyssey::WidgetLabel       *_targetHealthBackground { nullptr };
	Odyssey::WidgetButton      *_firstTargetButton { nullptr };
	Odyssey::WidgetButton      *_secondTargetButton { nullptr };
	Odyssey::WidgetButton      *_thirdTargetButton { nullptr };

	// Action menus (KotOR.js' ActionMenuManager)

	struct ArrowWidget {
		Odyssey::WidgetButton *button { nullptr };
		float offsetX { 0.0f }; ///< Position relative to the panel's button.
		float offsetY { 0.0f };
	};

	ActionMenuTargetPanels _targetPanels;
	ActionMenuSelfPanels   _selfPanels;

	ArrowWidget _targetUp[kActionMenuTargetPanels];
	ArrowWidget _targetDown[kActionMenuTargetPanels];

	Odyssey::WidgetButton *_selfButtons[kActionMenuSelfPanels] {};
	Odyssey::WidgetLabel  *_selfLabels[kActionMenuSelfPanels] {};

	Odyssey::WidgetLabel *_actionDescription { nullptr };
	Odyssey::WidgetLabel *_actionDescriptionBackground { nullptr };
	float _actionDescriptionTime { 0.0f };

	float _actionMenuRefreshTime { 0.0f };
	std::map<Common::UString, ActionMenuItemEntry> _itemDescriptions;

	Odyssey::WidgetButton *getTargetButton(size_t panel) const;
	void initArrow(ArrowWidget &arrow, const Common::UString &tag, Odyssey::WidgetButton *button);
	void placeArrow(ArrowWidget &arrow, Odyssey::WidgetButton *button, bool visible);
	void setButtonIcon(Odyssey::WidgetButton *button, const Common::UString &icon);

	ActionMenuInput gatherActionMenuInput();
	void refreshActionMenus();
	void updateSelfActionWidgets();

	void useTargetAction(size_t panel);
	void useSelfAction(size_t panel);
	void cycleTargetPanel(size_t panel, bool forward);
	void cycleSelfPanel(size_t panel, bool forward);

	void showActionDescription(const ActionMenuOption *option);
	void hideActionDescription();

	void updateTargetObject();
	void updateHoveredObject();
	void setCursorToTarget();

	// Target buttons

	void getTargetButtonSize(float &width, float &height) const;
	float getTargetButtonsDistance() const;

	void updateTargetButtons(float originX, float originY);
	void showTargetButtons();
	void hideTargetButtons();

	bool hasTargetActions() const;

	// Target information

	void showTargetInformation(Object *object);
	void updateTargetInformation(Object *object, float x, float y);
	void hideTargetInformation();

	Odyssey::WidgetLabel *_journalNotification { nullptr };
	float                _journalNotificationTime { 0.0f };

	FloatingTextManager _floatingText;
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_GUI_HUD_H
