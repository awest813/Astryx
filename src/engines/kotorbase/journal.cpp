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
 *  KotOR journal catalog backed by global.jrl when available.
 */

#include "src/common/endianness.h"
#include "src/common/util.h"

#include "src/aurora/gff3file.h"
#include "src/aurora/locstring.h"
#include "src/aurora/types.h"

#include "src/engines/kotorbase/journal.h"

namespace Engines {

namespace KotORBase {

JournalCatalog &JournalCatalog::get() {
	static JournalCatalog instance;
	return instance;
}

JournalCatalog::JournalCatalog() {
}

void JournalCatalog::load() {
	if (_loaded)
		return;

	_loaded = true;

	try {
		const Aurora::GFF3File jrl("global", Aurora::kFileTypeJRL, MKTAG('J', 'R', 'L', ' '));
		const Aurora::GFF3Struct &top = jrl.getTopLevel();

		if (!top.hasField("Categories"))
			return;

		for (const auto &category : top.getList("Categories")) {
			if (!category)
				continue;

			const Common::UString tag = category->getString("Tag");
			if (tag.empty())
				continue;

			QuestInfo quest;

			Aurora::LocString name;
			if (category->getLocString("Name", name))
				quest.title = name.getString();

			if (!category->hasField("EntryList"))
				continue;

			for (const auto &entry : category->getList("EntryList")) {
				if (!entry)
					continue;

				const uint32_t id = entry->getUint("ID");
				QuestEntry questEntry;
				questEntry.end = entry->getBool("End");

				Aurora::LocString text;
				if (entry->getLocString("Text", text))
					questEntry.text = text.getString();

				quest.entries[id] = questEntry;
			}

			_quests[tag] = quest;
		}
	} catch (...) {
		// global.jrl is optional in test/CI environments.
	}
}

Common::UString JournalCatalog::getQuestTitle(const Common::UString &questTag) const {
	const_cast<JournalCatalog *>(this)->load();

	const auto it = _quests.find(questTag);
	if (it != _quests.end() && !it->second.title.empty())
		return it->second.title;

	return questTag;
}

Common::UString JournalCatalog::getQuestEntryText(const Common::UString &questTag, uint32_t state) const {
	const_cast<JournalCatalog *>(this)->load();

	const auto it = _quests.find(questTag);
	if (it == _quests.end())
		return Common::UString();

	const auto exact = it->second.entries.find(state);
	if (exact != it->second.entries.end() && !exact->second.text.empty())
		return exact->second.text;

	// Fall back to the highest entry at or below the current state.
	const QuestEntry *best = nullptr;
	uint32_t bestId = 0;
	for (const auto &entry : it->second.entries) {
		if (entry.first <= state && entry.first >= bestId && !entry.second.text.empty()) {
			bestId = entry.first;
			best = &entry.second;
		}
	}

	return best ? best->text : Common::UString();
}

bool JournalCatalog::isQuestCompleted(const Common::UString &questTag, uint32_t state) const {
	const_cast<JournalCatalog *>(this)->load();

	const auto it = _quests.find(questTag);
	if (it == _quests.end())
		return false;

	const auto exact = it->second.entries.find(state);
	if (exact != it->second.entries.end() && exact->second.end)
		return true;

	for (const auto &entry : it->second.entries) {
		if (entry.first <= state && entry.second.end)
			return true;
	}

	return false;
}

} // End of namespace KotORBase

} // End of namespace Engines
