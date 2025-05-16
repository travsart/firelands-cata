/*
 * This file is part of the FirelandsCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Affero General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FIRELANDS_PET_DEFINES_H
#define _FIRELANDS_PET_DEFINES_H

enum ReactStates : uint8;

enum PetType
{
    SUMMON_PET              = 0,
    HUNTER_PET              = 1,
    MAX_PET_TYPE            = 4
};

#define MAX_PET_STABLES 20

// stored in character_pet.slot
enum PetSaveMode
{
    PET_SAVE_AS_DELETED        = -1,  // removes pet from DB and erases from playerPetDataStore
    PET_SAVE_UPADTE_SLOT       =  0,  // not used yet
    PET_SAVE_CURRENT_STATE     =  1,  // Saves everything like it is atm, current = true
    PET_SAVE_DISMISS           =  2,  // Saves everything like it is atm, removes auras and current = false
    PET_SAVE_LOGOUT            =  3,  // Saves everything like it is atm, removes auras and current = true
    PET_SAVE_NEW_PET           =  4,   // Saves everything like it is atm, current = true and pushes new into playerPetDataStore
    PET_SAVE_TEMP_UNSUMMON = PET_SAVE_LOGOUT
};

enum PetStableSlot
{
    PET_SLOT_FIRST             = 0,
    PET_SLOT_LAST              = 20,
    PET_SLOT_FIRST_ACTIVE_SLOT = PET_SLOT_FIRST,
    PET_SLOT_LAST_ACTIVE_SLOT  = 4,
    PET_SLOT_FIRST_STABLE_SLOT = 5,
    PET_SLOT_LAST_STABLE_SLOT  = PET_SLOT_LAST
};

enum PetSpellState
{
    PETSPELL_UNCHANGED = 0,
    PETSPELL_CHANGED   = 1,
    PETSPELL_NEW       = 2,
    PETSPELL_REMOVED   = 3
};

enum PetSpellType
{
    PETSPELL_NORMAL = 0,
    PETSPELL_FAMILY = 1,
    PETSPELL_TALENT = 2
};

enum ActionFeedback
{
    FEEDBACK_NONE            = 0,
    FEEDBACK_PET_DEAD        = 1,
    FEEDBACK_NOTHING_TO_ATT  = 2,
    FEEDBACK_CANT_ATT_TARGET = 3
};

enum PetTalk
{
    PET_TALK_SPECIAL_SPELL  = 0,
    PET_TALK_ATTACK         = 1
};

enum PetLoadState
{
    PET_LOAD_OK                 = 0,
    PET_LOAD_NO_RESULT          = 1,
    PET_LOAD_ERROR              = 2
};

enum NPCEntries
{
    // Warlock
    NPC_INFERNAL                = 89,
    NPC_IMP                     = 416,
    NPC_FELHUNTER               = 417,
    NPC_VOIDWALKER              = 1860,
    NPC_SUCCUBUS                = 1863,
    NPC_DOOMGUARD               = 11859,
    NPC_FELGUARD                = 17252,
    NPC_EYE_OF_KILROGG          = 4277,
    NPC_EBON_IMP                  = 50675,

    // Mage
    NPC_WATER_ELEMENTAL_TEMP    = 510,
    NPC_MIRROR_IMAGE            = 31216,
    NPC_WATER_ELEMENTAL_PERM    = 37994,

    // Druid
    NPC_TREANT                  = 1964,

    // Priest
    NPC_SHADOWFIEND             = 19668,

    // Shaman
    NPC_FIRE_ELEMENTAL          = 15438,
    NPC_EARTH_ELEMENTAL         = 15352,
    NPC_SPIRIT_WOLF             = 29264,

    // Death Knight
    NPC_GHOUL                   = 26125,
    NPC_BLOODWORM               = 28017,
    NPC_ARMY_OF_THE_DEAD        = 24207,
    NPC_EBON_GARGOYLE           = 27829,
    NPC_RISEN_ALLY              = 30230,
    NPC_RUNIC_WEAPON            = 27893,
    NPC_SHADOWFIEND             = 19668,

    // Hunter
    NPC_VENOMOUS_SNAKE          = 19833,
    NPC_VIPER                   = 19921,

    // Generic
    NPC_GENERIC_IMP             = 12922,
    NPC_GENERIC_VOIDWALKER      = 8996
};

enum PetScalingSpells
{
    SPELL_PET_AVOIDANCE                 = 32233,
    SPELL_PET_SCALING_MASTER_06         = 67561, // Serverside - Pet Scaling - Master Spell 06 - Spell Hit, Expertise, Spell Penetration

    // Hunter
    SPELL_HUNTER_PET_SCALING_01         = 34902,
    SPELL_HUNTER_PET_SCALING_02         = 34903,
    SPELL_HUNTER_PET_SCALING_03         = 34904,
    SPELL_HUNTER_PET_SCALING_04         = 61017, // Hit / Expertise

    // Warlock
    SPELL_WARLOCK_PET_SCALING_01        = 34947,
    SPELL_WARLOCK_PET_SCALING_02        = 34956,
    SPELL_WARLOCK_PET_SCALING_03        = 34957,
    SPELL_WARLOCK_PET_SCALING_04        = 34958,
    SPELL_WARLOCK_PET_SCALING_05        = 61013, // Hit / Expertise
    SPELL_GLYPH_OF_FELGUARD             = 56246,
    SPELL_GLYPH_OF_VOIDWALKER           = 56247,
    SPELL_INFERNAL_SCALING_01           = 36186,
    SPELL_INFERNAL_SCALING_02           = 36188,
    SPELL_INFERNAL_SCALING_03           = 36189,
    SPELL_INFERNAL_SCALING_04           = 36190,
    SPELL_RITUAL_ENSLAVEMENT            = 22987,

    // Shaman
    SPELL_FERAL_SPIRIT_SPIRIT_HUNT      = 58877,
    SPELL_FERAL_SPIRIT_SCALING_01       = 35674,
    SPELL_FERAL_SPIRIT_SCALING_02       = 35675,
    SPELL_FERAL_SPIRIT_SCALING_03       = 35676,
    SPELL_FIRE_ELEMENTAL_SCALING_01     = 35665,
    SPELL_FIRE_ELEMENTAL_SCALING_02     = 35666,
    SPELL_FIRE_ELEMENTAL_SCALING_03     = 35667,
    SPELL_FIRE_ELEMENTAL_SCALING_04     = 35668,
    SPELL_EARTH_ELEMENTAL_SCALING_01    = 65225,
    SPELL_EARTH_ELEMENTAL_SCALING_02    = 65226,
    SPELL_EARTH_ELEMENTAL_SCALING_03    = 65227,
    SPELL_EARTH_ELEMENTAL_SCALING_04    = 65228,
    SPELL_ORC_RACIAL_COMMAND_SHAMAN     = 65223,

    // Priest
    SPELL_SHADOWFIEND_SCALING_01        = 35661,
    SPELL_SHADOWFIEND_SCALING_02        = 35662,
    SPELL_SHADOWFIEND_SCALING_03        = 35663,
    SPELL_SHADOWFIEND_SCALING_04        = 35664,

    // Druid
    SPELL_TREANT_SCALING_01             = 35669,
    SPELL_TREANT_SCALING_02             = 35670,
    SPELL_TREANT_SCALING_03             = 35671,
    SPELL_TREANT_SCALING_04             = 35672,

    // Mage
    SPELL_MAGE_PET_SCALING_01           = 35657,
    SPELL_MAGE_PET_SCALING_02           = 35658,
    SPELL_MAGE_PET_SCALING_03           = 35659,
    SPELL_MAGE_PET_SCALING_04           = 35660,

    // Death Knight
    SPELL_ORC_RACIAL_COMMAND_DK         = 65221,
    SPELL_NIGHT_OF_THE_DEAD_AVOIDANCE   = 62137,
    SPELL_DK_PET_SCALING_01             = 54566,
    SPELL_DK_PET_SCALING_02             = 51996,
    SPELL_DK_PET_SCALING_03             = 61697,
    SPELL_DK_AVOIDANCE                  = 65220,
    SPELL_DK_ARMY_OF_THE_DEAD_PASSIVE   = 49040,
};

// Used by companions (minipets) and quest slot summons
constexpr float DEFAULT_FOLLOW_DISTANCE = 2.5f;
constexpr float DEFAULT_FOLLOW_DISTANCE_PET = 3.f;
#endif
