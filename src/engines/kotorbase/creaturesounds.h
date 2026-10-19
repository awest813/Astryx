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
 *  Sounds of creature animation events (footsteps, weapon swings and hits)
 *  and soundset barks, for KotOR games.
 */

#ifndef ENGINES_KOTORBASE_CREATURESOUNDS_H
#define ENGINES_KOTORBASE_CREATURESOUNDS_H

#include "src/common/types.h"
#include "src/common/ustring.h"

#include "src/sound/types.h"

namespace Aurora {
	class TwoDAFile;
}

namespace Engines {

namespace KotORBase {

/** Entries of a KotOR soundset (SSF), soundset.2da's RESREF. */
enum SoundSetEntry {
	kSoundSetBattleCry1     =  0,
	kSoundSetBattleCry2     =  1,
	kSoundSetBattleCry3     =  2,
	kSoundSetBattleCry4     =  3,
	kSoundSetBattleCry5     =  4,
	kSoundSetBattleCry6     =  5,
	kSoundSetSelect1        =  6,
	kSoundSetSelect2        =  7,
	kSoundSetSelect3        =  8,
	kSoundSetAttack1        =  9,
	kSoundSetAttack2        = 10,
	kSoundSetAttack3        = 11,
	kSoundSetPain1          = 12,
	kSoundSetPain2          = 13,
	kSoundSetLowHealth      = 14,
	kSoundSetDead           = 15,
	kSoundSetCriticalHit    = 16,
	kSoundSetTargetImmune   = 17,
	kSoundSetLayMine        = 18,
	kSoundSetDisarmMine     = 19,
	kSoundSetStealth        = 20,
	kSoundSetSearch         = 21,
	kSoundSetUnlock         = 22,
	kSoundSetUnlockFail     = 23,
	kSoundSetUnlockSuccess  = 24,
	kSoundSetSoloMode       = 25,
	kSoundSetPartyMode      = 26,
	kSoundSetPoisoned       = 27
};

/** The sound a creature animation event makes. */
enum AnimationEventSound {
	kAnimationEventSoundNone,
	kAnimationEventSoundFootstep,   ///< "snd_footstep": footstepsounds.2da.
	kAnimationEventSoundSwingShort, ///< "Swingshort": weaponsounds.2da swingshort0-2.
	kAnimationEventSoundSwingLong,  ///< "Swinglong": weaponsounds.2da swinglong0-2.
	kAnimationEventSoundSwingTwirl, ///< "Swingtwirl": weaponsounds.2da swingtwirl0-1.
	kAnimationEventSoundParry,      ///< "HitParry": weaponsounds.2da parry0-1.
	kAnimationEventSoundClash,      ///< "Clash", "Contact": weaponsounds.2da clash0-1.
	kAnimationEventSoundHit         ///< "Hit": weaponsounds.2da, by what was hit.
};

/** What a weapon hit, choosing the weaponsounds.2da hit columns. */
enum WeaponHitMaterial {
	kWeaponHitCloth,
	kWeaponHitLeather,
	kWeaponHitArmor,
	kWeaponHitForceField,
	kWeaponHitMetal,
	kWeaponHitWood,
	kWeaponHitStone
};

/** The sound an animation event (as named in the model) makes, if any. */
AnimationEventSound getAnimationEventSound(const Common::UString &event);

/** How many variants (numbered columns) a weaponsounds.2da sound has. */
int getWeaponSoundVariantCount(AnimationEventSound sound);

/** The footstepsounds.2da column prefix ("dirt", "stone", ...) for walking on
 *  a walkmesh surface (a surfacemat.2da row). Unknown surfaces sound like dirt. */
const char *getFootstepSurfaceColumn(uint32_t surface);

/** The footstep sound of footstepsounds.2da row @p footstepType (appearance.2da's
 *  "footsteptype") on @p surface. @p variant (0-2) picks one of the surface's
 *  sounds; when that one is empty, the first one is used, then the "rolling"
 *  sound (droids that roll instead of walking). Empty when there's none. */
Common::UString getFootstepSound(const Aurora::TwoDAFile &footsteps, int footstepType,
                                 uint32_t surface, int variant);

/** The weaponsounds.2da row of a base item (a baseitems.2da row), or -1. */
int getWeaponSoundRow(const Aurora::TwoDAFile &baseItems, int baseItem);

/** The weaponsounds.2da sound of an animation event, for a weapon of
 *  weaponsounds.2da row @p row. @p material matters for hits only. A variant
 *  that's out of range or empty falls back to the first one. Empty when
 *  there's none. */
Common::UString getWeaponSound(const Aurora::TwoDAFile &weaponSounds, int row,
                               AnimationEventSound sound, WeaponHitMaterial material, int variant);

/** Play a sound once at a position in the world.
 *
 *  @param minDistance Up to this distance, the sound plays at full volume.
 *  @param maxDistance Beyond this distance, the sound doesn't get any quieter.
 */
Sound::ChannelHandle playSoundAt(const Common::UString &sound, float x, float y, float z,
                                 Sound::SoundType type, float minDistance, float maxDistance,
                                 bool pitchVariance = true);

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_CREATURESOUNDS_H
