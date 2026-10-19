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

#include "src/common/string.h"
#include "src/common/error.h"
#include "src/common/debug.h"
#include "src/common/random.h"
#include "src/common/readstream.h"

#include "src/aurora/2dafile.h"
#include "src/aurora/resman.h"

#include "src/sound/sound.h"

#include "src/engines/kotorbase/creaturesounds.h"

namespace Engines {

namespace KotORBase {

AnimationEventSound getAnimationEventSound(const Common::UString &event) {
	static const struct {
		const char *name;
		AnimationEventSound sound;
	} kEventSounds[] = {
		{ "snd_footstep", kAnimationEventSoundFootstep   },
		{ "swingshort"  , kAnimationEventSoundSwingShort },
		{ "swinglong"   , kAnimationEventSoundSwingLong  },
		{ "swingtwirl"  , kAnimationEventSoundSwingTwirl },
		{ "hitparry"    , kAnimationEventSoundParry      },
		{ "clash"       , kAnimationEventSoundClash      },
		{ "contact"     , kAnimationEventSoundClash      },
		{ "hit"         , kAnimationEventSoundHit        }
	};

	for (const auto &eventSound : kEventSounds)
		if (event.equalsIgnoreCase(eventSound.name))
			return eventSound.sound;

	return kAnimationEventSoundNone;
}

int getWeaponSoundVariantCount(AnimationEventSound sound) {
	switch (sound) {
		case kAnimationEventSoundSwingShort:
		case kAnimationEventSoundSwingLong:
			return 3;

		case kAnimationEventSoundSwingTwirl:
		case kAnimationEventSoundParry:
		case kAnimationEventSoundClash:
		case kAnimationEventSoundHit:
			return 2;

		default:
			break;
	}

	return 0;
}

const char *getFootstepSurfaceColumn(uint32_t surface) {
	// surfacemat.2da rows
	switch (surface) {
		case  3: return "grass";
		case  4: return "stone";
		case  5: return "wood";
		case  6: return "water";
		case  9: return "carpet";
		case 10: return "metal";
		case 11:          // Puddles
		case 13: return "puddles"; // Mud
		case 14: return "leaves";
		default:
			break;
	}

	return "dirt";
}

Common::UString getFootstepSound(const Aurora::TwoDAFile &footsteps, int footstepType,
                                 uint32_t surface, int variant) {

	if ((footstepType < 0) || (static_cast<size_t>(footstepType) >= footsteps.getRowCount()))
		return "";

	const Aurora::TwoDARow &row = footsteps.getRow(footstepType);
	const Common::UString surfaceColumn = getFootstepSurfaceColumn(surface);

	if ((variant >= 0) && (variant < 3)) {
		const Common::UString &sound = row.getString(Common::String::format("%s%d", surfaceColumn.c_str(), variant));
		if (!sound.empty())
			return sound;
	}

	const Common::UString &first = row.getString(surfaceColumn + "0");
	if (!first.empty())
		return first;

	return row.getString("rolling");
}

int getWeaponSoundRow(const Aurora::TwoDAFile &baseItems, int baseItem) {
	if ((baseItem < 0) || (static_cast<size_t>(baseItem) >= baseItems.getRowCount()))
		return -1;

	const Aurora::TwoDARow &row = baseItems.getRow(baseItem);
	if (!row.empty("weaponmattype"))
		return row.getInt("weaponmattype");

	if (!row.empty("powereditem"))
		return row.getInt("powereditem");

	return -1;
}

static const char *getWeaponSoundColumn(AnimationEventSound sound, WeaponHitMaterial material) {
	switch (sound) {
		case kAnimationEventSoundSwingShort:
			return "swingshort";
		case kAnimationEventSoundSwingLong:
			return "swinglong";
		case kAnimationEventSoundSwingTwirl:
			return "swingtwirl";
		case kAnimationEventSoundParry:
			return "parry";
		case kAnimationEventSoundClash:
			return "clash";

		case kAnimationEventSoundHit:
			switch (material) {
				case kWeaponHitCloth:
					return "cloth";
				case kWeaponHitArmor:
					return "armor";
				case kWeaponHitForceField:
					return "forcefield";
				case kWeaponHitMetal:
					return "metal";
				case kWeaponHitWood:
					return "wood";
				case kWeaponHitStone:
					return "stone";
				case kWeaponHitLeather:
				default:
					break;
			}
			return "leather";

		default:
			break;
	}

	return nullptr;
}

Common::UString getWeaponSound(const Aurora::TwoDAFile &weaponSounds, int row,
                               AnimationEventSound sound, WeaponHitMaterial material, int variant) {

	if ((row < 0) || (static_cast<size_t>(row) >= weaponSounds.getRowCount()))
		return "";

	const char *column = getWeaponSoundColumn(sound, material);
	if (!column)
		return "";

	const Aurora::TwoDARow &soundRow = weaponSounds.getRow(row);

	if ((variant > 0) && (variant < getWeaponSoundVariantCount(sound))) {
		const Common::UString &variantSound = soundRow.getString(Common::String::format("%s%d", column, variant));
		if (!variantSound.empty())
			return variantSound;
	}

	return soundRow.getString(Common::String::format("%s0", column));
}

Sound::ChannelHandle playSoundAt(const Common::UString &sound, float x, float y, float z,
                                 Sound::SoundType type, float minDistance, float maxDistance,
                                 bool pitchVariance) {

	Sound::ChannelHandle channel;
	if (sound.empty())
		return channel;

	try {
		Common::SeekableReadStream *soundStream = ResMan.getResource(Aurora::kResourceSound, sound);
		if (!soundStream) {
			debugC(Common::kDebugEngineSound, 2, "No sound \"%s\"", sound.c_str());
			return channel;
		}

		channel = SoundMan.playSoundFile(soundStream, type, false);

		SoundMan.setChannelRelative(channel, false);
		SoundMan.setChannelPosition(channel, x, y, z);
		SoundMan.setChannelDistance(channel, minDistance, maxDistance);

		if (pitchVariance)
			SoundMan.setChannelPitch(channel, 0.95f + RNG.getNext(0, 101) / 1000.0f);

		SoundMan.startChannel(channel);

		debugC(Common::kDebugEngineSound, 2, "Playing sound \"%s\" at %.2f, %.2f, %.2f",
		       sound.c_str(), x, y, z);

	} catch (...) {
		Common::exceptionDispatcherWarning();
	}

	return channel;
}

} // End of namespace KotORBase

} // End of namespace Engines
