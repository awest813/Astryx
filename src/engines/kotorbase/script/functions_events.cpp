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
 *  Star Wars: Knights of the Old Republic engine functions for handling events.
 */

#include "src/common/util.h"

#include "src/aurora/nwscript/functioncontext.h"

#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/types.h"

#include "src/engines/kotorbase/script/functions.h"
#include "src/engines/kotorbase/script/event.h"

namespace Engines {

namespace KotORBase {

void Functions::signalEvent(Aurora::NWScript::FunctionContext &ctx) {
	Object *object = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (!object)
		object = _game->getModule().getPC();

	Event *e = static_cast<Event *>(ctx.getParams()[1].getEngineType());
	if (!e) {
		warning("Functions::signalEvent(): invalid event");
		return;
	}

	switch (e->getType()) {
		case kEventUserDefined:
			_game->getModule().signalUserDefinedEvent(object, e->getUserDefinedNumber());
			break;
		case kEventSpellCastAt:
			if (!object)
				return;
			_game->getModule().setSpellScriptContext(e->getSpellId(), object, 12, e->getSpellHarmful());
			object->runScript(kScriptSpellCastAt, object, ctx.getCaller());
			break;
		default:
			warning("Functions::signalEvent(): Event type \"%d\" not implemented", e->getType());
			break;
	}
}

void Functions::eventUserDefined(Aurora::NWScript::FunctionContext &ctx) {
	Event e(kEventUserDefined);
	e.setUserDefinedNumber(ctx.getParams()[0].getInt());
	ctx.getReturn() = e;
}

void Functions::eventSpellCastAt(Aurora::NWScript::FunctionContext &ctx) {
	// EventSpellCastAt(object oCaster, int nSpell, int bHarmful=TRUE)
	Event e(kEventSpellCastAt);
	e.setSpellId(ctx.getParams()[1].getInt());
	e.setSpellHarmful(ctx.getParams()[2].getInt() != 0);
	ctx.getReturn() = e;
}

void Functions::getUserDefinedEventNumber(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getUserDefinedEventNumber();
}

} // End of namespace KotORBase

} // End of namespace Engines
