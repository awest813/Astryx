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
 *  Base dialog GUI for KotOR games.
 */

#ifndef ENGINES_KOTORBASE_GUI_DIALOG_H
#define ENGINES_KOTORBASE_GUI_DIALOG_H

#include <memory>
#include "src/common/ustring.h"

#include "src/aurora/dlgfile.h"

#include "src/aurora/nwscript/object.h"

#include "src/graphics/aurora/kotordialogframe.h"

#include "src/sound/types.h"

#include "src/engines/kotorbase/gui/dialog.h"

#include "src/engines/kotorbase/gui/gui.h"

namespace Engines {

namespace KotORBase {

class Module;
class Object;

class DialogGUI : public GUI, Events::Notifyable {
public:
	DialogGUI(Module &module);

	// Basic visuals

	void show();
	void hide();

	// Conversation

	bool isConversationActive() const;

	void startConversation(const Common::UString &name, Aurora::NWScript::Object *owner = 0);
	/** Per-frame update: lines that only continue advance after their voice-over. */
	void update(float frameTime);
	/** Drop the conversation without running any scripts (e.g. on module unload). */
	void stopConversation();

	bool isObjectInConversation(Aurora::NWScript::Object *object) const;

	void callbackActive(Widget &widget);
	void callbackKeyInput(const Events::Key &key, const Events::EventType &type);

protected:
	virtual void getTextColor(float &r, float &g, float &b) const = 0;

	virtual void preprocessEntry(Common::UString &text);

	/** Updates the gui when a resize occurs or it is created. */
	void update(int width, int height);

private:
	Module &_module;
	bool _isActive;
	bool _pickedReply { false }; ///< Has the player picked a reply in this conversation?
	bool  _autoAdvance { false };     ///< Continue by itself (voice-over / delay)?
	float _autoAdvanceDelay { 0.0f }; ///< Seconds left; negative while the voice plays.
	std::unique_ptr<Graphics::Aurora::KotORDialogFrame> _frame;
	std::unique_ptr<Aurora::DLGFile> _dlg;
	std::vector<uint32_t> _replyIds;
	std::unique_ptr<Sound::ChannelHandle> _voice;
	std::unique_ptr<Sound::ChannelHandle> _sound;
	Common::UString _owner;
	std::vector<uint32_t> _stuntActors; ///< IDs of creatures playing a stunt model.
	Common::UString _curSpeaker;

	void refresh();
	Object *findParticipant(const Common::UString &tag) const;
	void updateCamera(const Aurora::DLGFile::Line &entry, Object *speaker, Object *listener);
	/** Give the conversation's stunt actors their stunt models' animations. */
	void applyStunts();
	/** Take them back (the conversation ended). */
	void releaseStunts();
	/** The line's screen fade (FadeType, FadeDelay, FadeLength, FadeColor). */
	void applyFade(const Aurora::DLGFile::Line &entry);
	void playParticipantAnimations(const Aurora::DLGFile::Line &entry);
	void armAutoAdvance(const Aurora::DLGFile::Line &entry,
	                    const std::vector<const Aurora::DLGFile::Line *> &replies);
	void playSounds();
	void stopSounds();
	void pickReply(int index);

	void makeLookAtPC(const Common::UString &tag);
	void playDefaultAnimations(const Common::UString &tag);
	void playTalkAnimations(const Common::UString &tag);

	void notifyResized(int oldWidth, int oldHeight, int newWidth, int newHeight);
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_GUI_DIALOG_H
