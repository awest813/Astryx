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
 *  The ingame minimap.
 */

#include "external/glm/mat4x4.hpp"
#include "external/glm/gtc/type_ptr.hpp"
#include "external/glm/gtc/matrix_transform.hpp"

#include "src/common/util.h"

#include "src/engines/kotor/gui/ingame/minimap.h"

namespace Engines {

namespace KotOR {

namespace {

static const int kFogCellSize = 32;
static const int kFogCols = 17;
static const int kFogRows = 9;

} // Anonymous namespace

Minimap::Minimap(const Common::UString &map, int northAxis,
                 float mapPt1X, float mapPt1Y, float mapPt2X, float mapPt2Y,
                 float worldPt1X, float worldPt1Y, float worldPt2X, float worldPt2Y) :
		_mapQuad("lbl_map" + map, 0, 0, 512, 256), _mapTexture("lbl_map" + map), _northAxis(northAxis),
		_mapPt1X(mapPt1X), _mapPt1Y(mapPt1Y), _mapPt2X(mapPt2X), _mapPt2Y(mapPt2Y),
		_worldPt1X(worldPt1X), _worldPt1Y(worldPt1Y), _worldPt2X(worldPt2X), _worldPt2Y(worldPt2Y) {

	add(&_mapQuad);
	buildFogTiles();

	glm::mat4 projection(glm::ortho(0.0f, 120.0f, 0.0f, 120.0f, -1.0f, 1.0f));

	setProjectionMatrix(projection);
}

void Minimap::buildFogTiles() {
	_fogTiles.clear();

	for (int row = 0; row < kFogRows; ++row) {
		for (int col = 0; col < kFogCols; ++col) {
			const float x1 = static_cast<float>(col * kFogCellSize);
			const float y1 = static_cast<float>(256 - (row + 1) * kFogCellSize);
			const float x2 = x1 + kFogCellSize;
			const float y2 = y1 + kFogCellSize;

			std::unique_ptr<Graphics::Aurora::GUIQuad> fog(
				new Graphics::Aurora::GUIQuad(_mapTexture, x1, y1, x2, y2));
			fog->setColor(0.0f, 0.0f, 0.0f, 0.85f);
			add(fog.get());
			_fogTiles.push_back(std::move(fog));
		}
	}
}

void Minimap::setMapExplored(const std::vector<bool> &explored) {
	for (size_t i = 0; i < _fogTiles.size(); ++i) {
		const bool fogged = i >= explored.size() || !explored[i];
		if (fogged)
			_fogTiles[i]->show();
		else
			_fogTiles[i]->hide();
	}
}

void Minimap::setPosition(float x, float y) {
	glm::mat4 transformation;

	float scaleX, scaleY, relX, relY;
	switch (_northAxis) {
		case 0:
			scaleY = (_mapPt1Y - _mapPt2Y) / (_worldPt1Y - _worldPt2Y);
			scaleX = (_mapPt1X - _mapPt2X) / (_worldPt1X - _worldPt2X);
			relX = (x - _worldPt1X) * scaleX + _mapPt1X;
			relY = (y - _worldPt1Y) * scaleY + _mapPt1Y;
			transformation = glm::translate(transformation, glm::vec3(-(relX * 435) + 60, -(256 - relY * 256) + 60, 0));
			break;

		case 3:
			scaleX = (_mapPt1Y - _mapPt2Y) / (_worldPt1X - _worldPt2X);
			scaleY = (_mapPt1X - _mapPt2X) / (_worldPt1Y - _worldPt2Y);
			relX = (y - _worldPt1Y) * scaleY + _mapPt1X;
			relY = (x - _worldPt1X) * scaleX + _mapPt1Y;
			transformation = glm::translate(transformation, glm::vec3(-(relX * 435) + 60, -(256 - relY * 256) + 60, 0));
			break;

		default:
			warning("Unknown north axis");
	}

	setGlobalTransformationMatrix(transformation);
}

int Minimap::getNorthAxis() {
	return _northAxis;
}

} // End of namespace KotOR

} // End of namespace Engines
