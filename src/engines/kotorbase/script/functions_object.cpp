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
 *  Star Wars: Knights of the Old Republic engine functions messing with objects.
 */

// TODO: check what happens on using invalid objects.

#include <algorithm>
#include <cmath>
#include <memory>

#include "src/common/util.h"

#include "src/aurora/nwscript/functioncontext.h"

#include "src/engines/kotorbase/types.h"
#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/placeable.h"
#include "src/engines/kotorbase/situated.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/location.h"
#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/creaturesearch.h"
#include "src/engines/kotorbase/effect.h"

#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

void Functions::getClickingObject(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = ctx.getTriggerer();
}

void Functions::getEnteringObject(Aurora::NWScript::FunctionContext &ctx) {
	// TODO: This should return the *last* entered object, i.e. it should remember past triggerers.
	ctx.getReturn() = ctx.getTriggerer();
}

void Functions::getExitingObject(Aurora::NWScript::FunctionContext &ctx) {
	// TODO: This should return the *last* exited object, i.e. it should remember past triggerers.
	ctx.getReturn() = ctx.getTriggerer();
}

void Functions::getIsObjectValid(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = getParamObject(ctx, 0) != 0;
}

void Functions::getIsPC(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = KotORBase::ObjectContainer::toPC(getParamObject(ctx, 0)) != 0;
}

void Functions::getObjectByTag(Aurora::NWScript::FunctionContext &ctx) {
	Common::UString name = ctx.getParams()[0].getString();
	int nth = ctx.getParams()[1].getInt();

	std::unique_ptr<Aurora::NWScript::ObjectSearch> search(_game->getModule().findObjectsByTag(name));
	for (int i = 0; i < nth; ++i) {
		search->next();
	}

	ctx.getReturn() = search->get();
}

void Functions::getMinOneHP(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *object = ctx.getParams()[0].getObject();

	Object *kotorObject = ObjectContainer::toObject(object);

	if (!kotorObject) {
		warning("Functions::getMinOneHP(): invalid object");
		ctx.getReturn() = 0;
		return;
	}

	ctx.getReturn() = kotorObject->getMinOneHitPoints();
}

void Functions::setMinOneHP(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *object = ctx.getParams()[0].getObject();
	bool enabled = ctx.getParams()[1].getInt();

	Object *kotorObject = ObjectContainer::toObject(object);

	if (!kotorObject) {
		warning("Functions::setMinOneHP(): invalid object");
		return;
	}

	kotorObject->setMinOneHitPoints(enabled);
}

void Functions::getCurrentHitPoints(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *object = ctx.getParams()[0].getObject();

	Object *kotorObject = ObjectContainer::toObject(object);

	ctx.getReturn() = kotorObject ? kotorObject->getCurrentHitPoints() : 0;
}

void Functions::getMaxHitPoints(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *object = ctx.getParams()[0].getObject();

	Object *kotorObject = ObjectContainer::toObject(object);

	ctx.getReturn() = kotorObject ? kotorObject->getMaxHitPoints() : 0;
}

void Functions::setMaxHitPoints(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *object = ctx.getParams()[0].getObject();
	int maxHitPoints = ctx.getParams()[1].getInt();

	if (maxHitPoints == 0)
		maxHitPoints = 1;

	Object *kotorObject = ObjectContainer::toObject(object);
	if (!kotorObject) {
		warning("Functions::setMaxHitPoints(): invalid object");
		return;
	}

	kotorObject->setCurrentHitPoints(maxHitPoints);
	kotorObject->setMaxHitPoints(maxHitPoints);
}

void Functions::getStandardFaction(Aurora::NWScript::FunctionContext &ctx) {
	const Object *object = ObjectContainer::toObject(ctx.getParams()[0].getObject());

	if (!object)
		ctx.getReturn() = kFactionInvalid;
	else
		ctx.getReturn() = object->getFaction();
}

void Functions::changeToStandardFaction(Aurora::NWScript::FunctionContext &ctx) {
	Object *object = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	int faction = ctx.getParams()[1].getInt();

	if (!object) {
		warning("Functions::changeToStandardFaction(): invalid object");
		return;
	}

	object->setFaction(Faction(faction));
}

void Functions::createItemOnObject(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString &itemTag = ctx.getParams()[0].getString();
	Aurora::NWScript::Object *object = ctx.getParams()[1].getObject();
	int32_t count = ctx.getParams()[2].getInt();

	Creature *creature = ObjectContainer::toCreature(object);
	if (creature) {
		creature->getInventory().addItem(itemTag, count);
		Item *item = creature->addScriptItem(itemTag);
		if (item)
			ctx.getReturn() = item;
		return;
	}

	Placeable *placeable = ObjectContainer::toPlaceable(object);
	if (placeable) {
		placeable->getInventory().addItem(itemTag, count);
		return;
	}

	warning("Functions::createItemOnObject(): invalid object");
}

void Functions::getArea(Aurora::NWScript::FunctionContext &ctx) {
	// TODO: return current area of the specified object
	ctx.getReturn() = _game->getModule().getCurrentArea();
}

void Functions::getLocation(Aurora::NWScript::FunctionContext &ctx) {
	const Object *object = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (!object)
		return;

	ctx.getReturn() = object->getLocation();
}

void Functions::getPositionFromLocation(Aurora::NWScript::FunctionContext &ctx) {
	Location *location = ObjectContainer::toLocation(ctx.getParams()[0].getEngineType());
	ctx.getReturn().setType(Aurora::NWScript::kTypeVector);

	if (!location) {
		ctx.getReturn().setVector(0.0f, 0.0f, 0.0f);
		return;
	}

	float x, y, z;
	location->getPosition(x, y, z);
	ctx.getReturn().setVector(x, y, z);
}

void Functions::getFacingFromLocation(Aurora::NWScript::FunctionContext &ctx) {
	Location *location = ObjectContainer::toLocation(ctx.getParams()[0].getEngineType());
	ctx.getReturn() = location ? location->getFacing() : 0.0f;
}

void Functions::jumpToLocation(Aurora::NWScript::FunctionContext &ctx) {
	Object *object = ObjectContainer::toObject(ctx.getCaller());
	Location *moveTo = ObjectContainer::toLocation(ctx.getParams()[0].getEngineType());

	if (!object || !moveTo)
		return;

	float x, y, z;
	moveTo->getPosition(x, y, z);
	object->setPosition(x, y, z);
}

void Functions::jumpToObject(Aurora::NWScript::FunctionContext &ctx) {
	Object *caller = ObjectContainer::toObject(ctx.getCaller());
	Object *target = ObjectContainer::toObject(getParamObject(ctx, 0));

	if (!caller || !target)
		return;

	float x, y, z;
	target->getPosition(x, y, z);
	caller->setPosition(x, y, z);
}

void Functions::locationCreate(Aurora::NWScript::FunctionContext &ctx) {
	// Location(vector position, float facing) -> location engine type
	float x, y, z;
	ctx.getParams()[0].getVector(x, y, z);
	float facing = ctx.getParams()[1].getFloat();

	Location loc;
	loc.setPosition(x, y, z);
	loc.setFacing(facing);

	ctx.getReturn() = loc;
}

void Functions::getWaypointByTag(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString &tag = ctx.getParams()[0].getString();

	std::unique_ptr<Aurora::NWScript::ObjectSearch> search(
		_game->getModule().findObjectsByTag(tag));

	// Iterate until we find a waypoint
	Aurora::NWScript::Object *found = nullptr;
	while (Aurora::NWScript::Object *o = search->next()) {
		if (ObjectContainer::toWaypoint(o)) {
			found = o;
			break;
		}
	}

	ctx.getReturn() = found;
}

void Functions::getItemInSlot(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = 0;

	Aurora::NWScript::Object *object = ctx.getParams()[1].getObject();
	if (object)
		creature = ObjectContainer::toCreature(object);
	else
		creature = _game->getModule().getPC();

	if (!creature) {
		warning("Functions::getItemInSlot(): invalid creature");
		ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
		return;
	}

	int slot = ctx.getParams()[0].getInt();
	Item *item = creature->getEquipedItem(static_cast<InventorySlot>(slot));
	if (item)
		ctx.getReturn() = item;
}

void Functions::getNearestCreature(Aurora::NWScript::FunctionContext &ctx) {
	CreatureSearchCriteria criteria;
	criteria.firstCriteriaType = static_cast<CreatureType>(ctx.getParams()[0].getInt());
	criteria.firstCriteriaValue = ctx.getParams()[1].getInt();

	Object *target = ObjectContainer::toCreature(ctx.getParams()[2].getObject());
	int nth = ctx.getParams()[3].getInt();

	criteria.secondCriteriaType = static_cast<CreatureType>(ctx.getParams()[4].getInt());
	criteria.secondCriteriaValue = ctx.getParams()[5].getInt();
	criteria.thirdCriteriaType = static_cast<CreatureType>(ctx.getParams()[6].getInt());
	criteria.thirdCriteriaValue = ctx.getParams()[7].getInt();

	ctx.getReturn() = _game->getModule().getCurrentArea()->getNearestCreature(target, nth, criteria);
}

void Functions::getNearestObject(Aurora::NWScript::FunctionContext &ctx) {
	// Nearest object ignoring criteria or finding just basic placeables.
	// For Endar Spire, we can just return a null object for now since full 
	// spatial searches for non-creatures aren't implemented in Area yet.
	ctx.getReturn() = (Aurora::NWScript::Object *) nullptr;
}

void Functions::getNearestObjectByTag(Aurora::NWScript::FunctionContext &ctx) {
	// Often equivalent to GetObjectByTag for a single module unless distance actually matters heavily.
	// Simple stub for progression:
	const Common::UString &tag = ctx.getParams()[0].getString();
	ctx.getReturn() = _game->getModule().getCurrentArea()->getObjectByTag(tag);
}

void Functions::getSpellTargetObject(Aurora::NWScript::FunctionContext &ctx) {
	// Spell targeting context is not yet tracked globally.
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::getTag(Aurora::NWScript::FunctionContext &ctx) {
	Object *object = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (!object) {
		warning("Functions::getTag(): invalid object");
		ctx.getReturn() = Common::UString();
		return;
	}

	ctx.getReturn() = object->getTag();
}

void Functions::destroyObject(Aurora::NWScript::FunctionContext &ctx) {
	Object *object = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (object)
		_game->getModule().getCurrentArea()->removeObject(object);
}

void Functions::getPosition(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn().setVector(0.0f, 0.0f, 0.0f);

	Object *object = ObjectContainer::toObject(getParamObject(ctx, 0));
	if (!object)
		return;

	float x, y, z;
	object->getPosition(x, y, z);
	ctx.getReturn().setVector(x, y, z);
}

void Functions::getFacing(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = 0.0f;

	Object *object = ObjectContainer::toObject(getParamObject(ctx, 0));
	if (!object)
		return;

	float x, y, z, angle;
	object->getOrientation(x, y, z, angle);
	ctx.getReturn() = angle;
}

void Functions::getItemPossessor(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);

	Aurora::NWScript::Object *rawObj = ctx.getParams()[0].getObject();
	if (!rawObj)
		return;

	// Search all creatures in the current area for one that has this item equipped or in script items
	const std::vector<Creature *> &creatures = _game->getModule().getCurrentArea()->getCreatures();
	for (Creature *creature : creatures) {
		if (!creature)
			continue;
		// Check equipped items
		for (int slot = 0; slot < kInventorySlotMAX; ++slot) {
			Item *equipped = creature->getEquipedItem(static_cast<InventorySlot>(slot));
			if (equipped && equipped == static_cast<Object *>(rawObj)) {
				ctx.getReturn() = creature;
				return;
			}
		}
		// Check script items
		Item *found = creature->findInventoryItemByTag(
		    static_cast<Object *>(rawObj)->getTag());
		if (found && found == static_cast<Object *>(rawObj)) {
			ctx.getReturn() = creature;
			return;
		}
	}
}

void Functions::getItemPossessedBy(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);

	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	if (!creature)
		return;

	const Common::UString &tag = ctx.getParams()[1].getString();

	// Check equipped items first (live Item objects)
	Item *item = creature->findInventoryItemByTag(tag);
	if (item) {
		ctx.getReturn() = item;
		return;
	}

	// If tag is in inventory (tag+count), create a live Item object for scripting
	if (creature->getInventory().hasItem(tag)) {
		item = creature->addScriptItem(tag);
		if (item)
			ctx.getReturn() = item;
	}
}

void Functions::getObjectType(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<int32_t>(kObjectTypeInvalid);

	Object *object = ObjectContainer::toObject(getParamObject(ctx, 0));
	if (!object || (static_cast<uint32_t>(object->getType()) >= kObjectTypeMAX))
		return;

	ctx.getReturn() = static_cast<int32_t>(object->getType());
}

void Functions::getDistanceToObject(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = -1.0f;

	Object *target = ObjectContainer::toObject(getParamObject(ctx, 0));
	Object *caller = ObjectContainer::toObject(ctx.getCaller());
	if (!target || !caller)
		return;

	float x1, y1, z1;
	target->getPosition(x1, y1, z1);

	float x2, y2, z2;
	caller->getPosition(x2, y2, z2);

	float dx = x1 - x2;
	float dy = y1 - y2;
	float dz = z1 - z2;
	ctx.getReturn() = std::sqrt(dx * dx + dy * dy + dz * dz);
}

void Functions::getDistanceToObject2D(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = -1.0f;

	Object *target = ObjectContainer::toObject(getParamObject(ctx, 0));
	Object *caller = ObjectContainer::toObject(ctx.getCaller());
	if (!target || !caller)
		return;

	float x1, y1, z1;
	float x2, y2, z2;
	target->getPosition(x1, y1, z1);
	caller->getPosition(x2, y2, z2);

	const float dx = x1 - x2;
	const float dy = y1 - y2;
	ctx.getReturn() = std::sqrt(dx * dx + dy * dy);
}

void Functions::getDistanceBetween(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = -1.0f;

	Object *first = ObjectContainer::toObject(getParamObject(ctx, 0));
	Object *second = ObjectContainer::toObject(getParamObject(ctx, 1));
	if (!first || !second)
		return;

	float x1, y1, z1;
	float x2, y2, z2;
	first->getPosition(x1, y1, z1);
	second->getPosition(x2, y2, z2);

	const float dx = x1 - x2;
	const float dy = y1 - y2;
	const float dz = z1 - z2;
	ctx.getReturn() = std::sqrt(dx * dx + dy * dy + dz * dz);
}

void Functions::getDistanceBetween2D(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = -1.0f;

	Object *first = ObjectContainer::toObject(getParamObject(ctx, 0));
	Object *second = ObjectContainer::toObject(getParamObject(ctx, 1));
	if (!first || !second)
		return;

	float x1, y1, z1;
	float x2, y2, z2;
	first->getPosition(x1, y1, z1);
	second->getPosition(x2, y2, z2);

	const float dx = x1 - x2;
	const float dy = y1 - y2;
	ctx.getReturn() = std::sqrt(dx * dx + dy * dy);
}

void Functions::getDistanceBetweenLocations(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = -1.0f;

	Location *first = ObjectContainer::toLocation(ctx.getParams()[0].getEngineType());
	Location *second = ObjectContainer::toLocation(ctx.getParams()[1].getEngineType());
	if (!first || !second)
		return;

	float x1, y1, z1;
	float x2, y2, z2;
	first->getPosition(x1, y1, z1);
	second->getPosition(x2, y2, z2);

	const float dx = x1 - x2;
	const float dy = y1 - y2;
	const float dz = z1 - z2;
	ctx.getReturn() = std::sqrt(dx * dx + dy * dy + dz * dz);
}

void Functions::getDistanceBetweenLocations2D(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = -1.0f;

	Location *first = ObjectContainer::toLocation(ctx.getParams()[0].getEngineType());
	Location *second = ObjectContainer::toLocation(ctx.getParams()[1].getEngineType());
	if (!first || !second)
		return;

	float x1, y1, z1;
	float x2, y2, z2;
	first->getPosition(x1, y1, z1);
	second->getPosition(x2, y2, z2);

	const float dx = x1 - x2;
	const float dy = y1 - y2;
	ctx.getReturn() = std::sqrt(dx * dx + dy * dy);
}

void Functions::exploreAreaForPlayer(Aurora::NWScript::FunctionContext &ctx) {
	// Marks the entire area as explored on the minimap for the given player.
	// Full minimap revelation requires a rendering-layer hook; this stub
	// satisfies script execution so the Endar Spire module runs without error.
}

void Functions::getFirstItemInInventory(Aurora::NWScript::FunctionContext &ctx) {
	Object *object = ObjectContainer::toObject(getParamObject(ctx, 0));
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);

	_inventoryIterObject = object;
	_inventoryIterTags.clear();
	_inventoryIterIndex = 0;

	if (!object)
		return;

	Creature *creature = ObjectContainer::toCreature(object);
	if (creature) {
		for (const auto &kv : creature->getInventory().getItems())
			_inventoryIterTags.push_back(kv.first);
	} else {
		Placeable *placeable = ObjectContainer::toPlaceable(object);
		if (placeable)
			for (const auto &kv : placeable->getInventory().getItems())
				_inventoryIterTags.push_back(kv.first);
	}

	if (_inventoryIterTags.empty())
		return;

	const Common::UString &tag = _inventoryIterTags[_inventoryIterIndex++];
	creature = ObjectContainer::toCreature(object);
	if (creature) {
		Item *item = creature->addScriptItem(tag);
		if (item)
			ctx.getReturn() = item;
	} else {
		Placeable *placeable = ObjectContainer::toPlaceable(object);
		if (placeable) {
			Item *item = placeable->addScriptItem(tag);
			if (item)
				ctx.getReturn() = item;
		}
	}
}

void Functions::getNextItemInInventory(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);

	if (_inventoryIterIndex >= _inventoryIterTags.size())
		return;

	const Common::UString &tag = _inventoryIterTags[_inventoryIterIndex++];
	Object *object = _inventoryIterObject;
	if (!object)
		return;

	Creature *creature = ObjectContainer::toCreature(object);
	if (creature) {
		Item *item = creature->addScriptItem(tag);
		if (item)
			ctx.getReturn() = item;
	} else {
		Placeable *placeable = ObjectContainer::toPlaceable(object);
		if (placeable) {
			Item *item = placeable->addScriptItem(tag);
			if (item)
				ctx.getReturn() = item;
		}
	}
}

// ---------------------------------------------------------------------------
// Faction relationship helpers
// ---------------------------------------------------------------------------

/** Return true if a faction is considered hostile (attacks friendly factions). */
static bool isFactionHostile(Engines::KotORBase::Faction f) {
	return f == Engines::KotORBase::kFactionHostile1 ||
	       f == Engines::KotORBase::kFactionHostile2 ||
	       f == Engines::KotORBase::kFactionEndarSpire;
}

/** Return true if a faction is considered friendly (cooperative with other friendly factions). */
static bool isFactionFriendly(Engines::KotORBase::Faction f) {
	return f == Engines::KotORBase::kFactionFriendly1 ||
	       f == Engines::KotORBase::kFactionFriendly2;
}

void Functions::getIsEnemy(Aurora::NWScript::FunctionContext &ctx) {
	// GetIsEnemy(object oTarget, object oSource=OBJECT_SELF)
	// Returns TRUE if oSource and oTarget are in mutually hostile factions.
	ctx.getReturn() = 0;

	const Object *target = ObjectContainer::toObject(getParamObject(ctx, 0));
	const Object *source = ObjectContainer::toObject(getParamObject(ctx, 1));
	if (!target || !source)
		return;

	Faction tf = target->getFaction();
	Faction sf = source->getFaction();

	bool enemy = (isFactionHostile(tf) && isFactionFriendly(sf)) ||
	             (isFactionHostile(sf) && isFactionFriendly(tf));
	ctx.getReturn() = enemy ? 1 : 0;
}

void Functions::getIsFriend(Aurora::NWScript::FunctionContext &ctx) {
	// GetIsFriend(object oTarget, object oSource=OBJECT_SELF)
	// Returns TRUE if oSource and oTarget are in mutually friendly factions.
	ctx.getReturn() = 0;

	const Object *target = ObjectContainer::toObject(getParamObject(ctx, 0));
	const Object *source = ObjectContainer::toObject(getParamObject(ctx, 1));
	if (!target || !source)
		return;

	Faction tf = target->getFaction();
	Faction sf = source->getFaction();

	// Same non-invalid faction, or both in a friendly faction family
	bool friendly = (tf != kFactionInvalid && tf == sf) ||
	                (isFactionFriendly(tf) && isFactionFriendly(sf));
	ctx.getReturn() = friendly ? 1 : 0;
}

void Functions::getIsNeutral(Aurora::NWScript::FunctionContext &ctx) {
	// GetIsNeutral(object oTarget, object oSource=OBJECT_SELF)
	// Returns TRUE if oSource and oTarget are neither friends nor enemies.
	ctx.getReturn() = 0;

	const Object *target = ObjectContainer::toObject(getParamObject(ctx, 0));
	const Object *source = ObjectContainer::toObject(getParamObject(ctx, 1));
	if (!target || !source)
		return;

	Faction tf = target->getFaction();
	Faction sf = source->getFaction();

	bool isEnemy  = (isFactionHostile(tf) && isFactionFriendly(sf)) ||
	                (isFactionHostile(sf) && isFactionFriendly(tf));
	bool isFriend = (tf != kFactionInvalid && tf == sf) ||
	                (isFactionFriendly(tf) && isFactionFriendly(sf));

	ctx.getReturn() = (!isEnemy && !isFriend) ? 1 : 0;
}

void Functions::getName(Aurora::NWScript::FunctionContext &ctx) {
	// GetName(object oObject) -> string
	const Object *object = ObjectContainer::toObject(getParamObject(ctx, 0));
	ctx.getReturn() = object ? object->getName() : Common::UString();
}

void Functions::setIsDestroyable(Aurora::NWScript::FunctionContext &ctx) {
	// SetIsDestroyable(int bDestroyable, int bRaiseDeadPossible=TRUE, int bSelectableWhenDead=FALSE)
	// Full destroyable semantics require a persistence layer.  For the Endar
	// Spire milestone this is a no-op that satisfies script execution without error.
}

void Functions::getIsInConversation(Aurora::NWScript::FunctionContext &ctx) {
	// GetIsInConversation(object oObject) -> int
	// Returns TRUE if the module's conversation GUI is currently active,
	// regardless of which specific object is being addressed.
	ctx.getReturn() = _game->getModule().isConversationActive() ? 1 : 0;
}

// ---------------------------------------------------------------------------
// Area object iteration
// ---------------------------------------------------------------------------

void Functions::getFirstObjectInArea(Aurora::NWScript::FunctionContext &ctx) {
	// GetFirstObjectInArea(object oArea=OBJECT_INVALID, int nObjectFilter=OBJECT_TYPE_CREATURE)
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);

	int filter = ctx.getParams()[1].getInt();

	_areaIterObjects.clear();
	_areaIterIndex = 0;

	// Collect all matching objects from the module's object container.
	// Each set bit in `filter` corresponds to a KotOR ObjectType value.
	for (int bit = 1; bit < static_cast<int>(kObjectTypeMAX); bit <<= 1) {
		if (!(filter & bit))
			continue;

		std::unique_ptr<Aurora::NWScript::ObjectSearch> search(
			_game->getModule().findObjectsByType(static_cast<ObjectType>(bit)));

		Aurora::NWScript::Object *raw = nullptr;
		while ((raw = search->next())) {
			Object *obj = ObjectContainer::toObject(raw);
			if (obj)
				_areaIterObjects.push_back(obj);
		}
	}

	if (_areaIterObjects.empty())
		return;

	ctx.getReturn() = _areaIterObjects[_areaIterIndex++];
}

void Functions::getNextObjectInArea(Aurora::NWScript::FunctionContext &ctx) {
	// GetNextObjectInArea(object oArea=OBJECT_INVALID, int nObjectFilter=OBJECT_TYPE_CREATURE)
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);

	if (_areaIterIndex >= _areaIterObjects.size())
		return;

	ctx.getReturn() = _areaIterObjects[_areaIterIndex++];
}

void Functions::getLockUnlockDC(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	if (!situated) {
		ctx.getReturn() = 0;
		return;
	}

	if (!situated->isLocked()) {
		ctx.getReturn() = 0;
		return;
	}

	ctx.getReturn() = situated->isKeyRequired() ? 100 : 10;
}

void Functions::getReputation(Aurora::NWScript::FunctionContext &ctx) {
	// GetReputation(object oSource, object oTarget) → int [0–100]
	Object *source = ObjectContainer::toObject(getParamObject(ctx, 0));
	Object *target = ObjectContainer::toObject(getParamObject(ctx, 1));

	if (!source || !target) {
		ctx.getReturn() = 50;
		return;
	}

	int srcFaction = static_cast<int>(source->getFaction());
	int tgtFaction = static_cast<int>(target->getFaction());
	ctx.getReturn() = _game->getModule().getReputation(srcFaction, tgtFaction);
}

void Functions::adjustReputation(Aurora::NWScript::FunctionContext &ctx) {
	// AdjustReputation(object oTarget, object oSourceFactionMember, int nAdjustment)
	Object *target = ObjectContainer::toObject(getParamObject(ctx, 0));
	Object *source = ObjectContainer::toObject(getParamObject(ctx, 1));
	int delta = ctx.getParams()[2].getInt();

	if (!target || !source)
		return;

	int srcFaction = static_cast<int>(source->getFaction());
	int tgtFaction = static_cast<int>(target->getFaction());
	_game->getModule().adjustReputation(tgtFaction, srcFaction, delta);
}

// ---------------------------------------------------------------------------
// ChangeFaction — switch a creature's faction to match another creature's
// ---------------------------------------------------------------------------

void Functions::changeFaction(Aurora::NWScript::FunctionContext &ctx) {
	// ChangeFaction(object oObjectToChangeFaction, object oMemberOfFactionToJoin)
	Object *target = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	Object *source = ObjectContainer::toObject(ctx.getParams()[1].getObject());
	if (!target || !source)
		return;

	target->setFaction(source->getFaction());
}

// ---------------------------------------------------------------------------
// GetFirstObjectInShape / GetNextObjectInShape — AoE radius iteration
// ---------------------------------------------------------------------------

void Functions::getFirstObjectInShape(Aurora::NWScript::FunctionContext &ctx) {
	// GetFirstObjectInShape(int nShape, float fSize, location lTarget,
	//                       int bLineOfSight=FALSE, int nObjectFilter=OBJECT_TYPE_CREATURE,
	//                       vector vOrigin=...)
	// nShape: 0=SHAPE_SPHERE, 1=SHAPE_CUBE, etc.  We treat all as a sphere (radius fSize).
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
	_shapeIterObjects.clear();
	_shapeIterIndex = 0;

	float radius    = ctx.getParams()[1].getFloat();
	int   objFilter = ctx.getParams()[4].getInt();

	// Determine the centre from the location parameter.
	Location *loc = ObjectContainer::toLocation(ctx.getParams()[2].getEngineType());
	float cx = 0.0f, cy = 0.0f, cz = 0.0f;
	if (loc)
		loc->getPosition(cx, cy, cz);

	// Gather all objects of the requested type(s) within the radius.
	for (int bit = 1; bit < static_cast<int>(kObjectTypeMAX); bit <<= 1) {
		if (!(objFilter & bit))
			continue;

		std::unique_ptr<Aurora::NWScript::ObjectSearch> search(
			_game->getModule().findObjectsByType(static_cast<ObjectType>(bit)));

		Aurora::NWScript::Object *raw = nullptr;
		while ((raw = search->next())) {
			Object *obj = ObjectContainer::toObject(raw);
			if (!obj)
				continue;
			float ox, oy, oz;
			obj->getPosition(ox, oy, oz);
			float dx = ox - cx, dy = oy - cy, dz = oz - cz;
			float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
			if (dist <= radius)
				_shapeIterObjects.push_back(obj);
		}
	}

	if (!_shapeIterObjects.empty())
		ctx.getReturn() = _shapeIterObjects[_shapeIterIndex++];
}

void Functions::getNextObjectInShape(Aurora::NWScript::FunctionContext &ctx) {
	// GetNextObjectInShape(...) — same signature as GetFirstObjectInShape, ignored.
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
	if (_shapeIterIndex < _shapeIterObjects.size())
		ctx.getReturn() = _shapeIterObjects[_shapeIterIndex++];
}

// ---------------------------------------------------------------------------
// CreateObject — spawn a creature or item in the world
// ---------------------------------------------------------------------------

void Functions::createObject(Aurora::NWScript::FunctionContext &ctx) {
	// CreateObject(int nObjectType, string sTemplate, location lLocation, int bUseAppearAnimation=FALSE)
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);

	int objectType = ctx.getParams()[0].getInt();
	const Common::UString &tmpl = ctx.getParams()[1].getString();
	Location *loc = ObjectContainer::toLocation(ctx.getParams()[2].getEngineType());

	if (tmpl.empty() || !loc)
		return;

	float x, y, z;
	loc->getPosition(x, y, z);

	if (objectType & kObjectTypeCreature) {
		// Create a creature from its blueprint template.
		Creature *creature = nullptr;
		try {
			creature = _game->getModule().createCreatureByTemplate(tmpl);
		} catch (...) {
			warning("Functions::createObject(): failed to create creature \"%s\"", tmpl.c_str());
			return;
		}
		if (!creature)
			return;

		creature->setPosition(x, y, z);
		_game->getModule().getCurrentArea()->addCreature(creature);
		ctx.getReturn() = creature;
	}
	// Item creation via CreateObject is rare in the progression path;
	// CreateItemOnObject covers most use-cases and is already implemented.
}

// ---------------------------------------------------------------------------
// GetModuleItemAcquired / GetModuleItemAcquiredFrom
// ---------------------------------------------------------------------------

void Functions::getModuleItemAcquired(Aurora::NWScript::FunctionContext &ctx) {
	// Returns the item last acquired (set by the module's OnAcquireItem event).
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(
	    _game->getModule().getLastAcquiredItem());
}

void Functions::getModuleItemAcquiredFrom(Aurora::NWScript::FunctionContext &ctx) {
	// Returns the object from which the item was acquired.
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

// ---------------------------------------------------------------------------
// SetCustomToken — dialogue token replacement
// ---------------------------------------------------------------------------

void Functions::setCustomToken(Aurora::NWScript::FunctionContext &ctx) {
	// SetCustomToken(int nCustomTokenNumber, string sTokenValue)
	int tokenNum = ctx.getParams()[0].getInt();
	const Common::UString &value = ctx.getParams()[1].getString();
	_customTokens[tokenNum] = value;
}

// ---------------------------------------------------------------------------
// Perception state queries — OnPerceive script support
// ---------------------------------------------------------------------------

void Functions::getLastPerceived(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (caller)
		ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(caller->getLastPerceived());
}

void Functions::getLastPerceptionHeard(Aurora::NWScript::FunctionContext &ctx) {
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	ctx.getReturn() = (caller && caller->getLastPerceptionHeard()) ? 1 : 0;
}

void Functions::getLastPerceptionInaudible(Aurora::NWScript::FunctionContext &ctx) {
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	ctx.getReturn() = (caller && caller->getLastPerceptionInaudible()) ? 1 : 0;
}

void Functions::getLastPerceptionSeen(Aurora::NWScript::FunctionContext &ctx) {
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	ctx.getReturn() = (caller && caller->getLastPerceptionSeen()) ? 1 : 0;
}

void Functions::getLastPerceptionVanished(Aurora::NWScript::FunctionContext &ctx) {
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	ctx.getReturn() = (caller && caller->getLastPerceptionVanished()) ? 1 : 0;
}

// ---------------------------------------------------------------------------
// GetObjectSeen / GetObjectHeard — perception membership queries
// ---------------------------------------------------------------------------

void Functions::getObjectSeen(Aurora::NWScript::FunctionContext &ctx) {
	// GetObjectSeen(object oTarget, object oSource=OBJECT_SELF) → int
	const Object *target = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	const Creature *source = ObjectContainer::toCreature(ctx.getParams()[1].getObject()
	                             ? ctx.getParams()[1].getObject() : ctx.getCaller());
	if (!target || !source) {
		ctx.getReturn() = 0;
		return;
	}
	ctx.getReturn() = source->hasSeenObject(target) ? 1 : 0;
}

void Functions::getObjectHeard(Aurora::NWScript::FunctionContext &ctx) {
	// GetObjectHeard(object oTarget, object oSource=OBJECT_SELF) → int
	const Object *target = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	const Creature *source = ObjectContainer::toCreature(ctx.getParams()[1].getObject()
	                             ? ctx.getParams()[1].getObject() : ctx.getCaller());
	if (!target || !source) {
		ctx.getReturn() = 0;
		return;
	}
	ctx.getReturn() = source->hasHeardObject(target) ? 1 : 0;
}

// ---------------------------------------------------------------------------
// GetNearestCreatureToLocation
// ---------------------------------------------------------------------------

void Functions::getNearestCreatureToLocation(Aurora::NWScript::FunctionContext &ctx) {
	// GetNearestCreatureToLocation(int nFirstCriteriaType, int nFirstCriteriaValue,
	//   location lLocation, int nNth=1, ...) → object
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);

	Location *loc = ObjectContainer::toLocation(ctx.getParams()[2].getEngineType());
	if (!loc)
		return;

	float lx, ly, lz;
	loc->getPosition(lx, ly, lz);

	int nth = ctx.getParams()[3].getInt();
	if (nth < 1) nth = 1;

	CreatureSearchCriteria criteria;
	criteria.firstCriteriaType  = static_cast<CreatureType>(ctx.getParams()[0].getInt());
	criteria.firstCriteriaValue = ctx.getParams()[1].getInt();
	criteria.secondCriteriaType  = static_cast<CreatureType>(ctx.getParams()[4].getInt());
	criteria.secondCriteriaValue = ctx.getParams()[5].getInt();
	criteria.thirdCriteriaType   = static_cast<CreatureType>(ctx.getParams()[6].getInt());
	criteria.thirdCriteriaValue  = ctx.getParams()[7].getInt();

	// Use a temporary location object as the reference point.
	Location refLoc;
	refLoc.setPosition(lx, ly, lz);

	// Walk through all creatures sorted by distance to lLocation.
	const std::vector<Creature *> &creatures =
	    _game->getModule().getCurrentArea()->getCreatures();

	// Sort a copy by distance to the target location.
	std::vector<std::pair<float, Creature *>> sorted;
	for (Creature *c : creatures) {
		if (!c || c->isDead())
			continue;
		float cx, cy, cz;
		c->getPosition(cx, cy, cz);
		float dx = cx - lx, dy = cy - ly, dz = cz - lz;
		float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
		sorted.push_back({dist, c});
	}
	std::sort(sorted.begin(), sorted.end(),
	          [](const std::pair<float, Creature *> &a, const std::pair<float, Creature *> &b) {
	              return a.first < b.first;
	          });

	int count = 0;
	for (auto &p : sorted) {
		Creature *c = p.second;
		if (c->matchSearchCriteria(nullptr, criteria)) {
			++count;
			if (count == nth) {
				ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(c);
				return;
			}
		}
	}
}

// ---------------------------------------------------------------------------
// BeginConversation — start a conversation from a script
// ---------------------------------------------------------------------------

void Functions::getFactionEqual(Aurora::NWScript::FunctionContext &ctx) {
	// GetFactionEqual(object oCreatureToTest, object oCreature=OBJECT_SELF) → int
	// Returns TRUE if both objects belong to the same faction.
	ctx.getReturn() = 0;
	const Object *target = ObjectContainer::toObject(getParamObject(ctx, 0));
	const Object *source = ObjectContainer::toObject(getParamObject(ctx, 1));
	if (!target || !source)
		return;
	Faction tf = target->getFaction();
	Faction sf = source->getFaction();
	ctx.getReturn() = (tf != kFactionInvalid && tf == sf) ? 1 : 0;
}

void Functions::getPlotFlag(Aurora::NWScript::FunctionContext &ctx) {
	// GetPlotFlag(object oTarget=OBJECT_SELF) → int
	// Returns TRUE if the target has the plot flag set (immune to permanent death).
	ctx.getReturn() = 0;
	const Object *target = ObjectContainer::toObject(getParamObject(ctx, 0));
	if (target)
		ctx.getReturn() = target->getPlotFlag() ? 1 : 0;
}

void Functions::setPlotFlag(Aurora::NWScript::FunctionContext &ctx) {
	// SetPlotFlag(object oTarget, int nPlotFlag)
	Object *target = ObjectContainer::toObject(getParamObject(ctx, 0));
	bool plot = ctx.getParams()[1].getInt() != 0;
	if (target)
		target->setPlotFlag(plot);
}

void Functions::getEffectType(Aurora::NWScript::FunctionContext &ctx) {
	// GetEffectType(effect eEffect) → int
	// Returns the integer EffectType of the given effect engine type.
	ctx.getReturn() = -1;
	Aurora::NWScript::EngineType *raw = ctx.getParams()[0].getEngineType();
	const Effect *e = dynamic_cast<const Effect *>(raw);
	if (e)
		ctx.getReturn() = static_cast<int>(e->getType());
}

void Functions::getLastDamager(Aurora::NWScript::FunctionContext &ctx) {
	// GetLastDamager(object oCreature=OBJECT_SELF) → object
	// Returns the last creature that inflicted damage on oCreature.
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
	Object *target = ObjectContainer::toObject(getParamObject(ctx, 0));
	Creature *c = dynamic_cast<Creature *>(target);
	if (c)
		ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(c->getLastHostileActor());
}

void Functions::beginConversation(Aurora::NWScript::FunctionContext &ctx) {
	// BeginConversation(string sResRef="", object oObjectToDialog=OBJECT_SELF) → int
	const Common::UString &resRef = ctx.getParams()[0].getString();
	Aurora::NWScript::Object *raw = ctx.getParams()[1].getObject()
	                                ? ctx.getParams()[1].getObject() : ctx.getCaller();
	Creature *target = ObjectContainer::toCreature(raw);

	Common::UString convName = resRef;
	if (convName.empty() && target)
		convName = target->getConversation();

	ctx.getReturn() = 0;
	if (convName.empty())
		return;

	_game->getModule().delayConversation(convName, target);
	ctx.getReturn() = 1;
}

void Functions::getItemInSlot(Aurora::NWScript::FunctionContext &ctx) {
	int slot = ctx.getParams()[0].getInt();
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 1));
	if (!creature) {
		ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
		return;
	}

	ctx.getReturn() = creature->getEquipedItem(static_cast<InventorySlot>(slot));
}

void Functions::getItemPossessedBy(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	const Common::UString &tag = ctx.getParams()[1].getString();
	if (!creature) {
		ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
		return;
	}

	ctx.getReturn() = creature->findInventoryItemByTag(tag);
}

void Functions::getIsEnemy(Aurora::NWScript::FunctionContext &ctx) {
	const Object *target = ObjectContainer::toObject(getParamObject(ctx, 0));
	const Object *source = ObjectContainer::toObject(getParamObject(ctx, 1));
	if (!target || !source) {
		ctx.getReturn() = 0;
		return;
	}
	// Basic enemy check: Factions are different and one is hostile.
	ctx.getReturn() = (target->getFaction() != source->getFaction()) ? 1 : 0;
}

void Functions::getIsFriend(Aurora::NWScript::FunctionContext &ctx) {
	const Object *target = ObjectContainer::toObject(getParamObject(ctx, 0));
	const Object *source = ObjectContainer::toObject(getParamObject(ctx, 1));
	ctx.getReturn() = (target && source && target->getFaction() == source->getFaction()) ? 1 : 0;
}

void Functions::getIsNeutral(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = 0;
}

void Functions::getIsReactionTypeHostile(Aurora::NWScript::FunctionContext &ctx) {
	getIsEnemy(ctx);
}

void Functions::getIsReactionTypeFriendly(Aurora::NWScript::FunctionContext &ctx) {
	getIsFriend(ctx);
}

void Functions::getIsReactionTypeNeutral(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = 0;
}

void Functions::getLockUnlockDC(Aurora::NWScript::FunctionContext &ctx) {
	const Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	ctx.getReturn() = situated ? situated->getLockDC() : 0;
}

void Functions::getLockKeyTag(Aurora::NWScript::FunctionContext &ctx) {
	const Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	ctx.getReturn() = situated ? situated->getKeyTag() : Common::UString();
}

void Functions::getLockRequiredSkill(Aurora::NWScript::FunctionContext &ctx) {
	const Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	ctx.getReturn() = situated ? situated->getLockRequiredSkill() : 0;
}


void Functions::clearAllEffects(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *object = getParamObject(ctx, 0);
	if (object) {
		// Logic to clear effects
		debugC(Common::kDebugEngineLogic, 1, "ClearAllEffects on %s", object->getTag().c_str());
	}
}

void Functions::getLastHostileTarget(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = (Aurora::NWScript::Object *)nullptr;
}

void Functions::getLastAttackAction(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = 0;
}

void Functions::getWasForcePowerSuccessful(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = 1; // Default to success
}


void Functions::getFirstEffect(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = new Effect(kEffectVFX, 0); }
void Functions::getNextEffect(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = new Effect(kEffectVFX, 0); }
void Functions::removeEffect(Aurora::NWScript::FunctionContext &ctx) {}
void Functions::getIsEffectValid(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 0; }
void Functions::getEffectDurationType(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 0; }
void Functions::getEffectSubType(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 0; }
void Functions::getEffectCreator(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = (Aurora::NWScript::Object *)nullptr; }


// ---------------------------------------------------------------------------
// Faction iteration: GetFirstFactionMember / GetNextFactionMember
// ---------------------------------------------------------------------------

void Functions::getFirstFactionMember(Aurora::NWScript::FunctionContext &ctx) {
	// GetFirstFactionMember(object oMemberOfFaction, int bPCOnly=TRUE)
	// Resets the faction iterator; the next call to GetNextFactionMember
	// will return the first member.  Returns void (KOTOR I signature).
	_factionIterRef     = nullptr;
	_factionIterObjects.clear();
	_factionIterIndex   = 0;
}

void Functions::getNextFactionMember(Aurora::NWScript::FunctionContext &ctx) {
	// GetNextFactionMember(object oMemberOfFaction, int bPCOnly=TRUE) -> object
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);

	Aurora::NWScript::Object *ref = getParamObject(ctx, 0);
	int bPCOnly = ctx.getParams()[1].getInt();

	// Rebuild the list when the reference object changes (or on first call).
	if (ref != _factionIterRef) {
		_factionIterRef = ref;
		_factionIterObjects.clear();
		_factionIterIndex = 0;

		Object *refObj = ObjectContainer::toObject(ref);
		if (!refObj)
			return;

		int faction = static_cast<int>(refObj->getFaction());

		// Walk all creatures in the area and collect faction members.
		std::unique_ptr<Aurora::NWScript::ObjectSearch> search(
			_game->getModule().findObjectsByType(kObjectTypeCreature));

		Aurora::NWScript::Object *raw = nullptr;
		while ((raw = search->next())) {
			Object *obj = ObjectContainer::toObject(raw);
			if (!obj)
				continue;
			if (static_cast<int>(obj->getFaction()) != faction)
				continue;
			if (bPCOnly && !ObjectContainer::toPC(raw))
				continue;
			_factionIterObjects.push_back(obj);
		}
	}

	if (_factionIterIndex >= _factionIterObjects.size())
		return;

	ctx.getReturn() = _factionIterObjects[_factionIterIndex++];
}

// ---------------------------------------------------------------------------
// FaceObjectAwayFromObject (ID 553)
// ---------------------------------------------------------------------------

void Functions::faceObjectAwayFromObject(Aurora::NWScript::FunctionContext &ctx) {
	// FaceObjectAwayFromObject(object oFacer, object oObjectToFaceAwayFrom)
	Object *facer  = ObjectContainer::toObject(getParamObject(ctx, 0));
	Object *source = ObjectContainer::toObject(getParamObject(ctx, 1));
	if (!facer || !source)
		return;

	float fx, fy, fz, sx, sy, sz;
	facer ->getPosition(fx, fy, fz);
	source->getPosition(sx, sy, sz);

	// Angle pointing AWAY from source (opposite of toward source).
	float angle = atan2f(fy - sy, fx - sx) * (180.0f / M_PI);
	facer->setOrientation(0.0f, 0.0f, 1.0f, angle);
}

// ---------------------------------------------------------------------------
// Inventory disturb event queries (IDs 352, 353)
// ---------------------------------------------------------------------------

void Functions::getInventoryDisturbType(Aurora::NWScript::FunctionContext &ctx) {
	// GetInventoryDisturbType() -> int
	// Returns the disturb type (0=Added, 1=Removed, 2=Stolen) for the current
	// OnDisturbed event.  We store this in session state.
	ctx.getReturn() = _lastInventoryDisturbType;
}

void Functions::getInventoryDisturbItem(Aurora::NWScript::FunctionContext &ctx) {
	// GetInventoryDisturbItem() -> object
	// Returns the item that triggered the current OnDisturbed event.
	ctx.getReturn() = _lastInventoryDisturbItem;
}

// ---------------------------------------------------------------------------
// Item activation event queries (IDs 439, 440)
// ---------------------------------------------------------------------------

void Functions::getItemActivated(Aurora::NWScript::FunctionContext &ctx) {
	// GetItemActivated() -> object
	// Returns the item that was activated in the current OnActivateItem event.
	ctx.getReturn() = _lastItemActivated;
}

void Functions::getItemActivator(Aurora::NWScript::FunctionContext &ctx) {
	// GetItemActivator() -> object
	// Returns the creature that activated the item in the current
	// OnActivateItem event.
	ctx.getReturn() = _lastItemActivator;
}

} // End of namespace KotORBase
} // End of namespace Engines
