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

#ifndef __UNIT_H
#define __UNIT_H

#include "DBCEnums.h"
#include "CombatManager.h"
#include "EventProcessor.h"
#include "FollowerRefMgr.h"
#include "FollowerReference.h"
#include "HostileRefMgr.h"
#include "ItemTemplate.h"
#include "Object.h"
#include "SpellAuraDefines.h"
#include "SpellDefines.h"
#include "SpellPacketsCommon.h"
#include "TaskScheduler.h"
#include "ThreatMgr.h"
#include "Timer.h"
#include "UnitDefines.h"
#include "UnitUtils.h"
#include "Util.h"

#include <array>
#include <map>
#include <memory>
#include <queue>
#include <stack>

#define WORLD_TRIGGER 12999

#define MAX_AGGRO_RESET_TIME 10 // in seconds
#define MAX_AGGRO_RADIUS 45.0f  // yards

constexpr float DEFAULT_FOLLOW_ANGLE = float(M_PI / 2);

static constexpr uint32 MAX_CREATURE_SPELLS = 8;
static constexpr uint32 infinityCooldownDelay = 0x9A7EC800; // used for set "infinity cooldowns" for spells and check, MONTH*IN_MILLISECONDS
static constexpr uint32 infinityCooldownDelayCheck = 0x4D3F6400; // MONTH*IN_MILLISECONDS/2;

struct AbstractPursuer;
struct FactionTemplateEntry;
struct LiquidData;
struct LiquidTypeEntry;
struct MountCapabilityEntry;
struct SpellValue;
struct CharmInfo;

class Aura;
class AuraApplication;
class AuraEffect;
class Creature;
class DynamicObject;
class GameClient;
class GameObject;
class Guardian;
class Item;
class Minion;
class MotionMaster;
class MotionTransport;
class Pet;
class PetAura;
class Spell;
class SpellCastTargets;
class SpellHistory;
class SpellInfo;
class StaticTransport;
class Totem;
class Transport;
class TransportBase;
class UnitAI;
class UnitAura;
class Vehicle;
class VehicleJoinEvent;

typedef std::list<Unit*> UnitList;
typedef std::list< std::pair<Aura*, uint8> > DispelChargesList;

enum CharmType : uint8;

enum VictimState
{
    VICTIMSTATE_INTACT = 0, // set when attacker misses
    VICTIMSTATE_HIT = 1,    // victim got clear/blocked hit
    VICTIMSTATE_DODGE = 2,
    VICTIMSTATE_PARRY = 3,
    VICTIMSTATE_INTERRUPT = 4,
    VICTIMSTATE_BLOCKS = 5, // unused? not set when blocked, even on full block
    VICTIMSTATE_EVADES = 6,
    VICTIMSTATE_IS_IMMUNE = 7,
    VICTIMSTATE_DEFLECTS = 8
};

// i would like to remove this: (it is defined in item.h
enum InventorySlot
{
    NULL_BAG = 0,
    NULL_SLOT = 255
};

enum ZLiquidStatus : uint32;

namespace Movement
{
    class ExtraMovementStatusElement;
    class MoveSpline;
} // namespace Movement

class DispelableAura
{
  public:
    DispelableAura(Aura* aura, int32 dispelChance, uint8 dispelCharges);
    ~DispelableAura();

    Aura* GetAura() const { return _aura; }
    bool RollDispel() const;
    uint8 GetDispelCharges() const { return _charges; }

    void IncrementCharges() { ++_charges; }
    bool DecrementCharge()
    {
        if (!_charges)
            return false;

        --_charges;
        return _charges > 0;
    }

  private:
    Aura* _aura;
    int32 _chance;
    uint8 _charges;
};

typedef std::unordered_multimap<uint32 /*type*/, uint32 /*spellId*/> SpellImmuneContainer;

enum UnitModifierType
{
    BASE_VALUE = 0,
    BASE_PCT = 1,
    TOTAL_VALUE = 2,
    TOTAL_PCT = 3,
    MODIFIER_TYPE_END = 4
};

enum WeaponDamageRange
{
    MINDAMAGE,
    MAXDAMAGE,

    MAX_WEAPON_DAMAGE_RANGE
};

enum UnitMods
{
    UNIT_MOD_STAT_STRENGTH, // UNIT_MOD_STAT_STRENGTH..UNIT_MOD_STAT_SPIRIT must be in existed order, it's accessed by index values of Stats enum.
    UNIT_MOD_STAT_AGILITY,
    UNIT_MOD_STAT_STAMINA,
    UNIT_MOD_STAT_INTELLECT,
    UNIT_MOD_STAT_SPIRIT,
    UNIT_MOD_HEALTH,
    UNIT_MOD_MANA, // UNIT_MOD_MANA..UNIT_MOD_RUNIC_POWER must be in existed order, it's accessed by index values of Powers enum.
    UNIT_MOD_RAGE,
    UNIT_MOD_FOCUS,
    UNIT_MOD_ENERGY,
    UNIT_MOD_UNUSED, // Old UNIT_MOD_HAPPINESS
    UNIT_MOD_RUNE,
    UNIT_MOD_RUNIC_POWER,
    UNIT_MOD_SOUL_SHARDS,
    UNIT_MOD_ECLIPSE,
    UNIT_MOD_HOLY_POWER,
    UNIT_MOD_ALTERNATIVE,
    UNIT_MOD_ARMOR, // UNIT_MOD_ARMOR..UNIT_MOD_RESISTANCE_ARCANE must be in existed order, it's accessed by index values of SpellSchools enum.
    UNIT_MOD_RESISTANCE_HOLY,
    UNIT_MOD_RESISTANCE_FIRE,
    UNIT_MOD_RESISTANCE_NATURE,
    UNIT_MOD_RESISTANCE_FROST,
    UNIT_MOD_RESISTANCE_SHADOW,
    UNIT_MOD_RESISTANCE_ARCANE,
    UNIT_MOD_ATTACK_POWER,
    UNIT_MOD_ATTACK_POWER_RANGED,
    UNIT_MOD_DAMAGE_MAINHAND,
    UNIT_MOD_DAMAGE_OFFHAND,
    UNIT_MOD_DAMAGE_RANGED,
    UNIT_MOD_END,
    // synonyms
    UNIT_MOD_STAT_START = UNIT_MOD_STAT_STRENGTH,
    UNIT_MOD_STAT_END = UNIT_MOD_STAT_SPIRIT + 1,
    UNIT_MOD_RESISTANCE_START = UNIT_MOD_ARMOR,
    UNIT_MOD_RESISTANCE_END = UNIT_MOD_RESISTANCE_ARCANE + 1,
    UNIT_MOD_POWER_START = UNIT_MOD_MANA,
    UNIT_MOD_POWER_END = UNIT_MOD_ALTERNATIVE + 1
};

enum BaseModGroup
{
    CRIT_PERCENTAGE,
    RANGED_CRIT_PERCENTAGE,
    OFFHAND_CRIT_PERCENTAGE,
    SHIELD_BLOCK_VALUE,
    BASEMOD_END
};

enum BaseModType
{
    FLAT_MOD,
    PCT_MOD,
    MOD_END
};

enum DeathState
{
    ALIVE = 0,
    JUST_DIED = 1,
    CORPSE = 2,
    DEAD = 3,
    JUST_RESPAWNED = 4
};

extern float baseMoveSpeed[MAX_MOVE_TYPE];
extern float playerBaseMoveSpeed[MAX_MOVE_TYPE];

enum UnitState : uint32
{
    UNIT_STATE_DIED = 0x00000001,            // player has fake death aura
    UNIT_STATE_MELEE_ATTACKING = 0x00000002, // player is melee attacking someone
    UNIT_STATE_CHARMED = 0x00000004,         // having any kind of charm aura on self
    UNIT_STATE_STUNNED = 0x00000008,
    UNIT_STATE_ROAMING = 0x00000010,
    UNIT_STATE_CHASE = 0x00000020,
    UNIT_STATE_FOCUSING = 0x00000040,
    UNIT_STATE_FLEEING = 0x00000080,
    UNIT_STATE_IN_FLIGHT = 0x00000100, // player is in flight mode
    UNIT_STATE_FOLLOW = 0x00000200,
    UNIT_STATE_ROOT = 0x00000400,
    UNIT_STATE_CONFUSED = 0x00000800,
    UNIT_STATE_DISTRACTED = 0x00001000,
    UNIT_STATE_ISOLATED = 0x00002000, // area auras do not affect other players
    UNIT_STATE_ATTACK_PLAYER = 0x00004000,
    UNIT_STATE_CASTING = 0x00008000,
    UNIT_STATE_POSSESSED = 0x00010000, // being possessed by another unit
    UNIT_STATE_CHARGING = 0x00020000,
    UNIT_STATE_JUMPING = 0x00040000,
    UNIT_STATE_MOVE = 0x00100000,
    UNIT_STATE_ROTATING = 0x00200000,
    UNIT_STATE_EVADE = 0x00400000,
    UNIT_STATE_ROAMING_MOVE = 0x00800000,
    UNIT_STATE_CONFUSED_MOVE = 0x01000000,
    UNIT_STATE_FLEEING_MOVE = 0x02000000,
    UNIT_STATE_CHASE_MOVE = 0x04000000,
    UNIT_STATE_FOLLOW_MOVE = 0x08000000,
    UNIT_STATE_IGNORE_PATHFINDING = 0x10000000, // do not use pathfinding in any MovementGenerator

    UNIT_STATE_ALL_STATE_SUPPORTED = UNIT_STATE_DIED | UNIT_STATE_MELEE_ATTACKING | UNIT_STATE_CHARMED | UNIT_STATE_STUNNED | UNIT_STATE_ROAMING | UNIT_STATE_CHASE | UNIT_STATE_FOCUSING |
                                     UNIT_STATE_FLEEING | UNIT_STATE_IN_FLIGHT | UNIT_STATE_FOLLOW | UNIT_STATE_ROOT | UNIT_STATE_CONFUSED | UNIT_STATE_DISTRACTED | UNIT_STATE_ISOLATED |
                                     UNIT_STATE_ATTACK_PLAYER | UNIT_STATE_CASTING | UNIT_STATE_POSSESSED | UNIT_STATE_CHARGING | UNIT_STATE_JUMPING | UNIT_STATE_MOVE | UNIT_STATE_ROTATING |
                                     UNIT_STATE_EVADE | UNIT_STATE_ROAMING_MOVE | UNIT_STATE_CONFUSED_MOVE | UNIT_STATE_FLEEING_MOVE | UNIT_STATE_CHASE_MOVE | UNIT_STATE_FOLLOW_MOVE |
                                     UNIT_STATE_IGNORE_PATHFINDING,

    UNIT_STATE_UNATTACKABLE = UNIT_STATE_IN_FLIGHT,
    UNIT_STATE_MOVING = UNIT_STATE_ROAMING_MOVE | UNIT_STATE_CONFUSED_MOVE | UNIT_STATE_FLEEING_MOVE | UNIT_STATE_CHASE_MOVE | UNIT_STATE_FOLLOW_MOVE,
    UNIT_STATE_CONTROLLED = UNIT_STATE_CONFUSED | UNIT_STATE_STUNNED | UNIT_STATE_FLEEING,
    UNIT_STATE_CANT_CLIENT_CONTROL = UNIT_STATE_CHARMED | UNIT_STATE_FLEEING | UNIT_STATE_CONFUSED | UNIT_STATE_POSSESSED,
    UNIT_STATE_LOST_CONTROL = UNIT_STATE_CONTROLLED | UNIT_STATE_POSSESSED | UNIT_STATE_JUMPING | UNIT_STATE_CHARGING,
    UNIT_STATE_CANNOT_AUTOATTACK = UNIT_STATE_CONTROLLED | UNIT_STATE_CHARGING | UNIT_STATE_CASTING,
    UNIT_STATE_SIGHTLESS = UNIT_STATE_LOST_CONTROL | UNIT_STATE_EVADE,
    UNIT_STATE_CANNOT_TURN = UNIT_STATE_LOST_CONTROL | UNIT_STATE_ROTATING | UNIT_STATE_FOCUSING,
    UNIT_STATE_NOT_MOVE = UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_DIED | UNIT_STATE_DISTRACTED,

    UNIT_STATE_ALL_ERASABLE = UNIT_STATE_ALL_STATE_SUPPORTED & ~(UNIT_STATE_IGNORE_PATHFINDING),
    UNIT_STATE_ALL_STATE = 0xffffffff
};

enum CombatRating
{
    CR_WEAPON_SKILL = 0,
    CR_DEFENSE_SKILL = 1, // Removed in 4.0.1
    CR_DODGE = 2,
    CR_PARRY = 3,
    CR_BLOCK = 4,
    CR_HIT_MELEE = 5,
    CR_HIT_RANGED = 6,
    CR_HIT_SPELL = 7,
    CR_CRIT_MELEE = 8,
    CR_CRIT_RANGED = 9,
    CR_CRIT_SPELL = 10,
    CR_HIT_TAKEN_MELEE = 11,  // Deprecated since Cataclysm
    CR_HIT_TAKEN_RANGED = 12, // Deprecated since Cataclysm
    CR_HIT_TAKEN_SPELL = 13,  // Deprecated since Cataclysm
    CR_RESILIENCE_CRIT_TAKEN = 14,
    CR_RESILIENCE_PLAYER_DAMAGE_TAKEN = 15,
    CR_CRIT_TAKEN_SPELL = 16, // Deprecated since Cataclysm
    CR_HASTE_MELEE = 17,
    CR_HASTE_RANGED = 18,
    CR_HASTE_SPELL = 19,
    CR_WEAPON_SKILL_MAINHAND = 20,
    CR_WEAPON_SKILL_OFFHAND = 21,
    CR_WEAPON_SKILL_RANGED = 22,
    CR_EXPERTISE = 23,
    CR_ARMOR_PENETRATION = 24,
    CR_MASTERY = 25,
};

#define MAX_COMBAT_RATING 26

enum DamageEffectType : uint8
{
    DIRECT_DAMAGE = 0,       // used for normal weapon damage (not for class abilities or spells)
    SPELL_DIRECT_DAMAGE = 1, // spell/class abilities damage
    DOT = 2,
    HEAL = 3,
    NODAMAGE = 4, // used also in case when damage applied to health but not applied to spell channelInterruptFlags/etc
    SELF_DAMAGE = 5
};

namespace Movement
{
    class MoveSpline;
}

enum class MovementChangeType : uint8
{
    INVALID,

    ROOT,
    WATER_WALK,
    SET_HOVER,
    SET_CAN_FLY,
    SET_CAN_TRANSITION_BETWEEN_SWIM_AND_FLY,
    FEATHER_FALL,
    GRAVITY_DISABLE,

    SPEED_CHANGE_WALK,
    SPEED_CHANGE_RUN,
    SPEED_CHANGE_RUN_BACK,
    SPEED_CHANGE_SWIM,
    SPEED_CHANGE_SWIM_BACK,
    RATE_CHANGE_TURN,
    SPEED_CHANGE_FLIGHT_SPEED,
    SPEED_CHANGE_FLIGHT_BACK_SPEED,
    RATE_CHANGE_PITCH,

    SET_COLLISION_HGT,
    TELEPORT,
    KNOCK_BACK
};

struct PlayerMovementPendingChange
{
    PlayerMovementPendingChange();

    uint32 movementCounter = 0;
    MovementChangeType movementChangeType = MovementChangeType::INVALID;
    uint32 time;

    float newValue = 0.0f; // used if speed or height change
    bool apply = false;    // used if movement flag change
    struct KnockbackInfo
    {
        float vcos = 0.0f;
        float vsin = 0.0f;
        float speedXY = 0.0f;
        float speedZ = 0.0f;
    } knockbackInfo; // used if knockback
};

struct DiminishingReturn
{
    DiminishingReturn() : stack(0), hitTime(0), hitCount(DIMINISHING_LEVEL_1) {}

    void Clear()
    {
        stack = 0;
        hitTime = 0;
        hitCount = DIMINISHING_LEVEL_1;
    }

    uint16 stack;
    uint32 hitTime;
    uint32 hitCount;
};

enum MeleeHitOutcome : uint8
{
    MELEE_HIT_EVADE,
    MELEE_HIT_MISS,
    MELEE_HIT_DODGE,
    MELEE_HIT_BLOCK,
    MELEE_HIT_PARRY,
    MELEE_HIT_GLANCING,
    MELEE_HIT_CRIT,
    MELEE_HIT_CRUSHING,
    MELEE_HIT_NORMAL
};

enum ExtraAttackSpells
{
    SPELL_SWORD_SPECIALIZATION   = 16459,
    SPELL_HACK_AND_SLASH         = 66923
};

class DispelInfo
{
  public:
    explicit DispelInfo(Unit* dispeller, uint32 dispellerSpellId, uint8 chargesRemoved) : _dispellerUnit(dispeller), _dispellerSpell(dispellerSpellId), _chargesRemoved(chargesRemoved) {}

    Unit* GetDispeller() const { return _dispellerUnit; }
    uint32 GetDispellerSpellId() const { return _dispellerSpell; }
    uint8 GetRemovedCharges() const { return _chargesRemoved; }
    void SetRemovedCharges(uint8 amount) { _chargesRemoved = amount; }

  private:
    Unit* _dispellerUnit;
    uint32 _dispellerSpell;
    uint8 _chargesRemoved;
};

struct CleanDamage
{
    CleanDamage(uint32 mitigated, uint32 absorbed, WeaponAttackType _attackType, MeleeHitOutcome _hitOutCome)
        : absorbed_damage(absorbed), mitigated_damage(mitigated), attackType(_attackType), hitOutCome(_hitOutCome)
    {
    }

    uint32 absorbed_damage;
    uint32 mitigated_damage;

    WeaponAttackType attackType;
    MeleeHitOutcome hitOutCome;
};

struct CalcDamageInfo;
struct SpellNonMeleeDamage;

class FC_GAME_API DamageInfo
{
  private:
    Unit* const m_attacker;
    Unit* const m_victim;
    uint32 m_damage;
    SpellInfo const* const m_spellInfo;
    SpellSchoolMask const m_schoolMask;
    DamageEffectType const m_damageType;
    WeaponAttackType m_attackType;
    uint32 m_absorb;
    uint32 m_resist;
    uint32 m_block;
    uint32 m_hitMask;
    uint32 m_cleanDamage;

    // amalgamation constructor (used for proc)
    DamageInfo(DamageInfo const& dmg1, DamageInfo const& dmg2);
  public:
    DamageInfo(Unit* attacker, Unit* victim, uint32 damage, SpellInfo const* spellInfo, SpellSchoolMask schoolMask, DamageEffectType damageType, WeaponAttackType attackType, uint32 cleanDamage = 0);
    explicit DamageInfo(CalcDamageInfo const& dmgInfo);
    explicit DamageInfo(CalcDamageInfo const& dmgInfo, uint8 damageIndex);
    DamageInfo(SpellNonMeleeDamage const& spellNonMeleeDamage, DamageEffectType damageType, WeaponAttackType attackType, uint32 hitMask);
    DamageInfo(SpellNonMeleeDamage const& spellNonMeleeDamage, DamageEffectType damageType);

    void ModifyDamage(int32 amount);
    void AbsorbDamage(uint32 amount);
    void ResistDamage(uint32 amount);
    void BlockDamage(uint32 amount);

    Unit* GetAttacker() const { return m_attacker; }
    Unit* GetVictim() const { return m_victim; }
    SpellInfo const* GetSpellInfo() const { return m_spellInfo; }
    SpellSchoolMask GetSchoolMask() const { return m_schoolMask; }
    DamageEffectType GetDamageType() const { return m_damageType; }
    WeaponAttackType GetAttackType() const { return m_attackType; }
    uint32 GetDamage() const { return m_damage; }
    uint32 GetAbsorb() const { return m_absorb; }
    uint32 GetResist() const { return m_resist; }
    uint32 GetBlock() const { return m_block; }

    uint32 GetHitMask() const;
    [[nodiscard]] uint32 GetUnmitigatedDamage() const;
};

class FC_GAME_API HealInfo
{
  private:
    Unit* const m_healer;
    Unit* const m_target;
    uint32 m_heal;
    uint32 m_effectiveHeal;
    uint32 m_absorb;
    SpellInfo const* const m_spellInfo;
    SpellSchoolMask const m_schoolMask;
    uint32 m_hitMask;

  public:
    HealInfo(Unit* healer, Unit* target, uint32 heal, SpellInfo const* spellInfo, SpellSchoolMask schoolMask);
    explicit HealInfo(Unit* _healer, Unit* _target, uint32 _heal, SpellInfo const* _spellInfo, SpellSchoolMask _schoolMask)
        : m_healer(_healer), m_target(_target), m_heal(_heal), m_spellInfo(_spellInfo), m_schoolMask(_schoolMask), m_absorb(0),
        m_effectiveHeal(0),m_hitMask(0){}

    void AbsorbHeal(uint32 amount);
    void SetHeal(uint32 amount)
    {
        m_heal = amount;
    }
    void SetEffectiveHeal(uint32 amount) { m_effectiveHeal = amount; }

    Unit* GetHealer() const { return m_healer; }
    Unit* GetTarget() const { return m_target; }
    uint32 GetHeal() const { return m_heal; }
    uint32 GetEffectiveHeal() const { return m_effectiveHeal; }
    uint32 GetAbsorb() const { return m_absorb; }
    SpellInfo const* GetSpellInfo() const { return m_spellInfo; };
    SpellSchoolMask GetSchoolMask() const { return m_schoolMask; };
    uint32 GetHitMask() const { return m_hitMask; };
};

class FC_GAME_API ProcEventInfo
{
    private:
        Unit* const _actor;
        Unit* const _actionTarget;
        Unit* const _procTarget;
        uint32 _typeMask;
        uint32 _spellTypeMask;
        uint32 _spellPhaseMask;
        uint32 _hitMask;
        uint32 _cooldown;
        Spell* _spell;
        DamageInfo* _damageInfo;
        HealInfo* _healInfo;
        SpellInfo const* const _triggeredByAuraSpell;
        int8 _procAuraEffectIndex;
        std::optional<float> _chance;
  public:
    explicit ProcEventInfo(Unit* actor, Unit* actionTarget, Unit* procTarget, uint32 typeMask, uint32 spellTypeMask, uint32 spellPhaseMask, uint32 hitMask, Spell const* spell, DamageInfo* damageInfo, HealInfo* healInfo, SpellInfo const* triggeredByAuraSpell = nullptr, int8 procAuraEffectIndex = -1);


    Unit* GetActor() { return _actor; }
    Unit* GetActionTarget() const { return _actionTarget; }
    Unit* GetProcTarget() const { return _procTarget; }

    uint32 GetTypeMask() const { return _typeMask; }
    uint32 GetSpellTypeMask() const { return _spellTypeMask; }
    uint32 GetSpellPhaseMask() const { return _spellPhaseMask; }
    uint32 GetHitMask() const { return _hitMask; }

    SpellInfo const* GetSpellInfo() const;
    SpellSchoolMask GetSchoolMask() const;

    DamageInfo* GetDamageInfo() const { return _damageInfo; }
    HealInfo* GetHealInfo() const { return _healInfo; }

    Spell const* GetProcSpell() const { return _spell; }

    [[nodiscard]] SpellInfo const* GetTriggerAuraSpell() const { return _triggeredByAuraSpell; }
    [[nodiscard]] int8 GetTriggerAuraEffectIndex() const { return _procAuraEffectIndex; }
    [[nodiscard]] uint32 GetProcCooldown() const { return _cooldown; }
    void SetProcCooldown(uint32 cooldown) { _cooldown = cooldown; }
    [[nodiscard]] std::optional<float> GetProcChance() const { return _chance; }
    void SetProcChance(float chance) { _chance = chance; }
    void ResetProcChance() { _chance.reset(); }
};

// Struct for use in Unit::CalculateMeleeDamage
// Need create structure like in SMSG_ATTACKERSTATEUPDATE opcode
struct CalcDamageInfo
{
    Unit* attacker; // Attacker
    Unit* target;   // Target for damage
    
    struct
    {
        uint32 damageSchoolMask;
        uint32 damage;
        uint32 absorb;
        uint32 resist;
    } damages[MAX_ITEM_PROTO_DAMAGES];

    uint32 blocked_amount;
    uint32 HitInfo;
    uint32 TargetState;
    // Helper
    WeaponAttackType attackType; //
    uint32 procAttacker;
    uint32 procVictim;
    uint32 cleanDamage;         // Used only for rage calculation
    MeleeHitOutcome hitOutCome; /// @todo remove this field (need use TargetState)
};

// Spell damage info structure based on structure sending in SMSG_SPELLNONMELEEDAMAGELOG opcode
struct FC_GAME_API SpellNonMeleeDamage
{
    SpellNonMeleeDamage(Unit* _attacker, Unit* _target, SpellInfo const* _spellInfo, uint32 _schoolMask)
        : target(_target), attacker(_attacker), spellInfo(_spellInfo), damage(0), overkill(0), schoolMask(_schoolMask), absorb(0), resist(0), physicalLog(false), unused(false), blocked(0), HitInfo(0),
          cleanDamage(0), fullBlock(false)
    {
    }

    Unit* target;
    Unit* attacker;
    SpellInfo const* spellInfo;
    uint32 damage;
    uint32 overkill;
    uint32 schoolMask;
    uint32 absorb;
    uint32 resist;
    bool physicalLog;
    bool unused;
    uint32 blocked;
    uint32 HitInfo;
    // Used for help
    uint32 cleanDamage;
    bool fullBlock;
};

struct SpellPeriodicAuraLogInfo
{
    SpellPeriodicAuraLogInfo(AuraEffect const* _auraEff, uint32 _damage, uint32 _overDamage, uint32 _absorb, uint32 _resist, float _multiplier, bool _critical)
        : auraEff(_auraEff), damage(_damage), overDamage(_overDamage), absorb(_absorb), resist(_resist), multiplier(_multiplier), critical(_critical)
    {
    }

    AuraEffect const* auraEff;
    uint32 damage;
    uint32 overDamage; // overkill/overheal
    uint32 absorb;
    uint32 resist;
    float multiplier;
    bool critical;
};

void createProcFlags(SpellInfo const* spellInfo, WeaponAttackType attackType, bool positive, uint32& procAttacker, uint32& procVictim);
uint32 createProcHitMask(SpellNonMeleeDamage* damageInfo, SpellMissInfo missCondition);

enum CurrentSpellTypes : uint8
{
    CURRENT_MELEE_SPELL = 0,
    CURRENT_GENERIC_SPELL = 1,
    CURRENT_CHANNELED_SPELL = 2,
    CURRENT_AUTOREPEAT_SPELL = 3
};

#define CURRENT_FIRST_NON_MELEE_SPELL 1
#define CURRENT_MAX_SPELL 4

typedef std::list<Player*> SharedVisionList;


struct AttackPosition {
    AttackPosition(Position pos) : _pos(std::move(pos)), _taken(false) {}
    bool operator==(const int val)
    {
        return !val;
    };
    int operator=(const int val)
    {
        if (!val)
        {
            // _pos = nullptr;
            _taken = false;
            return 0; // nullptr
        }
        return 0; // nullptr
    };
    Position _pos;
    bool _taken;
};

// for clearing special attacks
#define REACTIVE_TIMER_START 4000  // @todo should this be 5000

enum ReactiveType
{
    REACTIVE_DEFENSE = 0,
    REACTIVE_HUNTER_PARRY = 1,
    REACTIVE_OVERPOWER      = 2,
    REACTIVE_WOLVERINE_BITE = 3,

    MAX_REACTIVE
};

enum PlayerTotemType
{
    SUMMON_TYPE_TOTEM_FIRE = 63,
    SUMMON_TYPE_TOTEM_EARTH = 81,
    SUMMON_TYPE_TOTEM_WATER = 82,
    SUMMON_TYPE_TOTEM_AIR = 83
};

/// Spell cooldown flags sent in SMSG_SPELL_COOLDOWN
enum SpellCooldownFlags
{
    SPELL_COOLDOWN_FLAG_NONE                    = 0x0,
    SPELL_COOLDOWN_FLAG_INCLUDE_GCD             = 0x1,  ///< Starts GCD in addition to normal cooldown specified in the packet
    SPELL_COOLDOWN_FLAG_INCLUDE_EVENT_COOLDOWNS = 0x2   ///< Starts GCD for spells that should start their cooldown on events, requires SPELL_COOLDOWN_FLAG_INCLUDE_GCD set
};

typedef std::unordered_map<uint32, uint32> PacketCooldowns;

// delay time next attack to prevent client attack animation problems
#define ATTACK_DISPLAY_DELAY 200
#define MAX_PLAYER_STEALTH_DETECT_RANGE 30.0f // max distance for detection targets by player

struct SpellProcEventEntry;                                 // used only privately

struct PositionUpdateInfo
{
    void Reset()
    {
        Relocated = false;
        Turned = false;
    }

    bool Relocated = false;
    bool Turned = false;
};

struct SpellCastRequestItemData
{
    SpellCastRequestItemData(uint8 bagSlot, uint8 slot, ObjectGuid castItem) : BagSlot(bagSlot), Slot(slot), CastItem(castItem) {}

    uint8 BagSlot = 0;
    uint8 Slot = 0;
    ObjectGuid CastItem;
};

struct PendingSpellCastRequest
{
    PendingSpellCastRequest(WorldPackets::Spells::SpellCastRequest&& castRequest, Optional<SpellCastRequestItemData> castItemData = {}) : CastRequest(castRequest), CastItemData(castItemData) {}

    WorldPackets::Spells::SpellCastRequest CastRequest;
    Optional<SpellCastRequestItemData> CastItemData;
};

class FC_GAME_API Unit : public WorldObject
{
    friend class WorldSession;

  public:
    typedef std::set<Unit*> AttackerSet;
    typedef std::set<Unit*> ControlSet;
    typedef std::vector<Unit*> UnitVector;

    typedef std::multimap<uint32, Aura*> AuraMap;
    typedef std::pair<AuraMap::const_iterator, AuraMap::const_iterator> AuraMapBounds;
    typedef std::pair<AuraMap::iterator, AuraMap::iterator> AuraMapBoundsNonConst;

    typedef std::multimap<uint32, AuraApplication*> AuraApplicationMap;
    typedef std::pair<AuraApplicationMap::const_iterator, AuraApplicationMap::const_iterator> AuraApplicationMapBounds;
    typedef std::pair<AuraApplicationMap::iterator, AuraApplicationMap::iterator> AuraApplicationMapBoundsNonConst;

    typedef std::multimap<AuraStateType, AuraApplication*> AuraStateAurasMap;
    typedef std::pair<AuraStateAurasMap::const_iterator, AuraStateAurasMap::const_iterator> AuraStateAurasMapBounds;

    typedef std::list<AuraEffect*> AuraEffectList;
    typedef std::list<Aura*> AuraList;
    typedef std::list<AuraApplication*> AuraApplicationList;
    typedef std::list<DiminishingReturn> Diminishing;
    typedef GuidUnorderedSet ComboPointHolderSet;

    typedef std::vector<std::pair<uint8 /*procEffectMask*/, AuraApplication*>> AuraApplicationProcContainer;
    typedef std::vector<ObjectGuid> FormationFollowerGUIDContainer;

    typedef std::map<uint8, AuraApplication*> VisibleAuraMap;

    typedef std::unordered_map<uint32 /*spellId*/, AuraList> AurasBySpellIdMap;

    virtual ~Unit();

    void Update(uint32 time) override;

    UnitAI* GetAI() { return i_AI; }
    void SetAI(UnitAI* newAI) { i_AI = newAI; }

    void AddToWorld() override;
    void RemoveFromWorld() override;

    void CleanupBeforeRemoveFromMap(bool finalCleanup);
    void CleanupsBeforeDelete(bool finalCleanup = true) override;                        // used in ~Creature/~Player (or before mass creature delete to remove cross-references to already deleted units)

    [[nodiscard]] virtual bool isBeingLoaded() const { return false;}
    [[nodiscard]] bool IsDuringRemoveFromWorld() const {return m_duringRemoveFromWorld;}


    /*********************************************************/
    /***                    UNIT HELPERS                   ***/
    /*********************************************************/

    [[nodiscard]] Unit* GetOwner() const;

    ObjectGuid GetOwnerGUID() const { return GetGuidValue(UNIT_FIELD_SUMMONEDBY); }
    void SetOwnerGUID(ObjectGuid owner);
    ObjectGuid GetCreatorGUID() const { return GetGuidValue(UNIT_FIELD_CREATEDBY); }
    void SetCreatorGUID(ObjectGuid creator) { SetGuidValue(UNIT_FIELD_CREATEDBY, creator); }
    ObjectGuid GetMinionGUID() const { return GetGuidValue(UNIT_FIELD_SUMMON); }
    void SetMinionGUID(ObjectGuid guid) { SetGuidValue(UNIT_FIELD_SUMMON, guid); }
    [[nodiscard]] ObjectGuid GetCharmerGUID() const { return GetGuidValue(UNIT_FIELD_CHARMEDBY); }
    void SetCharmerGUID(ObjectGuid owner) { SetGuidValue(UNIT_FIELD_CHARMEDBY, owner); }
    ObjectGuid GetPetGUID() const { return m_SummonSlot[SUMMON_SLOT_PET]; }
    void SetPetGUID(ObjectGuid guid) { m_SummonSlot[SUMMON_SLOT_PET] = guid; }
    ObjectGuid GetCritterGUID() const { return GetGuidValue(UNIT_FIELD_CRITTER); }
    void SetCritterGUID(ObjectGuid guid) { SetGuidValue(UNIT_FIELD_CRITTER, guid); }
    ObjectGuid GetTransGUID() const override;

    Pet* ToPet()
    {
        if (IsPet())
            return reinterpret_cast<Pet*>(this);
        else
            return nullptr;
    }
    Pet const* ToPet() const
    {
        if (IsPet())
            return reinterpret_cast<Pet const*>(this);
        else
            return nullptr;
    }

    Totem* ToTotem()
    {
        if (IsTotem())
            return reinterpret_cast<Totem*>(this);
        else
            return nullptr;
    }
    Totem const* ToTotem() const
    {
        if (IsTotem())
            return reinterpret_cast<Totem const*>(this);
        else
            return nullptr;
    }

    TempSummon* ToTempSummon()
    {
        if (IsSummon())
            return reinterpret_cast<TempSummon*>(this);
        else
            return nullptr;
    }
    TempSummon const* ToTempSummon() const
    {
        if (IsSummon())
            return reinterpret_cast<TempSummon const*>(this);
        else
            return nullptr;
    }

    void AddUnitState(uint32 f) { m_state |= f; }
    bool HasUnitState(const uint32 f) const { return (m_state & f) != 0; }
    void ClearUnitState(uint32 f) { m_state &= ~f; }
    bool CanFreeMove() const;

    // Unit mask
    [[nodiscard]] uint32 HasUnitTypeMask(uint32 mask) const { return mask & m_unitTypeMask; }
    void AddUnitTypeMask(uint32 mask) { m_unitTypeMask |= mask; }
    [[nodiscard]] uint32 GetUnitTypeMask() const { return m_unitTypeMask; }

    // Unit flags
    UnitFlags GetUnitFlags() const { return UnitFlags(GetUInt32Value(UNIT_FIELD_FLAGS)); }
    bool HasUnitFlag(UnitFlags flags) const { return HasFlag(UNIT_FIELD_FLAGS, flags); }   /// @brief UnitFlags available in UnitDefines.h
    void SetUnitFlag(UnitFlags flags) { SetFlag(UNIT_FIELD_FLAGS, flags); }   /// @brief UnitFlags available in UnitDefines.h
    void RemoveUnitFlag(UnitFlags flags) { RemoveFlag(UNIT_FIELD_FLAGS, flags); }   /// @brief Remove the Unit flag specify only
    void ReplaceAllUnitFlags(UnitFlags flags) { SetUInt32Value(UNIT_FIELD_FLAGS, flags); } /// @brief Remove all UnitFlags and set new ones. UnitFlags available in UnitDefines.h

    UnitFlags2 GetUnitFlags2() const { return UnitFlags2(GetUInt32Value(UNIT_FIELD_FLAGS_2)); }
    bool HasUnitFlag2(UnitFlags2 flags) const { return HasFlag(UNIT_FIELD_FLAGS_2, flags); }
    void SetUnitFlag2(UnitFlags2 flags) { SetFlag(UNIT_FIELD_FLAGS_2, flags); }
    void RemoveUnitFlag2(UnitFlags2 flags) { RemoveFlag(UNIT_FIELD_FLAGS_2, flags); }
    void ReplaceAllUnitFlags2(UnitFlags2 flags) { SetUInt32Value(UNIT_FIELD_FLAGS_2, flags); }

    void SetEmoteState(Emote emoteState) { SetUInt32Value(UNIT_NPC_EMOTESTATE, emoteState); }  /// @brief Sets emote state (looping emote). Emotes available in SharedDefines.h
    void ClearEmoteState() { SetEmoteState(EMOTE_ONESHOT_NONE); }  /// @brief Clears emote state (looping emote)

    // NPC flags
    NPCFlags GetNpcFlags() const { return NPCFlags(GetUInt32Value(UNIT_NPC_FLAGS)); }
    bool HasNpcFlag(NPCFlags flags) const { return HasFlag(UNIT_NPC_FLAGS, flags) != 0; }
    void SetNpcFlag(NPCFlags flags) { SetFlag(UNIT_NPC_FLAGS, flags); }
    void RemoveNpcFlag(NPCFlags flags) { RemoveFlag(UNIT_NPC_FLAGS, flags); }
    void ReplaceAllNpcFlags(NPCFlags flags) { SetUInt32Value(UNIT_NPC_FLAGS, flags); }

    uint32 GetDynamicFlags() const override { return GetUInt32Value(UNIT_DYNAMIC_FLAGS); }
    void ReplaceAllDynamicFlags(uint32 flag) override { SetUInt32Value(UNIT_DYNAMIC_FLAGS, flag); }

    // Movement flags
    void AddUnitMovementFlag(uint32 f) { m_movementInfo.flags |= f; }
    void RemoveUnitMovementFlag(uint32 f) { m_movementInfo.flags &= ~f; }
    [[nodiscard]] bool HasUnitMovementFlag(uint32 f) const { return (m_movementInfo.flags & f) == f; }
    [[nodiscard]] uint32 GetUnitMovementFlags() const { return m_movementInfo.flags; }
    void SetUnitMovementFlags(uint32 f) { m_movementInfo.flags = f; }

    void AddExtraUnitMovementFlag(uint16 f) { m_movementInfo.flags2 |= f; }
    void RemoveExtraUnitMovementFlag(uint16 f) { m_movementInfo.flags2 &= ~f; }
    [[nodiscard]] uint16 HasExtraUnitMovementFlag(uint16 f) const { return m_movementInfo.flags2 & f; }
    [[nodiscard]] uint16 GetExtraUnitMovementFlags() const { return m_movementInfo.flags2; }
    void SetExtraUnitMovementFlags(uint16 f) { m_movementInfo.flags2 = f; }
    bool IsSplineEnabled() const;

    /*********************************************************/
    /***           UNIT TYPES, CLASSES, RACES...           ***/
    /*********************************************************/

    // Unit type methods
    [[nodiscard]] bool IsSummon() const { return m_unitTypeMask & UNIT_MASK_SUMMON; }
    [[nodiscard]] bool IsMinion() const { return (m_unitTypeMask & UNIT_MASK_MINION) != 0; }
    [[nodiscard]] bool IsGuardian() const { return m_unitTypeMask & UNIT_MASK_GUARDIAN; }
    [[nodiscard]] bool IsControllableGuardian() const { return m_unitTypeMask & UNIT_MASK_CONTROLABLE_GUARDIAN; }
    [[nodiscard]] bool IsPet() const { return m_unitTypeMask & UNIT_MASK_PET; }
    [[nodiscard]] bool IsHunterPet() const { return m_unitTypeMask & UNIT_MASK_HUNTER_PET; }
    [[nodiscard]] bool IsTotem() const { return m_unitTypeMask & UNIT_MASK_TOTEM; }
    [[nodiscard]] bool IsVehicle() const { return m_unitTypeMask & UNIT_MASK_VEHICLE; }
    [[nodiscard]] bool IsCritter() const { return GetCreatureType() == CREATURE_TYPE_CRITTER; }

    // NPC type methods
    [[nodiscard]] bool IsVendor()       const { return HasNpcFlag(UNIT_NPC_FLAG_VENDOR); }
    [[nodiscard]] bool IsTrainer()      const { return HasNpcFlag(UNIT_NPC_FLAG_TRAINER); }
    [[nodiscard]] bool IsQuestGiver()   const { return HasNpcFlag(UNIT_NPC_FLAG_QUESTGIVER); }
    [[nodiscard]] bool IsGossip()       const { return HasNpcFlag(UNIT_NPC_FLAG_GOSSIP); }
    [[nodiscard]] bool IsTaxi()         const { return HasNpcFlag(UNIT_NPC_FLAG_FLIGHTMASTER); }
    [[nodiscard]] bool IsGuildMaster()  const { return HasNpcFlag(UNIT_NPC_FLAG_PETITIONER); }
    [[nodiscard]] bool IsBattleMaster() const { return HasNpcFlag(UNIT_NPC_FLAG_BATTLEMASTER); }
    [[nodiscard]] bool IsBanker()       const { return HasNpcFlag(UNIT_NPC_FLAG_BANKER); }
    [[nodiscard]] bool IsInnkeeper()    const { return HasNpcFlag(UNIT_NPC_FLAG_INNKEEPER); }
    [[nodiscard]] bool IsSpiritHealer() const { return HasNpcFlag(UNIT_NPC_FLAG_SPIRITHEALER); }
    [[nodiscard]] bool IsSpiritGuide()  const { return HasNpcFlag(UNIT_NPC_FLAG_SPIRITGUIDE); }
    [[nodiscard]] bool IsTabardDesigner() const { return HasNpcFlag(UNIT_NPC_FLAG_TABARDDESIGNER); }
    [[nodiscard]] bool IsAuctioner()    const { return HasNpcFlag(UNIT_NPC_FLAG_AUCTIONEER); }
    [[nodiscard]] bool IsArmorer()      const { return HasNpcFlag(UNIT_NPC_FLAG_REPAIR); }
    [[nodiscard]] bool IsServiceProvider() const
    {
        return HasNpcFlag(UNIT_NPC_FLAG_VENDOR | UNIT_NPC_FLAG_TRAINER | UNIT_NPC_FLAG_FLIGHTMASTER |
                       UNIT_NPC_FLAG_PETITIONER | UNIT_NPC_FLAG_BATTLEMASTER | UNIT_NPC_FLAG_BANKER |
                       UNIT_NPC_FLAG_INNKEEPER | UNIT_NPC_FLAG_SPIRITHEALER |
                       UNIT_NPC_FLAG_SPIRITGUIDE | UNIT_NPC_FLAG_TABARDDESIGNER | UNIT_NPC_FLAG_AUCTIONEER);
    }
    [[nodiscard]] bool IsSpiritService() const { return HasNpcFlag(UNIT_NPC_FLAG_SPIRITHEALER | UNIT_NPC_FLAG_SPIRITGUIDE); }
    

    // Creature type
    [[nodiscard]] uint32 GetCreatureType() const;
    [[nodiscard]] uint32 GetCreatureTypeMask() const
    {
        uint32 creatureType = GetCreatureType();
        return (creatureType >= 1) ? (1 << (creatureType - 1)) : 0;
    }

    // Race methods
    [[nodiscard]] uint8 getRace(bool original = false) const;
    void setRace(uint8 race);
    [[nodiscard]] uint32 getRaceMask() const { return 1 << (getRace(true) - 1); }
    [[nodiscard]] DisplayRace GetDisplayRaceFromModelId(uint32 modelId) const;
    [[nodiscard]] DisplayRace GetDisplayRace() const { return GetDisplayRaceFromModelId(GetDisplayId()); };

    // Class methods
    [[nodiscard]] uint8 getClass() const { return GetByteValue(UNIT_FIELD_BYTES_0, 1); }
    [[nodiscard]] virtual bool IsClass(Classes unitClass, [[maybe_unused]] ClassContext context = CLASS_CONTEXT_NONE) const { return (getClass() == unitClass); }
    [[nodiscard]] uint32 getClassMask() const { return 1 << (getClass() - 1); }

    // Gender methods
    [[nodiscard]] uint8 getGender() const { return GetByteValue(UNIT_FIELD_BYTES_0, 2); }

    // Factions methods
    [[nodiscard]] uint32 GetFaction() const { return GetUInt32Value(UNIT_FIELD_FACTIONTEMPLATE); }
    [[nodiscard]] FactionTemplateEntry const* GetFactionTemplateEntry() const;
    void SetFaction(uint32 faction);
    void RestoreFaction();
    [[nodiscard]] uint32 GetOldFactionId() const { return _oldFactionId; }


    /*********************************************************/
    /***            METHODS RELATED TO COMBATS             ***/
    /*********************************************************/
    // Targets
    [[nodiscard]] ObjectGuid GetTarget() const { return GetGuidValue(UNIT_FIELD_TARGET); }
    virtual void SetTarget(ObjectGuid /*guid*/ = ObjectGuid::Empty) = 0;

    bool isTargetableForAttack(bool checkFakeDeath = true, Unit const* byWho = nullptr) const;
    bool IsValidAttackTarget(Unit const* target, SpellInfo const* bySpell = nullptr) const;
    bool _IsValidAttackTarget(Unit const* target, SpellInfo const* bySpell, WorldObject const* obj = nullptr) const;
    bool IsValidAssistTarget(Unit const* target) const;
    bool _IsValidAssistTarget(Unit const* target, SpellInfo const* bySpell) const;

    // Combat range
    [[nodiscard]] float GetCombatReach() const override { return m_floatValues[UNIT_FIELD_COMBATREACH]; }
    [[nodiscard]] float GetMeleeReach() const { float reach = m_floatValues[UNIT_FIELD_COMBATREACH]; return reach > MIN_MELEE_REACH ? reach : MIN_MELEE_REACH; }
    [[nodiscard]] bool IsWithinRange(Unit const* obj, float dist) const;
    bool IsWithinCombatRange(Unit const* obj, float dist2compare) const;
    bool IsWithinMeleeRange(Unit const* obj, float dist = 0.f) const;
    float GetMeleeRange(Unit const* target) const;

    void setAttackTimer(WeaponAttackType type, int32 time) { m_attackTimer[type] = time; }  /// @todo - Look to convert to std::chrono
    void resetAttackTimer(WeaponAttackType type = BASE_ATTACK);
    [[nodiscard]] int32 getAttackTimer(WeaponAttackType type) const { return m_attackTimer[type]; }
    [[nodiscard]] bool isAttackReady(WeaponAttackType type = BASE_ATTACK) const { return m_attackTimer[type] <= 0; }

    virtual SpellSchoolMask GetMeleeDamageSchoolMask(WeaponAttackType attackType = BASE_ATTACK, uint8 damageIndex = 0) const = 0;
    bool GetRandomContactPoint(Unit const* target, float& x, float& y, float& z, bool force = false) const;

    [[nodiscard]] Unit* getAttackerForHelper() const;                 // If someone wants to help, who to give them

    bool Attack(Unit* victim, bool meleeAttack);

    void CastStop(uint32 except_spellid = 0, bool withInstant = true);
    bool AttackStop();
    void RemoveAllAttackers();
    [[nodiscard]] AttackerSet const& getAttackers() const { return m_attackers; }
    [[nodiscard]] bool GetMeleeAttackPoint(Unit* attacker, Position& pos);
    [[nodiscard]] bool isAttackingPlayer() const;
    [[nodiscard]] Unit* GetVictim() const { return m_attacking; }

    void CombatStop(bool includingCast = false);
    void CombatStopWithPets(bool includingCast = false);
    void StopAttackFaction(uint32 faction_id);
    void StopAttackingInvalidTarget();
    Unit* SelectNearbyTarget(Unit* exclude = nullptr, float dist = NOMINAL_MELEE_RANGE) const;
    Unit* SelectNearbyNoTotemTarget(Unit* exclude = nullptr, float dist = NOMINAL_MELEE_RANGE) const;
    void SendMeleeAttackStop(Unit* victim = nullptr);
    void SendMeleeAttackStart(Unit* victim, Player* sendTo = nullptr);
    void UpdatePetCombatState();

    [[nodiscard]] uint32 GetAttackTime(WeaponAttackType att) const
    {
        float f_BaseAttackTime = GetFloatValue(static_cast<uint16>(UNIT_FIELD_BASEATTACKTIME) + att) / m_modAttackSpeedPct[att];
        return (uint32)f_BaseAttackTime;
    }

    void SetAttackTime(WeaponAttackType att, uint32 val) { SetFloatValue(static_cast<uint16>(UNIT_FIELD_BASEATTACKTIME) + att, val * m_modAttackSpeedPct[att]); }
    void ApplyAttackTimePercentMod(WeaponAttackType att, float val, bool apply);
    void ApplyHasteRegenMod(WeaponAttackType att, float val, bool apply);
    void ApplyCastTimePercentMod(float val, bool apply);

    void SetImmuneToAll(bool apply, bool keepCombat = false) { SetImmuneToPC(apply, keepCombat); SetImmuneToNPC(apply, keepCombat); }
    bool IsImmuneToAll() const { return IsImmuneToPC() && IsImmuneToNPC(); }
    void SetImmuneToPC(bool apply, bool keepCombat = false);
    bool IsImmuneToPC() const { return HasUnitFlag(UNIT_FLAG_IMMUNE_TO_PC); }
    void SetImmuneToNPC(bool apply, bool keepCombat = false);
    bool IsImmuneToNPC() const { return HasUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC); }

    bool IsEngaged() const { return IsInCombat(); }
    bool IsEngagedBy(Unit const* who) const { return IsInCombatWith(who); }

    [[nodiscard]] bool IsInCombat() const { return HasUnitFlag(UNIT_FLAG_IN_COMBAT); }
    bool IsInCombatWith(Unit const* who) const;

    [[nodiscard]] bool IsPetInCombat() const { return HasUnitFlag(UNIT_FLAG_PET_IN_COMBAT); }
    void CombatStart(Unit* target, bool initialAggro = true);
    void CombatStartOnCast(Unit* target, bool initialAggro = true, uint32 duration = 0);
    void SetInCombatState(bool PvP, Unit* enemy = nullptr, uint32 duration = 0);
    void SetInCombatWith(Unit* enemy, uint32 duration = 0);
    void ClearInCombat();
    void ClearInPetCombat();
    [[nodiscard]] uint32 GetCombatTimer() const { return m_CombatTimer; }
    void SetCombatTimer(uint32 timer) { m_CombatTimer = timer; }

    // Threat related methods
    [[nodiscard]] bool CanHaveThreatList(bool skipAliveCheck = false) const;
    void AddThreat(Unit* victim, float fThreat, SpellSchoolMask schoolMask = SPELL_SCHOOL_MASK_NORMAL, SpellInfo const* threatSpell = nullptr);
    float ApplyTotalThreatModifier(float fThreat, SpellSchoolMask schoolMask = SPELL_SCHOOL_MASK_NORMAL);
    void TauntApply(Unit* victim);
    void TauntFadeOut(Unit* taunter);
    ThreatMgr& GetThreatMgr() { return m_ThreatMgr; }
    ThreatMgr const& GetThreatMgr() const { return m_ThreatMgr; }
    void addHatedBy(HostileReference* pHostileReference) { m_HostileRefMgr.insertFirst(pHostileReference); };
    void removeHatedBy(HostileReference* /*pHostileReference*/) { /* nothing to do yet */ }
    HostileRefMgr& getHostileRefMgr() { return m_HostileRefMgr; }

    // Redirect Threat
    void SetRedirectThreat(ObjectGuid guid, uint32 pct) { _redirectThreatInfo.Set(guid, pct); }
    void ResetRedirectThreat() { SetRedirectThreat(ObjectGuid::Empty, 0); }
    void ModifyRedirectThreat(int32 amount) { _redirectThreatInfo.ModifyThreatPct(amount); }
    uint32 GetRedirectThreatPercent() { return _redirectThreatInfo.GetThreatPct(); }
    [[nodiscard]] Unit* GetRedirectThreatTarget() const;

    void SetLastDamagedTargetGuid(ObjectGuid const& guid) { _lastDamagedTargetGuid = guid; }
    [[nodiscard]] ObjectGuid const& GetLastDamagedTargetGuid() const { return _lastDamagedTargetGuid; }

    void AttackerStateUpdate (Unit* victim, WeaponAttackType attType = BASE_ATTACK, bool extra = false, bool ignoreCasting = false);

    // Weapons systems
    [[nodiscard]] bool haveOffhandWeapon() const;
    [[nodiscard]] bool CanDualWield() const { return m_canDualWield; }
    virtual void SetCanDualWield(bool value) { m_canDualWield = value; }

    virtual bool HasWeapon(WeaponAttackType type) const = 0;
    inline bool HasMainhandWeapon() const { return HasWeapon(BASE_ATTACK); }
    inline bool HasOffhandWeapon() const { return HasWeapon(OFF_ATTACK); }
    inline bool HasRangedWeapon() const { return HasWeapon(RANGED_ATTACK); }

    inline bool hasMainhandWeaponForAttack() const { return HasWeaponForAttack(BASE_ATTACK); }
    virtual bool HasWeaponForAttack(WeaponAttackType type) const { return CanUseAttackType(type); }
    inline bool HasMainhandWeaponForAttack() const { return HasWeaponForAttack(BASE_ATTACK); }
    inline bool HasOffhandWeaponForAttack() const { return HasWeaponForAttack(OFF_ATTACK); }
    inline bool HasRangedWeaponForAttack() const { return HasWeaponForAttack(RANGED_ATTACK); }
    [[nodiscard]] bool CanUseAttackType(uint8 attacktype) const
    {
        switch (attacktype)
        {
            case BASE_ATTACK:
                return !HasUnitFlag(UNIT_FLAG_DISARMED);
            case OFF_ATTACK:
                return !HasUnitFlag2(UNIT_FLAG2_DISARM_OFFHAND);
            case RANGED_ATTACK:
                return !HasUnitFlag2(UNIT_FLAG2_DISARM_RANGED);
            default:
                return true;
        }
    }

    // Extra attacks
    void HandleProcExtraAttackFor(Unit* victim, uint32 count);
    void SetLastExtraAttackSpell(uint32 spellId) { _lastExtraAttackSpell = spellId; }
    [[nodiscard]] uint32 GetLastExtraAttackSpell() const { return _lastExtraAttackSpell; }
    void AddExtraAttacks(uint32 count);

    // Combot points system
    [[nodiscard]] uint8 GetComboPoints(Unit const* who = nullptr) const { return (who && m_comboTarget != who) ? 0 : m_comboPoints; }
    [[nodiscard]] uint8 GetComboPoints(ObjectGuid const& guid) const { return (m_comboTarget && m_comboTarget->GetGUID() == guid) ? m_comboPoints : 0; }
    [[nodiscard]] Unit* GetComboTarget() const { return m_comboTarget; }
    [[nodiscard]] ObjectGuid const GetComboTargetGUID() const { return m_comboTarget ? m_comboTarget->GetGUID() : ObjectGuid::Empty; }

    void AddComboPoints(Unit* target, int8 count);
    void AddComboPoints(int8 count) { AddComboPoints(nullptr, count); }
    void ClearComboPoints();

    void AddComboPointHolder(Unit* unit) { m_ComboPointHolders.insert(unit); }
    void RemoveComboPointHolder(Unit* unit) { m_ComboPointHolders.erase(unit); }
    void ClearComboPointHolders();

    void ValidateAttackersAndOwnTarget();

    // PvP
    void SetContestedPvP(Player* attackedPlayer = nullptr, bool lookForNearContestedGuards = true);
    [[nodiscard]] bool IsContestedGuard() const
    {
        if (FactionTemplateEntry const* entry = GetFactionTemplateEntry())
            return entry->IsContestedGuardFaction();

        return false;
    }
    [[nodiscard]] bool RespondsToCallForHelp() const
    {
        if (FactionTemplateEntry const* entry = GetFactionTemplateEntry())
            return entry->FactionRespondsToCallForHelp();

        return false;
    }
    [[nodiscard]] bool IsInSanctuary() const { return HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY); }
    [[nodiscard]] bool IsPvP() const { return HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_PVP); }
    [[nodiscard]] bool IsFFAPvP() const { return HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP); }
    void SetPvP(bool state)
    {
        if (state)
            SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_PVP);
        else
            RemoveByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_PVP);
    }


    /*********************************************************/
    /***              METHODS RELATED TO STATS             ***/
    /*********************************************************/
    // stat system
    [[nodiscard]] float GetStat(Stats stat) const { return float(GetUInt32Value(static_cast<uint16>(UNIT_FIELD_STAT0) + stat)); }
    void SetStat(Stats stat, int32 val) { SetStatInt32Value(static_cast<uint16>(UNIT_FIELD_STAT0) + stat, val); }

    [[nodiscard]] Stats GetStatByAuraGroup(UnitMods unitMod) const;

    [[nodiscard]] float GetCreateStat(Stats stat) const { return m_createStats[stat]; }
    void SetCreateStat(Stats stat, float val) { m_createStats[stat] = val; }

    [[nodiscard]] float GetPosStat(Stats stat) const { return GetFloatValue(static_cast<uint16>(UNIT_FIELD_POSSTAT0) +  stat); }
    [[nodiscard]] float GetNegStat(Stats stat) const { return GetFloatValue(static_cast<uint16>(UNIT_FIELD_NEGSTAT0) +  stat); }

    void InitStatBuffMods()
    {
        for (uint8 i = STAT_STRENGTH; i < MAX_STATS; ++i) SetFloatValue(static_cast<uint16>(UNIT_FIELD_POSSTAT0) +  i, 0);
        for (uint8 i = STAT_STRENGTH; i < MAX_STATS; ++i) SetFloatValue(static_cast<uint16>(UNIT_FIELD_NEGSTAT0) +  i, 0);
    }

    bool HandleStatModifier(UnitMods unitMod, UnitModifierType modifierType, float amount, bool apply);
    void SetModifierValue(UnitMods unitMod, UnitModifierType modifierType, float value) { m_auraModifiersGroup[unitMod][modifierType] = value; }
    [[nodiscard]] float GetModifierValue(UnitMods unitMod, UnitModifierType modifierType) const;
    [[nodiscard]] float GetTotalStatValue(Stats stat, float additionalValue = 0.0f) const;
    static float CalculateAverageResistReduction(Unit const* attacker, SpellSchoolMask schoolMask, Unit const* victim, SpellInfo const* spellInfo = nullptr);

    void SetCanModifyStats(bool modifyStats) { m_canModifyStats = modifyStats; }
    [[nodiscard]] bool CanModifyStats() const { return m_canModifyStats; }

    void ApplyStatBuffMod(Stats stat, float val, bool apply) { ApplyModSignedFloatValue((val > 0 ? static_cast<uint16>(UNIT_FIELD_POSSTAT0) +  stat : static_cast<uint16>(UNIT_FIELD_NEGSTAT0) +  stat), val, apply); }
    void ApplyStatPercentBuffMod(Stats stat, float val, bool apply);

    // Unit level methods
    [[nodiscard]] uint8 GetLevel() const { return uint8(GetUInt32Value(UNIT_FIELD_LEVEL)); }
    uint8 getLevelForTarget(WorldObject const* /*target*/) const override { return GetLevel(); }
    void SetLevel(uint8 lvl, bool showLevelChange = true);

    // Health methods
    [[nodiscard]] uint32 GetHealth()    const { return GetUInt32Value(UNIT_FIELD_HEALTH); }
    [[nodiscard]] uint32 GetMaxHealth() const { return GetUInt32Value(UNIT_FIELD_MAXHEALTH); }
    [[nodiscard]] float GetHealthPct() const { return GetMaxHealth() ? 100.f * GetHealth() / GetMaxHealth() : 0.0f; }
    int32 GetHealthGain(int32 dVal);
    [[nodiscard]] uint32 GetCreateHealth() const { return GetUInt32Value(UNIT_FIELD_BASE_HEALTH); }
    void SetHealth(uint32 val);
    void SetMaxHealth(uint32 val);
    inline void SetFullHealth() { SetHealth(GetMaxHealth()); }
    int32 ModifyHealth(int32 val);
    void SetCreateHealth(uint32 val) { SetUInt32Value(UNIT_FIELD_BASE_HEALTH, val); }
    [[nodiscard]] bool IsFullHealth() const { return GetHealth() == GetMaxHealth(); }
    virtual void RegenerateHealth() {}

    [[nodiscard]] bool HealthBelowPct(int32 pct) const { return GetHealth() < CountPctFromMaxHealth(pct); }
    [[nodiscard]] bool HealthBelowPctDamaged(int32 pct, uint32 damage) const { return int64(GetHealth()) - int64(damage) < int64(CountPctFromMaxHealth(pct)); }
    [[nodiscard]] bool HealthAbovePct(int32 pct) const { return GetHealth() > CountPctFromMaxHealth(pct); }
    [[nodiscard]] bool HealthAbovePctHealed(int32 pct, uint32 heal) const { return uint64(GetHealth()) + uint64(heal) > CountPctFromMaxHealth(pct); }

    [[nodiscard]] uint32 CountPctFromMaxHealth(int32 pct) const { return CalculatePct(GetMaxHealth(), pct); }
    [[nodiscard]] uint32 CountPctFromCurHealth(int32 pct) const { return CalculatePct(GetHealth(), pct); }


    // Power methods
    [[nodiscard]] Powers getPowerType() const { return Powers(GetByteValue(UNIT_FIELD_BYTES_0, 3)); }
    [[nodiscard]] virtual bool HasActivePowerType(Powers power) { return getPowerType() == power; }
    [[nodiscard]] Powers GetPowerTypeByAuraGroup(UnitMods unitMod) const;

    [[nodiscard]] uint32 GetPower(Powers power) const { return GetUInt32Value(static_cast<uint16>(UNIT_FIELD_POWER1) + power); }
    [[nodiscard]] int32 GetMinPower(Powers power) const { return power == POWER_ECLIPSE ? -100 : 0; }
    [[nodiscard]] uint32 GetMaxPower(Powers power) const { return GetUInt32Value(static_cast<uint16>(UNIT_FIELD_MAXPOWER1) + power); }
    [[nodiscard]] float GetPowerPct(Powers power) const { return GetMaxPower(power) ? 100.f * GetPower(power) / GetMaxPower(power) : 0.0f; }
    [[nodiscard]] uint32 GetCreatePowers(Powers power) const;
    [[nodiscard]] virtual uint32 GetPowerIndex(Powers power) const = 0;

    void UpdateDisplayPower();
    void setPowerType(Powers power);
    void SetPower(Powers power, uint32 val, bool withPowerUpdate = true, bool fromRegenerate = false);
    void SetMaxPower(Powers power, uint32 val);
    virtual void UpdatePowerRegeneration(Powers /*powerType*/) {}
    void SetMaxPower(Powers power, int32 val);
    void SetPowerBarID(uint32 id) { _powerBarId = id; }
    void Regenerate(Powers powerType, uint32 diff);

    inline void SetFullPower(Powers power) { SetPower(power, GetMaxPower(power)); }

    int32 ModifyPower(Powers power, int32 val, bool withPowerUpdate = true);
    int32 ModifyPowerPct(Powers power, float pct, bool apply = true);
    static float GetBasePowerRegen(uint32 powerBarId, Powers powerType, bool isInCombat);
    float GetPowerRegen(Powers powerType, bool isInCombat) const;

    void RewardRage(uint32 damage, uint32 weaponSpeedHitFactor, bool attacker);

    [[nodiscard]] uint32 GetCreateMana() const { return GetUInt32Value(UNIT_FIELD_BASE_MANA); }
    void SetCreateMana(uint32 val) { SetUInt32Value(UNIT_FIELD_BASE_MANA, val); }
    [[nodiscard]] bool CanRestoreMana(SpellInfo const* spellInfo) const;
    void SetLastManaUse(uint32 spellCastTime) { m_lastManaUse = spellCastTime; }
    [[nodiscard]] bool IsUnderLastManaUseEffect() const;

    float GetAPMultiplier(WeaponAttackType attType, bool normalized);

    // Secondary attributes
    [[nodiscard]] uint32 GetArmor() const { return GetResistance(SPELL_SCHOOL_NORMAL); }
    void SetArmor(int32 val) { SetResistance(SPELL_SCHOOL_NORMAL, val); }

    [[nodiscard]] float GetUnitDodgeChance(WeaponAttackType attType, Unit const* victim) const;
    [[nodiscard]] float GetUnitParryChance(WeaponAttackType attType, Unit const* victim) const;
    [[nodiscard]] float GetUnitBlockChance(Unit const* victim) const;

    [[nodiscard]] float GetMeleeMissChance(Unit const* victim) const;
    [[nodiscard]] float GetUnitMissChance(WeaponAttackType attType) const;
    float GetUnitCriticalChanceDone(WeaponAttackType attackType) const;
    float GetUnitCriticalChanceTaken(WeaponAttackType attackType, float critDone) const;
    float GetUnitCriticalChance(WeaponAttackType attackType, Unit const* victim) const;
    MeleeHitOutcome RollMeleeOutcomeAgainst (Unit const* victim, WeaponAttackType attType) const;
    MeleeHitOutcome RollMeleeOutcomeAgainst (Unit const* victim, WeaponAttackType attType, int32 crit_chance, int32 miss_chance, int32 dodge_chance, int32 parry_chance, int32 block_chance) const;

    // Resilience
    static void ApplyResilience(Unit const* victim, float* crit, int32* damage, bool isCrit, CombatRating type);
    [[nodiscard]] bool CanApplyResilience() const { return m_applyResilience; }

    // Skills values
    [[nodiscard]] virtual uint32 GetShieldBlockValue() const = 0;
    [[nodiscard]] uint32 GetShieldBlockValue(uint32 soft_cap, uint32 hard_cap) const
    {
        uint32 value = GetShieldBlockValue();
        if (value >= hard_cap)
        {
            value = (soft_cap + hard_cap) / 2;
        }
        else if (value > soft_cap)
        {
            value = soft_cap + ((value - soft_cap) / 2);
        }

        return value;
    }

    // Resistances methods
    int32 GetMechanicResistChance(SpellInfo const* spell);
    [[nodiscard]] uint32 GetResistance(SpellSchoolMask mask) const;
    [[nodiscard]] uint32 GetResistance(SpellSchools school) const { return GetUInt32Value(static_cast<uint16>(UNIT_FIELD_RESISTANCES) + school); }
    float CalculateAverageResistReduction(Unit const* attacker, SpellSchoolMask schoolMask, Unit const* victim, SpellInfo const* spellInfo);
    [[nodiscard]] float GetResistanceBuffMods(SpellSchools school, bool positive) const { return GetFloatValue(positive ? static_cast<uint16>(UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE) + school : static_cast<uint16>(UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE) +  + school); }

    void SetResistance(SpellSchools school, int32 val) { SetStatInt32Value(static_cast<uint16>(UNIT_FIELD_RESISTANCES) + school, val); }
    void SetResistanceBuffMods(SpellSchools school, bool positive, float val) { SetFloatValue(positive ? static_cast<uint16>(UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE) + school : static_cast<uint16>(UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE) +  + school, val); }

    void ApplyResistanceBuffModsMod(SpellSchools school, bool positive, float val, bool apply) { ApplyModSignedFloatValue(positive ? static_cast<uint16>(UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE) + school : static_cast<uint16>(UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE) +  + school, val, apply); }
    void ApplyResistanceBuffModsPercentMod(SpellSchools school, bool positive, float val, bool apply) { ApplyPercentModFloatValue(positive ? static_cast<uint16>(UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE) + school : static_cast<uint16>(UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE) +  + school, val, apply); }

    uint16 GetMaxSkillValueForLevel(Unit const* target = nullptr) const { return (target ? getLevelForTarget(target) : GetLevel()) * 5; }
    [[nodiscard]] float GetTotalAuraModValue(UnitMods unitMod) const;

    [[nodiscard]] SpellSchools GetSpellSchoolByAuraGroup(UnitMods unitMod) const;

    [[nodiscard]] float GetWeaponDamageRange(WeaponAttackType attType, WeaponDamageRange type, uint8 damageIndex = 0) const;
    float GetTotalAttackPowerValue(WeaponAttackType attType, Unit* pVictim = nullptr) const;
    void SetBaseWeaponDamage(WeaponAttackType attType, WeaponDamageRange damageRange, float value, uint8 damageIndex = 0) { m_weaponDamage[attType][damageRange][damageIndex] = value; }

    // Virtual methods
    virtual bool UpdateStats(Stats stat) = 0;
    virtual bool UpdateAllStats() = 0;
    virtual void UpdateResistances(uint32 school) = 0;
    virtual void UpdateAllResistances();
    virtual void UpdateArmor() = 0;
    virtual void UpdateMaxHealth() = 0;
    virtual void UpdateMaxPower(Powers power) = 0;
    virtual void UpdateDamagePhysical(WeaponAttackType attType);


    /*********************************************************/
    /***       METHODS RELATED TO DAMAGE CACULATIONS       ***/
    /*********************************************************/

    static uint32 DealDamage(Unit* attacker, Unit* victim, uint32 damage, CleanDamage const* cleanDamage = nullptr, DamageEffectType damagetype = DIRECT_DAMAGE, SpellSchoolMask damageSchoolMask = SPELL_SCHOOL_MASK_NORMAL, SpellInfo const* spellProto = nullptr, bool durabilityLoss = true, bool allowGM = false, Spell const* spell = nullptr);
    void DealMeleeDamage(CalcDamageInfo* damageInfo, bool durabilityLoss);
    void DealSpellDamage(SpellNonMeleeDamage* damageInfo, bool durabilityLoss, Spell const* spell = nullptr);
    static void DealDamageMods(Unit const* victim, uint32& damage, uint32* absorb);

    static void Kill(Unit* killer, Unit* victim, bool durabilityLoss = true, WeaponAttackType attackType = BASE_ATTACK, SpellInfo const* spellProto = nullptr, Spell const* spell = nullptr);
    void KillSelf(bool durabilityLoss = true, WeaponAttackType attackType = BASE_ATTACK, SpellInfo const* spellProto = nullptr, Spell const* spell = nullptr) { Kill(this, this, durabilityLoss, attackType, spellProto, spell); };

    // Calculate methods
    uint32 CalculateDamage(WeaponAttackType attType, bool normalized, bool addTotalPct, uint8 itemDamagesMask = 0);
    virtual void CalculateMinMaxDamage(WeaponAttackType attType, bool normalized, bool addTotalPct, float& minDamage, float& maxDamage, uint8 damageIndex = 0) = 0;   
    void CalculateMeleeDamage(Unit* victim, CalcDamageInfo* damageInfo, WeaponAttackType attackType = BASE_ATTACK, const bool sittingVictim = false);
    void CalculateSpellDamageTaken(SpellNonMeleeDamage* damageInfo, int32 damage, SpellInfo const* spellInfo, WeaponAttackType attackType = BASE_ATTACK, bool crit = false);
    int32 CalculateSpellDamage(Unit const* target, SpellInfo const* spellProto, uint8 effect_index, int32 const* basePoints = nullptr) const;
    float CalculateDefaultCoefficient(SpellInfo const* spellInfo, DamageEffectType damagetype) const;

    // Melee damage bonus
    uint32 MeleeDamageBonusDone(
        Unit* pVictim, uint32 damage, WeaponAttackType attType, SpellInfo const* spellProto = nullptr, bool useSpellBonusData = true, SpellSchoolMask damageSchoolMask = SPELL_SCHOOL_MASK_NORMAL);
    uint32 MeleeDamageBonusTaken(Unit* attacker, uint32 pdamage, WeaponAttackType attType, SpellInfo const* spellProto = nullptr, SpellSchoolMask damageSchoolMask = SPELL_SCHOOL_MASK_NORMAL);

    // Spell damage bonus
    int32 SpellBaseDamageBonusDone(SpellSchoolMask schoolMask, bool withSpellPowerPctMod = false) const;
    int32 SpellBaseDamageBonusTaken(SpellSchoolMask schoolMask, bool isDoT = false);
    float SpellPctDamageModsDone(Unit* victim, SpellInfo const* spellProto, DamageEffectType damagetype);
    uint32 SpellDamageBonusDone(Unit* victim, SpellInfo const* spellProto, uint32 pdamage, DamageEffectType damagetype, uint8 effIndex, float TotalMod = 0.0f, uint32 stack = 1);
    uint32 SpellDamageBonusTaken(Unit* caster, SpellInfo const* spellProto, uint32 pdamage, DamageEffectType damagetype, uint32 stack = 1);
    float SpellDamagePctDone(Unit* victim, SpellInfo const* spellProto, DamageEffectType damagetype) const;

    // AOE damages
    int32 CalculateAOEDamageReduction(int32 damage, uint32 schoolMask, bool npcCaster) const;

    // Armor reduction
    static bool IsDamageReducedByArmor(SpellSchoolMask damageSchoolMask, SpellInfo const* spellInfo = nullptr, uint8 effIndex = MAX_SPELL_EFFECTS);
    static uint32 CalcArmorReducedDamage(Unit const* attacker, Unit const* victim, const uint32 damage, SpellInfo const* spellInfo, uint8 attackerLevel = 0, WeaponAttackType attackType = MAX_ATTACK);

    /*********************************************************/
    /***         METHODS RELATED TO CHARM SYSTEM           ***/
    /*********************************************************/
    CharmInfo* GetCharmInfo() { return m_charmInfo; }
    CharmInfo* InitCharmInfo();
    
    [[nodiscard]] Unit* GetCharmer() const;
    [[nodiscard]] Unit* GetCharm() const;
    [[nodiscard]] Unit* GetCharmerOrOwner() const { return GetCharmerGUID() ? GetCharmer() : GetOwner(); }
    [[nodiscard]] Unit* GetCharmerOrOwnerOrSelf() const
    {
        if (Unit* u = GetCharmerOrOwner())
            return u;

        return (Unit*)this;
    }
    [[nodiscard]] Player* GetCharmerOrOwnerPlayerOrPlayerItself() const;
    [[nodiscard]] ObjectGuid GetCharmerOrOwnerGUID() const { return GetCharmerGUID() ? GetCharmerGUID() : GetOwnerGUID(); }
    [[nodiscard]] ObjectGuid GetCharmerOrOwnerOrOwnGUID() const
    {
        if (ObjectGuid guid = GetCharmerOrOwnerGUID())
            return guid;

        return GetGUID();
    }

    [[nodiscard]] Player* GetAffectingPlayer() const;
    [[nodiscard]] Unit* GetFirstControlled() const;

    [[nodiscard]] bool IsControlledByPlayer() const { return m_ControlledByPlayer; }
    [[nodiscard]] bool IsCreatedByPlayer() const { return m_CreatedByPlayer; }
    [[nodiscard]] bool IsCharmedOwnedByPlayerOrPlayer() const { return GetCharmerOrOwnerOrOwnGUID().IsPlayer(); }
    [[nodiscard]] bool IsCharmed() const { return GetCharmerGUID(); }
    [[nodiscard]] bool isPossessed() const { return HasUnitState(UNIT_STATE_POSSESSED); }
    [[nodiscard]] bool isPossessedByPlayer() const { return HasUnitState(UNIT_STATE_POSSESSED) && GetCharmerGUID().IsPlayer(); }
    [[nodiscard]] bool isPossessing() const
    {
        if (Unit* u = GetCharm())
            return u->isPossessed();
        else
            return false;
    }
    bool isPossessing(Unit* u) const { return u->isPossessed() && IsCharmed() == u->GetGUID(); }

    void SetCharm(Unit* target, bool apply);
    bool SetCharmedBy(Unit* charmer, CharmType type, AuraApplication const* aurApp = nullptr);

    void UpdateCharmAI();

    void RemoveCharmedBy(Unit* charmer);
    void RemoveCharmAuras();
    void RemoveAllControlled(bool onDeath = false);
    void DeleteCharmInfo();

    // base client control of this unit (possess effects, vehicles and similar). Not affected by temporary CC.
    bool IsCharmerOrSelfPlayer() const { return GetCharmerOrSelf()->IsPlayer(); }
    Unit* GetCharmerOrSelf() const;
    Player* GetCharmerOrSelfPlayer() const { return GetCharmerOrSelf()->ToPlayer(); }
    Unit* GetCharmedOrSelf() const { return IsCharmed() ? GetCharm() : const_cast<Unit*>(this); }

    /*********************************************************/
    /***          METHODS RELATED TO COOLDOWNS             ***/
    /*********************************************************/
    [[nodiscard]] virtual bool HasSpellCooldown(uint32 /*spell_id*/) const { return false; }
    [[nodiscard]] virtual bool HasSpellItemCooldown(uint32 /*spell_id*/, uint32 /*itemid*/) const { return false; }
    virtual void AddSpellCooldown(uint32 /*spell_id*/, uint32 /*itemid*/, uint32 /*end_time*/, bool needSendToClient = false, bool forceSendToSpectator = false)
    {
        // workaround for unused parameters
        (void)needSendToClient;
        (void)forceSendToSpectator;
    }

    void BuildCooldownPacket(WorldPacket& data, uint8 flags, uint32 spellId, uint32 cooldown);
    void BuildCooldownPacket(WorldPacket& data, uint8 flags, PacketCooldowns const& cooldowns);

    /*********************************************************/
    /***            METHODS RELATED TO AURAS               ***/
    /*********************************************************/
    Aura* AddAura(uint32 spellId, Unit* target);
    Aura* AddAura(SpellInfo const* spellInfo, uint8 effMask, Unit* target);

    void SetAuraStack(uint32 spellId, Unit* target, uint32 stack);

    // aura apply/remove helpers - you should better not use these
    Aura* _TryStackingOrRefreshingExistingAura(SpellInfo const* newAura, uint8 effMask, Unit* caster, int32* baseAmount = nullptr, Item* castItem = nullptr, ObjectGuid casterGUID = ObjectGuid::Empty, bool periodicReset = false);
    void _AddAura(UnitAura* aura, Unit* caster);
    AuraApplication* _CreateAuraApplication(Aura* aura, uint8 effMask);

    void _ApplyAuraEffect(Aura* aura, uint8 effIndex);
    void _ApplyAura(AuraApplication* aurApp, uint8 effMask);
    void _UnapplyAura(AuraApplicationMap::iterator& i, AuraRemoveMode removeMode);
    void _UnapplyAura(AuraApplication* aurApp, AuraRemoveMode removeMode);
    void _RemoveNoStackAuraApplicationsDueToAura(Aura* aura);
    void _RemoveNoStackAurasDueToAura(Aura* aura);
    bool _IsNoStackAuraDueToAura(Aura* appliedAura, Aura* existingAura) const;
    void _RegisterAuraEffect(AuraEffect* aurEff, bool apply);

    // m_ownedAuras container management
    AuraMap&       GetOwnedAuras()       { return m_ownedAuras; }
    [[nodiscard]] AuraMap const& GetOwnedAuras() const { return m_ownedAuras; }

    void RemoveOwnedAura(AuraMap::iterator& i, AuraRemoveMode removeMode = AuraRemoveMode::ByDefault);
    void RemoveOwnedAura(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0, AuraRemoveMode removeMode = AuraRemoveMode::ByDefault);
    void RemoveOwnedAura(Aura* aura, AuraRemoveMode removeMode = AuraRemoveMode::ByDefault);

    Aura* GetOwnedAura(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, ObjectGuid itemCasterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0, Aura* except = nullptr) const;

    // m_appliedAuras container management
    AuraApplicationMap&       GetAppliedAuras()       { return m_appliedAuras; }
    [[nodiscard]] AuraApplicationMap const& GetAppliedAuras() const { return m_appliedAuras; }

    void RemoveAura(AuraApplicationMap::iterator& i, AuraRemoveMode mode = AuraRemoveMode::ByDefault);
    void RemoveAura(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0, AuraRemoveMode removeMode = AuraRemoveMode::ByDefault);
    void RemoveAura(AuraApplication* aurApp, AuraRemoveMode mode = AuraRemoveMode::ByDefault);
    void RemoveAura(Aura* aur, AuraRemoveMode mode = AuraRemoveMode::ByDefault);

    void RemoveBindSightAuras();

    // Convenience methods removing auras by predicate
    void RemoveAppliedAuras(std::function<bool(AuraApplication const*)> const& check);
    void RemoveOwnedAuras(std::function<bool(Aura const*)> const& check);

    // Optimized overloads taking advantage of map key
    void RemoveAppliedAuras(uint32 spellId, std::function<bool(AuraApplication const*)> const& check);
    void RemoveOwnedAuras(uint32 spellId, std::function<bool(Aura const*)> const& check);

    void RemoveAurasDueToSpell(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0, AuraRemoveMode removeMode = AuraRemoveMode::ByDefault);
    void RemoveAuraFromStack(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, AuraRemoveMode removeMode = AuraRemoveMode::ByDefault);
    void RemoveAurasDueToSpellByDispel(uint32 spellId, uint32 dispellerSpellId, ObjectGuid casterGUID, Unit* dispeller, uint8 chargesRemoved = 1);
    void RemoveAurasDueToSpellBySteal(uint32 spellId, ObjectGuid casterGUID, Unit* stealer);
    void RemoveAurasDueToItemSpell(uint32 spellId, ObjectGuid castItemGuid);
    void RemoveAurasByType(AuraType auraType, ObjectGuid casterGUID = ObjectGuid::Empty, Aura* except = nullptr, bool negative = true, bool positive = true);
    void RemoveNotOwnSingleTargetAuras();
    template <typename InterruptFlags> void RemoveAurasWithInterruptFlags(InterruptFlags flag, uint32 except = 0, Spell* interruptingSpell = nullptr);
    void RemoveAurasWithAttribute(uint32 flags);
    void RemoveAurasWithFamily(SpellFamilyNames family, uint32 familyFlag1, uint32 familyFlag2, uint32 familyFlag3, ObjectGuid casterGUID);
    void RemoveAurasWithMechanic(uint32 mechanic_mask, AuraRemoveMode removemode = AuraRemoveMode::ByDefault, uint32 except = 0);
    void RemoveMovementImpairingAuras(bool withRoot);
    void RemoveAurasByShapeShift();

    // @todo do we need?
    void RemoveNotOwnLimitedTargetAuras(bool onPhaseChange = false);
    void RemoveAurasOnEvade();
    void RemoveAllGroupBuffsFromCaster(ObjectGuid casterGUID);

    void RemoveAreaAurasDueToLeaveWorld();
    void RemoveAllAuras();
    void RemoveArenaAuras();
    void RemoveAllAurasOnDeath();
    void RemoveAllAurasRequiringDeadTarget();
    void RemoveAllAurasExceptType(AuraType type);
    //void RemoveAllAurasExceptType(AuraType type1, AuraType type2); // pussywizard: replaced with RemoveEvadeAuras()
    void RemoveEvadeAuras();
    void DelayOwnedAuras(uint32 spellId, ObjectGuid caster, int32 delaytime);

    void _RemoveAllAuraStatMods();
    void _ApplyAllAuraStatMods();

    [[nodiscard]] AuraEffectList const& GetAuraEffectsByType(AuraType type) const { return m_modAuras[type]; }
    AuraList&       GetSingleCastAuras()       { return m_scAuras; }
    [[nodiscard]] AuraList const& GetSingleCastAuras() const { return m_scAuras; }

    [[nodiscard]] AuraEffect* GetAuraEffect(uint32 spellId, uint8 effIndex, ObjectGuid casterGUID = ObjectGuid::Empty) const;
    [[nodiscard]] AuraEffect* GetAuraEffectOfRankedSpell(uint32 spellId, uint8 effIndex, ObjectGuid casterGUID = ObjectGuid::Empty) const;
    [[nodiscard]] AuraEffect* GetAuraEffect(AuraType type, SpellFamilyNames name, uint32 iconId, uint8 effIndex) const; // spell mustn't have familyflags
    [[nodiscard]] AuraEffect* GetAuraEffect(AuraType type, SpellFamilyNames family, uint32 familyFlag1, uint32 familyFlag2, uint32 familyFlag3, ObjectGuid casterGUID = ObjectGuid::Empty) const;
    [[nodiscard]] AuraEffect* GetAuraEffectDummy(uint32 spellid) const;
    [[nodiscard]] inline AuraEffect* GetDummyAuraEffect(SpellFamilyNames name, uint32 iconId, uint8 effIndex) const { return GetAuraEffect(SPELL_AURA_DUMMY, name, iconId, effIndex);}
    AuraList& GetLimitedCastAuras(uint32 spellId) { return m_ltAuras[spellId]; }
    AurasBySpellIdMap& GetAllLimitedCastAuras() { return m_ltAuras; }
    bool HasLimitedTargetAuraForSpell(uint32 spellId) const;

    AuraApplication* GetAuraApplication(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, ObjectGuid itemCasterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0, AuraApplication* except = nullptr) const;
    [[nodiscard]] Aura* GetAura(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, ObjectGuid itemCasterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0) const;

    AuraApplication* GetAuraApplicationOfRankedSpell(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, ObjectGuid itemCasterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0, AuraApplication* except = nullptr) const;
    [[nodiscard]] Aura* GetAuraOfRankedSpell(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, ObjectGuid itemCasterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0) const;

    void GetDispellableAuraList(Unit* caster, uint32 dispelMask, DispelChargesList& dispelList, SpellInfo const* dispelSpell);

    [[nodiscard]] bool HasAuraEffect(uint32 spellId, uint8 effIndex, ObjectGuid caster = ObjectGuid::Empty) const;
    [[nodiscard]] uint32 GetAuraCount(uint32 spellId) const;

    /**
    * @brief Check if unit has ANY or ALL specified auras.
    *
    * @param sm The search method to use
    *           - SearchMethod::MatchAll : The function checks for all of the spell id's on the unit.
    *           - SearchMethod::MatchAny : The function checks for any of the spell id's on the unit.
    *
    * @param spellIds List of spell id's to check for on the unit.
    *
    * @return Returns true if the search method condition is met. Otherwise false.
    */
    bool HasAuras(SearchMethod sm, std::vector<uint32>& spellIds) const;

    /**
     * @brief Checks if the unit has ANY specified auras.
     *
     * @tparam Auras Can be any type convertible to uint32.
     * @param spellIds List of spell id's to check for on the unit.
     *
     * @return Returns true if the unit has ANY of the specified auras. Otherwise false.
     */
    template <typename... Auras>
    bool HasAnyAuras(Auras... spellIds) const
    {
        std::vector<uint32> spellList = { static_cast<uint32>(spellIds)... };
        return HasAuras(SearchMethod::MatchAny, spellList);
    }

    /**
     * @brief Checks if the unit has ALL specified auras.
     *
     * @tparam Auras Can be any type convertible to uint32.
     * @param spellIds List of spell id's to check for on the unit.
     *
     * @return Returns true if the unit has ALL of the specified auras. Otherwise false.
     */
    template <typename... Auras>
    bool HasAllAuras(Auras... spellIds) const
    {
        std::vector<uint32> spellList = { static_cast<uint32>(spellIds)... };
        return HasAuras(SearchMethod::MatchAll, spellList);
    }

    [[nodiscard]] bool HasAura(uint32 spellId, ObjectGuid casterGUID = ObjectGuid::Empty, ObjectGuid itemCasterGUID = ObjectGuid::Empty, uint8 reqEffMask = 0) const;
    [[nodiscard]] bool HasAuraType(AuraType auraType) const;
    [[nodiscard]] bool HasAuraTypeWithCaster(AuraType auratype, ObjectGuid caster) const;
    [[nodiscard]] bool HasAuraTypeWithMiscvalue(AuraType auratype, int32 miscvalue) const;
    bool HasAuraTypeWithAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const;
    [[nodiscard]] bool HasAuraTypeWithValue(AuraType auratype, int32 value) const;
    [[nodiscard]] bool HasAuraTypeWithTriggerSpell(AuraType auratype, uint32 triggerSpell) const;
    template <typename InterruptFlags> bool HasNegativeAuraWithInterruptFlag(InterruptFlags flag, ObjectGuid guid = ObjectGuid::Empty) const;
    [[nodiscard]] bool HasVisibleAuraType(AuraType auraType) const;
    bool HasNegativeAuraWithAttribute(uint32 flag, ObjectGuid guid = ObjectGuid::Empty);
    [[nodiscard]] bool HasAuraWithMechanic(uint32 mechanicMask) const;
    bool HasStrongerAuraWithDR(SpellInfo const* auraSpellInfo, Unit* caster, bool triggered) const;

    [[nodiscard]] bool HasAuraTypeWithFamilyFlags(AuraType auraType, uint32 familyName, uint32 familyFlags) const;

    [[nodiscard]] bool HasBreakableByDamageAuraType(AuraType type, uint32 excludeAura = 0) const;
    bool HasBreakableByDamageCrowdControlAura(Unit* excludeCasterChannel = nullptr) const;

    AuraEffect* IsScriptOverriden(SpellInfo const* spell, int32 script) const;
    uint32 GetDiseasesByCaster(ObjectGuid casterGUID, uint8 mode = 0);
    [[nodiscard]] uint32 GetDoTsByCaster(ObjectGuid casterGUID) const;

    [[nodiscard]] int32 GetTotalAuraModifierAreaExclusive(AuraType auratype) const;
    [[nodiscard]] int32 GetTotalAuraModifier(AuraType auratype) const;
    [[nodiscard]] float GetTotalAuraMultiplier(AuraType auratype) const;
    int32 GetMaxPositiveAuraModifier(AuraType auratype);
    [[nodiscard]] int32 GetMaxNegativeAuraModifier(AuraType auratype) const;

    [[nodiscard]] int32 GetTotalAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask) const;
    [[nodiscard]] float GetTotalAuraMultiplierByMiscMask(AuraType auratype, uint32 misc_mask) const;
    int32 GetMaxPositiveAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask, const AuraEffect* except = nullptr) const;
    [[nodiscard]] int32 GetMaxNegativeAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask) const;

    [[nodiscard]] int32 GetTotalAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const;
    [[nodiscard]] float GetTotalAuraMultiplierByMiscValue(AuraType auratype, int32 misc_value) const;
    [[nodiscard]] int32 GetMaxPositiveAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const;
    [[nodiscard]] int32 GetMaxNegativeAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const;

    int32 GetTotalAuraModifierByAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const;
    float GetTotalAuraMultiplierByAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const;
    int32 GetMaxPositiveAuraModifierByAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const;
    int32 GetMaxNegativeAuraModifierByAffectMask(AuraType auratype, SpellInfo const* affectedSpell) const;

    VisibleAuraMap const* GetVisibleAuras() { return &m_visibleAuras; }
    AuraApplication* GetVisibleAura(uint8 slot)
    {
        VisibleAuraMap::iterator itr = m_visibleAuras.find(slot);
        if (itr != m_visibleAuras.end())
            return itr->second;
        return nullptr;
    }
    
    void SetVisibleAura(uint8 slot, AuraApplication* aur) { m_visibleAuras[slot] = aur; UpdateAuraForGroup(slot);}
    void RemoveVisibleAura(uint8 slot) { m_visibleAuras.erase(slot); UpdateAuraForGroup(slot);}

    void ModifyAuraState(AuraStateType flag, bool apply);
    uint32 BuildAuraStateUpdateForTarget(Unit* target) const;
    bool HasAuraState(AuraStateType flag, SpellInfo const* spellProto = nullptr, Unit const* Caster = nullptr) const;

    /*********************************************************/
    /***         METHODS RELATED TO PROCS SYSTEM           ***/
    /*********************************************************/
    bool CanProc() { return !m_procDeep; }
    void SetCantProc(bool apply);

    static void ProcDamageAndSpell(Unit* actor, Unit* victim, uint32 procAttacker, uint32 procVictim, uint32 procEx, uint32 amount, WeaponAttackType attType = BASE_ATTACK, SpellInfo const* procSpellInfo = nullptr, SpellInfo const* procAura = nullptr, int8 procAuraEffectIndex = -1, Spell const* procSpell = nullptr, DamageInfo* damageInfo = nullptr, HealInfo* healInfo = nullptr, uint32 procPhase = 2 /*PROC_SPELL_PHASE_HIT*/);
    void ProcDamageAndSpellFor(bool isVictim, Unit* target, uint32 procFlag, uint32 procExtra, WeaponAttackType attType, SpellInfo const* procSpellInfo, uint32 damage, SpellInfo const* procAura = nullptr, int8 procAuraEffectIndex = -1, Spell const* procSpell = nullptr, DamageInfo* damageInfo = nullptr, HealInfo* healInfo = nullptr, uint32 procPhase = 2 /*PROC_SPELL_PHASE_HIT*/);

    void GetProcAurasTriggeredOnEvent(std::list<AuraApplication*>& aurasTriggeringProc, std::list<AuraApplication*>* procAuras, ProcEventInfo eventInfo);

    void TriggerAurasProcOnEvent(CalcDamageInfo& damageInfo);
    void TriggerAurasProcOnEvent(std::list<AuraApplication*>* myProcAuras, std::list<AuraApplication*>* targetProcAuras, Unit* actionTarget, uint32 typeMaskActor, uint32 typeMaskActionTarget, uint32 spellTypeMask, uint32 spellPhaseMask, uint32 hitMask, Spell* spell, DamageInfo* damageInfo, HealInfo* healInfo);
    void TriggerAurasProcOnEvent(ProcEventInfo& eventInfo, std::list<AuraApplication*>& procAuras);

    [[nodiscard]] float GetWeaponProcChance() const;
    float GetPPMProcChance(uint32 WeaponSpeed, float PPM,  SpellInfo const* spellProto) const;

    /*********************************************************/
    /***            METHODS RELATED TO SPELLS              ***/
    /*********************************************************/
    [[nodiscard]] Player* GetSpellModOwner() const;
    [[nodiscard]] Spell* GetCurrentSpell(CurrentSpellTypes spellType) const { return m_currentSpells[spellType]; }
    [[nodiscard]] Spell* GetCurrentSpell(uint32 spellType) const { return m_currentSpells[spellType]; }
    [[nodiscard]] Spell* FindCurrentSpellBySpellId(uint32 spell_id) const;
    [[nodiscard]] int32 GetCurrentSpellCastTime(uint32 spell_id) const;

    [[nodiscard]] bool virtual HasSpell(uint32 /*spellID*/) const { return false; }
    void SetCurrentCastedSpell(Spell* pSpell);
    virtual void ProhibitSpellSchool(SpellSchoolMask /*idSchoolMask*/, uint32 /*unTimeMs*/) { }

    void SetInstantCast(bool set) { _instantCast = set; }
    [[nodiscard]] bool CanInstantCast() const { return _instantCast; }

    // set withDelayed to true to account delayed spells as casted
    // delayed+channeled spells are always accounted as casted
    // we can skip channeled or delayed checks using flags
    [[nodiscard]] bool IsNonMeleeSpellCast(bool withDelayed, bool skipChanneled = false, bool skipAutorepeat = false, bool isAutoshoot = false, bool skipInstant = true) const;

    // set withDelayed to true to interrupt delayed spells too
    // delayed+channeled spells are always interrupted
    void InterruptNonMeleeSpells(bool withDelayed, uint32 spellid = 0, bool withInstant = true, bool bySelf = false);

    // target dependent range checks
    float GetSpellMaxRangeForTarget(Unit const* target, SpellInfo const* spellInfo) const;
    float GetSpellMinRangeForTarget(Unit const* target, SpellInfo const* spellInfo) const;

    // Spell interrupt
    bool HasInterruptFlag(SpellAuraInterruptFlags flags) const { return m_interruptMask.HasFlag(flags); }
    bool HasInterruptFlag(SpellAuraInterruptFlags2 flags) const { return m_interruptMask2.HasFlag(flags); }
    void AddInterruptMask(SpellAuraInterruptFlags flags, SpellAuraInterruptFlags2 flags2)
    {
        m_interruptMask |= flags;
        m_interruptMask2 |= flags2;
    }

    void UpdateInterruptMask();
    void InterruptSpell(CurrentSpellTypes spellType, bool withDelayed = true, bool withInstant = true, bool bySelf = false);
    bool isSpellBlocked(Unit* victim, SpellInfo const* spellProto, WeaponAttackType attackType = BASE_ATTACK);
    void FinishSpell(CurrentSpellTypes spellType, bool ok = true);

    // Spell hit methods
    Unit* GetMagicHitRedirectTarget(Unit* victim, SpellInfo const* spellInfo);
    Unit* GetMeleeHitRedirectTarget(Unit* victim, SpellInfo const* spellInfo = nullptr);
    [[nodiscard]] float MeleeSpellMissChance(Unit const* victim, WeaponAttackType attType, int32 skillDiff, uint32 spellId) const;
    [[nodiscard]] SpellMissInfo MeleeSpellHitResult(Unit* victim, SpellInfo const* spell);
    [[nodiscard]] SpellMissInfo MagicSpellHitResult(Unit* victim, SpellInfo const* spell);
    [[nodiscard]] SpellMissInfo SpellHitResult(Unit* victim, SpellInfo const* spell, bool canReflect = false);
    [[nodiscard]] SpellMissInfo SpellHitResult(Unit* victim, Spell const* spell, bool canReflect = false);

    // Healling spells
    static int32 DealHeal(Unit* healer, Unit* victim, uint32 addhealth);
    void SendHealSpellLog(HealInfo const& healInfo, bool critical = false);
    int32 HealBySpell(HealInfo& healInfo, bool critical = false);

    
    uint32 SpellHealingBonusDone(Unit* victim, SpellInfo const* spellProto, uint32 healamount, DamageEffectType damagetype, uint8 effIndex, uint32 stack = 1) const;
    int32 SpellBaseHealingBonusDone(SpellSchoolMask schoolMask);
    int32 SpellBaseHealingBonusTaken(SpellSchoolMask schoolMask);
    float SpellPctHealingModsDone(Unit* victim, SpellInfo const* spellProto, DamageEffectType damagetype);
    uint32 SpellHealingBonusDone(Unit* victim, SpellInfo const* spellProto, uint32 healamount, DamageEffectType damagetype, uint8 effIndex, float TotalMod = 0.0f, uint32 stack = 1);
    uint32 SpellHealingBonusTaken(Unit* caster, SpellInfo const* spellProto, uint32 healamount, DamageEffectType damagetype, uint32 stack = 1);
    static uint32 SpellCriticalHealingBonus(Unit const* caster, uint32 damage);

    static void CalcAbsorbResist(DamageInfo& dmgInfo, bool Splited = false);
    static void CalcHealAbsorb(HealInfo& healInfo);

    // Energize spells
    void SendEnergizeSpellLog(Unit* victim, uint32 SpellID, uint32 Damage, Powers powertype);
    void EnergizeBySpell(Unit* victim, uint32 SpellID, uint32 Damage, Powers powertype);

    // Spells immunities
    void ApplySpellImmune(uint32 spellId, uint32 op, uint32 type, bool apply, SpellImmuneBlockType blockType = SPELL_BLOCK_TYPE_ALL);
    void ApplySpellDispelImmunity(SpellInfo const* spellProto, DispelType type, bool apply);
    virtual bool IsImmunedToSpell(SpellInfo const* spellInfo, Spell const* spell = nullptr);

    [[nodiscard]] bool IsImmunedToDamage(SpellSchoolMask meleeSchoolMask) const;
    [[nodiscard]] bool IsImmunedToDamage(SpellInfo const* spellInfo) const;
    [[nodiscard]] bool IsImmunedToDamage(Spell const* spell) const;
    [[nodiscard]] bool IsImmunedToSchool(SpellSchoolMask meleeSchoolMask) const;
    [[nodiscard]] bool IsImmunedToSchool(SpellInfo const* spellInfo) const;
    [[nodiscard]] bool IsImmunedToSchool(Spell const* spell) const;
    [[nodiscard]] bool IsImmunedToDamageOrSchool(SpellSchoolMask meleeSchoolMask) const;
    bool IsImmunedToDamageOrSchool(SpellInfo const* spellInfo) const;
    virtual bool IsImmunedToSpellEffect(SpellInfo const* spellInfo, uint32 index) const;

    // Critic chances
    bool isBlockCritical();
    float SpellDoneCritChance(Unit const* /*victim*/, SpellInfo const* spellProto, SpellSchoolMask schoolMask, WeaponAttackType attackType, bool skipEffectCheck) const;
    float SpellTakenCritChance(Unit const* caster, SpellInfo const* spellProto, SpellSchoolMask schoolMask, float doneChance, WeaponAttackType attackType, bool skipEffectCheck) const;

    // Others modifiers
    float ApplyEffectModifiers(SpellInfo const* spellProto, uint8 effect_index, float value) const;
    int32 CalcSpellDuration(SpellInfo const* spellProto);
    int32 ModSpellDuration(SpellInfo const* spellProto, Unit const* target, int32 duration, bool positive, uint32 effectMask);
    void  ModSpellCastTime(SpellInfo const* spellProto, int32& castTime, Spell* spell = nullptr);
    float CalculateLevelPenalty(SpellInfo const* spellProto) const;

    uint32 GetCastingTimeForBonus(SpellInfo const* spellProto, DamageEffectType damagetype, uint32 CastingTime) const;
    void CastDelayedSpellWithPeriodicAmount(Unit* caster, uint32 spellId, AuraType auraType, int32 addAmount, uint8 effectIndex = 0);

    // SpellCastResult methods
    SpellCastResult CastSpell(SpellCastTargets const& targets, SpellInfo const* spellInfo, CustomSpellValues const* value, TriggerCastFlags triggerFlags = TRIGGERED_NONE, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    SpellCastResult CastSpell(Unit* victim, uint32 spellId, bool triggered, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    SpellCastResult CastSpell(Unit* victim, uint32 spellId, TriggerCastFlags triggerFlags = TRIGGERED_NONE, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    SpellCastResult CastSpell(Unit* victim, SpellInfo const* spellInfo, bool triggered, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    SpellCastResult CastSpell(Unit* victim, SpellInfo const* spellInfo, TriggerCastFlags triggerFlags = TRIGGERED_NONE, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    SpellCastResult CastSpell(float x, float y, float z, uint32 spellId, bool triggered, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    SpellCastResult CastSpell(GameObject* go, uint32 spellId, bool triggered, Item* castItem = nullptr, AuraEffect* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    SpellCastResult CastCustomSpell(Unit* victim, uint32 spellId, int32 const* bp0, int32 const* bp1, int32 const* bp2, bool triggered, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    SpellCastResult CastCustomSpell(uint32 spellId, SpellValueMod mod, int32 value, Unit* victim, bool triggered, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    SpellCastResult CastCustomSpell(uint32 spellId, SpellValueMod mod, int32 value, Unit* victim = nullptr, TriggerCastFlags triggerFlags = TRIGGERED_NONE, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);
    SpellCastResult CastCustomSpell(uint32 spellId, CustomSpellValues const& value, Unit* victim = nullptr, TriggerCastFlags triggerFlags = TRIGGERED_NONE, Item* castItem = nullptr, AuraEffect const* triggeredByAura = nullptr, ObjectGuid originalCaster = ObjectGuid::Empty);

    /*********************************************************/
    /***     METHODS RELATED TO GAMEOBJECT & DYNOBEJCTS    ***/
    /*********************************************************/
    void _RegisterDynObject(DynamicObject* dynObj);
    void _UnregisterDynObject(DynamicObject* dynObj);
    DynamicObject* GetDynObject(uint32 spellId);
    bool RemoveDynObject(uint32 spellId);
    void RemoveAllDynObjects();

    [[nodiscard]] GameObject* GetGameObject(uint32 spellId) const;
    void AddGameObject(GameObject* gameObj);
    void RemoveGameObject(GameObject* gameObj, bool del);
    void RemoveGameObject(uint32 spellid, bool del);
    void RemoveAllGameObjects();

    /*********************************************************/
    /***           METHODS RELATED TO MOVEMENTS            ***/
    /*********************************************************/
    [[nodiscard]] bool IsPolymorphed() const;
    [[nodiscard]] bool isFrozen() const;
    [[nodiscard]] bool IsInFlight()  const { return HasUnitState(UNIT_STATE_IN_FLIGHT); }
    [[nodiscard]] bool IsLevitating() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY); }
    [[nodiscard]] bool IsWalking() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_WALKING); }
    [[nodiscard]] bool isMoving() const   { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_MASK_MOVING); }
    [[nodiscard]] bool isTurning() const  { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_MASK_TURNING); }
    [[nodiscard]] bool IsHovering() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_HOVER); }
    [[nodiscard]] bool isSwimming() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_SWIMMING); }
    [[nodiscard]] virtual bool CanFly() const = 0;
    [[nodiscard]] bool IsFlying() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FLYING | MOVEMENTFLAG_DISABLE_GRAVITY); }
    [[nodiscard]] bool IsFalling() const;

    [[nodiscard]] float GetHoverHeight() const { return IsHovering() ? GetFloatValue(UNIT_FIELD_HOVERHEIGHT) : 0.0f; }

    [[nodiscard]] virtual bool IsMovementPreventedByCasting() const;

    [[nodiscard]] virtual bool CanEnterWater() const = 0;
    [[nodiscard]] virtual bool CanSwim() const;
    [[nodiscard]] bool CanFreeMove() const
    {
        return !HasUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING | UNIT_STATE_IN_FLIGHT |
                             UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_DISTRACTED) && !GetOwnerGUID();
    }

    void KnockbackFrom(float x, float y, float speedXY, float speedZ);
    void JumpTo(float speedXY, float speedZ, bool forward = true);
    void JumpTo(WorldObject* obj, float speedZ);

    void  UpdateSpeed(UnitMoveType mtype, bool forced);
    [[nodiscard]] float GetSpeed(UnitMoveType mtype) const;
    [[nodiscard]] float GetSpeedRate(UnitMoveType mtype) const { return m_speed_rate[mtype]; }
    void SetSpeed(UnitMoveType mtype, float rate, bool forced = false);
    void SetSpeedRate(UnitMoveType mtype, float rate) { m_speed_rate[mtype] = rate; }

    void propagateSpeedChange() { GetMotionMaster()->propagateSpeedChange(); }

    void SendMonsterMove(float NewPosX, float NewPosY, float NewPosZ, uint32 TransitTime, SplineFlags sf = SPLINEFLAG_WALK_MODE); // pussywizard: need to just send packet, with no movement/spline
    void MonsterMoveWithSpeed(float x, float y, float z, float speed);

    virtual bool SetWalk(bool enable);
    virtual bool SetDisableGravity(bool disable, bool packetOnly = false, bool updateAnimationTier = true);
    virtual bool SetSwim(bool enable);
    virtual bool SetCanFly(bool enable, bool packetOnly = false);
    virtual bool SetWaterWalking(bool enable, bool packetOnly = false);
    virtual bool SetFeatherFall(bool enable, bool packetOnly = false);
    virtual bool SetHover(bool enable, bool packetOnly = false, bool updateAnimationTier = true);

    MotionMaster* GetMotionMaster() { return i_motionMaster; }
    [[nodiscard]] const MotionMaster* GetMotionMaster() const { return i_motionMaster; }
    [[nodiscard]] virtual MovementGeneratorType GetDefaultMovementType() const;

    [[nodiscard]] bool IsStopped() const { return !(HasUnitState(UNIT_STATE_MOVING)); }
    void StopMoving();
    void StopMovingOnCurrentPos();   /// @brief Disable the unit movement by clearing UNIT_STATE_MOVING and stopping the spline.
    virtual void PauseMovement(uint32 timer = 0, uint8 slot = 0); // timer in ms
    void ResumeMovement(uint32 timer = 0, uint8 slot = 0);

    void SetControlled(bool apply, UnitState state, Unit* source = nullptr, bool isFear = false);
    void DisableRotate(bool apply);
    void DisableSpline();

    /*********************************************************/
    /***                  MISC METHODS                     ***/
    /*********************************************************/
    // SheathState
    [[nodiscard]] SheathState GetSheath() const { return SheathState(GetByteValue(UNIT_FIELD_BYTES_2, 0)); }
    virtual void SetSheath(SheathState sheathed) { SetByteValue(UNIT_FIELD_BYTES_2, 0, sheathed); }

    // StandState
    [[nodiscard]] uint8 getStandState() const { return GetByteValue(UNIT_FIELD_BYTES_1, 0); }
    [[nodiscard]] bool IsSitState() const;
    [[nodiscard]] bool IsStandState() const;
    void SetStandState(uint8 state);

    void  SetStandFlags(uint8 flags) { SetByteFlag(UNIT_FIELD_BYTES_1,  UNIT_BYTES_1_OFFSET_VIS_FLAG, flags); }
    void  RemoveStandFlags(uint8 flags) { RemoveByteFlag(UNIT_FIELD_BYTES_1,  UNIT_BYTES_1_OFFSET_VIS_FLAG, flags); }

    // DeathState
    DeathState getDeathState() { return m_deathState; };
    virtual void setDeathState(DeathState s, bool despawn = false);           // overwrited in Creature/Player/Pet

    [[nodiscard]] bool IsAlive() const { return (m_deathState == DeathState::ALIVE); };
    [[nodiscard]] bool isDying() const { return (m_deathState == DeathState::JUST_DIED); };
    [[nodiscard]] bool isDead() const { return (m_deathState == DeathState::DEAD || m_deathState == DeathState::CORPSE); };

    // Spell Aura helpers
    [[nodiscard]] bool HasGhostAura()               const { return HasAuraType(SPELL_AURA_GHOST); };
    [[nodiscard]] bool HasMountedAura()             const { return HasAuraType(SPELL_AURA_MOUNTED); };
    [[nodiscard]] bool HasWaterWalkAura()           const { return HasAuraType(SPELL_AURA_WATER_WALK); };
    [[nodiscard]] bool HasFeatherFallAura()         const { return HasAuraType(SPELL_AURA_FEATHER_FALL); };
    [[nodiscard]] bool HasHoverAura()               const { return HasAuraType(SPELL_AURA_HOVER); };
    [[nodiscard]] bool HasFlyAura()                 const { return HasAuraType(SPELL_AURA_FLY); };
    [[nodiscard]] bool HasSpiritOfRedemptionAura()  const { return HasAuraType(SPELL_AURA_SPIRIT_OF_REDEMPTION); };
    [[nodiscard]] bool HasPreventsFleeingAura()     const { return HasAuraType(SPELL_AURA_PREVENTS_FLEEING); };
    [[nodiscard]] bool HasPreventDurabilityLossAura()  const { return HasAuraType(SPELL_AURA_PREVENT_DURABILITY_LOSS); };
    [[nodiscard]] bool HasPreventResurectionAura()  const { return HasAuraType(SPELL_AURA_PREVENT_RESURRECTION); };
    [[nodiscard]] bool HasTransformAura()           const { return HasAuraType(SPELL_AURA_TRANSFORM); };
    [[nodiscard]] bool HasInterruptRegenAura()      const { return HasAuraType(SPELL_AURA_INTERRUPT_REGEN); };
    [[nodiscard]] bool HasNoPVPCreditAura()         const { return HasAuraType(SPELL_AURA_NO_PVP_CREDIT); };
    [[nodiscard]] bool HasWaterBreathingAura()      const { return HasAuraType(SPELL_AURA_WATER_BREATHING); };
    [[nodiscard]] bool HasIgnoreHitDirectionAura()  const { return HasAuraType(SPELL_AURA_IGNORE_HIT_DIRECTION); };
    [[nodiscard]] bool HasSpellMagnetAura()         const { return HasAuraType(SPELL_AURA_SPELL_MAGNET); };
    [[nodiscard]] bool HasOpenStableAura()          const { return HasAuraType(SPELL_AURA_OPEN_STABLE); };
    [[nodiscard]] bool HasCloneCasterAura()         const { return HasAuraType(SPELL_AURA_CLONE_CASTER); };
    [[nodiscard]] bool HasReflectSpellsAura()       const { return HasAuraType(SPELL_AURA_REFLECT_SPELLS); };
    [[nodiscard]] bool HasDetectAmoreAura()         const { return HasAuraType(SPELL_AURA_DETECT_AMORE); };
    [[nodiscard]] bool HasAllowOnlyAbilityAura()    const { return HasAuraType(SPELL_AURA_ALLOW_ONLY_ABILITY); };
    [[nodiscard]] bool HasPeriodicDummyAura()       const { return HasAuraType(SPELL_AURA_PERIODIC_DUMMY); };
    [[nodiscard]] bool HasControlVehicleAura()      const { return HasAuraType(SPELL_AURA_CONTROL_VEHICLE); };
    [[nodiscard]] bool HasAOECharmAura()            const { return HasAuraType(SPELL_AURA_AOE_CHARM); };
    [[nodiscard]] bool HasDetectSpellsAura()        const { return HasAuraType(SPELL_AURA_DEFLECT_SPELLS); };
    [[nodiscard]] bool HasPacifySilenceAura()       const { return HasAuraType(SPELL_AURA_MOD_PACIFY_SILENCE); }
    [[nodiscard]] bool HasSilenceAura()             const { return HasAuraType(SPELL_AURA_MOD_SILENCE); }
    [[nodiscard]] bool HasShapeshiftAura()          const { return HasAuraType(SPELL_AURA_MOD_SHAPESHIFT); }
    [[nodiscard]] bool HasDecreaseSpeedAura()       const { return HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED); }
    [[nodiscard]] bool HasPacifyAura()              const { return HasAuraType(SPELL_AURA_MOD_PACIFY); }
    [[nodiscard]] bool HasIgnoreTargetResistAura()  const { return HasAuraType(SPELL_AURA_MOD_IGNORE_TARGET_RESIST); }
    [[nodiscard]] bool HasIncreaseMountedSpeedAura() const { return HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED); }
    [[nodiscard]] bool HasIncreaseMountedFlightSpeedAura() const { return HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED); }
    [[nodiscard]] bool HasThreatAura()              const { return HasAuraType(SPELL_AURA_MOD_THREAT); }
    [[nodiscard]] bool HasAttackerSpellCritChanceAura() const { return HasAuraType(SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE); }
    [[nodiscard]] bool HasUnattackableAura()        const { return HasAuraType(SPELL_AURA_MOD_UNATTACKABLE); }
    [[nodiscard]] bool HasHealthRegenInCombatAura() const { return HasAuraType(SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT); }
    [[nodiscard]] bool HasRegenDuringCombatAura()   const { return HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT); }
    [[nodiscard]] bool HasFearAura()                const { return HasAuraType(SPELL_AURA_MOD_FEAR); }
    [[nodiscard]] bool HasConfuseAura()             const { return HasAuraType(SPELL_AURA_MOD_CONFUSE); }
    [[nodiscard]] bool HasRootAura()                const { return HasAuraType(SPELL_AURA_MOD_ROOT); }
    [[nodiscard]] bool HasStunAura()                const { return HasAuraType(SPELL_AURA_MOD_STUN); }
    [[nodiscard]] bool HasTauntAura()               const { return HasAuraType(SPELL_AURA_MOD_TAUNT); }
    [[nodiscard]] bool HasStealthAura()             const { return HasAuraType(SPELL_AURA_MOD_STEALTH); }
    [[nodiscard]] bool HasStealthDetectAura()       const { return HasAuraType(SPELL_AURA_MOD_STEALTH_DETECT); }
    [[nodiscard]] bool HasInvisibilityAura()        const { return HasAuraType(SPELL_AURA_MOD_INVISIBILITY); }
    [[nodiscard]] bool HasInvisibilityDetectAura()  const { return HasAuraType(SPELL_AURA_MOD_INVISIBILITY_DETECT); }

    // React methods
    bool IsHostileTo(Unit const* unit) const;
    [[nodiscard]] bool IsHostileToPlayers() const;
    bool IsFriendlyTo(Unit const* unit) const;
    [[nodiscard]] bool IsNeutralToAll() const;

    // Reactive attacks
    void ClearAllReactives();
    void StartReactiveTimer(ReactiveType reactive) { m_reactiveTimer[reactive] = REACTIVE_TIMER_START;}
    void UpdateReactives(uint32 p_time);

    // Diminish returns system
    DiminishingLevels GetDiminishing(DiminishingGroup group);
    void IncrDiminishing(DiminishingGroup group);
    float ApplyDiminishingToDuration(DiminishingGroup group, int32& duration, Unit* caster, DiminishingLevels Level, int32 limitduration);
    void ApplyDiminishingAura(DiminishingGroup group, bool apply);
    void ClearDiminishings() { m_Diminishing.clear(); }

    // Group methods
    bool IsInPartyWith(Unit const* unit) const;
    bool IsInRaidWith(Unit const* unit) const;
    void GetPartyMembers(std::list<Unit*>& units);
    Unit* GetNextRandomRaidMemberOrPet(float radius);
    void UpdateAuraForGroup(uint8 slot);

    // Reputations system
    ReputationRank GetReactionTo(Unit const* target, bool checkOriginalFaction = false) const;
    ReputationRank GetFactionReactionTo(FactionTemplateEntry const* factionTemplateEntry, Unit const* target) const;
    static ReputationRank GetFactionReactionTo(FactionTemplateEntry const* factionTemplateEntry, FactionTemplateEntry const* targetFactionTemplateEntry);

    // Shared vision
    SharedVisionList const& GetSharedVisionList() { return m_sharedVision; }
    void AddPlayerToVision(Player* player);
    void RemovePlayerFromVision(Player* player);
    [[nodiscard]] bool HasSharedVision() const { return !m_sharedVision.empty(); }

    // Virtual items
    uint32 GetVirtualItemId(uint32 slot) const;
    void SetVirtualItem(uint32 slot, uint32 itemId);

    // Mount methods
    [[nodiscard]] bool IsMounted() const { return HasUnitFlag(UNIT_FLAG_MOUNT); }
    [[nodiscard]] uint32 GetMountID() const { return GetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID); }
    void Mount(uint32 mount, uint32 vehicleId = 0, uint32 creatureEntry = 0);
    void Dismount();
    [[nodiscard]] bool IsInDisallowedMountForm() const;

    // Followers
    void addFollower(FollowerReference* pRef) { m_FollowingRefMgr.insertFirst(pRef); }
    void removeFollower(FollowerReference* /*pRef*/) { /* nothing to do yet */ }
    [[nodiscard]] virtual float GetFollowAngle() const { return static_cast<float>(M_PI / 2); }

    // Pets, guardians, minions...
    [[nodiscard]] Guardian* GetGuardianPet() const;
    [[nodiscard]] Minion* GetFirstMinion() const;
    [[nodiscard]] Creature* GetCompanionPet() const;

    Pet* CreateTamedPetFrom(Creature* creatureTarget, uint32 spell_id = 0);
    Pet* CreateTamedPetFrom(uint32 creatureEntry, uint32 spell_id = 0);
    bool InitTamedPet(Pet* pet, uint8 level, uint32 spell_id);

    void SetMinion(Minion* minion, bool apply);
    void GetAllMinionsByEntry(std::list<Creature*>& Minions, uint32 entry);
    void RemoveAllMinionsByEntry(uint32 entry);

    void AddPetAura(PetAura const* petSpell);
    void RemovePetAura(PetAura const* petSpell);
    void CastPetAura(PetAura const* aura);
    bool IsPetAura(Aura const* aura);

    void PetSpellFail(SpellInfo const* spellInfo, Unit* target, uint32 result);

    void UnsummonAllTotems(bool onDeath = false);

    // Veichles 
    [[nodiscard]] TransportBase* GetDirectTransport() const;    /// Returns the transport this unit is on directly (if on vehicle and transport, return vehicle)

    bool CreateVehicleKit(uint32 id, uint32 creatureEntry);
    void RemoveVehicleKit();
    [[nodiscard]] Vehicle* GetVehicleKit()const { return m_vehicleKit; }
    [[nodiscard]] Vehicle* GetVehicle()   const { return m_vehicle; }
    bool IsOnVehicle(Unit const* vehicle) const { return m_vehicle && m_vehicle == vehicle->GetVehicleKit(); }
    [[nodiscard]] Unit* GetVehicleBase()  const;
    [[nodiscard]] Creature* GetVehicleCreatureBase() const;

    void EnterVehicle(Unit* base, int8 seatId = -1);
    void EnterVehicleUnattackable(Unit* base, int8 seatId = -1);
    void ExitVehicle(Position const* exitPosition = nullptr);
    void ChangeSeat(int8 seatId, bool next = true);

    // Should only be called by AuraEffect::HandleAuraControlVehicle(AuraApplication const* auraApp, uint8 mode, bool apply) const;
    bool HandleSpellClick(Unit* clicker, int8 seatId = -1);
    void _ExitVehicle(Position const* exitPosition = nullptr);
    void _EnterVehicle(Vehicle* vehicle, int8 seatId, AuraApplication const* aurApp = nullptr);

    // Emote
    void HandleEmoteCommand(uint32 emoteId);

    // Chat
    virtual void Talk(std::string_view text, ChatMsg msgType, Language language, float textRange, WorldObject const* target);
    virtual void Say(std::string_view text, Language language, WorldObject const* target = nullptr);
    virtual void Yell(std::string_view text, Language language, WorldObject const* target = nullptr);
    virtual void TextEmote(std::string_view text, WorldObject const* target = nullptr, bool isBossEmote = false);
    virtual void Whisper(std::string_view text, Language language, Player* target, bool isBossWhisper = false);
    virtual void Talk(uint32 textId, ChatMsg msgType, float textRange, WorldObject const* target);
    virtual void Say(uint32 textId, WorldObject const* target = nullptr);
    virtual void Yell(uint32 textId, WorldObject const* target = nullptr);
    virtual void TextEmote(uint32 textId, WorldObject const* target = nullptr, bool isBossEmote = false);
    virtual void Whisper(uint32 textId, Player* target, bool isBossWhisper = false);

    // ShapeShitForm (use by druid)
    [[nodiscard]] ShapeshiftForm GetShapeshiftForm() const { return ShapeshiftForm(GetByteValue(UNIT_FIELD_BYTES_2, 3)); }
    void SetShapeshiftForm(ShapeshiftForm form);
    bool IsAttackSpeedOverridenShapeShift() const;
    [[nodiscard]] bool IsInFeralForm() const
    {
        ShapeshiftForm form = GetShapeshiftForm();
        return form == FORM_CAT || form == FORM_BEAR || form == FORM_DIREBEAR || form == FORM_GHOSTWOLF; // Xinef: added shamans Ghost Wolf, should behave exactly like druid forms
    }

    // Unit transform
    void setTransForm(uint32 spellid) { m_transform = spellid;}
    [[nodiscard]] uint32 getTransForm() const { return m_transform;}
    void DeMorph();

    // Unit models
    virtual float GetNativeObjectScale() const { return 1.0f; }
    virtual void RecalculateObjectScale();
    [[nodiscard]] uint32 GetDisplayId() const { return GetUInt32Value(UNIT_FIELD_DISPLAYID); }
    virtual void SetDisplayId(uint32 modelId, float displayScale = 1.f);
    [[nodiscard]] uint32 GetNativeDisplayId() const { return GetUInt32Value(UNIT_FIELD_NATIVEDISPLAYID); }
    void RestoreDisplayId();
    void SetNativeDisplayId(uint32 displayId) { SetUInt32Value(UNIT_FIELD_NATIVEDISPLAYID, displayId); }

    [[nodiscard]] uint32 GetModelForForm(ShapeshiftForm form, uint32 spellId) const;
    uint32 GetModelForTotem(PlayerTotemType totemType);

    // Unit positons
    [[nodiscard]] virtual bool IsInWater() const;
    [[nodiscard]] virtual bool IsUnderWater() const;

    void NearTeleportTo(Position& pos, bool casting = false, bool vehicleTeleport = false, bool withPet = false, bool removeTransport = false);
    void NearTeleportTo(float x, float y, float z, float orientation, bool casting = false, bool vehicleTeleport = false, bool withPet = false, bool removeTransport = false);

    void SetInFront(WorldObject const* target);
    void SetFacingTo(float ori);
    void SetFacingToObject(WorldObject* object);

    bool isInAccessiblePlaceFor(Creature const* c) const;
    bool isInFrontInMap(Unit const* target, float distance, float arc = M_PI) const;
    bool isInBackInMap(Unit const* target, float distance, float arc = M_PI) const;

    [[nodiscard]] float GetCollisionHeight() const override;
    [[nodiscard]] float GetCollisionWidth() const override;
    [[nodiscard]] float GetCollisionRadius() const override;

    void UpdateOrientation(float orientation);

    void UpdateHeight(float newZ);

    virtual bool UpdatePosition(float x, float y, float z, float ang, bool teleport = false);
    bool UpdatePosition(const Position& pos, bool teleport = false) { return UpdatePosition(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), teleport); }

    void ProcessPositionDataChanged(PositionFullTerrainStatus const& data) override;
    virtual void ProcessTerrainStatusUpdate();

    // Visibility & Phase system
    [[nodiscard]] bool IsVisible() const { return m_serverSideVisibility.GetValue(SERVERSIDE_VISIBILITY_GM) <= SEC_PLAYER; }
    void SetVisible(bool x);
    void SetModelVisible(bool on);
    [[nodiscard]] uint32 GetPhaseByAuras() const;
    void SetPhaseMask(uint32 newPhaseMask, bool update) override;// overwrite WorldObject::SetPhaseMask
    void UpdateObjectVisibility(bool forced = true, bool fromUpdate = false) override;

    // Pointers
    void AddPointedBy(SafeUnitPointer* sup) { SafeUnitPointerSet.insert(sup); }
    void RemovePointedBy(SafeUnitPointer* sup) { SafeUnitPointerSet.erase(sup); }
    static void HandleSafeUnitPointersOnDelete(Unit* thisUnit);

    // Senders methods
    void SendAttackStateUpdate(CalcDamageInfo* damageInfo);
    void SendAttackStateUpdate(uint32 HitInfo, Unit* target, uint8 SwingType, SpellSchoolMask damageSchoolMask, uint32 Damage, uint32 AbsorbDamage, uint32 Resist, VictimState TargetState, uint32 BlockedAmount);

    void SendComboPoints();

    void SendPlaySpellVisual(uint32 id);
    void SendPlaySpellVisual(ObjectGuid guid, uint32 id);
    void SendPlaySpellImpact(ObjectGuid guid, uint32 id);

    void SendPetActionFeedback (uint8 msg);
    void SendPetTalk (uint32 pettalk);
    void SendPetAIReaction(ObjectGuid guid);

    void SendPeriodicAuraLog(SpellPeriodicAuraLogInfo* pInfo);

    void SendSpellNonMeleeDamageLog(SpellNonMeleeDamage* log);
    void SendSpellNonMeleeReflectLog(SpellNonMeleeDamage* log, Unit* attacker);
    void SendSpellNonMeleeDamageLog(Unit* target, SpellInfo const* spellInfo, uint32 Damage, SpellSchoolMask damageSchoolMask, uint32 AbsorbedDamage, uint32 Resist, bool PhysicalDamage, uint32 Blocked, bool CriticalHit = false, bool Split = false);
    void SendSpellMiss(Unit* target, uint32 spellID, SpellMissInfo missInfo);
    void SendSpellDamageResist(Unit* target, uint32 spellId);
    void SendSpellDamageImmune(Unit* target, uint32 spellId);

    void SendTameFailure(uint8 result);

    void SendTeleportPacket(Position& pos);

    void SendMovementFlagUpdate(bool self = false);
    void SendMovementWaterWalking(Player* sendTo);
    void SendMovementFeatherFall(Player* sendTo);
    void SendMovementHover(Player* sendTo);

    void SendChangeCurrentVictimOpcode(HostileReference* pHostileReference);
    void SendClearThreatListOpcode();
    void SendRemoveFromThreatListOpcode(HostileReference* pHostileReference);
    void SendThreatListUpdate();
    void SendClearTarget();

    // Misc functions
    void ExecuteDelayedUnitRelocationEvent();
    void ExecuteDelayedUnitAINotifyEvent();

    void BuildHeartBeatMsg(WorldPacket* data) const;
    void BuildMovementPacket(ByteBuffer* data) const;

    // Debug
    void OutDebugInfo() const;
    void SetCannotReachTargetUnit(bool target, bool isChase);
    [[nodiscard]] bool CanNotReachTarget() const;

    bool m_cannotReachTarget;

    //----------- Public variables ----------//
    uint32 m_extraAttacks;
    bool m_canDualWield;

    ControlSet m_Controlled;

    SafeUnitPointer m_movedByPlayer;

    ObjectGuid m_SummonSlot[MAX_SUMMON_SLOT];
    ObjectGuid m_ObjectSlot[MAX_GAMEOBJECT_SLOT];

    float m_modMeleeHitChance;
    float m_modRangedHitChance;
    float m_modSpellHitChance;
    int32 m_baseSpellCritChance;

    float m_threatModifier[MAX_SPELL_SCHOOL];
    float m_modAttackSpeedPct[3];

    SpellImmuneList m_spellImmune[MAX_SPELL_IMMUNITY];
    uint32 m_lastSanctuaryTime;

    // pet auras
    typedef std::set<PetAura const*> PetAuraSet;
    PetAuraSet m_petAuras;

    bool IsAIEnabled;
    bool NeedChangeAI;

    bool m_ControlledByPlayer;
    bool m_CreatedByPlayer;

    // Safe mover
    std::set<SafeUnitPointer*> SafeUnitPointerSet;

    // Relocation Nofier optimization
    Position m_last_notify_position;
    uint32 m_last_notify_mstime;
    uint16 m_delayed_unit_relocation_timer;
    uint16 m_delayed_unit_ai_notify_timer;
    bool bRequestForcedVisibilityUpdate;

    // Movement info
    Movement::MoveSpline* movespline;

  protected:
    void SetAI(UnitAI* newAI);
    UnitAI* GetTopAI() const { return i_AIs.empty() ? nullptr : i_AIs.top().get(); }
    void RefreshAI();
    UnitAI* GetScheduledChangeAI();
    bool HasScheduledAIChange() const;

  public:
    void AddToWorld() override;
    void RemoveFromWorld() override;

    void CleanupBeforeRemoveFromMap(bool finalCleanup);
    void CleanupsBeforeDelete(bool finalCleanup = true) override; // used in ~Creature/~Player (or before mass creature delete to remove cross-references to already deleted units)

    virtual bool IsAffectedByDiminishingReturns() const { return (GetCharmerOrOwnerPlayerOrPlayerItself() != nullptr); }
    DiminishingLevels GetDiminishing(DiminishingGroup group) const;
    void IncrDiminishing(SpellInfo const* auraSpellInfo, bool triggered);
    bool ApplyDiminishingToDuration(SpellInfo const* auraSpellInfo, bool triggered, int32& duration, Unit* caster, DiminishingLevels previousLevel) const;
    void ApplyDiminishingAura(DiminishingGroup group, bool apply);
    void ClearDiminishings();

    virtual void Update(uint32 time) override;
   
    void SetCannotReachTargetUnit(bool target, bool isChase);
    [[nodiscard]] bool CanNotReachTarget() const;
    bool m_cannotReachTarget;
    UnitAI* i_AI;
    UnitAI* i_disabledAI;
    
    uint32 m_extraAttacks;
    bool m_canDualWield;

    ControlSet m_Controlled;

    SafeUnitPointer m_movedByPlayer;

    ObjectGuid m_SummonSlot[MAX_SUMMON_SLOT];
    ObjectGuid m_ObjectSlot[MAX_GAMEOBJECT_SLOT];

    float m_modMeleeHitChance;
    float m_modRangedHitChance;
    float m_modSpellHitChance;
    int32 m_baseSpellCritChance;

    float m_threatModifier[MAX_SPELL_SCHOOL];
    float m_modAttackSpeedPct[3];

    SpellImmuneList m_spellImmune[MAX_SPELL_IMMUNITY];
    uint32 m_lastSanctuaryTime;

    // pet auras
    typedef std::set<PetAura const*> PetAuraSet;
    PetAuraSet m_petAuras;

    bool IsAIEnabled;
    bool NeedChangeAI;

    bool m_ControlledByPlayer;
    bool m_CreatedByPlayer;

    void _addAttacker(Unit* pAttacker);    // must be called only from Unit::Attack(Unit*)
    void _removeAttacker(Unit* pAttacker); // must be called only from Unit::AttackStop()

    // Use this only when 100% sure there is a victim
    Unit* EnsureVictim() const
    {
        ASSERT(m_attacking);
        return m_attacking;
    }

    SheathState GetSheath() const { return SheathState(GetByteValue(UNIT_FIELD_BYTES_2, UNIT_BYTES_2_OFFSET_SHEATH_STATE)); }
    virtual void SetSheath(SheathState sheathed);

    ReputationRank GetReactionTo(Unit const* target) const;
    ReputationRank static GetFactionReactionTo(FactionTemplateEntry const* factionTemplateEntry, Unit const* target);

    bool IsHostileTo(Unit const* unit) const;
    bool IsHostileToPlayers() const;
    bool IsFriendlyTo(Unit const* unit) const;
    bool IsNeutralToAll() const;
    bool IsInPartyWith(Unit const* unit) const;
    bool IsInRaidWith(Unit const* unit) const;
    void GetPartyMembers(std::list<Unit*>& units);

    uint8 GetStandState() const { return GetByteValue(UNIT_FIELD_BYTES_1, UNIT_BYTES_1_OFFSET_STAND_STATE); }
    bool IsSitState() const;
    bool IsStandState() const;
    void SetStandState(uint8 state);

    void SetAnimationTier(AnimationTier tier, bool immediate = true);
    AnimationTier GetAnimationTier() const { return static_cast<AnimationTier>(GetByteValue(UNIT_FIELD_BYTES_1, UNIT_BYTES_1_OFFSET_ANIM_TIER)); }

    void SetStandFlags(uint8 flags) { SetByteFlag(UNIT_FIELD_BYTES_1, UNIT_BYTES_1_OFFSET_VIS_FLAG, flags); }
    void RemoveStandFlags(uint8 flags) { RemoveByteFlag(UNIT_FIELD_BYTES_1, UNIT_BYTES_1_OFFSET_VIS_FLAG, flags); }

    // DeathState
    DeathState getDeathState() { return m_deathState; };
    virtual void setDeathState(DeathState s, bool despawn = false);           // overwrited in Creature/Player/Pet
    [[nodiscard]] bool IsAlive() const { return (m_deathState == DeathState::ALIVE); };
    [[nodiscard]] bool isDying() const { return (m_deathState == DeathState::JUST_DIED); };
    [[nodiscard]] bool isDead() const { return (m_deathState == DeathState::DEAD || m_deathState == DeathState::CORPSE); };

    bool IsMounted() const { return HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_MOUNT); }
    uint32 GetMountID() const { return GetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID); }
    void Mount(uint32 mount, uint32 vehicleId = 0, uint32 creatureEntry = 0);
    void Dismount();
    MountCapabilityEntry const* GetMountCapability(uint32 mountType) const;
    void UpdateMountCapability();

    void SendDurabilityLoss(Player* receiver, uint32 percent);
    void PlayOneShotAnimKitId(uint16 animKitId);

    static void ProcSkillsAndAuras(Unit* actor, Unit* actionTarget, uint32 typeMaskActor, uint32 typeMaskActionTarget, uint32 spellTypeMask, uint32 spellPhaseMask, uint32 hitMask, Spell* spell,
        DamageInfo* damageInfo, HealInfo* healInfo);

    void HandleEmoteCommand(uint32 anim_id);

    // player or player's pet resilience (-1%)
    uint32 GetDamageReduction(uint32 damage) const { return GetCombatRatingDamageReduction(CR_RESILIENCE_PLAYER_DAMAGE_TAKEN, 1.0f, 100.0f, damage); }

    int32 CalculateAOEAvoidance(int32 damage, uint32 schoolMask, ObjectGuid const& casterGuid) const;
    
    virtual uint32 GetBlockPercent() const { return 30; }

    bool IsInFlight() const { return HasUnitState(UNIT_STATE_IN_FLIGHT); }

    // Combat handling
    CombatManager& GetCombatManager() { return m_combatManager; }
    CombatManager const& GetCombatManager() const { return m_combatManager; }
    void AtTargetAttacked(Unit* target, bool canInitialAggro);

    void SetInCombatWithZone();
    bool CanHaveThreadList();



    void SendClearTarget();

    bool virtual HasSpell(uint32 /*spellID*/) const { return false; }

    // Spell Aura helpers
    [[nodiscard]] bool HasGhostAura()               const { return HasAuraType(SPELL_AURA_GHOST); };
    [[nodiscard]] bool HasMountedAura()             const { return HasAuraType(SPELL_AURA_MOUNTED); };
    [[nodiscard]] bool HasWaterWalkAura()           const { return HasAuraType(SPELL_AURA_WATER_WALK); };
    [[nodiscard]] bool HasFeatherFallAura()         const { return HasAuraType(SPELL_AURA_FEATHER_FALL); };
    [[nodiscard]] bool HasHoverAura()               const { return HasAuraType(SPELL_AURA_HOVER); };
    [[nodiscard]] bool HasFlyAura()                 const { return HasAuraType(SPELL_AURA_FLY); };
    [[nodiscard]] bool HasSpiritOfRedemptionAura()  const { return HasAuraType(SPELL_AURA_SPIRIT_OF_REDEMPTION); };
    [[nodiscard]] bool HasPreventsFleeingAura()     const { return HasAuraType(SPELL_AURA_PREVENTS_FLEEING); };
    [[nodiscard]] bool HasPreventDurabilityLossAura()  const { return HasAuraType(SPELL_AURA_PREVENT_DURABILITY_LOSS); };
    [[nodiscard]] bool HasPreventResurectionAura()  const { return HasAuraType(SPELL_AURA_PREVENT_RESURRECTION); };
    [[nodiscard]] bool HasTransformAura()           const { return HasAuraType(SPELL_AURA_TRANSFORM); };
    [[nodiscard]] bool HasInterruptRegenAura()      const { return HasAuraType(SPELL_AURA_INTERRUPT_REGEN); };
    [[nodiscard]] bool HasNoPVPCreditAura()         const { return HasAuraType(SPELL_AURA_NO_PVP_CREDIT); };
    [[nodiscard]] bool HasWaterBreathingAura()      const { return HasAuraType(SPELL_AURA_WATER_BREATHING); };
    [[nodiscard]] bool HasIgnoreHitDirectionAura()  const { return HasAuraType(SPELL_AURA_IGNORE_HIT_DIRECTION); };
    [[nodiscard]] bool HasSpellMagnetAura()         const { return HasAuraType(SPELL_AURA_SPELL_MAGNET); };
    [[nodiscard]] bool HasOpenStableAura()          const { return HasAuraType(SPELL_AURA_OPEN_STABLE); };
    [[nodiscard]] bool HasCloneCasterAura()         const { return HasAuraType(SPELL_AURA_CLONE_CASTER); };
    [[nodiscard]] bool HasReflectSpellsAura()       const { return HasAuraType(SPELL_AURA_REFLECT_SPELLS); };
    [[nodiscard]] bool HasDetectAmoreAura()         const { return HasAuraType(SPELL_AURA_DETECT_AMORE); };
    [[nodiscard]] bool HasAllowOnlyAbilityAura()    const { return HasAuraType(SPELL_AURA_ALLOW_ONLY_ABILITY); };
    [[nodiscard]] bool HasPeriodicDummyAura()       const { return HasAuraType(SPELL_AURA_PERIODIC_DUMMY); };
    [[nodiscard]] bool HasControlVehicleAura()      const { return HasAuraType(SPELL_AURA_CONTROL_VEHICLE); };
    [[nodiscard]] bool HasAOECharmAura()            const { return HasAuraType(SPELL_AURA_AOE_CHARM); };
    [[nodiscard]] bool HasDetectSpellsAura()        const { return HasAuraType(SPELL_AURA_DEFLECT_SPELLS); };
    [[nodiscard]] bool HasPacifySilenceAura()       const { return HasAuraType(SPELL_AURA_MOD_PACIFY_SILENCE); }
    [[nodiscard]] bool HasSilenceAura()             const { return HasAuraType(SPELL_AURA_MOD_SILENCE); }
    [[nodiscard]] bool HasShapeshiftAura()          const { return HasAuraType(SPELL_AURA_MOD_SHAPESHIFT); }
    [[nodiscard]] bool HasDecreaseSpeedAura()       const { return HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED); }
    [[nodiscard]] bool HasPacifyAura()              const { return HasAuraType(SPELL_AURA_MOD_PACIFY); }
    [[nodiscard]] bool HasIgnoreTargetResistAura()  const { return HasAuraType(SPELL_AURA_MOD_IGNORE_TARGET_RESIST); }
    [[nodiscard]] bool HasIncreaseMountedSpeedAura() const { return HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED); }
    [[nodiscard]] bool HasIncreaseMountedFlightSpeedAura() const { return HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED); }
    [[nodiscard]] bool HasThreatAura()              const { return HasAuraType(SPELL_AURA_MOD_THREAT); }
    [[nodiscard]] bool HasAttackerSpellCritChanceAura() const { return HasAuraType(SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE); }
    [[nodiscard]] bool HasUnattackableAura()        const { return HasAuraType(SPELL_AURA_MOD_UNATTACKABLE); }
    [[nodiscard]] bool HasHealthRegenInCombatAura() const { return HasAuraType(SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT); }
    [[nodiscard]] bool HasRegenDuringCombatAura()   const { return HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT); }
    [[nodiscard]] bool HasFearAura()                const { return HasAuraType(SPELL_AURA_MOD_FEAR); }
    [[nodiscard]] bool HasConfuseAura()             const { return HasAuraType(SPELL_AURA_MOD_CONFUSE); }
    [[nodiscard]] bool HasRootAura()                const { return HasAuraType(SPELL_AURA_MOD_ROOT); }
    [[nodiscard]] bool HasStunAura()                const { return HasAuraType(SPELL_AURA_MOD_STUN); }
    [[nodiscard]] bool HasTauntAura()               const { return HasAuraType(SPELL_AURA_MOD_TAUNT); }
    [[nodiscard]] bool HasStealthAura()             const { return HasAuraType(SPELL_AURA_MOD_STEALTH); }
    [[nodiscard]] bool HasStealthDetectAura()       const { return HasAuraType(SPELL_AURA_MOD_STEALTH_DETECT); }
    [[nodiscard]] bool HasInvisibilityAura()        const { return HasAuraType(SPELL_AURA_MOD_INVISIBILITY); }
    [[nodiscard]] bool HasInvisibilityDetectAura()  const { return HasAuraType(SPELL_AURA_MOD_INVISIBILITY_DETECT); }


    bool IsPolymorphed() const;

    bool isFrozen() const;

    bool IsInWater() const;
    bool IsUnderWater() const;
    bool isInAccessiblePlaceFor(Creature const* c) const;

    void SendEnergizeSpellLog(Unit* victim, uint32 spellId, int32 damage, Powers powerType);
    void EnergizeBySpell(Unit* victim, uint32 spellId, int32 damage, Powers powerType);
    void EnergizeBySpell(Unit* victim, SpellInfo const* spellInfo, int32 damage, Powers powerType);

    // CastSpell's third arg can be a variety of things - check out CastSpellExtraArgs' constructors!
    void CastSpell(SpellCastTargets const& targets, uint32 spellId, CastSpellExtraArgs const& args = {});
    void CastSpell(WorldObject* target, uint32 spellId, CastSpellExtraArgs const& args = {});
    void CastSpell(Position const& dest, uint32 spellId, CastSpellExtraArgs const& args = {});


    void SendPlaySpellVisualKit(uint32 id, uint32 type, uint32 duration) const;
    void SendPlaySpellVisual(ObjectGuid guid, uint32 id);
    void SendPlaySpellVisual(uint32 spellVisualId, Unit const* target = nullptr, Optional<Position> targetPosition = {}, float travelSpeed = 0.f, uint16 missReason = 0, uint16 reflectStatus = 0,
        bool speedAsTime = false) const;
    void CancelSpellMissiles(uint32 spellId, bool reverseMissile = false);

    void DeMorph();

    void SendAttackStateUpdate(CalcDamageInfo* damageInfo);
    void SendAttackStateUpdate(
        uint32 HitInfo, Unit* target, uint8 SwingType, SpellSchoolMask damageSchoolMask, uint32 Damage, uint32 AbsorbDamage, uint32 Resist, VictimState TargetState, uint32 BlockedAmount);
    void SendSpellNonMeleeDamageLog(SpellNonMeleeDamage* log);
    void SendSpellNonMeleeDamageLog(
        Unit* target, uint32 SpellID, uint32 Damage, SpellSchoolMask damageSchoolMask, uint32 AbsorbedDamage, uint32 Resist, bool PhysicalDamage, uint32 Blocked, bool CriticalHit = false);
    void SendPeriodicAuraLog(SpellPeriodicAuraLogInfo* pInfo);
    void SendSpellMiss(Unit* target, uint32 spellID, SpellMissInfo missInfo);
    void SendSpellDamageResist(Unit* target, uint32 spellId);
    void SendSpellDamageImmune(Unit* target, uint32 spellId);

    void NearTeleportTo(Position const& pos, bool casting = false);
    void NearTeleportTo(float x, float y, float z, float orientation, bool casting = false) { NearTeleportTo(Position(x, y, z, orientation), casting); }
    void SendTeleportPacket(Position const& pos);
    virtual bool UpdatePosition(float x, float y, float z, float ang, bool teleport = false);
    // returns true if unit's position really changed
    virtual bool UpdatePosition(const Position& pos, bool teleport = false);
    void UpdateOrientation(float orientation);
    void UpdateHeight(float newZ);

    void SendMoveKnockBack(Player* player, float speedXY, float speedZ, float vcos, float vsin);
    void KnockbackFrom(float x, float y, float speedXY, float speedZ);
    void JumpTo(float speedXY, float speedZ, bool forward = true, Optional<Position> dest = {});
    void JumpTo(WorldObject* obj, float speedZ, bool withOrientation = false);

    void MonsterMoveWithSpeed(float x, float y, float z, float speed, bool generatePath = false, bool forceDestination = false);

    void SendSetPlayHoverAnim(bool enable);
    void SendMovementSetSplineAnim(AnimationTier anim);

    bool IsGravityDisabled() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY); }
    bool IsWalking() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_WALKING); }
    bool IsHovering() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_HOVER); }
    bool SetWalk(bool enable);
    virtual bool SetDisableGravity(bool disable, bool packetOnly = false, bool updateAnimationTier = true);
    bool SetFall(bool enable);
    bool SetSwim(bool enable);
    bool SetFlying(bool enable);
    virtual bool SetCanFly(bool enable, bool packetOnly = false);
    virtual bool SetCanTransitionBetweenSwimAndFly(bool enable);
    bool SetWaterWalking(bool enable, bool packetOnly = false);
    bool SetFeatherFall(bool enable, bool packetOnly = false);
    virtual bool SetHover(bool enable, bool packetOnly = false, bool updateAnimationTier = true);
    void SendSetVehicleRecId(uint32 vehicleId);

    // Sets the internal orientation value to face the provided target. Does NOT have clientside effects.
    void SetOrientationTowards(WorldObject const* target);
    // Launches a spline to face the given orientation.
    void SetFacingTo(float const ori, bool force = true);
    // Launches a spline to face the given object
    void SetFacingToObject(WorldObject const* object, bool force = true);

    bool IsAlive() const { return (m_deathState == ALIVE); }
    bool isDying() const { return (m_deathState == JUST_DIED); }
    bool isDead() const { return (m_deathState == DEAD || m_deathState == CORPSE); }
    DeathState getDeathState() const { return m_deathState; }
    virtual void setDeathState(DeathState s); // overwrited in Creature/Player/Pet



    void SetMinion(Minion* minion, bool apply);
    void GetAllMinionsByEntry(std::list<Creature*>& Minions, uint32 entry);
    void RemoveAllMinionsByEntry(uint32 entry);
    Unit* GetNextRandomRaidMemberOrPet(float radius);

    // real time client control status of this unit (possess effects, vehicles and similar). For example, if this unit is a player temporarly under fear, it will return false.
    bool IsMovedByClient() const { return _gameClientMovingMe != nullptr; }
    bool IsMovedByServer() const { return !IsMovedByClient(); }
    GameClient* GetGameClientMovingMe() const { return _gameClientMovingMe; }
    void SetGameClientMovingMe(GameClient* gameClientMovingMe);

    SharedVisionList const& GetSharedVisionList() { return m_sharedVision; }
    void AddPlayerToVision(Player* player);
    void RemovePlayerFromVision(Player* player);
    bool HasSharedVision() const { return !m_sharedVision.empty(); }
    void RemoveCharmAuras();

    Pet* CreateTamedPetFrom(Creature* creatureTarget, uint32 spell_id = 0);
    Pet* CreateTamedPetFrom(uint32 creatureEntry, uint32 spell_id = 0);
    bool InitTamedPet(Pet* pet, uint8 level, uint32 spell_id);

    int32 GetTotalAuraModifier(AuraType auraType) const;
    float GetTotalAuraMultiplier(AuraType auraType) const;
    int32 GetMaxPositiveAuraModifier(AuraType auraType) const;
    int32 GetMaxNegativeAuraModifier(AuraType auraType) const;

    int32 GetTotalAuraModifier(AuraType auraType, std::function<bool(AuraEffect const*)> const& predicate) const;
    float GetTotalAuraMultiplier(AuraType auraType, std::function<bool(AuraEffect const*)> const& predicate) const;
    int32 GetMaxPositiveAuraModifier(AuraType auraType, std::function<bool(AuraEffect const*)> const& predicate) const;
    int32 GetMaxNegativeAuraModifier(AuraType auraType, std::function<bool(AuraEffect const*)> const& predicate) const;

    int32 GetTotalAuraModifierByMiscMask(AuraType auraType, uint32 misc_mask) const;
    float GetTotalAuraMultiplierByMiscMask(AuraType auraType, uint32 misc_mask) const;
    int32 GetMaxPositiveAuraModifierByMiscMask(AuraType auraType, uint32 misc_mask, AuraEffect const* except = nullptr) const;
    int32 GetMaxNegativeAuraModifierByMiscMask(AuraType auraType, uint32 misc_mask) const;

    int32 GetTotalAuraModifierByMiscValue(AuraType auraType, int32 misc_value) const;
    float GetTotalAuraMultiplierByMiscValue(AuraType auraType, int32 misc_value) const;
    int32 GetMaxPositiveAuraModifierByMiscValue(AuraType auraType, int32 misc_value) const;
    int32 GetMaxNegativeAuraModifierByMiscValue(AuraType auraType, int32 misc_value) const;

    int32 GetTotalAuraModifierByAffectMask(AuraType auraType, SpellInfo const* affectedSpell) const;
    float GetTotalAuraMultiplierByAffectMask(AuraType auraType, SpellInfo const* affectedSpell) const;
    int32 GetMaxPositiveAuraModifierByAffectMask(AuraType auraType, SpellInfo const* affectedSpell) const;
    int32 GetMaxNegativeAuraModifierByAffectMask(AuraType auraType, SpellInfo const* affectedSpell) const;

    void UpdateResistanceBuffModsMod(SpellSchools school);
    void UpdateStatBuffMod(Stats stat);

    ObjectGuid GetChannelObjectGuid() const { return GetGuidValue(UNIT_FIELD_CHANNEL_OBJECT); }
    void SetChannelObjectGuid(ObjectGuid guid) { SetGuidValue(UNIT_FIELD_CHANNEL_OBJECT, guid); }

    void SetCurrentCastSpell(Spell* pSpell);



    virtual bool HasSpellFocus(Spell const* /*focusSpell*/ = nullptr) const { return false; }

    SpellHistory* GetSpellHistory() { return m_spellHistory; }
    SpellHistory const* GetSpellHistory() const { return m_spellHistory; }

    ObjectGuid m_SummonSlot[MAX_SUMMON_SLOT];
    ObjectGuid m_ObjectSlot[MAX_GAMEOBJECT_SLOT];

    void AddSummonedCreature(ObjectGuid guid, uint32 entry);
    void RemoveSummonedCreature(ObjectGuid guid);
    Creature* GetSummonedCreatureByEntry(uint32 entry);
    void UnsummonCreatureByEntry(uint32 entry, uint32 ms = 0);

    ShapeshiftForm GetShapeshiftForm() const { return ShapeshiftForm(GetByteValue(UNIT_FIELD_BYTES_2, UNIT_BYTES_2_OFFSET_SHAPESHIFT_FORM)); }
    void SetShapeshiftForm(ShapeshiftForm form);

    bool IsInFeralForm() const;

    bool IsInDisallowedMountForm() const;

    float m_modMeleeHitChance;
    float m_modRangedHitChance;
    float m_modSpellHitChance;
    int32 m_baseSpellCritChance;

    float m_modAttackSpeedPct[3];

    // Event handler
    EventProcessor m_Events;

    // stat system
    // stat system
    void HandleStatFlatModifier(UnitMods unitMod, UnitModifierType modifierType, float amount, bool apply);
    void ApplyStatPctModifier(UnitMods unitMod, UnitModifierType modifierType, float amount);

    void SetStatFlatModifier(UnitMods unitMod, UnitModifierType modifierType, float val);
    void SetStatPctModifier(UnitMods unitMod, UnitModifierType modifierType, float val);

    float GetFlatModifierValue(UnitMods unitMod, UnitModifierType modifierType) const;
    float GetPctModifierValue(UnitMods unitMod, UnitModifierType modifierType) const;

    void UpdateUnitMod(UnitMods unitMod);

    // only players have item requirements
    virtual bool CheckAttackFitToAuraRequirement(WeaponAttackType /*attackType*/, AuraEffect const* /*aurEff*/) const { return true; }

    virtual void UpdateDamageDoneMods(WeaponAttackType attackType);
    void UpdateAllDamageDoneMods();

    void UpdateDamagePctDoneMods(WeaponAttackType attackType);
    void UpdateAllDamagePctDoneMods();

    bool isInFrontInMap(Unit const* target, float distance, float arc = float(M_PI)) const;
    bool isInBackInMap(Unit const* target, float distance, float arc = float(M_PI)) const;

    // Visibility system
    bool IsVisible() const;
    void SetVisible(bool x);

    // common function for visibility checks for player/creatures with detection code
    void OnPhaseChange();
    void UpdateObjectVisibility(bool forced = true) override;

    SpellImmuneContainer m_spellImmune[MAX_SPELL_IMMUNITY];
    uint32 m_lastSanctuaryTime;

    virtual float GetNativeObjectScale() const { return 1.0f; }
    virtual void RecalculateObjectScale();
    uint32 GetDisplayId() const { return GetUInt32Value(UNIT_FIELD_DISPLAYID); }
    virtual void SetDisplayId(uint32 modelId);
    uint32 GetNativeDisplayId() const { return GetUInt32Value(UNIT_FIELD_NATIVEDISPLAYID); }
    void RestoreDisplayId();
    void SetNativeDisplayId(uint32 modelId) { SetUInt32Value(UNIT_FIELD_NATIVEDISPLAYID, modelId); }
    void setTransForm(uint32 spellid) { m_transform = spellid; }
    uint32 getTransForm() const { return m_transform; }

    // DynamicObject management
    void _RegisterDynObject(DynamicObject* dynObj);
    void _UnregisterDynObject(DynamicObject* dynObj);
    DynamicObject* GetDynObject(uint32 spellId) const;
    std::vector<DynamicObject*> GetDynObjects(uint32 spellId) const;
    void RemoveDynObject(uint32 spellId);
    void RemoveAllDynObjects();

    GameObject* GetGameObject(uint32 spellId) const;
    std::vector<GameObject*> GetGameObjects(uint32 spellId) const;
    void AddGameObject(GameObject* gameObj);
    void RemoveGameObject(GameObject* gameObj, bool del);
    void RemoveGameObject(uint32 spellid, bool del);
    void RemoveAllGameObjects();

    void UnsummonAllTotems();
    bool IsMagnet() const;

    bool isBlockCritical();
    float SpellCritChanceDone(SpellInfo const* spellInfo, SpellSchoolMask schoolMask, WeaponAttackType attackType = BASE_ATTACK, bool isPeriodic = false) const;
    float SpellCritChanceTaken(Unit const* caster, SpellInfo const* spellInfo, SpellSchoolMask schoolMask, float doneChance, WeaponAttackType attackType = BASE_ATTACK, bool isPeriodic = false) const;
    static uint32 SpellCriticalDamageBonus(Unit const* caster, SpellInfo const* spellProto, uint32 damage);

    uint32 GetCastingTimeForBonus(SpellInfo const* spellProto, DamageEffectType damagetype, uint32 CastingTime) const;

    void ApplySpellImmune(uint32 spellId, uint32 op, uint32 type, bool apply);
    virtual bool IsImmunedToSpell(SpellInfo const* spellInfo, Unit* caster, Optional<uint8> effectMask = {}) const; // redefined in Creature
    uint32 GetSchoolImmunityMask() const;
    uint32 GetDamageImmunityMask() const;
    uint32 GetMechanicImmunityMask() const;

    bool IsImmunedToDamage(SpellSchoolMask meleeSchoolMask) const;
    bool IsImmunedToDamage(SpellInfo const* spellInfo) const;
    virtual bool IsImmunedToSpellEffect(SpellInfo const* spellInfo, uint32 index, Unit* caster) const; // redefined in Creature

    uint32 CalcSpellResistedDamage(Unit const* attacker, Unit* victim, uint32 damage, SpellSchoolMask schoolMask, SpellInfo const* spellInfo);

    void UpdateSpeed(UnitMoveType mtype);
    float GetSpeed(UnitMoveType mtype) const;
    float GetSpeedRate(UnitMoveType mtype) const { return m_speed_rate[mtype]; }
    void SetSpeed(UnitMoveType mtype, float newValue);
    void SetSpeedRate(UnitMoveType mtype, float rate);

  private:
    void SetSpeedRateReal(UnitMoveType mtype, float rate);

  public:
    float ApplyEffectModifiers(SpellInfo const* spellProto, uint8 effect_index, float value) const;
    int32 ModSpellDuration(SpellInfo const* spellProto, Unit const* target, int32 duration, bool positive, uint32 effectMask);
    void ModSpellCastTime(SpellInfo const* spellProto, int32& castTime, Spell* spell = nullptr);
    void ModSpellDurationTime(SpellInfo const* spellProto, int32& castTime, Spell* spell = nullptr);

    // Makes the unit follow the given target. Use this function above using the MotionMaster::MoveFollow for default follow behaivior.
    void FollowTarget(Unit* target);
     // Followers
    void addFollower(FollowerReference* pRef) { m_FollowingRefMgr.insertFirst(pRef); }
    void removeFollower(FollowerReference* /*pRef*/) { /* nothing to do yet */ }
    [[nodiscard]] virtual float GetFollowAngle() const { return DEFAULT_FOLLOW_ANGLE; }

    FormationFollowerGUIDContainer GetFormationFollowers() { return _formationFollowers; }
    void AddFormationFollower(Unit* follower) { _formationFollowers.push_back(follower->GetGUID()); }
    void RemoveFormationFollower(Unit* follower);
    bool HasFormationFollower(Unit* follower) const;

    void PursuerAdded(PursuingType type, AbstractPursuer* pursuer) { _unitsPursuingMe[AsUnderlyingType(type)].insert(pursuer); };
    void PursuerRemoved(PursuingType type, AbstractPursuer* pursuer) { _unitsPursuingMe[AsUnderlyingType(type)].erase(pursuer); };

    // Sets the target of all stored AbstractPursuers to nullptr, allowing the involved movement generators to do safe nullptr checks
    void RemoveAllPursuers();

    MotionMaster* GetMotionMaster() { return i_motionMaster; }
    MotionMaster const* GetMotionMaster() const { return i_motionMaster; }

    bool IsStopped() const { return !(HasUnitState(UNIT_STATE_MOVING)); }
    void StopMoving();
    void PauseMovement(uint32 timer = 0, uint8 slot = 0, bool forced = true); // timer in ms
    void ResumeMovement(uint32 timer = 0, uint8 slot = 0);                    // timer in ms

    void SetControlled(bool apply, UnitState state);
    void ApplyControlStatesIfNeeded();

    ///----------Pet responses methods-----------------
    void SendPetActionFeedback(uint8 msg);
    void SendPetTalk(uint32 pettalk);
    void SendPetAIReaction(ObjectGuid guid);
    ///----------End of Pet responses methods----------

    void propagateSpeedChange();

    // reactive attacks
    void ClearAllReactives();
    void StartReactiveTimer(ReactiveType reactive) { m_reactiveTimer[reactive] = REACTIVE_TIMER_START; }
    void UpdateReactives(uint32 p_time);

    // group updates
    void UpdateAuraForGroup(uint8 slot);

    uint32 GetModelForForm(ShapeshiftForm form, uint32 spellId) const;
    uint32 GetModelForTotem(PlayerTotemType totemType);

    friend class VehicleJoinEvent;
    ObjectGuid LastCharmerGUID;
    bool CreateVehicleKit(uint32 id, uint32 creatureEntry, bool loading = false);
    void RemoveVehicleKit(bool onRemoveFromWorld = false);
    Vehicle* GetVehicleKit() const { return m_vehicleKit; }
    Vehicle* GetVehicle() const { return m_vehicle; }
    void SetVehicle(Vehicle* vehicle) { m_vehicle = vehicle; }
    bool IsOnVehicle(Unit const* vehicle) const;
    Unit* GetVehicleBase() const;
    Creature* GetVehicleCreatureBase() const;
    /// Returns the transport this unit is on directly (if on vehicle and transport, return vehicle)
    TransportBase* GetDirectTransport() const;

    bool m_ControlledByPlayer;

    bool HandleSpellClick(Unit* clicker, int8 seatId = -1);
    void EnterVehicle(Unit* base, int8 seatId = -1);
    void ExitVehicle(Position const* exitPosition = nullptr);
    void ChangeSeat(int8 seatId, bool next = true);

    // Should only be called by AuraEffect::HandleAuraControlVehicle(AuraApplication const* auraApp, uint8 mode, bool apply) const;
    void _ExitVehicle(Position const* exitPosition = nullptr);
    void _EnterVehicle(Vehicle* vehicle, int8 seatId, AuraApplication const* aurApp = nullptr);

    void WriteMovementInfo(WorldPacket& data, Movement::ExtraMovementStatusElement* extras = nullptr, uint32* movementCounter = nullptr);

    bool isMoving() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_MASK_MOVING); }
    bool isTurning() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_MASK_TURNING); }
    virtual bool CanFly() const = 0;
    bool IsFlying() const { return m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FLYING | MOVEMENTFLAG_DISABLE_GRAVITY); }
    bool IsFalling() const;
    virtual bool CanEnterWater() const = 0;
    virtual bool CanSwim() const;

    float GetHoverOffset() const { return HasUnitMovementFlag(MOVEMENTFLAG_HOVER) ? GetFloatValue(UNIT_FIELD_HOVERHEIGHT) : 0.0f; }

    uint32 GetMovementCounterAndInc() { return m_movementCounter++; }
    uint32 GetMovementCounter() { return m_movementCounter; }
    void ClearPendingMovementChangeForType(MovementChangeType changeType);
    void AssignPendingMovementChange(MovementChangeType changeType, PlayerMovementPendingChange&& newChange);
    bool HasPendingMovementChange() const { return !m_pendingMovementChanges.empty(); }
    PlayerMovementPendingChange const* GetPendingMovementChange(MovementChangeType changeType) const;
    void PurgeAndApplyPendingMovementChanges(bool informObservers = true);

    void OutDebugInfo() const;
    virtual bool IsLoading() const { return false; }
    bool IsDuringRemoveFromWorld() const { return m_duringRemoveFromWorld; }

    // Movement info
    Movement::MoveSpline* movespline;

    int32 GetHighestExclusiveSameEffectSpellGroupValue(AuraEffect const* aurEff, AuraType auraType, bool checkMiscValue = false, int32 miscValue = 0) const;
    bool IsHighestExclusiveAura(Aura const* aura, bool removeOtherAuraApplications = false);
    bool IsHighestExclusiveAuraEffect(SpellInfo const* spellInfo, AuraType auraType, int32 effectAmount, uint8 auraEffectMask, bool removeOtherAuraApplications = false);

    virtual void Talk(std::string const& text, ChatMsg msgType, Language language, float textRange, WorldObject const* target);
    virtual void Say(std::string const& text, Language language, WorldObject const* target = nullptr);
    virtual void Yell(std::string const& text, Language language, WorldObject const* target = nullptr);
    virtual void TextEmote(std::string const& text, WorldObject const* target = nullptr, bool isBossEmote = false);
    virtual void Whisper(std::string const& text, Language language, Player* target, bool isBossWhisper = false);
    virtual void Talk(uint32 textId, ChatMsg msgType, float textRange, WorldObject const* target);
    virtual void Say(uint32 textId, WorldObject const* target = nullptr);
    virtual void Yell(uint32 textId, WorldObject const* target = nullptr);
    virtual void TextEmote(uint32 textId, WorldObject const* target = nullptr, bool isBossEmote = false);
    virtual void Whisper(uint32 textId, Player* target, bool isBossWhisper = false);

    void SendMapObjectEvents(int32 mapobjectid, std::vector<uint8> values);

    TaskScheduler& GetScheduler() { return _scheduler; }

    float GetCollisionHeight() const override;

    // returns if the unit is allowed to enter combat
    bool IsIgnoringCombat() const { return _isIgnoringCombat; }
    // enables/disables combat permission of this unit
    void SetIgnoringCombat(bool apply) { _isIgnoringCombat = apply; }

    // Queues up a spell cast request that has been received via packet and processes it whenever possible.
    void RequestSpellCast(PendingSpellCastRequest castRequest, SpellInfo const* spellInfo);
    void CancelPendingCastRequest();
    bool CanRequestSpellCast(SpellInfo const* spell) const;

    void DestroyForPlayer(Player* target, bool onDeath = false) const override;

    void GetAttackableUnitListInRange(std::list<Unit*>& list, float fMaxSearchRange) const;
    void GetFriendlyUnitListInRange(std::list<Unit*>& list, float fMaxSearchRange, bool exceptSelf = false) const;
    void GetAreatriggerListInRange(std::list<AreaTrigger*>& list, float fMaxSearchRange) const;
    void GetAreaTriggerListWithSpellIDInRange(std::list<AreaTrigger*>& list, uint32 spellid, float fMaxSearchRange) const;

  protected:
    explicit Unit(bool isWorldObject);

    void BuildValuesUpdate(uint8 updatetype, ByteBuffer* data, Player* target) const override;

    void _UpdateSpells(uint32 time);
    void _DeleteRemovedAuras();

    void _UpdateAutoRepeatSpell();

    bool m_AutoRepeatFirstCast;

    uint32 m_attackTimer[MAX_ATTACK];

    float m_createStats[MAX_STATS];

    AttackerSet m_attackers;
    Unit* m_attacking;

    DeathState m_deathState;

    int32 m_procDeep;

    typedef std::list<DynamicObject*> DynObjectList;
    DynObjectList m_dynObj;

    typedef std::list<GameObject*> GameObjectList;
    GameObjectList m_gameObj;

    uint32 m_transform;

    Spell* m_currentSpells[CURRENT_MAX_SPELL];

    AuraMap m_ownedAuras;
    AuraApplicationMap m_appliedAuras;
    AuraList m_removedAuras;
    AuraMap::iterator m_auraUpdateIterator;
    uint32 m_removedAurasCount;

    AuraEffectList m_modAuras[TOTAL_AURAS];
    AuraList m_scAuras;                        // casted singlecast auras
    AurasBySpellIdMap m_ltAuras;              // cast limited target auras
    AuraApplicationList m_interruptableAuras; // auras which have interrupt mask applied on unit
    AuraStateAurasMap m_auraStateAuras;       // Used for improve performance of aura state checks on aura apply/remove
    EnumFlag<SpellAuraInterruptFlags> m_interruptMask;
    EnumFlag<SpellAuraInterruptFlags2> m_interruptMask2;

    float m_auraModifiersGroup[UNIT_MOD_END][MODIFIER_TYPE_END];
    float m_weaponDamage[MAX_ATTACK][MAX_WEAPON_DAMAGE_RANGE][MAX_ITEM_PROTO_DAMAGES];
    bool m_canModifyStats;
    VisibleAuraMap m_visibleAuras;

    float m_speed_rate[MAX_MOVE_TYPE];

    Unit* m_charmer; // Unit that is charming ME
    Unit* m_charmed; // Unit that is being charmed BY ME

    CharmInfo* m_charmInfo;

    int32 m_regenTimer;

    ThreatMgr m_ThreatMgr;
    typedef std::map<ObjectGuid, float> CharmThreatMap;
    CharmThreatMap _charmThreatInfo;

    CharmInfo* m_charmInfo;
    SharedVisionList m_sharedVision;
    GameClient* _gameClientMovingMe;

    MotionMaster* i_motionMaster;

    uint32 m_reactiveTimer[MAX_REACTIVE];

    Vehicle* m_vehicle;
    Vehicle* m_vehicleKit;

    uint32 m_unitTypeMask;
    LiquidTypeEntry const* _lastLiquid;

    uint32 _powerBarId;
    float _powerFraction[MAX_POWERS_PER_CLASS];
    int32 _powerUpdateTimer;
    int32 _healthRegenerationTimer;
    bool m_applyResilience;
    bool _instantCast;

    uint32 m_rootTimes;

    bool IsAlwaysVisibleFor(WorldObject const* seer) const override;
    bool IsAlwaysDetectableFor(WorldObject const* seer) const override;

    void DisableSpline();

    void ProcessPositionDataChanged(PositionFullTerrainStatus const& data) override;
    virtual void ProcessTerrainStatusUpdate(ZLiquidStatus oldLiquidStatus, Optional<LiquidData> const& newLiquidData);

    // notifiers
    virtual void AtEnterCombat();
    virtual void AtExitCombat();

    virtual void AtEngage(Unit* /*target*/) {}
    virtual void AtDisengage() {}

    void InterruptMovementBasedAuras();
    void CheckPendingMovementAcks();

    uint32 GetPowerUpdateInterval() const { return IsPlayer() ? PLAYER_POWER_UPDATE_INTERVAL : UNIT_POWER_UPDATE_INTERVAL; }

  private:
    void UpdateSplineMovement(uint32 t_diff);
    void UpdateSplinePosition();

    // player or player's pet
    float GetCombatRatingReduction(CombatRating cr) const;
    uint32 GetCombatRatingDamageReduction(CombatRating cr, float rate, float cap, uint32 damage) const;

    void ProcSkillsAndReactives(bool isVictim, Unit* procTarget, uint32 typeMask, uint32 hitMask, WeaponAttackType attType);

  protected:
    void SetFeared(bool apply);
    void SetConfused(bool apply);
    void SetStunned(bool apply);
    void SetRooted(bool apply, bool packetOnly = false);

  private:
    uint32 m_state;                                     // Even derived shouldn't modify
    uint32 m_CombatTimer;
    uint32 m_lastManaUse;  
    TimeTrackerSmall m_splineSyncTimer;

    DiminishingReturn m_Diminishing[DIMINISHING_MAX];

    Diminishing m_Diminishing;
    // Manage all Units that are threatened by us
    HostileRefMgr m_HostileRefMgr;

    FollowerRefMgr m_FollowingRefMgr;

    Unit* m_comboTarget;
    int8 m_comboPoints;
    std::unordered_set<Unit*> m_ComboPointHolders;

    RedirectThreatInfo _redirectThreatInfo;

    uint32 _oldFactionId;           ///< faction before charm
    bool _isWalkingBeforeCharm;     ///< Are we walking before we were charmed?

    uint32 _lastExtraAttackSpell;
    std::unordered_map<ObjectGuid /*guid*/, uint32 /*count*/> extraAttacksTargets;
    ObjectGuid _lastDamagedTargetGuid;

    // Threat+combat management
    bool m_isEngaged;
    // Manage all Units that are threatened by us
    friend class CombatManager;
    CombatManager m_combatManager;

    void UpdateCharmAI();
    void RestoreDisabledAI();
    typedef std::stack<std::shared_ptr<UnitAI>> UnitAIStack;
    UnitAIStack i_AIs;
    std::shared_ptr<UnitAI> i_AI;
    bool m_aiLocked;


    bool m_cleanupDone;           // lock made to not add stuff after cleanup before delete
    bool m_duringRemoveFromWorld; // lock made to not add stuff after begining removing from world

    SpellHistory* m_spellHistory;

    TaskScheduler _scheduler;

    std::unordered_map<ObjectGuid, uint32 /*entry*/> m_SummonedCreatures;

    PositionUpdateInfo _positionUpdateInfo;

    FormationFollowerGUIDContainer _formationFollowers;

    std::array<std::unordered_set<AbstractPursuer*>, AsUnderlyingType(PursuingType::Max)> _unitsPursuingMe;

    bool _isIgnoringCombat;

    Optional<PendingSpellCastRequest> _pendingSpellCastRequest;
    void ProcessPendingSpellCastRequest();
    void ProcessItemCast(PendingSpellCastRequest const& castRequest, SpellCastTargets const& targets);
    bool CanExecutePendingSpellCastRequest(SpellInfo const* spellInfo) const;

    /* Player Movement fields START*/

    // when a player controls this unit, and when change is made to this unit which requires an ack from the client to be acted (change of speed for example), this movementCounter is incremented
    uint32 m_movementCounter;
    std::unordered_map<MovementChangeType, PlayerMovementPendingChange> m_pendingMovementChanges;

    /* Player Movement fields END*/
};

namespace Firelands
{
    // Binary predicate for sorting Units based on percent value of a power
    class PowerPctOrderPred
    {
      public:
        PowerPctOrderPred(Powers power, bool ascending = true) : _power(power), _ascending(ascending) {}

        bool operator()(WorldObject const* objA, WorldObject const* objB) const
        {
            Unit const* a = objA->ToUnit();
            Unit const* b = objB->ToUnit();
            float rA = (a && a->GetMaxPower(_power)) ? float(a->GetPower(_power)) / float(a->GetMaxPower(_power)) : 0.0f;
            float rB = (b && b->GetMaxPower(_power)) ? float(b->GetPower(_power)) / float(b->GetMaxPower(_power)) : 0.0f;
            return _ascending ? rA < rB : rA > rB;
        }

        bool operator()(Unit const* a, Unit const* b) const
        {
            float rA = a->GetMaxPower(_power) ? float(a->GetPower(_power)) / float(a->GetMaxPower(_power)) : 0.0f;
            float rB = b->GetMaxPower(_power) ? float(b->GetPower(_power)) / float(b->GetMaxPower(_power)) : 0.0f;
            return _ascending ? rA < rB : rA > rB;
        }

      private:
        Powers const _power;
        bool const _ascending;
    };

    // Binary predicate for sorting Units based on percent value of health
    class HealthPctOrderPred
    {
      public:
        HealthPctOrderPred(bool ascending = true) : _ascending(ascending) {}

        bool operator()(WorldObject const* objA, WorldObject const* objB) const
        {
            Unit const* a = objA->ToUnit();
            Unit const* b = objB->ToUnit();
            float rA = (a && a->GetMaxHealth()) ? float(a->GetHealth()) / float(a->GetMaxHealth()) : 0.0f;
            float rB = (b && b->GetMaxHealth()) ? float(b->GetHealth()) / float(b->GetMaxHealth()) : 0.0f;
            return _ascending ? rA < rB : rA > rB;
        }

        bool operator()(Unit const* a, Unit const* b) const
        {
            float rA = a->GetMaxHealth() ? float(a->GetHealth()) / float(a->GetMaxHealth()) : 0.0f;
            float rB = b->GetMaxHealth() ? float(b->GetHealth()) / float(b->GetMaxHealth()) : 0.0f;
            return _ascending ? rA < rB : rA > rB;
        }

      private:
        bool const _ascending;
    };
} // namespace Firelands

#endif
