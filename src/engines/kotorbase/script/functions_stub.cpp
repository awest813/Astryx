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
 *  Safe architectural stubs for KotOR NWScript functions not yet fully implemented.
 */

#include "src/common/debug.h"
#include "src/common/ustring.h"

#include "src/aurora/nwscript/functioncontext.h"
#include "src/aurora/nwscript/types.h"

#include "src/engines/kotorbase/effect.h"
#include "src/engines/kotorbase/location.h"
#include "src/engines/kotorbase/script/event.h"
#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

static bool isEffectPassThroughStub(const Common::UString &name) {
	return name == "MagicalEffect" ||
	       name == "SupernaturalEffect" ||
	       name == "ExtraordinaryEffect" ||
	       name == "VersusAlignmentEffect" ||
	       name == "VersusRacialTypeEffect" ||
	       name == "VersusTrapEffect";
}

static bool isLocationStub(const Common::UString &name) {
	return name.contains("Location") || name == "GetStartingLocation";
}

static bool isEventStub(const Common::UString &name) {
	return name.beginsWith("Event");
}

static int stubIntDefault(const Common::UString &name) {
	if (name == "GetIsDay" || name == "GetIsDawn")
		return 1;
	if (name == "GetIsNight" || name == "GetIsDusk")
		return 0;
	if (name == "GetSpellSaveDC")
		return 12;
	if (name == "GetIdentified")
		return 1;
	return 0;
}

static void stubEngineTypeReturn(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString &name = ctx.getName();

	if (isEffectPassThroughStub(name)) {
		const Effect *effect = dynamic_cast<const Effect *>(ctx.getParams()[0].getEngineType());
		ctx.getReturn() = effect ? effect->clone() : new Effect(kKotOREffectVisual, 0);
		return;
	}

	if (isLocationStub(name)) {
		ctx.getReturn() = new Location();
		return;
	}

	if (isEventStub(name)) {
		ctx.getReturn() = new Event(kEventUserDefined);
		return;
	}

	ctx.getReturn() = new Effect(kKotOREffectVisual, 0);
}

static void applyStubReturn(Aurora::NWScript::FunctionContext &ctx, bool useSmartIntDefaults) {
	switch (ctx.getReturn().getType()) {
		case Aurora::NWScript::kTypeInt:
			ctx.getReturn().setType(Aurora::NWScript::kTypeInt);
			ctx.getReturn() = useSmartIntDefaults ? stubIntDefault(ctx.getName()) : 0;
			break;
		case Aurora::NWScript::kTypeFloat:
			ctx.getReturn().setType(Aurora::NWScript::kTypeFloat);
			ctx.getReturn() = 0.0f;
			break;
		case Aurora::NWScript::kTypeString:
			ctx.getReturn().setType(Aurora::NWScript::kTypeString);
			ctx.getReturn() = Common::UString("");
			break;
		case Aurora::NWScript::kTypeObject:
			ctx.getReturn().setType(Aurora::NWScript::kTypeObject);
			break;
		case Aurora::NWScript::kTypeVector:
			ctx.getReturn().setType(Aurora::NWScript::kTypeVector);
			ctx.getReturn().setVector(0.0f, 0.0f, 0.0f);
			break;
		case Aurora::NWScript::kTypeEngineType:
			if (useSmartIntDefaults)
				stubEngineTypeReturn(ctx);
			break;
		default:
			break;
	}
}

void Functions::stubFunction(Aurora::NWScript::FunctionContext &ctx) {
	debugC(Common::kDebugEngineLogic, 3, "NWScript stub: %s", ctx.getName().c_str());
	applyStubReturn(ctx, true);
}

void Functions::stubSWMGFunction(Aurora::NWScript::FunctionContext &ctx) {
	debugC(Common::kDebugEngineLogic, 3, "NWScript SWMG stub: %s", ctx.getName().c_str());
	applyStubReturn(ctx, false);
}

} // End of namespace KotORBase

} // End of namespace Engines
