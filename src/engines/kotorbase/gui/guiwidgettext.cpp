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
 *  Helpers for setting dynamic text on KotOR GUI widgets.
 */

#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/label.h"
#include "src/engines/odyssey/widget.h"

#include "src/engines/kotorbase/gui/gui.h"

namespace Engines {

namespace KotORBase {

static const Common::UString kDefaultWidgetFont("fnt_d16x16");

static void applyWidgetText(Odyssey::Widget *widget, const Common::UString &text) {
	if (!widget)
		return;

	if (!widget->hasTextComponent())
		widget->ensureText(kDefaultWidgetFont);

	widget->setText(text);
}

void GUI::setWidgetText(const Common::UString &tag, const Common::UString &text) {
	if (Odyssey::WidgetLabel *label = getLabel(tag, false)) {
		applyWidgetText(label, text);
		return;
	}

	if (Odyssey::WidgetButton *button = getButton(tag, false))
		applyWidgetText(button, text);
}

void GUI::setWidgetTextAliases(const char *const *tags, size_t count, const Common::UString &text) {
	for (size_t i = 0; i < count; ++i)
		setWidgetText(tags[i], text);
}

} // End of namespace KotORBase

} // End of namespace Engines
