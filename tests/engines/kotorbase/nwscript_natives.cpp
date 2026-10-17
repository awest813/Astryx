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
 *  Unit tests for KotOR NWScript natives that only manipulate their
 *  FunctionContext: return types must match the signature (the VM throws on a
 *  mismatch) and parameters must not be read past the declared count.
 */

#include "gtest/gtest.h"

#include <cstdlib>

#include "src/common/ustring.h"

#include "src/aurora/nwscript/types.h"
#include "src/aurora/nwscript/variable.h"
#include "src/aurora/nwscript/functioncontext.h"

#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/effect.h"
#include "src/engines/kotorbase/script/functions.h"

using namespace Engines::KotORBase;

using Aurora::NWScript::FunctionContext;
using Aurora::NWScript::Signature;
using Aurora::NWScript::kTypeInt;
using Aurora::NWScript::kTypeFloat;
using Aurora::NWScript::kTypeString;
using Aurora::NWScript::kTypeEngineType;

namespace {

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
		// Game only stores the console's address.
		alignas(16) static unsigned char storage[16];
		return reinterpret_cast<Engines::Console &>(storage);
	}
};

class TestFunctions : public Functions {
public:
	explicit TestFunctions(Game &game) : Functions(game) {}

	void registerFunctions() override {}

	using Functions::getStringLeft;
	using Functions::getStringRight;
	using Functions::roundsToSeconds;
	using Functions::hoursToSeconds;
	using Functions::turnsToSeconds;
	using Functions::effectPoison;
	using Functions::effectSeeInvisible;
	using Functions::effectLinkEffects;
	using Functions::setEffectIcon;
};

struct NativeFixture {
	TestGame game;
	TestFunctions functions { game };
};

Common::UString callString(void (TestFunctions::*fn)(FunctionContext &),
                           const Common::UString &str, int n) {
	NativeFixture f;
	FunctionContext ctx("test");
	ctx.setSignature(Signature { kTypeString, kTypeString, kTypeInt });
	ctx.getParams()[0] = str;
	ctx.getParams()[1] = static_cast<int32_t>(n);

	(f.functions.*fn)(ctx);
	return ctx.getReturn().getString();
}

float callIntToFloat(void (TestFunctions::*fn)(FunctionContext &), int n) {
	NativeFixture f;
	FunctionContext ctx("test");
	ctx.setSignature(Signature { kTypeFloat, kTypeInt });
	ctx.getParams()[0] = static_cast<int32_t>(n);

	(f.functions.*fn)(ctx);
	return ctx.getReturn().getFloat();
}

} // End of anonymous namespace

GTEST_TEST(KotORNWScriptNatives, getStringLeft) {
	EXPECT_STREQ(callString(&TestFunctions::getStringLeft, "abc", 2).c_str(), "ab");
	EXPECT_STREQ(callString(&TestFunctions::getStringLeft, "abc", 3).c_str(), "abc");
	EXPECT_STREQ(callString(&TestFunctions::getStringLeft, "abc", 7).c_str(), "abc");
	EXPECT_STREQ(callString(&TestFunctions::getStringLeft, "abc", 0).c_str(), "");
	EXPECT_STREQ(callString(&TestFunctions::getStringLeft, "abc", -1).c_str(), "");
}

GTEST_TEST(KotORNWScriptNatives, getStringRight) {
	EXPECT_STREQ(callString(&TestFunctions::getStringRight, "abc", 1).c_str(), "c");
	EXPECT_STREQ(callString(&TestFunctions::getStringRight, "abc", 3).c_str(), "abc");
	EXPECT_STREQ(callString(&TestFunctions::getStringRight, "abc", 4).c_str(), "abc");
	EXPECT_STREQ(callString(&TestFunctions::getStringRight, "abc", 0).c_str(), "");
}

GTEST_TEST(KotORNWScriptNatives, timeConversionsReturnFloat) {
	EXPECT_FLOAT_EQ(callIntToFloat(&TestFunctions::roundsToSeconds, 2), 12.0f);
	EXPECT_FLOAT_EQ(callIntToFloat(&TestFunctions::turnsToSeconds, 2), 120.0f);
	EXPECT_FLOAT_EQ(callIntToFloat(&TestFunctions::hoursToSeconds, 1), 3600.0f);
}

GTEST_TEST(KotORNWScriptNatives, effectPoisonReadsOnlyPoisonType) {
	NativeFixture f;
	FunctionContext ctx("EffectPoison");
	ctx.setSignature(Signature { kTypeEngineType, kTypeInt });
	ctx.getParams()[0] = static_cast<int32_t>(5);

	f.functions.effectPoison(ctx);

	const Effect *effect = dynamic_cast<const Effect *>(ctx.getReturn().getEngineType());
	ASSERT_NE(effect, nullptr);
	EXPECT_EQ(effect->getType(), kKotOREffectPoison);
	EXPECT_GT(effect->getAmount(), 0);
}

GTEST_TEST(KotORNWScriptNatives, effectSeeInvisibleTakesNoParameters) {
	NativeFixture f;
	FunctionContext ctx("EffectSeeInvisible");
	ctx.setSignature(Signature { kTypeEngineType });

	f.functions.effectSeeInvisible(ctx);

	const Effect *effect = dynamic_cast<const Effect *>(ctx.getReturn().getEngineType());
	ASSERT_NE(effect, nullptr);
	EXPECT_EQ(effect->getType(), kKotOREffectSeeInvisible);
}

GTEST_TEST(KotORNWScriptNatives, effectLinkEffectsPrefersGameplayEffect) {
	NativeFixture f;
	FunctionContext ctx("EffectLinkEffects");
	ctx.setSignature(Signature { kTypeEngineType, kTypeEngineType, kTypeEngineType });

	const Effect visual(kKotOREffectVisual, 42);
	const Effect damage(kKotOREffectDamage, 7);

	ctx.getParams()[0] = &visual;
	ctx.getParams()[1] = &damage;
	f.functions.effectLinkEffects(ctx);

	const Effect *linked = dynamic_cast<const Effect *>(ctx.getReturn().getEngineType());
	ASSERT_NE(linked, nullptr);
	EXPECT_EQ(linked->getType(), kKotOREffectDamage);
	EXPECT_EQ(linked->getAmount(), 7);

	ctx.getParams()[0] = &damage;
	ctx.getParams()[1] = &visual;
	f.functions.effectLinkEffects(ctx);

	linked = dynamic_cast<const Effect *>(ctx.getReturn().getEngineType());
	ASSERT_NE(linked, nullptr);
	EXPECT_EQ(linked->getType(), kKotOREffectDamage);
}

GTEST_TEST(KotORNWScriptNatives, setEffectIconReturnsEffect) {
	NativeFixture f;
	FunctionContext ctx("SetEffectIcon");
	ctx.setSignature(Signature { kTypeEngineType, kTypeEngineType, kTypeInt });

	const Effect haste(kKotOREffectHaste, 3);
	ctx.getParams()[0] = &haste;
	ctx.getParams()[1] = static_cast<int32_t>(12);

	f.functions.setEffectIcon(ctx);

	const Effect *effect = dynamic_cast<const Effect *>(ctx.getReturn().getEngineType());
	ASSERT_NE(effect, nullptr);
	EXPECT_EQ(effect->getType(), kKotOREffectHaste);
	EXPECT_EQ(effect->getAmount(), 3);
}
