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

#include "src/aurora/talkman.h"

#include "src/engines/odyssey/listbox.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotorbase/journal.h"
#include "src/engines/kotorbase/module.h"

#include "src/engines/kotor/gui/ingame/menu_jou.h"

namespace Engines {

namespace KotOR {

static Common::UString journalText(uint32_t strref, const char *fallback) {
	const Common::UString &localized = TalkMan.getString(strref);
	return localized.empty() ? fallback : localized;
}

static Common::UString worldEntryListLabel(const KotORBase::JournalWorldEntry &entry) {
	if (!entry.text.empty()) {
		const size_t newline = entry.text.findFirst('\n');
		if (newline != Common::UString::npos)
			return entry.text.substr(0, newline);
		return entry.text;
	}

	return entry.tag;
}

MenuJournal::MenuJournal(Console *console) :
		KotORBase::GUI(console),
		_category(kCategoryActive) {

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
	_questTags.clear();
	_worldTags.clear();

	const KotORBase::JournalCatalog &catalog = KotORBase::JournalCatalog::get();

	const std::map<Common::UString, uint32_t> &journal = _module->getJournal();
	for (const auto &entry : journal) {
		const bool completed = catalog.isQuestCompleted(entry.first, entry.second);

		if ((_category == kCategoryActive && completed) ||
		    (_category == kCategoryCompleted && !completed))
			continue;

		_questTags.push_back(entry.first);
		list->addItem(catalog.getQuestTitle(entry.first));
	}

	for (const auto &worldEntry : _module->getJournalWorldEntries()) {
		if (_category != kCategoryActive)
			continue;

		_worldTags.push_back(worldEntry.tag);
		list->addItem(worldEntryListLabel(worldEntry));
	}

	list->refreshItemWidgets();

	if (list->isEmpty()) {
		setWidgetText("LBL_QUESTDESC", _category == kCategoryActive ?
			journalText(395, "No active missions in your log.") :
			journalText(396, "No completed missions in your log."));
	} else {
		setWidgetText("LBL_QUESTDESC", journalText(397, "Select a mission to review your progress."));
	}
}

void MenuJournal::showQuestDescription(int index) {
	if (!_module || index < 0)
		return;

	const KotORBase::JournalCatalog &catalog = KotORBase::JournalCatalog::get();

	if (index < (int)_questTags.size()) {
		const Common::UString &quest = _questTags[index];
		const uint32_t state = _module->getJournalQuestState(quest);
		const Common::UString title = catalog.getQuestTitle(quest);
		Common::UString body = catalog.getQuestEntryText(quest, state);

		if (body.empty())
			body = journalText(398, "No journal text is available for this objective yet.");

		setWidgetText("LBL_QUESTDESC", title + "\n\n" + body);
		return;
	}

	const int worldIndex = index - static_cast<int>(_questTags.size());
	if (worldIndex >= 0 && worldIndex < (int)_worldTags.size()) {
		for (const auto &worldEntry : _module->getJournalWorldEntries()) {
			if (worldEntry.tag == _worldTags[worldIndex]) {
				const Common::UString title = worldEntryListLabel(worldEntry);
				setWidgetText("LBL_QUESTDESC", title + "\n\n" + worldEntry.text);
				return;
			}
		}
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

	if (tag == "LIST_QUESTS" || tag.beginsWith("LIST_QUESTS")) {
		Odyssey::WidgetListBox *list = dynamic_cast<Odyssey::WidgetListBox *>(&widget);
		if (!list)
			list = getListBox("LIST_QUESTS");
		if (list)
			showQuestDescription(list->getSelectedIndex());
		return;
	}

	if (tag == "BTN_EXIT") {
		_returnCode = 1;
		return;
	}
}

} // End of namespace KotOR

} // End of namespace Engines
