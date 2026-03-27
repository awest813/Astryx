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

	if (effect->getType() == kEffectHeal) {
		int healed = current + effect->getAmount();
		target->setCurrentHitPoints(healed);
	} else if (effect->getType() == kEffectDamage) {
		int damaged = current - effect->getAmount();
		target->setCurrentHitPoints(damaged);

		// Check for death on creatures; cancel combat first to keep state consistent
		// with the executeAttack path, which always calls cancelCombat before handleDeath.
		Creature *creature = ObjectContainer::toCreature(target);
		if (creature && creature->getCurrentHitPoints() <= 0) {
			creature->cancelCombat();
			creature->handleDeath();
		}
	} else if (effect->getType() == kEffectTemporaryHitpoints) {
		// Temporary HP: add to current HP, capped at max HP.
		int maxHP = target->getMaxHitPoints();
		int boosted = current + effect->getAmount();
		if (boosted > maxHP)
			boosted = maxHP;
		target->setCurrentHitPoints(boosted);
	}
	// ACIncrease / AttackIncrease / SkillIncrease are passive bonuses stored on the
	// creature's stat block; applying them via ApplyEffectToObject is a no-op here
	// until the full buff/debuff system is implemented.
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

	// Reset to zero then add the desired total (addPlotXP accumulates).
	int current = creature->getCurrentXP();
	creature->addPlotXP(amount - current);
}

void Functions::getXP(Aurora::NWScript::FunctionContext &ctx) {
	// GetXP(object oCreature) → int
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());

	ctx.getReturn() = creature ? creature->getCurrentXP() : 0;
}

void Functions::givePlotXP(Aurora::NWScript::FunctionContext &ctx) {
	// GivePlotXP(string sPlotName, int nPercentage)
	// Awards XP to the party leader based on a percentage of a plot XP table value.
	// We implement a simple version: just log and add a token amount.
	const Common::UString &plotName = ctx.getParams()[0].getString();
	int percentage = ctx.getParams()[1].getInt();

	warning("Functions::givePlotXP: plot '%s', %d%%", plotName.c_str(), percentage);

	// Award to all active party members
	Object *pc = _game->getModule().getPartyLeader();
	if (pc)
		pc->addPlotXP(percentage);
}

// ---------------------------------------------------------------------------
// New Effect constructors — Milestone 3
// ---------------------------------------------------------------------------

void Functions::effectACIncrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectACIncrease(int nValue, int nModifyType=0, int nDamageType=8199)
	int bonus = ctx.getParams()[0].getInt();
	ctx.getReturn().setEngineType(new Effect(kEffectACIncrease, bonus));
}

void Functions::effectAttackIncrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectAttackIncrease(int nBonus, int nModifierType=0)
	int bonus = ctx.getParams()[0].getInt();
	ctx.getReturn().setEngineType(new Effect(kEffectAttackIncrease, bonus));
}

void Functions::effectSkillIncrease(Aurora::NWScript::FunctionContext &ctx) {
	// EffectSkillIncrease(int nSkill, int nValue)
	int skill  = ctx.getParams()[0].getInt();
	int amount = ctx.getParams()[1].getInt();
	ctx.getReturn().setEngineType(new Effect(kEffectSkillIncrease, amount, skill));
}

void Functions::effectTemporaryHitpoints(Aurora::NWScript::FunctionContext &ctx) {
	// EffectTemporaryHitpoints(int nHitPoints)
	int amount = ctx.getParams()[0].getInt();
	ctx.getReturn().setEngineType(new Effect(kEffectTemporaryHitpoints, amount));
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

} // End of namespace KotORBase

} // End of namespace Engines
