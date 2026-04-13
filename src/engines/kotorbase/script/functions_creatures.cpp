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
#include "src/common/random.h"
#include "src/common/debug.h"

#include "src/aurora/nwscript/functioncontext.h"

#include "src/engines/kotorbase/types.h"
#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/effect.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/talent.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/game.h"

#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

namespace {

Talent *createInvalidTalent() {
	return new Talent();
}

bool hasAnySkillTalent(const Creature *creature) {
	for (int skill = kSkillComputerUse; skill < kSkillMAX; ++skill) {
		if (creature->getCreatureInfo().getSkillRank(static_cast<Skill>(skill)) > 0)
			return true;
	}

	return false;
}

int getBestSkillTalentID(const Creature *creature) {
	int bestSkill = -1;
	int bestRank = -1;

	for (int skill = kSkillComputerUse; skill < kSkillMAX; ++skill) {
		const int rank = creature->getCreatureInfo().getSkillRank(static_cast<Skill>(skill));
		if (rank > bestRank) {
			bestRank = rank;
			bestSkill = skill;
		}
	}

	if (bestRank <= 0)
		return -1;

	return bestSkill;
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

	// Add the Jedi class at level 1
	creature->getCreatureInfo().incrementClassLevel(static_cast<Class>(jediClass));

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

	debug("Script SkillCheck: %s, Skill %d, Rank %d, Roll %d -> %s (DC %d)",
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

	debug("Script AbilityCheck: %s, Ability %d, Mod %d, Roll %d -> %s (DC %d)",
	      creature->getTag().c_str(), nAbility, mod, roll, (roll + mod >= nDC) ? "SUCCESS" : "FAILURE", nDC);
}

void Functions::getHasFeatEffect(Aurora::NWScript::FunctionContext &ctx) {
	getHasFeat(ctx);
}

void Functions::talentSpell(Aurora::NWScript::FunctionContext &ctx) {
	const int spellID = ctx.getParams()[0].getInt();
	ctx.getReturn() = new Talent(kTalentTypeSpell, spellID);
}

void Functions::talentFeat(Aurora::NWScript::FunctionContext &ctx) {
	const int featID = ctx.getParams()[0].getInt();
	ctx.getReturn() = new Talent(kTalentTypeFeat, featID);
}

void Functions::talentSkill(Aurora::NWScript::FunctionContext &ctx) {
	const int skillID = ctx.getParams()[0].getInt();
	ctx.getReturn() = new Talent(kTalentTypeSkill, skillID);
}

void Functions::getCreatureHasTalent(Aurora::NWScript::FunctionContext &ctx) {
	const Talent *talent = ObjectContainer::toTalent(ctx.getParams()[0].getEngineType());
	const Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 1));
	if (!talent || !talent->isValid() || !creature) {
		ctx.getReturn() = 0;
		return;
	}

	switch (talent->getType()) {
		case kTalentTypeFeat:
			ctx.getReturn() = static_cast<int32_t>(creature->hasFeat(static_cast<uint32_t>(talent->getID())));
			return;

		case kTalentTypeSkill: {
			const int skillID = talent->getID();
			if (skillID < kSkillComputerUse || skillID >= kSkillMAX) {
				ctx.getReturn() = 0;
				return;
			}

			ctx.getReturn() = static_cast<int32_t>(
				creature->getCreatureInfo().getSkillRank(static_cast<Skill>(skillID)) > 0);
			return;
		}

		case kTalentTypeSpell:
		case kTalentTypeInvalid:
		default:
			ctx.getReturn() = 0;
			return;
	}
}

void Functions::getCreatureTalentRandom(Aurora::NWScript::FunctionContext &ctx) {
	const int category = ctx.getParams()[0].getInt();
	const Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 1));
	if (!creature) {
		ctx.getReturn() = createInvalidTalent();
		return;
	}

	// Ignore visibility gating until combat perception masks are implemented.
	const std::vector<uint32_t> &feats = creature->getCreatureInfo().getFeats();
	if ((category == kTalentTypeFeat || category == kTalentTypeInvalid) && !feats.empty()) {
		const int index = getRandom(0, static_cast<int>(feats.size()) - 1);
		ctx.getReturn() = new Talent(kTalentTypeFeat, static_cast<int>(feats[index]));
		return;
	}

	if ((category == kTalentTypeSkill || category == kTalentTypeInvalid) && hasAnySkillTalent(creature)) {
		std::vector<int> skills;
		for (int skill = kSkillComputerUse; skill < kSkillMAX; ++skill) {
			if (creature->getCreatureInfo().getSkillRank(static_cast<Skill>(skill)) > 0)
				skills.push_back(skill);
		}

		if (!skills.empty()) {
			const int index = getRandom(0, static_cast<int>(skills.size()) - 1);
			ctx.getReturn() = new Talent(kTalentTypeSkill, skills[index]);
			return;
		}
	}

	ctx.getReturn() = createInvalidTalent();
}

void Functions::getCreatureTalentBest(Aurora::NWScript::FunctionContext &ctx) {
	const int category = ctx.getParams()[0].getInt();
	const Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 2));
	if (!creature) {
		ctx.getReturn() = createInvalidTalent();
		return;
	}

	// nCRMax/nClass/bOffensive filters are ignored until full feat/spell
	// metadata and combat scoring are implemented.
	const std::vector<uint32_t> &feats = creature->getCreatureInfo().getFeats();
	if ((category == kTalentTypeFeat || category == kTalentTypeInvalid) && !feats.empty()) {
		ctx.getReturn() = new Talent(kTalentTypeFeat, static_cast<int>(feats.front()));
		return;
	}

	if (category == kTalentTypeSkill || category == kTalentTypeInvalid) {
		const int bestSkill = getBestSkillTalentID(creature);
		if (bestSkill >= 0) {
			ctx.getReturn() = new Talent(kTalentTypeSkill, bestSkill);
			return;
		}
	}

	ctx.getReturn() = createInvalidTalent();
}

void Functions::getIsTalentValid(Aurora::NWScript::FunctionContext &ctx) {
	const Talent *talent = ObjectContainer::toTalent(ctx.getParams()[0].getEngineType());
	ctx.getReturn() = talent ? static_cast<int32_t>(talent->isValid()) : 0;
}

void Functions::getTypeFromTalent(Aurora::NWScript::FunctionContext &ctx) {
	const Talent *talent = ObjectContainer::toTalent(ctx.getParams()[0].getEngineType());
	ctx.getReturn() = talent ? static_cast<int32_t>(talent->getType()) : 0;
}

void Functions::getIdFromTalent(Aurora::NWScript::FunctionContext &ctx) {
	const Talent *talent = ObjectContainer::toTalent(ctx.getParams()[0].getEngineType());
	ctx.getReturn() = talent ? static_cast<int32_t>(talent->getID()) : -1;
}

void Functions::getCategoryFromTalent(Aurora::NWScript::FunctionContext &ctx) {
	const Talent *talent = ObjectContainer::toTalent(ctx.getParams()[0].getEngineType());
	ctx.getReturn() = talent ? static_cast<int32_t>(talent->getCategory()) : 0;
}

void Functions::getLastCombatFeatUsed(Aurora::NWScript::FunctionContext &ctx) {
	const Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	ctx.getReturn() = creature ? creature->getLastCombatFeatUsed() : -1;
}

void Functions::getAbilityScore(Aurora::NWScript::FunctionContext &ctx) {
	int nAbilityType = ctx.getParams()[0].getInt();
	Aurora::NWScript::Object *object = ctx.getParams()[1].getObject();

	Creature *creature = ObjectContainer::toCreature(object);

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
	ctx.getReturn() = new Effect(kEffectHeal, amount);
}

void Functions::effectDamage(Aurora::NWScript::FunctionContext &ctx) {
	int amount = ctx.getParams()[0].getInt();
	int damageType = ctx.getParams()[1].getInt();
	// param 2 (damage power) is not used in the basic implementation
	ctx.getReturn() = new Effect(kEffectDamage, amount, damageType);
}

void Functions::effectVisualEffect(Aurora::NWScript::FunctionContext &ctx) {
	const int visualEffect = ctx.getParams()[0].getInt();
	const int exposeToNetwork = ctx.getParams()[1].getInt();
	ctx.getReturn() = new Effect(kEffectVisual, visualEffect, exposeToNetwork);
}

void Functions::applyEffectToObject(Aurora::NWScript::FunctionContext &ctx) {
	// int nDurationType, effect eEffect, object oTarget, float fDuration=0.0
	const Effect *effect = dynamic_cast<const Effect *>(ctx.getParams()[1].getEngineType());
	Object *target = ObjectContainer::toObject(ctx.getParams()[2].getObject());

	if (!effect || !target)
		return;

	int current = target->getCurrentHitPoints();
	if (targetCreature) {
		targetCreature->applyEffect(*effect);
	} else {
		// Fallback for non-creature objects (e.g. placeable damage)
		int current = target->getCurrentHitPoints();
		if (effect->getType() == kEffectDamage) {
			target->setCurrentHitPoints(MAX(0, current - effect->getAmount()));
		} else if (effect->getType() == kEffectHeal) {
			target->setCurrentHitPoints(MIN(target->getMaxHitPoints(), current + effect->getAmount()));
		}
	}
}

void Functions::getGoodEvilValue(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *rawParam = ctx.getParams()[0].getObject();
	Object *obj = ObjectContainer::toObject(rawParam ? rawParam : ctx.getCaller());

	ctx.getReturn() = obj ? obj->getGoodEvilValue() : 50;
}

void Functions::getAlignmentGoodEvil(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *rawParam = ctx.getParams()[0].getObject();
	Object *obj = ObjectContainer::toObject(rawParam ? rawParam : ctx.getCaller());

	// Return alignment constant: 1 = good (>70), 0 = neutral (30..70), 2 = evil (<30)
	int value = obj ? obj->getGoodEvilValue() : 50;
	if (value > 70)
		ctx.getReturn() = 1; // ALIGNMENT_GOOD
	else if (value < 30)
		ctx.getReturn() = 2; // ALIGNMENT_EVIL
	else
		ctx.getReturn() = 0; // ALIGNMENT_NEUTRAL
}

void Functions::adjustAlignment(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *rawTarget = ctx.getParams()[0].getObject();
	Object *target = ObjectContainer::toObject(rawTarget ? rawTarget : ctx.getCaller());
	// param 1: alignment shift direction (0=ALIGNMENT_GOOD, 1=ALIGNMENT_EVIL)
	// param 2: amount
	int direction = ctx.getParams()[1].getInt();
	int amount    = ctx.getParams()[2].getInt();

	if (!target || amount <= 0)
		return;

	int delta = (direction == 0) ? amount : -amount;
	target->adjustGoodEvilValue(delta);
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

	// Fortitude = 10 + Constitution modifier + class save bonus
	int conMod = creature->getCreatureInfo().getAbilityModifier(kAbilityConstitution);
	int roll   = std::rand() % 20 + 1;

	if (roll == 20) {
		ctx.getReturn() = 2; // automatic success
		return;
	}
	if (roll == 1) {
		ctx.getReturn() = 0; // automatic failure
		return;
	}
	ctx.getReturn() = ((roll + conMod + 10) >= dc) ? 1 : 0;
}

void Functions::reflexSave(Aurora::NWScript::FunctionContext &ctx) {
	// ReflexSave(object oCreature, int nDC, int nSaveType=0, object oSaveVersus=OBJECT_SELF)
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	int dc = ctx.getParams()[1].getInt();

	if (!creature) {
		ctx.getReturn() = 0;
		return;
	}

	int dexMod = creature->getCreatureInfo().getAbilityModifier(kAbilityDexterity);
	int roll   = std::rand() % 20 + 1;

	if (roll == 20) {
		ctx.getReturn() = 2;
		return;
	}
	if (roll == 1) {
		ctx.getReturn() = 0;
		return;
	}
	ctx.getReturn() = ((roll + dexMod + 10) >= dc) ? 1 : 0;
}

void Functions::willSave(Aurora::NWScript::FunctionContext &ctx) {
	// WillSave(object oCreature, int nDC, int nSaveType=0, object oSaveVersus=OBJECT_SELF)
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	int dc = ctx.getParams()[1].getInt();

	if (!creature) {
		ctx.getReturn() = 0;
		return;
	}

	int wisMod = creature->getCreatureInfo().getAbilityModifier(kAbilityWisdom);
	int roll   = std::rand() % 20 + 1;

	if (roll == 20) {
		ctx.getReturn() = 2;
		return;
	}
	if (roll == 1) {
		ctx.getReturn() = 0;
		return;
	}
	ctx.getReturn() = ((roll + wisMod + 10) >= dc) ? 1 : 0;
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

	warning("Functions::givePlotXP: plot '%s', %d%%", plotName.c_str(), percentage);

	// Award to all active party members
	Creature *pc = _game->getModule().getPC();
	if (pc)
		pc->addPlotXP(percentage);
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
		debug("Stealth XP awarded: %d", xp);
		
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
	ctx.getReturn() = new Effect(kEffectACIncrease, bonus);
}

void Functions::effectAttackIncrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectAttackIncrease(int nBonus, int nModifierType=0)
	int bonus = ctx.getParams()[0].getInt();
	ctx.getReturn() = new Effect(kEffectAttackIncrease, bonus);
}

void Functions::effectSkillIncrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectSkillIncrease(int nSkill, int nValue)
	int skill  = ctx.getParams()[0].getInt();
	int amount = ctx.getParams()[1].getInt();
	ctx.getReturn() = new Effect(kEffectSkillIncrease, amount, skill);
}

void Functions::effectTemporaryHitpoints(Aurora::NWScript::FunctionContext &ctx) {
	// EffectTemporaryHitpoints(int nHitPoints)
	int amount = ctx.getParams()[0].getInt();
	ctx.getReturn() = new Effect(kEffectTemporaryHitpoints, amount);
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
	ctx.getReturn() = new Effect(kEffectDeath, 0);
}

void Functions::effectKnockdown(Aurora::NWScript::FunctionContext &ctx) {
	// EffectKnockdown() — knocks target prone, losing any Dex bonus to AC.
	ctx.getReturn() = new Effect(kEffectKnockdown, 0);
}

void Functions::effectParalyze(Aurora::NWScript::FunctionContext &ctx) {
	// EffectParalyze() — used by Force Stasis; target cannot act or dodge.
	ctx.getReturn() = new Effect(kEffectParalyze, 0);
}

void Functions::effectStunned(Aurora::NWScript::FunctionContext &ctx) {
	// EffectStunned() — stuns target (interrupts action queue).
	ctx.getReturn() = new Effect(kEffectStunned, 0);
}

void Functions::effectHaste(Aurora::NWScript::FunctionContext &ctx) {
	// EffectHaste() — grants one extra attack per round.
	ctx.getReturn() = new Effect(kEffectHaste, 0);
}

void Functions::effectAbilityIncrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectAbilityIncrease(int nAbilityToIncrease, int nModifyBy)
	int ability  = ctx.getParams()[0].getInt();
	int modifyBy = ctx.getParams()[1].getInt();
	// Pack ability type into damageType field, amount into amount field.
	ctx.getReturn() = new Effect(kEffectAbilityIncrease, modifyBy, ability);
}

void Functions::effectMovementSpeedIncrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectMovementSpeedIncrease(int nNewSpeedPercent)
	int percent = ctx.getParams()[0].getInt();
	ctx.getReturn() = new Effect(kEffectMovementSpeedIncrease, percent);
}

void Functions::effectResurrection(Aurora::NWScript::FunctionContext &ctx) {
	// EffectResurrection() — revives a dead creature at 1 HP.
	ctx.getReturn() = new Effect(kEffectResurrection, 1);
}

void Functions::effectCutSceneHorrified(Aurora::NWScript::FunctionContext &ctx) {
	// Cut-scene variant: treat as hard crowd-control for script progression.
	ctx.getReturn() = new Effect(kEffectStunned, 0);
}

void Functions::effectCutSceneParalyze(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = new Effect(kEffectParalyze, 0);
}

void Functions::effectCutSceneStunned(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = new Effect(kEffectStunned, 0);
}

void Functions::effectForcePushed(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = new Effect(kEffectForcePush, 0);
}

void Functions::effectForcePushTargeted(Aurora::NWScript::FunctionContext &ctx) {
	// Object oSource is usually the person who cast it.
	ctx.getReturn() = new Effect(kEffectForcePush, 0);
}

void Functions::effectForceDrain(Aurora::NWScript::FunctionContext &ctx) {
	int amount = ctx.getParams()[0].getInt();
	ctx.getReturn() = new Effect(kEffectForceDrain, amount);
}

void Functions::effectForceShield(Aurora::NWScript::FunctionContext &ctx) {
	int amount = ctx.getParams()[0].getInt();
	ctx.getReturn() = new Effect(kEffectForceShield, amount);
}

void Functions::effectHealForcePoints(Aurora::NWScript::FunctionContext &ctx) {
	int amount = ctx.getParams()[0].getInt();
	ctx.getReturn() = new Effect(kEffectForcePointsHeal, amount);
}

void Functions::effectDamageForcePoints(Aurora::NWScript::FunctionContext &ctx) {
	int amount = ctx.getParams()[0].getInt();
	ctx.getReturn() = new Effect(kEffectForcePointsDamage, amount);
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
	// Spell state is not fully tracked globally yet. Standard fallback is -1.
	ctx.getReturn() = -1;
}

void Functions::getLastSpellHarmful(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = 0;
}

void Functions::getSpellTargetLocation(Aurora::NWScript::FunctionContext &ctx) {
	Location loc;
	ctx.getReturn() = loc;
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
	ctx.getReturn() = new Effect(kEffectSpeed, (float)attacks); // Simplified as speed effect for now
}

void Functions::effectLightsaberThrow(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = new Effect(kEffectVFX, 100); // Dummy light side VFX as placeholder
}

void Functions::effectWhirlWind(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = new Effect(kEffectStun, 6.0f); // Whirlwind as stun for now
}


void Functions::setEffectIcon(Aurora::NWScript::FunctionContext &ctx) {}
void Functions::getFactionLeader(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = (Aurora::NWScript::Object *)nullptr; }
void Functions::setNPCAIStyle(Aurora::NWScript::FunctionContext &ctx) {}
void Functions::setNPCSelectability(Aurora::NWScript::FunctionContext &ctx) {}
void Functions::getNPCSelectability(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 1; }
void Functions::getIsDebilitated(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 0; }
void Functions::getFirstAttacker(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = (Aurora::NWScript::Object *)nullptr; }
void Functions::getNextAttacker(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = (Aurora::NWScript::Object *)nullptr; }
void Functions::playRoomAnimation(Aurora::NWScript::FunctionContext &ctx) {}
void Functions::effectPsychicStatic(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = new Effect(kEffectVFX, 0); }
void Functions::playVisualAreaEffect(Aurora::NWScript::FunctionContext &ctx) {}
void Functions::aurPostString(Aurora::NWScript::FunctionContext &ctx) {
	Common::UString str = ctx.getParams()[0].getString();
	debugC(Common::kDebugEngineLogic, 1, "[AUR POST] %s", str.c_str());
}


void Functions::getLastItemEquipped(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = (Aurora::NWScript::Object *)nullptr; }
void Functions::getSubScreenID(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 0; }
void Functions::getCasterLevel(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 1; }
void Functions::resistForce(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 0; }
void Functions::getLastSpellCaster(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = (Aurora::NWScript::Object *)nullptr; }
void Functions::getLastSpell(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 0; }
void Functions::effectConfused(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = new Effect(kEffectConfusion, 0); }
void Functions::effectFrightened(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = new Effect(kEffectDazed, 0); }
void Functions::effectChoke(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = new Effect(kEffectStun, 0); }


// ---------------------------------------------------------------------------
// Effect constructors -- saving throw / damage reduction / invisibility
// ---------------------------------------------------------------------------

void Functions::effectSavingThrowIncrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectSavingThrowIncrease(int nSavingThrow, int nValue, int nSavingThrowType=0)
	int savingThrow = ctx.getParams()[0].getInt();
	int value       = ctx.getParams()[1].getInt();
	ctx.getReturn() = new Effect(kEffectSavingThrowIncrease, value, savingThrow);
}

void Functions::effectDamageReduction(Aurora::NWScript::FunctionContext &ctx) {
	// EffectDamageReduction(int nEnhancement, int nDmgReductionType, int nAmount)
	int amount = ctx.getParams()[2].getInt();
	ctx.getReturn() = new Effect(kEffectDamageReduction, amount);
}

void Functions::effectInvisibility(Aurora::NWScript::FunctionContext &ctx) {
	// EffectInvisibility(int nInvisibilityType)
	ctx.getReturn() = new Effect(kEffectInvisibility, ctx.getParams()[0].getInt());
}

void Functions::effectSeeInvisible(Aurora::NWScript::FunctionContext &ctx) {
	// EffectSeeInvisible() -- grants ability to see invisible creatures.
	ctx.getReturn() = new Effect(kEffectInvisibility, 0);
}

// ---------------------------------------------------------------------------
// Effect constructors -- attribute / combat decreasers
// ---------------------------------------------------------------------------

void Functions::effectAbilityDecrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectAbilityDecrease(int nAbility, int nModifyBy)
	int ability  = ctx.getParams()[0].getInt();
	int modifyBy = ctx.getParams()[1].getInt();
	ctx.getReturn() = new Effect(kEffectAbilityDecrease, modifyBy, ability);
}

void Functions::effectAttackDecrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectAttackDecrease(int nPenalty, int nModifierType=0)
	ctx.getReturn() = new Effect(kEffectAttackDecrease, ctx.getParams()[0].getInt());
}

void Functions::effectDamageDecrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectDamageDecrease(int nPenalty, int nDamageType=8)
	ctx.getReturn() = new Effect(kEffectDamageDecrease, ctx.getParams()[0].getInt());
}

void Functions::effectDamageImmunityDecrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectDamageImmunityDecrease(int nDamageType, int nPercentImmunity)
	ctx.getReturn() = new Effect(kEffectDamageImmunityDecrease, ctx.getParams()[1].getInt());
}

void Functions::effectACDecrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectACDecrease(int nValue, int nModifyType=0, int nDamageType=8199)
	ctx.getReturn() = new Effect(kEffectACDecrease, ctx.getParams()[0].getInt());
}

void Functions::effectMovementSpeedDecrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectMovementSpeedDecrease(int nPercentChange)
	ctx.getReturn() = new Effect(kEffectMovementSpeedDecrease, ctx.getParams()[0].getInt());
}

void Functions::effectSavingThrowDecrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectSavingThrowDecrease(int nSave, int nValue, int nSaveType=0)
	int savingThrow = ctx.getParams()[0].getInt();
	int value       = ctx.getParams()[1].getInt();
	ctx.getReturn() = new Effect(kEffectSavingThrowDecrease, value, savingThrow);
}

void Functions::effectSkillDecrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectSkillDecrease(int nSkill, int nValue)
	int skill  = ctx.getParams()[0].getInt();
	int amount = ctx.getParams()[1].getInt();
	ctx.getReturn() = new Effect(kEffectSkillDecrease, amount, skill);
}

void Functions::effectForceResistanceDecrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectForceResistanceDecrease(int nValue)
	ctx.getReturn() = new Effect(kEffectForceResistanceDecrease, ctx.getParams()[0].getInt());
}

// ---------------------------------------------------------------------------
// Force power tracking
// ---------------------------------------------------------------------------

void Functions::getLastForcePowerUsed(Aurora::NWScript::FunctionContext &ctx) {
	// GetLastForcePowerUsed() -> int
	// Returns the ID of the last force power used by the calling creature.
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	ctx.getReturn() = caller ? caller->getLastForcePowerUsed() : -1;
}

} // End of namespace KotORBase
} // End of namespace Engines
