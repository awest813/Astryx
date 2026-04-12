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

MenuJournal::MenuJournal(Console *console) : KotORBase::GUI(console) {
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

	Odyssey::WidgetListbox *list = getListbox("LIST_QUESTS");
	if (!list)
		return;

	list->clear();

	const std::map<Common::UString, uint32_t> &journal = _module->getJournal();
	for (auto const& [quest, state] : journal) {
		// In a full implementation, we would use the quest ID to look up
		// the localized name and description from journal.gff.
		// For now, we display the quest tag as a placeholder.
		list->add(quest);
	}

	Odyssey::WidgetLabel *desc = getLabel("LBL_QUESTDESC");
	if (desc) {
		if (journal.empty())
			desc->setText("Your journal is currently empty.");
		else
			desc->setText("Select a quest to see more details.");
	}
}

void MenuJournal::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	if (tag == "LIST_QUESTS") {
		Odyssey::WidgetListBox *list = dynamic_cast<Odyssey::WidgetListBox *>(&widget);
		if (list) {
			int index = list->getSelectedIndex();
			if (index >= 0) {
				Common::UString selected = list->getItem(index);
				Odyssey::WidgetLabel *desc = getLabel("LBL_QUESTDESC");
				if (desc) {
					// In a full implementation, we'd lookup localized description
					desc->setText("Selected Mission: " + selected + "\n\nDetails of the current objective and historical logs for this mission will be displayed here.");
				}
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
