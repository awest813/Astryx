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
 *  The Quad to handle one color fades.
 */

#include "src/common/util.h"
#include "src/common/maths.h"

#include "src/graphics/aurora/fadequad.h"

#include "src/events/events.h"

namespace Graphics {

namespace Aurora {

FadeQuad::FadeQuad() : Renderable(kRenderableTypeGUIFront),
	_fadeType(kFadeNone), _r(1.0f), _g(0.0f), _b(0.0f),
	_opacity(0.0f), _startOpacity(0.0f), _wait(0), _run(0), _start(0) {
}

void FadeQuad::getColor(float &r, float &g, float &b) const {
	r = _r;
	g = _g;
	b = _b;
}

void FadeQuad::setColor(float r, float g, float b) {
	_r = r;
	_g = g;
	_b = b;
}

void FadeQuad::getWaitTime(float &wait) const {
	wait = _wait / 1000;
}

void FadeQuad::setWaitTime(float wait) {
	_wait = wait * 1000;
}

void FadeQuad::getRunTime(float &run) const {
	run = _run / 1000;
}

void FadeQuad::setRunTime(float run) {
	_run = run * 1000;
}

float FadeQuad::getFadeStartOpacity(bool fadeIn, float currentOpacity, uint32_t run) {
	// SetGlobalFadeIn(fWait, fLength): "a Fade In that starts after fWait
	// seconds and fades for fLength seconds ... from a color". The screen is
	// covered from the call on, whatever was rendered before.
	if (fadeIn && (run > 0))
		return 1.0f;

	return currentOpacity;
}

float FadeQuad::getFadeOpacity(bool fadeIn, float startOpacity,
                               uint32_t elapsed, uint32_t wait, uint32_t run) {

	if (elapsed <= wait)
		return startOpacity;

	const float time = static_cast<float>(elapsed - wait);

	if (run == 0)
		return fadeIn ? 0.0f : 1.0f;

	const float progress = MIN(1.0f, time / run);

	// From wherever the fade started, so an already covered screen doesn't flash.
	if (fadeIn)
		return MAX(0.0f, startOpacity * (1.0f - progress));

	return MIN(1.0f, startOpacity + (1.0f - startOpacity) * progress);
}

void FadeQuad::fadeIn() {
	_fadeType = kFadeIn;
	_start = EventMan.getTimestamp();
	_opacity = getFadeStartOpacity(true, _opacity, _run);
	_startOpacity = _opacity;
	show();
}

void FadeQuad::fadeOut() {
	_fadeType = kFadeOut;
	_start = EventMan.getTimestamp();
	_opacity = getFadeStartOpacity(false, _opacity, _run);
	// An immediate cut covers the screen at once, not at the next frame.
	if ((_wait == 0) && (_run == 0))
		_opacity = 1.0f;
	_startOpacity = _opacity;
	show();
}

void FadeQuad::setHoldFadeIn(bool hold) {
	_holdFadeIn = hold;
}

bool FadeQuad::isHoldFadeIn() const {
	return _holdFadeIn;
}

void FadeQuad::calculateDistance() {
}

void FadeQuad::render(Graphics::RenderPass pass) {
	bool isTransparent = _opacity < 1.0f;
	if (((pass == Graphics::kRenderPassOpaque     ) &&  isTransparent) ||
	    ((pass == Graphics::kRenderPassTransparent) && !isTransparent))
		return;

	if (_fadeType == kFadeNone)
		return;

	const uint32_t time = EventMan.getTimestamp() - _start;

	_opacity = getFadeOpacity(_fadeType == kFadeIn, _startOpacity, time, _wait, _run);

	if (_holdFadeIn && _fadeType == kFadeIn)
		_opacity = 1.0f;

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	glColor4f(_r, _g, _b, _opacity);

	float width, height;
	width = viewport[2] / 2;
	height = viewport[3] / 2;

	glBegin(GL_QUADS);
	glVertex3f(width, height, -FLT_MAX);
	glVertex3f(-width, height, -FLT_MAX);
	glVertex3f(-width, -height, -FLT_MAX);
	glVertex3f(width, -height, -FLT_MAX);
	glEnd();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

} // End of namespace Aurora

} // End of namespace Graphics
