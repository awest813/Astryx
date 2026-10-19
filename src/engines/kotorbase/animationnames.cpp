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
 *  KotOR animation ID to model animation name mapping.
 */

#include <map>
#include <string>
#include <vector>

#include "src/common/string.h"

#include "src/aurora/2dafile.h"
#include "src/aurora/2dareg.h"

#include "src/engines/kotorbase/animationnames.h"

namespace Engines {

namespace KotORBase {

static Common::UString getHardcodedAnimationName(int animID) {
	switch (animID) {
		case  0: return "pause1";
		case  1: return "pause2";
		case  2: return "listen";
		case  3: return "meditate";
		case  4: return "worship";
		case  5: return "drunk";
		case  6: return "talk_injured";
		case  7: return "listen_injured";
		case  8: return "treatinjury";
		case  9: return "getlow";
		case 10: return "talk";
		case 11: return "talklooking";
		case 12: return "deadf";
		case 13: return "deadb";
		case 14: return "conjure1";
		case 15: return "conjure2";
		case 16: return "victory1";
		case 17: return "victory2";
		case 18: return "victory3";
		case 19: return "getmid";
		case 38: return "attack1";
		case 39: return "attack2";
		case 40: return "dodge";
		case 41: return "attack3";
		case 44: return "die";
		case 45: return "dead";
		case 48: return "g8a1";
		case 49: return "g8a2";
		case 56: return "castout";
		case 57: return "castin";
		case 58: return "castarea";
		default: return Common::UString();
	}
}

static const std::map<int, Common::UString> &getAnimationTable() {
	static std::map<int, Common::UString> table;
	static bool loaded = false;

	if (!loaded) {
		loaded = true;

		try {
			const Aurora::TwoDAFile &twoda = TwoDAReg.get2DA("animations");
			for (size_t i = 0; i < twoda.getRowCount(); ++i) {
				const Aurora::TwoDARow &row = twoda.getRow(i);
				Common::UString name = row.getString("name");
				if (name.empty())
					name = row.getString("label");
				if (!name.empty())
					table[static_cast<int>(i)] = name;
			}
		} catch (...) {
			// Fall back to the hardcoded subset when animations.2da is unavailable.
		}
	}

	return table;
}

Common::UString getAnimationNameById(int animID) {
	const std::map<int, Common::UString> &table = getAnimationTable();
	auto it = table.find(animID);
	if (it != table.end())
		return it->second;

	return getHardcodedAnimationName(animID);
}

bool isMeleeWeaponAnimation(int weapon) {
	return ((weapon >= 1) && (weapon <= 4)) || (weapon == 8);
}

Common::UString chooseAttackAnimation(int weapon, bool engaged, uint32_t roll,
                                      const HasAnimation &hasAnimation) {

	if ((weapon < 1) || (weapon > 8))
		return "";

	std::vector<Common::UString> candidates;

	if (!isMeleeWeaponAnimation(weapon)) {
		candidates.push_back(Common::String::format("b%da1", weapon));
		candidates.push_back(Common::String::format("g%da1", weapon));
	} else {
		if (engaged && (weapon <= 4))
			candidates.push_back(Common::String::format("c%da%u", weapon, 1 + (roll % 5)));

		candidates.push_back(Common::String::format("g%da%u", weapon, 1 + (roll % 2)));
		candidates.push_back(Common::String::format("g%da1", weapon));
	}

	for (const Common::UString &anim : candidates)
		if (hasAnimation(anim))
			return anim;

	return "";
}

Common::UString chooseDefenseAnimation(int weapon, const Common::UString &attack,
                                       const HasAnimation &hasAnimation) {

	if ((weapon < 1) || (weapon > 8))
		return "";

	// A dueling attack "c<weapon>a<n>" is answered by the parry "c<weapon>p<n>"
	const std::string a = attack.toString();
	if ((a.size() == 4) && (a[0] == 'c') && (a[2] == 'a') && ((a[1] - '0') == weapon)) {
		const Common::UString parry = Common::String::format("c%cp%c", a[1], a[3]);
		if (hasAnimation(parry))
			return parry;
	}

	const Common::UString dodge = Common::String::format("g%dg1", weapon);
	if (hasAnimation(dodge))
		return dodge;

	return "";
}

} // End of namespace KotORBase

} // End of namespace Engines
