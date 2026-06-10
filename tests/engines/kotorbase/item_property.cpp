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
 *  Unit tests for KotOR item-property combat bonus aggregation.
 */

#include "gtest/gtest.h"

#include "src/engines/kotorbase/types.h"

using Engines::KotORBase::kItemPropertyACBonus;
using Engines::KotORBase::kItemPropertyAttackBonus;
using Engines::KotORBase::kItemPropertyDamageBonus;
using Engines::KotORBase::kItemPropertyEnhancementBonus;

static int propertyBonusSum(const std::vector<std::pair<int, int>> &properties, int type) {
	int sum = 0;
	for (const auto &prop : properties) {
		if (prop.first == type)
			sum += prop.second;
	}
	return sum;
}

static int calcAC(int baseArmor, const std::vector<std::pair<int, int>> &properties) {
	return baseArmor + propertyBonusSum(properties, kItemPropertyACBonus);
}

static int calcAttackBonus(const std::vector<std::pair<int, int>> &properties) {
	return propertyBonusSum(properties, kItemPropertyAttackBonus) +
	       propertyBonusSum(properties, kItemPropertyEnhancementBonus);
}

static int calcDamageBonus(const std::vector<std::pair<int, int>> &properties) {
	return propertyBonusSum(properties, kItemPropertyEnhancementBonus) +
	       propertyBonusSum(properties, kItemPropertyDamageBonus);
}

GTEST_TEST(KotORItemProperty, acBonusStacksPropertyValue) {
	std::vector<std::pair<int, int>> props = {
		{ kItemPropertyACBonus, 2 },
		{ kItemPropertyEnhancementBonus, 1 }
	};

	EXPECT_EQ(calcAC(4, props), 6);
}

GTEST_TEST(KotORItemProperty, attackBonusIncludesEnhancement) {
	std::vector<std::pair<int, int>> props = {
		{ kItemPropertyAttackBonus, 1 },
		{ kItemPropertyEnhancementBonus, 2 }
	};

	EXPECT_EQ(calcAttackBonus(props), 3);
}

GTEST_TEST(KotORItemProperty, damageBonusIncludesEnhancementAndDamage) {
	std::vector<std::pair<int, int>> props = {
		{ kItemPropertyEnhancementBonus, 2 },
		{ kItemPropertyDamageBonus, 1 }
	};

	EXPECT_EQ(calcDamageBonus(props), 3);
}
