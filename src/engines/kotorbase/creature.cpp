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

#include <cassert>

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
#include "src/aurora/locstring.h"
#include "src/aurora/resman.h"

#include "src/graphics/aurora/modelnode.h"
#include "src/graphics/aurora/model.h"
#include "src/graphics/aurora/animationchannel.h"

#include "src/engines/aurora/util.h"
#include "src/engines/aurora/model.h"

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/creaturesearch.h"

#include "src/engines/kotorbase/gui/chargeninfo.h"

namespace Engines {

namespace KotORBase {

Creature::Creature(const Common::UString &resRef) :
		Object(kObjectTypeCreature),
		_commandable(true),
		_walkRate(0.0f),
		_runRate(0.0f) {

	init();

	std::unique_ptr<Aurora::GFF3File> utc(loadOptionalGFF3(resRef, Aurora::kFileTypeUTC));
	if (!utc)
		throw Common::Exception("Creature \"%s\" has no blueprint", resRef.c_str());

	load(utc->getTopLevel());
}

Creature::Creature(const Aurora::GFF3Struct &creature) :
		Object(kObjectTypeCreature),
		_commandable(true),
		_walkRate(0.0f),
		_runRate(0.0f) {

	init();
	load(creature);
}

Creature::Creature() :
		Object(kObjectTypeCreature),
		_commandable(true),
		_walkRate(0.0f),
		_runRate(0.0f) {

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
	if (_visible)
		return;

	_visible = true;

	if (_model)
		_model->show();
}

void Creature::hide() {
	if (!_visible)
		return;

	if (_model)
		_model->hide();

	_visible = false;
}

bool Creature::isVisible() const {
	return _visible;
}

bool Creature::isPC() const {
	return _isPC;
}

bool Creature::isPartyMember() const {
	return _isPC;
}

bool Creature::matchSearchCriteria(const Object *UNUSED(target), const CreatureSearchCriteria &UNUSED(criteria)) const {
	// TODO: Implement pattern matching
	return false;
}

bool Creature::isCommandable() const {
	return _commandable;
}

void Creature::setUsable(bool usable) {
	Object::setUsable(usable);
	if (_model)
		_model->setClickable(isClickable());
}

void Creature::setCommandable(bool commandable) {
	_commandable = commandable;
}

Gender Creature::getGender() const {
	return _gender;
}

int Creature::getLevel(const Class &c) const {
	return _info.getClassLevel(c);
}

int Creature::getLevelByPosition(int position) const {
	return _info.getLevelByPosition(position);
}

Class Creature::getClassByPosition(int position) const {
	return _info.getClassByPosition(position);
}

Race Creature::getRace() const {
	return _race;
}

SubRace Creature::getSubRace() const {
	return _subRace;
}

float Creature::getWalkRate() const {
	return _walkRate;
}

float Creature::getRunRate() const {
	return _runRate;
}

int Creature::getSkillRank(Skill skill) {
	const int baseRank = _info.getSkillRank(skill);
	const int modified = baseRank + getSkillModifier(skill);
	return (modified < 0) ? 0 : modified;
}

void Creature::setForcePoints(int fp) {
	_info.setForcePoints(MAX(0, fp));
}

void Creature::setMaxForcePoints(int fp) {
	_info.setMaxForcePoints(MAX(0, fp));
}

int Creature::getForcePoints() const {
	return _info.getForcePoints();
}

int Creature::getMaxForcePoints() const {
	return _info.getMaxForcePoints();
}

int Creature::computeMaxForcePoints() const {
	int totalFP = 0;
	int jediLevel = 0;

	for (int i = 0; i < _info.getNumClasses(); ++i) {
		int lv = _info.getLevelByPosition(i);
		KotORBase::Class pcClass = _info.getClassByPosition(i);

		int perLevel = 0;
		switch (pcClass) {
			case kClassJediGuardian:  perLevel = 4; break;
			case kClassJediSentinel:  perLevel = 6; break;
			case kClassJediConsular:  perLevel = 8; break;
			// Prestige classes (mostly follow the same patterns)
			case kClassJediWeaponMaster: perLevel = 4; break;
			case kClassJediWatchMan:     perLevel = 6; break;
			case kClassJediMaster:       perLevel = 8; break;
			case kClassSithMarauder:     perLevel = 4; break;
			case kClassSithAssassin:     perLevel = 6; break;
			case kClassSithLord:         perLevel = 8; break;
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

void Creature::setPosition(float x, float y, float z) {
	Object::setPosition(x, y, z);
	Object::getPosition(x, y, z);

	if (_model)
		_model->setPosition(x, y, z);
}

void Creature::setOrientation(float x, float y, float z, float angle) {
	Object::setOrientation(x, y, z, angle);
	Object::getOrientation(x, y, z, angle);

	if (_model)
		_model->setOrientation(x, y, z, angle);
}

void Creature::load(const Aurora::GFF3Struct &creature) {
	_templateResRef = creature.getString("TemplateResRef");

	std::unique_ptr<Aurora::GFF3File> utc;
	if (!_templateResRef.empty())
		utc.reset(loadOptionalGFF3(_templateResRef, Aurora::kFileTypeUTC, MKTAG('U', 'T', 'C', ' ')));

	load(creature, utc ? &utc->getTopLevel() : 0);

	if (!utc)
		warning("Creature \"%s\" has no blueprint", _tag.c_str());
}

void Creature::load(const Aurora::GFF3Struct &instance, const Aurora::GFF3Struct *blueprint) {
	_info = CreatureInfo(instance);

	// General properties

	if (blueprint)
		loadProperties(*blueprint);  // Blueprint
	loadProperties(instance, false); // Instance


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

void Creature::loadProperties(const Aurora::GFF3Struct &gff, bool clearScripts) {
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
}

void Creature::loadPortrait(const Aurora::GFF3Struct &gff) {
	uint32_t portraitID = gff.getUint("PortraitId");
	if (portraitID != 0) {
		const Aurora::TwoDAFile &twoda = TwoDAReg.get2DA("portraits");

		Common::UString portrait = twoda.getRow(portraitID).getString("BaseResRef");
		if (!portrait.empty()) {
			if (portrait.beginsWith("po_"))
				_portrait = portrait;
			else
				_portrait = "po_" + portrait;
		}
	}

	_portrait = gff.getString("Portrait", _portrait);
}

void Creature::loadEquipment(const Aurora::GFF3Struct &gff) {
	if (!gff.hasField("Equip_ItemList"))
		return;

	for (const auto &i : gff.getList("Equip_ItemList")) {
		InventorySlot slot = InventorySlot(static_cast<int>(std::log2f(i->getID())));
		Common::UString tag = i->getString("EquippedRes");
		equipItem(tag, slot, false);
	}
}

void Creature::loadAbilities(const Aurora::GFF3Struct &gff) {
	if (gff.hasField("Str"))
		_info.setAbilityScore(kAbilityStrength, gff.getUint("Str"));
	if (gff.hasField("Dex"))
		_info.setAbilityScore(kAbilityDexterity, gff.getUint("Dex"));
	if (gff.hasField("Con"))
		_info.setAbilityScore(kAbilityConstitution, gff.getUint("Con"));
	if (gff.hasField("Int"))
		_info.setAbilityScore(kAbilityIntelligence, gff.getUint("Int"));
	if (gff.hasField("Wis"))
		_info.setAbilityScore(kAbilityWisdom, gff.getUint("Wis"));
	if (gff.hasField("Cha"))
		_info.setAbilityScore(kAbilityCharisma, gff.getUint("Cha"));
}

void Creature::getModelState(uint32_t &state, uint8_t &textureVariation) {
	state = 'a';
	textureVariation = 1;

	if (_info.isInventorySlotEquipped(kInventorySlotBody)) {
		Item *item = _equipment[kInventorySlotBody].get();
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
		if (!ResMan.hasResource(parts.bodyTexture))
			parts.bodyTexture = appearance.getString(Common::UString("tex") + state) + "01";

	} else {
		parts.body = appearance.getString("race");
		parts.bodyTexture = appearance.getString("racetex");
	}

	if ((_modelType == "B") || (_modelType == "P")) {
		const int headNormalID = appearance.getInt("normalhead");
		const int headBackupID = appearance.getInt("backuphead");

		const Aurora::TwoDAFile &heads = TwoDAReg.get2DA("heads");

		if      (headNormalID >= 0)
			parts.head = heads.getRow(headNormalID).getString("head");
		else if (headBackupID >= 0)
			parts.head = heads.getRow(headBackupID).getString("head");
	}

	loadMovementRate(appearance.getString("moverate"));
}

void Creature::loadBody(PartModels &parts) {
	// Model "P_BastilaBB" has broken animations. Replace it with the
	// correct one.
	if (Common::String::equalsIgnoreCase(parts.body.c_str(), "P_BastilaBB"))
		parts.body = "P_BastilaBB02";

	GfxMan.lockFrame();
	_model.reset(loadModelObject(parts.body, parts.bodyTexture));
	GfxMan.unlockFrame();

	if (!_model)
		return;

	_ids.clear();
	_ids.push_back(_model->getID());

	_model->setTag(_tag);
	_model->setClickable(isClickable());

	if (_modelType != "B" && _modelType != "P")
		_model->addAnimationChannel(Graphics::Aurora::kAnimationChannelHead);
}

void Creature::loadHead(PartModels &parts) {
	if (!_model || parts.head.empty())
		return;

	_headModel = loadModelObject(parts.head);
	if (!_headModel)
		return;

	GfxMan.lockFrame();
	_model->attachModel("headhook", _headModel);
	GfxMan.unlockFrame();
}

void Creature::loadMovementRate(const Common::UString &name) {
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
	} else {
		getPartModels(parts, state, textureVariation);
		if ((_modelType == "P") || parts.body.empty()) {
			warning("TODO: Model \"%s\": ModelType \"%s\" (\"%s\")",
			        _tag.c_str(), _modelType.c_str(), parts.body.c_str());

			return;
		}
	}

	loadBody(parts);
	loadHead(parts);

	if (!_model)
		return;

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
		case kInventorySlotLeftWeapon:
			hookNode = "lhand";
			break;
		case kInventorySlotRightWeapon:
			hookNode = "rhand";
			break;
		default:
			throw Common::Exception("Unsupported equip slot");
	}

	if (!_model->hasNode(hookNode)) {
		warning("Creature::attachWeaponModel(): Model \"%s\" does not have node \"%s\"",
		        _model->getName().c_str(), hookNode.c_str());

		return;
	}

	Graphics::Aurora::Model *weaponModel = 0;

	if (_info.isInventorySlotEquipped(slot)) {
		Item *item = _equipment[slot].get();
		weaponModel = loadModelObject(item->getModelName());
	}

	GfxMan.lockFrame();
	_model->attachModel(hookNode, weaponModel);
	GfxMan.unlockFrame();
}

void Creature::initAsFakePC() {
	_name = "Fakoo McFakeston";
	_tag  = Common::String::format("[PC: %s]", _name.c_str());

	_isPC = true;
}

void Creature::initAsPC(const CharacterGenerationInfo &chargenInfo, const CreatureInfo &info) {
	_name = chargenInfo.getName();
	_usable = false;
	_isPC = true;

	_race = kRaceHuman;
	_subRace = kSubRaceNone;

	_gender = chargenInfo.getGender();

	switch (_gender) {
		case kGenderMale:
		case kGenderFemale:
			break;
		default:
			throw Common::Exception("Unknown gender");
	}

	_info = info;

	_skin = chargenInfo.getSkin();
	_face = chargenInfo.getFace();

	_minOneHitPoint = true;

	// Compute starting max HP: class hit die (max value) + Constitution modifier.
	// The fallback of 6 matches the Scoundrel hit die and is also the minimum
	// d6 that any KotOR class uses.
	static const int kDefaultHitDie = 6;
	int hitDie = kDefaultHitDie;
	try {
		const Aurora::TwoDAFile &classes = TwoDAReg.get2DA("classes");
		Common::UString label;
		switch (chargenInfo.getClass()) {
			case kClassSoldier:   label = "Soldier";   break;
			case kClassScout:     label = "Scout";     break;
			case kClassScoundrel: label = "Scoundrel"; break;
			default: break;
		}
		if (!label.empty())
			hitDie = classes.getRow("label", label).getInt("hitdie");
	} catch (...) {
		// Keep the default if the 2DA lookup fails.
	}

	int conMod = _info.getAbilityModifier(kAbilityConstitution);
	int hp = hitDie + conMod;
	if (hp < 1)
		hp = 1;
	_currentHitPoints = _maxHitPoints = hp;

	setMaxForcePoints(computeMaxForcePoints());
	setForcePoints(getMaxForcePoints());

	reloadEquipment();
	loadEquippedModel();
}

const Common::UString &Creature::getCursor() const {
	static Common::UString talkCursor("talk");
	static Common::UString killCursor("kill");
	static Common::UString useCursor("use");

	if (!isEnemy())
		return talkCursor;

	return isDead() ? useCursor : killCursor;
}

void Creature::highlight(bool enabled) {
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

const Common::UString &Creature::getConversation() const {
	return _conversation;
}

CreatureInfo &Creature::getCreatureInfo() {
	return _info;
}

const CreatureInfo &Creature::getCreatureInfo() const {
	return _info;
}

bool Creature::hasFeat(uint32_t feat) const {
	return _info.hasFeat(feat);
}

float Creature::getCameraHeight() const {
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

void Creature::equipItem(Common::UString tag, InventorySlot slot, bool updateModel) {
	equipItem(tag, slot, _info, updateModel);
}

void Creature::equipItem(Common::UString tag, InventorySlot slot, CreatureInfo &invOwner, bool updateModel) {
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

	if (!updateModel)
		return;

	switch (slot) {
		case kInventorySlotBody:
			loadEquippedModel();
			break;
		case kInventorySlotLeftWeapon:
		case kInventorySlotRightWeapon:
			attachWeaponModel(slot);
			break;
		default:
			break;
	}
}

Inventory &Creature::getInventory() {
	return _info.getInventory();
}

Item *Creature::getEquipedItem(InventorySlot slot) const {
	if (!_info.isInventorySlotEquipped(slot))
		return nullptr;

	return _equipment.find(slot)->second.get();
}

float Creature::getMaxAttackRange() const {
	const Item *rightWeapon = getEquipedItem(kInventorySlotRightWeapon);
	if (rightWeapon && rightWeapon->isRangedWeapon())
		return rightWeapon->getMaxAttackRange();

	return 1.0f;
}

Item *Creature::addScriptItem(const Common::UString &tag) {
	// Return an existing script item for this tag if one already exists
	for (auto &item : _scriptItems)
		if (item && item->getTag() == tag)
			return item.get();

	try {
		_scriptItems.push_back(std::make_unique<Item>(tag));
		return _scriptItems.back().get();
	} catch (Common::Exception &e) {
		e.add("Failed to create script item \"%s\"", tag.c_str());
		Common::printException(e, "WARNING: ");
		return nullptr;
	}
}

Item *Creature::findInventoryItemByTag(const Common::UString &tag) const {
	// Search equipped items first
	for (const auto &kv : _equipment) {
		if (kv.second && kv.second->getTag() == tag)
			return kv.second.get();
	}
	// Search live script items in inventory
	for (const auto &item : _scriptItems) {
		if (item && item->getTag() == tag)
			return item.get();
	}
	return nullptr;
}

int Creature::getHitDice() const {
	int total = 0;
	const int numClasses = _info.getNumClasses();
	for (int i = 0; i < numClasses; ++i)
		total += _info.getLevelByPosition(i);
	return total;
}

int Creature::getBAB() const {
	return _info.getBAB();
}

int Creature::getAC() const {
	int ac = 10 + _info.getAbilityModifier(kAbilityDexterity);

	const Item *armor = getEquipedItem(kInventorySlotBody);
	if (armor)
		ac += armor->getACBonus();

	ac += _armorClassModifier;
	return ac;
}

void Creature::playDefaultAnimation() {
	if (_model)
		_model->playDefaultAnimation();
}

void Creature::playDefaultHeadAnimation() {
	if (!_model)
		return;

	Graphics::Aurora::AnimationChannel *headChannel = 0;

	if (_modelType == "B" || _modelType == "P") {
		Graphics::Aurora::Model *head = _model->getAttachedModel("headhook");
		if (head)
			headChannel = head->getAnimationChannel(Graphics::Aurora::kAnimationChannelAll);
	} else
		headChannel = _model->getAnimationChannel(Graphics::Aurora::kAnimationChannelHead);

	if (headChannel)
		headChannel->playDefaultAnimation();
}

void Creature::playDrawWeaponAnimation() {
	if (!_model)
		return;

	const Item *rightWeapon = getEquipedItem(kInventorySlotRightWeapon);
	const Item *leftWeapon = getEquipedItem(kInventorySlotLeftWeapon);

	if (rightWeapon && !leftWeapon) {
		switch (rightWeapon->getWeaponWield()) {
			case kWeaponWieldBaton:
				_model->playAnimation("g1w1");
				break;
			case kWeaponWieldSword:
				_model->playAnimation("g2w1");
				break;
			case kWeaponWieldStaff:
				_model->playAnimation("g3w1");
				break;
			case kWeaponWieldPistol:
				_model->playAnimation("g5w1");
				break;
			case kWeaponWieldRifle:
				_model->playAnimation("g7w1");
				break;
			default:
				break;
		}
		return;
	}

	if (rightWeapon && leftWeapon) {
		switch (rightWeapon->getWeaponWield()) {
			case kWeaponWieldSword:
				_model->playAnimation("g4w1");
				break;
			case kWeaponWieldPistol:
				_model->playAnimation("g6w1");
				break;
			default:
				break;
		}
	}
}

void Creature::playAttackAnimation() {
	if (!_model)
		return;

	const Item *rightWeapon = getEquipedItem(kInventorySlotRightWeapon);
	const Item *leftWeapon = getEquipedItem(kInventorySlotLeftWeapon);

	if (rightWeapon && !leftWeapon) {
		switch (rightWeapon->getWeaponWield()) {
			case kWeaponWieldBaton:
				_model->playAnimation("g1a1");
				break;
			case kWeaponWieldSword:
				_model->playAnimation("g2a1");
				break;
			case kWeaponWieldStaff:
				_model->playAnimation("g3a1");
				break;
			case kWeaponWieldPistol:
				_model->playAnimation("b5a1");
				break;
			case kWeaponWieldRifle:
				_model->playAnimation("b7a1");
				break;
			default:
				break;
		}
		return;
	}

	if (rightWeapon && leftWeapon) {
		switch (rightWeapon->getWeaponWield()) {
			case kWeaponWieldSword:
				_model->playAnimation("g4a1");
				break;
			case kWeaponWieldPistol:
				_model->playAnimation("b6a1");
				break;
			default:
				break;
		}
		return;
	}

	_model->playAnimation("g8a1");
}

void Creature::playDodgeAnimation() {
	if (!_model)
		return;

	int number = getWeaponAnimationNumber();
	if (number != -1)
		_model->playAnimation(Common::String::format("g%dg1", number));
}

void Creature::playAnimation(const Common::UString &anim, bool restart, float length, float speed) {
	if (_model)
		_model->playAnimation(anim, restart, length, speed);
}

void Creature::getTooltipAnchor(float &x, float &y, float &z) const {
	if (!_model) {
		Object::getTooltipAnchor(x, y, z);
		return;
	}

	_model->getTooltipAnchor(x, y, z);
}

void Creature::updatePerception(Creature &object) {
	const float kPerceptionRange = 16.0f;

	float distance = glm::distance(
		glm::make_vec3(_position),
		glm::make_vec3(object._position));

	if (distance <= kPerceptionRange) {
		handleObjectSeen(object);
		handleObjectHeard(object);

		object.handleObjectSeen(*this);
		object.handleObjectHeard(*this);

	} else {
		handleObjectVanished(object);
		handleObjectInaudible(object);

		object.handleObjectVanished(*this);
		object.handleObjectInaudible(*this);
	}
}

bool Creature::isInCombat() const {
	return _inCombat;
}

Object *Creature::getAttackTarget() const {
	return _attackTarget;
}

int Creature::getAttackRound() const {
	return _attackRound;
}

Object *Creature::getAttemptedAttackTarget() const {
	return _attemptedAttackTarget;
}

Object *Creature::getLastHostileActor() const {
	return _lastHostileActor;
}

int Creature::getLastCombatFeatUsed() const {
	return _lastCombatFeatUsed;
}

int Creature::getQueuedCombatFeat() const {
	return _queuedCombatFeat;
}

void Creature::setAttemptedAttackTarget(Object *target) {
	_attemptedAttackTarget = target;
}

void Creature::setLastCombatFeatUsed(int featID) {
	_lastCombatFeatUsed = featID;
}

void Creature::queueCombatFeat(int featID) {
	_queuedCombatFeat = featID;
}

int Creature::consumeQueuedCombatFeat() {
	const int queuedFeat = _queuedCombatFeat;
	_queuedCombatFeat = -1;
	return queuedFeat;
}

void Creature::adjustAttackModifier(int amount) {
	_attackModifier += amount;
}

void Creature::adjustArmorClassModifier(int amount) {
	_armorClassModifier += amount;
}

void Creature::adjustSkillModifier(Skill skill, int amount) {
	if (skill < kSkillComputerUse || skill >= kSkillMAX)
		return;

	_skillModifiers[static_cast<int>(skill)] += amount;
}

int Creature::getAttackModifier() const {
	return _attackModifier;
}

int Creature::getArmorClassModifier() const {
	return _armorClassModifier;
}

int Creature::getSkillModifier(Skill skill) const {
	if (skill < kSkillComputerUse || skill >= kSkillMAX)
		return 0;

	return _skillModifiers[static_cast<int>(skill)];
}

void Creature::startCombat(Object *target, int round) {
	if (!target) {
		cancelCombat();
		return;
	}

	_inCombat = true;
	_attackTarget = target;
	_attackRound = round;
	_attemptedAttackTarget = nullptr;
}

void Creature::cancelCombat() {
	_inCombat = false;
	_attackTarget = nullptr;
}

void Creature::applyEffect(const Effect &effect) {
	int current = getCurrentHitPoints();

	switch (effect.getType()) {
		case kEffectHeal: {
			int maxHP = getMaxHitPoints();
			int healed = current + effect.getAmount();
			if (healed > maxHP)
				healed = maxHP;
			setCurrentHitPoints(healed);
			break;
		}
		case kEffectDamage: {
			int minHp = getMinOneHitPoints() ? 1 : 0;
			int damaged = current - effect.getAmount();
			if (damaged < minHp)
				damaged = minHp;
			setCurrentHitPoints(damaged);

			if (getCurrentHitPoints() <= 0) {
				cancelCombat();
				handleDeath();
			}
			break;
		}
		case kEffectTemporaryHitpoints: {
			int maxHP = getMaxHitPoints();
			int boosted = current + effect.getAmount();
			if (boosted > maxHP)
				boosted = maxHP;
			setCurrentHitPoints(boosted);
			break;
		}
		case kEffectACIncrease:
			adjustArmorClassModifier(effect.getAmount());
			break;
		case kEffectAttackIncrease:
			adjustAttackModifier(effect.getAmount());
			break;
		case kEffectSkillIncrease: {
			const int skillID = effect.getDamageType();
			if (skillID >= kSkillComputerUse && skillID < kSkillMAX)
				adjustSkillModifier(static_cast<Skill>(skillID), effect.getAmount());
			break;
		}
		case kEffectDeath:
			setCurrentHitPoints(getMinOneHitPoints() ? 1 : 0);
			if (!isDead()) {
				cancelCombat();
				handleDeath();
			}
			break;
		case kEffectKnockdown:
			adjustArmorClassModifier(-4);
			break;
		case kEffectParalyze:
			clearActions();
			adjustArmorClassModifier(-4);
			break;
		case kEffectStunned:
			clearActions();
			adjustArmorClassModifier(-2);
			break;
		case kEffectMovementSpeedIncrease:
			// Implement movement speed boost logic here in the future
			break;
		default:
			break;
	}
}

void Creature::executeAttack(Object *target, int babPenalty, int damageMod, int activeFeat) {
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

	// Track hostility for NWScript callers (GetLastHostileActor/GetLastAttacker).
	if (targetCreature)
		targetCreature->_lastHostileActor = this;

	const Item *rightWeapon = getEquipedItem(kInventorySlotRightWeapon);
	const Item *leftWeapon  = getEquipedItem(kInventorySlotLeftWeapon);

	// Determine attack stat: Dex for ranged, Str for melee.
	bool ranged = (rightWeapon && rightWeapon->isRangedWeapon()) ||
	              (leftWeapon  && leftWeapon->isRangedWeapon());
	int abMod = ranged ? _info.getAbilityModifier(kAbilityDexterity)
	                   : _info.getAbilityModifier(kAbilityStrength);

	// Feat modifiers: attack/damage bonuses from active combat feats.
	int featAttackMod = 0;
	int featDamageMod = damageMod; // caller may pass per-attack bonuses

	const bool hasActivePowerAttack = (activeFeat == kFeatPowerAttack || activeFeat == kFeatImprovedPowerAttack);
	const bool hasActiveCriticalStrike = (activeFeat == kFeatCriticalStrike || activeFeat == kFeatImprovedCriticalStrike);

	if (!ranged && hasActivePowerAttack) {
		if (activeFeat == kFeatImprovedPowerAttack && _info.hasFeat(kFeatImprovedPowerAttack))
			{ featAttackMod -= 3; featDamageMod += 6; }
		else if (_info.hasFeat(kFeatPowerAttack))
			{ featAttackMod -= 3; featDamageMod += 3; }
	}

	// Weapon Focus / Specialization bonuses.
	if (rightWeapon || leftWeapon) {
		if (ranged) {
			if (_info.hasFeat(kFeatWeaponFocusBlaster) || _info.hasFeat(kFeatWeaponFocusBlasterRifle))
				featAttackMod += 1;
			if (_info.hasFeat(kFeatWeaponSpecializationBlaster) || _info.hasFeat(kFeatWeaponSpecializationBlasterRifle))
				featDamageMod += 2;
		} else {
			if (_info.hasFeat(kFeatWeaponFocusMelee))
				featAttackMod += 1;
			if (_info.hasFeat(kFeatWeaponSpecializationMelee))
				featDamageMod += 2;
		}
	}

	// Full attack bonus = BAB - iterative penalty + ability mod + feats.
	int bab = getBAB();
	int attackBonus = (bab + babPenalty) + abMod + featAttackMod + _attackModifier;

	// Roll d20 (1..20 inclusive) vs target AC.
	int d20 = RNG.getNext(1, 21);
	int attackRoll = d20 + attackBonus;
	int targetAC   = targetCreature ? targetCreature->getAC() : 10;

	// --- Lightsaber Deflection (Blaster attacks only) ---
	if (ranged && targetCreature && targetCreature->hasLightsaberEquipped()) {
		// Rule: Opposed roll. If Deflect roll >= Attack roll, the bolt is deflected.
		int deflectD20 = RNG.getNext(1, 21);
		int deflectBonus = targetCreature->getBAB() + targetCreature->getCreatureInfo().getAbilityModifier(kAbilityDexterity);
		
		if (targetCreature->getCreatureInfo().hasFeat(kFeatMasterJediDefense))
			deflectBonus += 10;
		else if (targetCreature->getCreatureInfo().hasFeat(kFeatImprovedJediDefense))
			deflectBonus += 6;

		int deflectTotal = deflectD20 + deflectBonus;
		if (deflectTotal >= attackRoll) {
			debugC(Common::kDebugEngineLogic, 1,
			       "DEFLECTED: Blaster bolt from \"%s\" deflected by \"%s\" (Deflect %d vs Attack %d)",
			       _tag.c_str(), targetCreature->getTag().c_str(), deflectTotal, attackRoll);
			// In a full implementation, we would trigger a "deflect" animation here.
			return;
		}
	}

	// Natural 1 always misses; natural 20 always hits.
	bool hit = (d20 == 20) || (d20 != 1 && attackRoll >= targetAC);

	if (!hit) {
		debugC(Common::kDebugEngineLogic, 1,
		       "Object \"%s\" missed \"%s\" (d20=%d bab=%d abMod=%d feat=%d effect=%d total=%d vs AC %d)",
		       _tag.c_str(), target->getTag().c_str(),
		       d20, bab + babPenalty, abMod, featAttackMod, _attackModifier, attackRoll, targetAC);
		return;
	}

	// --- Critical hit system ---
	// Threat on natural 20 (weapons may lower this, but 20 is the universal minimum).
	// Critical Strike feat: threat on 19-20; Improved: 18-20.
	int critThreat = 20;
	if (hasActiveCriticalStrike && activeFeat == kFeatImprovedCriticalStrike && _info.hasFeat(kFeatImprovedCriticalStrike))
		critThreat = 18;
	else if (hasActiveCriticalStrike && _info.hasFeat(kFeatCriticalStrike))
		critThreat = 19;

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
	if (rightWeapon && leftWeapon)
		damage = computeWeaponDamage(leftWeapon) + computeWeaponDamage(rightWeapon);
	else if (rightWeapon)
		damage = computeWeaponDamage(rightWeapon);
	else
		// Unarmed: 1 + Str modifier (minimum 1 die).
		damage = 1 + _info.getAbilityModifier(kAbilityStrength);

	// On a confirmed critical, double the weapon dice (not the fixed modifiers).
	if (isCrit) {
		int dieDamage;
		int modDamage;
		if (rightWeapon && leftWeapon) {
			int rMod = _info.getAbilityModifier(kAbilityStrength);
			int lMod = _info.getAbilityModifier(kAbilityStrength);
			dieDamage = damage - rMod - lMod;
			modDamage  = rMod + lMod;
		} else if (rightWeapon) {
			int mod = ranged ? _info.getAbilityModifier(kAbilityDexterity)
			                 : _info.getAbilityModifier(kAbilityStrength);
			dieDamage = damage - mod;
			modDamage  = mod;
		} else {
			// Unarmed: the "1" is the die, Str mod is the modifier.
			int mod = _info.getAbilityModifier(kAbilityStrength);
			dieDamage = damage - mod;
			modDamage  = mod;
		}
		// Double the dice portion only.
		damage = dieDamage * 2 + modDamage;
	}

	// Apply feat damage bonus.
	damage += featDamageMod;

	// Sneak Attack (Scoundrel class feature): roll +1d6 per rank when the
	// target is flat-footed (not currently in combat) or knocked down/paralysed.
	// Condition: target is a creature not in combat (hasn't acted yet this round).
	bool targetFlatFooted = targetCreature && !targetCreature->isInCombat();
	if (hit && targetFlatFooted) {
		// Count sneak attack ranks.
		int sneakRanks = 0;
		for (int rank = kFeatSneakAttack1; rank <= kFeatSneakAttack9; ++rank) {
			if (_info.hasFeat(static_cast<uint32_t>(rank)))
				++sneakRanks;
		}
		if (sneakRanks > 0) {
			int sneakDamage = 0;
			for (int i = 0; i < sneakRanks; ++i)
				sneakDamage += RNG.getNext(1, 7); // 1d6 per rank
			damage += sneakDamage;
			debugC(Common::kDebugEngineLogic, 1,
			       "Sneak Attack x%d: +%d damage on \"%s\"",
			       sneakRanks, sneakDamage, target->getTag().c_str());
		}
	}

	if (damage < 1)
		damage = 1;

	int hp    = target->getCurrentHitPoints() - damage;
	int minHp = target->getMinOneHitPoints() ? 1 : 0;
	if (hp < minHp)
		hp = minHp;

	target->setCurrentHitPoints(hp);

	debugC(Common::kDebugEngineLogic, 1,
	       "Object \"%s\" was %shit by \"%s\" (d20=%d bab=%d abMod=%d effect=%d total=%d vs AC %d), %d dmg, %d/%d HP",
	       target->getTag().c_str(), isCrit ? "CRITICALLY " : "",
	       _tag.c_str(), d20, bab + babPenalty, abMod, _attackModifier, attackRoll, targetAC,
	       damage, hp, target->getMaxHitPoints());

	if (hp <= minHp) {
		cancelCombat();
		if (targetCreature) {
			targetCreature->cancelCombat();
			if (targetCreature->handleDeath()) {
				// Signal the module to award XP to the killer's party.
				// We use userDefinedEvent 1007 (already fired by handleCreaturesDeath) — XP
				// is awarded there via the module's updateXPOnKill() call.
			}
		}
	}
}

bool Creature::isDead() const {
	return _dead;
}

bool Creature::isXPAwarded() const {
	return _xpAwarded;
}

void Creature::markXPAwarded() {
	_xpAwarded = true;
}

bool Creature::handleDeath() {
	if (!_dead && _currentHitPoints <= 0) {
		_dead = true;
		if (_model) {
			_model->clearDefaultAnimations();
			_model->addDefaultAnimation("dead", 100);
			_model->playAnimation("die", false);
		} else {
			warning("Creature::handleDeath(): \"%s\" has no model; applying logical death only", _tag.c_str());
		}
		return true;
	}
	return false;
}

Object *Creature::getLastPerceived() const {
	return _lastPerceived;
}
bool Creature::getLastPerceptionSeen() const {
	return _lastPerceptionSeen;
}
bool Creature::getLastPerceptionVanished() const {
	return _lastPerceptionVanished;
}
bool Creature::getLastPerceptionHeard() const {
	return _lastPerceptionHeard;
}
bool Creature::getLastPerceptionInaudible() const {
	return _lastPerceptionInaudible;
}

bool Creature::hasSeenObject(const Object *obj) const {
	return _seenObjects.count(const_cast<Object *>(obj)) > 0;
}

bool Creature::hasHeardObject(const Object *obj) const {
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
		debugC(Common::kDebugEngineLogic, 2,
			"Creature \"%s\" have seen \"%s\"",
			_tag.c_str(), object.getTag().c_str());
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
		debugC(Common::kDebugEngineLogic, 2,
			"Object \"%s\" have vanished from \"%s\"",
			object.getTag().c_str(), _tag.c_str());
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

void Creature::playHeadAnimation(const Common::UString &anim, bool restart, float length, float speed) {
	if (!_model)
		return;

	Graphics::Aurora::AnimationChannel *headChannel = 0;

	if (_modelType == "B" || _modelType == "P") {
		Graphics::Aurora::Model *head = _model->getAttachedModel("headhook");
		if (head)
			headChannel = head->getAnimationChannel(Graphics::Aurora::kAnimationChannelAll);
	} else
		headChannel = _model->getAnimationChannel(Graphics::Aurora::kAnimationChannelHead);

	if (headChannel)
		headChannel->playAnimation(anim, restart, length, speed);
}

const Action *Creature::getCurrentAction() const {
	return _actions.getCurrent();
}

void Creature::clearActions() {
	_actions.clear();
}

void Creature::addAction(const Action &action) {
	_actions.add(action);
}

void Creature::popAction() {
	_actions.pop();
}

void Creature::performCutsceneAttack(Object *target, int flags) {
	if (!target)
		return;

	// 1. Ensure we face each other
	makeLookAt(target);
	Creature *targetCreature = ObjectContainer::toCreature(target);
	if (targetCreature)
		targetCreature->makeLookAt(this);

	// 2. Play attack animation
	// Standard attack for cutscenes
	playAnimation("attack1", false, 1.2f);

	// 3. Handle reactions
	if (flags & kCutsceneAttackForceHit) {
		if (targetCreature) {
			// Small delay for impact feel would be better, but for now immediate flinch
			targetCreature->playAnimation("dodge", false, 0.5f);
		}
	} else if (flags & kCutsceneAttackForceMiss) {
		if (targetCreature) {
			targetCreature->playAnimation("g8a2", false, 0.8f); // Side dodge
		}
	}

	if (flags & kCutsceneAttackKnockback) {
		if (targetCreature) {
			targetCreature->playAnimation("die", false, 1.5f);
		}
	}
}

void Creature::setDefaultAnimations() {
	if (!_model)
		return;

	if (_modelType == "S" || _modelType == "L")
		_model->addDefaultAnimation("cpause1", 100);
	else
		_model->addDefaultAnimation("pause1", 100);
}

void Creature::reloadEquipment() {
	for (int i = static_cast<int>(kInventorySlotHead); i < static_cast<int>(kInventorySlotMAX); ++i) {
		InventorySlot slot = InventorySlot(i);
		if (_info.isInventorySlotEquipped(slot))
			addItemToEquipment(_info.getEquippedItem(slot), slot);
	}
}

bool Creature::addItemToEquipment(const Common::UString &tag, InventorySlot slot) {
	try {
		_equipment.insert(std::make_pair(slot, std::make_unique<Item>(tag)));
	} catch (Common::Exception &e) {
		e.add("Failed to load item \"%s\"", tag.c_str());
		Common::printException(e, "WARNING: ");
		return false;
	}

	return true;
}

int Creature::getWeaponAnimationNumber() const {
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

int Creature::computeWeaponDamage(const Item *weapon) const {
	int result = 0;
	Ability ability = weapon->isRangedWeapon() ? kAbilityDexterity : kAbilityStrength;
	int mod = _info.getAbilityModifier(ability);

	for (int i = 0; i < weapon->getNumDice(); ++i) {
		result += RNG.getNext(1, weapon->getDieToRoll() + 1);
	}

	result += mod;
	return (result < 1) ? 1 : result;
}

bool Creature::hasLightsaberEquipped() const {
	const Item *right = getEquipedItem(kInventorySlotRightWeapon);
	const Item *left = getEquipedItem(kInventorySlotLeftWeapon);

	auto isLasso = [](const Item *item) {
		if (!item) return false;
		int base = item->getBaseItem();
		return base == kBaseItemLightsaber || 
		       base == kBaseItemDoubleLightsaber || 
		       base == kBaseItemShortLightsaber;
	};

	return isLasso(right) || isLasso(left);
}

int Creature::getAlignment() const {
	return _info.getAlignment();
}

void Creature::adjustAlignment(int shift) {
	_info.adjustAlignment(shift);
}

void Creature::setAIArchetype(AIArchetype archetype) {
	_aiArchetype = archetype;
}

void Creature::think() {
	if (_aiArchetype == kAIArchetypeNone || isPC() || isDead() || !_area)
		return;

	if (_aiCooldown > 0.0f)
		return;

	Creature *target = _area->findNearestEnemy(this);
	if (!target)
		return;

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
			_actions.add(Action(kActionAttackObject, target)); // TODO: Add Poison Effect chance
			_aiCooldown = 1.5f;
			break;

		case kAIArchetypeTacticalHumanoid:
			// Standard Mandalorian or mercenary AI.
			_actions.clear();
			_actions.add(Action(kActionAttackObject, target));
			_aiCooldown = 2.0f;
			break;

		case kAIArchetypeForceUser:
			// Advanced Force-using NPCs (Sith/Dark Jedi).
			_actions.clear();
			_actions.add(Action(kActionAttackObject, target));
			_aiCooldown = 1.0f;
			break;

		default:
			break;
	}
}

void Creature::applyEffect(EffectType type, float duration, int value) {
	Effect e;
	e.type = type;
	e.duration = duration;
	e.value = value;
	_effects.push_back(e);
}

void Creature::updateEffects(float dt) {
	for (auto it = _effects.begin(); it != _effects.end(); ) {
		it->duration -= dt;

		if (it->type == kEffectPoison) {
			// Periodic poison damage (simplified: value is damage per sec)
			if (_hpCurrent > 0) {
				float dam = it->value * dt;
				_hpCurrent -= (int)dam;
				if (_hpCurrent < 1) _hpCurrent = 1; // Poison usually doesn't kill in KOTOR
			}
		}

		if (it->duration <= 0.0f) {
			it = _effects.erase(it);
		} else {
			++it;
		}
	}
}

bool Creature::hasEffect(EffectType type) const {
	for (const auto &e : _effects) {
		if (e.type == type)
			return true;
	}
	return false;
}

void Creature::update(float dt) {
	if (isDead())
		return;

	updateEffects(dt);
	} // End of namespace KotORBase

} // End of namespace Engines
