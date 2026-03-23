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
 *  Unit tests for Milestone B (Taris) progression-state behavior:
 *  party membership tracking, solo mode toggle, save-entry flag lifecycle,
 *  and global string variable persistence.
 */

#include <map>
#include <vector>
#include <utility>

#include "gtest/gtest.h"

#include "src/common/ustring.h"

namespace {

struct PartyStateModel {
	std::vector<std::pair<int, int> > members;
	bool soloMode { false };
	bool loadedFromSaveGame { false };
	std::map<Common::UString, Common::UString> globalStrings;
	std::map<int, bool> planetSelectable;
	std::map<int, bool> planetAvailable;
	int selectedPlanet { -1 };

	void addPartyMember(int npc, int objectId) {
		members.push_back(std::make_pair(npc, objectId));
	}

	void removePartyMember(int npc) {
		std::vector<std::pair<int, int> > kept;
		kept.reserve(members.size());
		for (const auto &entry : members) {
			if (entry.first != npc)
				kept.push_back(entry);
		}
		members = kept;
	}

	int getPartyMemberCount() const {
		return static_cast<int>(members.size());
	}

	int getPartyMemberByIndex(int index) const {
		if (index < 0 || index >= static_cast<int>(members.size()))
			return -1;
		return members[index].second;
	}

	bool isNPCPartyMember(int npc) const {
		for (const auto &entry : members) {
			if (entry.first == npc)
				return true;
		}
		return false;
	}

	void setSoloMode(bool enabled) {
		soloMode = enabled;
	}

	bool getSoloMode() const {
		return soloMode;
	}

	void markLoadedFromSaveGame() {
		loadedFromSaveGame = true;
	}

	void markNormalModuleTransition() {
		loadedFromSaveGame = false;
	}

	bool getLoadFromSaveGame() const {
		return loadedFromSaveGame;
	}

	void setGlobalString(const Common::UString &id, const Common::UString &value) {
		globalStrings[id] = value;
	}

	Common::UString getGlobalString(const Common::UString &id) const {
		std::map<Common::UString, Common::UString>::const_iterator it = globalStrings.find(id);
		if (it == globalStrings.end())
			return Common::UString();
		return it->second;
	}

	void setPlanetSelectable(int planet, bool selectable) {
		planetSelectable[planet] = selectable;
	}

	bool getPlanetSelectable(int planet) const {
		std::map<int, bool>::const_iterator it = planetSelectable.find(planet);
		return it == planetSelectable.end() ? false : it->second;
	}

	void setPlanetAvailable(int planet, bool available) {
		planetAvailable[planet] = available;
	}

	bool getPlanetAvailable(int planet) const {
		std::map<int, bool>::const_iterator it = planetAvailable.find(planet);
		return it == planetAvailable.end() ? false : it->second;
	}

	int getSelectedPlanet() const {
		return selectedPlanet;
	}

	void showGalaxyMap() {
		if (selectedPlanet != -1 && getPlanetAvailable(selectedPlanet) && getPlanetSelectable(selectedPlanet))
			return;

		for (const auto &entry : planetAvailable) {
			if (entry.second && getPlanetSelectable(entry.first)) {
				selectedPlanet = entry.first;
				return;
			}
		}

		for (const auto &entry : planetAvailable) {
			if (entry.second) {
				selectedPlanet = entry.first;
				return;
			}
		}
	}
};

} // anonymous namespace

GTEST_TEST(KotORBaseTarisParty, partyCountAndIndexReflectAdds) {
	PartyStateModel state;
	state.addPartyMember(-1, 100);
	state.addPartyMember(0, 101);
	state.addPartyMember(1, 102);

	EXPECT_EQ(state.getPartyMemberCount(), 3);
	EXPECT_EQ(state.getPartyMemberByIndex(0), 100);
	EXPECT_EQ(state.getPartyMemberByIndex(1), 101);
	EXPECT_EQ(state.getPartyMemberByIndex(2), 102);
	EXPECT_EQ(state.getPartyMemberByIndex(3), -1);
}

GTEST_TEST(KotORBaseTarisParty, removePartyMemberUpdatesMembership) {
	PartyStateModel state;
	state.addPartyMember(-1, 100);
	state.addPartyMember(0, 101);
	state.addPartyMember(1, 102);

	EXPECT_TRUE(state.isNPCPartyMember(0));
	EXPECT_TRUE(state.isNPCPartyMember(1));

	state.removePartyMember(0);

	EXPECT_EQ(state.getPartyMemberCount(), 2);
	EXPECT_FALSE(state.isNPCPartyMember(0));
	EXPECT_TRUE(state.isNPCPartyMember(1));
	EXPECT_EQ(state.getPartyMemberByIndex(0), 100);
	EXPECT_EQ(state.getPartyMemberByIndex(1), 102);
}

GTEST_TEST(KotORBaseTarisParty, soloModeRoundTrips) {
	PartyStateModel state;

	EXPECT_FALSE(state.getSoloMode());
	state.setSoloMode(true);
	EXPECT_TRUE(state.getSoloMode());
	state.setSoloMode(false);
	EXPECT_FALSE(state.getSoloMode());
}

GTEST_TEST(KotORBaseTarisParty, loadFromSaveGameFlagResetsOnNormalTransition) {
	PartyStateModel state;

	EXPECT_FALSE(state.getLoadFromSaveGame());
	state.markLoadedFromSaveGame();
	EXPECT_TRUE(state.getLoadFromSaveGame());
	state.markNormalModuleTransition();
	EXPECT_FALSE(state.getLoadFromSaveGame());
}

GTEST_TEST(KotORBaseTarisParty, globalStringRoundTripsAndOverwrite) {
	PartyStateModel state;

	EXPECT_EQ(state.getGlobalString("taris_intro"), Common::UString());

	state.setGlobalString("taris_intro", "seen");
	EXPECT_EQ(state.getGlobalString("taris_intro"), Common::UString("seen"));

	state.setGlobalString("taris_intro", "completed");
	EXPECT_EQ(state.getGlobalString("taris_intro"), Common::UString("completed"));
}

GTEST_TEST(KotORBaseTarisParty, galaxyMapFlagsDefaultFalseAndRoundTrip) {
	PartyStateModel state;

	EXPECT_FALSE(state.getPlanetSelectable(3));
	EXPECT_FALSE(state.getPlanetAvailable(3));
	EXPECT_EQ(state.getSelectedPlanet(), -1);

	state.setPlanetSelectable(3, true);
	state.setPlanetAvailable(3, true);
	EXPECT_TRUE(state.getPlanetSelectable(3));
	EXPECT_TRUE(state.getPlanetAvailable(3));
	state.showGalaxyMap();
	EXPECT_EQ(state.getSelectedPlanet(), 3);
}
