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
 *  Star Wars: Knights of the Old Republic engine functions managing
 *  local variables.
 */

#include "src/common/ustring.h"
#include "src/common/strutil.h"

#include "src/aurora/nwscript/functioncontext.h"
#include "src/aurora/nwscript/variable.h"
#include "src/aurora/nwscript/types.h"
#include "src/aurora/nwscript/object.h"

#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

void Functions::getLocalInt(Aurora::NWScript::FunctionContext &ctx) {
	Object *object = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (object)
		ctx.getReturn() = object->getLocalInt(ctx.getParams()[1].getString());
	else
		ctx.getReturn() = (int32_t) 0;
}

void Functions::setLocalInt(Aurora::NWScript::FunctionContext &ctx) {
	Object *object = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (object)
		object->setLocalInt(ctx.getParams()[1].getString(), ctx.getParams()[2].getInt());
}

void Functions::getLocalFloat(Aurora::NWScript::FunctionContext &ctx) {
	Object *object = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (object)
		ctx.getReturn() = object->getLocalFloat(ctx.getParams()[1].getString());
	else
		ctx.getReturn() = 0.0f;
}

void Functions::setLocalFloat(Aurora::NWScript::FunctionContext &ctx) {
	Object *object = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (object)
		object->setLocalFloat(ctx.getParams()[1].getString(), ctx.getParams()[2].getFloat());
}

void Functions::getLocalString(Aurora::NWScript::FunctionContext &ctx) {
	Object *object = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (object)
		ctx.getReturn() = object->getLocalString(ctx.getParams()[1].getString());
	else
		ctx.getReturn() = Common::UString("");
}

void Functions::setLocalString(Aurora::NWScript::FunctionContext &ctx) {
	Object *object = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (object)
		object->setLocalString(ctx.getParams()[1].getString(), ctx.getParams()[2].getString());
}

// Local booleans and local numbers are separate per-object tables in KotOR
// (index 0..63 of each), so they must not share variable names. They are
// kept as object local ints, which object save states persist.

static Common::UString localBooleanName(int index) {
	return "__localbool_" + Common::composeString(index);
}

static Common::UString localNumberName(int index) {
	return "__localnum_" + Common::composeString(index);
}

void Functions::getLocalBoolean(Aurora::NWScript::FunctionContext &ctx) {
	Object *object = ObjectContainer::toObject(getParamObject(ctx, 0));
	ctx.getReturn() = (object && object->getLocalInt(localBooleanName(ctx.getParams()[1].getInt())) != 0) ? 1 : 0;
}

void Functions::setLocalBoolean(Aurora::NWScript::FunctionContext &ctx) {
	Object *object = ObjectContainer::toObject(getParamObject(ctx, 0));
	if (object)
		object->setLocalInt(localBooleanName(ctx.getParams()[1].getInt()), (ctx.getParams()[2].getInt() != 0) ? 1 : 0);
}

void Functions::getLocalNumber(Aurora::NWScript::FunctionContext &ctx) {
	Object *object = ObjectContainer::toObject(getParamObject(ctx, 0));
	ctx.getReturn() = object ? object->getLocalInt(localNumberName(ctx.getParams()[1].getInt())) : 0;
}

void Functions::setLocalNumber(Aurora::NWScript::FunctionContext &ctx) {
	Object *object = ObjectContainer::toObject(getParamObject(ctx, 0));
	if (object)
		object->setLocalInt(localNumberName(ctx.getParams()[1].getInt()), ctx.getParams()[2].getInt());
}

} // End of namespace KotORBase

} // End of namespace Engines
