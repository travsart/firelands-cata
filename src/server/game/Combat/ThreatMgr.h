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

 #ifndef FIRELANDS_THREATMANAGER_H
 #define FIRELANDS_THREATMANAGER_H

#include "IteratorPair.h"
#include "ObjectGuid.h"
#include "Reference.h"
#include "SharedDefines.h"
#include "UnitEvents.h"
#include <list>

//==============================================================

class Unit;
class Creature;
class ThreatMgr;
class SpellInfo;

#define THREAT_UPDATE_INTERVAL 2 * IN_MILLISECONDS    // Server should send threat update to client periodically each second

//==============================================================
// Class to calculate the real threat based

struct ThreatCalcHelper
{
    static float calcThreat(Unit* hatedUnit, float threat, SpellSchoolMask schoolMask = SPELL_SCHOOL_MASK_NORMAL, SpellInfo const* threatSpell = nullptr);
    static bool isValidProcess(Unit* hatedUnit, Unit* hatingUnit, SpellInfo const* threatSpell = nullptr);
};

//==============================================================
class HostileReference : public Reference<Unit, ThreatMgr>
{
public:
    HostileReference(Unit* refUnit, ThreatMgr* threatMgr, float threat);

    Unit* GetOwner() const;
    Unit* GetVictim() const { return getTarget(); }

    //=================================================
    void AddThreat(float modThreat);

    void SetThreat(float threat) { AddThreat(threat - GetThreat()); }

    void addThreatPercent(int32 percent);

    [[nodiscard]] float GetThreat() const { return iThreat; }

    void ClearThreat() { removeReference(); }

    [[nodiscard]] bool IsOnline() const { return iOnline; }
    [[nodiscard]] bool IsAvailable() const { return iOnline; } // unused for now
    [[nodiscard]] bool IsOffline() const { return !iOnline; } // unused for now

    // used for temporary setting a threat and reducting it later again.
    // the threat modification is stored
    void setTempThreat(float threat)
    {
        addTempThreat(threat - GetThreat());
    }

    void addTempThreat(float threat)
    {
        iTempThreatModifier = threat;
        if (iTempThreatModifier != 0.0f)
            AddThreat(iTempThreatModifier);
    }

    void resetTempThreat()
    {
        if (iTempThreatModifier != 0.0f)
        {
            AddThreat(-iTempThreatModifier);
            iTempThreatModifier = 0.0f;
        }
    }

    float getTempThreatModifier() { return iTempThreatModifier; }

    //=================================================
    // check, if source can reach target and set the status
    void updateOnlineStatus();

    void setOnlineOfflineState(bool isOnline);
    //=================================================

    bool operator == (const HostileReference& hostileRef) const { return hostileRef.getUnitGuid() == getUnitGuid(); }

    //=================================================

    [[nodiscard]] ObjectGuid getUnitGuid() const { return iUnitGuid; }

    //=================================================
    // reference is not needed anymore. realy delete it !

    void removeReference();

    //=================================================

    HostileReference* next() { return ((HostileReference*) Reference<Unit, ThreatMgr>::next()); }

    //=================================================

    // Tell our refTo (target) object that we have a link
    void targetObjectBuildLink() override;

    // Tell our refTo (taget) object, that the link is cut
    void targetObjectDestroyLink() override;

    // Tell our refFrom (source) object, that the link is cut (Target destroyed)
    void sourceObjectDestroyLink() override;
private:
    // Inform the source, that the status of that reference was changed
    void fireStatusChanged(ThreatRefStatusChangeEvent& threatRefStatusChangeEvent);

    Unit* GetSourceUnit();
private:
    float iThreat;
    float iTempThreatModifier;                          // used for taunt
    ObjectGuid iUnitGuid;
    bool iOnline;
};

class ThreatMgr;

class ThreatContainer
{
    friend class ThreatMgr;

public:
    typedef std::list<HostileReference*> StorageType;

    ThreatContainer() = default;

    ~ThreatContainer() { clearReferences(); }

    HostileReference* AddThreat(Unit* victim, float threat);

    void ModifyThreatByPercent(Unit* victim, int32 percent);

    HostileReference* SelectNextVictim(Creature* attacker, HostileReference* currentVictim) const;

    void setDirty(bool isDirty) { iDirty = isDirty; }

    [[nodiscard]] bool isDirty() const { return iDirty; }

    [[nodiscard]] bool empty() const
    {
        return iThreatList.empty();
    }

    [[nodiscard]] HostileReference* getMostHated() const
    {
        return iThreatList.empty() ? nullptr : iThreatList.front();
    }

    HostileReference* getReferenceByTarget(Unit const* victim) const;
    HostileReference* getReferenceByTarget(ObjectGuid const& guid) const;

    [[nodiscard]] StorageType const& GetThreatList() const { return iThreatList; }

private:
    void remove(HostileReference* hostileRef)
    {
        iThreatList.remove(hostileRef);
    }

    void addReference(HostileReference* hostileRef)
    {
        iThreatList.push_back(hostileRef);
    }

    void clearReferences();

    // Sort the list if necessary
    void update();

    StorageType iThreatList;
    bool iDirty{false};
};

struct RedirectThreatInfo
{
    RedirectThreatInfo() = default;
    ObjectGuid _targetGUID;
    uint32 _threatPct{ 0 };

    [[nodiscard]] ObjectGuid GetTargetGUID() const { return _targetGUID; }
    [[nodiscard]] uint32 GetThreatPct() const { return _threatPct; }

    void Set(ObjectGuid guid, uint32 pct)
    {
        _targetGUID = guid;
        _threatPct = pct;
    }

    void ModifyThreatPct(int32 amount)
    {
        amount += _threatPct;
        _threatPct = uint32(std::max(0, amount));
    }
};

//=================================================

namespace Firelands
{
    // Binary predicate for sorting HostileReferences based on threat value
    class ThreatOrderPred
    {
    public:
        ThreatOrderPred(bool ascending = false) : m_ascending(ascending) {}
        bool operator() (HostileReference const* a, HostileReference const* b) const
        {
            return m_ascending ? a->GetThreat() < b->GetThreat() : a->GetThreat() > b->GetThreat();
        }
    private:
        const bool m_ascending;
    };
}

 #endif
