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
 *  Unit tests for KotORBase::Inventory — item add/remove/query operations.
 *
 *  These tests exercise the pure data logic of the Inventory class without
 *  requiring any live game data (GFF files, models, etc.).  They guard the
 *  milestone-critical inventory path used during footlocker looting and the
 *  ActionPickUpItem executor.
 */

#include "gtest/gtest.h"

#include "src/engines/kotorbase/inventory.h"

using Engines::KotORBase::Inventory;

// ---------------------------------------------------------------------------
// Initial state
// ---------------------------------------------------------------------------

GTEST_TEST(KotORInventory, defaultIsEmpty) {
	Inventory inv;
	EXPECT_TRUE(inv.getItems().empty());
}

GTEST_TEST(KotORInventory, hasItemReturnsFalseOnEmpty) {
	Inventory inv;
	EXPECT_FALSE(inv.hasItem("g_i_boots01"));
}

// ---------------------------------------------------------------------------
// addItem
// ---------------------------------------------------------------------------

GTEST_TEST(KotORInventory, addSingleItem) {
	Inventory inv;
	inv.addItem("g_i_boots01");

	EXPECT_TRUE(inv.hasItem("g_i_boots01"));
	EXPECT_EQ(inv.getItems().at("g_i_boots01").count, 1);
}

GTEST_TEST(KotORInventory, addItemWithExplicitCount) {
	Inventory inv;
	inv.addItem("medpac", 3);

	EXPECT_TRUE(inv.hasItem("medpac"));
	EXPECT_EQ(inv.getItems().at("medpac").count, 3);
}

GTEST_TEST(KotORInventory, addItemStacksOnDuplicate) {
	Inventory inv;
	inv.addItem("medpac");
	inv.addItem("medpac");

	EXPECT_EQ(inv.getItems().at("medpac").count, 2);
}

GTEST_TEST(KotORInventory, addMultipleStacksAndCount) {
	Inventory inv;
	inv.addItem("medpac", 2);
	inv.addItem("medpac", 3);

	EXPECT_EQ(inv.getItems().at("medpac").count, 5);
}

GTEST_TEST(KotORInventory, addDifferentItemsAreIndependent) {
	Inventory inv;
	inv.addItem("g_i_boots01");
	inv.addItem("g_i_glove01");

	EXPECT_TRUE(inv.hasItem("g_i_boots01"));
	EXPECT_TRUE(inv.hasItem("g_i_glove01"));
	EXPECT_EQ(inv.getItems().size(), (size_t)2);
}

GTEST_TEST(KotORInventory, addEmptyTagIsNoOp) {
	Inventory inv;
	inv.addItem("");  // should not assert or modify state

	EXPECT_TRUE(inv.getItems().empty());
}

// ---------------------------------------------------------------------------
// removeItem
// ---------------------------------------------------------------------------

GTEST_TEST(KotORInventory, removeLastCopyErasesEntry) {
	Inventory inv;
	inv.addItem("medpac");
	inv.removeItem("medpac");

	EXPECT_FALSE(inv.hasItem("medpac"));
	EXPECT_TRUE(inv.getItems().empty());
}

GTEST_TEST(KotORInventory, removeReducesCount) {
	Inventory inv;
	inv.addItem("medpac", 5);
	inv.removeItem("medpac", 2);

	EXPECT_EQ(inv.getItems().at("medpac").count, 3);
}

GTEST_TEST(KotORInventory, removeExactCountErasesEntry) {
	Inventory inv;
	inv.addItem("medpac", 3);
	inv.removeItem("medpac", 3);

	EXPECT_FALSE(inv.hasItem("medpac"));
}

GTEST_TEST(KotORInventory, removeMoreThanPresentErasesEntry) {
	// Removing more than the count erases the entry rather than going negative.
	Inventory inv;
	inv.addItem("medpac", 2);
	inv.removeItem("medpac", 10);

	EXPECT_FALSE(inv.hasItem("medpac"));
}

GTEST_TEST(KotORInventory, removeAbsentItemIsNoOp) {
	Inventory inv;
	inv.addItem("g_i_boots01");
	inv.removeItem("other_item");

	EXPECT_TRUE(inv.hasItem("g_i_boots01"));
	EXPECT_EQ(inv.getItems().size(), (size_t)1);
}

GTEST_TEST(KotORInventory, removeEmptyTagIsNoOp) {
	Inventory inv;
	inv.addItem("g_i_boots01");
	inv.removeItem("");

	EXPECT_EQ(inv.getItems().size(), (size_t)1);
}

// ---------------------------------------------------------------------------
// removeAllItems
// ---------------------------------------------------------------------------

GTEST_TEST(KotORInventory, removeAllClearsInventory) {
	Inventory inv;
	inv.addItem("g_i_boots01");
	inv.addItem("medpac", 3);
	inv.removeAllItems();

	EXPECT_TRUE(inv.getItems().empty());
}

// ---------------------------------------------------------------------------
// ItemGroup tag field
// ---------------------------------------------------------------------------

GTEST_TEST(KotORInventory, itemGroupTagMatchesKey) {
	Inventory inv;
	inv.addItem("g_i_boots01");

	const auto &group = inv.getItems().at("g_i_boots01");
	EXPECT_EQ(group.tag, Common::UString("g_i_boots01"));
}

// ---------------------------------------------------------------------------
// Assignment operator
// ---------------------------------------------------------------------------

GTEST_TEST(KotORInventory, assignmentCopiesItems) {
	Inventory src;
	src.addItem("medpac", 2);
	src.addItem("g_i_boots01");

	Inventory dst;
	dst = src;

	EXPECT_TRUE(dst.hasItem("medpac"));
	EXPECT_EQ(dst.getItems().at("medpac").count, 2);
	EXPECT_TRUE(dst.hasItem("g_i_boots01"));
}

GTEST_TEST(KotORInventory, assignmentIsIndependent) {
	// Modifying the copy must not affect the original
	Inventory src;
	src.addItem("medpac", 2);

	Inventory dst;
	dst = src;
	dst.removeAllItems();

	EXPECT_TRUE(src.hasItem("medpac"));
}
