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
#include <utility>
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

namespace {

/** A script ANIMATION_* constant: the animations.2da row it plays (KotOR.js'
 *  animationConstantToAnimation, for a full creature and for a simple one),
 *  and the model animation name of that row. */
struct ScriptAnimation {
	int animation;
	int row;
	int simpleRow; ///< -1: the same as row.
	const char *name;
};

const ScriptAnimation kScriptAnimations[] = {
	{   0,   6, 256, "pause1"         }, // LOOPING_PAUSE
	{   1,   7, 257, "pause2"         }, // LOOPING_PAUSE2
	{   2,  18,  -1, "listen"         }, // LOOPING_LISTEN
	{   5,  25,  -1, "talknorm"       }, // LOOPING_TALK_NORMAL
	{   6,  27,  -1, "talkplead"      }, // LOOPING_TALK_PLEADING
	{   7,  26,  -1, "talkforce"      }, // LOOPING_TALK_FORCEFUL
	{   8,  29,  -1, "talklaugh"      }, // LOOPING_TALK_LAUGHING
	{   9,  28,  -1, "talksad"        }, // LOOPING_TALK_SAD
	{  10,  40,  -1, "getlow"         }, // LOOPING_GET_LOW
	{  11,  41,  -1, "getmid"         }, // LOOPING_GET_MID
	{  12,  14,  -1, "pausetrd"       }, // LOOPING_PAUSE_TIRED
	{  13,  15,  -1, "pausepsn"       }, // LOOPING_PAUSE_DRUNK
	{  14,  32,  -1, "flirt"          }, // LOOPING_FLIRT
	{  15,  43,  -1, "usecomp"        }, // LOOPING_USE_COMPUTER
	{  16,  53,  -1, "dance"          }, // LOOPING_DANCE
	{  17,  54,  -1, "dance1"         }, // LOOPING_DANCE1
	{  18,  74,  -1, "horror"         }, // LOOPING_HORROR
	{  20, 270,  -1, "deactivate"     }, // LOOPING_DEACTIVATE
	{  21,  77, 268, "spasm"          }, // LOOPING_SPASM
	{  22,  76,  -1, "sleep"          }, // LOOPING_SLEEP
	{  23,  79,  -1, "prone"          }, // LOOPING_PRONE
	{  24, 359,  -1, "pause3"         }, // LOOPING_PAUSE3
	{  25, 360,  -1, "weld"           }, // LOOPING_WELD
	{  26,  81, 275, "dead"           }, // LOOPING_DEAD
	{  27, 370,  -1, "talkinj"        }, // LOOPING_TALK_INJURED
	{  28, 371,  -1, "listeninj"      }, // LOOPING_LISTEN_INJURED
	{  29,  35,  -1, "treatinjlp"     }, // LOOPING_TREAT_INJURED
	{  30, 375,  -1, "deadprone"      }, // LOOPING_DEAD_PRONE
	{  31, 384,  -1, "kneeltalkangry" }, // LOOPING_KNEEL_TALK_ANGRY
	{  32, 385,  -1, "kneeltalksad"   }, // LOOPING_KNEEL_TALK_SAD
	{ 100,  11, 258, "hturnl"         }, // FIREFORGET_HEAD_TURN_LEFT
	{ 101,  10, 259, "hturnr"         }, // FIREFORGET_HEAD_TURN_RIGHT
	{ 102,  12,  -1, "pausesh"        }, // FIREFORGET_PAUSE_SCRATCH_HEAD
	{ 103,  13,  -1, "pausebrd"       }, // FIREFORGET_PAUSE_BORED
	{ 104,  16,  -1, "salute"         }, // FIREFORGET_SALUTE
	{ 105,  19,  -1, "bow"            }, // FIREFORGET_BOW
	{ 106,  31,  -1, "greeting"       }, // FIREFORGET_GREETING
	{ 107,  33, 263, "taunt"          }, // FIREFORGET_TAUNT
	{ 108,  17, 260, "victory"        }, // FIREFORGET_VICTORY1
	{ 109,  17, 260, "victory"        }, // FIREFORGET_VICTORY2
	{ 110,  17, 260, "victory"        }, // FIREFORGET_VICTORY3
	{ 112,  37,  -1, "inject"         }, // FIREFORGET_INJECT
	{ 113,  43,  -1, "usecomp"        }, // FIREFORGET_USE_COMPUTER
	{ 114,  68,  -1, "persuade"       }, // FIREFORGET_PERSUADE
	{ 115,  38,  -1, "activate"       }, // FIREFORGET_ACTIVATE
	{ 116,  72, 264, "choke"          }, // FIREFORGET_CHOKE
	{ 117,  57,  -1, "throwhigh"      }, // FIREFORGET_THROW_HIGH
	{ 118,  58,  -1, "throwlow"       }, // FIREFORGET_THROW_LOW
	{ 119, 346,  -1, "custom01"       }, // FIREFORGET_CUSTOM01
	{ 120,  34,  -1, "treatinj"       }, // FIREFORGET_TREAT_INJURED
};

/** LOOPING_READY: the combat stance of the weapon held (animations.2da rows
 *  per weapon animation number, as KotOR.js). */
const int kReadyRows[] = { 249, 92, 133, 174, 215, 223, 237, 245, 249 };

const int kScriptAnimationReady = 19;
const int kReadyRowSimple       = 278;

Common::UString getAnimationRowName(int row, const char *fallback) {
	const std::map<int, Common::UString> &table = getAnimationTable();
	if (!table.empty()) {
		auto it = table.find(row);
		return (it != table.end()) ? it->second : Common::UString();
	}

	return fallback;
}

} // End of anonymous namespace

int getDialogAnimationRow(int number) {
	static const std::pair<int, int> kDialogAnimations[] = {
		{  30,  18 }, {  35,  24 }, {  38,  25 }, {  39,  27 }, {  40,  26 }, {  41,  29 },
		{  42,  28 }, { 121,  44 }, { 127,  38 }, { 403, 462 }, { 404, 463 }, { 405, 464 },
		{ 406, 465 }, { 407, 466 }, { 408, 467 }, { 409, 468 }, { 410, 469 }, { 411, 470 },
		{ 412, 471 }, { 413, 472 }, { 424, 316 }, { 425, 317 }, { 426, 318 }, { 427, 319 },
		{ 428, 320 }, { 499, 557 }, { 500, 558 }, { 501, 559 }, { 502, 560 }, { 503, 561 },
		{ 504, 562 }, { 507, 565 }, { 508, 566 }
	};

	for (const auto &animation : kDialogAnimations)
		if (number == 10000 + animation.first)
			return animation.second;

	return -1;
}

Common::UString getScriptAnimationName(int animation, int weapon, bool simple) {
	if (animation == kScriptAnimationReady) {
		if ((weapon < 1) || (weapon > 8))
			weapon = 8;

		const Common::UString fallback = Common::String::format("g%dr1", weapon);
		return getAnimationRowName(simple ? kReadyRowSimple : kReadyRows[weapon], fallback.c_str());
	}

	for (const ScriptAnimation &anim : kScriptAnimations) {
		if (anim.animation != animation)
			continue;

		const int row = (simple && (anim.simpleRow >= 0)) ? anim.simpleRow : anim.row;
		return getAnimationRowName(row, anim.name);
	}

	return "";
}

bool isFireAndForgetScriptAnimation(int animation) {
	return (animation >= 100) && (animation < 200);
}

PlayAnimationTiming getPlayAnimationTiming(int animation, float duration, float speed,
                                           float animationLength) {
	PlayAnimationTiming timing;

	if (isFireAndForgetScriptAnimation(animation) || (duration == 0.0f)) {
		timing.modelLength = 0.0f;
		timing.wait = (speed > 0.0f) ? (animationLength / speed) : animationLength;
		return timing;
	}

	if (duration < 0.0f) {
		timing.modelLength = -1.0f;
		return timing;
	}

	timing.modelLength = duration;
	timing.wait = duration;
	timing.backToDefault = true;
	return timing;
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
