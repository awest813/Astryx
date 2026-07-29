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
 *  Round-trip tests for KotOR save serialization writers.
 */

#include <map>
#include <vector>
#include <cstring>

#include "gtest/gtest.h"

#include "src/common/memreadstream.h"
#include "src/common/memwritestream.h"

#include "src/aurora/gff3file.h"
#include "src/aurora/gff3writer.h"

#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/creatureinfo.h"
#include "src/engines/kotorbase/inventory.h"
#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/partystate.h"
#include "src/engines/kotorbase/types.h"

using Engines::KotORBase::CreatureInfo;
using Engines::KotORBase::Inventory;
using Engines::KotORBase::Object;
using Engines::KotORBase::kAbilityDexterity;
using Engines::KotORBase::kAbilityStrength;
using Engines::KotORBase::kClassSoldier;
using Engines::KotORBase::kInventorySlotBody;
using Engines::KotORBase::kSkillSecurity;

static Aurora::GFF3File roundTrip(Aurora::GFF3Writer &writer) {
	Common::MemoryWriteStreamDynamic out(true);
	writer.write(out);

	// Copy the buffer: MemoryWriteStreamDynamic frees its storage on destroy,
	// while GFF3File/MemoryReadStream need an independently owned block.
	const size_t size = out.size();
	byte *copy = new byte[size];
	std::memcpy(copy, out.getData(), size);
	return Aurora::GFF3File(new Common::MemoryReadStream(copy, size, true));
}

GTEST_TEST(KotORSaveSerialization, inventoryRoundTrip) {
	Inventory inv;
	inv.addItem("g_i_medpac01", 2);
	inv.addItem("g_i_boots01");

	Aurora::GFF3Writer writer(MKTAG('U', 'T', 'C', ' '));
	Aurora::GFF3WriterListPtr list = writer.getTopLevel()->addList("ItemList");
	inv.save(*list);

	Aurora::GFF3File gff = roundTrip(writer);
	Inventory loaded;
	loaded.read(gff.getTopLevel().getList("ItemList"));

	EXPECT_TRUE(loaded.hasItem("g_i_medpac01"));
	EXPECT_EQ(loaded.getItems().at("g_i_medpac01").count, 2);
	EXPECT_TRUE(loaded.hasItem("g_i_boots01"));
	EXPECT_EQ(loaded.getItems().at("g_i_boots01").count, 1);
}

GTEST_TEST(KotORSaveSerialization, creatureInfoHitPointsRoundTrip) {
	CreatureInfo info;
	info.setHitPoints(12, 20);
	info.setAbilityScore(kAbilityStrength, 14);

	Aurora::GFF3Writer writer(MKTAG('U', 'T', 'C', ' '));
	info.save(*writer.getTopLevel());

	Aurora::GFF3File gff = roundTrip(writer);
	CreatureInfo loaded;
	loaded.read(gff.getTopLevel());

	EXPECT_TRUE(loaded.hasHitPoints());
	EXPECT_EQ(loaded.getCurrentHitPoints(), 12);
	EXPECT_EQ(loaded.getMaxHitPoints(), 20);
	EXPECT_EQ(loaded.getAbilityScore(kAbilityStrength), 14);
}

GTEST_TEST(KotORSaveSerialization, creatureInfoRoundTrip) {
	CreatureInfo info;
	info.setAbilityScore(kAbilityStrength, 14);
	info.setAbilityScore(kAbilityDexterity, 12);
	info.setSkillRank(kSkillSecurity, 4);
	info.incrementClassLevel(kClassSoldier);
	info.addInventoryItem("g_i_medpac01", 2);
	info.getInventory().addGold(250);
	info.equipItem("g_i_boots01", kInventorySlotBody);
	info.setAlignment(55);
	info.setForcePoints(3);
	info.setMaxForcePoints(5);

	Aurora::GFF3Writer writer(MKTAG('U', 'T', 'C', ' '));
	info.save(*writer.getTopLevel());

	Aurora::GFF3File gff = roundTrip(writer);
	CreatureInfo loaded;
	loaded.read(gff.getTopLevel());

	EXPECT_EQ(loaded.getAbilityScore(kAbilityStrength), 14);
	EXPECT_EQ(loaded.getAbilityScore(kAbilityDexterity), 12);
	EXPECT_EQ(loaded.getSkillRank(kSkillSecurity), 4);
	EXPECT_EQ(loaded.getClassLevel(kClassSoldier), 1);
	EXPECT_TRUE(loaded.getInventory().hasItem("g_i_medpac01"));
	EXPECT_EQ(loaded.getInventory().getGold(), 250U);
	EXPECT_TRUE(loaded.isInventorySlotEquipped(kInventorySlotBody));
	EXPECT_EQ(loaded.getEquippedItem(kInventorySlotBody), Common::UString("g_i_boots01"));
	EXPECT_EQ(loaded.getAlignment(), 55);
	EXPECT_EQ(loaded.getForcePoints(), 3U);
	EXPECT_EQ(loaded.getMaxForcePoints(), 5U);
}

GTEST_TEST(KotORSaveSerialization, objectStateRoundTrip) {
	Object object(Engines::KotORBase::kObjectTypePlaceable);
	object.setMaxHitPoints(20);
	object.setCurrentHitPoints(7);
	object.setUsable(false);

	Aurora::GFF3Writer writer(MKTAG('G', 'F', 'F', ' '));
	object.saveState(*writer.getTopLevel());

	Aurora::GFF3File gff = roundTrip(writer);
	Object loaded(Engines::KotORBase::kObjectTypePlaceable);
	loaded.setMaxHitPoints(20);
	loaded.loadState(gff.getTopLevel());

	EXPECT_EQ(loaded.getCurrentHitPoints(), 7);
	EXPECT_FALSE(loaded.isUsable());
}

GTEST_TEST(KotORSaveSerialization, partyPCStateRoundTrip) {
	CreatureInfo info;
	info.addInventoryItem("g_i_medpac01", 3);
	info.getInventory().addGold(500);
	info.equipItem("g_i_boots01", kInventorySlotBody);
	info.setAbilityScore(kAbilityStrength, 16);

	Aurora::GFF3Writer writer(MKTAG('P', 'T', 'A', 'B'));
	Aurora::GFF3WriterStructPtr partyRoot = writer.getTopLevel();
	partyRoot->addUint32("PT_GOLD", info.getInventory().getGold());
	Aurora::GFF3WriterStructPtr pcState = partyRoot->addStruct("PT_PC_STATE");
	info.save(*pcState);

	Aurora::GFF3File gff = roundTrip(writer);
	CreatureInfo loaded;
	loaded.read(gff.getTopLevel().getStruct("PT_PC_STATE"));

	EXPECT_EQ(loaded.getInventory().getGold(), 500U);
	EXPECT_TRUE(loaded.getInventory().hasItem("g_i_medpac01"));
	EXPECT_EQ(loaded.getInventory().getItems().at("g_i_medpac01").count, 3);
	EXPECT_TRUE(loaded.isInventorySlotEquipped(kInventorySlotBody));
	EXPECT_EQ(loaded.getAbilityScore(kAbilityStrength), 16);
}

GTEST_TEST(KotORSaveSerialization, partyRosterRoundTrip) {
	CreatureInfo companionInfo;
	companionInfo.addInventoryItem("g_i_medpac01", 1);
	companionInfo.equipItem("g_i_boots01", kInventorySlotBody);
	companionInfo.setAbilityScore(kAbilityStrength, 13);
	companionInfo.setHitPoints(18, 24);

	Aurora::GFF3Writer writer(MKTAG('P', 'T', 'A', 'B'));
	Aurora::GFF3WriterStructPtr partyRoot = writer.getTopLevel();

	Aurora::GFF3WriterListPtr availList = partyRoot->addList("PT_AVAIL_NPCS");
	{
		Aurora::GFF3WriterStructPtr avail = availList->addStruct();
		avail->addSint32("Index", 0);
		avail->addResRef("NPCResRef", "p_bastilla");
	}
	{
		Aurora::GFF3WriterStructPtr avail = availList->addStruct();
		avail->addSint32("Index", 1);
		avail->addResRef("NPCResRef", "p_carth");
	}

	Aurora::GFF3WriterListPtr membersList = partyRoot->addList("PT_MEMBERS");
	{
		Aurora::GFF3WriterStructPtr member = membersList->addStruct();
		member->addSint32("NPCSlot", -1);
	}
	{
		Aurora::GFF3WriterStructPtr member = membersList->addStruct();
		member->addSint32("NPCSlot", 0);
		member->addResRef("TemplateResRef", "p_bastilla");
		Aurora::GFF3WriterStructPtr state = member->addStruct("CreatureState");
		companionInfo.save(*state);
	}
	partyRoot->addUint32("PT_LEADER_INDEX", 0);

	Aurora::GFF3File gff = roundTrip(writer);
	const Aurora::GFF3Struct &loadedRoot = gff.getTopLevel();

	std::map<int, Common::UString> availableNPCs;
	const Aurora::GFF3List &availLoaded = loadedRoot.getList("PT_AVAIL_NPCS");
	for (Aurora::GFF3List::const_iterator it = availLoaded.begin(); it != availLoaded.end(); ++it) {
		if (!*it)
			continue;
		availableNPCs[(*it)->getSint("Index")] = (*it)->getString("NPCResRef");
	}

	std::vector<Engines::KotORBase::SavedPartyMemberState> members;
	const Aurora::GFF3List &membersLoaded = loadedRoot.getList("PT_MEMBERS");
	for (Aurora::GFF3List::const_iterator it = membersLoaded.begin(); it != membersLoaded.end(); ++it) {
		if (!*it)
			continue;

		Engines::KotORBase::SavedPartyMemberState member;
		member.npcSlot = (*it)->getSint("NPCSlot", -1);
		member.templateResRef = (*it)->getString("TemplateResRef");
		if ((*it)->hasField("CreatureState"))
			member.info.read((*it)->getStruct("CreatureState"));
		members.push_back(member);
	}

	EXPECT_EQ(availableNPCs.size(), 2U);
	EXPECT_EQ(availableNPCs[0], Common::UString("p_bastilla"));
	EXPECT_EQ(availableNPCs[1], Common::UString("p_carth"));
	EXPECT_EQ(members.size(), 2U);
	EXPECT_EQ(members[0].npcSlot, -1);
	EXPECT_EQ(members[1].npcSlot, 0);
	EXPECT_EQ(members[1].templateResRef, Common::UString("p_bastilla"));
	EXPECT_TRUE(members[1].info.getInventory().hasItem("g_i_medpac01"));
	EXPECT_TRUE(members[1].info.isInventorySlotEquipped(kInventorySlotBody));
	EXPECT_EQ(members[1].info.getAbilityScore(kAbilityStrength), 13);
	EXPECT_TRUE(members[1].info.hasHitPoints());
	EXPECT_EQ(members[1].info.getCurrentHitPoints(), 18);
	EXPECT_EQ(members[1].info.getMaxHitPoints(), 24);
	EXPECT_EQ(loadedRoot.getUint("PT_LEADER_INDEX", 0), 0U);
}

GTEST_TEST(KotORSaveSerialization, journalStateRoundTrip) {
	Aurora::GFF3Writer writer(MKTAG('G', 'V', 'A', 'R'));
	Aurora::GFF3WriterStructPtr root = writer.getTopLevel();
	root->addExoString("ReturnDestinationModule", "ebo_m12aa");

	Aurora::GFF3WriterListPtr journalList = root->addList("JournalEntries");
	{
		Aurora::GFF3WriterStructPtr item = journalList->addStruct();
		item->addExoString("Quest", "k_main_quest");
		item->addUint32("State", 5U);
	}

	Aurora::GFF3WriterListPtr worldJournalList = root->addList("WorldJournalEntries");
	{
		Aurora::GFF3WriterStructPtr item = worldJournalList->addStruct();
		item->addExoString("Tag", "endar_note");
		item->addExoString("Text", "The Endar Spire is under attack.");
	}

	Aurora::GFF3File gff = roundTrip(writer);
	const Aurora::GFF3Struct &loadedRoot = gff.getTopLevel();

	ASSERT_TRUE(loadedRoot.hasField("ReturnDestinationModule"));
	ASSERT_TRUE(loadedRoot.hasField("JournalEntries"));
	ASSERT_TRUE(loadedRoot.hasField("WorldJournalEntries"));
	EXPECT_EQ(loadedRoot.getString("ReturnDestinationModule"), Common::UString("ebo_m12aa"));

	std::map<Common::UString, uint32_t> journal;
	const Aurora::GFF3List &journalLoaded = loadedRoot.getList("JournalEntries");
	for (Aurora::GFF3List::const_iterator it = journalLoaded.begin(); it != journalLoaded.end(); ++it) {
		if (!*it)
			continue;
		journal[(*it)->getString("Quest")] = (*it)->getUint("State");
	}

	std::vector<std::pair<Common::UString, Common::UString>> worldJournal;
	const Aurora::GFF3List &worldLoaded = loadedRoot.getList("WorldJournalEntries");
	for (Aurora::GFF3List::const_iterator it = worldLoaded.begin(); it != worldLoaded.end(); ++it) {
		if (!*it)
			continue;
		worldJournal.emplace_back((*it)->getString("Tag"), (*it)->getString("Text"));
	}

	EXPECT_EQ(journal.size(), 1U);
	EXPECT_EQ(journal["k_main_quest"], 5U);
	EXPECT_EQ(worldJournal.size(), 1U);
	EXPECT_EQ(worldJournal[0].first, Common::UString("endar_note"));
	EXPECT_EQ(worldJournal[0].second, Common::UString("The Endar Spire is under attack."));
}

GTEST_TEST(KotORSaveSerialization, defaultMapExploredTileCount) {
	EXPECT_EQ(Engines::KotORBase::Area::getDefaultMapExploredTileCount(), 153U);
}

GTEST_TEST(KotORSaveSerialization, journalQuestPictureRoundTrip) {
	Aurora::GFF3Writer writer(MKTAG('G', 'V', 'A', 'R'));
	Aurora::GFF3WriterStructPtr root = writer.getTopLevel();
	root->addExoString("ReturnDestinationModule", "ebo_m12aa");

	Aurora::GFF3WriterListPtr pictureList = root->addList("JournalQuestPictures");
	{
		Aurora::GFF3WriterStructPtr item = pictureList->addStruct();
		item->addExoString("Quest", "k_main_quest");
		item->addUint32("State", 3U);
		item->addExoString("Portrait", "po_bastilla");
	}

	Aurora::GFF3File gff = roundTrip(writer);
	const Aurora::GFF3Struct &loadedRoot = gff.getTopLevel();
	ASSERT_TRUE(loadedRoot.hasField("JournalQuestPictures"));

	std::map<std::pair<Common::UString, uint32_t>, Common::UString> pictures;
	const Aurora::GFF3List &loaded = loadedRoot.getList("JournalQuestPictures");
	for (Aurora::GFF3List::const_iterator it = loaded.begin(); it != loaded.end(); ++it) {
		if (!*it)
			continue;
		pictures[{ (*it)->getString("Quest"), (*it)->getUint("State") }] = (*it)->getString("Portrait");
	}

	EXPECT_EQ(pictures.size(), 1U);
	const Common::UString portrait = pictures[{ Common::UString("k_main_quest"), 3U }];
	EXPECT_EQ(portrait, Common::UString("po_bastilla"));
}

GTEST_TEST(KotORSaveSerialization, planetAvailabilityRoundTrip) {
	Aurora::GFF3Writer writer(MKTAG('G', 'V', 'A', 'R'));
	Aurora::GFF3WriterStructPtr root = writer.getTopLevel();
	root->addSint32("SelectedPlanet", 6);

	Aurora::GFF3WriterListPtr avail = root->addList("PlanetAvailable");
	{
		Aurora::GFF3WriterStructPtr item = avail->addStruct();
		item->addSint32("Planet", 5);
		item->addByte("Available", 1);
	}
	{
		Aurora::GFF3WriterStructPtr item = avail->addStruct();
		item->addSint32("Planet", 6);
		item->addByte("Available", 1);
	}

	Aurora::GFF3WriterListPtr selectable = root->addList("PlanetSelectable");
	{
		Aurora::GFF3WriterStructPtr item = selectable->addStruct();
		item->addSint32("Planet", 6);
		item->addByte("Selectable", 1);
	}

	Aurora::GFF3File gff = roundTrip(writer);
	const Aurora::GFF3Struct &loaded = gff.getTopLevel();

	EXPECT_EQ(loaded.getSint("SelectedPlanet"), 6);

	std::map<int, bool> available;
	for (const auto &entry : loaded.getList("PlanetAvailable")) {
		if (!entry)
			continue;
		available[entry->getSint("Planet")] = entry->getUint("Available") != 0;
	}
	EXPECT_TRUE(available[5]);
	EXPECT_TRUE(available[6]);

	std::map<int, bool> select;
	for (const auto &entry : loaded.getList("PlanetSelectable")) {
		if (!entry)
			continue;
		select[entry->getSint("Planet")] = entry->getUint("Selectable") != 0;
	}
	EXPECT_TRUE(select[6]);
}
