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
 *  Star Wars: Knights of the Old Republic engine functions managing creatures.
 */

#include "src/common/error.h"
#include "src/common/util.h"
#include "src/engines/kotorbase/location.h"
#include "src/engines/kotorbase/effect.h"
#include "src/common/debug.h"
#include "src/common/random.h"
#include "src/common/debug.h"

#include "src/aurora/nwscript/functioncontext.h"
#include "src/aurora/2dafile.h"
#include "src/aurora/2dareg.h"

#include "src/engines/kotorbase/types.h"
#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/levelup.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/effect.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/talent.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/area.h"

#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

namespace {

Talent createInvalidTalent() {
	return Talent();
}

Talent toTalent(const TalentEntry &entry) {
	return Talent(entry.type, entry.id, entry.item);
}

} // End of anonymous namespace

void Functions::getGender(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *object = getParamObject(ctx, 0);

	Creature *creature = ObjectContainer::toCreature(object);

	if (!creature) {
		warning("Functions::getGender(): invalid creature");
		ctx.getReturn() = 0;
		return;
	}

	ctx.getReturn() = creature->getGender();
}

void Functions::getLevelByClass(Aurora::NWScript::FunctionContext &ctx) {
	Class creatureClass = Class(ctx.getParams()[0].getInt());
	Aurora::NWScript::Object *object = ctx.getParams()[1].getObject();

	Creature *creature = ObjectContainer::toCreature(object);

	if (!creature) {
		warning("Functions::getLevelByClass(): invalid creature");
		ctx.getReturn() = 0;
		return;
	}

	ctx.getReturn() = creature->getLevel(creatureClass);
}

void Functions::changeToJedi(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *object = ctx.getParams()[0].getObject();
	int jediClass = ctx.getParams()[1].getInt();

	Creature *creature = ObjectContainer::toCreature(object);

	if (!creature) {
		warning("Functions::changeToJedi(): invalid creature");
		return;
	}

	applyJediClass(*creature, static_cast<Class>(jediClass));

	debugC(Common::kDebugEngineLogic, 1, "Creature \"%s\" changed to Jedi class %d",
	       creature->getTag().c_str(), jediClass);
}

void Functions::getGoodEvilValue(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	ctx.getReturn() = creature ? creature->getAlignment() : 50;
}

void Functions::getAlignmentGoodEvil(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	if (!creature) {
		ctx.getReturn() = 2; // Neutral
		return;
	}

	int alignment = creature->getAlignment();
	if (alignment >= 70)
		ctx.getReturn() = 1; // Good
	else if (alignment <= 30)
		ctx.getReturn() = 3; // Evil
	else
		ctx.getReturn() = 2; // Neutral
}

void Functions::adjustAlignment(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	int shift = ctx.getParams()[1].getInt();
	
	if (creature)
		creature->adjustAlignment(shift);
}

void Functions::getLevelByPosition(Aurora::NWScript::FunctionContext &ctx) {
	int position = ctx.getParams()[0].getInt();
	Aurora::NWScript::Object *object = ctx.getParams()[1].getObject();

	Creature *creature = ObjectContainer::toCreature(object);

	if (!creature) {
		warning("Functions::getLevelByPosition(): invalid creature");
		ctx.getReturn() = 0;
		return;
	}

	ctx.getReturn() = creature->getLevelByPosition(position - 1);
}

void Functions::getClassByPosition(Aurora::NWScript::FunctionContext &ctx) {
	int position = ctx.getParams()[0].getInt();
	Aurora::NWScript::Object *object = ctx.getParams()[1].getObject();

	Creature *creature = ObjectContainer::toCreature(object);

	if (!creature) {
		warning("Functions::getClassByPosition(): invalid creature");
		ctx.getReturn() = 0;
		return;
	}

	ctx.getReturn() = creature->getClassByPosition(position - 1);
}

void Functions::getRacialType(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *object = ctx.getParams()[0].getObject();

	Creature *creature = ObjectContainer::toCreature(object);

	Race race;
	if (!creature)
		race = kRaceInvalid;
	else
		race = creature->getRace();

	ctx.getReturn() = race;
}

void Functions::getSubRace(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *object = ctx.getParams()[0].getObject();

	Creature *creature = ObjectContainer::toCreature(object);

	if (!creature) {
		warning("Functions::getSubRace(): invalid creature");
		ctx.getReturn() = 0;
		return;
	}

	ctx.getReturn() = creature->getSubRace();
}

void Functions::getHasFeat(Aurora::NWScript::FunctionContext &ctx) {
	const uint32_t feat = static_cast<uint32_t>(ctx.getParams()[0].getInt());
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 1));
	ctx.getReturn() = creature ? static_cast<int32_t>(creature->hasFeat(feat)) : 0;
}

void Functions::getHasSkill(Aurora::NWScript::FunctionContext &ctx) {
	int nSkill = ctx.getParams()[0].getInt();
	Aurora::NWScript::Object *object = ctx.getParams()[1].getObject();

	Creature *creature = ObjectContainer::toCreature(object);

	if (!creature) {
		warning("Functions::getHasSkill(): invalid creature");
		ctx.getReturn() = 0;
		return;
	}

	ctx.getReturn() = creature->getSkillRank(KotORBase::Skill(nSkill)) > 0;
}

void Functions::getSkillRank(Aurora::NWScript::FunctionContext &ctx) {
	int nSkill = ctx.getParams()[0].getInt();
	Aurora::NWScript::Object *object = ctx.getParams()[1].getObject();

	Creature *creature = ObjectContainer::toCreature(object);

	if (!creature) {
		warning("Functions::getSkillRank(): invalid creature");
		ctx.getReturn() = 0;
		return;
	}

	ctx.getReturn() = creature->getSkillRank(KotORBase::Skill(nSkill));
}

void Functions::getIsSkillSuccessful(Aurora::NWScript::FunctionContext &ctx) {
	// GetIsSkillSuccessful(object oTarget, int nSkill, int nDifficulty) → int
	Aurora::NWScript::Object *object = ctx.getParams()[0].getObject();
	int nSkill = ctx.getParams()[1].getInt();
	int nDC    = ctx.getParams()[2].getInt();

	Creature *creature = ObjectContainer::toCreature(object);
	if (!creature) {
		ctx.getReturn() = 0;
		return;
	}

	int rank = creature->getSkillRank(KotORBase::Skill(nSkill));
	int roll = RNG.getNext(1, 21);
	ctx.getReturn() = (roll + rank >= nDC) ? 1 : 0;

	debugC(Common::kDebugEngineLogic, 1, "Script SkillCheck: %s, Skill %d, Rank %d, Roll %d -> %s (DC %d)",
	      creature->getTag().c_str(), nSkill, rank, roll, (roll + rank >= nDC) ? "SUCCESS" : "FAILURE", nDC);
}

void Functions::getIsAbilitySuccessful(Aurora::NWScript::FunctionContext &ctx) {
	// GetIsAbilitySuccessful(object oTarget, int nAbility, int nDifficulty) → int
	Aurora::NWScript::Object *object = ctx.getParams()[0].getObject();
	int nAbility = ctx.getParams()[1].getInt();
	int nDC      = ctx.getParams()[2].getInt();

	Creature *creature = ObjectContainer::toCreature(object);
	if (!creature) {
		ctx.getReturn() = 0;
		return;
	}

	int mod  = creature->getCreatureInfo().getAbilityModifier(KotORBase::Ability(nAbility));
	int roll = RNG.getNext(1, 21);
	ctx.getReturn() = (roll + mod >= nDC) ? 1 : 0;

	debugC(Common::kDebugEngineLogic, 1, "Script AbilityCheck: %s, Ability %d, Mod %d, Roll %d -> %s (DC %d)",
	      creature->getTag().c_str(), nAbility, mod, roll, (roll + mod >= nDC) ? "SUCCESS" : "FAILURE", nDC);
}

void Functions::getHasFeatEffect(Aurora::NWScript::FunctionContext &ctx) {
	getHasFeat(ctx);
}

void Functions::talentSpell(Aurora::NWScript::FunctionContext &ctx) {
	const int spellID = ctx.getParams()[0].getInt();
	ctx.getReturn() = Talent(kTalentTypeSpell, spellID);
}

void Functions::talentFeat(Aurora::NWScript::FunctionContext &ctx) {
	const int featID = ctx.getParams()[0].getInt();
	ctx.getReturn() = Talent(kTalentTypeFeat, featID);
}

void Functions::talentSkill(Aurora::NWScript::FunctionContext &ctx) {
	const int skillID = ctx.getParams()[0].getInt();
	ctx.getReturn() = Talent(kTalentTypeSkill, skillID);
}

void Functions::getCreatureHasTalent(Aurora::NWScript::FunctionContext &ctx) {
	// GetCreatureHasTalent(talent tTalent, object oCreature=OBJECT_SELF) → int
	const Talent *talent = ObjectContainer::toTalent(ctx.getParams()[0].getEngineType());
	const Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 1));

	ctx.getReturn() = (talent && creature && creatureHasTalent(*creature, *talent)) ? 1 : 0;
}

void Functions::getCreatureTalentRandom(Aurora::NWScript::FunctionContext &ctx) {
	// GetCreatureTalentRandom(int nCategory, object oCreature=OBJECT_SELF, int nInclusion=0) → talent
	// A random usable talent whose spells/feat/skills.2da category matches nCategory.
	const int category = ctx.getParams()[0].getInt();
	const Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 1));
	if (!creature) {
		ctx.getReturn() = createInvalidTalent();
		return;
	}

	const std::vector<TalentEntry> talents = getCreatureTalents(*creature, TalentTables::fromGameData());
	const std::vector<const TalentEntry *> found = findTalentsInCategory(talents, category);
	if (found.empty()) {
		ctx.getReturn() = createInvalidTalent();
		return;
	}

	ctx.getReturn() = toTalent(*found[getRandom(0, static_cast<int>(found.size()) - 1)]);
}

void Functions::getCreatureTalentBest(Aurora::NWScript::FunctionContext &ctx) {
	// GetCreatureTalentBest(int nCategory, int nCRMax, object oCreature=OBJECT_SELF,
	//                       int nInclusion=0, int nExcludeType=-1, int nExcludeId=-1) → talent
	// The usable talent of nCategory whose maxcr is closest to nCRMax without going over.
	const int category = ctx.getParams()[0].getInt();
	const int crMax    = ctx.getParams()[1].getInt();
	const Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 2));
	const int excludeType = (ctx.getParams().size() > 4) ? ctx.getParams()[4].getInt() : -1;
	const int excludeId   = (ctx.getParams().size() > 5) ? ctx.getParams()[5].getInt() : -1;
	if (!creature) {
		ctx.getReturn() = createInvalidTalent();
		return;
	}

	const std::vector<TalentEntry> talents = getCreatureTalents(*creature, TalentTables::fromGameData());
	const TalentEntry *best = findBestTalent(talents, category, crMax, excludeType, excludeId);

	ctx.getReturn() = best ? toTalent(*best) : createInvalidTalent();
}

void Functions::getIsTalentValid(Aurora::NWScript::FunctionContext &ctx) {
	const Talent *talent = ObjectContainer::toTalent(ctx.getParams()[0].getEngineType());
	ctx.getReturn() = talent ? static_cast<int32_t>(talent->isValid()) : 0;
}

void Functions::getTypeFromTalent(Aurora::NWScript::FunctionContext &ctx) {
	// TALENT_TYPE_FORCE (0), TALENT_TYPE_FEAT (1) or TALENT_TYPE_SKILL (2).
	const Talent *talent = ObjectContainer::toTalent(ctx.getParams()[0].getEngineType());
	ctx.getReturn() = (talent && talent->isValid()) ? static_cast<int32_t>(talent->getType()) : -1;
}

void Functions::getIdFromTalent(Aurora::NWScript::FunctionContext &ctx) {
	const Talent *talent = ObjectContainer::toTalent(ctx.getParams()[0].getEngineType());
	ctx.getReturn() = talent ? static_cast<int32_t>(talent->getID()) : -1;
}

void Functions::getCategoryFromTalent(Aurora::NWScript::FunctionContext &ctx) {
	// The talent's "category" column in spells.2da, feat.2da or skills.2da.
	const Talent *talent = ObjectContainer::toTalent(ctx.getParams()[0].getEngineType());
	ctx.getReturn() = talent ? static_cast<int32_t>(talent->getCategory()) : -1;
}

void Functions::getLastCombatFeatUsed(Aurora::NWScript::FunctionContext &ctx) {
	const Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	ctx.getReturn() = creature ? creature->getLastCombatFeatUsed() : -1;
}

void Functions::getAbilityScore(Aurora::NWScript::FunctionContext &ctx) {
	// GetAbilityScore(object oCreature, int nAbilityType) → int
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	int nAbilityType = ctx.getParams()[1].getInt();

	if (!creature) {
		warning("Functions::getAbilityScore(): invalid creature");
		ctx.getReturn() = 0;
		return;
	}

	ctx.getReturn() = creature->getAbilityScore(KotORBase::Ability(nAbilityType));
}

void Functions::getHasSpell(Aurora::NWScript::FunctionContext &ctx) {
	int spell = ctx.getParams()[0].getInt();
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 1));

	ctx.getReturn() = (creature && creature->hasForcePower(spell)) ? 1 : 0;
}

void Functions::getHasSpellEffect(Aurora::NWScript::FunctionContext &ctx) {
	int spell = ctx.getParams()[0].getInt();
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 1));

	if (!creature) {
		ctx.getReturn() = 0;
		return;
	}

	ctx.getReturn() = creature->hasSpellEffect(spell) ? 1 : 0;
}

void Functions::getAbilityModifier(Aurora::NWScript::FunctionContext &ctx) {
	int nAbilityType = ctx.getParams()[0].getInt();
	Aurora::NWScript::Object *object = ctx.getParams()[1].getObject();

	Creature *creature = ObjectContainer::toCreature(object);

	if (!creature) {
		warning("Functions::getAbilityModifier(): invalid creature");
		ctx.getReturn() = 0;
		return;
	}

	ctx.getReturn() = creature->getCreatureInfo().getAbilityModifier(KotORBase::Ability(nAbilityType));
}

void Functions::getIsDead(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	ctx.getReturn() = creature ? (int32_t)creature->isDead() : 0;
}

void Functions::getHitDice(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	ctx.getReturn() = creature ? creature->getHitDice() : 0;
}

void Functions::getAC(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	ctx.getReturn() = creature ? creature->getAC() : 10;
}

void Functions::getAttackTarget(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);

	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	if (!creature)
		return;

	ctx.getReturn() = creature->getAttackTarget();
}

void Functions::getIsInCombat(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	ctx.getReturn() = creature ? (int32_t)creature->isInCombat() : 0;
}

void Functions::getLastHostileActor(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);

	// Parameter 0 defaults to OBJECT_SELF
	Aurora::NWScript::Object *rawParam = ctx.getParams()[0].getObject();
	Creature *creature = ObjectContainer::toCreature(rawParam ? rawParam : ctx.getCaller());
	if (!creature)
		return;

	ctx.getReturn() = creature->getLastHostileActor();
}

void Functions::effectHeal(Aurora::NWScript::FunctionContext &ctx) {
	int amount = ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectHeal, amount);
}

void Functions::effectDamage(Aurora::NWScript::FunctionContext &ctx) {
	int amount = ctx.getParams()[0].getInt();
	int damageType = ctx.getParams()[1].getInt();
	// param 2 (damage power) is not used in the basic implementation
	ctx.getReturn() = Effect(kKotOREffectDamage, amount, damageType);
}

void Functions::effectVisualEffect(Aurora::NWScript::FunctionContext &ctx) {
	const int visualEffect = ctx.getParams()[0].getInt();
	const int exposeToNetwork = ctx.getParams()[1].getInt();
	ctx.getReturn() = Effect(kKotOREffectVisual, visualEffect, exposeToNetwork);
}

void Functions::applyEffectToObject(Aurora::NWScript::FunctionContext &ctx) {
	// int nDurationType, effect eEffect, object oTarget, float fDuration=0.0
	const int durationType = ctx.getParams()[0].getInt();
	const Effect *effect = dynamic_cast<const Effect *>(ctx.getParams()[1].getEngineType());
	Object *target = ObjectContainer::toObject(ctx.getParams()[2].getObject());
	const float duration = (ctx.getParams().size() > 3) ? ctx.getParams()[3].getFloat() : 0.0f;

	if (!effect || !target)
		return;

	// Effects record the spell whose script applied them (GetHasSpellEffect, GetEffectSpellId).
	Effect applied(*effect);
	if (applied.getSpellId() < 0)
		applied.setSpellId(_game->getModule().getSpellScriptId());

	Creature *targetCreature = ObjectContainer::toCreature(target);
	if (targetCreature) {
		float durationOverride = -1.0f;
		if (durationType == 0) // DURATION_TYPE_INSTANT
			durationOverride = 0.0f;
		else if (durationType == 1) // DURATION_TYPE_TEMPORARY
			durationOverride = duration > 0.0f ? duration : 6.0f;
		else if (durationType == 2) // DURATION_TYPE_PERMANENT
			durationOverride = 1.0e9f;

		// Force push: thrown back away from the caster, as far as the walkmesh allows.
		Area *area = _game->getModule().getCurrentArea();
		if (area && applied.hasPart(kKotOREffectForcePush) && !targetCreature->isDead() &&
		    !targetCreature->isImmune(kImmunityTypeKnockdown)) {
			Object *source = _game->getModule().getSpellScriptCaster();
			if (!source)
				source = ObjectContainer::toObject(ctx.getCaller());

			float fromX, fromY, fromZ;
			if (source && (source != targetCreature)) {
				source->getPosition(fromX, fromY, fromZ);
			} else {
				targetCreature->getPosition(fromX, fromY, fromZ);
			}
			targetCreature->knockBack(*area, fromX, fromY, 5.0f);
		}

		targetCreature->applyEffect(applied, durationOverride);
	} else {
		// Fallback for non-creature objects (e.g. placeable damage)
		int current = target->getCurrentHitPoints();
		if (applied.getType() == kKotOREffectDamage) {
			target->setCurrentHitPoints(MAX(0, current - applied.getAmount()));
		} else if (applied.getType() == kKotOREffectHeal) {
			target->setCurrentHitPoints(MIN(target->getMaxHitPoints(), current + applied.getAmount()));
		}
	}
}







void Functions::setGoodEvilValue(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	int value = ctx.getParams()[1].getInt();

	if (creature)
		creature->setGoodEvilValue(value);
}

void Functions::fortitudeSave(Aurora::NWScript::FunctionContext &ctx) {
	// FortitudeSave(object oCreature, int nDC, int nSaveType=0, object oSaveVersus=OBJECT_SELF)
	// Returns 0 (failure), 1 (success), or 2 (natural 20 / automatic success)
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	int dc = ctx.getParams()[1].getInt();

	if (!creature) {
		ctx.getReturn() = 0;
		return;
	}

	// Fortitude save = d20 + class base save + Constitution modifier.
	int bonus = creature->getCreatureInfo().getSavingThrowBonus(kSavingThrowFortitude);
	int roll  = RNG.getNext(1, 21);

	if (roll == 20) {
		ctx.getReturn() = 2; // automatic success
		return;
	}
	if (roll == 1) {
		ctx.getReturn() = 0; // automatic failure
		return;
	}
	ctx.getReturn() = ((roll + bonus) >= dc) ? 1 : 0;
}

void Functions::reflexSave(Aurora::NWScript::FunctionContext &ctx) {
	// ReflexSave(object oCreature, int nDC, int nSaveType=0, object oSaveVersus=OBJECT_SELF)
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	int dc = ctx.getParams()[1].getInt();

	if (!creature) {
		ctx.getReturn() = 0;
		return;
	}

	// Reflex save = d20 + class base save + Dexterity modifier.
	int bonus = creature->getCreatureInfo().getSavingThrowBonus(kSavingThrowReflex);
	int roll  = RNG.getNext(1, 21);

	if (roll == 20) {
		ctx.getReturn() = 2;
		return;
	}
	if (roll == 1) {
		ctx.getReturn() = 0;
		return;
	}
	ctx.getReturn() = ((roll + bonus) >= dc) ? 1 : 0;
}

void Functions::willSave(Aurora::NWScript::FunctionContext &ctx) {
	// WillSave(object oCreature, int nDC, int nSaveType=0, object oSaveVersus=OBJECT_SELF)
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	int dc = ctx.getParams()[1].getInt();

	if (!creature) {
		ctx.getReturn() = 0;
		return;
	}

	// Will save = d20 + class base save + Wisdom modifier.
	int bonus = creature->getCreatureInfo().getSavingThrowBonus(kSavingThrowWill);
	int roll  = RNG.getNext(1, 21);

	if (roll == 20) {
		ctx.getReturn() = 2;
		return;
	}
	if (roll == 1) {
		ctx.getReturn() = 0;
		return;
	}
	ctx.getReturn() = ((roll + bonus) >= dc) ? 1 : 0;
}

void Functions::giveXPToCreature(Aurora::NWScript::FunctionContext &ctx) {
	// GiveXPToCreature(object oCreature, int nXPAmount)
	// Adds nXPAmount experience to the creature and fires ShowLevelUpGUI for the PC.
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	int amount = ctx.getParams()[1].getInt();

	if (!creature || amount <= 0)
		return;

	creature->addPlotXP(amount);
}

void Functions::setXP(Aurora::NWScript::FunctionContext &ctx) {
	// SetXP(object oCreature, int nXPAmount)
	// Directly sets the total XP of a creature.
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	int amount = ctx.getParams()[1].getInt();

	if (!creature)
		return;

	creature->setCurrentXP(amount);
}

void Functions::getXP(Aurora::NWScript::FunctionContext &ctx) {
	// GetXP(object oCreature) → int
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());

	ctx.getReturn() = creature ? creature->getCurrentXP() : 0;
}

void Functions::givePlotXP(Aurora::NWScript::FunctionContext &ctx) {
	// GivePlotXP(string sPlotName, int nPercentage)
	const Common::UString &plotName = ctx.getParams()[0].getString();
	int percentage = ctx.getParams()[1].getInt();

	// plot.2da: the row whose label is the plot name holds its base XP.
	int xp = percentage; // Fallback: previous behaviour, when the row is missing.
	try {
		const Aurora::TwoDARow &row = TwoDAReg.get2DA("plot").getRow("label", plotName);
		if (!row.empty("xp"))
			xp = (row.getInt("xp") * percentage) / 100;
		else
			warning("Functions::givePlotXP: no plot.2da row for \"%s\"", plotName.c_str());
	} catch (...) {
		warning("Functions::givePlotXP: plot.2da unavailable (plot \"%s\")", plotName.c_str());
	}

	debugC(Common::kDebugEngineLogic, 1, "GivePlotXP(\"%s\", %d%%): %d XP", plotName.c_str(), percentage, xp);

	_game->getModule().awardPartyXP(xp);
}

void Functions::awardStealthXP(Aurora::NWScript::FunctionContext &ctx) {
	// AwardStealthXP(object oCreatureTarget = OBJECT_SELF)
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	if (!creature)
		return;

	// In KotOR, stealth XP is usually handled by a global multiplier
	// and decremented as the player is detected or uses it.
	int xp = _game->getModule().getGlobalNumber("__stealth_xp_pool");
	if (xp > 0) {
		creature->addPlotXP(xp);
		debugC(Common::kDebugEngineLogic, 1, "Stealth XP awarded: %d", xp);
		
		// Auto-decrement if enabled
		if (_game->getModule().getGlobalBoolean("__stealth_xp_decrement_on_award")) {
			int decrement = _game->getModule().getGlobalNumber("__stealth_xp_decrement");
			_game->getModule().setGlobalNumber("__stealth_xp_pool", MAX(0, xp - decrement));
		}
	}
}

void Functions::getStealthXPEnabled(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getGlobalBoolean("__stealth_xp_enabled") ? 1 : 0;
}

void Functions::setStealthXPEnabled(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalBoolean("__stealth_xp_enabled", ctx.getParams()[0].getInt() != 0);
}

void Functions::getStealthXPDecrement(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getGlobalNumber("__stealth_xp_decrement");
}

void Functions::setStealthXPDecrement(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber("__stealth_xp_decrement", ctx.getParams()[0].getInt());
}

// ---------------------------------------------------------------------------
// New Effect constructors — Milestone 3
// ---------------------------------------------------------------------------

void Functions::effectACIncrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectACIncrease(int nValue, int nModifyType=0, int nDamageType=8199)
	int bonus = ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectACIncrease, bonus);
}

void Functions::effectAttackIncrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectAttackIncrease(int nBonus, int nModifierType=0)
	int bonus = ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectAttackIncrease, bonus);
}

void Functions::effectSkillIncrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectSkillIncrease(int nSkill, int nValue)
	int skill  = ctx.getParams()[0].getInt();
	int amount = ctx.getParams()[1].getInt();
	ctx.getReturn() = Effect(kKotOREffectSkillIncrease, amount, skill);
}

void Functions::effectTemporaryHitpoints(Aurora::NWScript::FunctionContext &ctx) {
	// EffectTemporaryHitpoints(int nHitPoints)
	int amount = ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectTemporaryHitpoints, amount);
}

// ---------------------------------------------------------------------------
// Item queries — Milestone 3
// ---------------------------------------------------------------------------

void Functions::getBaseItemType(Aurora::NWScript::FunctionContext &ctx) {
	// GetBaseItemType(object oItem) → int
	Item *item = ObjectContainer::toItem(getParamObject(ctx, 0));
	if (!item) {
		ctx.getReturn() = -1;
		return;
	}
	ctx.getReturn() = item->getBaseItem();
}

void Functions::getWeaponRanged(Aurora::NWScript::FunctionContext &ctx) {
	// GetWeaponRanged(object oItem) → int
	Item *item = ObjectContainer::toItem(getParamObject(ctx, 0));
	if (!item) {
		ctx.getReturn() = 0;
		return;
	}
	int baseItem = item->getBaseItem();
	// Ranged types in KotOR: blaster pistols, rifles, heavy weapons, bowcasters, grenades
	bool ranged = (baseItem >= 0 && baseItem <= 5) || (baseItem >= 9 && baseItem <= 12);
	ctx.getReturn() = ranged ? 1 : 0;
}

void Functions::getSkillPointBonus(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	ctx.getReturn() = creature ? creature->getSkillPointBonus() : 0;
}

void Functions::setSkillPointBonus(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	int bonus = ctx.getParams()[1].getInt();
	if (creature)
		creature->setSkillPointBonus(bonus);
}

// ---------------------------------------------------------------------------
// New effect constructors — combat/status effects for Dantooine parity
// ---------------------------------------------------------------------------

void Functions::effectDeath(Aurora::NWScript::FunctionContext &ctx) {
	// EffectDeath(int bSpectacularDeath=FALSE, int bDisplayFeedback=TRUE)
	// Instantly kills the target when applied.
	ctx.getReturn() = Effect(kKotOREffectDeath, 0);
}

void Functions::effectKnockdown(Aurora::NWScript::FunctionContext &ctx) {
	// EffectKnockdown() — knocks target prone, losing any Dex bonus to AC.
	ctx.getReturn() = Effect(kKotOREffectKnockdown, 0);
}

void Functions::effectParalyze(Aurora::NWScript::FunctionContext &ctx) {
	// EffectParalyze() — used by Force Stasis; target cannot act or dodge.
	ctx.getReturn() = Effect(kKotOREffectParalyze, 0);
}

void Functions::effectStunned(Aurora::NWScript::FunctionContext &ctx) {
	// EffectStunned() — stuns target (interrupts action queue).
	ctx.getReturn() = Effect(kKotOREffectStunned, 0);
}

void Functions::effectHaste(Aurora::NWScript::FunctionContext &ctx) {
	// EffectHaste() — grants one extra attack per round.
	ctx.getReturn() = Effect(kKotOREffectHaste, 0);
}

void Functions::effectAbilityIncrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectAbilityIncrease(int nAbilityToIncrease, int nModifyBy)
	int ability  = ctx.getParams()[0].getInt();
	int modifyBy = ctx.getParams()[1].getInt();
	// Pack ability type into damageType field, amount into amount field.
	ctx.getReturn() = Effect(kKotOREffectAbilityIncrease, modifyBy, ability);
}

void Functions::effectMovementSpeedIncrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectMovementSpeedIncrease(int nNewSpeedPercent)
	int percent = ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectMovementSpeedIncrease, percent);
}

void Functions::effectResurrection(Aurora::NWScript::FunctionContext &ctx) {
	// EffectResurrection() — revives a dead creature at 1 HP.
	ctx.getReturn() = Effect(kKotOREffectResurrection, 1);
}

void Functions::effectCutSceneHorrified(Aurora::NWScript::FunctionContext &ctx) {
	// Cut-scene variant: treat as hard crowd-control for script progression.
	ctx.getReturn() = Effect(kKotOREffectStunned, 0);
}

void Functions::effectCutSceneParalyze(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectParalyze, 0);
}

void Functions::effectCutSceneStunned(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectStunned, 0);
}

void Functions::effectForcePushed(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectForcePush, 0);
}

void Functions::effectForcePushTargeted(Aurora::NWScript::FunctionContext &ctx) {
	// Object oSource is usually the person who cast it.
	ctx.getReturn() = Effect(kKotOREffectForcePush, 0);
}

void Functions::effectForceDrain(Aurora::NWScript::FunctionContext &ctx) {
	int amount = ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectForceDrain, amount);
}

void Functions::effectForceShield(Aurora::NWScript::FunctionContext &ctx) {
	int amount = ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectForceShield, amount);
}

void Functions::effectHealForcePoints(Aurora::NWScript::FunctionContext &ctx) {
	int amount = ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectForcePointsHeal, amount);
}

void Functions::effectDamageForcePoints(Aurora::NWScript::FunctionContext &ctx) {
	int amount = ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectForcePointsDamage, amount);
}

// ---------------------------------------------------------------------------
// Touch attacks — Force power delivery mechanism
// ---------------------------------------------------------------------------

void Functions::touchAttackMelee(Aurora::NWScript::FunctionContext &ctx) {
	// TouchAttackMelee(object oTarget, int bDisplayFeedback=TRUE) → int
	// Returns: 0=miss, 1=hit, 2=critical.  Touch attacks ignore armour AC.
	Object *target = ObjectContainer::toObject(getParamObject(ctx, 0));
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller || !target) {
		ctx.getReturn() = 0;
		return;
	}

	Creature *targetCreature = ObjectContainer::toCreature(target);

	// Touch AC = 10 + Dex modifier only (no armour).
	int touchAC = 10;
	if (targetCreature)
		touchAC += targetCreature->getCreatureInfo().getAbilityModifier(kAbilityDexterity);

	int bab      = caller->getBAB();
	int strMod   = caller->getCreatureInfo().getAbilityModifier(kAbilityStrength);
	int d20Roll  = RNG.getNext(1, 21);
	int total    = d20Roll + bab + strMod;

	bool hit  = (d20Roll == 20) || (d20Roll != 1 && total >= touchAC);
	bool crit = hit && (d20Roll == 20);

	ctx.getReturn() = crit ? 2 : (hit ? 1 : 0);
}

void Functions::touchAttackRanged(Aurora::NWScript::FunctionContext &ctx) {
	// TouchAttackRanged(object oTarget, int bDisplayFeedback=TRUE) → int
	// Same as melee touch but uses Dex for the attack roll.
	Object *target = ObjectContainer::toObject(getParamObject(ctx, 0));
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller || !target) {
		ctx.getReturn() = 0;
		return;
	}

	Creature *targetCreature = ObjectContainer::toCreature(target);

	int touchAC = 10;
	if (targetCreature)
		touchAC += targetCreature->getCreatureInfo().getAbilityModifier(kAbilityDexterity);

	int bab     = caller->getBAB();
	int dexMod  = caller->getCreatureInfo().getAbilityModifier(kAbilityDexterity);
	int d20Roll = RNG.getNext(1, 21);
	int total   = d20Roll + bab + dexMod;

	bool hit  = (d20Roll == 20) || (d20Roll != 1 && total >= touchAC);
	bool crit = hit && (d20Roll == 20);

	ctx.getReturn() = crit ? 2 : (hit ? 1 : 0);
}

// ---------------------------------------------------------------------------
// Item stack size queries
// ---------------------------------------------------------------------------

void Functions::getItemStackSize(Aurora::NWScript::FunctionContext &ctx) {
	// GetItemStackSize(object oItem) → int
	Item *item = ObjectContainer::toItem(getParamObject(ctx, 0));
	// Stack size is stored on the item (all KotOR items default to 1 unless
	// they are stackable — grenades, medpacs, etc.).
	ctx.getReturn() = item ? item->getStackSize() : 0;
}

void Functions::setItemStackSize(Aurora::NWScript::FunctionContext &ctx) {
	// SetItemStackSize(object oItem, int nSize)
	Item *item = ObjectContainer::toItem(getParamObject(ctx, 0));
	int  size  = ctx.getParams()[1].getInt();
	if (item && size >= 0)
		item->setStackSize(size);
}

void Functions::setAIArchetype(Aurora::NWScript::FunctionContext &ctx) {
	int archetypeValue = ctx.getParams()[0].getInt();
	Aurora::NWScript::Object *object = getParamObject(ctx, 1);

	Creature *creature = ObjectContainer::toCreature(object);

	if (!creature) {
		warning("Functions::setAIArchetype(): invalid creature");
		return;
	}

	creature->setAIArchetype(static_cast<Creature::AIArchetype>(archetypeValue));
}

void Functions::getStealthMode(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	ctx.getReturn() = (creature && creature->getStealthMode()) ? 1 : 0;
}

void Functions::setStealthMode(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	bool mode = ctx.getParams()[1].getInt() != 0;
	if (creature)
		creature->setStealthMode(mode);
}

void Functions::setLightsaberPowered(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	bool powered = ctx.getParams()[1].getInt() != 0;
	if (creature)
		creature->setLightsaberPowered(powered);
}

void Functions::getAppearanceType(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	ctx.getReturn() = creature ? static_cast<int>(creature->getAppearanceType()) : -1;
}

void Functions::setAppearanceType(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	int appearance = ctx.getParams()[1].getInt();
	if (creature)
		creature->setAppearanceType(static_cast<uint32_t>(appearance));
}

void Functions::getSpellId(Aurora::NWScript::FunctionContext &ctx) {
	int spellId = _game->getModule().getSpellScriptId();
	if (spellId < 0) {
		Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
		if (caller)
			spellId = caller->getLastForcePowerUsed();
	}
	ctx.getReturn() = spellId;
}

void Functions::getLastSpellHarmful(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getSpellScriptHarmful() ? 1 : 0;
}

void Functions::getSpellTargetLocation(Aurora::NWScript::FunctionContext &ctx) {
	Location loc;
	Object *target = _game->getModule().getSpellScriptTarget();
	glm::vec3 location;
	if (_game->getModule().getSpellScriptTargetLocation(location)) {
		loc.setPosition(location.x, location.y, location.z);
	} else if (target) {
		float x, y, z;
		target->getPosition(x, y, z);
		loc.setPosition(x, y, z);
	}
	ctx.getReturn() = loc;
}

void Functions::getSpellTarget(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(_game->getModule().getSpellScriptTarget());
}

void Functions::getSpellSaveDC(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getSpellScriptSaveDC();
}

void Functions::getSpellCastItem(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(_game->getModule().getSpellScriptCastItem());
}

void Functions::getMetaMagicFeat(Aurora::NWScript::FunctionContext &ctx) {
	// KotOR does not use NWN-style metamagic; return 0 (METAMAGIC_NONE).
	ctx.getReturn() = 0;
}

void Functions::getEffectSpellId(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = -1;
	const Effect *effect = dynamic_cast<const Effect *>(ctx.getParams()[0].getEngineType());
	if (!effect)
		return;

	if (effect->getSpellId() >= 0) {
		ctx.getReturn() = effect->getSpellId();
		return;
	}

	// Spell-immunity effects pack the spell id in amount.
	if (effect->getType() == kKotOREffectSpellImmunity)
		ctx.getReturn() = effect->getAmount();
}

void Functions::getLastKiller(Aurora::NWScript::FunctionContext &ctx) {
	// GetLastKiller() takes no parameters: it queries the caller.
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
	Creature *creature = ObjectContainer::toCreature(ctx.getCaller());
	if (!creature)
		return;
	Object *killer = creature->getLastKiller();
	if (!killer)
		killer = creature->getLastHostileActor();
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(killer);
}

void Functions::getDamageDealtByType(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = -1;
	Creature *creature = ObjectContainer::toCreature(ctx.getCaller());
	if (!creature)
		return;
	ctx.getReturn() = creature->getDamageDealtByType(ctx.getParams()[0].getInt());
}

void Functions::getTotalDamageDealt(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getCaller());
	ctx.getReturn() = creature ? creature->getTotalDamageDealt() : 0;
}

void Functions::getLastAttackResult(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getCaller());
	ctx.getReturn() = creature ? creature->getLastAttackResult() : 0;
}

void Functions::getLastWeaponUsed(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	if (!creature)
		creature = ObjectContainer::toCreature(ctx.getCaller());
	if (creature)
		ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(creature->getLastWeaponUsed());
}

namespace {

/** Surrender, and make everyone in the area stop attacking the surrendering creature. */
void surrenderInArea(Creature &creature, Area *area, bool retainBuffs) {
	creature.surrenderToEnemies(retainBuffs);
	if (area)
		Creature::stopAttacksOn(area->getCreatures(), &creature, true);
}

} // End of anonymous namespace

void Functions::surrenderToEnemies(Aurora::NWScript::FunctionContext &ctx) {
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (caller)
		surrenderInArea(*caller, _game->getModule().getCurrentArea(), false);
}

void Functions::actionSurrenderToEnemies(Aurora::NWScript::FunctionContext &ctx) {
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (caller)
		surrenderInArea(*caller, _game->getModule().getCurrentArea(), false);
}

void Functions::surrenderByFaction(Aurora::NWScript::FunctionContext &ctx) {
	int faction = ctx.getParams()[0].getInt();
	Area *area = _game->getModule().getCurrentArea();
	if (!area)
		return;
	const std::vector<Creature *> &creatures = area->getCreatures();
	for (size_t i = 0; i < creatures.size(); ++i) {
		Creature *c = creatures[i];
		if (c && !c->isDead() && static_cast<int>(c->getFaction()) == faction)
			surrenderInArea(*c, area, false);
	}
}

void Functions::surrenderRetainBuffs(Aurora::NWScript::FunctionContext &ctx) {
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (caller)
		surrenderInArea(*caller, _game->getModule().getCurrentArea(), true);
}

void Functions::effectImmunity(Aurora::NWScript::FunctionContext &ctx) {
	int immunityType = ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectImmunity, immunityType);
}

void Functions::getIsImmune(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = 0;
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	int immunityType = ctx.getParams()[1].getInt();
	if (creature && creature->isImmune(immunityType))
		ctx.getReturn() = 1;
}

void Functions::effectDamageImmunityIncrease(Aurora::NWScript::FunctionContext &ctx) {
	int damageType = ctx.getParams()[0].getInt();
	int percent = ctx.getParams()[1].getInt();
	ctx.getReturn() = Effect(kKotOREffectDamageImmunityIncrease, percent, damageType);
}

void Functions::effectSpellImmunity(Aurora::NWScript::FunctionContext &ctx) {
	int spellId = ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectSpellImmunity, spellId);
}

void Functions::effectSleep(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectSleep, 0);
}

void Functions::effectRegenerate(Aurora::NWScript::FunctionContext &ctx) {
	int amount = ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectRegenerate, amount);
}

void Functions::effectTemporaryForcePoints(Aurora::NWScript::FunctionContext &ctx) {
	int amount = ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectTemporaryForcePoints, amount);
}

void Functions::effectAreaOfEffect(Aurora::NWScript::FunctionContext &ctx) {
	int aoeId = ctx.getParams()[0].getInt();
	_game->getModule().setLastAoECreator(ObjectContainer::toObject(ctx.getCaller()));
	ctx.getReturn() = Effect(kKotOREffectAreaOfEffect, aoeId);
}

void Functions::magicalEffect(Aurora::NWScript::FunctionContext &ctx) {
	const Effect *effect = dynamic_cast<const Effect *>(ctx.getParams()[0].getEngineType());
	if (effect)
		ctx.getReturn() = *effect;
	else
		ctx.getReturn() = Effect(kKotOREffectVisual, 0);
}

void Functions::supernaturalEffect(Aurora::NWScript::FunctionContext &ctx) {
	magicalEffect(ctx);
}

void Functions::extraordinaryEffect(Aurora::NWScript::FunctionContext &ctx) {
	magicalEffect(ctx);
}

void Functions::effectDamageResistance(Aurora::NWScript::FunctionContext &ctx) {
	// EffectDamageResistance(int nDamageType, int nAmount, int nLimit=0)
	int damageType = ctx.getParams()[0].getInt();
	int amount = ctx.getParams()[1].getInt();
	int limit = (ctx.getParams().size() > 2) ? ctx.getParams()[2].getInt() : 0;
	ctx.getReturn() = Effect(kKotOREffectDamageResistance, amount, damageType, -1, limit);
}

void Functions::effectConcealment(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectConcealment, ctx.getParams()[0].getInt());
}

void Functions::effectAssuredHit(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectAssuredHit, 1);
}

void Functions::effectAssuredDeflection(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectAssuredDeflection, 1);
}

void Functions::effectEntangle(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectEntangle, 0);
}

void Functions::effectForceJump(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectForceJump, 0);
}

void Functions::effectBeam(Aurora::NWScript::FunctionContext &ctx) {
	int beamType = ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectBeam, beamType);
}

void Functions::effectForceResistanceIncrease(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectForceResistanceIncrease, ctx.getParams()[0].getInt());
}

void Functions::effectBodyFuel(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectBodyFuel, 5);
}

void Functions::effectDamageIncrease(Aurora::NWScript::FunctionContext &ctx) {
	int amount = ctx.getParams()[0].getInt();
	int damageType = ctx.getParams().size() > 1 ? ctx.getParams()[1].getInt() : 0;
	ctx.getReturn() = Effect(kKotOREffectDamageIncrease, amount, damageType);
}

void Functions::effectHitPointChangeWhenDying(Aurora::NWScript::FunctionContext &ctx) {
	int amount = 1;
	if (!ctx.getParams().empty() && ctx.getParams()[0].getType() == Aurora::NWScript::kTypeInt)
		amount = ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectHitPointChangeWhenDying, amount);
}

void Functions::effectDroidStun(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectDroidStun, 0);
}

void Functions::effectForceResisted(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectForceResisted, 0);
}

void Functions::effectForceFizzle(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectForceFizzle, 0);
}

void Functions::effectDispelMagicAll(Aurora::NWScript::FunctionContext &ctx) {
	int casterLevel = ctx.getParams().empty() ? 0 : ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectDispelMagicAll, casterLevel);
}

void Functions::effectBlasterDeflectionIncrease(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectBlasterDeflectionIncrease, ctx.getParams()[0].getInt());
}

void Functions::effectBlasterDeflectionDecrease(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectBlasterDeflectionDecrease, ctx.getParams()[0].getInt());
}

void Functions::grantFeat(Aurora::NWScript::FunctionContext &ctx) {
	int feat = ctx.getParams()[0].getInt();
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 1));
	if (creature)
		creature->getCreatureInfo().addFeat(static_cast<uint32_t>(feat));
}

void Functions::grantSpell(Aurora::NWScript::FunctionContext &ctx) {
	int spell = ctx.getParams()[0].getInt();
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 1));
	if (creature)
		creature->getCreatureInfo().addForcePower(static_cast<uint32_t>(spell));
}


void Functions::getFortitudeSavingThrow(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	ctx.getReturn() = creature ? creature->getSavingThrowBonus(kSavingThrowFortitude) : 0;
}

void Functions::getWillSavingThrow(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	ctx.getReturn() = creature ? creature->getSavingThrowBonus(kSavingThrowWill) : 0;
}

void Functions::getReflexSavingThrow(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	ctx.getReturn() = creature ? creature->getSavingThrowBonus(kSavingThrowReflex) : 0;
}

void Functions::effectModifyAttacks(Aurora::NWScript::FunctionContext &ctx) {
	int attacks = ctx.getParams()[0].getInt();
	ctx.getReturn() = Effect(kKotOREffectMovementSpeedIncrease, attacks); // Simplified as speed effect for now
}

void Functions::effectLightsaberThrow(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectVisual, 100); // Dummy light side VFX as placeholder
}

void Functions::effectWhirlWind(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectStunned, 6); // Whirlwind as stun for now
}


void Functions::setEffectIcon(Aurora::NWScript::FunctionContext &ctx) {
	// SetEffectIcon(effect eEffect, int nIcon) → effect
	// Effect icons are not modelled; hand the effect back unchanged.
	ctx.getReturn() = ctx.getParams()[0].getEngineType();
}
void Functions::getFactionLeader(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = (Aurora::NWScript::Object *)nullptr; }
void Functions::setNPCAIStyle(Aurora::NWScript::FunctionContext &ctx) {}
void Functions::setNPCSelectability(Aurora::NWScript::FunctionContext &ctx) {}
void Functions::getNPCSelectability(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 1; }
void Functions::getIsDebilitated(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	if (!creature)
		creature = ObjectContainer::toCreature(ctx.getCaller());
	ctx.getReturn() = 0;
	if (!creature)
		return;
	if (creature->hasEffect(kEffectStun) ||
	    creature->hasEffect(kEffectKnockdown) ||
	    creature->hasEffect(kEffectConfusion) ||
	    creature->hasEffect(kEffectDazed))
		ctx.getReturn() = 1;
}
void Functions::getFirstAttacker(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = (Aurora::NWScript::Object *)nullptr; }
void Functions::getNextAttacker(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = (Aurora::NWScript::Object *)nullptr; }
void Functions::playRoomAnimation(Aurora::NWScript::FunctionContext &ctx) {}
void Functions::effectPsychicStatic(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = Effect(kKotOREffectVisual, 0); }
void Functions::playVisualAreaEffect(Aurora::NWScript::FunctionContext &ctx) {
	const int effectId = ctx.getParams()[0].getInt();
	Location *location = ObjectContainer::toLocation(ctx.getParams()[1].getEngineType());

	debugC(Common::kDebugEngineLogic, 1, "PlayVisualAreaEffect: %d", effectId);

	if (location) {
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		location->getPosition(x, y, z);
		(void)x;
		(void)y;
		(void)z;
	}

	if (effectId != 0)
		_game->getModule().shakeCamera(0.35f, 0.15f);
}
void Functions::aurPostString(Aurora::NWScript::FunctionContext &ctx) {
	Common::UString str = ctx.getParams()[0].getString();
	debugC(Common::kDebugEngineLogic, 1, "[AUR POST] %s", str.c_str());
}


void Functions::getLastItemEquipped(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = (Aurora::NWScript::Object *)nullptr; }
void Functions::getSubScreenID(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 0; }
void Functions::getCasterLevel(Aurora::NWScript::FunctionContext &ctx) {
	Creature *caster = ObjectContainer::toCreature(ctx.getCaller());
	if (!caster)
		caster = ObjectContainer::toCreature(_game->getModule().getSpellScriptCaster());
	ctx.getReturn() = caster ? caster->getHitDice() : 0;
}
void Functions::resistForce(Aurora::NWScript::FunctionContext &ctx) {
	// ResistForce(object oCaster, object oTarget) → TRUE if the Force power is resisted.
	Creature *caster = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	Creature *target = ObjectContainer::toCreature(ctx.getParams()[1].getObject());
	ctx.getReturn() = 0;
	if (!caster || !target)
		return;

	const int spellId = _game->getModule().getSpellScriptId();
	if (spellId >= 0 && target->isImmuneToSpell(spellId)) {
		ctx.getReturn() = 1;
		return;
	}

	const int resistance = target->getForceResistance();
	if (resistance <= 0)
		return;

	const int roll = RNG.getNext(1, 21);
	const int check = roll + caster->getHitDice();
	if (check < resistance)
		ctx.getReturn() = 1;
}
void Functions::getLastSpellCaster(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(_game->getModule().getSpellScriptCaster());
}
void Functions::getLastSpell(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getSpellScriptId();
}
void Functions::effectConfused(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = Effect(kKotOREffectStunned, 0); }
void Functions::effectFrightened(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = Effect(kKotOREffectStunned, 0); }
void Functions::effectChoke(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = Effect(kKotOREffectStunned, 0); }

void Functions::effectSavingThrowIncrease(Aurora::NWScript::FunctionContext &ctx) {
	int savingThrow = ctx.getParams()[0].getInt();
	int value       = ctx.getParams()[1].getInt();
	ctx.getReturn() = Effect(kKotOREffectSavingThrowIncrease, value, savingThrow);
}

void Functions::effectDamageReduction(Aurora::NWScript::FunctionContext &ctx) {
	// EffectDamageReduction(int nAmount, int nDamagePower, int nLimit=0)
	int amount = ctx.getParams()[0].getInt();
	int limit = (ctx.getParams().size() > 2) ? ctx.getParams()[2].getInt() : 0;
	ctx.getReturn() = Effect(kKotOREffectDamageReduction, amount, 0, -1, limit);
}

void Functions::effectInvisibility(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectInvisibility, ctx.getParams()[0].getInt());
}

void Functions::effectPoison(Aurora::NWScript::FunctionContext &ctx) {
	// EffectPoison(int nPoisonType) → effect
	// Retail poison types come in groups of four (mild, average, virulent,
	// deadly). Without poison.2da data, scale the damage per second by that
	// severity and use the creature's default poison duration (damageType 0).
	const int poisonType = ctx.getParams()[0].getInt();
	const int severity   = (poisonType >= 0) ? ((poisonType % 4) + 1) : 1;
	ctx.getReturn() = Effect(kKotOREffectPoison, severity, 0);
}

void Functions::effectSeeInvisible(Aurora::NWScript::FunctionContext &ctx) {
	// EffectSeeInvisible() → effect (no parameters)
	ctx.getReturn() = Effect(kKotOREffectSeeInvisible);
}

void Functions::effectAbilityDecrease(Aurora::NWScript::FunctionContext &ctx) {
	int ability  = ctx.getParams()[0].getInt();
	int modifyBy = ctx.getParams()[1].getInt();
	ctx.getReturn() = Effect(kKotOREffectAbilityDecrease, modifyBy, ability);
}

void Functions::effectAttackDecrease(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectAttackDecrease, ctx.getParams()[0].getInt());
}

void Functions::effectDamageDecrease(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectDamageDecrease, ctx.getParams()[0].getInt());
}

void Functions::effectDamageImmunityDecrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectDamageImmunityDecrease(int nDamageType, int nPercentImmunity)
	ctx.getReturn() = Effect(kKotOREffectDamageImmunityDecrease, ctx.getParams()[1].getInt(),
	                         ctx.getParams()[0].getInt());
}

void Functions::effectACDecrease(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectACDecrease, ctx.getParams()[0].getInt());
}

void Functions::effectMovementSpeedDecrease(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectMovementSpeedDecrease, ctx.getParams()[0].getInt());
}

void Functions::effectSavingThrowDecrease(Aurora::NWScript::FunctionContext &ctx) {
	int savingThrow = ctx.getParams()[0].getInt();
	int value       = ctx.getParams()[1].getInt();
	ctx.getReturn() = Effect(kKotOREffectSavingThrowDecrease, value, savingThrow);
}

void Functions::effectSkillDecrease(Aurora::NWScript::FunctionContext &ctx) {
	int skill  = ctx.getParams()[0].getInt();
	int amount = ctx.getParams()[1].getInt();
	ctx.getReturn() = Effect(kKotOREffectSkillDecrease, amount, skill);
}

void Functions::effectForceResistanceDecrease(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = Effect(kKotOREffectForceResistanceDecrease, ctx.getParams()[0].getInt());
}

void Functions::getLastForcePowerUsed(Aurora::NWScript::FunctionContext &ctx) {
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	ctx.getReturn() = caller ? caller->getLastForcePowerUsed() : -1;
}

} // End of namespace KotORBase
} // End of namespace Engines
