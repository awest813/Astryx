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
 *  Creature action executor for KotOR games.
 */

#include "external/glm/vec3.hpp"
#include "external/glm/gtc/type_ptr.hpp"

#include "src/common/maths.h"

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/actionexecutor.h"
#include "src/common/random.h"
#include "src/common/debug.h"
#include "src/aurora/2dafile.h"
#include "src/aurora/2dareg.h"

#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/door.h"
#include "src/engines/kotorbase/placeable.h"
#include "src/engines/kotorbase/inventory.h"
#include "src/engines/kotorbase/animationnames.h"

static const float kWalkDistance = 2.0f;

namespace Engines {

namespace KotORBase {
std::map<uint32_t, ActionExecutor::SpellInfo> ActionExecutor::_spells;
bool ActionExecutor::_spellsLoaded = false;


void ActionExecutor::execute(Action &action, const ExecutionContext &ctx) {
	if (!action.initialized) {
		if (action.type != kActionPlayAnimation)
			action.startTime = 0.0f;
		action.initialized = true;
	}

	switch (action.type) {
		case kActionMoveToPoint:
			executeMoveToPoint(action, ctx);
			break;
		case kActionFollowLeader:
			executeFollowLeader(action, ctx);
			break;
		case kActionOpenLock:
			executeOpenLock(action, ctx);
			break;
		case kActionUseObject:
			executeUseObject(action, ctx);
			break;
		case kActionAttackObject:
			executeAttackObject(action, ctx);
			break;
		case kActionPickUpItem:
			executePickUpItem(action, ctx);
			break;
		case kActionWait:
			executeWait(action, ctx);
			break;
		case kActionCastSpell:
			executeCastSpell(action, ctx);
			break;
		case kActionCutsceneAttack:
			executeCutsceneAttack(action, ctx);
			break;
		case kActionPlayAnimation:
			executePlayAnimation(action, ctx);
			break;
		case kActionOpenDoor:
			if (action.object) {
				Door *door = ObjectContainer::toDoor(action.object);
				if (door)
					door->open(ctx.creature);
			}
			ctx.creature->popAction();
			break;
		case kActionGiveItem:
		case kActionTakeItem:
			ctx.creature->popAction();
			break;
		default:
			ctx.creature->popAction();
			debugC(Common::kDebugEngineLogic, 1, "ActionExecutor: unhandled action type %u", (uint)action.type);
			break;
	}
}

void ActionExecutor::executeMoveToPoint(Action &action, const ExecutionContext &ctx) {
	if (moveTo(action.location, action.range, ctx, action.choreographyFlags != 0))
		ctx.creature->popAction();
}

void ActionExecutor::executeFollowLeader(Action &action, const ExecutionContext &ctx) {
	Creature *partyLeader = ctx.area->_module->getPartyLeader();

	float x, y, _;
	partyLeader->getPosition(x, y, _);

	moveTo(glm::vec2(x, y), action.range, ctx);
}

void ActionExecutor::executeOpenLock(Action &action, const ExecutionContext &ctx) {
	float x, y, _;
	action.object->getPosition(x, y, _);

	if (!moveTo(glm::vec2(x, y), action.range, ctx))
		return;

	ctx.creature->popAction();

	if (ctx.creature != ctx.area->_module->getPartyLeader()) {
		warning("ActionExecutor::executeOpenLock(): Creature is not the party leader");
		return;
	}

	Door *door = ObjectContainer::toDoor(action.object);
	Placeable *placeable = ObjectContainer::toPlaceable(action.object);
	Situated *situated = door ? (Situated *)door : (Situated *)placeable;

	if (!situated) {
		warning("Cannot unlock an object that is not a door or a placeable");
		return;
	}

	if (situated->isKeyRequired()) {
		Common::UString keyTag = situated->getKeyTag();
		if (ctx.creature->getInventory().hasItem(keyTag)) {
			ctx.creature->playAnimation(door ? "unlockdr" : "unlockcntr", false);
			situated->setLocked(false);
			if (door) door->open(ctx.creature);
			info("Object unlocked with key: %s", keyTag.c_str());
		} else {
			status("ActionSpeakString [%s]: A specific key is required to open this.", ctx.creature->getTag().c_str());
		}
		return;
	}

	// Security skill check: d20 + Security Rank vs LockDC
	int securityRank = ctx.creature->getSkillRank(kSkillSecurity);
	int d20Roll = RNG.getNext(1, 21);
	int total = d20Roll + securityRank;
	int dc = situated->getLockDC();

	if (total >= dc) {
		ctx.creature->playAnimation(door ? "unlockdr" : "unlockcntr", false);
		situated->setLocked(false);
		if (door) door->open(ctx.creature);
		info("Security Check SUCCESS: %d + %d = %d vs DC %d", d20Roll, securityRank, total, dc);
	} else {
		status("ActionSpeakString [%s]: Security check failed.", ctx.creature->getTag().c_str());
		info("Security Check FAILURE: %d + %d = %d vs DC %d", d20Roll, securityRank, total, dc);
	}
}

void ActionExecutor::executeUseObject(Action &action, const ExecutionContext &ctx) {
	float x, y, _;
	action.object->getPosition(x, y, _);

	if (!moveTo(glm::vec2(x, y), action.range, ctx))
		return;

	ctx.creature->popAction();

	Module *module = ctx.area->_module;

	if (ctx.creature != module->getPartyLeader()) {
		warning("ActionExecutor::executeUseObject(): Creature is not the party leader");
		return;
	}

	action.object->click(module->getPartyLeader());

	Creature *creatureObject = ObjectContainer::toCreature(action.object);
	if (creatureObject) {
		const Common::UString conversation = creatureObject->getConversation();
		if (!conversation.empty())
			module->delayConversation(conversation, creatureObject);

		return;
	}

	Situated *situated = ObjectContainer::toSituated(action.object);
	if (situated) {
		const Common::UString conversation = situated->getConversation();
		if (!conversation.empty()) {
			module->delayConversation(conversation, situated);
			return;
		}

		Placeable *placeable = ObjectContainer::toPlaceable(action.object);
		if (placeable && placeable->hasInventory()) {
			module->delayContainer(placeable);
			return;
		}
	}
}

void ActionExecutor::executeAttackObject(Action &action, const ExecutionContext &ctx) {
	float x, y, _;
	action.object->getPosition(x, y, _);

	if (!moveTo(glm::vec2(x, y), ctx.creature->getMaxAttackRange(), ctx))
		return;

	ctx.creature->popAction();
	
	if (action.choreographyFlags > 0)
		ctx.creature->performCutsceneAttack(action.object, action.choreographyFlags);
	else
		ctx.creature->startCombat(action.object, ctx.area->_module->getNextCombatRound());
}

void ActionExecutor::executePickUpItem(Action &action, const ExecutionContext &ctx) {
	if (!action.object)
		return;

	float x, y, _;
	action.object->getPosition(x, y, _);

	if (!moveTo(glm::vec2(x, y), action.range, ctx))
		return;

	Common::UString itemResRef = action.object->getTemplateResRef();
	if (itemResRef.empty())
		itemResRef = action.object->getTag();
	
	Object *itemObject = action.object;

	ctx.creature->popAction();
	if (!itemResRef.empty()) {
		ctx.creature->getInventory().addItem(itemResRef);
	}
	ctx.area->removeObject(itemObject);
}

bool ActionExecutor::isLocationReached(const glm::vec2 &location, float range, const ExecutionContext &ctx) {
	float x, y, _;
	ctx.creature->getPosition(x, y, _);
	return glm::distance(glm::vec2(x, y), location) <= range;
}

bool ActionExecutor::moveTo(const glm::vec2 &location, float range, const ExecutionContext &ctx, bool forceRun) {
	if (isLocationReached(location, range, ctx))
		return true;

	ctx.creature->makeLookAt(location.x, location.y);

	float oX, oY, oZ;
	ctx.creature->getPosition(oX, oY, oZ);
	glm::vec2 origin(oX, oY);

	glm::vec2 diff = location - origin;
	glm::vec2 dir = glm::normalize(diff);

	float dist = glm::length(diff);
	bool run = forceRun || dist > kWalkDistance;
	float moveRate = run ? ctx.creature->getRunRate() : ctx.creature->getWalkRate();

	float x = origin.x + moveRate * dir.x * ctx.frameTime;
	float y = origin.y + moveRate * dir.y * ctx.frameTime;
	float z = ctx.area->evaluateElevation(x, y);

	bool haveMovement = (z != FLT_MIN) &&
	                     ctx.area->walkable(glm::vec3(oX, oY, oZ + 0.1f),
	                                        glm::vec3(x, y, z + 0.1f));

	if (haveMovement) {
		ctx.creature->playAnimation(run ? "run" : "walk", false, -1.0f);
		ctx.creature->setPosition(x, y, z);

		if (ctx.creature == ctx.area->_module->getPartyLeader())
			ctx.area->_module->movedPartyLeader();
		else
			ctx.area->notifyObjectMoved(*ctx.creature);

		diff = location - glm::vec2(x, y);
		dist = glm::length(diff);

		if (dist <= range) {
			ctx.creature->playDefaultAnimation();
			return true;
		}

	} else {
		ctx.creature->playDefaultAnimation();
	}

	return false;
}

void ActionExecutor::executeWait(Action &action, const ExecutionContext &ctx) {
	action.startTime += ctx.frameTime;

	if (action.startTime >= action.range)
		ctx.creature->popAction();
}

void ActionExecutor::executeCastSpell(Action &action, const ExecutionContext &ctx) {
	Creature *caster = ctx.creature;
	int cost = 0;

	if (!_spellsLoaded)
		loadSpells();

	const SpellInfo *spell = getSpellInfo(action.actionID);
	if (spell)
		cost = spell->cost;

	// Alignment adjustments (KotOR rule: Opposite side costs more)
	int alignment = caster->getAlignment(); // 0-100, 100 is Light, 0 is Dark
	if (spell) {
		// Simplified heuristic: Hostile powers are Dark (category 3), 
		// Non-hostile are Light (category 2). In real game we use category column.
		if (spell->hostile) {
			// Dark Side Power
			if (alignment > 60)      cost = (cost * 3) / 2; // +50% for Light side users
			else if (alignment < 40) cost = (cost * 3) / 4; // -25% for Dark side users
		} else {
			// Light Side / Neutral (Simplified)
			if (alignment < 40)      cost = (cost * 3) / 2; // +50% for Dark side users
			else if (alignment > 60) cost = (cost * 3) / 4; // -25% for Light side users
		}
	}

	if (caster->getForcePoints() < cost) {
		debugC(Common::kDebugEngineLogic, 1, "Not enough Force Points to cast power %d", action.actionID);
		caster->popAction();
		return;
	}

	caster->setForcePoints(caster->getForcePoints() - cost);
	caster->setLastForcePowerUsed(action.actionID);

	// Apply power effects
	switch (action.actionID) {
		case 1: // Force Heal (Party heal)
			caster->playAnimation("castheal", false);
			for (int i = 0; i < ctx.area->_module->getPartyMemberCount(); ++i) {
				Creature *member = ctx.area->_module->getPartyMemberByIndex(i);
				if (member && !member->isDead())
					member->applyEffect(kEffectHeal, 0.0f, 15, action.actionID);
			}
			break;

		case 5: // Force Stun (Single target)
			if (action.object) {
				Creature *target = ObjectContainer::toCreature(action.object);
				if (target) {
					caster->playAnimation("castout", false);
					int level = caster->getHitDice();
					int dc = 10 + level + caster->getCreatureInfo().getAbilityModifier(kAbilityWisdom);
					if (!target->rollSavingThrow(kSavingThrowWill, dc)) {
						target->applyEffect(kEffectStun, 9.0f, 0, action.actionID);
						debugC(Common::kDebugEngineLogic, 1, "Force Stun SUCCESS on %s", target->getTag().c_str());
					}
				}
			}
			break;

		case 2: // Force Push (Single target)
			if (action.object) {
				Creature *target = ObjectContainer::toCreature(action.object);
				if (target) {
					caster->playAnimation("castout", false);
					int level = caster->getHitDice();
					int dc = 10 + level + caster->getCreatureInfo().getAbilityModifier(kAbilityWisdom);
					if (!target->rollSavingThrow(kSavingThrowFortitude, dc)) {
						target->applyEffect(kEffectKnockdown, 3.0f, 0, action.actionID);
						target->applyEffect(kEffectDamage, 0.0f, level, action.actionID);
						debugC(Common::kDebugEngineLogic, 1, "Force Push SUCCESS on %s", target->getTag().c_str());
					} else {
						// Half damage on save
						target->applyEffect(kEffectDamage, 0.0f, level / 2, action.actionID);
					}
				}
			}
			break;

		case 3: // Burst of Speed (Self)
			caster->playAnimation("castself", false);
			caster->applyEffect(kEffectSpeed, 0.0f, 50, action.actionID);
			break;

		case 14: // Shock (Single target lightning)
		case 15: // Force Lightning (Cone/Radius)
			if (action.object) {
				Creature *target = ObjectContainer::toCreature(action.object);
				if (target) {
					caster->playAnimation("castout", false);
					int level = caster->getHitDice();
					int damage = level * 3; // Simplified 1d6 per level approx
					
					int dc = 10 + level + caster->getCreatureInfo().getAbilityModifier(kAbilityWisdom);
					bool saved = target->rollSavingThrow(kSavingThrowWill, dc);
					if (saved) damage /= 2;

					target->applyEffect(kEffectDamage, 0.0f, damage, action.actionID);
					// Visual effect hook would go here
					debugC(Common::kDebugEngineLogic, 1, "Force Lightning hit %s for %d", target->getTag().c_str(), damage);
				}
			}
			break;

		case 25: // Plague (Unstoppable DoT)
			if (action.object) {
				Creature *target = ObjectContainer::toCreature(action.object);
				if (target) {
					caster->playAnimation("castout", false);
					// Plague is special: DC 100 (Unstoppable)
					target->applyEffect(kEffectPoison, 12.0f, 5, action.actionID); // 5 dmg per sec for 12s
				}
			}
			break;

		case 33: // Wound
		case 34: // Choke
			if (action.object) {
				Creature *target = ObjectContainer::toCreature(action.object);
				if (target) {
					caster->playAnimation("castout", false);
					int level = caster->getHitDice();
					int dc = 10 + level + caster->getCreatureInfo().getAbilityModifier(kAbilityWisdom);
					
					if (!target->rollSavingThrow(kSavingThrowFortitude, dc)) {
						target->applyEffect(kEffectStun, 6.0f, 0, action.actionID);
						target->applyEffect(kEffectDamage, 0.0f, level * 2, action.actionID);
					}
				}
			}
			break;

		case 4: // Affect Mind
			if (action.object) {
				caster->playAnimation("castself", false);
				Creature *target = ObjectContainer::toCreature(action.object);
				if (target)
					target->applyEffect(kEffectStun, 6.0f, 0, action.actionID);
			}
			break;

		default:
			warning("ActionExecutor::executeCastSpell(): Unknown power ID %d", action.actionID);
			break;
	}

	caster->popAction();
}

void ActionExecutor::executePlayAnimation(Action &action, const ExecutionContext &ctx) {
	const Common::UString animName = getAnimationNameById(action.actionID);
	if (animName.empty()) {
		ctx.creature->popAction();
		return;
	}

	if (action.startTime < 0.0f) {
		float speed = action.range > 0.0f ? action.range : 1.0f;
		float length = -action.startTime;
		ctx.creature->playAnimation(animName, true, length, speed);
		action.startTime = length;
	}

	action.startTime -= ctx.frameTime;
	if (action.startTime <= 0.0f) {
		ctx.creature->playDefaultAnimation();
		ctx.creature->popAction();
	}
}

void ActionExecutor::executeCutsceneAttack(Action &action, const ExecutionContext &ctx) {
	float x, y, _;
	action.object->getPosition(x, y, _);

	if (!moveTo(glm::vec2(x, y), ctx.creature->getMaxAttackRange(), ctx))
		return;

	ctx.creature->popAction();
	ctx.creature->performCutsceneAttack(action.object, action.choreographyFlags);
}

void ActionExecutor::loadSpells() {
	if (_spellsLoaded)
		return;

	try {
		const Aurora::TwoDAFile &twoda = TwoDAReg.get2DA("spells");
		for (size_t i = 0; i < twoda.getRowCount(); ++i) {
			const Aurora::TwoDARow &row = twoda.getRow(i);
			
			SpellInfo info;
			info.id = i;
			info.label = row.getString("label");
			if (!row.empty("name"))
				info.nameStrRef = row.getInt("name");
			info.cost = row.getInt("forcepoints");
			info.impactScript = row.getString("impactscript");
			info.hostile = row.getInt("hostile") != 0;
			
			_spells[info.id] = info;
		}
	} catch (...) {
		warning("ActionExecutor::loadSpells(): Could not load spells.2da");
	}

	_spellsLoaded = true;
}

const ActionExecutor::SpellInfo *ActionExecutor::getSpellInfo(uint32_t id) {
	if (!_spellsLoaded)
		loadSpells();

	auto it = _spells.find(id);
	if (it != _spells.end())
		return &it->second;

	return nullptr;
}

} // End of namespace KotORBase

} // End of namespace Engines
