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
 *  Dialog GUI for KotOR games.
 */

#include "src/common/configman.h"
#include "src/common/strutil.h"

#include "src/events/types.h"

#include "src/graphics/windowman.h"
#include "src/graphics/aurora/cursorman.h"

#include "src/sound/sound.h"

#include "src/engines/aurora/util.h"

#include "src/engines/odyssey/label.h"
#include "src/engines/odyssey/listbox.h"
#include "src/engines/odyssey/scrollbar.h"

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/inventory.h"
#include "src/engines/kotorbase/module.h"
#include "src/aurora/nwscript/objectman.h"
#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/script/functions.h"
#include "src/engines/kotorbase/animationnames.h"
#include "src/engines/kotorbase/cinematicshot.h"
#include "src/engines/kotorbase/objectcontainer.h"

#include "src/engines/kotorbase/gui/dialog.h"

namespace Engines {

namespace KotORBase {

DialogGUI::DialogGUI(Module &module, bool computer) :
		_module(module),
		_computer(computer),
		_isActive(false),
		_frame(new Graphics::Aurora::KotORDialogFrame()) {
}

bool DialogGUI::isComputerConversation(uint32_t conversationType) {
	return conversationType == kConversationTypeComputer;
}

uint32_t DialogGUI::getConversationType(const Common::UString &name) {
	try {
		return Aurora::DLGFile(name, nullptr).getConversationType();
	} catch (...) {
		return 0;
	}
}

bool DialogGUI::isComputer() const {
	return _computer;
}

void DialogGUI::startConversation(const Common::UString &name, Aurora::NWScript::Object *owner) {
	try {
		releaseStunts();
		_dlg = std::make_unique<Aurora::DLGFile>(name, owner);
		_dlg->startConversation();
		_owner = owner ? owner->getTag() : "";
		_pickedReply = false;
		applyStunts();
		refresh();
	} catch (Common::Exception &e) {
		warning("Failed to start conversation %s: %s", name.c_str(), e.what());
	}
}

void DialogGUI::stopConversation() {
	stopSounds();
	releaseStunts();
	_dlg.reset();
	_replyIds.clear();
	_owner.clear();
	_curSpeaker.clear();
	_isActive = false;
}

bool DialogGUI::isConversationActive() const {
	return _isActive;
}

bool DialogGUI::isObjectInConversation(Aurora::NWScript::Object *object) const {
	if (!_isActive || !object)
		return false;

	if (Creature *creature = ObjectContainer::toCreature(object)) {
		if (creature->isPC())
			return true;
	}

	const Common::UString &tag = object->getTag();
	if (!tag.empty() && tag == _owner)
		return true;
	if (!tag.empty() && tag == _curSpeaker)
		return true;

	return false;
}

void DialogGUI::show() {
	CursorMan.setGroup("default");
	GUI::show();

	// A terminal fills the screen itself: no letterbox.
	if (!_computer)
		_frame->show();
}

void DialogGUI::hide() {
	_frame->hide();
	GUI::hide();
}

void DialogGUI::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();
	if (!tag.beginsWith("LB_REPLIES_ITEM"))
		return;

	Odyssey::WidgetListBox *lbReplies = getListBox("LB_REPLIES");
	lbReplies->selectItemByWidgetTag(tag);

	int selectedIndex = lbReplies->getSelectedIndex();
	if (selectedIndex >= 0)
		pickReply(selectedIndex);
}

void DialogGUI::callbackKeyInput(const Events::Key &key,
                                     const Events::EventType &type) {
	if (type == Events::kEventKeyDown) {
		switch (key) {
			case Events::kKeyUp:
				getListBox("LB_REPLIES")->selectPreviousItem();
				break;
			case Events::kKeyDown:
				getListBox("LB_REPLIES")->selectNextItem();
				break;
			case Events::kKey1:
			case Events::kKey2:
			case Events::kKey3:
			case Events::kKey4:
			case Events::kKey5:
			case Events::kKey6:
			case Events::kKey7:
			case Events::kKey8:
			case Events::kKey9:
				pickReply(key - Events::kKey1);
				break;
			case Events::kKeyKP1:
			case Events::kKeyKP2:
			case Events::kKeyKP3:
			case Events::kKeyKP4:
			case Events::kKeyKP5:
			case Events::kKeyKP6:
			case Events::kKeyKP7:
			case Events::kKeyKP8:
			case Events::kKeyKP9:
				pickReply(key - Events::kKeyKP1);
				break;
			default:
				break;
		}
	}
}

void DialogGUI::preprocessEntry(Common::UString &text) {
	// KotOR uses <CUSTOMxxx> for dynamic tokens
	std::string stdText = text.toString();
	for (size_t pos = 0; (pos = stdText.find("<CUSTOM", pos)) != std::string::npos; ) {
		size_t end = stdText.find(">", pos);
		if (end == std::string::npos)
			break;

		std::string tag = stdText.substr(pos + 7, end - (pos + 7));
		int tokenID = 0;
		try {
			std::string cleanTag;
			for (char c : tag) {
				if (isdigit(c)) cleanTag += c;
			}
			Common::parseString(cleanTag, tokenID);
		} catch (...) {
			pos = end + 1;
			continue;
		}

		Common::UString replacement = _module.getGame().getFunctions().getCustomToken(tokenID);
		stdText.replace(pos, end - pos + 1, replacement.toString());
		pos += replacement.toString().size();
	}
	text = stdText;
}

void DialogGUI::update(int width, int height) {
	if (_computer) {
		// The computer GUI keeps the layout of its GUI file.
		float r, g, b;
		getTextColor(r, g, b);

		Odyssey::WidgetListBox *lbMessage = getListBox("LB_MESSAGE");
		if (lbMessage) {
			lbMessage->setAdjustHeight(true);
			lbMessage->setItemSelectionEnabled(false);
			lbMessage->setItemTextColor(r, g, b, 1.0f);
			lbMessage->createItemWidgets(1);
		}

		Odyssey::WidgetListBox *lbReplies = getListBox("LB_REPLIES");
		if (lbReplies) {
			lbReplies->setAdjustHeight(true);
			lbReplies->setSoundHoverItem("gui_actscroll");
			lbReplies->setSoundClickItem("gui_actuse");
			lbReplies->setItemTextColor(r, g, b, 1.0f);
			lbReplies->createItemWidgets(9);
		}

		return;
	}

	const int w = width;
	const float hh = height / 2.0f;
	const float rh = hh / 2.0f; // quarter of window height

	_frame->setRectangleHeight(rh);

	Odyssey::WidgetLabel *lblMessage = getLabel("LBL_MESSAGE");
	lblMessage->setHorizontalTextAlign(Graphics::Aurora::kHAlignCenter);
	lblMessage->setVerticalTextAlign(Graphics::Aurora::kVAlignBottom);
	lblMessage->setPosition(-w / 2.0f, hh - rh, -1.0f);
	lblMessage->setWidth(w);
	lblMessage->setHeight(rh);

	Odyssey::WidgetListBox *lbReplies = getListBox("LB_REPLIES");
	lbReplies->setAdjustHeight(true);
	lbReplies->setPosition(-w / 2.0f, -hh, -1.0f);
	lbReplies->setWidth(w);
	lbReplies->setHeight(rh);
	lbReplies->setSoundHoverItem("gui_actscroll");
	lbReplies->setSoundClickItem("gui_actuse");

	float r, g, b;
	getTextColor(r, g, b);

	lblMessage->setTextColor(r, g, b, 1.0f);
	lbReplies->setItemTextColor(r, g, b, 1.0f);

	lbReplies->createItemWidgets(9);
}

void DialogGUI::refresh() {
	const Aurora::DLGFile::Line *curEntry = _dlg->getCurrentEntry();
	if (!curEntry) {
		_isActive = false;
		releaseStunts();
		return;
	}

	stopSounds();
	playSounds();

	const std::vector<const Aurora::DLGFile::Line *> replies = _dlg->getCurrentReplies();
	if (replies.empty() && !_pickedReply) {
		// A single reply-less line is a one-liner (a bark): show it over the
		// speaker's head and end at once, still running EndConversation.
		Common::UString bark = curEntry->text.getString();
		preprocessEntry(bark);

		Object *barker = findParticipant(curEntry->speaker);
		if (barker && !bark.empty())
			_module.showFloatingText(barker, bark, 4.0f);

		_dlg->pickReply(Aurora::DLGFile::kEndLine);
		_isActive = false;
		releaseStunts();
		return;
	}

	_isActive = true;

	Common::UString text = curEntry->text.getString();
	preprocessEntry(text);

	if (curEntry->speaker.empty())
		_curSpeaker = _owner;
	else
		_curSpeaker = curEntry->speaker;

	Object *speakerObj = findParticipant(_curSpeaker);
	Object *listenerObj = curEntry->listener.empty() ? _module.getPartyLeader() : findParticipant(curEntry->listener);

	if (speakerObj) {
		makeLookAtPC(_curSpeaker);
		playTalkAnimations(_curSpeaker);
	}

	// A computer terminal keeps the camera where it is.
	if (!_computer)
		updateCamera(*curEntry, speakerObj, listenerObj);
	else
		updateComputerCounters();

	applyFade(*curEntry);
	playParticipantAnimations(*curEntry);
	armAutoAdvance(*curEntry, replies);

	setMessage(text);
	_replyIds.clear();
	Odyssey::WidgetListBox *lbReplies = getListBox("LB_REPLIES");
	lbReplies->removeAllItems();

	if (replies.empty()) {
		// The NPC's closing line: show it, and end the conversation (running
		// its EndConversation script) once the player continues.
		const uint32_t endLine = Aurora::DLGFile::kEndLine; // push_back() would ODR-use the constant
		_replyIds.push_back(endLine);
		lbReplies->addItem("1. [END DIALOGUE]");
		lbReplies->refreshItemWidgets();
		return;
	}

	int index = 1;
	for (std::vector<const Aurora::DLGFile::Line *>::const_iterator it = replies.begin();
			it != replies.end();
			++it) {
		const Aurora::DLGFile::Line *reply = *it;
		_replyIds.push_back(reply->id);

		text = reply->text.getString();

		preprocessEntry(text);
		if (text.empty())
			text = "[CONTINUE]";

		text = Common::String::format("%d. %s", index++, text.c_str());
		lbReplies->addItem(text);
	}
	lbReplies->refreshItemWidgets();
}

void DialogGUI::setMessage(const Common::UString &text) {
	if (Odyssey::WidgetLabel *lblMessage = getLabel("LBL_MESSAGE")) {
		lblMessage->setText(text);
		return;
	}

	Odyssey::WidgetListBox *lbMessage = getListBox("LB_MESSAGE");
	if (!lbMessage)
		return;

	lbMessage->removeAllItems();
	lbMessage->addItem(text);
	lbMessage->refreshItemWidgets();
}

void DialogGUI::updateComputerCounters() {
	Creature *leader = _module.getPartyLeader();
	Creature *pc = _module.getPC();

	// The party shares the PC's inventory.
	const Inventory *inventory = pc ? &pc->getInventory() : (leader ? &leader->getInventory() : nullptr);

	const int spikes = inventory ? inventory->countItems(kComputerSpikeTag) : 0;
	const int parts  = inventory ? inventory->countItems(kRepairPartTag) : 0;

	const int computerUse = leader ? leader->getSkillRank(kSkillComputerUse) : 0;
	const int repair      = leader ? leader->getSkillRank(kSkillRepair) : 0;

	if (Odyssey::WidgetLabel *label = getLabel("LBL_COMP_SPIKES_VAL"))
		label->setText(Common::composeString(spikes));
	if (Odyssey::WidgetLabel *label = getLabel("LBL_COMP_SKILL_VAL"))
		label->setText(Common::composeString(computerUse));
	if (Odyssey::WidgetLabel *label = getLabel("LBL_REP_UNITS_VAL"))
		label->setText(Common::composeString(parts));
	if (Odyssey::WidgetLabel *label = getLabel("LBL_REP_SKILL_VAL"))
		label->setText(Common::composeString(repair));
}

Object *DialogGUI::findParticipant(const Common::UString &tag) const {
	Area *area = _module.getCurrentArea();
	if (!area)
		return nullptr;

	if (tag.empty() || tag.equalsIgnoreCase("OWNER"))
		return _owner.empty() ? nullptr : area->getObjectByTag(_owner);

	if (tag.equalsIgnoreCase("PLAYER"))
		return _module.getPartyLeader();

	return area->getObjectByTag(tag);
}

void DialogGUI::updateCamera(const Aurora::DLGFile::Line &entry, Object *speaker, Object *listener) {
	const int angle = static_cast<int>(entry.cameraAngle);

	// Static (GIT) and animated cameras; anything unavailable falls back to
	// framing the speaker and listener.
	bool placed = false;
	if (angle == kDialogCameraStatic) {
		placed = _module.setStaticShot(entry.cameraID);
	} else if (angle == kDialogCameraAnimated || entry.cameraAnimation != 0) {
		const Common::UString &model = entry.cameraModel.empty() ? _dlg->getCameraModel() : entry.cameraModel;
		placed = _module.setAnimatedShot(model, entry.cameraAnimation);
	}

	if (!placed)
		_module.setDialogShot(angle, speaker, listener);

	if (angle != kDialogCameraStatic || !placed)
		_module.setShotFieldOfView(entry.camFieldOfView);
}

void DialogGUI::applyFade(const Aurora::DLGFile::Line &entry) {
	// FadeType (as KotOR.js reads it): 1 cut to the color, 2 cut from it,
	// 3 fade in from it over FadeLength, 4 fade out to it; after FadeDelay.
	if ((entry.fadeType < 1) || (entry.fadeType > 4))
		return;

	Graphics::Aurora::FadeQuad &fade = _module.getFadeQuad();

	const bool gradual = entry.fadeType >= 3;
	const bool fadeIn  = (entry.fadeType == 2) || (entry.fadeType == 3);

	fade.setColor(entry.fadeColor[0], entry.fadeColor[1], entry.fadeColor[2]);
	fade.setWaitTime(MAX(entry.fadeDelay, 0.0f));
	fade.setRunTime(gradual ? MAX(entry.fadeLength, 0.0f) : 0.0f);

	if (fadeIn)
		fade.fadeIn();
	else
		fade.fadeOut();
}

void DialogGUI::applyStunts() {
	if (!_dlg)
		return;

	for (const Aurora::DLGFile::Stunt &stunt : _dlg->getStunts()) {
		Creature *creature = ObjectContainer::toCreature(findParticipant(stunt.participant));
		if (!creature)
			continue;

		Graphics::Aurora::Model *model = _module.getStuntModel(stunt.stuntModel);
		if (!model)
			continue;

		creature->setStuntModel(model);
		_stuntActors.push_back(creature->getID());
	}
}

void DialogGUI::releaseStunts() {
	for (uint32_t id : _stuntActors) {
		Creature *creature = ObjectContainer::toCreature(ObjectMan.findObject(id));
		if (!creature)
			continue;

		creature->setStuntModel(nullptr);
		creature->playDefaultAnimation();
	}

	_stuntActors.clear();
}

void DialogGUI::playParticipantAnimations(const Aurora::DLGFile::Line &entry) {
	for (const Aurora::DLGFile::Animation &animation : entry.animations) {
		Creature *creature = ObjectContainer::toCreature(findParticipant(animation.participant));
		if (!creature)
			continue;

		// Cutscene animations (from 1200, usually a stunt model's) play once.
		if (animation.animation >= 1200 && animation.animation < 10000) {
			for (const Common::UString &name : cutsceneAnimationNames(animation.animation))
				if (creature->hasModelAnimation(name)) {
					creature->playAnimation(name, true, 0.0f);
					break;
				}

			continue;
		}

		// Numbers from 10000 are looping animations, the others play once.
		const bool looping = animation.animation >= 10000;
		const Common::UString name = getAnimationNameById(
			static_cast<int>(looping ? animation.animation - 10000 : animation.animation));
		if (name.empty())
			continue;

		creature->playAnimation(name, true, looping ? -1.0f : 0.0f);
	}
}

void DialogGUI::armAutoAdvance(const Aurora::DLGFile::Line &entry,
                               const std::vector<const Aurora::DLGFile::Line *> &replies) {
	_autoAdvance = false;
	_autoAdvanceDelay = 0.0f;

	// Only lines the player can merely continue from advance by themselves:
	// a closing line, or a single reply without text.
	const bool continueOnly = replies.empty() ||
		(replies.size() == 1 && replies.front()->text.getString().empty());
	if (!continueOnly)
		return;

	if (entry.delay != Aurora::DLGFile::kInvalidLine && entry.delay > 0) {
		_autoAdvance = true;
		_autoAdvanceDelay = static_cast<float>(entry.delay);

		// Let the line's fade finish first.
		if (entry.fadeType >= 3 && entry.fadeType <= 4)
			_autoAdvanceDelay = MAX(_autoAdvanceDelay, entry.fadeDelay + entry.fadeLength);
	} else if (_voice && SoundMan.isPlaying(*_voice)) {
		// Continue shortly after the voice-over finishes (a missing VO
		// resource leaves an empty handle: wait for the player then).
		_autoAdvance = true;
		_autoAdvanceDelay = -1.0f;
	}
}

void DialogGUI::update(float frameTime) {
	if (!_isActive || !_autoAdvance)
		return;

	if (_autoAdvanceDelay < 0.0f) {
		if (_voice && SoundMan.isPlaying(*_voice))
			return;

		_autoAdvanceDelay = 0.5f;
	}

	_autoAdvanceDelay -= frameTime;
	if (_autoAdvanceDelay > 0.0f)
		return;

	_autoAdvance = false;
	pickReply(0);
}

void DialogGUI::playSounds() {
	const Aurora::DLGFile::Line *entry = _dlg->getCurrentEntry();

	if (!entry->voice.empty())
		_voice = std::make_unique<Sound::ChannelHandle>(::Engines::playSound(entry->voice, Sound::kSoundTypeVoice));

	if (!entry->sound.empty())
		_sound = std::make_unique<Sound::ChannelHandle>(::Engines::playSound(entry->sound, Sound::kSoundTypeSFX));
}

void DialogGUI::stopSounds() {
	if (_voice) {
		SoundMan.stopChannel(*_voice.get());
		_voice.reset();
	}
	if (_sound) {
		SoundMan.stopChannel(*_sound.get());
		_sound.reset();
	}
}

void DialogGUI::pickReply(int index) {
	if ((int)_replyIds.size() <= index)
		return;

	// Actors are still acting out the line (ActionPauseConversation).
	if (_module.isConversationPaused())
		return;

	if (!_curSpeaker.empty()) {
		playDefaultAnimations(_curSpeaker);
		_curSpeaker.clear();
	}

	_pickedReply = true;
	_dlg->pickReply(_replyIds[index]);
	if (_dlg->hasEnded()) {
		stopSounds();
		_module.resetToOrbit();
		_isActive = false;
		releaseStunts();
	} else
		refresh();
}

void DialogGUI::makeLookAtPC(const Common::UString &tag) {
	Creature *pc = _module.getPC();
	if (!pc)
		return;

	Object *o = _module.getCurrentArea()->getObjectByTag(tag);
	if (!o)
		return;

	// Only creatures should orient themselves to the pc.
	Creature *creature = KotORBase::ObjectContainer::toCreature(o);
	if (creature && !creature->isLockOrientationInDialog())
		creature->makeLookAt(pc);

	if (!pc->isLockOrientationInDialog())
		pc->makeLookAt(o);

	// A computer terminal keeps the camera where it is.
	if (_computer)
		return;

	float x, y, z, a;
	pc->getOrientation(x, y, z, a);
	_module.setCameraYaw(Common::deg2rad(a - 15.0f));
}

void DialogGUI::playDefaultAnimations(const Common::UString &tag) {
	Object *o = _module.getCurrentArea()->getObjectByTag(tag);
	if (!o)
		return;

	Creature *creature = ObjectContainer::toCreature(o);
	if (!creature)
		return;

	creature->playDefaultHeadAnimation();
	creature->playDefaultAnimation();
}

void DialogGUI::playTalkAnimations(const Common::UString &tag) {
	Object *o = _module.getCurrentArea()->getObjectByTag(tag);
	if (!o)
		return;

	Creature *creature = ObjectContainer::toCreature(o);
	if (!creature)
		return;

	const Aurora::DLGFile::Line* entry = _dlg->getCurrentEntry();
	// animationID 0: tlknorm, 1: tlkslow, 2: tlksad, 3: tlkforce, etc.
	Common::UString anim = "tlknorm";
	if (entry->animation == 1) anim = "tlkslow";
	else if (entry->animation == 2) anim = "tlksad";
	else if (entry->animation == 3) anim = "tlkforce";

	creature->playAnimation(anim, true, -1.0f);
	if (!creature->isLockHeadFollowInDialog())
		creature->playHeadAnimation("talk", true, -1.0f, 0.25f);
}

void DialogGUI::notifyResized(int UNUSED(oldWidth), int UNUSED(oldHeight), int newWidth, int newHeight) {
	update(newWidth, newHeight);
}



} // End of namespace KotORBase

} // End of namespace Engines
