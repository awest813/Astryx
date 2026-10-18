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
 *  Faction reputations for KotOR games.
 *
 *  Defaults come from repute.2da: row N is faction N (row 0 is the player),
 *  and the column named after faction M's label holds their reputation. The
 *  original treats reputations as shared between two factions, so only the
 *  lower triangle (row > column) and the player row are read. Changes made by
 *  scripts are saved as a FAC GFF ("repute" in SAVEGAME.sav): FactionList
 *  plus a RepList of every pair below 100.
 */

#ifndef ENGINES_KOTORBASE_REPUTATION_H
#define ENGINES_KOTORBASE_REPUTATION_H

#include <map>
#include <utility>
#include <vector>

#include "src/common/ustring.h"
#include "src/common/singleton.h"

namespace Aurora {
	class TwoDAFile;
	class GFF3Struct;
	class GFF3WriterStruct;
}

namespace Engines {

namespace KotORBase {

class Reputations : public Common::Singleton<Reputations> {
public:
	/** At or below this, factions are hostile. */
	static const int kHostileMax  = 10;
	/** At or above this, factions are friendly. */
	static const int kFriendlyMin = 90;

	Reputations();

	/** Forget all changes, back to the defaults (a new game). */
	void reset();

	/** Use this repute.2da for the defaults (instead of loading it lazily). */
	void loadDefaults(const Aurora::TwoDAFile &repute);
	/** Use the built-in defaults, as if there were no repute.2da. */
	void useBuiltInDefaults();

	/** How faction a feels about faction b, 0 - 100. */
	int get(int a, int b);
	void set(int a, int b, int reputation);
	void adjust(int a, int b, int delta);

	bool isHostile(int a, int b);
	bool isFriendly(int a, int b);
	bool isNeutral(int a, int b);

	/** Script changes, to set aside while an old module unloads. */
	typedef std::map<std::pair<int, int>, int> Changes;
	const Changes &getChanges() const { return _changes; }
	void setChanges(const Changes &changes) { _changes = changes; }

	/** Write the current reputations as a FAC GFF top-level struct. */
	void save(Aurora::GFF3WriterStruct &fac);
	/** Take every reputation from a FAC GFF (pairs not listed are 100). */
	void load(const Aurora::GFF3Struct &fac);

private:
	typedef std::pair<int, int> Pair;

	bool _defaultsLoaded { false };
	std::vector<Common::UString> _labels; ///< repute.2da labels, by faction.
	std::map<Pair, int> _defaults;
	Changes _changes;

	void ensureDefaults();
	int builtInDefault(int a, int b) const;
	int getDefault(int a, int b);

	static Pair makePair(int a, int b);
};

} // End of namespace KotORBase

} // End of namespace Engines

/** Shortcut for accessing the faction reputations. */
#define ReputationMan Engines::KotORBase::Reputations::instance()

#endif // ENGINES_KOTORBASE_REPUTATION_H
