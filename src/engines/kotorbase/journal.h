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

#ifndef ENGINES_KOTORBASE_JOURNAL_H
#define ENGINES_KOTORBASE_JOURNAL_H

#include <map>

#include "src/common/ustring.h"

namespace Engines {

namespace KotORBase {

/** Lookup quest titles and entry text from global.jrl. */
class JournalCatalog {
public:
	static JournalCatalog &get();

	Common::UString getQuestTitle(const Common::UString &questTag) const;
	Common::UString getQuestEntryText(const Common::UString &questTag, uint32_t state) const;
	bool isQuestCompleted(const Common::UString &questTag, uint32_t state) const;

private:
	JournalCatalog();

	struct QuestEntry {
		Common::UString text;
		bool end { false };
	};

	struct QuestInfo {
		Common::UString title;
		std::map<uint32_t, QuestEntry> entries;
	};

	void load();

	bool _loaded { false };
	std::map<Common::UString, QuestInfo> _quests;
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_JOURNAL_H
