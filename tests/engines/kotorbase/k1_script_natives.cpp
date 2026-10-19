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
 *  Unit tests for KotOR script natives the K1 scripts rely on: SetIsDestroyable,
 *  GetLastAttackAction, the inventory-disturb getters, EventConversation /
 *  EventActivateItem and the PlayRoomAnimation animation names.
 */

#include <cstdlib>
#include <cstring>
#include <memory>

#include "gtest/gtest.h"

#include "src/common/memreadstream.h"
#include "src/common/memwritestream.h"

#include "src/aurora/gff3file.h"
#include "src/aurora/gff3writer.h"

#include "src/aurora/nwscript/types.h"
#include "src/aurora/nwscript/variable.h"
#include "src/aurora/nwscript/functioncontext.h"
#include "src/aurora/nwscript/objectman.h"

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/effect.h"
#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/inventory.h"
#include "src/engines/kotorbase/location.h"
#include "src/engines/kotorbase/room.h"
#include "src/engines/kotorbase/types.h"

#include "src/engines/kotorbase/script/event.h"
#include "src/engines/kotorbase/script/functions.h"

using namespace Engines::KotORBase;

using Aurora::NWScript::FunctionContext;
using Aurora::NWScript::Signature;
using Aurora::NWScript::kTypeVoid;
using Aurora::NWScript::kTypeInt;
using Aurora::NWScript::kTypeObject;
using Aurora::NWScript::kTypeEngineType;

namespace {

// nwscript.nss constants.
const int kNWActionAttackObject = 3;
const int kNWActionCastSpell    = 4;
const int kNWActionInvalid      = 65535;
const int kNWDisturbAdded       = 0;
const int kNWDisturbRemoved     = 1;

class TestCreature : public Creature {
public:
	TestCreature() : Creature() {
		setMaxHitPoints(10);
		setCurrentHitPoints(10);
	}

	/** Give the creature an object ID, as the module does when it loads it. */
	void registerAs(uint32_t id) {
		_id = id;
		ObjectMan.registerObject(this);
	}

	~TestCreature() override {
		if (_id != Aurora::kObjectIDInvalid)
			ObjectMan.unregisterObject(this);
	}

	void kill() {
		setCurrentHitPoints(0);
		handleDeath();
	}

protected:
	void getPartModelsPC(PartModels &, uint32_t, uint8_t) override {
	}
};

/** The natives under test never touch the game; it only has to exist. */
class TestGame : public Game {
public:
	TestGame() : Game(dummyConsole()) {}

	bool hasModule(const Common::UString &) const override { return false; }
	Functions &getFunctions() override { std::abort(); }
	void run() override {}
	void showLevelUpGUI(Creature *) override {}
	void openStoreGUI(const Common::UString &) override {}

protected:
	const Common::UString &getDefaultMenuMusic() const override {
		static const Common::UString kMusic;
		return kMusic;
	}

private:
	static Engines::Console &dummyConsole() {
		alignas(16) static unsigned char storage[16];
		return reinterpret_cast<Engines::Console &>(storage);
	}
};

class TestFunctions : public Functions {
public:
	explicit TestFunctions(Game &game) : Functions(game) {}

	void registerFunctions() override {}

	using Functions::setIsDestroyable;
	using Functions::getLastAttackAction;
	using Functions::getInventoryDisturbType;
	using Functions::getInventoryDisturbItem;
	using Functions::eventConversation;
	using Functions::eventActivateItem;
	using Functions::signalEvent;
	using Functions::getItemActivated;
	using Functions::getItemActivator;
	using Functions::getItemActivatedTarget;
};

struct NativeFixture {
	TestGame game;
	TestFunctions functions { game };
};

Aurora::GFF3File roundTrip(Aurora::GFF3Writer &writer) {
	Common::MemoryWriteStreamDynamic out(true);
	writer.write(out);

	const size_t size = out.size();
	byte *copy = new byte[size];
	std::memcpy(copy, out.getData(), size);
	return Aurora::GFF3File(new Common::MemoryReadStream(copy, size, true));
}

void callSetIsDestroyable(NativeFixture &f, Creature &caller, int destroyable, int raiseable, int selectable) {
	FunctionContext ctx("SetIsDestroyable");
	ctx.setSignature(Signature { kTypeVoid, kTypeInt, kTypeInt, kTypeInt });
	ctx.setCaller(&caller);
	ctx.getParams()[0] = static_cast<int32_t>(destroyable);
	ctx.getParams()[1] = static_cast<int32_t>(raiseable);
	ctx.getParams()[2] = static_cast<int32_t>(selectable);

	f.functions.setIsDestroyable(ctx);
}

Aurora::NWScript::Object *callObjectGetter(NativeFixture &f, void (TestFunctions::*fn)(FunctionContext &),
                                           Aurora::NWScript::Object *caller) {
	FunctionContext ctx("getter");
	ctx.setSignature(Signature { kTypeObject });
	ctx.setCaller(caller);

	(f.functions.*fn)(ctx);
	return ctx.getReturn().getObject();
}

} // End of anonymous namespace

GTEST_TEST(KotORK1Natives, creaturesAreDestroyableByDefault) {
	TestCreature creature;

	EXPECT_TRUE(creature.isDestroyable());
	EXPECT_TRUE(creature.isRaiseable());
	EXPECT_TRUE(creature.isSelectableWhenDead());
}

GTEST_TEST(KotORK1Natives, setIsDestroyableSetsTheCallersFlags) {
	NativeFixture f;
	TestCreature creature;

	// k35_vic_collapse: SetIsDestroyable(FALSE, FALSE, FALSE)
	callSetIsDestroyable(f, creature, 0, 0, 0);
	EXPECT_FALSE(creature.isDestroyable());
	EXPECT_FALSE(creature.isRaiseable());
	EXPECT_FALSE(creature.isSelectableWhenDead());

	// k_pkor_destvictm: SetIsDestroyable(TRUE) (bRaiseDeadPossible = TRUE, bSelectableWhenDead = FALSE)
	callSetIsDestroyable(f, creature, 1, 1, 0);
	EXPECT_TRUE(creature.isDestroyable());
	EXPECT_TRUE(creature.isRaiseable());
	EXPECT_FALSE(creature.isSelectableWhenDead());
}

GTEST_TEST(KotORK1Natives, nonDestroyableCorpseIsClickableOnlyWhenSelectable) {
	TestCreature creature;
	creature.setDestroyable(false, false, false);
	EXPECT_TRUE(creature.isClickable()); // Alive: as always.

	creature.kill();
	ASSERT_TRUE(creature.isDead());
	EXPECT_FALSE(creature.isClickable());

	creature.setDestroyable(false, false, true);
	EXPECT_TRUE(creature.isClickable());

	// Destroyable corpses stay lootable.
	TestCreature looted;
	looted.kill();
	EXPECT_TRUE(looted.isClickable());
}

GTEST_TEST(KotORK1Natives, resurrectionNeedsARaiseableCreature) {
	TestCreature creature;
	creature.setDestroyable(false, false, false);
	creature.kill();

	creature.applyEffect(Effect(kKotOREffectResurrection, 1));
	EXPECT_TRUE(creature.isDead());

	creature.setDestroyable(false, true, false);
	creature.applyEffect(Effect(kKotOREffectResurrection, 1));
	EXPECT_FALSE(creature.isDead());
	EXPECT_EQ(creature.getCurrentHitPoints(), 1);
}

GTEST_TEST(KotORK1Natives, destroyableFlagsSurviveSaveAndLoad) {
	TestCreature creature;
	creature.setDestroyable(false, false, true);

	Aurora::GFF3Writer writer(MKTAG('G', 'F', 'F', ' '));
	creature.saveState(*writer.getTopLevel());
	Aurora::GFF3File gff = roundTrip(writer);

	TestCreature loaded;
	loaded.loadState(gff.getTopLevel());

	EXPECT_FALSE(loaded.isDestroyable());
	EXPECT_FALSE(loaded.isRaiseable());
	EXPECT_TRUE(loaded.isSelectableWhenDead());
}

GTEST_TEST(KotORK1Natives, getLastAttackActionReportsTheLastCombatAction) {
	NativeFixture f;
	TestCreature attacker;

	FunctionContext ctx("GetLastAttackAction");
	ctx.setSignature(Signature { kTypeInt, kTypeObject });
	ctx.getParams()[0] = static_cast<Aurora::NWScript::Object *>(&attacker);

	f.functions.getLastAttackAction(ctx);
	EXPECT_EQ(ctx.getReturn().getInt(), kNWActionInvalid);

	attacker.setLastAttackAction(kActionAttackObject);
	f.functions.getLastAttackAction(ctx);
	EXPECT_EQ(ctx.getReturn().getInt(), kNWActionAttackObject);

	attacker.setLastAttackAction(kActionCastSpell);
	f.functions.getLastAttackAction(ctx);
	EXPECT_EQ(ctx.getReturn().getInt(), kNWActionCastSpell);

	// Not a creature: ACTION_INVALID.
	ctx.getParams()[0] = static_cast<Aurora::NWScript::Object *>(nullptr);
	f.functions.getLastAttackAction(ctx);
	EXPECT_EQ(ctx.getReturn().getInt(), kNWActionInvalid);
}

GTEST_TEST(KotORK1Natives, inventoryDisturbGettersReadTheCaller) {
	NativeFixture f;
	TestCreature container;
	TestCreature item; // Any registered object stands in for the item here.
	item.registerAs(0x7F000101);

	FunctionContext type("GetInventoryDisturbType");
	type.setSignature(Signature { kTypeInt });
	type.setCaller(&container);

	f.functions.getInventoryDisturbType(type);
	EXPECT_EQ(type.getReturn().getInt(), 0);
	EXPECT_EQ(callObjectGetter(f, &TestFunctions::getInventoryDisturbItem, &container), nullptr);

	container.setInventoryDisturbance(kInventoryDisturbTypeAdded, &item);
	f.functions.getInventoryDisturbType(type);
	EXPECT_EQ(type.getReturn().getInt(), kNWDisturbAdded);
	EXPECT_EQ(callObjectGetter(f, &TestFunctions::getInventoryDisturbItem, &container), &item);

	container.setInventoryDisturbance(kInventoryDisturbTypeRemoved, nullptr);
	f.functions.getInventoryDisturbType(type);
	EXPECT_EQ(type.getReturn().getInt(), kNWDisturbRemoved);
	EXPECT_EQ(callObjectGetter(f, &TestFunctions::getInventoryDisturbItem, &container), nullptr);
}

GTEST_TEST(KotORK1Natives, eventConstructorsMakeTheirOwnEventTypes) {
	NativeFixture f;
	TestCreature activator, item, target;
	activator.registerAs(0x7F000201);
	item.registerAs(0x7F000202);
	target.registerAs(0x7F000203);

	FunctionContext conversation("EventConversation");
	conversation.setSignature(Signature { kTypeEngineType });
	f.functions.eventConversation(conversation);

	const Event *e = dynamic_cast<const Event *>(conversation.getReturn().getEngineType());
	ASSERT_NE(e, nullptr);
	EXPECT_EQ(e->getType(), kEventConversation);

	Location location;
	location.setPosition(1.0f, 2.0f, 3.0f);

	FunctionContext activate("EventActivateItem");
	activate.setSignature(Signature { kTypeEngineType, kTypeObject, kTypeEngineType, kTypeObject });
	activate.setCaller(&activator);
	activate.getParams()[0] = static_cast<Aurora::NWScript::Object *>(&item);
	activate.getParams()[1] = &location;
	activate.getParams()[2] = static_cast<Aurora::NWScript::Object *>(&target);
	f.functions.eventActivateItem(activate);

	e = dynamic_cast<const Event *>(activate.getReturn().getEngineType());
	ASSERT_NE(e, nullptr);
	EXPECT_EQ(e->getType(), kEventActivateItem);
	EXPECT_EQ(e->getItem(), &item);
	EXPECT_EQ(e->getActivator(), &activator);
	EXPECT_EQ(e->getTarget(), &target);

	float x, y, z;
	e->getTargetLocation().getPosition(x, y, z);
	EXPECT_FLOAT_EQ(x, 1.0f);
	EXPECT_FLOAT_EQ(y, 2.0f);
	EXPECT_FLOAT_EQ(z, 3.0f);

	// Cloned (stored in a variable, DelayCommand) with everything.
	std::unique_ptr<Aurora::NWScript::EngineType> copy(e->clone());
	const Event *cloned = dynamic_cast<const Event *>(copy.get());
	ASSERT_NE(cloned, nullptr);
	EXPECT_EQ(cloned->getType(), kEventActivateItem);
	EXPECT_EQ(cloned->getItem(), &item);
	EXPECT_EQ(cloned->getTarget(), &target);
}

GTEST_TEST(KotORK1Natives, itemActivationIsOnlyKnownWhileItsEventRuns) {
	NativeFixture f;
	TestCreature module, item;
	module.registerAs(0x7F000301);
	item.registerAs(0x7F000302);

	EXPECT_EQ(callObjectGetter(f, &TestFunctions::getItemActivated, &module), nullptr);
	EXPECT_EQ(callObjectGetter(f, &TestFunctions::getItemActivator, &module), nullptr);
	EXPECT_EQ(callObjectGetter(f, &TestFunctions::getItemActivatedTarget, &module), nullptr);

	Event e(kEventActivateItem);
	e.setActivateItem(&item, &module, nullptr, Location());

	// The target has no OnActivateItem script: nothing runs, nothing sticks.
	FunctionContext signal("SignalEvent");
	signal.setSignature(Signature { kTypeVoid, kTypeObject, kTypeEngineType });
	signal.getParams()[0] = static_cast<Aurora::NWScript::Object *>(&module);
	signal.getParams()[1] = &e;
	f.functions.signalEvent(signal);

	EXPECT_EQ(callObjectGetter(f, &TestFunctions::getItemActivated, &module), nullptr);

	// EventConversation on an object without OnConversation is harmless too.
	Event conversation(kEventConversation);
	signal.getParams()[1] = &conversation;
	f.functions.signalEvent(signal);
}

GTEST_TEST(KotORK1Natives, roomAnimationNamesAreScriptLoops) {
	// ANIMATION_ROOM_SCRIPTLOOPnn = nn; k_pebn_remove plays 7 on the Ebon Hawk.
	EXPECT_STREQ(getRoomScriptLoopAnimation(1).c_str(), "scriptloop01");
	EXPECT_STREQ(getRoomScriptLoopAnimation(7).c_str(), "scriptloop07");
	EXPECT_STREQ(getRoomScriptLoopAnimation(11).c_str(), "scriptloop11");
}

GTEST_TEST(KotORK1Natives, movingContainerItems) {
	Inventory container, party;
	container.addItem("g_w_fraggren01", 3);

	EXPECT_EQ(moveInventoryItems(container, party, "g_w_fraggren01", 2), 2);
	EXPECT_EQ(container.countItems("g_w_fraggren01"), 1);
	EXPECT_EQ(party.countItems("g_w_fraggren01"), 2);

	// Never more than there are; the key may be the erased entry's own.
	const Common::UString &key = container.getItems().begin()->first;
	EXPECT_EQ(moveInventoryItems(container, party, key, 5), 1);
	EXPECT_FALSE(container.hasItem("g_w_fraggren01"));
	EXPECT_EQ(party.countItems("g_w_fraggren01"), 3);

	EXPECT_EQ(moveInventoryItems(container, party, "g_w_fraggren01", 1), 0);
	EXPECT_EQ(moveInventoryItems(party, container, "g_w_fraggren01", 0), 0);
}
