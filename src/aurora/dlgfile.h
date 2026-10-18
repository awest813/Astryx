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
 *  Handling BioWare's DLGs (dialog / conversation files).
 */

#ifndef AURORA_DLGFILE_H
#define AURORA_DLGFILE_H

#include <vector>

#include "src/common/types.h"
#include "src/common/ustring.h"

#include "src/aurora/types.h"
#include "src/aurora/locstring.h"

namespace Common {
	class SeekableReadStream;
}

namespace Aurora {

class GFF3Struct;

namespace NWScript {
	class Object;
}

// TODO: KotOR:
//       - "ConversationType", "StuntList", "Skippable", "AmbientTrack",
//         "AnimatedCut", ...

class DLGFile {
public:
	static const uint32_t kEndLine     = 0xFFFFFFFE;
	static const uint32_t kInvalidLine = 0xFFFFFFFF;

	/** KotOR: an animation a conversation participant plays during a line. */
	struct Animation {
		Common::UString participant; ///< Tag of the participant ("OWNER" for the owner).
		uint32_t        animation;   ///< DLG animation number.
	};

	struct Line {
		uint32_t id; ///< ID of this line (entry-local).

		Common::UString speaker; ///< Tag of the speaker, empty if default.
		LocString       text;    ///< The actual text of the entry.

		Common::UString sound; ///< ResRef of the sound to play while speaking this entry.
		Common::UString voice; ///< ResRef of the voice over for KotOR games.

		uint32_t animation; ///< Animation to play while speaking this entry.

		Common::UString quest;      ///< Quest name to modify when speaking this entry.
		uint32_t        questEntry; ///< Entry ID to set the quest to.


		Common::UString listener;    ///< KotOR: Tag of the listener, empty if default.

		Common::UString cameraModel; ///< KotOR: Camera model to use.
		uint32_t        cameraID;    ///< KotOR: Camera ID to use.
		float           cameraAngle; ///< KotOR: Camera angle (shot type) to use.
		uint32_t        cameraAnimation; ///< KotOR: Animation of the camera model (0: none).
		float           camFieldOfView;  ///< KotOR: Field of view for this line (0: default).
		uint32_t        delay;           ///< KotOR: Seconds to show the line (kInvalidLine: default).

		std::vector<Animation> animations; ///< KotOR: Participant animations during this line.
		uint32_t        camVidEffect;///< KotOR: Camera video effect.
		uint32_t        fadeType;    ///< KotOR: Screen fade type.

		uint32_t        emotion;     ///< KotOR2: Emotion to display.
		uint32_t        facialAnim;  ///< KotOR2: Facial animation to play.

		uint32_t        waitFlags;   ///< Flags to wait for (VO, animation etc.).
		uint32_t        cameraHostile; ///< Whether the camera should be hostile.


		bool isEnd; ///< Are there no replies to this line?
	};


	/** Take over this stream and read a DLG file out of it. */
	DLGFile(Common::SeekableReadStream *dlg, NWScript::Object *owner = 0, bool repairNWNPremium = false);
	/** Request this resource from the ResourceManager and read a DLG file out of it. */
	DLGFile(const Common::UString &dlg, NWScript::Object *owner = 0, bool repairNWNPremium = false);
	~DLGFile();

	DLGFile(const DLGFile &) = delete;
	DLGFile &operator=(const DLGFile &) = delete;

	/** Does starting the conversation zoom in the camera onto the speaker or not? */
	bool getNoZoomIn() const;

	/** Return the number of seconds to wait before showing each entry. */
	uint32_t getDelayEntry() const;
	/** Return the number of seconds to wait before showing each reply. */
	uint32_t getDelayReply() const;

	/** KotOR: Is the conversation skippable? */
	bool isSkippable() const;

	/** KotOR: Return the stunt list. */
	Common::UString getStuntList() const;

	/** KotOR: Camera model for animated camera shots. */
	const Common::UString &getCameraModel() const;
	/** KotOR: Conversation type (0: cinematic/human, 1: computer). */
	uint32_t getConversationType() const;
	/** KotOR: Is this an animated cutscene? */
	bool isAnimatedCut() const;

	bool hasEnded() const;

	void startConversation();
	void abortConversation();

	void pickReply(uint32_t id);

	const Line *getCurrentEntry() const;
	const std::vector<const Line *> &getCurrentReplies() const;

	/** Return the first active non-branching entry. */
	const Line *getOneLiner() const;

private:
	/** A script used by an entry or a link. */
	struct Script {
		Common::UString name;            ///< Name of the script.
		std::vector<int> parameters;     ///< Parameter to call the script with.
		Common::UString parameterString; ///< String parameter to call the script with.
		bool negate;                     ///< Negation of the scripts result.
	};

	/** A link to a reply. */
	struct Link {
		uint32_t index;          ///< Index into the entries/replies.
		Script active1, active2; ///< Scripts that determine if this link is active.
	};

	/** A dialog entry. */
	struct Entry {
		bool isPC; ///< Is this a PC or NPC line?

		Script script1, script2; ///< Scripts to run when speaking this entry.

		Line line; ///< The line's contents.

		std::vector<Link> replies; ///< Reply lines.
	};


	NWScript::Object *_owner;

	uint32_t _delayEntry; ///< Number of seconds to wait before showing each entry.
	uint32_t _delayReply; ///< Number of seconds to wait before showing each reply.

	Script _convAbort; ///< Script to run when the conversation was aborted.
	Script _convEnd;   ///< Script to run when the conversation ended normally.

	bool _noZoomIn; ///< Starting the conversation does not zoom the camera onto the speaker.

	bool            _skippable; ///< KotOR: Is the conversation skippable?
	Common::UString _stuntList; ///< KotOR: Stunt list for the conversation.
	Common::UString _cameraModel;      ///< KotOR: Camera model for animated shots.
	uint32_t        _conversationType; ///< KotOR: Conversation type.
	bool            _animatedCut;      ///< KotOR: Animated cutscene.

	std::vector<Entry> _entriesNPC; ///< NPC dialog lines ("entries").
	std::vector<Entry> _entriesPC;  ///< PC dialog lines ("replies").

	std::vector<Link> _entriesStart; ///< NPC starting lines (greetings).

	std::vector<Entry>::iterator _currentEntry; ///< The current entry.
	std::vector<const Line *> _currentReplies;  ///< The current replies.

	bool _ended; ///< Has the conversation ended?


	void load(const GFF3Struct &dlg);

	void readEntries(const GFF3List &list, std::vector<Entry> &entries, bool isPC);
	void readLinks(const GFF3List &list, std::vector<Link> &links);

	void readEntry(const GFF3Struct &gff, Entry &entry);
	void readLink(const GFF3Struct &gff, Link &link);

	bool evaluateEntries(const std::vector<Link> &entries,
	                     std::vector<Entry>::iterator &active);
	bool evaluateReplies(const std::vector<Link> &entries,
	                     std::vector<const Line *> &active);

	bool runScript(const Script &script) const;
};

} // End of namespace Aurora

#endif // AURORA_DLGFILE_H
