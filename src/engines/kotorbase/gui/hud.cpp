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

#include <set>

#include "src/common/util.h"
#include "src/common/error.h"
#include "src/common/strutil.h"

#include "src/graphics/graphics.h"
#include "src/graphics/aurora/cursorman.h"

#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/label.h"
#include "src/engines/odyssey/progressbar.h"

#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/trigger.h"
#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/door.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/inventory.h"

#include "src/engines/kotorbase/gui/hud.h"

namespace Engines {

namespace KotORBase {

HUD::HUD(Module &module, ::Engines::Console *console) :
		GUI(console),
		_module(module),
		_hoveredCircle(new SelectionCircle()),
		_targetCircle(new SelectionCircle()),
		_grenadeReticle(new SelectionCircle()) {

	_hoveredCircle->setHovered(true);
	_targetCircle->setTarget(true);
	_grenadeReticle->setTarget(true);
}

void HUD::setMinimap(const Common::UString &UNUSED(map), int UNUSED(northAxis),
                     float UNUSED(worldPt1X), float UNUSED(worldPt1Y), float UNUSED(worldPt2X), float UNUSED(worldPt2Y),
                     float UNUSED(mapPt1X), float UNUSED(mapPt1Y), float UNUSED(mapPt2X), float UNUSED(mapPt2Y)) {
}

void HUD::setPosition(float UNUSED(x), float UNUSED(y)) {
}

void HUD::setRotation(float UNUSED(angle)) {
}

void HUD::updateMinimapExplored(const std::vector<bool> &UNUSED(explored)) {
}

void HUD::updateMinimapMapPins(const std::vector<MapPin> &UNUSED(pins)) {
}

void HUD::setReturnStrref(uint32_t UNUSED(id)) {
}

void HUD::setReturnQueryStrref(uint32_t UNUSED(id)) {
}

void HUD::setReturnEnabled(bool UNUSED(enabled)) {
}

void HUD::showContainer(Inventory &UNUSED(inv)) {
}

void HUD::setPartyLeader(Creature *UNUSED(creature)) {
}

void HUD::setPartyMember1(Creature *UNUSED(creature)) {
}

void HUD::setPartyMember2(Creature *UNUSED(creature)) {
}

Object *HUD::getHoveredObject() const {
	return _hoveredObject;
}

Object *HUD::getTargetObject() const {
	return _targetObject;
}

void HUD::setHoveredObject(Object *object) {
	_hoveredObject = object;

	if (!object || (object != _targetObject)) {
		CursorMan.set();
		return;
	}

	setCursorToTarget();
}

void HUD::setTargetObject(Object *object) {
	if (_targetObject == object)
		return;

	_targetObject = object;
	_targetDirty = true;

	setCursorToTarget();
}

void HUD::resetSelection() {
	_hoveredObject = nullptr;
	_targetObject = nullptr;
}

void HUD::updateSelection() {
	updateTargetObject();
	updateHoveredObject();
}

void HUD::updateGrenadeReticle(int screenX, int screenY) {
	if (!_grenadeReticle)
		return;

	Area *area = _module.getCurrentArea();
	if (!area) {
		_grenadeReticle->hide();
		return;
	}

	float worldX, worldY, worldZ;
	if (!area->getGroundPointAtScreen(screenX, screenY, worldX, worldY, worldZ)) {
		_grenadeReticle->hide();
		return;
	}

	float screenProjX, screenProjY, screenProjZ;
	if (!GfxMan.project(worldX, worldY, worldZ, screenProjX, screenProjY, screenProjZ) ||
	    screenProjZ < 0.0f || screenProjZ > 1.0f) {
		_grenadeReticle->hide();
		return;
	}

	_grenadeReticle->setPosition(screenProjX, screenProjY);
	_grenadeReticle->show();
}

void HUD::hideGrenadeReticle() {
	if (_grenadeReticle)
		_grenadeReticle->hide();
}

void HUD::hideSelection() {
	_hoveredCircle->hide();
	_targetCircle->hide();
	hideTargetInformation();
}

void HUD::reset() {
	hideSelection();
	resetSelection();
	_hoveredCircle->setHovered(true);
	_targetCircle->setTarget(true);

	for (ActionMenuPanel &panel : _targetPanels)
		panel.clear();
	for (ActionMenuPanel &panel : _selfPanels)
		panel.clear();

	for (size_t i = 0; i < kActionMenuTargetPanels; ++i) {
		_targetUp[i] = ArrowWidget();
		_targetDown[i] = ArrowWidget();
	}
	for (size_t i = 0; i < kActionMenuSelfPanels; ++i) {
		_selfButtons[i] = nullptr;
		_selfLabels[i] = nullptr;
		_selfIcons[i].clear();
	}

	_actionDescription = nullptr;
	_actionDescriptionBackground = nullptr;
	_actionDescriptionTime = 0.0f;
	_actionMenuRefreshTime = 0.0f;

	_targetDirty = false;

	_targetName = nullptr;
	_targetNameBackground = nullptr;
	_targetHealth = nullptr;
	_targetHealthBackground = nullptr;
	_firstTargetButton = nullptr;
	_secondTargetButton = nullptr;
	_thirdTargetButton = nullptr;
}

void HUD::init() {
	_targetName = getLabel("LBL_NAME");
	_targetNameBackground = getLabel("LBL_NAMEBG");
	_targetHealth = getProgressbar("PB_HEALTH");
	_targetHealthBackground = getLabel("LBL_HEALTHBG");
	_firstTargetButton = getButton("BTN_TARGET0");
	_secondTargetButton = getButton("BTN_TARGET1");
	_thirdTargetButton = getButton("BTN_TARGET2");

	for (size_t i = 0; i < kActionMenuTargetPanels; ++i) {
		const Common::UString n = Common::composeString(i);
		initArrow(_targetUp[i],   "BTN_TARGETUP"   + n, getTargetButton(i));
		initArrow(_targetDown[i], "BTN_TARGETDOWN" + n, getTargetButton(i));
	}

	for (size_t i = 0; i < kActionMenuSelfPanels; ++i) {
		const Common::UString n = Common::composeString(i);
		_selfButtons[i] = getButton("BTN_ACTION" + n);
		_selfLabels[i]  = getLabel("LBL_ACTION" + n);
	}

	_actionDescription = getLabel("LBL_ACTIONDESC");
	_actionDescriptionBackground = getLabel("LBL_ACTIONDESCBG");

	// Fill the menus at the next update.
	_actionMenuRefreshTime = 0.0f;
}

void HUD::callbackActive(Widget &widget) {
	handleActionMenuWidget(widget.getTag());
}

/** Does @p tag name the widget @p prefix followed by a panel index below @p count? */
static bool matchPanelTag(const Common::UString &tag, const char *prefix, size_t count, size_t &panel) {
	for (size_t i = 0; i < count; ++i) {
		if (tag == (Common::UString(prefix) + Common::composeString(i))) {
			panel = i;
			return true;
		}
	}

	return false;
}

bool HUD::handleActionMenuWidget(const Common::UString &tag) {
	size_t panel = 0;

	if (matchPanelTag(tag, "BTN_TARGETUP", kActionMenuTargetPanels, panel)) {
		cycleTargetPanel(panel, false);
		return true;
	}
	if (matchPanelTag(tag, "BTN_TARGETDOWN", kActionMenuTargetPanels, panel)) {
		cycleTargetPanel(panel, true);
		return true;
	}
	if (matchPanelTag(tag, "BTN_TARGET", kActionMenuTargetPanels, panel)) {
		useTargetAction(panel);
		return true;
	}

	if (matchPanelTag(tag, "BTN_ACTIONUP", kActionMenuSelfPanels, panel)) {
		cycleSelfPanel(panel, false);
		return true;
	}
	if (matchPanelTag(tag, "BTN_ACTIONDOWN", kActionMenuSelfPanels, panel)) {
		cycleSelfPanel(panel, true);
		return true;
	}
	if (matchPanelTag(tag, "BTN_ACTION", kActionMenuSelfPanels, panel) ||
	    matchPanelTag(tag, "LBL_ACTION", kActionMenuSelfPanels, panel)) {
		useSelfAction(panel);
		return true;
	}

	return false;
}

void HUD::updateTargetObject() {
	if (_targetObject) {
		Door *door = ObjectContainer::toDoor(_targetObject);
		if (door && !door->isUsable())
			_targetObject = nullptr;
	}

	if (!_targetObject) {
		_targetCircle->hide();
		hideTargetInformation();
		return;
	}

	_targetCircle->setHovered(_hoveredObject == _targetObject);

	float sX, sY;
	bool onScreen = _targetCircle->moveTo(_targetObject, sX, sY);
	if (onScreen) {
		_targetCircle->show();
		updateTargetInformation(_targetObject, sX, sY);
		showTargetInformation(_targetObject);
	} else {
		_targetCircle->hide();
		hideTargetInformation();
	}
}

void HUD::updateHoveredObject() {
	if (_hoveredObject) {
		Door *door = ObjectContainer::toDoor(_hoveredObject);
		if (door && !door->isUsable())
			_hoveredObject = nullptr;
	}

	if (!_hoveredObject || (_hoveredObject == _targetObject)) {
		_hoveredCircle->hide();
		return;
	}

	float _, __;
	bool onScreen = _hoveredCircle->moveTo(_hoveredObject, _, __);

	if (onScreen)
		_hoveredCircle->show();
	else
		_hoveredCircle->hide();
}

void HUD::setCursorToTarget() {
	if (!_targetObject) {
		CursorMan.set();
		return;
	}

	const Common::UString &cursor = _targetObject->getCursor();
	if (!cursor.empty())
		CursorMan.setGroup(cursor);
	else
		CursorMan.set();
}

void HUD::getTargetButtonSize(float &width, float &height) const {
	if (!_firstTargetButton)
		return;

	width = _firstTargetButton->getWidth();
	height = _firstTargetButton->getHeight();
}

float HUD::getTargetButtonsDistance() const {
	if (!_firstTargetButton || !_secondTargetButton)
		throw Common::Exception("HUD::getTargetButtonsDistance(): Buttons not initialized");

	float x1, x2, _, __;
	_firstTargetButton->getPosition(x1, _, __);
	_secondTargetButton->getPosition(x2, _, __);

	return x2 - x1;
}

void HUD::updateTargetButtons(float originX, float originY) {
	if (!_firstTargetButton || !_secondTargetButton || !_thirdTargetButton)
		return;

	float dist = getTargetButtonsDistance();

	float width = 0.0f, _;
	getTargetButtonSize(width, _);
	float halfWidth = width / 2.0f;

	float halfSelectionSize = kSelectionCircleSize / 2.0f;
	float y = originY + halfSelectionSize + 1.0f;

	_firstTargetButton->setPosition(originX - dist - halfWidth, y, -100.0f);
	_secondTargetButton->setPosition(originX - halfWidth, y, -100.0f);
	_thirdTargetButton->setPosition(originX + dist - halfWidth, y, -100.0f);

	for (size_t i = 0; i < kActionMenuTargetPanels; ++i) {
		Odyssey::WidgetButton *button = getTargetButton(i);

		const ActionMenuOption *option = _targetPanels[i].getSelected();
		setButtonIcon(button, option ? option->icon : "");

		// The arrows cycle the options; only shown when there is a choice.
		const bool cycle = _targetPanels[i].size() > 1;
		placeArrow(_targetUp[i], button, cycle);
		placeArrow(_targetDown[i], button, cycle);
	}
}

Odyssey::WidgetButton *HUD::getTargetButton(size_t panel) const {
	switch (panel) {
		case 0:
			return _firstTargetButton;
		case 1:
			return _secondTargetButton;
		case 2:
			return _thirdTargetButton;
		default:
			break;
	}

	return nullptr;
}

void HUD::initArrow(ArrowWidget &arrow, const Common::UString &tag, Odyssey::WidgetButton *button) {
	arrow = ArrowWidget();
	arrow.button = getButton(tag);
	if (!arrow.button || !button)
		return;

	// Keep the arrow where the GUI file puts it relative to its button.
	float bx, by, bz, ax, ay, az;
	button->getPosition(bx, by, bz);
	arrow.button->getPosition(ax, ay, az);

	arrow.offsetX = ax - bx;
	arrow.offsetY = ay - by;

	// An offset far off the button would be a guess of the GUI layout: put
	// the arrows over and under the button instead, as KotOR.js does.
	const float maxOffset = 2.0f * MAX(button->getWidth(), button->getHeight());
	if ((ABS(arrow.offsetX) > maxOffset) || (ABS(arrow.offsetY) > maxOffset)) {
		const bool up = tag.contains("UP");
		arrow.offsetX = (button->getWidth() - arrow.button->getWidth()) / 2.0f;
		arrow.offsetY = up ? button->getHeight() : -arrow.button->getHeight();
	}
}

void HUD::placeArrow(ArrowWidget &arrow, Odyssey::WidgetButton *button, bool visible) {
	if (!arrow.button || !button)
		return;

	if (visible) {
		float x, y, z;
		button->getPosition(x, y, z);
		arrow.button->setPosition(x + arrow.offsetX, y + arrow.offsetY, z);
		arrow.button->setInvisible(false);
		if (_visible)
			arrow.button->show();
	} else {
		arrow.button->setInvisible(true);
		arrow.button->hide();
	}
}

void HUD::showTargetButtons() {
	if (_firstTargetButton) {
		_firstTargetButton->setInvisible(false);
		_firstTargetButton->show();
	}
	if (_secondTargetButton) {
		_secondTargetButton->setInvisible(false);
		_secondTargetButton->show();
	}
	if (_thirdTargetButton) {
		_thirdTargetButton->setInvisible(false);
		_thirdTargetButton->show();
	}
}

void HUD::hideTargetButtons() {
	if (_firstTargetButton) {
		_firstTargetButton->setInvisible(true);
		_firstTargetButton->hide();
	}
	if (_secondTargetButton) {
		_secondTargetButton->setInvisible(true);
		_secondTargetButton->hide();
	}
	if (_thirdTargetButton) {
		_thirdTargetButton->setInvisible(true);
		_thirdTargetButton->hide();
	}

	for (size_t i = 0; i < kActionMenuTargetPanels; ++i) {
		placeArrow(_targetUp[i], getTargetButton(i), false);
		placeArrow(_targetDown[i], getTargetButton(i), false);
	}
}

bool HUD::hasTargetActions() const {
	for (const ActionMenuPanel &panel : _targetPanels)
		if (!panel.empty())
			return true;

	return false;
}

ActionMenuInput HUD::gatherActionMenuInput() {
	ActionMenuInput input;

	Creature *leader = _module.getPartyLeader();
	if (!leader)
		return input;

	input.leader = &leader->getCreatureInfo();

	const Item *weapon = leader->getEquipedItem(kInventorySlotRightWeapon);
	input.rangedWeapon = weapon && weapon->isRangedWeapon();

	Object *target = _targetObject;
	if (target && (target != leader)) {
		Creature *creature = ObjectContainer::toCreature(target);
		if (creature && !creature->isDead())
			input.target = target->isEnemy() ? kActionMenuTargetHostile : kActionMenuTargetFriendly;
		else
			input.target = kActionMenuTargetObject;

		for (int action : target->getPossibleActions())
			if (action == kActionOpenLock)
				input.targetCanBeUnlocked = true;

		Trigger *trigger = dynamic_cast<Trigger *>(target);
		input.targetIsMine = trigger && trigger->isTrap() && trigger->isTrapActive() &&
		                     trigger->isTrapDetected() && trigger->isTrapDisarmable();
	}

	for (const Creature::ItemPower &power : leader->getEquippedItemPowers())
		input.itemPowers.push_back(std::make_pair(power.spellId, power.item));

	// The party shares the PC's inventory.
	Creature *pc = _module.getPC();
	if (pc) {
		for (const auto &group : pc->getInventory().getItems()) {
			if (group.second.count <= 0)
				continue;

			auto described = _itemDescriptions.find(group.first);
			if (described == _itemDescriptions.end()) {
				ActionMenuItemEntry entry;
				entry.tag = group.first;
				try {
					const Item item(group.first);
					entry = describeActionMenuItem(item);
					entry.tag = group.first;
				} catch (...) {
					entry.kind = ActionMenuItemEntry::kKindNone;
				}

				described = _itemDescriptions.insert(std::make_pair(group.first, entry)).first;
			}

			if (described->second.kind != ActionMenuItemEntry::kKindNone)
				input.items.push_back(described->second);
		}
	}

	return input;
}

void HUD::refreshActionMenus() {
	const ActionMenuInput input = gatherActionMenuInput();
	const ActionMenuTalents &talents = ActionMenuTalents::get();

	const ActionMenuTargetPanels target = buildTargetActionMenu(input, talents);
	for (size_t i = 0; i < kActionMenuTargetPanels; ++i)
		_targetPanels[i].replace(target[i]);

	const ActionMenuSelfPanels self = buildSelfActionMenu(input, talents);
	for (size_t i = 0; i < kActionMenuSelfPanels; ++i)
		_selfPanels[i].replace(self[i]);

	updateSelfActionWidgets();
}

/** Action icons whose texture is missing, so they are reported only once. */
static std::set<Common::UString> missingActionIcons;

void HUD::setButtonIcon(Odyssey::WidgetButton *button, const Common::UString &icon) {
	if (!button)
		return;

	if (missingActionIcons.count(icon) > 0) {
		button->setIcon("");
		return;
	}

	try {
		button->setIcon(icon);
	} catch (Common::Exception &e) {
		warning("HUD: no action icon \"%s\": %s", icon.c_str(), e.what());
		missingActionIcons.insert(icon);
		button->setIcon("");
	}
}

void HUD::updateSelfActionWidgets() {
	for (size_t i = 0; i < kActionMenuSelfPanels; ++i) {
		const ActionMenuOption *option = _selfPanels[i].getSelected();
		Common::UString icon = option ? option->icon : "";
		if (missingActionIcons.count(icon) > 0)
			icon.clear();

		// As KotOR.js, the icon fills the panel's label, else the button shows it.
		if (!_selfLabels[i]) {
			setButtonIcon(_selfButtons[i], icon);
			continue;
		}

		if (_selfIcons[i] == icon)
			continue;

		try {
			_selfLabels[i]->setFill(icon);
			_selfIcons[i] = icon;
		} catch (Common::Exception &e) {
			warning("HUD: no action icon \"%s\": %s", icon.c_str(), e.what());
			missingActionIcons.insert(icon);
			_selfLabels[i]->setFill("");
			_selfIcons[i].clear();
		}
	}
}

void HUD::useTargetAction(size_t panel) {
	if ((panel >= kActionMenuTargetPanels) || !_targetObject)
		return;

	const ActionMenuOption *option = _targetPanels[panel].getSelected();
	if (!option)
		return;

	const ActionMenuOption chosen = *option;
	showActionDescription(&chosen);
	_module.useActionMenuOption(chosen, _targetObject);

	// Items may be gone, powers unaffordable: rebuild soon.
	_actionMenuRefreshTime = 0.0f;
}

void HUD::useSelfAction(size_t panel) {
	if (panel >= kActionMenuSelfPanels)
		return;

	const ActionMenuOption *option = _selfPanels[panel].getSelected();
	if (!option)
		return;

	const ActionMenuOption chosen = *option;
	showActionDescription(&chosen);
	_module.useActionMenuOption(chosen, nullptr);

	_actionMenuRefreshTime = 0.0f;
}

void HUD::cycleTargetPanel(size_t panel, bool forward) {
	if (panel >= kActionMenuTargetPanels)
		return;

	if (forward)
		_targetPanels[panel].next();
	else
		_targetPanels[panel].previous();

	setButtonIcon(getTargetButton(panel), _targetPanels[panel].getSelected() ?
	              _targetPanels[panel].getSelected()->icon : "");
	showActionDescription(_targetPanels[panel].getSelected());
}

void HUD::cycleSelfPanel(size_t panel, bool forward) {
	if (panel >= kActionMenuSelfPanels)
		return;

	if (forward)
		_selfPanels[panel].next();
	else
		_selfPanels[panel].previous();

	updateSelfActionWidgets();
	showActionDescription(_selfPanels[panel].getSelected());
}

void HUD::showActionDescription(const ActionMenuOption *option) {
	if (!_actionDescription || !option || option->name.empty())
		return;

	_actionDescription->setText(option->name);
	_actionDescription->setInvisible(false);
	if (_actionDescriptionBackground)
		_actionDescriptionBackground->setInvisible(false);

	if (_visible) {
		if (_actionDescriptionBackground)
			_actionDescriptionBackground->show();
		_actionDescription->show();
	}

	_actionDescriptionTime = 2.0f;
}

void HUD::hideActionDescription() {
	_actionDescriptionTime = 0.0f;

	if (_actionDescriptionBackground) {
		_actionDescriptionBackground->setInvisible(true);
		_actionDescriptionBackground->hide();
	}
	if (_actionDescription) {
		_actionDescription->setInvisible(true);
		_actionDescription->hide();
	}
}

void HUD::showTargetInformation(Object *UNUSED(object)) {
	if (_targetNameBackground) {
		_targetNameBackground->setInvisible(false);
		_targetNameBackground->show();
	}
	if (_targetName) {
		_targetName->setInvisible(false);
		_targetName->show();
	}

	if (_targetHealthBackground) {
		_targetHealthBackground->setInvisible(false);
		_targetHealthBackground->show();
	}
	if (_targetHealth) {
		_targetHealth->setInvisible(false);
		_targetHealth->show();
	}
}

void HUD::updateTargetInformation(KotORBase::Object *object, float x, float y) {
	if (_targetDirty)
		refreshActionMenus();

	const bool hasActions = hasTargetActions();

	float halfSelectionSize = kSelectionCircleSize / 2.0f;
	float elementY = y + halfSelectionSize + 1.0f;

	if (hasActions) {
		float _, targetBtnHeight = 0.0f;
		getTargetButtonSize(_, targetBtnHeight);
		elementY += targetBtnHeight + 1.0f;
	}

	if (_targetHealthBackground)
		_targetHealthBackground->setPosition(x - 100, elementY, -FLT_MAX);

	if (_targetHealth) {
		if (object->isEnemy())
			_targetHealth->setProgressFill("redfill");
		else
			_targetHealth->setProgressFill("bluefill");
		_targetHealth->setPosition(x - 100, elementY, -FLT_MAX);

		_targetHealth->setMaxValue(object->getMaxHitPoints());
		_targetHealth->setCurrentValue(object->getCurrentHitPoints());

		elementY += _targetHealth->getHeight() + 1.0f;
	}

	if (_targetNameBackground)
		_targetNameBackground->setPosition(x - 100, elementY, -100);

	if (_targetName) {
		if (object->isEnemy())
			_targetName->setTextColor(0.745098039f, 0.105882353f, 0.0f, 1.0f);
		else
			_targetName->setTextColor(0.321569f, 0.462745f, 0.917647f, 1.0f);
		_targetName->setPosition(x - 100, elementY, -FLT_MAX);
		_targetName->setText(object->getName());
	}

	if (hasActions) {
		updateTargetButtons(x, y);
		showTargetButtons();
	} else {
		hideTargetButtons();
	}

	_targetDirty = false;
}

void HUD::hideTargetInformation() {
	if (_targetNameBackground) {
		_targetNameBackground->setInvisible(true);
		_targetNameBackground->hide();
	}
	if (_targetName) {
		_targetName->setInvisible(true);
		_targetName->hide();
	}

	if (_targetHealthBackground) {
		_targetHealthBackground->setInvisible(true);
		_targetHealthBackground->hide();
	}
	if (_targetHealth) {
		_targetHealth->setInvisible(true);
		_targetHealth->hide();
	}

	hideTargetButtons();
}

void HUD::showFloatingText(Object *object, const Common::UString &text, float duration) {
	_floatingText.show(object, text, duration);
}

void HUD::notifyJournalUpdated() {
	if (!_journalNotification)
		_journalNotification = getLabel("LBL_JOURNAL_NOTIFY");

	if (_journalNotification) {
		_journalNotification->setText("Journal Updated");
		_journalNotification->setTextColor(0.0f, 0.9f, 1.0f, 1.0f);
		_journalNotification->show();
		_journalNotificationTime = 3.0f;
	}
}

void HUD::update(float dt) {
	_floatingText.update(dt);

	// The leader, its target, inventory, feats and powers change: rebuild
	// the action menus a few times a second, keeping each panel's selection.
	_actionMenuRefreshTime -= dt;
	if (_actionMenuRefreshTime <= 0.0f) {
		_actionMenuRefreshTime = 0.5f;
		refreshActionMenus();
	}

	if (_actionDescriptionTime > 0.0f) {
		_actionDescriptionTime -= dt;
		if (_actionDescriptionTime <= 0.0f)
			hideActionDescription();
	}

	if (_journalNotificationTime > 0.0f) {
		_journalNotificationTime -= dt;

		// Smooth fade-out in the last 1 second
		if (_journalNotificationTime < 1.0f)
			_journalNotification->setTextColor(0.0f, 0.9f, 1.0f, _journalNotificationTime);
		else
			_journalNotification->setTextColor(0.0f, 0.9f, 1.0f, 1.0f);

		if (_journalNotificationTime <= 0.0f)
			_journalNotification->hide();
	}
}

} // End of namespace KotORBase

} // End of namespace Engines
