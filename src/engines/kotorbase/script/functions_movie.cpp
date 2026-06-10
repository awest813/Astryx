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

 *  Star Wars: Knights of the Old Republic engine functions handling movies.

 */



#include "src/common/util.h"
#include "src/common/debug.h"



#include "src/aurora/nwscript/functioncontext.h"



#include "src/engines/aurora/util.h"



#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/module.h"

#include "src/engines/kotorbase/script/functions.h"



namespace Engines {



namespace KotORBase {



void Functions::playMovie(Aurora::NWScript::FunctionContext &ctx) {

	playVideo(ctx.getParams()[0].getString());

}




void Functions::isMoviePlaying(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().isMoviePlaying() ? 1 : 0;
}

void Functions::queueMovie(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().queueMovie(ctx.getParams()[0].getString());
}

void Functions::playMovieQueue(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().playMovieQueue(ctx.getParams()[0].getInt() != 0);
}

void Functions::enableVideoEffect(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber("__video_effect", ctx.getParams()[0].getInt());
}

void Functions::disableVideoEffect(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber("__video_effect", 0);
}

void Functions::setDialogPlaceableCamera(Aurora::NWScript::FunctionContext &ctx) {
	int cameraId = ctx.getParams()[0].getInt();
	_game->getModule().setCinematicCamera(static_cast<uint32_t>(cameraId), 0.0f, Common::UString());
}

} // End of namespace KotORBase
} // End of namespace Engines
