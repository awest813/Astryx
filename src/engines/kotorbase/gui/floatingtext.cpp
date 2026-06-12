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
 *  Floating speech / feedback text above creatures.
 */

#include "src/graphics/graphics.h"
#include "src/graphics/types.h"

#include "src/graphics/aurora/fontman.h"

#include "src/engines/kotorbase/object.h"

#include "src/engines/kotorbase/gui/floatingtext.h"

namespace Engines {

namespace KotORBase {

void FloatingTextManager::show(Object *object, const Common::UString &text, float duration) {
	if (!object || text.empty())
		return;

	if (_font.empty())
		_font = FontMan.get("fnt_d16x16");

	for (auto &entry : _entries) {
		if (entry.object == object) {
			entry.label->setText(text);
			entry.label->show();
			entry.remaining = duration;
			return;
		}
	}

	Entry entry;
	entry.object = object;
	entry.remaining = duration;
	entry.label = std::make_unique<Graphics::Aurora::Text>(
		_font, 320.0f, 64.0f, text,
		1.0f, 1.0f, 1.0f, 1.0f,
		Graphics::Aurora::kHAlignCenter, Graphics::Aurora::kVAlignBottom);
	entry.label->disableColorTokens(true);
	entry.label->show();
	_entries.push_back(std::move(entry));
}

void FloatingTextManager::update(float dt) {
	for (auto it = _entries.begin(); it != _entries.end();) {
		if (!it->object) {
			it->label->hide();
			it = _entries.erase(it);
			continue;
		}

		float x, y, z;
		it->object->getTooltipAnchor(x, y, z);

		float sX, sY, sZ;
		GfxMan.project(x, y, z, sX, sY, sZ);

		if (sZ < 0.0f || sZ > 1.0f) {
			it->label->hide();
		} else {
			it->label->setPosition(sX, sY - 24.0f, -FLT_MAX);
			it->label->show();
		}

		it->remaining -= dt;
		if (it->remaining <= 0.0f) {
			it->label->hide();
			it = _entries.erase(it);
		} else {
			if (it->remaining < 1.0f)
				it->label->setColor(1.0f, 1.0f, 1.0f, it->remaining);
			else
				it->label->setColor(1.0f, 1.0f, 1.0f, 1.0f);

			++it;
		}
	}
}

} // End of namespace KotORBase

} // End of namespace Engines
