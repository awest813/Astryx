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
 *  Situated object within an area in KotOR games.
 */

#ifndef ENGINES_KOTORBASE_SITUATED_H
#define ENGINES_KOTORBASE_SITUATED_H

#include "external/glm/vec3.hpp"

#include <memory>

#include "src/aurora/types.h"

#include "src/graphics/aurora/types.h"

#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/trap.h"

namespace Engines {

namespace KotORBase {

/** Bashing locked doors and containers open (KotOR.js' physical attack on a
 *  locked target). Every blow loses the object's Hardness off its damage; at
 *  0 hit points the object breaks: a door opens for good (and no longer blocks
 *  the way), a container is left unlocked and open for the taking. */
namespace BashRules {

/** A melee attacker reaches a door or container from this much (m) farther
 *  than a creature: its position is the middle of the object. */
extern const float kExtraReach;

/** Can an object be bashed open? It has to be locked, have hit points, and be
 *  neither plot (indestructible) nor "NotBlastable", nor broken already. */
bool canBash(bool locked, bool plot, bool notBlastable, bool broken, int hitPoints);

/** The damage a blow of @p damage does to an object with @p hardness. */
int damageAfterHardness(int damage, int hardness);

/** The hit points an object has left after @p damage; a Min1HP object keeps one. */
int hitPointsAfterDamage(int hitPoints, int damage, bool minOneHitPoint);

} // End of namespace BashRules

class Situated : public Object {
public:
	// Basic visuals

	/** Is the situated object's model visible? */
	bool isVisible() const;

	/** Show the situated object's model. */
	void show();
	/** Hide the situated object's model. */
	void hide();

	// Basic properties

	/** Return the object that last opened this situated object. */
	Object *getLastOpenedBy() const;
	/** Return the object that last closed this situated object. */
	Object *getLastClosedBy() const;
	/** Return the object that last used this situated object. */
	Object *getLastUsedBy() const;
	/** Drop every reference to an object that is about to be destroyed. */
	void forgetObject(const Object *object);
	/** Get the conversation for this object. */
	const Common::UString &getConversation() const;
	/** Get the model name. */
	const Common::UString &getModelName() const;

	// Interactive properties

	const std::vector<int> getPossibleActions() const;

	void setUsable(bool usable);

	// State

	/** Is the situated object open? */
	virtual bool isOpen() const = 0;
	/** Is the situated object locked? */
	bool isLocked() const;
	/** Is a key required to unlock this situated object? */
	bool isKeyRequired() const;
	/** Get the key tag for this situated object. */
	const Common::UString &getKeyTag() const;
	/** Get the lock DC for this situated object. */
	int getLockDC() const;
	int getLockRequiredSkill() const;

	// Trap state

	bool getIsTrapped() const;
	bool getTrapActive() const;
	bool getTrapDetectable() const;
	bool getTrapDisarmable() const;
	bool getTrapFlagged() const;
	bool getTrapOneShot() const;
	uint8_t getTrapBaseType() const;
	uint8_t getTrapDetectDC() const;
	uint8_t getTrapDisarmDC() const;
	const Common::UString &getTrapKeyTag() const;
	Object *getTrapDetectedBy() const;
	Object *getTrapCreator() const;

	void setTrapDetectedBy(Object *detector);
	void setTrapDisabled();
	void triggerTrap(Object *triggerer);

	Trap &getTrap();
	const Trap &getTrap() const;

	/** Lock/Unlock the situated object. */
	virtual void setLocked(bool locked);

	// Bashing

	/** The blueprint's "Hardness": soaked off every blow's damage. */
	int getHardness() const;
	/** The blueprint's "NotBlastable": can't be bashed or blown open. */
	bool isNotBlastable() const;
	/** Has the object been bashed open (broken)? */
	bool isBroken() const;
	/** A broken door or container counts as dead for the attacker. */
	bool isDead() const override;
	/** Can the party bash this (locked) object open? */
	bool canBeBashed() const;

	/** A blow of @p damage (before Hardness) by @p attacker landed. The OnDamaged
	 *  and OnDeath scripts run later, from the area (see consume*Event()). */
	void takeBashDamage(int damage, Object *attacker);
	/** Damage from an effect (EffectDamage): no Hardness, otherwise as a blow.
	 *  An object without hit points can't be harmed. */
	void takeDamage(int damage, Object *attacker);
	/** @p attacker swung at the object (its OnMeleeAttacked script runs later). */
	void notifyAttacked(Object *attacker);

	/** The last object that attacked or damaged this one (GetLastAttacker/Damager). */
	Object *getLastHostileActor() const;

	bool consumeAttackedEvent();
	bool consumeDamagedEvent();
	bool consumeDeathEvent();

	/** The object broke under the blows of @p breaker: open it for good. Runs
	 *  after its OnDeath script. */
	virtual void breakOpen(Object *breaker);

	// Positioning

	/** Set the situated object's position. */
	void setPosition(float x, float y, float z);
	/** Set the situated object's orientation. */
	void setOrientation(float x, float y, float z, float angle);

	// Animation

	void playAnimation(const Common::UString &anim,
	                   bool restart = true,
	                   float length = 0.0f,
	                   float speed = 1.0f);

	// Tooltip

	void getTooltipAnchor(float &x, float &y, float &z) const;

	void saveState(Aurora::GFF3WriterStruct &gff) const override;
	void loadState(const Aurora::GFF3Struct &gff) override;

protected:
	Common::UString _modelName; ///< The model's resource name.

	uint32_t _appearanceID; ///< The index within the situated appearance 2DA.
	uint32_t _soundAppType; ///< The index within the situated sounds 2DA.

	bool _locked; ///< Is the situated object locked?
	bool _keyRequired { false };
	Common::UString _keyTag;
	int  _lockDC { 20 };
	int  _lockSkill { kSkillSecurity };

	int  _hardness { 0 };          ///< Damage soaked off every blow.
	bool _notBlastable { false };  ///< Can't be bashed or blown open.
	bool _broken { false };        ///< Bashed open.
	bool _attackedEventPending { false };
	bool _damagedEventPending { false };
	bool _deathEventPending { false };
	Object *_lastHostileActor { nullptr };

	Common::UString _soundOpened;    ///< The sound the object makes when opened.
	Common::UString _soundClosed;    ///< The sound the object makes when closed.
	Common::UString _soundDestroyed; ///< The sound the object makes when destroyed.
	Common::UString _soundUsed;      ///< The sound the object makes when used.
	Common::UString _soundLocked;    ///< The sound the object makes when locked.

	Common::UString _conversation; ///< The optional conversation with this situated object.

	Object *_lastOpenedBy; ///< The object that last opened this situated object.
	Object *_lastClosedBy; ///< The object that last closed this situated object.
	Object *_lastUsedBy;   ///< The object that last used this situated object.

	Trap _trap; ///< The trap on the object, if any.

	std::unique_ptr<Graphics::Aurora::Model> _model; ///< The situated object's model.

	Situated(ObjectType type);

	/** Load the situated object from an instance and its blueprint. */
	void load(const Aurora::GFF3Struct &instance, const Aurora::GFF3Struct *blueprint = 0);

	/** Load object-specific properties. */
	virtual void loadObject(const Aurora::GFF3Struct &gff) = 0;
	/** Load appearance-specific properties. */
	virtual void loadAppearance() = 0;

private:
	void loadProperties(const Aurora::GFF3Struct &gff);
	void loadPortrait(const Aurora::GFF3Struct &gff);
	void loadSounds();
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_SITUATED_H
