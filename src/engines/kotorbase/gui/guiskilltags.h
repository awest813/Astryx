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
 *  KotOR GUI widget tag mappings for skill point displays.
 */

#ifndef ENGINES_KOTORBASE_GUI_GUISKILLTAGS_H
#define ENGINES_KOTORBASE_GUI_GUISKILLTAGS_H

#include "src/engines/kotorbase/types.h"

namespace Engines {

namespace KotORBase {

struct SkillWidgetTags {
	const char *pointTag;
	const char *plusTag;
	const char *minusTag;
	const char *legacyPointTag;
	const char *legacyPlusTag;
	const char *legacyMinusTag;
	Skill skill;
};

static const SkillWidgetTags kSkillWidgetTags[] = {
	{ "COMPUTER_USE_POINTS_BTN", "COM_PLUS_BTN", "COM_MINUS_BTN", "LBL_COMP_USE",    "BTN_COMP_USE_PLUS",    "BTN_COMP_USE_MINUS",    kSkillComputerUse },
	{ "DEMOLITIONS_POINTS_BTN",  "DEM_PLUS_BTN", "DEM_MINUS_BTN", "LBL_DEMOLITIONS", "BTN_DEMOLITIONS_PLUS", "BTN_DEMOLITIONS_MINUS", kSkillDemolitions },
	{ "STEALTH_POINTS_BTN",      "STE_PLUS_BTN", "STE_MINUS_BTN", "LBL_STEALTH",     "BTN_STEALTH_PLUS",     "BTN_STEALTH_MINUS",     kSkillStealth },
	{ "AWARENESS_POINTS_BTN",    "AWA_PLUS_BTN", "AWA_MINUS_BTN", "LBL_AWARENESS",   "BTN_AWARENESS_PLUS",   "BTN_AWARENESS_MINUS",   kSkillAwareness },
	{ "PERSUADE_POINTS_BTN",     "PER_PLUS_BTN", "PER_MINUS_BTN", "LBL_PERSUADE",    "BTN_PERSUADE_PLUS",    "BTN_PERSUADE_MINUS",    kSkillPersuade },
	{ "REPAIR_POINTS_BTN",       "REP_PLUS_BTN", "REP_MINUS_BTN", "LBL_REPAIR",      "BTN_REPAIR_PLUS",      "BTN_REPAIR_MINUS",      kSkillRepair },
	{ "SECURITY_POINTS_BTN",     "SEC_PLUS_BTN", "SEC_MINUS_BTN", "LBL_SECURITY",    "BTN_SECURITY_PLUS",    "BTN_SECURITY_MINUS",    kSkillSecurity },
	{ "TREAT_INJURY_POINTS_BTN", "TRE_PLUS_BTN", "TRE_MINUS_BTN", "LBL_TREAT_INJ",   "BTN_TREAT_INJ_PLUS",   "BTN_TREAT_INJ_MINUS",   kSkillTreatInjury },
};

static const size_t kSkillWidgetTagCount = sizeof(kSkillWidgetTags) / sizeof(kSkillWidgetTags[0]);

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_GUI_GUISKILLTAGS_H
