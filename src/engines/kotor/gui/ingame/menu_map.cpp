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
 *  The ingame map menu.
 */

#include "src/aurora/talkman.h"

#include "src/graphics/graphics.h"

#include "src/engines/aurora/widget.h"

#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/module.h"

#include "src/engines/kotor/gui/ingame/menu_map.h"
#include "src/engines/kotor/gui/ingame/minimap.h"

#include "src/engines/kotor/gui/dialogs/confirm.h"

namespace Engines {

namespace KotOR {

MenuMap::MenuMap(Console *console) : KotORBase::GUI(console) {
	load("map");
}

void MenuMap::setModule(KotORBase::Module *module) {
	_module = module;
}

void MenuMap::setReturnStrref(uint32_t id) {
	Odyssey::WidgetButton *btnReturn = getButton("BTN_RETURN");
	if (btnReturn)
		btnReturn->setText(TalkMan.getString(id));
}

void MenuMap::setReturnQueryStrref(uint32_t id) {
	_returnQueryMessage = TalkMan.getString(id);
}

void MenuMap::setReturnEnabled(bool enabled) {
	Odyssey::WidgetButton *btnReturn = getButton("BTN_RETURN");
	if (btnReturn)
		btnReturn->setDisabled(!enabled);
}

void MenuMap::show() {
	KotORBase::GUI::show();
	refreshAreaMap();
}

void MenuMap::refreshAreaMap() {
	if (!_module)
		return;

	KotORBase::Area *area = _module->getCurrentArea();
	if (!area)
		return;

	Odyssey::WidgetLabel *mapView = getLabel("LBL_MAPVIEW");
	if (!mapView)
		mapView = getLabel("LBL_MAPAREA");
	if (!mapView)
		return;

	float mapPt1X, mapPt1Y, mapPt2X, mapPt2Y;
	area->getMapPoint1(mapPt1X, mapPt1Y);
	area->getMapPoint2(mapPt2X, mapPt2Y);

	float worldPt1X, worldPt1Y, worldPt2X, worldPt2Y;
	area->getWorldPoint1(worldPt1X, worldPt1Y);
	area->getWorldPoint2(worldPt2X, worldPt2Y);

	GfxMan.lockFrame();

	_areaMap = std::make_unique<Minimap>(_module->getMinimapMapId(), area->getNorthAxis(),
	                                     mapPt1X, mapPt1Y, mapPt2X, mapPt2Y,
	                                     worldPt1X, worldPt1Y, worldPt2X, worldPt2Y);
	_areaMap->setMapExplored(area->getMapExplored());

	if (KotORBase::Creature *leader = _module->getPartyLeader()) {
		float x, y, z;
		leader->getPosition(x, y, z);
		_areaMap->setPosition(x, y);
	}

	std::vector<MinimapMapPin> pins;
	for (const auto &pin : _module->getMapPins())
		pins.push_back({ pin.worldX, pin.worldY });
	_areaMap->setMapPins(pins);

	mapView->setSubScene(_areaMap.get());

	GfxMan.unlockFrame();
}

void MenuMap::callbackActive(Widget &widget) {
	if (widget.getTag() == "BTN_RETURN") {
		ConfirmDialog dialog(_console);
		dialog.setText(_returnQueryMessage);

		sub(dialog, kStartCodeNone, true, false);

		if (dialog.getAccepted() && _module) {
			_module->playMovie("Hyperspace");
			_module->load(_module->getReturnDestinationModule());
			_returnCode = 2;
		}
		return;
	}

	if (widget.getTag() == "BTN_EXIT") {
		_returnCode = 1;
		return;
	}
}

} // End of namespace KotOR

} // End of namespace Engines
