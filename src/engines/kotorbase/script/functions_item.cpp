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
 *  KotOR NWScript item property and identification helpers.
 */

#include "src/aurora/nwscript/functioncontext.h"

#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

static const Common::UString kIdentifiedLocalKey("XOREOS_IDENTIFIED");

void Functions::getIdentified(Aurora::NWScript::FunctionContext &ctx) {
	Object *item = ObjectContainer::toObject(getParamObject(ctx, 0));
	ctx.getReturn() = (item && item->getLocalInt(kIdentifiedLocalKey) != 0) ? 1 : 0;
}

void Functions::setIdentified(Aurora::NWScript::FunctionContext &ctx) {
	int identified = ctx.getParams()[0].getInt();
	Object *item = ObjectContainer::toObject(getParamObject(ctx, 1));
	if (item)
		item->setLocalInt(kIdentifiedLocalKey, identified != 0 ? 1 : 0);
}

void Functions::getGoldPieceValue(Aurora::NWScript::FunctionContext &ctx) {
	Item *item = ObjectContainer::toItem(getParamObject(ctx, 0));
	ctx.getReturn() = item ? item->getCost() : 0;
}

void Functions::getItemHasItemProperty(Aurora::NWScript::FunctionContext &ctx) {
	Item *item = ObjectContainer::toItem(getParamObject(ctx, 0));
	int property = ctx.getParams()[1].getInt();
	ctx.getReturn() = (item && item->hasItemProperty(property)) ? 1 : 0;
}

void Functions::getItemACValue(Aurora::NWScript::FunctionContext &ctx) {
	Item *item = ObjectContainer::toItem(getParamObject(ctx, 0));
	ctx.getReturn() = item ? item->getACBonus() : 0;
}

} // End of namespace KotORBase

} // End of namespace Engines
