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

#ifndef _FIRELANDS_TEMPSUMMON_H
#define _FIRELANDS_TEMPSUMMON_H

#include "Creature.h"

struct SummonPropertiesEntry;

enum SummonerType
{
    SUMMONER_TYPE_CREATURE      = 0,
    SUMMONER_TYPE_GAMEOBJECT    = 1,
    SUMMONER_TYPE_MAP           = 2
};

/// Stores data for temp summons
struct TempSummonData
{
    uint32 entry;        ///< Entry of summoned creature
    Position pos;        ///< Position, where should be creature spawned
    TempSummonType type; ///< Summon type, see TempSummonType for available types
    uint32 time;         ///< Despawn time, usable only with certain temp summon types
};


enum PlayerPetSpells
{
    // Risen Ghoul
    SPELL_PET_RISEN_GHOUL_SPAWN_IN  = 47448,
    SPELL_PET_RISEN_GHOUL_SELF_STUN = 47466,

    // Hunter Pets
    SPELL_PET_ENERGIZE              = 99289
};

class FC_GAME_API TempSummon : public Creature
{
    public:
        explicit TempSummon(SummonPropertiesEntry const* properties, ObjectGuid owner, bool isWorldObject);
        virtual ~TempSummon() { }
        void Update(uint32 time) override;
        virtual void InitStats(uint32 lifetime);
        virtual void InitSummon();
        void UpdateObjectVisibilityOnCreate() override;
        virtual void UnSummon(uint32 msTime = 0);
        void RemoveFromWorld() override;
        void SetTempSummonType(TempSummonType type);
        void SaveToDB(uint32 /*mapid*/, uint8 /*spawnMask*/) override { }
        [[nodiscard]] WorldObject* GetSummoner() const;
        [[nodiscard]] Unit* GetSummonerUnit() const;
        [[nodiscard]] Creature* GetSummonerCreatureBase() const;
        [[nodiscard]] GameObject* GetSummonerGameObject() const;
        ObjectGuid GetSummonerGUID() const { return m_summonerGUID; }
        TempSummonType const& GetSummonType() { return m_type; }
        uint32 GetTimer() const { return m_timer; }
        void SetTimer(uint32 t) { m_timer = t; }

        void SetVisibleBySummonerOnly(bool visibleBySummonerOnly) { _visibleBySummonerOnly = visibleBySummonerOnly; }
        [[nodiscard]] bool IsVisibleBySummonerOnly() const { return _visibleBySummonerOnly; }

        SummonPropertiesEntry const* const m_Properties;
    private:
        TempSummonType m_type;
        uint32 m_timer;
        uint32 m_lifetime;
        ObjectGuid m_summonerGUID;
        bool _visibleBySummonerOnly;
};

class FC_GAME_API Minion : public TempSummon
{
    public:
        Minion(SummonPropertiesEntry const* properties, ObjectGuid owner, bool isWorldObject);
        void InitStats(uint32 duration) override;
        void RemoveFromWorld() override;
        [[nodiscard]] Unit* GetOwner() const;
        [[nodiscard]] float GetFollowAngle() const override { return m_followAngle; }
        void SetFollowAngle(float angle) { m_followAngle = angle; }
        bool IsPetGhoul() const { return GetEntry() == NPC_GHOUL; } // Ghoul may be guardian or pet
        bool IsRisenAlly() const { return GetEntry() == NPC_RISEN_ALLY; }
        bool IsSpiritWolf() const { return GetEntry() == NPC_SPIRIT_WOLF; } // Spirit wolf from feral spirits
        bool IsGuardianPet() const;
        bool IsWarlockMinion() const;
        void setDeathState(DeathState s, bool despawn = false) override;                   // override virtual Unit::setDeathState
    protected:
        ObjectGuid const m_owner;
        float m_followAngle;
};

class FC_GAME_API Guardian : public Minion
{
    public:
        Guardian(SummonPropertiesEntry const* properties, ObjectGuid owner, bool isWorldObject);
        void InitStats(uint32 duration) override;
        bool InitStatsForLevel(uint8 level);
        void InitSummon() override;

        bool UpdateStats(Stats stat) override;
        bool UpdateAllStats() override;
        void UpdateResistances(uint32 school) override;
        void UpdateArmor() override;
        void UpdateMaxHealth() override;
        void UpdateMaxPower(Powers power) override;
        void UpdateAttackPowerAndDamage(bool ranged = false) override;
        void UpdateDamagePhysical(WeaponAttackType attType) override;

        int32 GetBonusDamage() const { return m_bonusSpellDamage; }
        float GetBonusStatFromOwner(Stats stat) const { return m_statFromOwner[stat]; }
        void SetBonusDamage(int32 damage);
    protected:
        int32   m_bonusSpellDamage;
        float   m_statFromOwner[MAX_STATS];
};

class FC_GAME_API Puppet : public Minion
{
    public:
        Puppet(SummonPropertiesEntry const* properties, ObjectGuid owner);
        void InitStats(uint32 duration) override;
        void InitSummon() override;
        void Update(uint32 time) override;
        void RemoveFromWorld() override;
    protected:
        [[nodiscard]] Player* GetOwner() const;
        const ObjectGuid m_owner;
};

class FC_GAME_API ForcedUnsummonDelayEvent : public BasicEvent
{
public:
    ForcedUnsummonDelayEvent(TempSummon& owner) : BasicEvent(), m_owner(owner) { }
    bool Execute(uint64 e_time, uint32 p_time) override;

private:
    TempSummon& m_owner;
};
#endif
