# xoreos - A reimplementation of BioWare's Aurora engine
#
# xoreos is the legal property of its developers, whose names
# can be found in the AUTHORS file distributed with this source
# distribution.
#
# xoreos is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 3
# of the License, or (at your option) any later version.
#
# xoreos is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with xoreos. If not, see <http://www.gnu.org/licenses/>.

# Unit tests for the KotORBase namespace.

kotorbase_LIBS = \
    $(test_LIBS) \
    src/engines/kotorbase/libkotorbase.la \
    src/engines/libengines.la \
    src/events/libevents.la \
    src/video/libvideo.la \
    src/sound/libsound.la \
    src/graphics/libgraphics.la \
    src/aurora/libaurora.la \
    src/common/libcommon.la \
    tests/version/libversion.la \
    external/imgui/libimgui.la \
    $(LDADD)

check_PROGRAMS                                       += tests/engines/kotorbase/test_creatureinfo
tests_engines_kotorbase_test_creatureinfo_SOURCES     = tests/engines/kotorbase/creatureinfo.cpp
tests_engines_kotorbase_test_creatureinfo_LDADD       = $(kotorbase_LIBS)
tests_engines_kotorbase_test_creatureinfo_CXXFLAGS    = $(test_CXXFLAGS)

check_PROGRAMS                                       += tests/engines/kotorbase/test_combat
tests_engines_kotorbase_test_combat_SOURCES           = tests/engines/kotorbase/combat.cpp
tests_engines_kotorbase_test_combat_LDADD             = $(kotorbase_LIBS)
tests_engines_kotorbase_test_combat_CXXFLAGS          = $(test_CXXFLAGS)

check_PROGRAMS                                       += tests/engines/kotorbase/test_inventory
tests_engines_kotorbase_test_inventory_SOURCES        = tests/engines/kotorbase/inventory.cpp
tests_engines_kotorbase_test_inventory_LDADD          = $(kotorbase_LIBS)
tests_engines_kotorbase_test_inventory_CXXFLAGS       = $(test_CXXFLAGS)

check_PROGRAMS                                       += tests/engines/kotorbase/test_effect
tests_engines_kotorbase_test_effect_SOURCES           = tests/engines/kotorbase/effect.cpp
tests_engines_kotorbase_test_effect_LDADD             = $(kotorbase_LIBS)
tests_engines_kotorbase_test_effect_CXXFLAGS          = $(test_CXXFLAGS)

check_PROGRAMS                                           += tests/engines/kotorbase/test_nwscript_functions
tests_engines_kotorbase_test_nwscript_functions_SOURCES   = tests/engines/kotorbase/nwscript_functions.cpp
tests_engines_kotorbase_test_nwscript_functions_LDADD     = $(kotorbase_LIBS)
tests_engines_kotorbase_test_nwscript_functions_CXXFLAGS  = $(test_CXXFLAGS)

check_PROGRAMS                                           += tests/engines/kotorbase/test_alignment_xp
tests_engines_kotorbase_test_alignment_xp_SOURCES         = tests/engines/kotorbase/alignment_xp.cpp
tests_engines_kotorbase_test_alignment_xp_LDADD           = $(kotorbase_LIBS)
tests_engines_kotorbase_test_alignment_xp_CXXFLAGS        = $(test_CXXFLAGS)

check_PROGRAMS                                           += tests/engines/kotorbase/test_endar_spire_golden
tests_engines_kotorbase_test_endar_spire_golden_SOURCES   = tests/engines/kotorbase/endar_spire_golden.cpp
tests_engines_kotorbase_test_endar_spire_golden_LDADD     = $(kotorbase_LIBS)
tests_engines_kotorbase_test_endar_spire_golden_CXXFLAGS  = $(test_CXXFLAGS)

check_PROGRAMS                                           += tests/engines/kotorbase/test_taris_progression
tests_engines_kotorbase_test_taris_progression_SOURCES    = tests/engines/kotorbase/taris_progression.cpp
tests_engines_kotorbase_test_taris_progression_LDADD      = $(kotorbase_LIBS)
tests_engines_kotorbase_test_taris_progression_CXXFLAGS   = $(test_CXXFLAGS)

check_PROGRAMS                                            += tests/engines/kotorbase/test_milestone3_functions
tests_engines_kotorbase_test_milestone3_functions_SOURCES  = tests/engines/kotorbase/milestone3_functions.cpp
tests_engines_kotorbase_test_milestone3_functions_LDADD    = $(kotorbase_LIBS)
tests_engines_kotorbase_test_milestone3_functions_CXXFLAGS = $(test_CXXFLAGS)
