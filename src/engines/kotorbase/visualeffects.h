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

#ifndef ENGINES_KOTORBASE_VISUALEFFECTS_H
#define ENGINES_KOTORBASE_VISUALEFFECTS_H

#include <functional>
#include <memory>
#include <vector>

#include "src/common/ustring.h"

namespace Aurora {
	class TwoDAFile;
}

namespace Graphics {
	namespace Aurora {
		class Model;
	}
}

namespace Engines {

namespace KotORBase {

/** Where on a creature a visual effect model goes. */
enum VisualEffectHook {
	kVisualEffectHookImpact, ///< Where hits land: the "impact" node.
	kVisualEffectHookHead,   ///< Above the head: "headconjure".
	kVisualEffectHookHand,   ///< In the (left) hand: "handconjure".
	kVisualEffectHookRoot    ///< At the feet: the model's root.
};

/** A row of visualeffects.2da. */
struct VisualEffectInfo {
	int id { -1 };
	/** "type_fd": 'F' plays once (fire and forget), 'D' lasts as long as its
	 *  effect, 'B' is a beam between two objects. */
	char type { 'F' };

	Common::UString impactModel; ///< "imp_impact_node".
	Common::UString headModel;   ///< "imp_headcon_node".
	/** "imp_root_s_node", "imp_root_m_node", "imp_root_l_node", "imp_root_h_node". */
	Common::UString rootModels[4];

	Common::UString impactSound; ///< "soundimpact".

	Common::UString label; ///< "label", e.g. "VFX_IMP_SCREEN_SHAKE".

	int   shakeType { 0 };        ///< "shaketype": does it shake the screen (and how)?
	float shakeDelay { 0.0f };    ///< "shakedelay", in seconds.
	float shakeDuration { 0.0f }; ///< "shakeduration", in seconds.

	bool isDuration() const { return type == 'D'; }
	bool isBeam() const { return type == 'B'; }
};

/** Read row @p id of visualeffects.2da. False if there's no such row. */
bool loadVisualEffect(const Aurora::TwoDAFile &visualEffects, int id, VisualEffectInfo &info);

/** Does the effect shake the camera, and for how long and how hard?
 *
 *  As in the Aurora visualeffects.2da, the "shaketype"/"shakeduration" columns
 *  make an effect shake the screen (PlayExplosion's VFX_IMP_SCREEN_SHAKE); a
 *  table without them still shakes for a row labelled as a screen shake.
 */
bool getVisualEffectShake(const VisualEffectInfo &info, float &duration, float &intensity);

/** The root model of a visual effect for a creature of appearance.2da
 *  "sizecategory" @p sizeCategory (1 tiny, 2 small, 3 medium, 4 large, 5 huge). */
const Common::UString &getVisualEffectRootModel(const VisualEffectInfo &info, int sizeCategory);

/** The node of a creature model a visual effect model goes on.
 *
 *  @param hook    Where the effect goes.
 *  @param hasNode Does the creature model have this node?
 *  @return The node, or an empty string for the model's root.
 */
Common::UString chooseVisualEffectNode(VisualEffectHook hook,
                                       const std::function<bool(const Common::UString &)> &hasNode);

/** The casting visuals of a spells.2da row.
 *
 *  The visuals are model names or, when numeric, visualeffects.2da rows. Both
 *  the "conjhandvfx" and the "conjhandvisual" spellings of the columns are read.
 */
struct SpellCastVisuals {
	Common::UString conjureHand;   ///< "conjhandvfx": in the hand, while conjuring.
	Common::UString conjureHead;   ///< "conjheadvfx".
	Common::UString conjureGround; ///< "conjgrndvfx".
	Common::UString castHand;      ///< "casthandvfx": in the hand, when releasing the power.
	Common::UString castHead;      ///< "castheadvfx".
	Common::UString castGround;    ///< "castgrndvfx".

	Common::UString conjureSound;       ///< "conjsoundvfx".
	Common::UString conjureSoundMale;   ///< "conjsoundmale".
	Common::UString conjureSoundFemale; ///< "conjsoundfemale".
	Common::UString castSound;          ///< "castsound".

	float conjureTime { 0.0f }; ///< "conjtime", in seconds.

	bool empty() const;
};

/** Read the casting visuals of row @p spell of spells.2da. False if there's no such row. */
bool loadSpellCastVisuals(const Aurora::TwoDAFile &spells, int spell, SpellCastVisuals &visuals);

/** The visual effect models playing on one creature model.
 *
 *  The models are attached to the creature model with addEffectModel(), which
 *  owns them. When the creature's model is replaced (re-equipping reloads
 *  it), the old effect models went with it; lasting effects are put on the
 *  new model.
 */
class CreatureVisualEffects {
public:
	CreatureVisualEffects();
	~CreatureVisualEffects();

	/** Play a visualeffects.2da effect.
	 *
	 *  @param target       The creature's model.
	 *  @param effect       The effect.
	 *  @param sizeCategory The creature's appearance.2da "sizecategory".
	 *  @param lasting      Keep it until stopEffect() (duration effects).
	 */
	void playEffect(Graphics::Aurora::Model &target, const VisualEffectInfo &effect,
	                int sizeCategory, bool lasting);
	/** Stop one instance of lasting effect @p id. */
	void stopEffect(Graphics::Aurora::Model *target, int id);

	/** Play a model once at a hook, after @p delay seconds. */
	void playModel(Graphics::Aurora::Model &target, const Common::UString &model,
	               VisualEffectHook hook, float delay = 0.0f);

	/** Advance time; @p target is the creature's current model (or nullptr). */
	void update(Graphics::Aurora::Model *target, float dt);

	/** Remove all effect models. */
	void clear(Graphics::Aurora::Model *target);

	/** How many effect models are playing or waiting to play? */
	size_t getCount() const;

private:
	struct Active {
		int id { -1 };                       ///< visualeffects.2da row, -1 for a plain model.
		Common::UString modelName;
		VisualEffectHook hook { kVisualEffectHookImpact };
		Graphics::Aurora::Model *model { nullptr };
		bool lasting { false };
		float delay { 0.0f };                ///< Seconds until it starts.
		float timeLeft { 0.0f };             ///< Seconds until it ends, unless lasting.
	};

	Graphics::Aurora::Model *_target; ///< The model the effect models are on.
	std::vector<Active> _active;

	void start(Graphics::Aurora::Model &target, Active &active);
	void stop(Graphics::Aurora::Model *target, Active &active);
	void setTarget(Graphics::Aurora::Model *target);
};

/** Visual effects played at a place in the world (ApplyEffectAtLocation). */
class WorldVisualEffects {
public:
	WorldVisualEffects();
	~WorldVisualEffects();

	/** Play the models of a visualeffects.2da effect at a position. */
	void playEffect(const VisualEffectInfo &effect, float x, float y, float z, float duration = 0.0f);

	/** Advance time, removing finished effects. */
	void update(float dt);
	/** Remove all effects. */
	void clear();

	size_t getCount() const;

private:
	struct Active {
		std::unique_ptr<Graphics::Aurora::Model> model;
		float timeLeft { 0.0f };
	};

	std::vector<Active> _active;
};

/** Load a model for a visual effect, nullptr if there's no such model. */
Graphics::Aurora::Model *loadVisualEffectModel(const Common::UString &name);

/** Start a visual effect model's animation and return how long it lasts.
 *  @param lasting Loop it ("duration" animations of lasting effects). */
float playVisualEffectAnimation(Graphics::Aurora::Model &model, bool lasting);

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_VISUALEFFECTS_H
