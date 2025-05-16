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

#include "TemporarySummon.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Log.h"
#include "Object.h"
#include "ObjectAccessor.h"
#include "Pet.h"
#include "Player.h"
#include "ScriptMgr.h"


TempSummon::TempSummon(SummonPropertiesEntry const* properties, ObjectGuid owner, bool isWorldObject) :
Creature(isWorldObject), m_Properties(properties), m_type(TEMPSUMMON_MANUAL_DESPAWN),
m_timer(0), m_lifetime(0)
{
    if (owner)
    {
        m_summonerGUID = owner;
    }

    m_unitTypeMask |= UNIT_MASK_SUMMON;
}

WorldObject* TempSummon::GetSummoner() const
{
    return m_summonerGUID ? ObjectAccessor::GetWorldObject(*this, m_summonerGUID) : nullptr;
}

Unit* TempSummon::GetSummonerUnit() const
{
    if (WorldObject* summoner = GetSummoner())
    {
        return summoner->ToUnit();
    }

    return nullptr;
}

Creature* TempSummon::GetSummonerCreatureBase() const
{
    return m_summonerGUID ? ObjectAccessor::GetCreature(*this, m_summonerGUID) : nullptr;
}

void TempSummon::Update(uint32 diff)
{
    Creature::Update(diff);

    if (m_deathState == DeathState::DEAD)
    {
        UnSummon();
        return;
    }
    switch (m_type)
    {
        case TEMPSUMMON_MANUAL_DESPAWN:
        case TEMPSUMMON_DEAD_DESPAWN:
            break;
        case TEMPSUMMON_TIMED_DESPAWN:
        {
            if (m_timer <= diff)
            {
                UnSummon();
                return;
            }

            m_timer -= diff;
            break;
        }
        case TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT:
        {
            if (!IsInCombat())
            {
                if (m_timer <= diff)
                {
                    UnSummon();
                    return;
                }

                m_timer -= diff;
            }
            else if (m_timer != m_lifetime)
                m_timer = m_lifetime;

            break;
        }

        case TEMPSUMMON_CORPSE_TIMED_DESPAWN:
        {
            if (m_deathState == CORPSE)
            {
                if (m_timer <= diff)
                {
                    UnSummon();
                    return;
                }

                m_timer -= diff;
            }
            break;
        }
        case TEMPSUMMON_CORPSE_DESPAWN:
        {
            // if m_deathState is DEAD, CORPSE was skipped
            if (m_deathState == DeathState::CORPSE)
            {
                if (m_timer <= diff)
                {
                    UnSummon();
                    return;
                }

                m_timer -= diff;
            }

            break;
        }
        case TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN:
        {
            if (m_deathState == DeathState::CORPSE)
            {
                UnSummon();
                return;
            }

            if (!IsInCombat())
            {
                if (m_timer <= diff)
                {
                    UnSummon();
                    return;
                }
                else
                    m_timer -= diff;
            }
            else if (m_timer != m_lifetime)
                m_timer = m_lifetime;
            break;
        }
        case TEMPSUMMON_TIMED_OR_DEAD_DESPAWN:
        {
            if (!IsInCombat() && IsAlive())
            {
                if (m_timer <= diff)
                {
                    UnSummon();
                    return;
                }
                else
                    m_timer -= diff;
            }
            else if (m_timer != m_lifetime)
                m_timer = m_lifetime;
            break;
        }
        default:
            UnSummon();
            LOG_ERROR("entities.unit", "Temporary summoned creature (entry: %u) have unknown type %u of ", GetEntry(), m_type);
            break;
    }
}

void TempSummon::InitStats(uint32 duration)
{
    ASSERT(!IsPet());

    Unit* owner = GetSummonerUnit();
    if (owner)
        if (Player* player = owner->ToPlayer())
            sScriptMgr->OnPlayerBeforeTempSummonInitStats(player, this, duration);

    m_timer = duration;
    m_lifetime = duration;

    if (m_type == TEMPSUMMON_MANUAL_DESPAWN)
        m_type = (duration == 0) ? TEMPSUMMON_DEAD_DESPAWN : TEMPSUMMON_TIMED_DESPAWN;

    if (owner)
    {
        if (IsTrigger() && m_spells[0])
        {
            SetFaction(owner->GetFaction());
            SetLevel(owner->getLevel());
            if (owner->IsPlayer())
                m_ControlledByPlayer = true;
        }

        if (owner->IsPlayer()) { 
            m_CreatedByPlayer = true;
        }
    }

    if (!m_Properties)
        return;

    if (owner)
    {
        int32 slot = m_Properties->Slot;
        if (slot > 0)
        {
            if (owner->m_SummonSlot[slot] && owner->m_SummonSlot[slot] != GetGUID())
            {
                Creature* oldSummon = GetMap()->GetCreature(owner->m_SummonSlot[slot]);
                if (oldSummon && oldSummon->IsSummon())
                    oldSummon->ToTempSummon()->UnSummon();
            }
            owner->m_SummonSlot[slot] = GetGUID();
        }

        if (m_Properties->Control != SUMMON_CATEGORY_WILD)
        {
            if (!m_Properties->Faction)
                SetFaction(owner->GetFaction());

            // Creator guid is always set for allied summons
            SetCreatorGUID(owner->GetGUID());

            // Summons inherit their player summoner's guild data
            if (owner && (owner->IsPlayer() || owner->IsTotem()))
            {
                ObjectGuid guildGUID = owner->GetGuidValue(OBJECT_FIELD_DATA);
                if (guildGUID)
                {
                    SetGuidValue(OBJECT_FIELD_DATA, owner->GetGuidValue(OBJECT_FIELD_DATA));
                    SetUInt16Value(OBJECT_FIELD_TYPE, 1, 1); // Has guild data
                }
            }
        }

        if (owner->IsTotem())
            owner->m_Controlled.insert(this);
    }

    // If property has a faction defined, use it.
    if (m_Properties->Faction)
        SetFaction(m_Properties->Faction);
    else if (IsVehicle() && owner) // properties should be vehicle
        SetFaction(owner->GetFaction());
}

void TempSummon::InitSummon()
{
    WorldObject* owner = GetSummoner();
    if (owner)
    {
        if (owner->IsCreature())
        {
            if (owner->ToCreature()->IsAIEnabled)
            {
                owner->ToCreature()->AI()->JustSummoned(this);
            }
        }
        else if (owner->IsGameObject())
        {
            if (owner->ToGameObject()->AI())
            {
                owner->ToGameObject()->AI()->JustSummoned(this);
            }
        }

        if (IsAIEnabled) {
            AI()->IsSummonedBy(owner);
        }
    }
}

void TempSummon::UpdateObjectVisibilityOnCreate()
{
    WorldObject::UpdateObjectVisibility(true);
}

void TempSummon::SetTempSummonType(TempSummonType type)
{
    m_type = type;
}

void TempSummon::UnSummon(uint32 msTime)
{
    if (msTime)
    {
        ForcedUnsummonDelayEvent* pEvent = new ForcedUnsummonDelayEvent(*this);

        m_Events.AddEvent(pEvent, m_Events.CalculateTime(msTime));
        return;
    }

    if (m_type == TEMPSUMMON_MANUAL_DESPAWN)
        return;
    SetTempSummonType(TEMPSUMMON_MANUAL_DESPAWN);

    //ASSERT(!IsPet());
    if (IsPet())
    {
        ((Pet*)this)->Remove(PET_SAVE_DISMISS);
        ASSERT(!IsInWorld());
        return;
    }

    if (WorldObject* owner = GetSummoner())
    {
        if (owner->IsCreature() && owner->ToCreature()->IsAIEnabled) {
            owner->ToCreature()->AI()->SummonedCreatureDespawn(this);
        }
        else if (owner->IsGameObject() && owner->ToGameObject()->AI()) {
            owner->ToGameObject()->AI()->SummonedCreatureDespawn(this);
        }
    }

    AddObjectToRemoveList();
}

bool ForcedUnsummonDelayEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    m_owner.UnSummon();
    return true;
}

void TempSummon::RemoveFromWorld()
{
    if (!IsInWorld())
        return;

    if (m_Properties)
    {
        int32 slot = m_Properties->Slot;
        if (slot > 0)
            if (Unit* owner = GetSummonerUnit())
                if (owner->m_SummonSlot[slot] == GetGUID())
                    owner->m_SummonSlot[slot].Clear();
    }

    //if (GetOwnerGUID())
    //    LOG_ERROR("entities.unit", "Unit %u has owner guid when removed from world", GetEntry());

    Creature::RemoveFromWorld();
}

Minion::Minion(SummonPropertiesEntry const* properties, ObjectGuid owner, bool isWorldObject) : TempSummon(properties, owner, isWorldObject)
    , m_owner(owner)
{
    ASSERT(m_owner);
    m_unitTypeMask |= UNIT_MASK_MINION;
    m_followAngle = DEFAULT_FOLLOW_ANGLE;
}

void Minion::InitStats(uint32 duration)
{
    TempSummon::InitStats(duration);
    SetReactState(REACT_PASSIVE);

    if (Unit* owner = GetOwner())
    {
        SetCreatorGUID(owner->GetGUID());
        SetFaction(owner->GetFaction());
    }

    // Controlable guardians and minions shall receive a summoner guid
    if ((IsMinion() || IsControlableGuardian()) && !IsTotem() && !IsVehicle()) {
        GetOwner()->SetMinion(this, true);
    }
    else if (!IsPet() && !IsHunterPet())
    {
        GetOwner()->m_Controlled.insert(this);

        // Store the totem elementals in players controlled list as well to trigger aggro mechanics
        if (GetOwner()->IsTotem()) {
            if (Unit* totemOwner = GetOwner()->GetOwner()) { 
                totemOwner->m_Controlled.insert(this);
            }
        }
    }

    if (m_Properties && m_Properties->Slot == SUMMON_SLOT_MINIPET)
    {
        SelectLevel();       // some summoned creaters have different from 1 DB data for level/hp
        SetUInt32Value(UNIT_NPC_FLAGS, GetCreatureTemplate()->npcflag);
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
    }
}

void Minion::RemoveFromWorld()
{
    if (!IsInWorld())
        return;

    Unit* owner = GetOwner();

    if ((IsMinion() || IsControlableGuardian()) && !IsTotem() && !IsVehicle()) {
        owner->SetMinion(this, false);
    }
    else if (!IsPet() && !IsHunterPet())
    {
        if (owner->m_Controlled.find(this) != owner->m_Controlled.end()) {
            owner->m_Controlled.erase(this);
        }
        else {
            LOG_FATAL("entities.unit", "Minion::RemoveFromWorld: Owner %s tried to remove a non-existing controlled unit %s from controlled unit set.", owner->GetGUID().ToString().c_str(), GetGUID().ToString().c_str());
        }

        if (owner->IsTotem()) {
            if (Unit* totemOwner = owner->GetOwner()) {
                if (totemOwner->m_Controlled.find(this) != totemOwner->m_Controlled.end()) {
                    totemOwner->m_Controlled.erase(this);
                }
            }
        }
    }

    TempSummon::RemoveFromWorld();
}

bool Minion::IsGuardianPet() const
{
    return IsPet() || (m_Properties && m_Properties->Control == SUMMON_CATEGORY_PET);
}

bool Minion::IsWarlockMinion() const
{
    switch (GetEntry())
    {
        case NPC_IMP:
        case NPC_VOIDWALKER:
        case NPC_SUCCUBUS:
        case NPC_FELHUNTER:
        case NPC_FELGUARD:
            return true;
        default:
            return false;
    }
}

Unit* Minion::GetOwner() const
{
    return ObjectAccessor::GetUnit(*this, m_owner);
}

void Minion::setDeathState(DeathState s, bool despawn)
{
    Creature::setDeathState(s, despawn);

    if (s == DeathState::JUST_DIED && IsGuardianPet()) {
        if (Unit* owner = GetOwner()) {
            if (owner->IsPlayer() && owner->GetMinionGUID() == GetGUID()) {
                for (Unit::ControlSet::const_iterator itr = owner->m_Controlled.begin(); itr != owner->m_Controlled.end(); ++itr) {
                    if ((*itr)->IsAlive() && (*itr)->GetEntry() == GetEntry())
                    {
                        owner->SetMinionGUID((*itr)->GetGUID());
                        owner->SetPetGUID((*itr)->GetGUID());
                        owner->ToPlayer()->CharmSpellInitialize();
                    }
                }
            }
        }
    }
}

Guardian::Guardian(SummonPropertiesEntry const* properties, ObjectGuid owner, bool isWorldObject) : Minion(properties, owner, isWorldObject)
, m_bonusSpellDamage(0)
{
    memset(m_statFromOwner, 0, sizeof(float)*MAX_STATS);
    m_unitTypeMask |= UNIT_MASK_GUARDIAN;
    if (properties && SummonTitle(properties->Title) == SummonTitle::Pet)
    {
        m_unitTypeMask |= UNIT_MASK_CONTROLABLE_GUARDIAN;
        InitCharmInfo();
    }
}

void Guardian::InitStats(uint32 duration)
{
    Minion::InitStats(duration);

     if (Unit* m_owner = GetOwner())
    {
        InitStatsForLevel(m_owner->getLevel());

        if (m_owner->IsPlayer() && HasUnitTypeMask(UNIT_MASK_CONTROLABLE_GUARDIAN)) {
            m_charmInfo->InitCharmCreateSpells();
        }
    }

    SetReactState(REACT_AGGRESSIVE);
}

void Guardian::InitSummon()
{
    TempSummon::InitSummon();

    if (Unit* m_owner = GetOwner())
    {
        if (m_owner->IsPlayer() && m_owner->GetMinionGUID() == GetGUID() && !m_owner->GetCharmedGUID())
        {
            m_owner->ToPlayer()->CharmSpellInitialize();
        }
    }
}

Puppet::Puppet(SummonPropertiesEntry const* properties, ObjectGuid owner) : Minion(properties, owner, false), m_owner(owner)
{
    ASSERT(owner.IsPlayer());
    m_unitTypeMask |= UNIT_MASK_PUPPET;
}

void Puppet::InitStats(uint32 duration)
{
    Minion::InitStats(duration);
    SetLevel(GetOwner()->getLevel());
    SetReactState(REACT_PASSIVE);
}

void Puppet::InitSummon()
{
    Minion::InitSummon();
    if (!SetCharmedBy(GetOwner(), CHARM_TYPE_POSSESS))
        ABORT();
}

void Puppet::Update(uint32 time)
{
    Minion::Update(time);
    //check if caster is channelling?
    if (IsInWorld())
    {
        if (!IsAlive())
        {
            UnSummon();
            /// @todo why long distance .die does not remove it
        }
    }
}

void Puppet::RemoveFromWorld()
{
    if (!IsInWorld())
        return;

    RemoveCharmedBy(nullptr);
    Minion::RemoveFromWorld();
}

Player* Puppet::GetOwner() const
{
    return ObjectAccessor::GetPlayer(*this, m_owner);
}
