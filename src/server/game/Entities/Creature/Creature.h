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

#ifndef _FIRELANDS_CREATURE_H
#define _FIRELANDS_CREATURE_H

#include "Unit.h"
#include "Common.h"
#include "CreatureData.h"
#include "DatabaseEnvFwd.h"
#include "Duration.h"
#include "Loot.h"
#include "MapObject.h"

#include "Cell.h"
#include "CharmInfo.h"
#include "LootMgr.h"

#include <list>

class CreatureAI;
class CreatureGroup;
class Group;
class Quest;
class Player;
class SpellInfo;
class WorldSession;

enum MovementGeneratorType : uint8;

struct VendorItemCount
{
    VendorItemCount(uint32 _item, uint32 _count);

    uint32 itemId;
    uint32 count;
    time_t lastIncrementTime;
};

typedef std::list<VendorItemCount> VendorItemCounts;

// max different by z coordinate for creature aggro reaction
#define CREATURE_Z_ATTACK_RANGE 3

#define MAX_VENDOR_ITEMS 150 // Limitation in 4.x.x item count in SMSG_VENDOR_INVENTORY
static constexpr uint8 VENDOR_INVENTORY_REASON_INVENTORY_EMPTY = 1;

// used for handling non-repeatable random texts
typedef std::vector<uint8> CreatureTextRepeatIds;
typedef std::unordered_map<uint8, CreatureTextRepeatIds> CreatureTextRepeatGroup;

class FC_GAME_API Creature : public Unit, public GridObject<Creature>, public MovableMapObject
{
  public:
    explicit Creature(bool isWorldObject = false);
    ~Creature() override;

    void AddToWorld() override;
    void RemoveFromWorld() override;

    float GetNativeObjectScale() const override;
    void SetObjectScale(float scale) override;
    void SetDisplayId(uint32 modelId) override;
    void SetDisplayFromModel(uint32 modelIdx);

    void DisappearAndDie();

    [[nodiscard]] bool isVendorWithIconSpeak() const;

    bool Create(ObjectGuid::LowType guidlow, Map* map, uint32 entry, Position const& pos, CreatureData const* data = nullptr, uint32 vehId = 0, bool dynamic = false);
    bool LoadCreaturesAddon(bool reload = false);
    void SelectLevel(bool changelevel = true);
    void UpdateLevelDependantStats();
    void LoadEquipment(int8 id = 1, bool force = false);
    void SetSpawnHealth();
    void Reload(bool skipDatabase);

    [[nodiscard]] ObjectGuid::LowType GetSpawnId() const { return m_spawnId; }

    void Update(uint32 time) override; // overwrited Unit::Update
    void GetRespawnPosition(float& x, float& y, float& z, float* ori = nullptr, float* dist = nullptr) const;

    bool IsSpawnedOnTransport() const { return m_creatureData && m_creatureData->mapId != GetMapId(); }

    void SetCorpseDelay(uint32 delay) { m_corpseDelay = delay; }
    void SetCorpseRemoveTime(uint32 delay);
    uint32 GetCorpseDelay() const { return m_corpseDelay; }
    [[nodiscard]] bool HasFlagsExtra(uint32 flag) const { return GetCreatureTemplate()->HasFlagsExtra(flag); }
    bool IsRacialLeader() const { return GetCreatureTemplate()->RacialLeader; }
    bool IsCivilian() const { return (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_CIVILIAN) != 0; }
    bool IsTrigger() const { return (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_TRIGGER) != 0; }
    bool IsGuard() const { return (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_GUARD) != 0; }

    CreatureMovementData const& GetMovementTemplate() const;
    bool CanWalk() const { return GetMovementTemplate().IsGroundAllowed(); }
    bool CanSwim() const override;
    bool CanEnterWater() const override;
    bool CanFly() const override { return GetMovementTemplate().IsFlightAllowed() || IsFlying(); }
    bool CanHover() const { return GetMovementTemplate().IsHoverEnabled() || IsHovering(); }
    [[nodiscard]] bool IsRooted() const { return GetMovementTemplate().IsRooted(); }
    bool SetDisableGravity(bool disable, bool packetOnly = false, bool updateAnimationTier = true) override;
    bool SetHover(bool enable, bool packetOnly = false, bool updateAnimationTier = true) override;
    bool SetCanFly(bool enable, bool packetOnly = false) override;
    bool HasSpell(uint32 spellID) const override;
    bool SetWalk(bool enable) override;
    bool SetSwim(bool enable) override;
    bool SetWaterWalking(bool enable, bool packetOnly = false) override;
    bool SetFeatherFall(bool enable, bool packetOnly = false) override;
    bool SetHover(bool enable, bool packetOnly = false, bool updateAnimationTier = true) override;

    bool IsDungeonBoss() const { return (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_DUNGEON_BOSS) != 0; }
    bool IsAffectedByDiminishingReturns() const override { return Unit::IsAffectedByDiminishingReturns() || (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_ALL_DIMINISH) != 0; }

    MovementGeneratorType GetDefaultMovementType() const override { return m_defaultMovementType; }
    void SetDefaultMovementType(MovementGeneratorType mgt) { m_defaultMovementType = mgt; }

    Unit* SelectVictim();

    // For spells with Cone targets
    void PrepareChanneledCast(float facing, uint32 spellId = 0, bool triggered = false);
    void RemoveChanneledCast(ObjectGuid target);

    void SetReactState(ReactStates st) { m_reactState = st; }
    ReactStates GetReactState() const { return m_reactState; }
    bool HasReactState(ReactStates state) const { return (m_reactState == state); }
    void InitializeReactState();

    using Unit::IsImmuneToAll;
    using Unit::SetImmuneToAll;
    void SetImmuneToAll(bool apply) override { Unit::SetImmuneToAll(apply, HasReactState(REACT_PASSIVE)); }
    using Unit::IsImmuneToPC;
    using Unit::SetImmuneToPC;
    void SetImmuneToPC(bool apply) override { Unit::SetImmuneToPC(apply, HasReactState(REACT_PASSIVE)); }
    using Unit::IsImmuneToNPC;
    using Unit::SetImmuneToNPC;
    void SetImmuneToNPC(bool apply) override { Unit::SetImmuneToNPC(apply, HasReactState(REACT_PASSIVE)); }

    /// @todo Rename these properly
    bool isCanInteractWithBattleMaster(Player* player, bool msg) const;
    bool CanResetTalents(Player* player) const;
    bool IsClassTrainerOf(Player const* player) const;
    bool CanCreatureAttack(Unit const* victim, bool force = true) const;
    void LoadTemplateImmunities();
    bool IsImmunedToSpell(SpellInfo const* spellInfo, Unit* caster, Optional<uint8> effectMask = {}) const override;
    bool IsImmunedToSpellEffect(SpellInfo const* spellInfo, uint32 index, Unit* caster) const override;
    [[nodiscard]] bool HasMechanicTemplateImmunity(uint32 mask) const;

    [[nodiscard]] bool isElite() const
    {
        if (IsPet())
            return false;

        uint32 rank = GetCreatureTemplate()->rank;
        return rank != CREATURE_ELITE_NORMAL && rank != CREATURE_ELITE_RARE;
    }

    [[nodiscard]] bool isWorldBoss() const
    {
        if (IsPet())
            return false;

        return GetCreatureTemplate()->type_flags & CREATURE_TYPE_FLAG_BOSS_MOB;
    }

    uint8 getLevelForTarget(WorldObject const* target) const override; // overwrite Unit::getLevelForTarget for boss level support

    [[nodiscard]] bool IsImmuneToKnockback() const;
    [[nodiscard]] bool IsAvoidingAOE() const { return HasFlagsExtra(CREATURE_FLAG_EXTRA_AVOID_AOE); }

    uint8 getLevelForTarget(WorldObject const* target) const override; // overwrite Unit::getLevelForTarget for boss level support

    [[nodiscard]] bool IsInEvadeMode() const { return HasUnitState(UNIT_STATE_EVADE); }
    [[nodiscard]] bool IsEvadingAttacks() const { return IsInEvadeMode() || CanNotReachTarget(); }

    bool AIM_Initialize(CreatureAI* ai = nullptr);
    void Motion_Initialize();

    bool AIM_Destroy();
    bool AIM_Create(CreatureAI* ai = nullptr);
    bool AIM_Initialize(CreatureAI* ai = nullptr);
    void Motion_Initialize();

    CreatureAI* AI() const { return reinterpret_cast<CreatureAI*>(GetAI()); }

    struct
    {
        ::Spell const* Spell = nullptr;
        uint32 Delay = 0;         // ms until the creature's target should snap back (0 = no snapback scheduled)
        ObjectGuid Target;        // the creature's "real" target while casting
        float Orientation = 0.0f; // the creature's "real" orientation while casting
    } _spellFocusInfo;

    SpellSchoolMask GetMeleeDamageSchoolMask(WeaponAttackType /*attackType*/ = BASE_ATTACK) const override { return m_meleeDamageSchoolMask; }

    void SetMeleeDamageSchool(SpellSchools school) { m_meleeDamageSchoolMask = SpellSchoolMask(1 << school); }

    void _AddCreatureSpellCooldown(uint32 spell_id, uint16 categoryId, uint32 end_time);
    void AddSpellCooldown(uint32 spell_id, uint32 /*itemid*/, uint32 end_time, bool needSendToClient = false, bool forceSendToSpectator = false) override;
    [[nodiscard]] bool HasSpellCooldown(uint32 spell_id) const override;
    [[nodiscard]] uint32 GetSpellCooldown(uint32 spell_id) const;
    void ProhibitSpellSchool(SpellSchoolMask idSchoolMask, uint32 unTimeMs) override;
    [[nodiscard]] bool IsSpellProhibited(SpellSchoolMask idSchoolMask) const;
    void ClearProhibitedSpellTimers();

    void UpdateMovementFlags(bool initializeDBStates);
    uint32 GetRandomId(uint32 id1, uint32 id2, uint32 id3);
    bool UpdateEntry(uint32 entry, const CreatureData* data = nullptr, bool changelevel = true, bool updateAI = false);
    bool UpdateEntry(uint32 entry, bool updateAI) { return UpdateEntry(entry, nullptr, true, updateAI); }
    bool UpdateStats(Stats stat) override;
    bool UpdateAllStats() override;
    void UpdateResistances(uint32 school) override;
    void UpdateArmor() override;
    void UpdateMaxHealth() override;
    void UpdateMaxPower(Powers power) override;
    void UpdateAttackPowerAndDamage(bool ranged = false) override;
    void CalculateMinMaxDamage(WeaponAttackType attType, bool normalized, bool addTotalPct, float& minDamage, float& maxDamage, uint8 damageIndex) override;
    uint32 GetPowerIndex(Powers power) const override;

    void LoadSparringPct();
    [[nodiscard]] float GetSparringPct() const { return _sparringPct; }

    bool HasWeapon(WeaponAttackType type) const override;
    bool HasWeaponForAttack(WeaponAttackType type) const override { return (Unit::HasWeaponForAttack(type) && HasWeapon(type)); }
    void SetCanDualWield(bool value) override;
    int8 GetOriginalEquipmentId() const { return m_originalEquipmentId; }
    uint8 GetCurrentEquipmentId() const { return m_equipmentId; }
    void SetCurrentEquipmentId(uint8 id) { m_equipmentId = id; }

    float GetSpellDamageMod(int32 Rank) const;

    VendorItemData const* GetVendorItems() const;
    uint32 GetVendorItemCurrentCount(VendorItem const* vItem);
    uint32 UpdateVendorItemCurrentCount(VendorItem const* vItem, uint32 used_count);

    CreatureTemplate const* GetCreatureTemplate() const { return m_creatureInfo; }
    CreatureData const* GetCreatureData() const { return m_creatureData; }
    void SetDetectionDistance(float dist) { m_detectionDistance = dist; }
    CreatureAddon const* GetCreatureAddon() const;

    std::string const& GetAIName() const;
    std::string GetScriptName() const;
    uint32 GetScriptId() const;

    // override WorldObject function for proper name localization
    std::string const& GetNameForLocaleIdx(LocaleConstant locale_idx) const override;

    void setDeathState(DeathState s, bool despawn = false) override; // override virtual Unit::setDeathState

    bool LoadFromDB(ObjectGuid::LowType guid, Map* map, bool allowDuplicate = false) { return LoadCreatureFromDB(guid, map, false, allowDuplicate); }
    bool LoadCreatureFromDB(ObjectGuid::LowType guid, Map* map, bool addToMap = true, bool allowDuplicate = false);
    void SaveToDB();
    // overriden in Pet
    virtual void SaveToDB(uint32 mapid, uint8 spawnMask, uint32 phaseMask);
    static bool DeleteFromDB(ObjectGuid::LowType spawnId);

    Loot loot;
    void StartPickPocketRefillTimer();
    void ResetPickPocketRefillTimer() { lootPickPocketRestoreTime = 0; }
    bool CanGeneratePickPocketLoot() const;
    ObjectGuid GetLootRecipientGUID() const { return m_lootRecipient; }
    Player* GetLootRecipient() const;
    [[nodiscard]] ObjectGuid::LowType GetLootRecipientGroupGUID() const { return m_lootRecipientGroup; }
    Group* GetLootRecipientGroup() const;
    bool hasLootRecipient() const { return !m_lootRecipient.IsEmpty() || m_lootRecipientGroup; }
    bool isTappedBy(Player const* player) const; // return true if the creature is tapped by the player or a member of his party.

    void SetLootRecipient(Unit* unit, bool withGroup = true);
    void AllLootRemovedFromCorpse();

    uint16 GetLootMode() const { return m_LootMode; }
    bool HasLootMode(uint16 lootMode) { return (m_LootMode & lootMode) != 0; }
    void SetLootMode(uint16 lootMode) { m_LootMode = lootMode; }
    void AddLootMode(uint16 lootMode) { m_LootMode |= lootMode; }
    void RemoveLootMode(uint16 lootMode) { m_LootMode &= ~lootMode; }
    void ResetLootMode() { m_LootMode = LOOT_MODE_DEFAULT; }

    SpellInfo const* reachWithSpellAttack(Unit* victim);
    SpellInfo const* reachWithSpellCure(Unit* victim);

    uint32 m_spells[MAX_CREATURE_SPELLS];
    CreatureSpellCooldowns m_CreatureSpellCooldowns;
    uint32 m_ProhibitSchoolTime[7];

    bool CanStartAttack(Unit const* u, bool force) const;
    float GetAttackDistance(Unit const* player) const;
    float GetAggroRange(Unit const* target) const;
    [[nodiscard]] float GetDetectionRange() const { return m_detectionDistance; }

    void SendAIReaction(AiReaction reactionType);

    Unit* SelectNearestTarget(float dist = 0, bool playerOnly = false) const;
    Unit* SelectNearestTargetInAttackDistance(float dist = 0) const;
    Unit* SelectNearestHostileUnitInAggroRange(bool useLOS = false) const;

    void DoFleeToGetAssistance();
    void CallForHelp(float fRadius, Unit* target = nullptr);
    void CallAssistance(Unit* target = nullptr);
    void SetNoCallAssistance(bool val) { m_AlreadyCallAssistance = val; }
    void SetNoSearchAssistance(bool val) { m_AlreadySearchedAssistance = val; }
    bool HasSearchedAssistance() { return m_AlreadySearchedAssistance; }
    bool CanAssistTo(Unit const* u, Unit const* enemy, bool checkfaction = true) const;
    bool _IsTargetAcceptable(const Unit* target) const;
    [[nodiscard]] bool CanIgnoreFeignDeath() const { return HasFlagsExtra(CREATURE_FLAG_EXTRA_IGNORE_FEIGN_DEATH); }

    void UpdateMoveInLineOfSightState();
    bool IsMoveInLineOfSightDisabled() { return m_moveInLineOfSightDisabled; }
    bool IsMoveInLineOfSightStrictlyDisabled() { return m_moveInLineOfSightStrictlyDisabled; }

    void RemoveCorpse(bool setSpawnTime = true, bool destroyForNearbyPlayers = true);

    void DespawnOrUnsummon(uint32 msTimeToDespawn = 0, Seconds forceRespawnTime = 0s);
    void DespawnOrUnsummon(Milliseconds time, Seconds forceRespawnTime = 0s) { DespawnOrUnsummon(uint32(time.count()), forceRespawnTime); }
    void DespawnOrUnsummon(uint32 msTimeToDespawn = 0) { DespawnOrUnsummon(Milliseconds(msTimeToDespawn), 0s); };
    void DespawnOnEvade(Seconds respawnDelay = 20s);
    void DespawnCreaturesInArea(uint32 entry, float range = 125.0f);

    time_t const& GetRespawnTime() const { return m_respawnTime; }
    time_t GetRespawnTimeEx() const;
    void SetRespawnTime(uint32 respawn);
    void Respawn(bool force = false);
    void SaveRespawnTime(uint32 forceDelay = 0);

    uint32 GetRespawnDelay() const { return m_respawnDelay; }
    void SetRespawnDelay(uint32 delay) { m_respawnDelay = delay; }

    float GetRespawnRadius() const { return m_wanderDistance; }
    void SetRespawnRadius(float dist) { m_wanderDistance = dist; }

    void DoImmediateBoundaryCheck() { m_boundaryCheckTime = 0; }
    uint32 GetCombatPulseDelay() const { return m_combatPulseDelay; }
    void SetCombatPulseDelay(uint32 delay) // (secs) interval at which the creature pulses the entire zone into combat (only works in dungeons)
    {
        m_combatPulseDelay = delay;
        if (m_combatPulseTime == 0 || m_combatPulseTime > delay)
            m_combatPulseTime = delay;
    }

    [[nodiscard]] float GetWanderDistance() const { return m_wanderDistance; }
    void SetWanderDistance(float dist) { m_wanderDistance = dist; }

    uint32 m_groupLootTimer;                 // (msecs)timer used for group loot
    ObjectGuid::LowType lootingGroupLowGUID; // used to find group which is looting corpse

    void SendZoneUnderAttackMessage(Player* attacker);
    void SetInCombatWithZone();
    void SetCombatDistance(float dist) { m_CombatDistance = dist < 5.0f ? 5.0f : dist; }

    bool hasQuest(uint32 quest_id) const override;
    bool hasInvolvedQuest(uint32 quest_id) const override;

    bool isRegeneratingHealth() { return m_regenHealth; }
    void setRegeneratingHealth(bool regenHealth) { m_regenHealth = regenHealth; }
    void SetRegeneratingPower(bool enable) { m_regenPower = enable; }
    virtual uint8 GetPetAutoSpellSize() const { return MAX_SPELL_CHARM; }
    [[nodiscard]] virtual uint32 GetPetAutoSpellOnPos(uint8 pos) const
    {
        if (pos >= MAX_SPELL_CHARM || m_charmInfo->GetCharmSpell(pos)->GetType() != ACT_ENABLED)
            return 0;
        else
            return m_charmInfo->GetCharmSpell(pos)->GetAction();
    }

    float GetPetChaseDistance() const;

    void SetCannotReachTarget(bool cannotReach)
    {
        if (cannotReach == m_cannotReachTarget)
            return;
        m_cannotReachTarget = cannotReach;
        m_cannotReachTimer = 0;
    }
    bool CanNotReachTarget() const { return m_cannotReachTarget; }
    [[nodiscard]] bool IsNotReachableAndNeedRegen() const;

    void SetPosition(float x, float y, float z, float o);
    void SetPosition(const Position& pos) { SetPosition(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation()); }

    void SetHomePosition(float x, float y, float z, float o) { m_homePosition.Relocate(x, y, z, o); }
    void SetHomePosition(const Position& pos) { m_homePosition.Relocate(pos); }
    void GetHomePosition(float& x, float& y, float& z, float& ori) const { m_homePosition.GetPosition(x, y, z, ori); }
    Position const& GetHomePosition() const { return m_homePosition; }

    void SetTransportHomePosition(float x, float y, float z, float o) { m_transportHomePosition.Relocate(x, y, z, o); }
    void SetTransportHomePosition(const Position& pos) { m_transportHomePosition.Relocate(pos); }
    void GetTransportHomePosition(float& x, float& y, float& z, float& ori) const { m_transportHomePosition.GetPosition(x, y, z, ori); }
    Position const& GetTransportHomePosition() const { return m_transportHomePosition; }

    uint32 GetWaypointPath() const { return m_waypointID; }
    void LoadPath(uint32 pathid) { m_path_id = pathid; }

    [[nodiscard]] uint32 GetCurrentWaypointID() const { return m_waypointID; }
    void UpdateWaypointID(uint32 wpID) { m_waypointID = wpID; }

    uint32 GetCyclicSplinePathId() const { return _cyclicSplinePathId; }

    // nodeId, pathId
    std::pair<uint32, uint32> GetCurrentWaypointInfo() const { return _currentWaypointNodeInfo; }
    void UpdateCurrentWaypointInfo(uint32 nodeId, uint32 pathId) { _currentWaypointNodeInfo = {nodeId, pathId}; }

    bool IsReturningHome() const;

    void SearchFormation();
    CreatureGroup* GetFormation() { return m_formation; }
    void SetFormation(CreatureGroup* formation) { m_formation = formation; }
    bool IsFormationLeader() const;
    void SignalFormationMovement();
    bool IsFormationLeaderMoveAllowed() const;

    Unit* SelectVictim();

    void SetDisableReputationGain(bool disable) { DisableReputationGain = disable; }
    bool IsReputationGainDisabled() const { return DisableReputationGain; }
    void SetLootRewardDisabled(bool disable) { DisableLootReward = disable; }
    [[nodiscard]] bool IsLootRewardDisabled() const { return DisableLootReward; }
    bool IsDamageEnoughForLootingAndReward() const;
    void LowerPlayerDamageReq(uint32 unDamage);
    void ResetPlayerDamageReq();
    [[nodiscard]] uint32 GetPlayerDamageReq() const;

    uint32 GetOriginalEntry() const { return m_originalEntry; }
    void SetOriginalEntry(uint32 entry) { m_originalEntry = entry; }

    // There's many places not ready for dynamic spawns. This allows them to live on for now.
    void SetRespawnCompatibilityMode(bool mode = true) { m_respawnCompatibilityMode = mode; }
    bool GetRespawnCompatibilityMode() { return m_respawnCompatibilityMode; }

    static float _GetDamageMod(int32 Rank);

    float m_SightDistance;
    float m_CombatDistance;

    bool m_isTempWorldObject; // true when possessed

    // Handling caster facing during spellcast
    void SetTarget(ObjectGuid guid = ObjectGuid::Empty) override;
    void ClearTarget() { SetTarget(); };
    void ReacquireSpellFocusTarget();
    void SetSpellFocus(Spell const* focusSpell, WorldObject const* target);
    bool HasSpellFocus(Spell const* focusSpell = nullptr) const override;
    void ReleaseSpellFocus(Spell const* focusSpell = nullptr, bool withDelay = true);
    void ResetSpellFocusInfo() { _spellFocusInfo.Reset(); }
    bool IsMovementPreventedByCasting() const override;

    // Part of Evade mechanics
    std::shared_ptr<time_t> const& GetLastLeashExtensionTimePtr() const;
    void SetLastLeashExtensionTimePtr(std::shared_ptr<time_t> const& timer);
    void ClearLastLeashExtensionTimePtr();
    time_t GetLastLeashExtensionTime() const;
    void UpdateLeashExtensionTime();
    time_t GetLastDamagedTime() const { return _lastDamagedTime; }
    void SetLastDamagedTime(time_t val) { _lastDamagedTime = val; }
    bool IsFreeToMove();

    static constexpr uint32 MOVE_CIRCLE_CHECK_INTERVAL = 3000;
    static constexpr uint32 MOVE_BACKWARDS_CHECK_INTERVAL = 2000;
    uint32 m_moveCircleMovementTime = MOVE_CIRCLE_CHECK_INTERVAL;
    uint32 m_moveBackwardsMovementTime = MOVE_BACKWARDS_CHECK_INTERVAL;

    CreatureTextRepeatIds GetTextRepeatGroup(uint8 textGroup);
    void SetTextRepeatId(uint8 textGroup, uint8 id);
    void ClearTextRepeatGroup(uint8 textGroup);
    bool IsEscorted() const;

    bool CanGiveExperience() const;

    void MakeInterruptable(bool apply);

    bool IsEngaged() const override;
    void AtEngage(Unit* target) override;
    void AtDisengage() override;

    bool HasSwimmingFlagOutOfCombat() const { return !_isMissingSwimmingFlagOutOfCombat; }
    void RefreshSwimmingFlag(bool recheck = false);

    CreatureMovementInfo const& GetCreatureMovementInfo() const { return _creatureMovementInfo; }

    // Sets the the max health percentage threshold at which uncontrolled/unowned creatures can no longer deal damage to the creature
    void SetNoNpcDamageBelowPctHealthValue(float value) { _noNpcDamageBelowPctHealth = std::clamp<float>(value, 0.f, 100.f); }
    void ResetNoNpcDamageBelowPctHealthValue() { _noNpcDamageBelowPctHealth = 0.f; }
    float GetNoNpcDamageBelowPctHealthValue() const { return _noNpcDamageBelowPctHealth; }

    [[nodiscard]] bool HasSwimmingFlagOutOfCombat() const { return !_isMissingSwimmingFlagOutOfCombat; }
    void RefreshSwimmingFlag(bool recheck = false);

    void SetAssistanceTimer(uint32 value) { m_assistanceTimer = value; }

    void ModifyThreatPercentTemp(Unit* victim, int32 percent, Milliseconds duration);

    /**
     * @brief Helper to resume chasing current victim.
     */
    void ResumeChasingVictim() { GetMotionMaster()->MoveChase(GetVictim()); };

    /**
     * @brief Returns true if the creature is able to cast the spell.
     */
    bool CanCastSpell(uint32 spellID) const;

    /**
     * @brief Helper to get the creature's summoner GUID, if it is a summon
     */
    [[nodiscard]] ObjectGuid GetSummonerGUID() const;

    // Used to control if MoveChase() is to be used or not in AttackStart(). Some creatures does not chase victims
    // NOTE: If you use SetCombatMovement while the creature is in combat, it will do NOTHING - This only affects AttackStart
    //       You should make the necessary to make it happen so.
    //       Remember that if you modified _isCombatMovementAllowed (e.g: using SetCombatMovement) it will not be reset at Reset().
    //       It will keep the last value you set.
    void SetCombatMovement(bool allowMovement);
    bool IsCombatMovementAllowed() const { return _isCombatMovementAllowed; }

  protected:
    bool CreateFromProto(ObjectGuid::LowType guidlow, uint32 Entry, uint32 vehId, const CreatureData* data = nullptr);
    bool InitEntry(uint32 entry, const CreatureData* data = nullptr);

    // vendor items
    VendorItemCounts m_vendorItemCounts;

    static float _GetHealthMod(int32 Rank);

    ObjectGuid m_lootRecipient;
    uint32 m_lootRecipientGroup;

    /// Timers
    time_t lootPickPocketRestoreTime;
    time_t m_corpseRemoveTime; // (msecs)timer for death or corpse disappearance
    time_t m_respawnTime;      // (secs) time of next respawn
    time_t m_respawnedTime;    // (secs) time when creature respawned
    uint32 m_respawnDelay;     // (secs) delay between corpse disappearance and respawning
    uint32 m_corpseDelay;      // (secs) delay between death and corpse disappearance
    float m_wanderDistance;
    uint32 m_boundaryCheckTime; // (msecs) remaining time for next evade boundary check
    uint32 m_combatPulseTime;   // (msecs) remaining time for next zone-in-combat pulse
    uint32 m_combatPulseDelay;  // (secs) how often the creature puts the entire zone in combat (only works in dungeons)

    ReactStates m_reactState; // for AI, not charmInfo
    void RegenerateHealth() override;
    void Regenerate(Powers power);
    MovementGeneratorType m_defaultMovementType;
    ObjectGuid::LowType m_spawnId; ///< For new or temporary creatures is 0 for saved it is lowguid
    uint8 m_equipmentId;
    int8 m_originalEquipmentId; // can be -1

    bool m_AlreadyCallAssistance;
    bool m_AlreadySearchedAssistance;
    bool m_regenHealth;
    bool m_regenPower;
    bool m_AI_locked;
    bool m_cannotReachTarget;
    uint32 m_cannotReachTimer;

    SpellSchoolMask m_meleeDamageSchoolMask;
    uint32 m_originalEntry;

    bool m_moveInLineOfSightDisabled;
    bool m_moveInLineOfSightStrictlyDisabled;

    Position m_homePosition;
    Position m_transportHomePosition;

    bool DisableReputationGain;
    bool DisableLootReward;

    CreatureTemplate const* m_creatureInfo; // Can differ from sObjectMgr->GetCreatureTemplate(GetEntry()) in difficulty mode > 0
    CreatureData const* m_creatureData;

    float m_detectionDistance;
    uint16 m_LootMode; // Bitmask (default: LOOT_MODE_DEFAULT) that determines what loot will be lootable

    float _sparringPct;

    bool IsInvisibleDueToDespawn() const override;
    bool CanAlwaysSee(WorldObject const* obj) const override;
    bool IsAlwaysDetectableFor(WorldObject const* seer) const override;

    // Initializes run and walk speed override data based on movementId override values stored in `creature_movement_info` table
    void InitializeCreatureMovementInfo(uint32 movementId);

    // Initializes move speed fields based on template and override data
    void InitializeMovementSpeeds();

  private:
    void ForcedDespawn(uint32 timeMSToDespawn = 0, Seconds forceRespawnTimer = 0s);
    bool CheckNoGrayAggroConfig(uint32 playerLevel, uint32 creatureLevel) const; // No aggro from gray creatures
    [[nodiscard]] bool CanPeriodicallyCallForAssistance() const;

    // Waypoint path
    uint32 m_waypointID;
    uint32 m_path_id;
    std::pair<uint32 /*nodeId*/, uint32 /*pathId*/> _currentWaypointNodeInfo;

    // Cyclic spline path
    uint32 _cyclicSplinePathId;

    // Formation var
    CreatureGroup* m_formation;
    bool m_triggerJustAppeared;
    bool m_respawnCompatibilityMode;

    // Shared timer between mobs who assist another.
    // Damaging one extends leash range on all of them.
    mutable std::shared_ptr<time_t> m_lastLeashExtensionTime;

    ObjectGuid m_cannotReachTarget;
    uint32 m_cannotReachTimer;

    // Spell Focusing
    CreatureSpellFocusData _spellFocusInfo;

    time_t _lastDamagedTime; // Part of Evade mechanics
    CreatureTextRepeatGroup m_textRepeat;

    bool _isMissingSwimmingFlagOutOfCombat;

    CreatureMovementInfo _creatureMovementInfo;

    float _noNpcDamageBelowPctHealth;

    // Shared timer between mobs who assist another.
    // Damaging one extends leash range on all of them.
    mutable std::shared_ptr<time_t> m_lastLeashExtensionTime;

    ObjectGuid m_cannotReachTarget;
    uint32 m_cannotReachTimer;

    Spell const* _focusSpell; ///> Locks the target during spell cast for proper facing

    bool _isMissingSwimmingFlagOutOfCombat;

    uint32 m_assistanceTimer;

    uint32 _playerDamageReq;
    bool _damagedByPlayer;
    bool _isCombatMovementAllowed;
};

class FC_GAME_API AssistDelayEvent : public BasicEvent
{
  public:
    AssistDelayEvent(ObjectGuid victim, Unit& owner) : BasicEvent(), m_victim(victim), m_owner(owner) {}

    bool Execute(uint64 e_time, uint32 p_time) override;
    void AddAssistant(ObjectGuid guid) { m_assistants.push_back(guid); }

  private:
    AssistDelayEvent();

    ObjectGuid m_victim;
    GuidList m_assistants;
    Unit& m_owner;
};

class FC_GAME_API ForcedDespawnDelayEvent : public BasicEvent
{
  public:
    ForcedDespawnDelayEvent(Creature& owner, Seconds respawnTimer) : BasicEvent(), m_owner(owner), m_respawnTimer(respawnTimer) {}
    bool Execute(uint64 e_time, uint32 p_time) override;

  private:
    Creature& m_owner;
    Seconds const m_respawnTimer;
};

#endif
