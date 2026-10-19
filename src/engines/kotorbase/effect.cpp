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
 *  Effect engine type for KotOR games.
 */

#include "src/engines/kotorbase/effect.h"

namespace Engines {

namespace KotORBase {

Effect::Effect(KotOREffectType type, int amount, int damageType, int spellId, int limit) :
		_type(type),
		_amount(amount),
		_damageType(damageType),
		_spellId(spellId),
		_limit(limit) {
}

KotOREffectType Effect::getType() const {
	return _type;
}

int Effect::getAmount() const {
	return _amount;
}

int Effect::getDamageType() const {
	return _damageType;
}

int Effect::getLimit() const {
	return _limit;
}

int Effect::getSpellId() const {
	return _spellId;
}

void Effect::setSpellId(int spellId) {
	_spellId = spellId;

	for (Effect &linked : _linked)
		linked._spellId = spellId;
}

const std::vector<Effect> &Effect::getLinkedEffects() const {
	return _linked;
}

void Effect::linkEffect(const Effect &effect) {
	Effect part(effect);
	part._linked.clear();
	_linked.push_back(part);

	_linked.insert(_linked.end(), effect._linked.begin(), effect._linked.end());
}

bool Effect::hasPart(KotOREffectType type) const {
	if (_type == type)
		return true;

	for (const Effect &linked : _linked)
		if (linked._type == type)
			return true;

	return false;
}

Aurora::NWScript::EngineType *Effect::clone() const {
	return new Effect(*this);
}

Effect linkEffects(const Effect &child, const Effect &parent) {
	Effect link(parent);
	link.linkEffect(child);
	return link;
}

} // End of namespace KotORBase

} // End of namespace Engines
