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
 *  Visual effects (visualeffects.2da, spells.2da casting visuals) for KotOR games.
 */

#include <cctype>

#include <algorithm>

#include "src/common/util.h"
#include "src/common/debug.h"

#include "src/aurora/2dafile.h"
#include "src/aurora/resman.h"

#include "src/graphics/graphics.h"

#include "src/graphics/aurora/model.h"

#include "src/engines/aurora/model.h"

#include "src/engines/kotorbase/visualeffects.h"

namespace Engines {

namespace KotORBase {

/** How long a model without animations stays, in seconds. */
static const float kDefaultEffectLength = 2.0f;
/** The longest a fire-and-forget effect stays, in seconds. */
static const float kMaxEffectLength = 8.0f;

bool loadVisualEffect(const Aurora::TwoDAFile &visualEffects, int id, VisualEffectInfo &info) {
	if ((id < 0) || (static_cast<size_t>(id) >= visualEffects.getRowCount()))
		return false;

	const Aurora::TwoDARow &row = visualEffects.getRow(id);

	info = VisualEffectInfo();
	info.id = id;

	const Common::UString &type = row.getString("type_fd");
	if (!type.empty())
		info.type = static_cast<char>(toupper(*type.begin()));

	info.impactModel    = row.getString("imp_impact_node");
	info.headModel      = row.getString("imp_headcon_node");
	info.rootModels[0]  = row.getString("imp_root_s_node");
	info.rootModels[1]  = row.getString("imp_root_m_node");
	info.rootModels[2]  = row.getString("imp_root_l_node");
	info.rootModels[3]  = row.getString("imp_root_h_node");
	info.impactSound    = row.getString("soundimpact");

	info.label          = row.getString("label");
	info.shakeType      = row.getInt("shaketype");
	info.shakeDelay     = row.getFloat("shakedelay");
	info.shakeDuration  = row.getFloat("shakeduration");

	return true;
}

/** How long a screen shake without a duration lasts, in seconds. */
static const float kDefaultShakeDuration = 1.0f;
/** How far the camera moves about in a screen shake. */
static const float kShakeIntensity = 0.25f;

bool getVisualEffectShake(const VisualEffectInfo &info, float &duration, float &intensity) {
	const bool shakeColumns = (info.shakeType > 0) || (info.shakeDuration > 0.0f);
	const bool shakeLabel   = info.label.toLower().contains("shake");
	if (!shakeColumns && !shakeLabel)
		return false;

	duration = (info.shakeDuration > 0.0f) ? info.shakeDuration : kDefaultShakeDuration;
	if (duration > 30.0f)
		duration /= 1000.0f; // In milliseconds, then.

	intensity = kShakeIntensity * CLIP(info.shakeType, 1, 3);
	return true;
}

const Common::UString &getVisualEffectRootModel(const VisualEffectInfo &info, int sizeCategory) {
	switch (sizeCategory) {
		case 1: // Tiny
		case 2: // Small
			return info.rootModels[0];
		case 4: // Large
			return info.rootModels[2];
		case 5: // Huge
			return info.rootModels[3];
		default:
			break;
	}

	return info.rootModels[1];
}

Common::UString chooseVisualEffectNode(VisualEffectHook hook,
                                       const std::function<bool(const Common::UString &)> &hasNode) {

	static const char * const kImpactNodes[] = { "impact" };
	static const char * const kHeadNodes  [] = { "headconjure", "headhook", "impact" };
	static const char * const kHandNodes  [] = { "handconjure", "lhand", "rhand", "impact" };

	const char * const *nodes = nullptr;
	size_t count = 0;

	switch (hook) {
		case kVisualEffectHookImpact:
			nodes = kImpactNodes;
			count = ARRAYSIZE(kImpactNodes);
			break;
		case kVisualEffectHookHead:
			nodes = kHeadNodes;
			count = ARRAYSIZE(kHeadNodes);
			break;
		case kVisualEffectHookHand:
			nodes = kHandNodes;
			count = ARRAYSIZE(kHandNodes);
			break;
		case kVisualEffectHookRoot:
		default:
			break;
	}

	for (size_t i = 0; i < count; i++)
		if (hasNode(nodes[i]))
			return nodes[i];

	return "";
}

bool SpellCastVisuals::empty() const {
	return conjureHand.empty() && conjureHead.empty() && conjureGround.empty() &&
	       castHand.empty() && castHead.empty() && castGround.empty() &&
	       conjureSound.empty() && conjureSoundMale.empty() && conjureSoundFemale.empty() &&
	       castSound.empty();
}

/** A cell of either spelling of a spells.2da visual column. */
static Common::UString getSpellVisual(const Aurora::TwoDARow &row, const char *prefix) {
	const Common::UString &vfx = row.getString(Common::UString(prefix) + "vfx");
	if (!vfx.empty())
		return vfx;

	return row.getString(Common::UString(prefix) + "visual");
}

bool loadSpellCastVisuals(const Aurora::TwoDAFile &spells, int spell, SpellCastVisuals &visuals) {
	if ((spell < 0) || (static_cast<size_t>(spell) >= spells.getRowCount()))
		return false;

	const Aurora::TwoDARow &row = spells.getRow(spell);

	visuals = SpellCastVisuals();

	visuals.conjureHand   = getSpellVisual(row, "conjhand");
	visuals.conjureHead   = getSpellVisual(row, "conjhead");
	visuals.conjureGround = getSpellVisual(row, "conjgrnd");
	visuals.castHand      = getSpellVisual(row, "casthand");
	visuals.castHead      = getSpellVisual(row, "casthead");
	visuals.castGround    = getSpellVisual(row, "castgrnd");

	visuals.conjureSound       = row.getString("conjsoundvfx");
	visuals.conjureSoundMale   = row.getString("conjsoundmale");
	visuals.conjureSoundFemale = row.getString("conjsoundfemale");
	visuals.castSound          = row.getString("castsound");

	// In milliseconds
	visuals.conjureTime = MAX(row.getFloat("conjtime"), 0.0f) / 1000.0f;

	return true;
}

Graphics::Aurora::Model *loadVisualEffectModel(const Common::UString &name) {
	if (name.empty() || !ResMan.hasResource(name, Aurora::kFileTypeMDL))
		return nullptr;

	return loadModelObject(name);
}

float playVisualEffectAnimation(Graphics::Aurora::Model &model, bool lasting) {
	if (lasting) {
		static const char * const kLastingAnimations[] = { "duration", "impact" };

		for (const char *anim : kLastingAnimations) {
			if (model.canPlayAnimation(anim)) {
				model.playAnimation(anim, true, -1.0f);
				return -1.0f;
			}
		}

		if (!model.getAnimationNames().empty())
			model.playAnimation(model.getAnimationNames().front(), true, -1.0f);

		return -1.0f;
	}

	static const char * const kAnimations[] = { "impact", "cast01", "conjure", "duration" };

	Common::UString anim;
	for (const char *candidate : kAnimations) {
		if (model.canPlayAnimation(candidate)) {
			anim = candidate;
			break;
		}
	}

	if (anim.empty() && !model.getAnimationNames().empty())
		anim = model.getAnimationNames().front();

	if (anim.empty())
		return kDefaultEffectLength;

	model.playAnimation(anim, true);

	const float length = model.getAnimationLength(anim);
	if (length <= 0.0f)
		return kDefaultEffectLength;

	return MIN(length, kMaxEffectLength);
}


CreatureVisualEffects::CreatureVisualEffects() : _target(nullptr) {
}

CreatureVisualEffects::~CreatureVisualEffects() {
	// The effect models are owned by the creature's model, which deletes them
}

void CreatureVisualEffects::setTarget(Graphics::Aurora::Model *target) {
	if (target == _target)
		return;

	/* The creature's model changed: the effect models went with the old one.
	 * Put the lasting effects on the new model, forget the others. */

	std::vector<Active> lasting;
	for (Active &active : _active) {
		if (!active.lasting)
			continue;

		active.model = nullptr;
		lasting.push_back(active);
	}

	_active.swap(lasting);
	_target = target;

	if (!_target)
		return;

	for (Active &active : _active)
		if (active.delay <= 0.0f)
			start(*_target, active);
}

void CreatureVisualEffects::start(Graphics::Aurora::Model &target, Active &active) {
	active.delay = 0.0f;

	Graphics::Aurora::Model *model = loadVisualEffectModel(active.modelName);
	if (!model) {
		active.timeLeft = 0.0f;
		return;
	}

	const Common::UString node = chooseVisualEffectNode(active.hook, [&target](const Common::UString &name) {
		return target.hasNode(name);
	});

	const float length = playVisualEffectAnimation(*model, active.lasting);
	if (!active.lasting)
		active.timeLeft = length;

	if (!target.addEffectModel(node, model)) {
		// addEffectModel() deleted the model
		active.timeLeft = 0.0f;
		return;
	}

	active.model = model;

	debugC(Common::kDebugEngineGraphics, 2, "Visual effect model \"%s\" on \"%s\" node \"%s\"",
	       active.modelName.c_str(), target.getName().c_str(), node.c_str());
}

void CreatureVisualEffects::stop(Graphics::Aurora::Model *target, Active &active) {
	if (active.model && target && (target == _target))
		target->removeEffectModel(active.model);

	active.model = nullptr;
}

void CreatureVisualEffects::playEffect(Graphics::Aurora::Model &target, const VisualEffectInfo &effect,
                                       int sizeCategory, bool lasting) {

	setTarget(&target);

	const struct {
		const Common::UString &model;
		VisualEffectHook hook;
	} kModels[] = {
		{ effect.impactModel                                   , kVisualEffectHookImpact },
		{ effect.headModel                                     , kVisualEffectHookHead   },
		{ getVisualEffectRootModel(effect, sizeCategory)       , kVisualEffectHookRoot   }
	};

	for (const auto &m : kModels) {
		if (m.model.empty())
			continue;

		Active active;
		active.id        = effect.id;
		active.modelName = m.model;
		active.hook      = m.hook;
		active.lasting   = lasting;

		start(target, active);
		if (active.model)
			_active.push_back(active);
	}
}

void CreatureVisualEffects::stopEffect(Graphics::Aurora::Model *target, int id) {
	setTarget(target);

	/* An effect has up to three models (impact, head, root). Stop the first
	 * instance of each, so that the same effect applied twice keeps one. */
	std::vector<VisualEffectHook> stopped;
	for (std::vector<Active>::iterator a = _active.begin(); a != _active.end(); ) {
		if (!a->lasting || (a->id != id) ||
		    (std::find(stopped.begin(), stopped.end(), a->hook) != stopped.end())) {
			++a;
			continue;
		}

		stopped.push_back(a->hook);
		stop(target, *a);
		a = _active.erase(a);
	}
}

void CreatureVisualEffects::playModel(Graphics::Aurora::Model &target, const Common::UString &model,
                                      VisualEffectHook hook, float delay) {

	if (model.empty())
		return;

	setTarget(&target);

	Active active;
	active.modelName = model;
	active.hook      = hook;
	active.delay     = MAX(delay, 0.0f);

	if (active.delay <= 0.0f) {
		start(target, active);
		if (!active.model)
			return;
	}

	_active.push_back(active);
}

void CreatureVisualEffects::update(Graphics::Aurora::Model *target, float dt) {
	if (_active.empty() && (target == _target))
		return;

	setTarget(target);

	for (std::vector<Active>::iterator a = _active.begin(); a != _active.end(); ) {
		if (a->delay > 0.0f) {
			a->delay -= dt;
			if ((a->delay <= 0.0f) && _target)
				start(*_target, *a);

			if (!a->model && (a->delay <= 0.0f)) {
				a = _active.erase(a);
				continue;
			}

			++a;
			continue;
		}

		if (a->lasting) {
			++a;
			continue;
		}

		a->timeLeft -= dt;
		if (a->timeLeft > 0.0f) {
			++a;
			continue;
		}

		stop(_target, *a);
		a = _active.erase(a);
	}
}

void CreatureVisualEffects::clear(Graphics::Aurora::Model *target) {
	if (target == _target)
		for (Active &active : _active)
			stop(target, active);

	_active.clear();
}

size_t CreatureVisualEffects::getCount() const {
	return _active.size();
}


WorldVisualEffects::WorldVisualEffects() {
}

WorldVisualEffects::~WorldVisualEffects() {
	clear();
}

void WorldVisualEffects::playEffect(const VisualEffectInfo &effect, float x, float y, float z, float duration) {
	const Common::UString *models[] = { &effect.impactModel, &effect.rootModels[1] };

	for (const Common::UString *name : models) {
		std::unique_ptr<Graphics::Aurora::Model> model(loadVisualEffectModel(*name));
		if (!model)
			continue;

		const bool lasting = duration > 0.0f;

		Active active;
		active.timeLeft = playVisualEffectAnimation(*model, lasting);
		if (lasting)
			active.timeLeft = duration;

		model->setPosition(x, y, z);
		model->show();

		active.model = std::move(model);
		_active.push_back(std::move(active));
	}
}

void WorldVisualEffects::update(float dt) {
	for (std::vector<Active>::iterator a = _active.begin(); a != _active.end(); ) {
		a->timeLeft -= dt;
		if (a->timeLeft > 0.0f) {
			++a;
			continue;
		}

		GfxMan.lockFrame();
		a->model->hide();
		a = _active.erase(a);
		GfxMan.unlockFrame();
	}
}

void WorldVisualEffects::clear() {
	if (_active.empty())
		return;

	GfxMan.lockFrame();

	for (Active &active : _active)
		active.model->hide();

	_active.clear();

	GfxMan.unlockFrame();
}

size_t WorldVisualEffects::getCount() const {
	return _active.size();
}

} // End of namespace KotORBase

} // End of namespace Engines
