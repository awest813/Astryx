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
 *  The ingame journal menu.
 */

#include "src/engines/odyssey/listbox.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotorbase/module.h"

#include "src/engines/kotor/gui/ingame/menu_jou.h"

namespace Engines {

namespace KotOR {

MenuJournal::MenuJournal(Console *console) : KotORBase::GUI(console), _category(kCategoryActive) {
	load("journal");
}

void MenuJournal::setModule(KotORBase::Module *module) {
	_module = module;
}

void MenuJournal::show() {
	KotORBase::GUI::show();
	fillJournal();
}

void MenuJournal::fillJournal() {
	if (!_module)
		return;

	Odyssey::WidgetListBox *list = getListBox("LIST_QUESTS");
	if (!list)
		return;

	list->removeAllItems();

	const std::map<Common::UString, uint32_t> &journal = _module->getJournal();
	for (auto const& [quest, state] : journal) {
		// Mock logic for completion
		bool completed = (state >= 100); 
		
		if ((_category == kCategoryActive && completed) ||
		    (_category == kCategoryCompleted && !completed))
			continue;

		list->addItem(quest);
	}

	if (list->isEmpty()) {
		setWidgetText("LBL_QUESTDESC", _category == kCategoryActive ?
			"No active missions in your log." :
			"No completed missions in your log.");
	} else {
		setWidgetText("LBL_QUESTDESC", "Select a mission to review your progress and historical data.");
	}
}

void MenuJournal::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	if (tag == "BTN_ACTIVE") {
		_category = kCategoryActive;
		fillJournal();
		return;
	}
	if (tag == "BTN_COMPLETED") {
		_category = kCategoryCompleted;
		fillJournal();
		return;
	}

	if (tag == "LIST_QUESTS") {
		Odyssey::WidgetListBox *list = dynamic_cast<Odyssey::WidgetListBox *>(&widget);
		if (list) {
			int index = list->getSelectedIndex();
			if (index >= 0) {
				Common::UString selected = list->getItem(index);
				setWidgetText("LBL_QUESTDESC",
					"Selected Mission: " + selected + "\n\nDetails of the current objective and historical logs for this mission will be displayed here.");
			}
		}
		return;
	}

	if (tag == "BTN_EXIT") {
		_returnCode = 1;
		return;
	}
}

} // End of namespace KotOR

} // End of namespace Engines
