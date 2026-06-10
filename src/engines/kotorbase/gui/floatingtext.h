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

#ifndef ENGINES_KOTORBASE_GUI_FLOATINGTEXT_H
#define ENGINES_KOTORBASE_GUI_FLOATINGTEXT_H

#include <memory>
#include <vector>

#include "src/common/ustring.h"

#include "src/graphics/aurora/fonthandle.h"
#include "src/graphics/aurora/text.h"

namespace Engines {

namespace KotORBase {

class Object;

class FloatingTextManager {
public:
	void show(Object *object, const Common::UString &text, float duration = 6.0f);
	void update(float dt);

private:
	struct Entry {
		Object *object;
		std::unique_ptr<Graphics::Aurora::Text> label;
		float remaining;
	};

	std::vector<Entry> _entries;
	Graphics::Aurora::FontHandle _font;
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_GUI_FLOATINGTEXT_H
