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

#ifndef _FORMATIONS_H
#define _FORMATIONS_H

#include "Define.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <map>

enum class GroupAIFlags : uint16
{
    GROUP_AI_FLAG_MEMBER_ASSIST_LEADER          = 0x001,
    GROUP_AI_FLAG_LEADER_ASSIST_MEMBER          = 0x002,
    GROUP_AI_FLAG_EVADE_TOGETHER                = 0x004,
    GROUP_AI_FLAG_RESPAWN_ON_EVADE              = 0x008,
    GROUP_AI_FLAG_DONT_RESPAWN_LEADER_ON_EVADE  = 0x010,
    GROUP_AI_FLAG_ACQUIRE_NEW_TARGET_ON_EVADE   = 0x020,
    //GROUP_AI_FLAG_UNK5                        = 0x040,
    //GROUP_AI_FLAG_UNK6                        = 0x080,
    //GROUP_AI_FLAG_UNK7                        = 0x100,
    GROUP_AI_FLAG_FOLLOW_LEADER                 = 0x200,

    GROUP_AI_FLAG_ASSIST_MASK                   = GROUP_AI_FLAG_MEMBER_ASSIST_LEADER | GROUP_AI_FLAG_LEADER_ASSIST_MEMBER,
    GROUP_AI_FLAG_EVADE_MASK                    = GROUP_AI_FLAG_EVADE_TOGETHER | GROUP_AI_FLAG_RESPAWN_ON_EVADE,

    // Used to verify valid and usable flags
    GROUP_AI_FLAG_SUPPORTED                     = GROUP_AI_FLAG_ASSIST_MASK | GROUP_AI_FLAG_EVADE_MASK | GROUP_AI_FLAG_DONT_RESPAWN_LEADER_ON_EVADE |
                                                  GROUP_AI_FLAG_FOLLOW_LEADER | GROUP_AI_FLAG_ACQUIRE_NEW_TARGET_ON_EVADE
};

struct FormationInfo
{
    FormationInfo() :
        leaderGUID(0),
        follow_dist(0.0f),
        follow_angle(0.0f),
        groupAI(0),
        point_1(0),
        point_2(0)
    {
    }

    ObjectGuid::LowType leaderGUID;
    float follow_dist;
    float follow_angle;
    uint16 groupAI;
    uint32 point_1;
    uint32 point_2;

    bool HasGroupFlag(uint16 flag) const { return !!(groupAI & flag); }
};

typedef std::unordered_map<uint32 /*memberDBGUID*/, FormationInfo>   CreatureGroupInfoType;

class FC_GAME_API FormationMgr
{
    private:
        FormationMgr() { }
        ~FormationMgr() { }

    public:
        static FormationMgr* instance();

        void AddCreatureToGroup(uint32 group_id, Creature* creature);
        void RemoveCreatureFromGroup(CreatureGroup* group, Creature* creature);
        void LoadCreatureFormations();
        CreatureGroupInfoType CreatureGroupMap;
};

class FC_GAME_API CreatureGroup
{    public:
        typedef std::map<Creature*, FormationInfo>  CreatureGroupMemberType;
        //Group cannot be created empty
        explicit CreatureGroup(uint32 id) : m_leader(nullptr), m_groupID(id), m_Formed(false) { }
        ~CreatureGroup() { }

        Creature* getLeader() const { return m_leader; }
        uint32 GetId() const { return m_groupID; }
        bool isEmpty() const { return m_members.empty(); }
        bool isFormed() const { return m_Formed; }
        bool IsLeader(Creature const* creature) const { return m_leader == creature; }
        const CreatureGroupMemberType& GetMembers() const { return m_members; }


        void AddMember(Creature* member);
        void RemoveMember(Creature* member);
        void FormationReset(bool dismiss, bool initMotionMaster);

        void LeaderStartedMoving();
        void MemberEngagingTarget(Creature* member, Unit* target);
        bool CanLeaderStartMoving() const;

        void LeaderMoveTo(float x, float y, float z, uint32 move_type);
        Unit* GetNewTargetForMember(Creature* member);
        void MemberEvaded(Creature* member);
        void RespawnFormation(bool force = false);
        [[nodiscard]] bool IsFormationInCombat();
        [[nodiscard]] bool IsAnyMemberAlive(bool ignoreLeader = false);
    private:
        Creature* m_leader; //Important do not forget sometimes to work with pointers instead synonims :D:D
        
        CreatureGroupMemberType m_members;

        uint32 m_groupID;
        bool m_Formed;

};

#define sFormationMgr FormationMgr::instance()

#endif
