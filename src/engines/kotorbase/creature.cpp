/* xoreos - A reimplementation of BioWare's Aurora engine * * xoreos is the legal property of its developers, whose names * can be found in the AUTHORS file distributed with this source * distribution. * * xoreos is free software;
you can redistribute it and/or * modify it under the terms of the GNU General Public License * as published by the Free Software Foundation;
either version 3 * of the License, or (at your option) any later version. * * xoreos is distributed in the hope that it will be useful, * but WITHOUT ANY WARRANTY;
without even the implied warranty of * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the * GNU General Public License for more details. * * You should have received a copy of the GNU General Public License * along with xoreos. If not, see <http://www.gnu.org/licenses/>. */
/** @file *  Creature within an area in KotOR games. */
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include "external/glm/gtc/type_ptr.hpp"
#include "src/common/util.h"
#include "src/common/maths.h"
#include "src/common/error.h"
#include "src/common/ustring.h"
#include "src/common/strutil.h"
#include "src/common/debug.h"
#include "src/common/random.h"
#include "src/aurora/2dafile.h"
#include "src/aurora/2dareg.h"
#include "src/aurora/gff3file.h"
#include "src/aurora/gff3writer.h"
#include "src/aurora/locstring.h"
#include "src/aurora/resman.h"
#include "src/aurora/talkman.h"
#include "src/aurora/ssffile.h"
#include "src/graphics/aurora/modelnode.h"
#include "src/graphics/aurora/model.h"
#include "src/graphics/aurora/animationchannel.h"
#include "src/engines/aurora/util.h"
#include "src/engines/aurora/model.h"
#include "src/sound/sound.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/effect.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/situated.h"
#include "src/engines/kotorbase/perception.h"
#include "src/engines/kotorbase/reputation.h"
#include "src/engines/kotorbase/creaturesearch.h"
#include "src/engines/kotorbase/gui/chargeninfo.h"
#include "src/engines/kotorbase/levelup.h"
#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/animationnames.h"
#include "src/engines/kotorbase/creaturesounds.h"
#include "src/engines/kotorbase/visualeffects.h"
namespace Engines {
namespace KotORBase {

/** animations.2da row of the fall a Force push throws a creature into (as KotOR.js plays it). */
static const int kAnimationForcePushed = 84;

		Creature::Creature(
		const Common::UString &resRef) :		Object(kObjectTypeCreature),		_commandable(true),		_walkRate(0.0f),		_runRate(0.0f) {
			init();
			std::unique_ptr<Aurora::GFF3File> utc(loadOptionalGFF3(resRef, Aurora::kFileTypeUTC));
			if (!utc)		throw Common::Exception("Creature \"%s\" has no blueprint", resRef.c_str());
			load(utc->getTopLevel());
		}
		Creature::Creature(
		const Aurora::GFF3Struct &creature) :		Object(kObjectTypeCreature),		_commandable(true),		_walkRate(0.0f),		_runRate(0.0f) {
			init();
			load(creature);
		}
		Creature::Creature() :		Object(kObjectTypeCreature),		_commandable(true),		_walkRate(0.0f),		_runRate(0.0f) {
			init();
		}
		Creature::~Creature() {
		}
		void Creature::init() {
			_isPC = false;
			_appearance = Aurora::kFieldIDInvalid;
			_gender = kGenderNone;
			_race = kRaceUnknown;
			_subRace = kSubRaceNone;
			_skin = kSkinMAX;
			_face = 0;
			_headModel = 0;
			_visible = false;
		}
		void Creature::show() {
			if (_visible)		return;
			_visible = true;
			if (_model)		_model->show();
		}
		void Creature::hide() {
			if (!_visible)		return;
			if (_model)		_model->hide();
			_visible = false;
		}
		bool Creature::isVisible()
		const {
			return _visible;
		}
		bool Creature::isPC()
		const {
			return _isPC;
		}
		bool Creature::isPartyMember()
		const {
			return _isPC;
		}
		bool Creature::matchSearchCriteria(
		const Object *UNUSED(target),
		const CreatureSearchCriteria &UNUSED(criteria))
		const {
			// TODO: Implement pattern matching
			return false;
		}
		bool Creature::isCommandable()
		const {
			return _commandable;
		}
		void Creature::setUsable(
		bool usable) {
			Object::setUsable(usable);
			if (_model)		_model->setClickable(isClickable());
		}
		void Creature::setCommandable(
		bool commandable) {
			_commandable = commandable;
		}
		void Creature::setLockOrientationInDialog(bool locked) {
			_lockOrientationInDialog = locked;
		}
		bool Creature::isLockOrientationInDialog() const {
			return _lockOrientationInDialog;
		}
		void Creature::setLockHeadFollowInDialog(bool locked) {
			_lockHeadFollowInDialog = locked;
		}
		bool Creature::isLockHeadFollowInDialog() const {
			return _lockHeadFollowInDialog;
		}
		Gender Creature::getGender()
		const {
			return _gender;
		}
		int Creature::getLevel(
		const Class &c)
		const {
			return _info.getClassLevel(c);
		}
		int Creature::getAlignment()
		const {
			return _info.getAlignment();
		}
		void Creature::adjustAlignment(
		int shift) {
			_info.adjustAlignment(shift);
		}
		bool Creature::rollSavingThrow(SavingThrow type,
		int dc) {
			int d20 = RNG.getNext(1, 21);
			int bonus = getSavingThrowBonus(type);
			bool success = (d20 + bonus >= dc);
			debugC(Common::kDebugEngineLogic, 1, 	       "Saving Throw (%d): d20(%d) + bonus(%d) = %d vs DC %d -> %s",	       (int)type, d20, bonus, d20 + bonus, dc, success ? "SUCCESS" : "FAILED");
			return success;
		}
		int Creature::getSavingThrowBonus(SavingThrow type) const {
			// The class bonus, with the current ability modifier instead of the base one.
			Ability ability = kAbilityConstitution;
			if (type == kSavingThrowReflex)
				ability = kAbilityDexterity;
			else if (type == kSavingThrowWill)
				ability = kAbilityWisdom;

			int bonus = _info.getSavingThrowBonus(type) - _info.getAbilityModifier(ability) + getAbilityModifier(ability);

			// Saving throw effects: SAVING_THROW_ALL, then the specific one.
			bonus += _savingThrowModifiers[0];
			if ((type >= kSavingThrowFortitude) && (type <= kSavingThrowWill))
				bonus += _savingThrowModifiers[type + 1];

			return bonus;
		}
		void Creature::setAIArchetype(AIArchetype archetype) {
			_aiArchetype = archetype;
		}
		int Creature::getLevelByPosition(
		int position)
		const {
			return _info.getLevelByPosition(position);
		}
		Class Creature::getClassByPosition(
		int position)
		const {
			return _info.getClassByPosition(position);
		}
		Race Creature::getRace()
		const {
			return _race;
		}
		SubRace Creature::getSubRace()
		const {
			return _subRace;
		}
		float Creature::getWalkRate()
		const {
			return _walkRate;
		}
		float Creature::getRunRate()
		const {
			return _runRate;
		}
		int Creature::getSkillRank(Skill skill) const {
			int baseRank = _info.getSkillRank(skill);
			int modified = baseRank + getSkillModifier(skill);
			return (modified < 0) ? 0 : modified;
		}

		int Creature::getAbilityScore(Ability ability) const {
			const int base = _info.getAbilityScore(ability);
			if ((ability < kAbilityStrength) || (ability > kAbilityCharisma))
				return base;

			const int bonus = _abilityModifiers[ability] + getEquipmentAbilityBonus(ability);

			// Penalties never take a score below 3.
			if (bonus < 0)
				return MAX(base + bonus, MIN(base, 3));

			return base + bonus;
		}

		int Creature::getAbilityModifier(Ability ability) const {
			const int delta = getAbilityScore(ability) - 10;
			if (delta >= 0)
				return delta / 2;

			return -(((-delta) + 1) / 2);
		}

		int Creature::getEquipmentAbilityBonus(Ability ability) const {
			int bonus = 0;
			for (const auto &equipped : _equipment)
				if (equipped.second)
					bonus += equipped.second->getAbilityBonus(ability);

			return bonus;
		}

		void Creature::setForcePoints(
		int fp) {
			_info.setForcePoints(MAX(0, fp));
		}
		void Creature::setMaxForcePoints(
		int fp) {
			_info.setMaxForcePoints(MAX(0, fp));
		}
		int Creature::getForcePoints()
		const {
			return _info.getForcePoints();
		}
		int Creature::getMaxForcePoints()
		const {
			return _info.getMaxForcePoints();
		}
		int Creature::computeMaxForcePoints()
		const {
			int totalFP = 0;
			int jediLevel = 0;
			for (
			int i = 0;
			i < _info.getNumClasses();
			++i) {
				int lv = _info.getLevelByPosition(i);
				KotORBase::Class pcClass = _info.getClassByPosition(i);
				int perLevel = 0;
				switch (pcClass) {
					case kClassJediGuardian:  perLevel = 4;
					break;
					case kClassJediSentinel:  perLevel = 6;
					break;
					case kClassJediConsular:  perLevel = 8;
					break;
					// Prestige classes (mostly follow the same patterns)
					case kClassJediWeaponMaster: perLevel = 4;
					break;
					case kClassJediWatchMan:     perLevel = 6;
					break;
					case kClassJediMaster:       perLevel = 8;
					break;
					case kClassSithMarauder:     perLevel = 4;
					break;
					case kClassSithAssassin:     perLevel = 6;
					break;
					case kClassSithLord:         perLevel = 8;
					break;
					default: break;
				}
				if (perLevel > 0) {
					totalFP += lv * perLevel;
					jediLevel += lv;
				}
			}
			if (jediLevel > 0) {
				int wisMod = _info.getAbilityModifier(kAbilityWisdom);
				int chaMod = _info.getAbilityModifier(kAbilityCharisma);
				totalFP += (wisMod + chaMod) * jediLevel;
			}
			return MAX(0, totalFP);
		}
		void Creature::setPosition(
		float x,
		float y,
		float z) {
			Object::setPosition(x, y, z);
			Object::getPosition(x, y, z);
			if (_model)		_model->setPosition(x, y, z);
		}
		void Creature::setOrientation(
		float x,
		float y,
		float z,
		float angle) {
			Object::setOrientation(x, y, z, angle);
			Object::getOrientation(x, y, z, angle);
			if (_model)		_model->setOrientation(x, y, z, angle);
		}
		void Creature::load(
		const Aurora::GFF3Struct &creature) {
			_templateResRef = creature.getString("TemplateResRef");
			std::unique_ptr<Aurora::GFF3File> utc;
			if (!_templateResRef.empty())		utc.reset(loadOptionalGFF3(_templateResRef, Aurora::kFileTypeUTC, MKTAG('U', 'T', 'C', ' ')));
			load(creature, utc ? &utc->getTopLevel() : 0);
			if (!utc)		warning("Creature \"%s\" has no blueprint", _tag.c_str());
		}
		void Creature::load(
		const Aurora::GFF3Struct &instance,
		const Aurora::GFF3Struct *blueprint) {
			// Area (GIT) instances only carry a TemplateResRef and a position;
			// classes, feats, skills, Force points and powers live in the blueprint.
			const bool instanceHasStats = instance.hasField("ClassList");
			_info = CreatureInfo((blueprint && !instanceHasStats) ? *blueprint : instance);

			// General properties
			if (blueprint)
				loadProperties(*blueprint); // Blueprint
			loadProperties(instance, false); // Instance

			// Force points: templated Jedi without stored values get the class formula.
			if (getMaxForcePoints() <= 0) {
				const int maxFP = computeMaxForcePoints();
				if (maxFP > 0) {
					setMaxForcePoints(maxFP);
					setForcePoints(maxFP);
				}
			}
			if (getForcePoints() > getMaxForcePoints())
				setForcePoints(getMaxForcePoints());

			// Appearance
			if (_appearance == Aurora::kFieldIDInvalid)
				throw Common::Exception("Creature without an appearance");

			loadEquippedModel();

			// Position
			setPosition(instance.getDouble("XPosition"),
			            instance.getDouble("YPosition"),
			            instance.getDouble("ZPosition"));

			// Orientation
			float bearingX = instance.getDouble("XOrientation");
			float bearingY = instance.getDouble("YOrientation");
			setOrientation(0.0f, 0.0f, 1.0f, -Common::rad2deg(atan2(bearingX, bearingY)));
		}
		void Creature::loadProperties(
		const Aurora::GFF3Struct &gff,
		bool clearScripts) {
			// Tag
			_tag = gff.getString("Tag", _tag);

			// Name
			Aurora::LocString firstName;
			gff.getLocString("FirstName", firstName);
			Aurora::LocString lastName;
			gff.getLocString("LastName", lastName);
			if (!firstName.empty()) {
				_name = firstName.getString();
				if (!lastName.empty())
					_name += " " + lastName.getString();
			}

			// Description
			_description = gff.getString("Description", _description);

			// Portrait
			loadPortrait(gff);

			// Equipment
			loadEquipment(gff);

			// Abilities
			loadAbilities(gff);

			// Appearance
			_appearance = gff.getUint("Appearance_Type", _appearance);

			// Static
			_static = gff.getBool("Static", _static);

			// Usable
			_usable = gff.getBool("Useable", _usable);

			// Challenge rating (kill XP, GetChallengeRating)
			if (gff.hasField("ChallengeRating"))
				_challengeRating = static_cast<float>(gff.getDouble("ChallengeRating"));

			// PC
			_isPC = gff.getBool("IsPC", _isPC);

			// Gender
			_gender = Gender(gff.getUint("Gender"));

			// Race
			_race = Race(gff.getSint("Race", _race));
			_subRace = SubRace(gff.getSint("SubraceIndex", _subRace));

			// Hit Points
			_currentHitPoints = gff.getSint("CurrentHitPoints", _maxHitPoints);
			_maxHitPoints = gff.getSint("MaxHitPoints", _currentHitPoints);
			_minOneHitPoint = gff.getBool("Min1HP", _minOneHitPoint);

			// Faction
			_faction = Faction(gff.getUint("FactionID", _faction));

			// Scripts
			readScripts(gff, clearScripts);

			_conversation = gff.getString("Conversation", _conversation);
			setSoundSet(gff.getUint("SoundSetFile", Aurora::kFieldIDInvalid));
		}
		void Creature::loadPortrait(
		const Aurora::GFF3Struct &gff) {
			uint32_t portraitID = gff.getUint("PortraitId");
			if (portraitID != 0) {
				const Aurora::TwoDAFile &twoda = TwoDAReg.get2DA("portraits");
				Common::UString portrait = twoda.getRow(portraitID).getString("BaseResRef");
				if (!portrait.empty()) {
					if (portrait.beginsWith("po_"))				_portrait = portrait;
					else				_portrait = "po_" + portrait;
				}
			}
			_portrait = gff.getString("Portrait", _portrait);
		}
		void Creature::loadEquipment(
		const Aurora::GFF3Struct &gff) {
			if (!gff.hasField("Equip_ItemList"))		return;
			for (
			const auto &i : gff.getList("Equip_ItemList")) {
				InventorySlot slot = InventorySlot(static_cast<int>(std::log2f(i->getID())));
				Common::UString tag = i->getString("EquippedRes");
				equipItem(tag, slot, false);
			}
		}
		void Creature::loadAbilities(
		const Aurora::GFF3Struct &gff) {
			if (gff.hasField("Str"))		_info.setAbilityScore(kAbilityStrength, gff.getUint("Str"));
			if (gff.hasField("Dex"))		_info.setAbilityScore(kAbilityDexterity, gff.getUint("Dex"));
			if (gff.hasField("Con"))		_info.setAbilityScore(kAbilityConstitution, gff.getUint("Con"));
			if (gff.hasField("Int"))		_info.setAbilityScore(kAbilityIntelligence, gff.getUint("Int"));
			if (gff.hasField("Wis"))		_info.setAbilityScore(kAbilityWisdom, gff.getUint("Wis"));
			if (gff.hasField("Cha"))		_info.setAbilityScore(kAbilityCharisma, gff.getUint("Cha"));
		}
		void Creature::getModelState(uint32_t &state, uint8_t &textureVariation) {
			state = 'a';
			textureVariation = 1;
			Item *item = getEquipedItem(kInventorySlotBody);
			if (item) {
				state += item->getBodyVariation() - 1;
				textureVariation = item->getTextureVariation();
			}
		}
		void Creature::getPartModels(PartModels &parts, uint32_t state, uint8_t textureVariation) {
			const Aurora::TwoDARow &appearance = TwoDAReg.get2DA("appearance").getRow(_appearance);
			_modelType = appearance.getString("modeltype");
			if (_modelType == "B") {
				parts.body = appearance.getString(Common::UString("model") + state);
				parts.bodyTexture = appearance.getString(Common::UString("tex") + state) + Common::String::format("%02u", textureVariation);
				// Fall back to a default texture variation
				if (!ResMan.hasResource(parts.bodyTexture))			parts.bodyTexture = appearance.getString(Common::UString("tex") + state) + "01";
			}
			else {
				parts.body = appearance.getString("race");
				parts.bodyTexture = appearance.getString("racetex");
			}
			if ((_modelType == "B") || (_modelType == "P")) {
				const
				int headNormalID = appearance.getInt("normalhead");
				const
				int headBackupID = appearance.getInt("backuphead");
				const Aurora::TwoDAFile &heads = TwoDAReg.get2DA("heads");
				if      (headNormalID >= 0)			parts.head = heads.getRow(headNormalID).getString("head");
				else
				if (headBackupID >= 0)			parts.head = heads.getRow(headBackupID).getString("head");
			}
			loadMovementRate(appearance.getString("moverate"));
		}
		void Creature::loadBody(PartModels &parts) {
			// Model "P_BastilaBB" has broken animations. Replace it with the	// correct one.
			if (Common::String::equalsIgnoreCase(parts.body.c_str(), "P_BastilaBB"))		parts.body = "P_BastilaBB02";
			GfxMan.lockFrame();
			_model.reset(loadModelObject(parts.body, parts.bodyTexture));
			GfxMan.unlockFrame();
			if (!_model)		return;
			_ids.clear();
			_ids.push_back(_model->getID());
			_model->setTag(_tag);
			_model->setClickable(isClickable());
			// Footsteps and weapon sounds: taken in updatePresentation()
			_model->setAnimationEventsEnabled(true);
			if (_modelType != "B" && _modelType != "P")		_model->addAnimationChannel(Graphics::Aurora::kAnimationChannelHead);
		}
		void Creature::loadHead(PartModels &parts) {
			if (!_model || parts.head.empty())		return;
			_headModel = loadModelObject(parts.head);
			if (!_headModel)		return;
			GfxMan.lockFrame();
			_model->attachModel("headhook", _headModel);
			GfxMan.unlockFrame();
		}
		void Creature::loadMovementRate(
		const Common::UString &name) {
			const Aurora::TwoDARow &speed = TwoDAReg.get2DA("creaturespeed").getRow("2daname", name);
			_walkRate = speed.getFloat("walkrate");
			_runRate = speed.getFloat("runrate");
		}
		void Creature::loadEquippedModel() {
			uint32_t state;
			uint8_t textureVariation;
			getModelState(state, textureVariation);
			PartModels parts;
			if (_isPC) {
				getPartModelsPC(parts, state, textureVariation);
				_portrait = parts.portrait;
			}
			else {
				getPartModels(parts, state, textureVariation);
				if ((_modelType == "P") || parts.body.empty()) {
					warning("TODO: Model \"%s\": ModelType \"%s\" (\"%s\")",			        _tag.c_str(), _modelType.c_str(), parts.body.c_str());
					return;
				}
			}
			loadBody(parts);
			loadHead(parts);
			if (!_model)		return;
			attachWeaponModel(kInventorySlotLeftWeapon);
			attachWeaponModel(kInventorySlotRightWeapon);
			setDefaultAnimations();
			if (_visible) {
				float x, y, z;
				getPosition(x, y, z);
				_model->setPosition(x, y, z);
				float angle;
				getOrientation(x, y, z, angle);
				_model->setOrientation(x, y, z, angle);
				_model->show();
			}
		}
		void Creature::attachWeaponModel(InventorySlot slot) {
			assert((slot == kInventorySlotLeftWeapon) || (slot == kInventorySlotRightWeapon));
			Common::UString hookNode;
			switch (slot) {
				case kInventorySlotLeftWeapon:			hookNode = "lhand";
				break;
				case kInventorySlotRightWeapon:			hookNode = "rhand";
				break;
				default:			throw Common::Exception("Unsupported equip slot");
			}
			if (!_model)
				return;
			if (!_model->hasNode(hookNode)) {
				warning("Creature::attachWeaponModel(): Model \"%s\" does not have node \"%s\"",		        _model->getName().c_str(), hookNode.c_str());
				return;
			}
			Graphics::Aurora::Model *weaponModel = 0;
			Item *item = getEquipedItem(slot);
			if (item)
				weaponModel = loadModelObject(item->getModelName());
			GfxMan.lockFrame();
			_model->attachModel(hookNode, weaponModel);
			GfxMan.unlockFrame();
		}
		void Creature::initAsFakePC() {
			_name = "Fakoo McFakeston";
			_tag  = Common::String::format("[PC: %s]", _name.c_str());
			_isPC = true;
			setFaction(kFactionPlayer);
		}
		void Creature::initAsPC(
		const CharacterGenerationInfo &chargenInfo,
		const CreatureInfo &info) {
			_name = chargenInfo.getName();
			_usable = false;
			_isPC = true;
			// The player's faction is repute.2da row 0.
			setFaction(kFactionPlayer);
			_race = kRaceHuman;
			_subRace = kSubRaceNone;
			_gender = chargenInfo.getGender();
			switch (_gender) {
				case kGenderMale:
				case kGenderFemale:			break;
				default:			throw Common::Exception("Unknown gender");
			}
			_info = info;
			_skin = chargenInfo.getSkin();
			_face = chargenInfo.getFace();
			if (_info.hasHitPoints()) {
				_currentHitPoints = _info.getCurrentHitPoints();
				_maxHitPoints = _info.getMaxHitPoints();
			} else {
				int hitDie = classHitDie(chargenInfo.getClass());
				int conMod = _info.getAbilityModifier(kAbilityConstitution);
				int hp = hitDie + conMod;
				if (hp < 1)		hp = 1;
				_currentHitPoints = _maxHitPoints = hp;
			}

			if (_info.getMaxForcePoints() > 0) {
				setMaxForcePoints(_info.getMaxForcePoints());
				setForcePoints(_info.getForcePoints());
			} else {
				setMaxForcePoints(computeMaxForcePoints());
				setForcePoints(getMaxForcePoints());
			}
			setCurrentXP(_info.getExperience());
			reloadEquipment();
			loadEquippedModel();
		}
		void Creature::applyCreatureInfo(const CreatureInfo &info) {
			_info = info;
			if (info.hasHitPoints()) {
				_currentHitPoints = info.getCurrentHitPoints();
				_maxHitPoints = info.getMaxHitPoints();
			}
			if (info.getMaxForcePoints() > 0) {
				setMaxForcePoints(info.getMaxForcePoints());
				setForcePoints(info.getForcePoints());
			}
			setCurrentXP(info.getExperience());
			reloadEquipment();
			loadEquippedModel();
		}
		CreatureInfo Creature::buildSavedState() const {
			CreatureInfo info = _info;
			info.setHitPoints(_currentHitPoints, _maxHitPoints);
			info.setForcePoints(getForcePoints());
			info.setMaxForcePoints(getMaxForcePoints());
			info.setExperience(getCurrentXP());
			return info;
		}
		const Common::UString &Creature::getCursor()
		const {
			static Common::UString talkCursor("talk");
			static Common::UString killCursor("kill");
			static Common::UString useCursor("use");
			if (isDead())
			return useCursor;
			return isEnemy() ? killCursor : talkCursor;
		}
		void Creature::highlight(
		bool enabled) {
			_model->drawBound(enabled);
		}
		bool Creature::click(Object *triggerer) {
			// Try the onDialog script first
			if (hasScript(kScriptDialogue))
			return runScript(kScriptDialogue, this, triggerer);
			// Next, look we have a generic onClick script
			if (hasScript(kScriptClick))
			return runScript(kScriptClick, this, triggerer);
			return false;
		}
		const Common::UString &Creature::getConversation()
		const {
			return _conversation;
		}
		CreatureInfo &Creature::getCreatureInfo() {
			return _info;
		}
		const CreatureInfo &Creature::getCreatureInfo()
		const {
			return _info;
		}
		bool Creature::hasFeat(uint32_t feat)
		const {
			return _info.hasFeat(feat);
		}
		float Creature::getCameraHeight()
		const {
			float height = 1.8f;
			if (_model) {
				Graphics::Aurora::ModelNode *node = _model->getNode("camerahook");
				if (node) {
					float x, y, z;
					node->getPosition(x, y, z);
					height = z;
				}
			}
			return height;
		}
		void Creature::equipItem(Common::UString tag, InventorySlot slot,
		bool updateModel) {
			equipItem(tag, slot, _info, updateModel);
		}
		void Creature::equipItem(Common::UString tag, InventorySlot slot, CreatureInfo &invOwner,
		bool updateModel) {
			if (_info.isInventorySlotEquipped(slot)) {
				Common::UString equippedItem = _info.getEquippedItem(slot);
				_info.unequipInventorySlot(slot);
				invOwner.addInventoryItem(equippedItem);
				_equipment.erase(slot);
			}
			if (!tag.empty() && addItemToEquipment(tag, slot)) {
				invOwner.removeInventoryItem(tag);
				_info.equipItem(tag, slot);
			}
			if (!updateModel)		return;
			switch (slot) {
				case kInventorySlotBody:			loadEquippedModel();
				break;
				case kInventorySlotLeftWeapon:
				case kInventorySlotRightWeapon:			attachWeaponModel(slot);
				break;
				default:			break;
			}
		}
		Inventory &Creature::getInventory() {
			return _info.getInventory();
		}
		Item *Creature::getEquipedItem(InventorySlot slot)
		const {
			if (!_info.isInventorySlotEquipped(slot))
			return nullptr;
			// The item may have failed to load (e.g. missing UTI in a save).
			auto it = _equipment.find(slot);
			return (it == _equipment.end()) ? nullptr : it->second.get();
		}
		float Creature::getMaxAttackRange()
		const {
			const Item *rightWeapon = getEquipedItem(kInventorySlotRightWeapon);
			if (rightWeapon && rightWeapon->isRangedWeapon()) {
				// An empty maxattackrange would leave ranged attackers unable to fire.
				const float range = rightWeapon->getMaxAttackRange();
				return range > 0.0f ? range : 10.0f;
			}
			return 1.0f;
		}
		Item *Creature::addScriptItem(
		const Common::UString &tag) {
			// Return an existing script item for this tag if one already exists
			for (auto &item : _scriptItems)
			if (item && item->getTag() == tag)
			return item.get();
			try {
				_scriptItems.push_back(std::make_unique<Item>(tag));
				return _scriptItems.back().get();
			}
			catch (Common::Exception &e) {
				e.add("Failed to create script item \"%s\"", tag.c_str());
				Common::printException(e, "WARNING: ");
				return nullptr;
			}
		}
		Item *Creature::findInventoryItemByTag(
		const Common::UString &tag)
		const {
			// Search equipped items first
			for (
			const auto &kv : _equipment) {
				if (kv.second && kv.second->getTag() == tag)
				return kv.second.get();
			}
			// Search live script items in inventory
			for (
			const auto &item : _scriptItems) {
				if (item && item->getTag() == tag)
				return item.get();
			}
			return nullptr;
		}
		std::vector<Creature::ItemPower> Creature::getEquippedItemPowers() const {
			std::vector<ItemPower> powers;

			for (const auto &equipped : _equipment) {
				if (!equipped.second)
					continue;

				const Common::UString &item = _info.getEquippedItem(equipped.first);
				for (int spell : equipped.second->getCastSpells()) {
					ItemPower power;
					power.spellId = spell;
					power.item    = item;
					powers.push_back(power);
				}
			}

			return powers;
		}

		std::vector<Creature::ItemPower> Creature::getInventoryItemPowers() const {
			std::vector<ItemPower> powers;

			for (const auto &item : _info.getInventory().getItems()) {
				for (int spell : Item::getConsumableSpells(item.first)) {
					ItemPower power;
					power.spellId = spell;
					power.item    = item.first;
					powers.push_back(power);
				}
			}

			return powers;
		}

		float Creature::getChallengeRating()
		const {
			if (_challengeRating >= 0.0f)
			return _challengeRating;
			return static_cast<float>(getHitDice());
		}
		int Creature::getHitDice()
		const {
			int total = 0;
			const
			int numClasses = _info.getNumClasses();
			for (
			int i = 0;
			i < numClasses;
			++i)		total += _info.getLevelByPosition(i);
			return total;
		}
		int Creature::getBAB()
		const {
			return _info.getBAB();
		}
		int Creature::getAC()
		const {
			int ac = 10 + getAbilityModifier(kAbilityDexterity);
			const Item *armor = getEquipedItem(kInventorySlotBody);
			if (armor)		ac += armor->getACBonus();
			ac += _armorClassModifier;
			return ac;
		}
		void Creature::playDefaultAnimation() {
			if (_model)		_model->playDefaultAnimation();
		}
		void Creature::playDefaultHeadAnimation() {
			if (!_model)		return;
			Graphics::Aurora::AnimationChannel *headChannel = 0;
			if (_modelType == "B" || _modelType == "P") {
				Graphics::Aurora::Model *head = _model->getAttachedModel("headhook");
				if (head)			headChannel = head->getAnimationChannel(Graphics::Aurora::kAnimationChannelAll);
			}
			else		headChannel = _model->getAnimationChannel(Graphics::Aurora::kAnimationChannelHead);
			if (headChannel)		headChannel->playDefaultAnimation();
		}
		void Creature::playDrawWeaponAnimation() {
			if (!_model)		return;
			const Item *rightWeapon = getEquipedItem(kInventorySlotRightWeapon);
			const Item *leftWeapon = getEquipedItem(kInventorySlotLeftWeapon);
			if (rightWeapon && !leftWeapon) {
				switch (rightWeapon->getWeaponWield()) {
					case kWeaponWieldBaton:				_model->playAnimation("g1w1");
					break;
					case kWeaponWieldSword:				_model->playAnimation("g2w1");
					break;
					case kWeaponWieldStaff:				_model->playAnimation("g3w1");
					break;
					case kWeaponWieldPistol:				_model->playAnimation("g5w1");
					break;
					case kWeaponWieldRifle:				_model->playAnimation("g7w1");
					break;
					default:				break;
				}
				return;
			}
			if (rightWeapon && leftWeapon) {
				switch (rightWeapon->getWeaponWield()) {
					case kWeaponWieldSword:				_model->playAnimation("g4w1");
					break;
					case kWeaponWieldPistol:				_model->playAnimation("g6w1");
					break;
					default:				break;
				}
			}
		}
		void Creature::playAttackAnimation() {
			if (!_model)		return;
			// An occasional grunt with the swing
			if (RNG.getNext(0, 3) == 0)
				playRandomSoundSetBark(kSoundSetAttack1, 3);

			const int weapon = getWeaponAnimationNumber();

			// Dueling: both fight in melee, each the other's target
			Creature *target = ObjectContainer::toCreature(_attemptedAttackTarget ? _attemptedAttackTarget : _attackTarget);
			const bool engaged = target && !target->isDead() && (target->getAttackTarget() == this) &&
			                     isMeleeWeaponAnimation(weapon) &&
			                     isMeleeWeaponAnimation(target->getWeaponAnimationNumber());

			const HasAnimation hasAnimation = [this](const Common::UString &anim) {
				return _model->canPlayAnimation(anim);
			};

			if (weapon == -1)
				return;

			Common::UString anim = chooseAttackAnimation(weapon, engaged, RNG.getNext(0, 1000), hasAnimation);
			if (anim.empty())
				anim = (weapon >= 5 && weapon <= 7) ? Common::String::format("b%da1", weapon) :
				                                       Common::String::format("g%da1", weapon);

			_model->playAnimation(anim);

			// The target answers this attack in playDodgeAnimation()
			if (target)
				target->_incomingAttackAnimation = anim;
		}
		void Creature::playDodgeAnimation() {
			if (!_model)		return;

			// Attacking this round: the attack animation shows, not a dodge
			if (_attemptedAttackTarget) {
				_incomingAttackAnimation.clear();
				return;
			}

			const int number = getWeaponAnimationNumber();
			const Common::UString anim = chooseDefenseAnimation(number, _incomingAttackAnimation,
				[this](const Common::UString &name) { return _model->canPlayAnimation(name); });
			_incomingAttackAnimation.clear();

			if (!anim.empty())
				_model->playAnimation(anim);
		}
		void Creature::playAnimation(
		const Common::UString &anim,
		bool restart,
		float length,
		float speed) {
			if (_model)		_model->playAnimation(anim, restart, length, speed);
		}
		void Creature::setStuntModel(Graphics::Aurora::Model *stunt) {
			if (_model)		_model->setStuntModel(stunt);
		}
		bool Creature::hasModelAnimation(const Common::UString &anim) {
			return _model && _model->canPlayAnimation(anim);
		}
		Common::UString Creature::getScriptAnimationName(int animation) const {
			const bool simple = (_modelType == "S") || (_modelType == "L");
			return KotORBase::getScriptAnimationName(animation, getWeaponAnimationNumber(), simple);
		}
		float Creature::getModelAnimationLength(const Common::UString &anim) {
			return _model ? _model->getAnimationLength(anim) : 0.0f;
		}
		void Creature::getTooltipAnchor(
		float &x,
		float &y,
		float &z)
		const {
			if (!_model) {
				Object::getTooltipAnchor(x, y, z);
				return;
			}
			_model->getTooltipAnchor(x, y, z);
		}
		bool Creature::canPerceive(const Creature &target, float distance) const {
			PerceptionRules::SightCheck check;
			check.hostile         = ReputationMan.isHostile(static_cast<int>(getFaction()),
			                                                static_cast<int>(target.getFaction()));
			check.targetInvisible = target.isInvisible();
			check.seesInvisible   = canSeeInvisible();
			check.targetStealthy  = target.getStealthMode();
			check.targetStealth   = target.getSkillRank(kSkillStealth);
			check.awareness       = getSkillRank(kSkillAwareness);
			check.distance        = distance;

			return PerceptionRules::canSee(check);
		}
		void Creature::updatePerception(Creature &object) {
			float distance = glm::distance(		glm::make_vec3(_position),		glm::make_vec3(object._position));
			// Hearing goes by distance; sight can be fooled (invisibility, stealth).
			if (distance <= PerceptionRules::kPerceptionRange) {
				handleObjectHeard(object);
				object.handleObjectHeard(*this);
			} else {
				handleObjectInaudible(object);
				object.handleObjectInaudible(*this);
			}

			if (canPerceive(object, distance))
				handleObjectSeen(object);
			else
				handleObjectVanished(object);

			if (object.canPerceive(*this, distance))
				object.handleObjectSeen(*this);
			else
				object.handleObjectVanished(*this);
		}
		bool Creature::isInCombat()
		const {
			return _inCombat;
		}
		Object *Creature::getAttackTarget()
		const {
			return _attackTarget;
		}
		int Creature::getAttackRound()
		const {
			return _attackRound;
		}
		Object *Creature::getAttemptedAttackTarget()
		const {
			return _attemptedAttackTarget;
		}
		Object *Creature::getLastHostileActor()
		const {
			return _lastHostileActor;
		}

		Object *Creature::getLastKiller() const {
			return _lastKiller;
		}

		void Creature::setLastKiller(Object *killer) {
			_lastKiller = killer;
		}

		void Creature::recordDamageTaken(int amount, int damageType, Object *damager) {
			_lastDamageTotal = amount;
			_lastDamageType = damageType;
			if (damager)
				_lastHostileActor = damager;
			if (amount > 0 && !_dead) {
				_damagedEventPending = true;
				if (_model)
					playRandomSoundSetBark(kSoundSetPain1, 2);
			}
		}

		int Creature::getDamageDealtByType(int damageType) const {
			if (damageType == 0)
				return _lastDamageTotal;
			if ((_lastDamageType & damageType) == 0)
				return -1;
			return _lastDamageTotal;
		}

		int Creature::getTotalDamageDealt() const {
			return _lastDamageTotal;
		}

		void Creature::recordAttackResult(int result, Object *weapon) {
			_lastAttackResult = result;
			if (weapon)
				_lastWeaponUsed = weapon;
		}

		int Creature::getLastAttackResult() const {
			return _lastAttackResult;
		}

		Object *Creature::getLastWeaponUsed() const {
			return _lastWeaponUsed;
		}

		void Creature::addImmunity(int immunityType) {
			_immunities.insert(immunityType);
		}

		void Creature::addSpellImmunity(int spellId) {
			_spellImmunities.insert(spellId);
		}

		bool Creature::isImmune(int immunityType) const {
			return _immunities.find(immunityType) != _immunities.end();
		}

		bool Creature::isImmuneToSpell(int spellId) const {
			return _spellImmunities.find(spellId) != _spellImmunities.end();
		}

		void Creature::adjustDamageImmunity(int damageType, int percentDelta) {
			int &value = _damageImmunityPercent[damageType];
			value += percentDelta;
			if (value < 0)
				value = 0;
			if (value > 100)
				value = 100;
		}

		int Creature::getDamageImmunityPercent(int damageType) const {
			std::map<int, int>::const_iterator it = _damageImmunityPercent.find(damageType);
			return it != _damageImmunityPercent.end() ? it->second : 0;
		}

		void Creature::clearImmunities() {
			_immunities.clear();
			_spellImmunities.clear();
			_damageImmunityPercent.clear();
		}

		void Creature::surrenderToEnemies(bool retainBuffs) {
			cancelCombat();
			clearActions();
			setFaction(kFactionSurrender1);
			if (!retainBuffs) {
				// Reverse the modifiers the effects applied, don't just drop them.
				clearActiveEffects();
				clearImmunities();
			}
		}

		void Creature::stopAttacksOn(const std::vector<Creature *> &creatures, const Object *target,
		                             bool clearQueuedActions) {
			if (!target)
				return;

			for (Creature *c : creatures) {
				if (!c || c == target)
					continue;

				const Action *current = c->getCurrentAction();
				const bool attacking = (c->_attackTarget == target) ||
				                       (c->_attemptedAttackTarget == target) ||
				                       (current && current->object == target &&
				                        (current->type == kActionAttackObject || current->type == kActionCastSpell));
				if (!attacking)
					continue;

				if (c->_attackTarget == target)
					c->cancelCombat();
				if (c->_attemptedAttackTarget == target)
					c->_attemptedAttackTarget = nullptr;

				if (clearQueuedActions)
					c->clearActions();
				else
					c->_actions.removeActionsTargeting(target);
			}
		}

		void Creature::regenerateForcePoints() {
			if (_dead || _inCombat)
				return;

			const int maxFP = getMaxForcePoints();
			const int fp = getForcePoints();
			if (maxFP <= 0 || fp >= maxFP)
				return;

			// Conservative: 5% of the maximum (at least 1) per heartbeat.
			setForcePoints(MIN(maxFP, fp + MAX(1, maxFP / 20)));
		}

		void Creature::restoreFully() {
			if (_dead)
				return;

			setCurrentHitPoints(getMaxHitPoints());
			setForcePoints(getMaxForcePoints());
		}

		int Creature::getDamageResistance(int damageType) const {
			std::map<int, int>::const_iterator it = _damageResistance.find(damageType);
			int resistance = (it != _damageResistance.end()) ? it->second : 0;

			for (const ActiveEffect &e : _effects)
				if ((e.type == kEffectModifier) && !e.expired &&
				    (e.kotorType == kKotOREffectDamageResistance) && (e.subType & damageType))
					resistance += e.value;

			return resistance;
		}

		int Creature::absorbDamage(int damage, int damageType) {
			if (damage <= 0)
				return 0;

			// Innate resistance (SetDamageResistance) takes its share of every hit.
			std::map<int, int>::const_iterator it = _damageResistance.find(damageType);
			if (it != _damageResistance.end())
				damage = MAX(0, damage - it->second);

			static const int kPhysicalDamage = kDamageTypeBludgeoning | kDamageTypePiercing | kDamageTypeSlashing;

			for (ActiveEffect &e : _effects) {
				if (damage <= 0)
					break;
				if ((e.type != kEffectModifier) || e.expired)
					continue;

				const bool resistance = (e.kotorType == kKotOREffectDamageResistance) ||
				                        (e.kotorType == kKotOREffectForceShield);
				const bool reduction  = (e.kotorType == kKotOREffectDamageReduction);
				if (!resistance && !reduction)
					continue;

				// Resistance and shields cover their damage types (all for 0),
				// damage reduction stops physical damage.
				if (resistance && (e.subType != 0) && !(e.subType & damageType))
					continue;
				if (reduction && !(damageType & kPhysicalDamage))
					continue;

				// value: absorbed per hit (0: all of it); limit: in total (0: no limit).
				int absorbed = (e.value > 0) ? MIN(e.value, damage) : damage;
				if (e.limit > 0) {
					absorbed = MIN(absorbed, e.limit);
					e.limit -= absorbed;
					if (e.limit <= 0) {
						// Used up: the effect ends at the next update.
						e.expired  = true;
						e.duration = 0.0f;
					}
				}

				damage -= absorbed;
			}

			return MAX(0, damage);
		}

		float Creature::knockBack(Area &area, float fromX, float fromY, float distance) {
			float x, y, z;
			getPosition(x, y, z);

			float dirX = x - fromX;
			float dirY = y - fromY;
			float length = std::sqrt(dirX * dirX + dirY * dirY);
			if (length < 0.001f) {
				// No source to push away from: push backwards from the creature's facing.
				float ox, oy, oz, angle;
				getOrientation(ox, oy, oz, angle);
				const float rad = Common::deg2rad(angle);
				dirX =  std::sin(rad);
				dirY = -std::cos(rad);
				length = 1.0f;
			}

			dirX /= length;
			dirY /= length;

			// Land as far away as possible, but only where the walkmesh goes.
			for (float d = distance; d >= 1.0f; d -= 1.0f) {
				const float nx = x + dirX * d;
				const float ny = y + dirY * d;
				const float nz = area.evaluateElevation(nx, ny);
				if (nz == FLT_MIN)
					continue;
				if (!area.walkable(glm::vec3(x, y, z + 0.1f), glm::vec3(nx, ny, nz + 0.1f)))
					continue;

				setPosition(nx, ny, nz);
				area.notifyObjectMoved(*this);
				return d;
			}

			return 0.0f;
		}

		void Creature::setDamageResistance(int damageType, int amount) {
			_damageResistance[damageType] = amount > 0 ? amount : 0;
		}

		int Creature::getLastCombatFeatUsed()
		const {
			return _lastCombatFeatUsed;
		}

		int Creature::getLastForcePowerUsed() const {
			return _lastForcePowerUsed;
		}

		void Creature::setLastForcePowerUsed(int spellID) {
			_lastForcePowerUsed = spellID;
		}
		int Creature::getQueuedCombatFeat()
		const {
			return _queuedCombatFeat;
		}
		void Creature::setAttemptedAttackTarget(Object *target) {
			_attemptedAttackTarget = target;
		}
		void Creature::setLastCombatFeatUsed(
		int featID) {
			_lastCombatFeatUsed = featID;
		}
		void Creature::queueCombatFeat(
		int featID) {
			_queuedCombatFeat = featID;
		}
		int Creature::consumeQueuedCombatFeat() {
			const
			int queuedFeat = _queuedCombatFeat;
			_queuedCombatFeat = -1;
			return queuedFeat;
		}
		void Creature::adjustAttackModifier(
		int amount) {
			_attackModifier += amount;
		}
		void Creature::adjustArmorClassModifier(
		int amount) {
			_armorClassModifier += amount;
		}
		void Creature::adjustSkillModifier(Skill skill,
		int amount) {
			if (skill < kSkillComputerUse || skill >= kSkillMAX)		return;
			_skillModifiers[static_cast<int>(skill)] += amount;
		}
		int Creature::getAttackModifier()
		const {
			return _attackModifier;
		}
		int Creature::getArmorClassModifier()
		const {
			return _armorClassModifier;
		}
		int Creature::getSkillModifier(Skill skill) const {
			if (skill < kSkillComputerUse || skill >= kSkillMAX)
				return 0;

			int bonus = _skillModifiers[static_cast<int>(skill)];

			// Ability modifiers for skills
			switch (skill) {
				case kSkillComputerUse:
				case kSkillDemolitions:
				case kSkillRepair:
				case kSkillSecurity:
					bonus += getAbilityModifier(kAbilityIntelligence);
					break;
				case kSkillStealth:
					bonus += getAbilityModifier(kAbilityDexterity);
					break;
				case kSkillAwareness:
				case kSkillTreatInjury:
					bonus += getAbilityModifier(kAbilityWisdom);
					break;
				case kSkillPersuade:
					bonus += getAbilityModifier(kAbilityCharisma);
					break;
				default:
					break;
			}

			return bonus;
		}
		Object *Creature::findCombatTarget(Area *area) {
			if (!area) return nullptr;

			Object *bestTarget = nullptr;
			float bestDist = 1000.0f;

			for (auto &obj : area->getCreatures()) {
				if (obj == this || obj->isDead()) continue;
				// TODO: Reputation check - treat all non-party as hostile for now

				float dist = getDistanceTo(obj);
				if (dist < bestDist) {
					bestDist = dist;
					bestTarget = obj;
				}
			}
			return bestTarget;
		}

		void Creature::updateCombatAI() {
			if (isPC() || isDead()) return;

			// If no target, try to find one
			if (!_attackTarget || _attackTarget->isDead()) { // may be a door/placeable
				_attackTarget = nullptr; // Combat target needs area context
			}

			if (!_attackTarget) {
				cancelCombat();
				return;
			}

			// Decide action
			float dist = getDistanceTo(_attackTarget);
			if (dist > getMaxAttackRange() + (ObjectContainer::toSituated(_attackTarget) ? BashRules::kExtraReach : 0.0f)) {
				// Move closer, unless already on the way (this runs every round).
				const Action *current = _actions.getCurrent();
				if (current && (current->type == kActionMoveToPoint ||
				                (current->type == kActionAttackObject && current->object == _attackTarget)))
					return;
				if (current && current->type != kActionFollowLeader)
					return; // Busy with something else (e.g. a scripted action).
				if (current)
					clearActions();

				float tx, ty, tz;
				_attackTarget->getPosition(tx, ty, tz);
				Action move(kActionMoveToPoint);
				move.location = glm::vec3(tx, ty, tz);
				move.range = getMaxAttackRange();
				addAction(move);
			} else {
				// Attack is already handled by notifyCombatRoundBegan if inCombat is true
				_inCombat = true;
			}
		}

		bool Creature::areOnOppositeSides(float defX, float defY,
		                                  float aX, float aY,
		                                  float bX, float bY) {
			const float ax = aX - defX;
			const float ay = aY - defY;
			const float bx = bX - defX;
			const float by = bY - defY;
			const float lenA = std::sqrt(ax * ax + ay * ay);
			const float lenB = std::sqrt(bx * bx + by * by);
			if (lenA < 0.01f || lenB < 0.01f)
				return false;
			const float dot = (ax / lenA) * (bx / lenB) + (ay / lenA) * (by / lenB);
			// ~120° or more apart (classic flanking cone).
			return dot < -0.5f;
		}

		bool Creature::isFlankedBy(Creature *attacker, Area *area) {
			if (!attacker || attacker == this || isDead() || !area)
				return false;

			float dx, dy, dz;
			getPosition(dx, dy, dz);
			float ax, ay, az;
			attacker->getPosition(ax, ay, az);

			const float threatRange = 5.0f;
			const bool attackerHostile = attacker->isEnemy();

			for (Creature *other : area->getCreatures()) {
				if (!other || other == this || other == attacker || other->isDead())
					continue;

				// Ally of the attacker: same hostility polarity toward the party.
				if (other->isEnemy() != attackerHostile)
					continue;

				if (getDistanceTo(other) > threatRange)
					continue;

				// Prefer creatures already engaged with us, but allow nearby melee allies.
				if (other->isInCombat() && other->getAttackTarget() != this)
					continue;

				float ox, oy, oz;
				other->getPosition(ox, oy, oz);
				if (areOnOppositeSides(dx, dy, ax, ay, ox, oy))
					return true;
			}

			return false;
		}
		void Creature::startCombat(Object *target,
		int round) {
			if (!target) {
				cancelCombat();
				return;
			}
			// Re-engaging the same target mid-round must not wipe the attack
			// notifyCombatRoundBegan() already scheduled for this round.
			if (_attackTarget != target)
				_attemptedAttackTarget = nullptr;
			if (!_inCombat && _model && !_dead && (RNG.getNext(0, 3) == 0))
				playRandomSoundSetBark(kSoundSetBattleCry1, 6);
			_inCombat = true;
			_attackTarget = target;
			_attackRound = round;
		}
		void Creature::cancelCombat() {
			_inCombat = false;
			_attackTarget = nullptr;
		}
		void Creature::forgetObject(const Object *object) {
			if (_attackTarget == object)
				cancelCombat();
			if (_attemptedAttackTarget == object)		_attemptedAttackTarget = nullptr;
			if (_lastHostileActor == object)		_lastHostileActor = nullptr;
			if (_lastKiller == object)		_lastKiller = nullptr;
			if (_lastWeaponUsed == object)		_lastWeaponUsed = nullptr;
			if (_lastPerceived == object)		_lastPerceived = nullptr;
			if (_blockingDoor == object)		_blockingDoor = nullptr;
			if (_lastAttacker == object) {
				_lastAttacker = nullptr;
				_attackedEventPending = false;
			}
			for (auto it = _perceptionEvents.begin(); it != _perceptionEvents.end(); ) {
				if (it->object == object)
					it = _perceptionEvents.erase(it);
				else
					++it;
			}
			_seenObjects.erase(const_cast<Object *>(object));
			_heardObjects.erase(const_cast<Object *>(object));
			_actions.removeActionsTargeting(object);
		}
		void Creature::applyEffect(
		const ActiveEffect &effect) {
			int current = getCurrentHitPoints();
			switch (effect.type) {
				case kEffectHeal: {
					int maxHP = getMaxHitPoints();
					int healed = current + effect.value;
					if (healed > maxHP)
						healed = maxHP;
					setCurrentHitPoints(healed);
					break;
				}
				case kEffectDamage: {
					int minHp = getMinOneHitPoints() ? 1 : 0;
					int damaged = current - effect.value;
					if (damaged < minHp)
						damaged = minHp;
					setCurrentHitPoints(damaged);
					if (getCurrentHitPoints() <= 0) {
						cancelCombat();
						handleDeath();
					}
					break;
				}
				case kEffectShield: {
					// Temporary HP (shield)
					int maxHP = getMaxHitPoints();
					int boosted = current + effect.value;
					if (boosted > maxHP)
						boosted = maxHP;
					setCurrentHitPoints(boosted);
					break;
				}
				case kEffectKnockdown:
					adjustArmorClassModifier(-4);
					break;
				case kEffectStun:
					clearActions();
					adjustArmorClassModifier(-2);
					break;
				case kEffectConfusion:
					clearActions();
					adjustArmorClassModifier(-4);
					break;
				case kEffectSpeed:
					// Movement speed increase - future implementation
					break;
				case kEffectModifier:
					adjustEffectModifiers(effect, 1);
					break;
				case kEffectVFX:
					playVisualEffect(effect.value, true);
					break;
				default:
					break;
			}
		}

		/** Duration of DURATION_TYPE_PERMANENT effects. */
		static const float kPermanentEffectDuration = 1.0e9f;

		void Creature::applyEffect(
		const Engines::KotORBase::Effect &effect,
		float durationOverride) {
			// Linked effects share their duration and a link id, so that removing
			// one of them removes them all.
			const int linkId = effect.getLinkedEffects().empty() ? 0 : ++_nextEffectLinkId;

			applyEffectPart(effect, durationOverride, linkId);
			for (const Effect &part : effect.getLinkedEffects()) {
				if (_dead && (part.getType() != kKotOREffectResurrection))
					break;

				applyEffectPart(part, durationOverride, linkId);
			}
		}

		void Creature::addActiveEffect(const ActiveEffect &effect) {
			_effects.push_back(effect);
			applyEffect(effect);
		}

		void Creature::adjustEffectModifiers(const ActiveEffect &effect, int sign) {
			const int value = effect.value * sign;

			switch (effect.kotorType) {
				case kKotOREffectACIncrease:
					adjustArmorClassModifier(value);
					break;
				case kKotOREffectACDecrease:
					adjustArmorClassModifier(-value);
					break;
				case kKotOREffectAttackIncrease:
					adjustAttackModifier(value);
					break;
				case kKotOREffectAttackDecrease:
					adjustAttackModifier(-value);
					break;
				case kKotOREffectDamageIncrease:
					_damageModifier += value;
					break;
				case kKotOREffectDamageDecrease:
					_damageModifier -= value;
					break;
				case kKotOREffectAbilityIncrease:
				case kKotOREffectAbilityDecrease:
					if ((effect.subType >= kAbilityStrength) && (effect.subType <= kAbilityCharisma))
						_abilityModifiers[effect.subType] +=
							(effect.kotorType == kKotOREffectAbilityIncrease) ? value : -value;
					break;
				case kKotOREffectSkillIncrease:
					adjustSkillModifier(static_cast<Skill>(effect.subType), value);
					break;
				case kKotOREffectSkillDecrease:
					adjustSkillModifier(static_cast<Skill>(effect.subType), -value);
					break;
				case kKotOREffectSavingThrowIncrease:
				case kKotOREffectSavingThrowDecrease:
					// SAVING_THROW_ALL (0), FORT (1), REFLEX (2), WILL (3).
					if ((effect.subType >= 0) && (effect.subType < static_cast<int>(_savingThrowModifiers.size())))
						_savingThrowModifiers[effect.subType] +=
							(effect.kotorType == kKotOREffectSavingThrowIncrease) ? value : -value;
					break;
				case kKotOREffectForceResistanceIncrease:
					adjustForceResistance(value);
					break;
				case kKotOREffectForceResistanceDecrease:
					adjustForceResistance(-value);
					break;
				case kKotOREffectBlasterDeflectionIncrease:
					adjustBlasterDeflection(value);
					break;
				case kKotOREffectBlasterDeflectionDecrease:
					adjustBlasterDeflection(-value);
					break;
				case kKotOREffectDamageImmunityIncrease:
					adjustDamageImmunity(effect.subType, value);
					break;
				case kKotOREffectDamageImmunityDecrease:
					adjustDamageImmunity(effect.subType, -value);
					break;
				case kKotOREffectInvisibility:
					_invisibility += sign;
					break;
				case kKotOREffectSeeInvisible:
					_seeInvisible += sign;
					break;
				default:
					// Damage resistance/reduction and Force shields act in absorbDamage().
					break;
			}
		}
		void Creature::applyEffectPart(
		const Engines::KotORBase::Effect &effect,
		float durationOverride,
		int linkId) {
			auto timed = [&](EffectType type, float defaultDuration, int value = 0) {
				const float duration = durationOverride >= 0.0f ? durationOverride : defaultDuration;
				ActiveEffect e;
				e.type = type;
				e.duration = duration;
				e.value = value;
				e.spellId = effect.getSpellId();
				e.linkId = linkId;
				if (duration <= 0.0f) {
					// DURATION_TYPE_INSTANT: apply mechanical impact, do not linger.
					applyEffect(e);
					removeEffect(e);
					return;
				}
				e.permanent = duration >= kPermanentEffectDuration;
				addActiveEffect(e);
			};

			// Lasting modifiers (AC, attack, abilities, resistances...): engine
			// calls without a duration make them permanent, instant ones do nothing.
			auto modifier = [&](int value, int subType, int limit) {
				const float duration = durationOverride >= 0.0f ? durationOverride : kPermanentEffectDuration;
				if (duration <= 0.0f)
					return;

				ActiveEffect e;
				e.type = kEffectModifier;
				e.duration = duration;
				e.value = value;
				e.spellId = effect.getSpellId();
				e.kotorType = effect.getType();
				e.subType = subType;
				e.limit = limit;
				e.linkId = linkId;
				e.permanent = duration >= kPermanentEffectDuration;
				addActiveEffect(e);
			};

			int current = getCurrentHitPoints();
			switch (effect.getType()) {
				case kKotOREffectHeal: {
					int maxHP = getMaxHitPoints();
					int healed = current + effect.getAmount();
					if (healed > maxHP)
						healed = maxHP;
					setCurrentHitPoints(healed);
					break;
				}
				case kKotOREffectDamage: {
					int amount = effect.getAmount();
					int damageType = effect.getDamageType();
					int immunity = getDamageImmunityPercent(damageType);
					if (immunity > 0)
						amount = amount - (amount * immunity) / 100;
					amount = absorbDamage(amount, damageType);
					if (amount < 0)
						amount = 0;
					recordDamageTaken(amount, damageType);
					int minHp = getMinOneHitPoints() ? 1 : 0;
					int damaged = current - amount;
					if (damaged < minHp)
						damaged = minHp;
					setCurrentHitPoints(damaged);
					if (getCurrentHitPoints() <= 0) {
						cancelCombat();
						if (_lastHostileActor)
							_lastKiller = _lastHostileActor;
						handleDeath();
					}
					break;
				}
				case kKotOREffectDeath:
					// Plot creatures cannot be killed (Object::_plotFlag: "immune to
					// permanent death"); death immunity blocks it as well. Min1HP keeps
					// the creature at 1 HP, which is why the original scripts clear it
					// with SetMinOneHP(FALSE) before applying EffectDeath.
					if (_dead || getPlotFlag() || isImmune(kImmunityTypeDeath))
						break;
					if (getMinOneHitPoints()) {
						setCurrentHitPoints(MIN(current, 1));
						break;
					}
					setCurrentHitPoints(0);
					cancelCombat();
					if (_lastHostileActor)
						_lastKiller = _lastHostileActor;
					handleDeath();
					break;
				case kKotOREffectTemporaryHitpoints: {
					int maxHP = getMaxHitPoints();
					int boosted = current + effect.getAmount();
					if (boosted > maxHP)
						boosted = maxHP;
					setCurrentHitPoints(boosted);
					break;
				}
				case kKotOREffectForceShield: {
					// forceshields.2da: the damage types it stops, how much of each hit
					// ("resistance", 0 for all of it) and how much in total ("amount").
					// Without the table: a shield that soaks 10 damage per shield row.
					int damageFlags = 0, perHit = 0, total = 10 * (MAX(effect.getAmount(), 0) + 1);
					try {
						const Aurora::TwoDAFile &shields = TwoDAReg.get2DA("forceshields");
						if ((effect.getAmount() >= 0) && (static_cast<size_t>(effect.getAmount()) < shields.getRowCount())) {
							const Aurora::TwoDARow &row = shields.getRow(effect.getAmount());
							damageFlags = row.getInt("damageflags");
							perHit      = row.getInt("resistance");
							total       = row.getInt("amount");
						}
					} catch (...) {
					}
					modifier(perHit, damageFlags, total);
					break;
				}
				case kKotOREffectForcePointsHeal:
					setForcePoints(MIN(getForcePoints() + MAX(effect.getAmount(), 0), getMaxForcePoints()));
					break;
				case kKotOREffectForcePointsDamage:
				case kKotOREffectForceDrain:
					setForcePoints(getForcePoints() - MAX(effect.getAmount(), 0));
					break;
				case kKotOREffectForcePush:
					// The knockback itself needs the area (see knockBack()); here the
					// target falls and is stunned while it gets up.
					if (!isImmune(kImmunityTypeKnockdown)) {
						const Common::UString anim = getAnimationNameById(kAnimationForcePushed);
						if (!anim.empty())
							playAnimation(anim, true);
						timed(kEffectKnockdown, 3.0f);
					}
					break;
				case kKotOREffectACIncrease:
				case kKotOREffectACDecrease:
				case kKotOREffectAttackIncrease:
				case kKotOREffectAttackDecrease:
				case kKotOREffectDamageIncrease:
				case kKotOREffectDamageDecrease:
				case kKotOREffectAbilityIncrease:
				case kKotOREffectAbilityDecrease:
				case kKotOREffectSkillIncrease:
				case kKotOREffectSkillDecrease:
				case kKotOREffectSavingThrowIncrease:
				case kKotOREffectSavingThrowDecrease:
				case kKotOREffectInvisibility:
				case kKotOREffectSeeInvisible:
				case kKotOREffectDamageImmunityDecrease:
					modifier(effect.getAmount(), effect.getDamageType(), 0);
					break;
				case kKotOREffectDamageReduction:
					modifier(effect.getAmount(), 0, effect.getLimit());
					break;
				case kKotOREffectKnockdown:
					timed(kEffectKnockdown, 3.0f);
					break;
				case kKotOREffectStunned:
				case kKotOREffectParalyze:
					timed(kEffectStun, 6.0f);
					break;
				case kKotOREffectHaste:
				case kKotOREffectMovementSpeedIncrease:
					timed(kEffectSpeed, durationOverride >= 0.0f ? durationOverride : 30.0f, 50);
					break;
				case kKotOREffectPoison:
					timed(kEffectPoison,
					      effect.getDamageType() > 0 ? static_cast<float>(effect.getDamageType()) : 6.0f,
					      effect.getAmount());
					break;
				case kKotOREffectImmunity:
					addImmunity(effect.getAmount());
					break;
				case kKotOREffectSpellImmunity:
					addSpellImmunity(effect.getAmount());
					break;
				case kKotOREffectDamageImmunityIncrease:
					modifier(effect.getAmount(), effect.getDamageType(), 0);
					break;
				case kKotOREffectSleep:
					if (!isImmune(kImmunityTypeSleep) && !isImmune(kImmunityTypeMindSpells))
						timed(kEffectStun, 12.0f);
					break;
				case kKotOREffectRegenerate:
					applyEffect(kEffectHeal, 0.0f, effect.getAmount(), effect.getSpellId());
					break;
				case kKotOREffectResurrection:
					resurrect(MAX(1, effect.getAmount()));
					break;
				case kKotOREffectTemporaryForcePoints: {
					int fp = getForcePoints() + effect.getAmount();
					setForcePoints(fp);
					break;
				}
				case kKotOREffectDamageResistance:
					modifier(effect.getAmount(), effect.getDamageType(), effect.getLimit());
					break;
				case kKotOREffectConcealment:
					_concealment = effect.getAmount();
					if (_concealment < 0) _concealment = 0;
					if (_concealment > 100) _concealment = 100;
					break;
				case kKotOREffectAssuredHit:
					_assuredHit = true;
					break;
				case kKotOREffectAssuredDeflection:
					adjustBlasterDeflection(effect.getAmount() > 0 ? effect.getAmount() : 50);
					break;
				case kKotOREffectEntangle:
				case kKotOREffectDroidStun:
					timed(kEffectStun, 6.0f);
					break;
				case kKotOREffectBodyFuel:
					applyEffect(kEffectHeal, 0.0f, effect.getAmount() > 0 ? effect.getAmount() : 5, effect.getSpellId());
					break;
				case kKotOREffectForceResistanceIncrease:
				case kKotOREffectForceResistanceDecrease:
				case kKotOREffectBlasterDeflectionIncrease:
				case kKotOREffectBlasterDeflectionDecrease:
					modifier(effect.getAmount(), 0, 0);
					break;
				case kKotOREffectDispelMagicAll:
					clearActiveEffects();
					break;
				case kKotOREffectVisual: {
					// Instant: play once. With a duration, a duration visual lasts
					// as long as the effects it's linked to (see updateEffects()).
					const float duration = durationOverride > 0.0f ? durationOverride : 0.0f;
					if (duration <= 0.0f) {
						playVisualEffect(effect.getAmount());
						break;
					}

					LastingVisual visual;
					visual.id = effect.getAmount();
					visual.linkId = linkId;
					visual.timeLeft = duration;
					_lastingVisuals.push_back(visual);

					playVisualEffect(visual.id, true);
					break;
				}
				case kKotOREffectAreaOfEffect:
				case kKotOREffectForceJump:
				case kKotOREffectBeam:
				case kKotOREffectForceResisted:
				case kKotOREffectForceFizzle:
				case kKotOREffectHitPointChangeWhenDying:
					// Visual / marker effects — no further mechanical state for P1.
					break;
				default:
					break;
			}
		}

		void Creature::clearActiveEffects() {
			while (!_effects.empty()) {
				removeEffect(_effects.back());
				_effects.pop_back();
			}
		}

		bool Creature::removeActiveEffectAt(size_t index) {
			if (index >= _effects.size())
				return false;

			const int linkId = _effects[index].linkId;
			removeEffect(_effects[index]);
			_effects.erase(_effects.begin() + static_cast<std::ptrdiff_t>(index));

			// Removing one part of a linked effect removes the whole link.
			if (linkId != 0) {
				for (size_t i = _effects.size(); i-- > 0; ) {
					if (_effects[i].linkId != linkId)
						continue;

					removeEffect(_effects[i]);
					_effects.erase(_effects.begin() + static_cast<std::ptrdiff_t>(i));
				}
			}

			return true;
		}
		void Creature::executeAttack(Object *target,
		int babPenalty,
		int damageMod,
		int activeFeat,
		Area *area) {
			if (!target) {
				cancelCombat();
				return;
			}
			if (_dead) {
				cancelCombat();
				return;
			}
			Creature *targetCreature = ObjectContainer::toCreature(target);
			if (targetCreature && targetCreature->isDead()) {
				cancelCombat();
				return;
			}
			// Attacking gives a sneaking creature away.
			if (_stealthMode)
				setStealthMode(false);
			// Track hostility for NWScript callers (GetLastHostileActor/GetLastAttacker).
			if (targetCreature) {
				targetCreature->_lastHostileActor = this;
				// OnAttacked runs later, outside the creature iteration.
				targetCreature->_lastAttacker = this;
				targetCreature->_attackedEventPending = true;
			} else if (Situated *situated = ObjectContainer::toSituated(target)) {
				if (situated->isDead()) {
					cancelCombat();
					return;
				}
				situated->notifyAttacked(this);
			}
			Item *rightWeapon = getEquipedItem(kInventorySlotRightWeapon);
			Item *leftWeapon  = getEquipedItem(kInventorySlotLeftWeapon);
			Object *weaponUsed = rightWeapon ? static_cast<Object *>(rightWeapon)
			                                 : static_cast<Object *>(leftWeapon);
			// Determine attack stat: Dex for ranged, Str for melee.
			bool ranged = (rightWeapon && rightWeapon->isRangedWeapon()) ||
			              (leftWeapon  && leftWeapon->isRangedWeapon());
			int abMod = ranged ? getAbilityModifier(kAbilityDexterity)
			                   : getAbilityModifier(kAbilityStrength);

			// Flanking bonus (+2)
			int flankingMod = 0;
			if (!ranged && targetCreature && targetCreature->isFlankedBy(this, area)) {
				flankingMod = 2;
				debugC(Common::kDebugEngineLogic, 1, "Attacker %s gains flanking bonus vs %s", getName().c_str(), target->getName().c_str());
			}

			// Ranged attacks require clear line of sight past closed doors.
			if (ranged && area && !area->hasLineOfSight(this, target)) {
				debugC(Common::kDebugEngineLogic, 1, "Object \"%s\" ranged attack blocked by cover vs \"%s\"",
				       _tag.c_str(), target->getTag().c_str());
				recordAttackResult(kAttackResultMiss, weaponUsed);
				return;
			}

			// Feat modifiers: attack/damage bonuses from active combat feats.
			int featAttackMod = flankingMod;
			int featDamageMod = damageMod;
			// (Sneak Attack damage is rolled once, after the hit is confirmed —
			//  see the "Sneak Attack" block further down.)

			// The round's activated combat feat (Power Attack/Blast, Critical
			// Strike, Sniper Shot...). Flurry and Rapid Shot penalties come in
			// babPenalty with the round's extra attack.
			int featThreatRange = 0;
			if ((activeFeat >= 0) && _info.hasFeat(static_cast<uint32_t>(activeFeat))) {
				const CombatFeatModifiers feat = getCombatFeatModifiers(activeFeat);
				if (ranged ? feat.ranged : feat.melee) {
					if (feat.extraAttacks == 0)
						featAttackMod += feat.attack;
					featDamageMod   += feat.damage;
					featThreatRange += feat.threatRange;
				}
			}
			// Weapon Focus / Specialization bonuses.
			if (rightWeapon || leftWeapon) {
				if (ranged) {
					if (_info.hasFeat(kFeatWeaponFocusBlaster) || _info.hasFeat(kFeatWeaponFocusBlasterRifle))				featAttackMod += 1;
					if (_info.hasFeat(kFeatWeaponSpecializationBlaster) || _info.hasFeat(kFeatWeaponSpecializationBlasterRifle))				featDamageMod += 2;
				}
				else if (hasLightsaberEquipped()) {
					if (_info.hasFeat(kFeatWeaponFocusLightsaber))				featAttackMod += 1;
					if (_info.hasFeat(kFeatWeaponSpecializationLightsaber))				featDamageMod += 2;
				}
				else {
					if (_info.hasFeat(kFeatWeaponFocusMelee))				featAttackMod += 1;
					if (_info.hasFeat(kFeatWeaponSpecializationMelee))				featDamageMod += 2;
				}
			}
			// Full attack bonus = BAB - iterative penalty + ability mod + feats.
			int weaponAttackBonus = 0;
			if (rightWeapon)
				weaponAttackBonus += rightWeapon->getAttackBonus();
			if (leftWeapon && !ranged)
				weaponAttackBonus += leftWeapon->getAttackBonus();

			int bab = getBAB();
			int attackBonus = (bab + babPenalty) + abMod + featAttackMod + _attackModifier + weaponAttackBonus;
			// Roll d20 (1..20 inclusive) vs target AC.
			int d20 = RNG.getNext(1, 21);
			int attackRoll = d20 + attackBonus;
			int targetAC   = targetCreature ? targetCreature->getAC() : 10;
			// --- Lightsaber Deflection (Blaster attacks only) ---
			if (ranged && targetCreature && targetCreature->hasLightsaberEquipped()) {
				// Rule: Opposed roll. If Deflect roll >= Attack roll, the bolt is deflected.
				int deflectD20 = RNG.getNext(1, 21);
				int deflectBonus = targetCreature->getBAB() + targetCreature->getAbilityModifier(kAbilityDexterity);
				if (targetCreature->getCreatureInfo().hasFeat(kFeatMasterJediDefense))			deflectBonus += 10;
				else
				if (targetCreature->getCreatureInfo().hasFeat(kFeatImprovedJediDefense))			deflectBonus += 6;
				int deflectTotal = deflectD20 + deflectBonus;
				if (deflectTotal >= attackRoll) {
					debugC(Common::kDebugEngineLogic, 1,			       "DEFLECTED: Blaster bolt from \"%s\" deflected by \"%s\" (Deflect %d vs Attack %d)",			       _tag.c_str(), targetCreature->getTag().c_str(), deflectTotal, attackRoll);
					// Trigger a deflect animation if possible
					targetCreature->playAnimation("g8g1", false, 0.4f);
					recordAttackResult(kAttackResultMiss, weaponUsed);
					// Quick block/deflect animation
					return;
				}
			}
			// Natural 1 always misses;
			// natural 20 always hits.
			bool hit = (d20 == 20) || (d20 != 1 && attackRoll >= targetAC);
			if (_assuredHit)
				hit = true;
			if (!hit) {
				debugC(Common::kDebugEngineLogic, 1,		       "Object \"%s\" missed \"%s\" (d20=%d bab=%d abMod=%d feat=%d effect=%d total=%d vs AC %d)",		       _tag.c_str(), target->getTag().c_str(),		       d20, bab + babPenalty, abMod, featAttackMod, _attackModifier, attackRoll, targetAC);
				recordAttackResult(kAttackResultMiss, weaponUsed);
				return;
			}
			// --- Critical hit system ---	// Threat on natural 20 (weapons may lower this, but 20 is the universal minimum).	// Critical Strike feat: threat on 19-20;
			// Improved: 18-20.
			int critThreat = 20 - featThreatRange;
			bool isThreat = (d20 >= critThreat);
			bool isCrit   = false;
			if (isThreat) {
				// Confirmation roll — same bonus, must also hit AC.
				int confirmD20   = RNG.getNext(1, 21);
				int confirmRoll  = confirmD20 + attackBonus;
				isCrit = (confirmD20 == 20) || (confirmD20 != 1 && confirmRoll >= targetAC);
			}
			// --- Damage calculation ---
			int damage;
			if (rightWeapon && leftWeapon)		damage = computeWeaponDamage(leftWeapon) + computeWeaponDamage(rightWeapon);
			else
			if (rightWeapon)		damage = computeWeaponDamage(rightWeapon);
			else
				// Unarmed: 1 + Str modifier (minimum 1 die).
				damage = 1 + getAbilityModifier(kAbilityStrength);
			// On a confirmed critical, double the weapon dice (not the fixed modifiers).
			if (isCrit) {
				int dieDamage;
				int modDamage;
				if (rightWeapon && leftWeapon) {
					int rMod = getAbilityModifier(kAbilityStrength);
					int lMod = getAbilityModifier(kAbilityStrength);
					dieDamage = damage - rMod - lMod;
					modDamage  = rMod + lMod;
				}
				else
				if (rightWeapon) {
					int mod = ranged ? getAbilityModifier(kAbilityDexterity)			                 : getAbilityModifier(kAbilityStrength);
					dieDamage = damage - mod;
					modDamage  = mod;
				}
				else {
					// Unarmed: the "1" is the die, Str mod is the modifier.
					int mod = getAbilityModifier(kAbilityStrength);
					dieDamage = damage - mod;
					modDamage  = mod;
				}
				// Double the dice portion only.
					damage = dieDamage * 2 + modDamage;
			}
			// Apply feat and effect damage bonuses.
			damage += featDamageMod + _damageModifier;
			// Sneak Attack (Scoundrel class feature): roll +1d6 per rank when the
			// target is flat-footed (not currently in combat), flanked, or
			// knocked down/stunned.
			bool targetVulnerable = targetCreature && (!targetCreature->isInCombat() ||
			                                            targetCreature->isFlankedBy(this, area) ||
			                                            targetCreature->hasEffect(kEffectStun) ||
			                                            targetCreature->hasEffect(kEffectKnockdown));
			if (hit && targetVulnerable) {
				// Count sneak attack ranks.
				int sneakRanks = 0;
				for (
				int rank = kFeatSneakAttack1;
				rank <= kFeatSneakAttack9;
				++rank) {
					if (_info.hasFeat(static_cast<uint32_t>(rank)))				++sneakRanks;
				}
				if (sneakRanks > 0) {
					int sneakDamage = 0;
					for (
					int i = 0;
					i < sneakRanks;
					++i)				sneakDamage += RNG.getNext(1, 7);
					// 1d6 per rank
					damage += sneakDamage;
					debugC(Common::kDebugEngineLogic, 1,			       "Sneak Attack x%d: +%d damage on \"%s\"",			       sneakRanks, sneakDamage, target->getTag().c_str());
				}
			}
			if (damage < 1)		damage = 1;
			int damageType = kDamageTypeBludgeoning;
			if (rightWeapon && rightWeapon->isRangedWeapon())
				damageType = kDamageTypeEnergy;
			else if (rightWeapon)
				damageType = kDamageTypeSlashing;
			if (targetCreature) {
				int immunity = targetCreature->getDamageImmunityPercent(damageType);
				if (immunity > 0)
					damage = damage - (damage * immunity) / 100;
				if (damage < 1)
					damage = 1;
				// Concealment: percent chance the attack is treated as a miss after the hit roll.
				if (targetCreature->getConcealment() > 0) {
					int concealRoll = RNG.getNext(1, 101);
					if (concealRoll <= targetCreature->getConcealment()) {
						recordAttackResult(kAttackResultMiss, weaponUsed);
						debugC(Common::kDebugEngineLogic, 1, "Attack concealed against \"%s\"", target->getTag().c_str());
						return;
					}
				}
				// Damage resistance, damage reduction and Force shields.
				damage = targetCreature->absorbDamage(damage, damageType);
				targetCreature->recordDamageTaken(damage, damageType, this);
			}
			if (_assuredHit)
				_assuredHit = false;
			recordAttackResult(isCrit ? kAttackResultCriticalHitSuccessful : kAttackResultHitSuccessful, weaponUsed);
			if (isCrit && _model)
				playSoundSetBark(kSoundSetCriticalHit);
			// Bashing a door or container: its Hardness soaks the blow; it breaks at 0 HP.
			if (Situated *situated = ObjectContainer::toSituated(target)) {
				situated->takeBashDamage(damage, this);
				if (situated->isDead())
					cancelCombat();
				return;
			}
			int hp    = target->getCurrentHitPoints() - damage;
			int minHp = target->getMinOneHitPoints() ? 1 : 0;
			if (hp < minHp)		hp = minHp;
			target->setCurrentHitPoints(hp);
			debugC(Common::kDebugEngineLogic, 1,	       "Object \"%s\" was %shit by \"%s\" (d20=%d bab=%d abMod=%d effect=%d total=%d vs AC %d), %d dmg, %d/%d HP",	       target->getTag().c_str(), isCrit ? "CRITICALLY " : "",	       _tag.c_str(), d20, bab + babPenalty, abMod, _attackModifier, attackRoll, targetAC,	       damage, hp, target->getMaxHitPoints());
			// A Min1HP target left at 1 HP is still alive: no kill bookkeeping.
			if (hp <= 0) {
				cancelCombat();
				if (targetCreature) {
					targetCreature->cancelCombat();
					targetCreature->setLastKiller(this);
					if (targetCreature->handleDeath()) {
						// Signal the module to award XP to the killer's party.				// We use userDefinedEvent 1007 (already fired by handleCreaturesDeath) — XP				// is awarded there via the module's updateXPOnKill() call.
					}
				}
			}
		}
		bool Creature::isDead()
		const {
			return _dead;
		}
		bool Creature::isXPAwarded()
		const {
			return _xpAwarded;
		}
		void Creature::markXPAwarded() {
			_xpAwarded = true;
		}
		bool Creature::handleDeath() {
			if (!_dead && _currentHitPoints <= 0) {
				_dead = true;
				_deathEventPending = true;
				_damagedEventPending = false;
				// The dead don't perceive, get attacked or end combat rounds.
				_attackedEventPending = false;
				_endRoundEventPending = false;
				_perceptionEvents.clear();
				if (!_lastKiller && _lastHostileActor)
					_lastKiller = _lastHostileActor;
				if (_model) {
					_model->clearDefaultAnimations();
					_model->addDefaultAnimation("dead", 100);
					_model->playAnimation("die", false);
					playSoundSetBark(kSoundSetDead, true);
				}
				else {
					warning("Creature::handleDeath(): \"%s\" has no model; applying logical death only", _tag.c_str());
				}
				stopLastingVisuals();
				return true;
			}
			return false;
		}
		void Creature::saveState(Aurora::GFF3WriterStruct &gff) const {
			Object::saveState(gff);
			gff.addByte("IsDead", _dead ? 1 : 0);
			gff.addDouble("XPosition", _position[0]);
			gff.addDouble("YPosition", _position[1]);
			gff.addDouble("ZPosition", _position[2]);
			float x, y, z, angle;
			getOrientation(x, y, z, angle);
			gff.addDouble("Facing", angle);
			gff.addSint32("FactionID", static_cast<int32_t>(getFaction()));
			gff.addByte("Min1HP", getMinOneHitPoints() ? 1 : 0);
		}
		void Creature::loadState(const Aurora::GFF3Struct &gff) {
			Object::loadState(gff);
			if (gff.hasField("XPosition"))
				setPosition(gff.getDouble("XPosition"), gff.getDouble("YPosition"), gff.getDouble("ZPosition"));
			if (gff.hasField("Facing"))
				setOrientation(0.0f, 0.0f, 1.0f, gff.getDouble("Facing"));
			// Scripts change factions (hostile on insult, surrender, ...).
			if (gff.hasField("FactionID"))
				setFaction(Faction(gff.getSint("FactionID")));
			// Scripts keep cutscene actors alive with SetMinOneHP.
			if (gff.hasField("Min1HP"))
				setMinOneHitPoints(gff.getBool("Min1HP"));
			// A restored creature already spawned once; don't run OnSpawn again.
			_spawnEventPending = false;
			if (gff.getBool("IsDead", false)) {
				// Dead in a previous visit: stay dead, without death events or XP.
				_currentHitPoints = 0;
				_dead = true;
				_xpAwarded = true;
				_deathEventPending = false;
				_damagedEventPending = false;
				if (_model) {
					_model->clearDefaultAnimations();
					_model->addDefaultAnimation("dead", 100);
				}
			}
		}
		void Creature::resurrect(int hitPoints) {
			if (!_dead)
				return;
			_dead = false;
			_deathEventPending = false;
			_damagedEventPending = false;
			setCurrentHitPoints(MAX(1, MIN(hitPoints, getMaxHitPoints())));
			if (_model) {
				_model->clearDefaultAnimations();
				setDefaultAnimations();
				playDefaultAnimation();
			}
		}
		bool Creature::consumeDeathEvent() {
			const bool pending = _deathEventPending;
			_deathEventPending = false;
			return pending;
		}
		bool Creature::consumeDamagedEvent() {
			const bool pending = _damagedEventPending;
			_damagedEventPending = false;
			return pending;
		}
		bool Creature::consumeSpawnEvent() {
			const bool pending = _spawnEventPending;
			_spawnEventPending = false;
			return pending;
		}
		bool Creature::consumeHeartbeatEvent() {
			const bool pending = _heartbeatEventPending;
			_heartbeatEventPending = false;
			return pending;
		}
		bool Creature::consumeEndRoundEvent() {
			const bool pending = _endRoundEventPending;
			_endRoundEventPending = false;
			return pending;
		}
		bool Creature::consumeAttackedEvent(Object *&attacker) {
			const bool pending = _attackedEventPending;
			attacker = _lastAttacker;
			_attackedEventPending = false;
			return pending;
		}
		bool Creature::consumePerceptionEvent(PerceptionEvent &event) {
			if (_perceptionEvents.empty())
				return false;
			event = _perceptionEvents.front();
			_perceptionEvents.erase(_perceptionEvents.begin());
			// GetLastPerceived & co. describe the event being dispatched.
			_lastPerceived           = event.object;
			_lastPerceptionSeen      = event.seen;
			_lastPerceptionVanished  = !event.seen;
			_lastPerceptionHeard     = event.seen;
			_lastPerceptionInaudible = !event.seen;
			return true;
		}
		Object *Creature::getLastPerceived()
		const {
			return _lastPerceived;
		}
		bool Creature::getLastPerceptionSeen()
		const {
			return _lastPerceptionSeen;
		}
		bool Creature::getLastPerceptionVanished()
		const {
			return _lastPerceptionVanished;
		}
		bool Creature::getLastPerceptionHeard()
		const {
			return _lastPerceptionHeard;
		}
		bool Creature::getLastPerceptionInaudible()
		const {
			return _lastPerceptionInaudible;
		}
		bool Creature::hasSeenObject(
		const Object *obj)
		const {
			return _seenObjects.count(const_cast<Object *>(obj)) > 0;
		}
		bool Creature::hasHeardObject(
		const Object *obj)
		const {
			return _heardObjects.count(const_cast<Object *>(obj)) > 0;
		}
		void Creature::handleObjectSeen(Object &object) {
			bool inserted = _seenObjects.insert(&object).second;
			if (inserted) {
				_lastPerceived           = &object;
				_lastPerceptionSeen      = true;
				_lastPerceptionVanished  = false;
				_lastPerceptionHeard     = false;
				_lastPerceptionInaudible = false;
				PerceptionEvent event;
				event.object = &object;
				event.seen = true;
				_perceptionEvents.push_back(event);
				debugC(Common::kDebugEngineLogic, 2,			"Creature \"%s\" have seen \"%s\"",			_tag.c_str(), object.getTag().c_str());
			}
		}
		void Creature::handleObjectVanished(Object &object) {
			size_t countErased = _seenObjects.erase(&object);
			if (countErased != 0) {
				_lastPerceived           = &object;
				_lastPerceptionSeen      = false;
				_lastPerceptionVanished  = true;
				_lastPerceptionHeard     = false;
				_lastPerceptionInaudible = false;
				PerceptionEvent event;
				event.object = &object;
				event.seen = false;
				_perceptionEvents.push_back(event);
				debugC(Common::kDebugEngineLogic, 2,			"Object \"%s\" have vanished from \"%s\"",			object.getTag().c_str(), _tag.c_str());
			}
		}
		void Creature::handleObjectHeard(Object &object) {
			bool inserted = _heardObjects.insert(&object).second;
			if (inserted) {
				_lastPerceived           = &object;
				_lastPerceptionSeen      = false;
				_lastPerceptionVanished  = false;
				_lastPerceptionHeard     = true;
				_lastPerceptionInaudible = false;
			}
		}
		void Creature::handleObjectInaudible(Object &object) {
			size_t countErased = _heardObjects.erase(&object);
			if (countErased != 0) {
				_lastPerceived           = &object;
				_lastPerceptionSeen      = false;
				_lastPerceptionVanished  = false;
				_lastPerceptionHeard     = false;
				_lastPerceptionInaudible = true;
			}
		}
		void Creature::playHeadAnimation(
		const Common::UString &anim,
		bool restart,
		float length,
		float speed) {
			if (!_model)		return;
			Graphics::Aurora::AnimationChannel *headChannel = 0;
			if (_modelType == "B" || _modelType == "P") {
				Graphics::Aurora::Model *head = _model->getAttachedModel("headhook");
				if (head)			headChannel = head->getAnimationChannel(Graphics::Aurora::kAnimationChannelAll);
			}
			else		headChannel = _model->getAnimationChannel(Graphics::Aurora::kAnimationChannelHead);
			if (headChannel)		headChannel->playAnimation(anim, restart, length, speed);
		}
		const Action *Creature::getCurrentAction()
		const {
			return _actions.getCurrent();
		}
		void Creature::clearActions() {
			_movePath.clear();
			_actions.clear();
		}
		void Creature::addAction(
		const Action &action) {
			_actions.add(action);
		}
		void Creature::popAction() {
			_actions.pop();
		}
		bool Creature::hasForcePower(uint32_t power) const {
			return _info.hasForcePower(power);
		}
		void Creature::performCutsceneAttack(Object *target,
		int flags, int damage, int animation) {
			if (!target || target->isDead())
				return;

			makeLookAt(target);
			Creature *targetCreature = ObjectContainer::toCreature(target);
			if (targetCreature)
				targetCreature->makeLookAt(this);

			// The animation the script asks for (an animations.2da row, as
			// KotOR.js plays it); else the attack of the weapon held.
			const Common::UString scripted = (animation >= 0) ? getAnimationNameById(animation) : "";
			if (!scripted.empty() && hasModelAnimation(scripted))
				playAnimation(scripted, true);
			else if (_model && (getWeaponAnimationNumber() != -1))
				playAttackAnimation();
			else
				playAnimation("attack1", false, 1.2f);

			if (flags & kCutsceneAttackForceHit) {
				if (targetCreature && (flags & kCutsceneAttackPlayReaction)) {
					int roll = RNG.getNext(0, 3);
					if (roll == 0)
						targetCreature->playAnimation("dodge", false, 0.5f);
					else if (roll == 1)
						targetCreature->playAnimation("g8a1", false, 0.6f);
					else
						targetCreature->playAnimation("v_flinch", false, 0.4f);
				}
			} else if (flags & kCutsceneAttackForceMiss) {
				if (targetCreature) {
					int roll = RNG.getNext(0, 2);
					if (roll == 0)
						targetCreature->playAnimation("g8a2", false, 0.8f);
					else
						targetCreature->playAnimation("dodge", false, 0.8f);
				}
			}
			if ((flags & kCutsceneAttackKnockback) || (flags & kCutsceneAttackFinishingBlow)) {
				if (targetCreature && !(flags & kCutsceneAttackNoDamage))
					targetCreature->playAnimation("die", false, 1.5f);
			}
			// The scripted damage lands with the blow, not when it was queued.
			if (targetCreature && damage > 0 && !(flags & kCutsceneAttackNoDamage)) {
				targetCreature->recordDamageTaken(damage, 0, this);
				targetCreature->applyEffect(Effect(kKotOREffectDamage, damage));
			}
		}
		void Creature::setDefaultAnimations() {
			if (!_model)		return;
			if (_modelType == "S" || _modelType == "L")		_model->addDefaultAnimation("cpause1", 100);
			else		_model->addDefaultAnimation("pause1", 100);
		}
		void Creature::reloadEquipment() {
			for (
			int i = static_cast<int>(kInventorySlotHead);
			i < static_cast<int>(kInventorySlotMAX);
			++i) {
				InventorySlot slot = InventorySlot(i);
				if (_info.isInventorySlotEquipped(slot))			addItemToEquipment(_info.getEquippedItem(slot), slot);
			}
		}
		bool Creature::addItemToEquipment(
		const Common::UString &tag, InventorySlot slot) {
			try {
				// Overwrite: companions rebuilt from a template and then given
				// saved state must not keep the template's gear.
				_equipment[slot] = std::make_unique<Item>(tag);
			}
			catch (Common::Exception &e) {
				e.add("Failed to load item \"%s\"", tag.c_str());
				Common::printException(e, "WARNING: ");
				return false;
			}
			return true;
		}
		int Creature::getWeaponAnimationNumber()
		const {
			const Item *rightWeapon = getEquipedItem(kInventorySlotRightWeapon);
			const Item *leftWeapon = getEquipedItem(kInventorySlotLeftWeapon);
			if (rightWeapon && !leftWeapon) {
				switch (rightWeapon->getWeaponWield()) {
					case kWeaponWieldBaton:
					return 1;
					case kWeaponWieldSword:
					return 2;
					case kWeaponWieldStaff:
					return 3;
					case kWeaponWieldPistol:
					return 5;
					case kWeaponWieldRifle:
					return 7;
					default:
					return -1;
				}
			}
			if (rightWeapon && leftWeapon) {
				switch (rightWeapon->getWeaponWield()) {
					case kWeaponWieldSword:
					return 4;
					case kWeaponWieldPistol:
					return 6;
					default:
					return -1;
				}
			}
			return 8;
		}
		int Creature::computeWeaponDamage(
		const Item *weapon)
		const {
			int result = 0;
			Ability ability = weapon->isRangedWeapon() ? kAbilityDexterity : kAbilityStrength;
			int mod = getAbilityModifier(ability);
			for (
			int i = 0;
			i < weapon->getNumDice();
			++i) {
				result += RNG.getNext(1, weapon->getDieToRoll() + 1);
			}
			result += mod;
			result += weapon->getEnhancementBonus();
			result += weapon->getDamageBonus();
			return (result < 1) ? 1 : result;
		}
		bool Creature::hasLightsaberEquipped()
		const {
			const Item *right = getEquipedItem(kInventorySlotRightWeapon);
			const Item *left = getEquipedItem(kInventorySlotLeftWeapon);
			auto isLasso = [](
			const Item *item) {
				if (!item)
				return false;
				int base = item->getBaseItem();
				return base == kBaseItemLightsaber || 		       base == kBaseItemDoubleLightsaber || 		       base == kBaseItemShortLightsaber;
			}
			;
			return _lightsaberPowered && (isLasso(right) || isLasso(left));
		}
		void Creature::setLightsaberPowered(bool powered) {
			_lightsaberPowered = powered;
		}
		bool Creature::isLightsaberPowered() const {
			return _lightsaberPowered;
		}
		void Creature::setBlockingDoor(Object *door) {
			_blockingDoor = door;
		}
		Object *Creature::getBlockingDoor() const {
			return _blockingDoor;
		}
		CombatFeatModifiers getCombatFeatModifiers(int feat) {
			// Penalties as KotOR.js TalentFeat applies them (attack and Defense for
			// one round); Power Attack/Blast damage as KotOR.js CombatAttackData.
			CombatFeatModifiers mods;
			switch (feat) {
				case kFeatFlurry:
				case kFeatImprovedFlurry:
				case kFeatMasterFlurry:
				case kFeatRapidShot:
				case kFeatImprovedRapidShot:
				case kFeatMasterRapidShot: {
					const bool flurry = (feat == kFeatFlurry) || (feat == kFeatImprovedFlurry) || (feat == kFeatMasterFlurry);
					const int penalty = ((feat == kFeatFlurry) || (feat == kFeatRapidShot)) ? 4 :
					                    (((feat == kFeatImprovedFlurry) || (feat == kFeatImprovedRapidShot)) ? 2 : 1);
					mods.melee        = flurry;
					mods.ranged       = !flurry;
					mods.attack       = -penalty;
					mods.armorClass   = -penalty;
					mods.extraAttacks = 1;
					break;
				}
				case kFeatPowerAttack:
				case kFeatImprovedPowerAttack:
				case kFeatMasterPowerAttack:
				case kFeatPowerBlast:
				case kFeatImprovedPowerBlast:
				case kFeatMasterPowerBlast: {
					const bool melee = (feat == kFeatPowerAttack) || (feat == kFeatImprovedPowerAttack) ||
					                   (feat == kFeatMasterPowerAttack);
					mods.melee  = melee;
					mods.ranged = !melee;
					mods.attack = -3;
					mods.damage = ((feat == kFeatPowerAttack) || (feat == kFeatPowerBlast)) ? 5 :
					              (((feat == kFeatImprovedPowerAttack) || (feat == kFeatImprovedPowerBlast)) ? 8 : 10);
					break;
				}
				case kFeatCriticalStrike:
				case kFeatImprovedCriticalStrike:
				case kFeatMasterCriticalStrike:
					mods.melee       = true;
					mods.armorClass  = -5;
					mods.threatRange = (feat == kFeatCriticalStrike) ? 1 : ((feat == kFeatImprovedCriticalStrike) ? 2 : 3);
					break;
				case kFeatSniperShot:
				case kFeatImprovedSniperShot:
				case kFeatMasterSniperShot:
					// The ranged Critical Strike.
					mods.ranged      = true;
					mods.threatRange = (feat == kFeatSniperShot) ? 1 : ((feat == kFeatImprovedSniperShot) ? 2 : 3);
					break;
				default:
					break;
			}
			return mods;
		}

		void Creature::think(Area *area) {
			if (_aiArchetype == kAIArchetypeNone || isPC() || isDead() || !area)
				return;
			if (_aiCooldown > 0.0f)
				return;
			Creature *target = area->findNearestEnemy(this);
			if (!target)		return;
			switch (_aiArchetype) {
				case kAIArchetypeBeastMelee:
					// Aggressive rush towards the nearest hostile.
					_actions.clear();
				_actions.add(Action(kActionAttackObject, target));
				_aiCooldown = 2.0f;
				break;
				case kAIArchetypeBeastPoison:
					// Beasts that prioritize applying poison status effects (e.g., Kinrath).
					_actions.clear();
					_actions.add(Action(kActionAttackObject, target));
					// TODO: Add Poison Effect chance
					_aiCooldown = 1.5f;
				break;
				case kAIArchetypeTacticalHumanoid:
					// Standard Mandalorian or mercenary AI.
					_actions.clear();
				_actions.add(Action(kActionAttackObject, target));
				_aiCooldown = 2.0f;
				break;
				default:			break;
			}
		}
		void Creature::removeEffect(
		const ActiveEffect &effect) {
			// Undo the persistent AC penalties applied in applyEffect() so that
			// knockdown/stun/confusion do not permanently (and cumulatively)
			// weaken the creature once the effect wears off.
			switch (effect.type) {
				case kEffectKnockdown:
					adjustArmorClassModifier(4);
					break;
				case kEffectStun:
					adjustArmorClassModifier(2);
					break;
				case kEffectConfusion:
					adjustArmorClassModifier(4);
					break;
				case kEffectModifier:
					adjustEffectModifiers(effect, -1);
					break;
				case kEffectVFX:
					stopVisualEffect(effect.value);
					break;
				default:
					break;
			}
		}
		void Creature::applyEffect(EffectType type,
		float duration,
		int value,
		int spellId) {
			ActiveEffect e;
			e.type = type;
			e.duration = duration;
			e.value = value;
			e.spellId = spellId;
			_effects.push_back(e);
			applyEffect(e);
		}
		void Creature::updateEffects(
		float dt) {
			for (auto it = _effects.begin();
			it != _effects.end();
			) {
				it->duration -= dt;
				if (it->type == kEffectPoison) {
					// Periodic poison damage: value is damage per second, applied in
					// whole-second ticks (truncating value * dt lost every tick).
					it->nextTick -= dt;
					while (it->nextTick <= 0.0f) {
						it->nextTick += 1.0f;
						if (_currentHitPoints > 1 && it->value > 0) {
							// Poison wears a creature down but doesn't kill it.
							const int dealt = MIN(it->value, _currentHitPoints - 1);
							recordDamageTaken(dealt, 0, _lastHostileActor);
							_currentHitPoints -= dealt;
						}
					}
				}
				if (it->duration <= 0.0f) {
					removeEffect(*it);
					it = _effects.erase(it);
				}
				else {
					++it;
				}
			}
			updateLastingVisuals(dt);
		}

		void Creature::updateLastingVisuals(float dt) {
			for (auto v = _lastingVisuals.begin(); v != _lastingVisuals.end(); ) {
				v->timeLeft -= dt;

				// A linked visual ends with the last of the effects it's linked to
				bool linkAlive = false;
				if (v->linkId != 0)
					for (const ActiveEffect &e : _effects)
						if (e.linkId == v->linkId)
							linkAlive = true;

				if (linkAlive)
					v->linkSeen = true;

				if ((v->timeLeft > 0.0f) && (linkAlive || !v->linkSeen)) {
					++v;
					continue;
				}

				stopVisualEffect(v->id);
				v = _lastingVisuals.erase(v);
			}
		}

		void Creature::stopLastingVisuals() {
			for (const LastingVisual &visual : _lastingVisuals)
				stopVisualEffect(visual.id);

			_lastingVisuals.clear();
		}
		bool Creature::hasEffect(EffectType type)
		const {
			for (
			const auto &e : _effects) {
				if (e.type == type)
				return true;
			}
			return false;
		}

		bool Creature::isIncapacitated() const {
			return hasEffect(kEffectStun) || hasEffect(kEffectKnockdown);
		}

		bool Creature::hasSpellEffect(int spellId) const {
			if (spellId < 0)
				return false;

			for (const auto &e : _effects) {
				if (e.spellId == spellId)
					return true;
			}
			return false;
		}
		void Creature::update(
		float dt) {
			if (isDead())		return;
			updateEffects(dt);
			if (_aiCooldown > 0.0f)		_aiCooldown -= dt;
		}

		// --- Sounds and visual effects ---

		/** Footsteps carry this far, in metres. */
		static const float kFootstepMinDistance = 1.0f;
		static const float kFootstepMaxDistance = 12.0f;
		/** Weapons and voices carry this far, in metres. */
		static const float kCombatSoundMinDistance = 3.0f;
		static const float kCombatSoundMaxDistance = 30.0f;

		void Creature::updatePresentation(float dt, Area *area) {
			for (auto s = _delayedSounds.begin(); s != _delayedSounds.end(); ) {
				s->delay -= dt;
				if (s->delay > 0.0f) {
					++s;
					continue;
				}

				playSoundHere(s->sound, kCombatSoundMinDistance, kCombatSoundMaxDistance);
				s = _delayedSounds.erase(s);
			}

			_visualEffects.update(_model.get(), dt);

			if (!_model)
				return;

			std::vector<Graphics::Aurora::Model::FiredAnimationEvent> events;
			_model->takeAnimationEvents(events);

			for (const auto &event : events)
				handleAnimationEvent(event.name, area);
		}

		void Creature::loadAppearanceSounds() {
			if (_appearanceSoundsFor == _appearance)
				return;

			_appearanceSoundsFor = _appearance;
			_footstepType = -1;
			_sizeCategory = 3;

			try {
				const Aurora::TwoDARow &row = TwoDAReg.get2DA("appearance").getRow(_appearance);

				if (!row.empty("footsteptype"))
					_footstepType = row.getInt("footsteptype");
				if (!row.empty("sizecategory"))
					_sizeCategory = row.getInt("sizecategory");
			} catch (...) {
				Common::exceptionDispatcherWarning("Failed to read the sounds of appearance %u", _appearance);
			}
		}

		const Item *Creature::getSoundWeapon() const {
			static const InventorySlot kSlots[] = {
				kInventorySlotRightWeapon, kInventorySlotLeftWeapon,
				kInventorySlotCWeaponR, kInventorySlotCWeaponL, kInventorySlotCWeaponB
			};

			for (InventorySlot slot : kSlots) {
				const Item *item = getEquipedItem(slot);
				if (item)
					return item;
			}

			return nullptr;
		}

		/** What a weapon hitting @p target sounds like hitting. */
		static WeaponHitMaterial getHitMaterial(const Object *target) {
			if (!target)
				return kWeaponHitLeather;

			const Creature *creature = dynamic_cast<const Creature *>(target);
			if (!creature)
				return kWeaponHitMetal; // Doors and containers

			if (creature->getRace() == kRaceDroid)
				return kWeaponHitMetal;

			return kWeaponHitLeather;
		}

		void Creature::playSoundHere(const Common::UString &sound, float minDistance, float maxDistance) {
			if (sound.empty())
				return;

			float x, y, z;
			getPosition(x, y, z);

			playSoundAt(sound, x, y, z, Sound::kSoundTypeSFX, minDistance, maxDistance);
		}

		void Creature::handleAnimationEvent(const Common::UString &event, Area *area) {
			const AnimationEventSound sound = getAnimationEventSound(event);
			if (sound == kAnimationEventSoundNone) {
				debugC(Common::kDebugEngineLogic, 3, "Creature \"%s\": animation event \"%s\"",
				       _tag.c_str(), event.c_str());
				return;
			}

			float x, y, z;
			getPosition(x, y, z);

			try {
				if (sound == kAnimationEventSoundFootstep) {
					loadAppearanceSounds();
					if (_footstepType < 0)
						return;

					const uint32_t surface = area ? area->getSurfaceAt(x, y) : UINT32_MAX;
					const Common::UString footstep = getFootstepSound(TwoDAReg.get2DA("footstepsounds"),
					                                                  _footstepType, surface, RNG.getNext(0, 3));

					playSoundAt(footstep, x, y, z, Sound::kSoundTypeSFX, kFootstepMinDistance, kFootstepMaxDistance);
					return;
				}

				const Item *weapon = getSoundWeapon();
				if (!weapon)
					return;

				const int row = getWeaponSoundRow(TwoDAReg.get2DA("baseitems"), weapon->getBaseItem());
				if (row < 0)
					return;

				const Object *target = _attemptedAttackTarget ? _attemptedAttackTarget : _attackTarget;
				const int variant = RNG.getNext(0, MAX(getWeaponSoundVariantCount(sound), 1));

				const Common::UString weaponSound = getWeaponSound(TwoDAReg.get2DA("weaponsounds"), row, sound,
				                                                   getHitMaterial(target), variant);

				playSoundAt(weaponSound, x, y, z, Sound::kSoundTypeSFX, kCombatSoundMinDistance, kCombatSoundMaxDistance);

			} catch (...) {
				Common::exceptionDispatcherWarning("Failed to play the sound of animation event \"%s\"", event.c_str());
			}
		}

		bool Creature::playSoundSetBark(SoundSetEntry entry, bool interrupt) {
			if (!_model)
				return false;

			if (!interrupt && SoundMan.isPlaying(_barkSound))
				return false;

			loadSSF();
			if (!_ssf)
				return false;

			const uint32_t strRef = _ssf->getStrRef(entry);
			if (strRef == Aurora::kStrRefInvalid)
				return false;

			const Common::UString &sound = TalkMan.getSoundResRef(strRef);
			if (sound.empty())
				return false;

			if (interrupt)
				SoundMan.stopChannel(_barkSound);

			float x, y, z;
			getPosition(x, y, z);

			// Raise the voice to head height
			_barkSound = playSoundAt(sound, x, y, z + 1.5f, Sound::kSoundTypeVoice,
			                         kCombatSoundMinDistance, kCombatSoundMaxDistance);
			return true;
		}

		bool Creature::playRandomSoundSetBark(SoundSetEntry first, int count, bool interrupt) {
			if (!_model || (count <= 0))
				return false;

			if (!interrupt && SoundMan.isPlaying(_barkSound))
				return false;

			// Not every soundset has every entry: try them all, starting at random
			const int start = RNG.getNext(0, count);
			for (int i = 0; i < count; i++) {
				const SoundSetEntry entry = static_cast<SoundSetEntry>(first + ((start + i) % count));
				if (playSoundSetBark(entry, interrupt))
					return true;
			}

			return false;
		}

		void Creature::playVisualEffect(int id, bool lasting) {
			if (!_model)
				return;

			VisualEffectInfo effect;
			try {
				if (!loadVisualEffect(TwoDAReg.get2DA("visualeffects"), id, effect))
					return;
			} catch (...) {
				Common::exceptionDispatcherWarning("Failed to read visual effect %d", id);
				return;
			}

			if (effect.isBeam()) {
				debugC(Common::kDebugEngineLogic, 2, "Creature \"%s\": beam visual effect %d is not shown",
				       _tag.c_str(), id);
				return;
			}

			loadAppearanceSounds();

			_visualEffects.playEffect(*_model, effect, _sizeCategory, lasting && effect.isDuration());
			playSoundHere(effect.impactSound, kCombatSoundMinDistance, kCombatSoundMaxDistance);
		}

		void Creature::stopVisualEffect(int id) {
			_visualEffects.stopEffect(_model.get(), id);
		}

		void Creature::playSpellVisual(const Common::UString &visual, VisualEffectHook hook, float delay) {
			if (visual.empty())
				return;

			const char *text = visual.c_str();
			if (std::strspn(text, "0123456789") == std::strlen(text)) {
				// A visualeffects.2da row
				if (delay <= 0.0f)
					playVisualEffect(std::atoi(text));
				return;
			}

			_visualEffects.playModel(*_model, visual, hook, delay);
		}

		void Creature::playSpellCastVisuals(int spell) {
			if (!_model)
				return;

			SpellCastVisuals visuals;
			try {
				if (!loadSpellCastVisuals(TwoDAReg.get2DA("spells"), spell, visuals) || visuals.empty())
					return;
			} catch (...) {
				Common::exceptionDispatcherWarning("Failed to read the casting visuals of spell %d", spell);
				return;
			}

			// Powers resolve at once: show the conjuring, then the casting a moment later
			const float castDelay = MIN(visuals.conjureTime, 1.5f);

			playSpellVisual(visuals.conjureHand  , kVisualEffectHookHand, 0.0f);
			playSpellVisual(visuals.conjureHead  , kVisualEffectHookHead, 0.0f);
			playSpellVisual(visuals.conjureGround, kVisualEffectHookRoot, 0.0f);

			playSpellVisual(visuals.castHand  , kVisualEffectHookHand, castDelay);
			playSpellVisual(visuals.castHead  , kVisualEffectHookHead, castDelay);
			playSpellVisual(visuals.castGround, kVisualEffectHookRoot, castDelay);

			playSoundHere(visuals.conjureSound, kCombatSoundMinDistance, kCombatSoundMaxDistance);

			const Common::UString &voice = (getGender() == kGenderFemale) ?
				visuals.conjureSoundFemale : visuals.conjureSoundMale;
			playSoundHere(voice, kCombatSoundMinDistance, kCombatSoundMaxDistance);

			if (!visuals.castSound.empty()) {
				if (castDelay <= 0.0f) {
					playSoundHere(visuals.castSound, kCombatSoundMinDistance, kCombatSoundMaxDistance);
				} else {
					DelayedSound delayed;
					delayed.delay = castDelay;
					delayed.sound = visuals.castSound;
					_delayedSounds.push_back(delayed);
				}
			}
		}

		void Creature::equipMostDamagingMelee() {
			// Simple heuristic: search inventory for the melee weapon with the
			// highest maximum damage potential.
			Item *bestItem = nullptr;
			int maxDmg = -1;

			for (const auto &it : _info.getInventory().getItems()) {
				Item *item = findInventoryItemByTag(it.first);
				if (!item || item->isRangedWeapon())
					continue;

				int potential = item->getDieToRoll() * item->getNumDice();
				if (potential > maxDmg) {
					maxDmg = potential;
					bestItem = item;
				}
			}

			if (bestItem)
				equipItem(bestItem->getTag(), kInventorySlotRightWeapon);
		}

		void Creature::equipMostDamagingRanged() {
			Item *bestItem = nullptr;
			int maxDmg = -1;

			for (const auto &it : _info.getInventory().getItems()) {
				Item *item = findInventoryItemByTag(it.first);
				if (!item || !item->isRangedWeapon())
					continue;

				int potential = item->getDieToRoll() * item->getNumDice();
				if (potential > maxDmg) {
					maxDmg = potential;
					bestItem = item;
				}
			}

			if (bestItem)
				equipItem(bestItem->getTag(), kInventorySlotRightWeapon);
		}

		void Creature::setAppearanceType(uint32_t appearance) {
			_appearance = appearance;
			// In a full engine this would trigger a model reload/rebind.
			// For script parity, we update the state so subsequent calls
			// to GetAppearanceType return the correct disguised value.
		}

		void Creature::setStealthMode(bool stealth) {
			_stealthMode = stealth;
			// Visual changes (transparency) would go here.
		}

} // End of namespace KotORBase

} // End of namespace Engines