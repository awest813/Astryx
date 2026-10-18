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
 */

#include <algorithm>

#include "src/common/util.h"

#include "src/aurora/2dafile.h"
#include "src/aurora/2dareg.h"
#include "src/aurora/gff3file.h"
#include "src/aurora/gff3writer.h"

#include "src/engines/kotorbase/types.h"
#include "src/engines/kotorbase/reputation.h"

DECLARE_SINGLETON(Engines::KotORBase::Reputations)

namespace Engines {

namespace KotORBase {

/** KotOR 1's standard factions, by repute.2da row. */
static const char * const kBuiltInLabels[] = {
	"Player", "Hostile_1", "Friendly_1", "Hostile_2", "Friendly_2", "Neutral",
	"Insane", "Ptat_Tuskan", "Glb_Xor", "Surrender_1", "Surrender_2", "Predator",
	"Prey", "Trap", "Endar_Spire", "Rancor", "Gizka_1", "Gizka_2"
};

static bool isPartySide(int faction) {
	return faction == kFactionPlayer || faction == kFactionFriendly1 || faction == kFactionFriendly2;
}

static bool isHostileSide(int faction) {
	return faction == kFactionHostile1 || faction == kFactionHostile2;
}

Reputations::Reputations() {
}

Reputations::Pair Reputations::makePair(int a, int b) {
	return Pair(a, b);
}

void Reputations::reset() {
	_changes.clear();
}

void Reputations::useBuiltInDefaults() {
	_labels.clear();
	_defaults.clear();
	_defaultsLoaded = true;
}

void Reputations::loadDefaults(const Aurora::TwoDAFile &repute) {
	useBuiltInDefaults();

	const size_t count = repute.getRowCount();
	for (size_t i = 0; i < count; i++)
		_labels.push_back(repute.getRow(i).getString("label"));

	// The cell for factions (row, column label), or -1 when it holds nothing.
	auto cell = [&](size_t row, size_t column) -> int {
		const Common::UString &label = _labels[column];
		if (label.empty() || repute.headerToColumn(label) == Aurora::kFieldIDInvalid)
			return -1;

		const Aurora::TwoDARow &twodaRow = repute.getRow(row);
		if (twodaRow.empty(label))
			return -1;

		return CLIP<int>(twodaRow.getInt(label), 0, 100);
	};

	for (size_t a = 0; a < count; a++) {
		for (size_t b = 0; b < count; b++) {
			int value;
			if (a == b)
				value = (a == 0) ? 100 : cell(a, a);
			else if (MIN(a, b) == 0)
				value = cell(0, MAX(a, b));           // The player row.
			else
				value = cell(MAX(a, b), MIN(a, b));   // The lower triangle.

			if (value >= 0)
				_defaults[makePair(static_cast<int>(a), static_cast<int>(b))] = value;
		}
	}
}

void Reputations::ensureDefaults() {
	if (_defaultsLoaded)
		return;

	try {
		loadDefaults(TwoDAReg.get2DA("repute"));
	} catch (...) {
		warning("Reputations: no repute.2da, using built-in faction relations");
		useBuiltInDefaults();
	}
}

int Reputations::builtInDefault(int a, int b) const {
	if (a == b)
		return 100;

	if (a == kFactionInsane || b == kFactionInsane)
		return 0;

	if ((isPartySide(a) && isPartySide(b)))
		return 100;

	if ((isPartySide(a) && (isHostileSide(b) || b == kFactionTrap)) ||
	    (isPartySide(b) && (isHostileSide(a) || a == kFactionTrap)))
		return 0;

	if ((a == kFactionPredator && b == kFactionPrey) || (a == kFactionPrey && b == kFactionPredator))
		return 0;

	return 50;
}

int Reputations::getDefault(int a, int b) {
	ensureDefaults();

	auto it = _defaults.find(makePair(a, b));
	if (it != _defaults.end())
		return it->second;

	return builtInDefault(a, b);
}

int Reputations::get(int a, int b) {
	if (a < 0 || b < 0)
		return 50;

	auto it = _changes.find(makePair(a, b));
	if (it != _changes.end())
		return it->second;

	return getDefault(a, b);
}

void Reputations::set(int a, int b, int reputation) {
	if (a < 0 || b < 0)
		return;

	// Like the original, a reputation is shared by the two factions.
	reputation = CLIP<int>(reputation, 0, 100);
	_changes[makePair(a, b)] = reputation;
	_changes[makePair(b, a)] = reputation;
}

void Reputations::adjust(int a, int b, int delta) {
	set(a, b, get(a, b) + delta);
}

bool Reputations::isHostile(int a, int b) {
	return get(a, b) <= kHostileMax;
}

bool Reputations::isFriendly(int a, int b) {
	return get(a, b) >= kFriendlyMin;
}

bool Reputations::isNeutral(int a, int b) {
	return !isHostile(a, b) && !isFriendly(a, b);
}

void Reputations::save(Aurora::GFF3WriterStruct &fac) {
	ensureDefaults();

	int count = static_cast<int>(_labels.size());
	if (count == 0)
		count = static_cast<int>(ARRAYSIZE(kBuiltInLabels));
	for (const auto &change : _changes)
		count = MAX(count, MAX(change.first.first, change.first.second) + 1);

	Aurora::GFF3WriterListPtr factions = fac.addList("FactionList");
	for (int i = 0; i < count; i++) {
		Common::UString label;
		if (static_cast<size_t>(i) < _labels.size())
			label = _labels[i];
		else if (static_cast<size_t>(i) < ARRAYSIZE(kBuiltInLabels))
			label = kBuiltInLabels[i];

		Aurora::GFF3WriterStructPtr faction = factions->addStruct();
		faction->addUint16("FactionGlobal", 0);
		faction->addExoString("FactionName", label);
		faction->addUint32("FactionParentID", 0xFFFFFFFF);
	}

	// Like the original, only pairs below 100 are listed.
	Aurora::GFF3WriterListPtr reps = fac.addList("RepList");
	for (int a = 0; a < count; a++) {
		for (int b = 0; b < count; b++) {
			const int value = get(a, b);
			if (value >= 100)
				continue;

			Aurora::GFF3WriterStructPtr rep = reps->addStruct();
			rep->addUint32("FactionID1", static_cast<uint32_t>(a));
			rep->addUint32("FactionID2", static_cast<uint32_t>(b));
			rep->addUint32("FactionRep", static_cast<uint32_t>(value));
		}
	}
}

void Reputations::load(const Aurora::GFF3Struct &fac) {
	_changes.clear();

	if (!fac.hasField("FactionList") || !fac.hasField("RepList"))
		return;

	const int count = static_cast<int>(fac.getList("FactionList").size());
	for (int a = 0; a < count; a++)
		for (int b = 0; b < count; b++)
			_changes[makePair(a, b)] = 100;

	for (const Aurora::GFF3Struct *rep : fac.getList("RepList")) {
		const int a = static_cast<int>(rep->getUint("FactionID1", 0xFFFFFFFF));
		const int b = static_cast<int>(rep->getUint("FactionID2", 0xFFFFFFFF));
		if (a < 0 || b < 0)
			continue;

		_changes[makePair(a, b)] = CLIP<int>(static_cast<int>(rep->getUint("FactionRep", 100)), 0, 100);
	}
}

} // End of namespace KotORBase

} // End of namespace Engines
