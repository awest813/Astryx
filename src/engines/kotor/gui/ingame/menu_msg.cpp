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
 *  The ingame messages menu.
 */

#include "src/aurora/talkman.h"

#include "src/engines/odyssey/listbox.h"

#include "src/engines/kotorbase/module.h"

#include "src/engines/kotor/gui/ingame/menu_msg.h"

namespace Engines {

namespace KotOR {

MenuMessages::MenuMessages(Console *console) : KotORBase::GUI(console) {
	load("messages");
}

void MenuMessages::setModule(KotORBase::Module *module) {
	_module = module;
}

void MenuMessages::show() {
	KotORBase::GUI::show();
	fillMessages();
}

void MenuMessages::fillMessages() {
	Odyssey::WidgetListBox *list = getListBox("LB_MESSAGES");
	if (!list)
		list = getListBox("LIST_MESSAGES");
	if (!list)
		return;

	list->removeAllItems();

	if (_module) {
		const std::vector<Common::UString> &messages = _module->getMessages();
		for (const auto &message : messages)
			list->addItem(message);
	}

	list->refreshItemWidgets();

	if (list->isEmpty()) {
		const Common::UString &empty = TalkMan.getString(399);
		setWidgetText("LBL_MESSAGE", empty.empty() ? "No messages." : empty);
	} else {
		setWidgetText("LBL_MESSAGE", "");
	}
}

void MenuMessages::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	if (tag == "LB_MESSAGES" || tag == "LIST_MESSAGES" || tag.beginsWith("LB_MESSAGES")) {
		Odyssey::WidgetListBox *list = dynamic_cast<Odyssey::WidgetListBox *>(&widget);
		if (!list)
			list = getListBox("LB_MESSAGES");
		if (!list)
			list = getListBox("LIST_MESSAGES");
		if (list) {
			const int index = list->getSelectedIndex();
			if (index >= 0)
				setWidgetText("LBL_MESSAGE", list->getItem(index));
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
