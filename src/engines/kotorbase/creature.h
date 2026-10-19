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
 *  Creature within an area in KotOR games.
 */

#ifndef ENGINES_KOTORBASE_CREATURE_H
#define ENGINES_KOTORBASE_CREATURE_H

#include <set>
#include <vector>
#include <map>
#include <array>
#include <memory>

#include "src/common/types.h"
#include "src/common/ustring.h"

#include "src/aurora/types.h"

#include "src/graphics/aurora/types.h"

#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/path/pathfollow.h"
#include "src/engines/kotorbase/inventory.h"
#include "src/engines/kotorbase/action.h"
#include "src/engines/kotorbase/creatureinfo.h"
#include "src/engines/kotorbase/actionqueue.h"
#include "src/engines/kotorbase/creaturesounds.h"
#include "src/engines/kotorbase/visualeffects.h"

namespace Engines {

namespace KotORBase {

class CharacterGenerationInfo;
class Item;
class Area;
class Effect;
struct CreatureSearchCriteria;

/** What an activated combat feat does during its combat round. */
struct CombatFeatModifiers {
	bool melee  { false };    ///< Used with melee weapons (Flurry, Power Attack, Critical Strike).
	bool ranged { false };    ///< Used with ranged weapons (Rapid Shot, Power Blast, Sniper Shot).
	int  attack { 0 };        ///< Attack modifier on every attack of the round.
	int  damage { 0 };        ///< Damage bonus on every hit.
	int  armorClass { 0 };    ///< Defense modifier until the next round.
	int  threatRange { 0 };   ///< Widens the critical threat range (20 - threatRange .. 20).
	int  extraAttacks { 0 };  ///< Additional attacks this round.
};

/** The modifiers of combat feat @p feat (a FeatID), all zero for other feats. */
CombatFeatModifiers getCombatFeatModifiers(int feat);

/** Duration (s) of a combat feat's penalties: one combat round. */
static const float kCombatFeatPenaltyDuration = 3.0f;

class Creature : public Object {
public:
	/** Create a dummy creature instance. Not playable as it is.*/
	Creature();
	/** Load from a creature instance. */
	Creature(const Aurora::GFF3Struct &creature);
	/** Load from a creature template. */
	Creature(const Common::UString &resRef);
	~Creature();

	// Character generation

	/** Initialize this creature as a fake player character for testing purposes. */
	void initAsFakePC();
	/** Initialize this creature as a player character from a specified info class. */
	void initAsPC(const CharacterGenerationInfo &chargenInfo, const CreatureInfo &info);
	/** Replace stats/inventory/equipment from a saved CreatureInfo snapshot. */
	void applyCreatureInfo(const CreatureInfo &info);

	// Basic visuals

	/** Is the creature's model visible? */
	bool isVisible() const;

	/** Show the creature's model. */
	void show();
	/** Hide the creature's model. */
	void hide();

	// Basic properties

	/** Get the gender of the creature. */
	Gender getGender() const;
	/** Get the level of the creature regarding a specific class. */
	int getLevel(const Class &c) const;
	/** Get the race of the creature. */
	Race getRace() const;
	/** Get the subrace of the creature. */
	SubRace getSubRace() const;
	/** Get the level by its position in the level vector. */
	int getLevelByPosition(int position) const;
	/** Get the class by its position in the level vector. */
	Class getClassByPosition(int position) const;
	/** Get the movement rate of this creature when walking. */
	float getWalkRate() const;
	/** Get the movement rate of this creature when running. */
	float getRunRate() const;
	/** Get id of the conversation with this creature. */
	const Common::UString &getConversation() const;
	/** Get abstract information of this creature. */
	CreatureInfo &getCreatureInfo();
	const CreatureInfo &getCreatureInfo() const;
	/** Build a save snapshot including live HP and force points. */
	CreatureInfo buildSavedState() const;
	bool hasFeat(uint32_t feat) const;

	/** Is the creature a player character? */
	bool isPC() const;
	/** Is the creature a party member? */
	bool isPartyMember() const;

	/** Does this creature match a specified search criteria? */
	bool matchSearchCriteria(const Object *target, const CreatureSearchCriteria &criteria) const;

	// Interactive properties

	bool isCommandable() const;

	/** Toggle usability of this object. */
	void setUsable(bool usable);
	/** Set whether this creature's action queue can be modified. */
	void setCommandable(bool commandable);

	/** Lock body orientation during dialog (SetLockOrientationInDialog). */
	void setLockOrientationInDialog(bool locked);
	bool isLockOrientationInDialog() const;

	/** Lock head look-at during dialog (SetLockHeadFollowInDialog). */
	void setLockHeadFollowInDialog(bool locked);
	bool isLockHeadFollowInDialog() const;

	// Attributes

	/** Get the current rank of the specified skill, with skill effects and ability changes. */
	int getSkillRank(Skill skill) const;
	/** Get the current score of the specified ability, with equipment and effects. */
	int getAbilityScore(Ability ability) const;
	/** The modifier of the current ability score: floor((score - 10) / 2). */
	int getAbilityModifier(Ability ability) const;
	/** Ability score bonus of the equipped items' Ability Bonus properties. */
	int getEquipmentAbilityBonus(Ability ability) const;
	/** Get the current Force Points. */
	int getForcePoints() const;
	/** Get the maximum Force Points. */
	int getMaxForcePoints() const;

	void setForcePoints(int fp);
	void setMaxForcePoints(int fp);

	/** Get the alignment of this creature. */
	int getAlignment() const;
	/** Adjust the alignment of this creature. */
	void adjustAlignment(int shift);

	/** Roll a d20 vs the specified DC. Return true if successful. */
	bool rollSavingThrow(SavingThrow type, int dc);
	/** Get the total bonus for the specified saving throw, with effects. */
	int getSavingThrowBonus(SavingThrow type) const;

	// Combat AI
	/** Built-in fallback AI for scripted encounters. Force users are run by
	 *  their AI scripts (talents), not by an archetype. */
	enum AIArchetype {
		kAIArchetypeNone             = 0,
		kAIArchetypeBeastMelee       = 1,
		kAIArchetypeBeastPoison      = 2,
		kAIArchetypeTacticalHumanoid = 3
	};

	void setAIArchetype(AIArchetype archetype);
	void think(Area *area);
	void updateCombatAI();
	Object *findCombatTarget(Area *area);
	/** True if an ally of @p attacker threatens this creature from the opposite side. */
	bool isFlankedBy(Creature *attacker, Area *area = nullptr);
	/** Geometry helper used by flanking (dot product of defender→attacker and defender→ally < -0.5). */
	static bool areOnOppositeSides(float defX, float defY,
	                               float aX, float aY,
	                               float bX, float bY);
	virtual void update(float dt);

	// Positioning

	/** Get the camera height for this creature. */
	float getCameraHeight() const;

	/** Set the creature's position. */
	void setPosition(float x, float y, float z);
	/** Set the creature's orientation. */
	void setOrientation(float x, float y, float z, float angle);

	// Object/Cursor interactions

	const Common::UString &getCursor() const;

	/** (Un)Highlight the creature. */
	void highlight(bool enabled);
	/** The creature was clicked. */
	bool click(Object *triggerer = 0);

	// Inventory and equipment

	Inventory &getInventory();
	Item *getEquipedItem(InventorySlot slot) const;
	float getMaxAttackRange() const;

	void equipItem(Common::UString tag, InventorySlot slot, bool updateModel = true);
	void equipItem(Common::UString tag, InventorySlot slot, CreatureInfo &invOwner, bool updateModel = true);

	void equipMostDamagingMelee();
	void equipMostDamagingRanged();

	/** Create a live Item object for an inventory tag, store it and return it. */
	Item *addScriptItem(const Common::UString &tag);
	/** Find a live Item by tag in script items or equipment. Returns nullptr if not found. */
	Item *findInventoryItemByTag(const Common::UString &tag) const;

	/** A spell an item lets the creature cast. */
	struct ItemPower {
		int spellId { -1 };
		Common::UString item; ///< The item's template (inventory tag).
	};

	/** Force powers of the equipped items' CastSpell properties (armbands, belts...). */
	std::vector<ItemPower> getEquippedItemPowers() const;
	/** Spells of the usable items in the inventory (medpacs, stims, grenades...). */
	std::vector<ItemPower> getInventoryItemPowers() const;

	/** Return the total character level (sum of all class levels). */
	int getHitDice() const;

	/** The blueprint's "ChallengeRating", or the hit dice when it has none. */
	float getChallengeRating() const;

	/** Compute the maximum Force Points for this creature based on classes and modifiers. */
	int computeMaxForcePoints() const;

	/** Compute the Base Attack Bonus from class/level. */
	int getBAB() const;

	/** Compute the full armor class (10 + Dex modifier + equipped armour bonus + runtime effects). */
	int getAC() const;

	// Animation

	void playDefaultAnimation();
	void playDefaultHeadAnimation();
	void playDrawWeaponAnimation();
	void playAttackAnimation();
	void playDodgeAnimation();

	void playAnimation(const Common::UString &anim,
	                   bool restart = true,
	                   float length = 0.0f,
	                   float speed = 1.0f);

	void playHeadAnimation(const Common::UString &anim,
	                       bool restart = true,
	                       float length = 0.0f,
	                       float speed = 1.0f);

	// Sounds and visual effects

	/** Once a frame, for the dead as well: play the sounds of the animation
	 *  events the model passed (footsteps, weapon swings and hits), delayed
	 *  sounds, and advance the visual effects. */
	void updatePresentation(float dt, Area *area);
	/** Bark a soundset entry where the creature stands. Unless @p interrupt,
	 *  nothing plays while an earlier bark still does. */
	bool playSoundSetBark(SoundSetEntry entry, bool interrupt = false);
	/** Bark one of @p count soundset entries from @p first on, at random. */
	bool playRandomSoundSetBark(SoundSetEntry first, int count, bool interrupt = false);
	/** Play visualeffects.2da effect @p id on the creature. With @p lasting,
	 *  a duration effect plays until stopVisualEffect(). */
	void playVisualEffect(int id, bool lasting = false);
	/** Stop a lasting visual effect. */
	void stopVisualEffect(int id);
	/** Show the conjuring and casting visuals of spells.2da row @p spell. */
	void playSpellCastVisuals(int spell);

	// Actions

	/** Get the current action that this creature is executing. */
	const Action *getCurrentAction() const;

	/** Clear all actions of this creature. */
	void clearActions();
	/** Add action to the action queue of this creature. */
	void addAction(const Action &action);
	/** Remove the current action from the action queue of this creature. */
	void popAction();
	size_t getActionCount() const { return _actions.size(); }

	struct ActiveEffect {
		EffectType type;
		float duration;
		int   value;
		int   spellId { -1 };
		float nextTick { 1.0f };
		bool  expired { false };
		/** kEffectModifier: the KotOREffectType, and its subtype (ability, skill,
		 *  saving throw or damage type) and limit (damage absorbed before it ends). */
		int   kotorType { -1 };
		int   subType { 0 };
		int   limit { 0 };
		/** Effects linked by EffectLinkEffects share a non-zero id and are removed together. */
		int   linkId { 0 };
		bool  permanent { false };
	};

	void applyEffect(EffectType type, float duration, int value = 0, int spellId = -1);
	void updateEffects(float dt);
	bool hasEffect(EffectType type) const;
	bool hasSpellEffect(int spellId) const;
	/** Stunned, paralysed, asleep or knocked down: no actions, no attacks. */
	bool isIncapacitated() const;
	const std::vector<ActiveEffect> &getActiveEffects() const { return _effects; }
	void clearActiveEffects();
	/** Remove the active effect at @p index and every effect linked to it. */
	bool removeActiveEffectAt(size_t index);

	/** Reduce @p damage of @p damageType by damage resistance, damage reduction and
	 *  Force shields, using up their limits. Returns the damage that gets through. */
	int absorbDamage(int damage, int damageType);
	/** Is the creature invisible (EffectInvisibility)? */
	bool isInvisible() const { return _invisibility > 0; }
	/** Damage bonus (or penalty) of EffectDamageIncrease/Decrease. */
	int getDamageModifier() const { return _damageModifier; }
	/** Force push: knock the creature back by up to @p distance metres away from
	 *  (fromX, fromY), stopping where the walkmesh ends. Returns the distance moved. */
	float knockBack(Area &area, float fromX, float fromY, float distance);

	void performCutsceneAttack(Object *target, int flags, int damage = 0);

	// Tooltip

	void getTooltipAnchor(float &x, float &y, float &z) const;

	// Perception

	void updatePerception(Creature &object);

	// Combat

	bool isInCombat() const;
	Object *getAttackTarget() const;
	int getAttackRound() const;
	Object *getAttemptedAttackTarget() const;
	Object *getLastHostileActor() const;
	void setLastHostileActor(Object *actor) { _lastHostileActor = actor; }
	int getLastCombatFeatUsed() const;
	int getLastForcePowerUsed() const;
	void setLastForcePowerUsed(int spellID);
	int getQueuedCombatFeat() const;

	/** Record the last killer (set on death). */
	Object *getLastKiller() const;
	void setLastKiller(Object *killer);

	/** Last damage event queries (GetDamageDealtByType / GetTotalDamageDealt). */
	void recordDamageTaken(int amount, int damageType, Object *damager = nullptr);
	int getDamageDealtByType(int damageType) const;
	int getTotalDamageDealt() const;

	/** Last attack result / weapon used (queried on the attacker). */
	void recordAttackResult(int result, Object *weapon = nullptr);
	int getLastAttackResult() const;
	Object *getLastWeaponUsed() const;

	/** Immunity tracking for EffectImmunity / GetIsImmune. */
	void addImmunity(int immunityType);
	void addSpellImmunity(int spellId);
	bool isImmune(int immunityType) const;
	bool isImmuneToSpell(int spellId) const;
	void adjustDamageImmunity(int damageType, int percentDelta);
	int getDamageImmunityPercent(int damageType) const;
	void clearImmunities();

	/** Cancel combat, clear actions, and optionally clear buffs; set surrender faction. */
	void surrenderToEnemies(bool retainBuffs = false);

	/** Make every creature in @p creatures that attacks @p target stop doing so.
	 *  With @p clearQueuedActions, their action queues are cleared as well
	 *  (surrender); otherwise only actions targeting @p target are dropped. */
	static void stopAttacksOn(const std::vector<Creature *> &creatures, const Object *target,
	                          bool clearQueuedActions);

	/** Regenerate a fraction of the maximum Force points (out of combat). */
	void regenerateForcePoints();
	/** Full rest: restore hit points and Force points to their maximum. */
	void restoreFully();

	int getAILevel() const { return _aiLevel; }
	void setAILevel(int level) { _aiLevel = level; }

	int getConcealment() const { return _concealment; }
	void setConcealment(int percent) { _concealment = percent; }

	bool hasAssuredHit() const { return _assuredHit; }
	void setAssuredHit(bool enabled) { _assuredHit = enabled; }

	int getBlasterDeflectionBonus() const { return _blasterDeflectionBonus; }
	void adjustBlasterDeflection(int delta) { _blasterDeflectionBonus += delta; }

	int getForceResistance() const { return _forceResistance; }
	void adjustForceResistance(int delta) { _forceResistance += delta; }
	void setForceResistance(int value) { _forceResistance = value; }

	int getDamageResistance(int damageType) const;
	void setDamageResistance(int damageType, int amount);

	// Perception results (set by handleObjectSeen / handleObjectVanished).
	Object *getLastPerceived() const;
	bool getLastPerceptionSeen() const;
	bool getLastPerceptionVanished() const;
	bool getLastPerceptionHeard() const;
	bool getLastPerceptionInaudible() const;
	bool hasSeenObject(const Object *obj) const;
	bool hasHeardObject(const Object *obj) const;

	void setAttemptedAttackTarget(Object *target);
	void setLastCombatFeatUsed(int featID);
	void queueCombatFeat(int featID);
	int consumeQueuedCombatFeat();

	void adjustAttackModifier(int amount);
	void adjustArmorClassModifier(int amount);
	void adjustSkillModifier(Skill skill, int amount);
	int getAttackModifier() const;
	int getArmorClassModifier() const;
	int getSkillModifier(Skill skill) const;

	void startCombat(Object *target, int round);
	void cancelCombat();
	/** Drop every reference to an object that is about to be destroyed. */
	void forgetObject(const Object *object);

	/** Time (s) the current movement has been blocked; reset on progress. */
	float addBlockedMoveTime(float dt) { return _blockedMoveTime += dt; }
	void resetBlockedMoveTime() { _blockedMoveTime = 0.0f; }
	/** Play animations from this model before the creature's own (dialog stunts). */
	void setStuntModel(Graphics::Aurora::Model *stunt);
	/** Does the creature's model (or its stunt model) have this animation? */
	bool hasModelAnimation(const Common::UString &anim);

	/** The walkmesh route the current movement follows. */
	MovePath &getMovePath() { return _movePath; }

	/** Apply an active effect tracking struct directly to this creature. */
	void applyEffect(const ActiveEffect &effect);

	/** Reverse the persistent modifiers an active effect applied (called on expiry). */
	void removeEffect(const ActiveEffect &effect);

	/** Apply an engine/NWScript level Effect object to this creature.
	 *  If durationOverride >= 0, timed effects use that duration instead of defaults. */
	void applyEffect(const Effect &effect, float durationOverride = -1.0f);

	/**
	 * Execute one attack iteration against target.
	 *
	 * @param target     The object being attacked.
	 * @param babPenalty Iterative-attack penalty applied to this swing (0 for the
	 *                   primary attack, -5 for the second, -10 for the third…).
	 * @param damageMod  Additional flat damage modifier (e.g. from caller-selected feats).
	 * @param activeFeat Activated combat feat for this attack sequence, or -1.
	 * @param area       Optional area used for flanking / LOS context.
	 */
	void executeAttack(Object *target, int babPenalty = 0, int damageMod = 0, int activeFeat = -1, Area *area = nullptr);
	
	/** Returns true if the creature has a lightsaber equipped in either hand. */
	bool hasLightsaberEquipped() const;
	void setLightsaberPowered(bool powered);
	bool isLightsaberPowered() const;

	void setBlockingDoor(Object *door);
	Object *getBlockingDoor() const;

	/** Return true if the creature has the specified Force power. */
	bool hasForcePower(uint32_t power) const;

	// Appearance and Stealth (Tatooine parity)
	uint32_t getAppearanceType() const { return _appearance; }
	void setAppearanceType(uint32_t appearance);

	bool getStealthMode() const { return _stealthMode; }
	void setStealthMode(bool stealth);

	// Death

	bool isDead() const override;

	/** Handle creature's death. Return true if changed from alive to dead. */
	bool handleDeath();

	// Area persistence: alive/dead, position and facing on top of Object's state.
	void saveState(Aurora::GFF3WriterStruct &gff) const override;
	void loadState(const Aurora::GFF3Struct &gff) override;
	/** Bring a dead creature back with the given hit points (at least 1). */
	void resurrect(int hitPoints);
	/** Return and clear whether OnDeath is due (set when the creature died). */
	bool consumeDeathEvent();
	/** Return and clear whether OnDamaged is due (set when damage was taken). */
	bool consumeDamagedEvent();
	/** Queue OnSpawn (when the creature enters an area) / OnHeartbeat. */
	void queueSpawnEvent() { _spawnEventPending = true; }
	/** A roster companion placed by SpawnAvailableNPC; never saved with the area. */
	void setRosterSpawn(bool rosterSpawn) { _rosterSpawn = rosterSpawn; }
	bool isRosterSpawn() const { return _rosterSpawn; }
	void queueHeartbeatEvent() { _heartbeatEventPending = true; }
	bool consumeSpawnEvent();
	bool consumeHeartbeatEvent();
	/** Queue OnEndCombatRound (creature was in combat when the round ended). */
	void queueEndRoundEvent() { _endRoundEventPending = true; }
	bool consumeEndRoundEvent();
	/** Return and clear whether OnAttacked is due; @p attacker receives the attacker. */
	bool consumeAttackedEvent(Object *&attacker);

	/** A change in what this creature perceives, queued for OnPerception. */
	struct PerceptionEvent {
		Object *object { nullptr };
		bool seen { false };   ///< true: object became visible; false: it vanished.
	};
	/** Pop the oldest pending perception event and make it the "last perceived" state. */
	bool consumePerceptionEvent(PerceptionEvent &event);

	/** Has kill XP for this creature already been awarded? */
	bool isXPAwarded() const;
	/** Mark kill XP as awarded. */
	void markXPAwarded();

	// Additional properties for Taris/Dantooine parity
	bool isResting() const { return _isResting; }
	void setResting(bool resting) { _isResting = resting; }

	int getInfluence() const { return _influence; }
	void setInfluence(int influence) { _influence = influence; }

	int getSkillPointBonus() const { return _skillPointBonus; }
	void setSkillPointBonus(int bonus) { _skillPointBonus = bonus; }

protected:
	// Parts of a creature's body.
	struct PartModels {
		Common::UString type;

		Common::UString body;
		Common::UString head;

		Common::UString bodyTexture;
		Common::UString portrait;
	};

	Gender _gender;
	Skin _skin; ///< The skin type of the creature.
	uint8_t _face; ///< The face of the creature.

	virtual void getPartModelsPC(PartModels &parts, uint32_t state, uint8_t textureVariation) = 0;
	void loadMovementRate(const Common::UString &name);

private:
	// Perception

	std::set<Object *> _seenObjects;
	std::set<Object *> _heardObjects;

	// Combat

	bool _inCombat { false };
	Object *_attackTarget { nullptr };
	int _attackRound { 0 };
	float _challengeRating { -1.0f }; ///< From the blueprint; < 0 when unset.
	Object *_attemptedAttackTarget { nullptr };
	Object *_lastHostileActor { nullptr };
	Object *_lastKiller { nullptr };
	Object *_lastWeaponUsed { nullptr };
	int _lastCombatFeatUsed { -1 };
	int _lastForcePowerUsed { -1 };
	int _queuedCombatFeat { -1 };
	int _lastDamageTotal { 0 };
	int _lastDamageType { 0 };
	int _lastAttackResult { 0 };
	std::set<int> _immunities;
	std::set<int> _spellImmunities;
	std::map<int, int> _damageImmunityPercent;
	std::map<int, int> _damageResistance;
	int _aiLevel { 0 };
	int _concealment { 0 };
	bool _assuredHit { false };
	int _blasterDeflectionBonus { 0 };
	int _forceResistance { 0 };
	int _attackModifier { 0 };
	int _armorClassModifier { 0 };
	int _damageModifier { 0 };
	int _invisibility { 0 };
	int _nextEffectLinkId { 0 };
	std::array<int, kSkillMAX> _skillModifiers {{}};
	std::array<int, kAbilityCharisma + 1> _abilityModifiers {{}};
	/** Saving throw effects, indexed by SAVING_THROW_* (0 all, 1 Fortitude, 2 Reflex, 3 Will). */
	std::array<int, 4> _savingThrowModifiers {{}};

	// Perception state — updated by handleObjectSeen/Vanished/Heard/Inaudible.
	Object *_lastPerceived { nullptr };
	bool _lastPerceptionSeen       { false };
	bool _lastPerceptionVanished   { false };
	bool _lastPerceptionHeard      { false };
	bool _lastPerceptionInaudible  { false };


	bool _isPC; ///< Is the creature a PC?

	uint32_t _appearance; ///< The creature's general appearance.

	Race _race; ///< The race of the creature.
	SubRace _subRace; ///< The subrace of the creature.

	Common::UString _modelType;
	std::unique_ptr<Graphics::Aurora::Model> _model; ///< The creature's model.
	Graphics::Aurora::Model *_headModel; ///< The creature's head model.
	bool _visible;
	bool _commandable;
	bool _lockOrientationInDialog { false };
	bool _lockHeadFollowInDialog { false };

	CreatureInfo _info;

	Common::UString _conversation;

	std::map<InventorySlot, std::unique_ptr<Item>> _equipment;

	/** Live Item objects created via scripts for inventory items. */
	std::vector<std::unique_ptr<Item>> _scriptItems;

	ActionQueue _actions;
	float _blockedMoveTime { 0.0f };
	MovePath _movePath;
	bool _deathEventPending { false };   ///< OnDeath queued; run outside creature iteration.
	bool _damagedEventPending { false }; ///< OnDamaged queued; run outside creature iteration.
	bool _spawnEventPending { false };     ///< OnSpawn queued; run outside creature iteration.
	bool _rosterSpawn { false };           ///< Placed by SpawnAvailableNPC.
	bool _heartbeatEventPending { false }; ///< OnHeartbeat queued; run outside creature iteration.
	bool _endRoundEventPending { false };  ///< OnEndCombatRound queued.
	bool _attackedEventPending { false };  ///< OnAttacked queued.
	Object *_lastAttacker { nullptr };     ///< Attacker for the pending OnAttacked.
	std::vector<PerceptionEvent> _perceptionEvents; ///< OnPerception queue.

	float _walkRate;
	float _runRate;

	bool _dead { false };
	bool _lightsaberPowered { true };
	Object *_blockingDoor { nullptr };
	std::vector<ActiveEffect> _effects;
	AIArchetype _aiArchetype { kAIArchetypeNone };
	float       _aiCooldown  { 0.0f };
	bool _xpAwarded { false }; ///< Has kill XP for this creature already been awarded?


	// Perception

	void handleObjectSeen(Object &object);
	void handleObjectVanished(Object &object);
	void handleObjectHeard(Object &object);
	void handleObjectInaudible(Object &object);


	void init();

	void load(const Aurora::GFF3Struct &creature);
	void load(const Aurora::GFF3Struct &instance, const Aurora::GFF3Struct *blueprint);

	void loadProperties(const Aurora::GFF3Struct &gff, bool clearScripts = true);
	void loadPortrait(const Aurora::GFF3Struct &gff);
	void loadEquipment(const Aurora::GFF3Struct &gff);
	void loadAbilities(const Aurora::GFF3Struct &gff);

	void getModelState(uint32_t &state, uint8_t &textureVariation);
	void getPartModels(PartModels &parts, uint32_t state, uint8_t textureVariation);
	void loadBody(PartModels &parts);
	void loadHead(PartModels &parts);

	void loadEquippedModel();
	void attachWeaponModel(InventorySlot slot);

	void setDefaultAnimations();

	void reloadEquipment();
	bool addItemToEquipment(const Common::UString &tag, InventorySlot slot);

	int getWeaponAnimationNumber() const;

	/** Apply one part of an Effect (linked effects are applied part by part). */
	void applyEffectPart(const Effect &effect, float durationOverride, int linkId);
	/** Store and apply an active effect. */
	void addActiveEffect(const ActiveEffect &effect);
	/** Add (@p sign 1) or take away (@p sign -1) the modifiers of a kEffectModifier effect. */
	void adjustEffectModifiers(const ActiveEffect &effect, int sign);
	int computeWeaponDamage(const Item *weapon) const;

	// Sounds and visual effects

	/** A sound to play after a delay (a power's casting sound). */
	struct DelayedSound {
		float delay { 0.0f };
		Common::UString sound;
	};

	/** A duration visual effect applied with a duration (EffectVisualEffect). */
	struct LastingVisual {
		int id { -1 };            ///< visualeffects.2da row.
		int linkId { 0 };         ///< The effects it's linked to, 0 if none.
		float timeLeft { 0.0f };
		bool linkSeen { false };  ///< Have the linked effects been active?
	};

	CreatureVisualEffects _visualEffects;
	std::vector<LastingVisual> _lastingVisuals;
	Sound::ChannelHandle _barkSound;
	std::vector<DelayedSound> _delayedSounds;
	int _footstepType { -1 };       ///< footstepsounds.2da row.
	int _sizeCategory { 3 };        ///< appearance.2da "sizecategory".
	/** The appearance _footstepType and _sizeCategory were read for. */
	uint32_t _appearanceSoundsFor { 0xFFFFFFFF };

	/** End the lasting visual effects whose time or linked effects ran out. */
	void updateLastingVisuals(float dt);
	/** End all lasting visual effects. */
	void stopLastingVisuals();
	/** Read the footstep type and size category from appearance.2da. */
	void loadAppearanceSounds();
	/** Play the sound of an animation event. */
	void handleAnimationEvent(const Common::UString &event, Area *area);
	/** The weapon whose sounds the creature's attacks make. */
	const Item *getSoundWeapon() const;
	/** Play a sound where the creature stands. */
	void playSoundHere(const Common::UString &sound, float minDistance, float maxDistance);
	/** Show a spells.2da casting visual: a model, or a visualeffects.2da row when numeric. */
	void playSpellVisual(const Common::UString &visual, VisualEffectHook hook, float delay);

	int _influence { 50 };
	int _skillPointBonus { 0 };
	bool _isResting { false };
	bool _stealthMode { false };
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_CREATURE_H
