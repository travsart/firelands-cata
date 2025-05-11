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

#ifndef SCRIPT_OBJECT_ACHIEVEMENT_SCRIPT_H_
#define SCRIPT_OBJECT_ACHIEVEMENT_SCRIPT_H_

#include "Duration.h"
#include "ScriptObject.h"
#include "Player.h"
#include "Guild.h"
#include <list>
#include <vector>


enum AchievementHook
{
    ACHIEVEMENTHOOK_SET_REALM_COMPLETED,
    ACHIEVEMENTHOOK_IS_COMPLETED_CRITERIA,
    ACHIEVEMENTHOOK_IS_REALM_COMPLETED,
    ACHIEVEMENTHOOK_ON_BEFORE_CHECK_CRITERIA,
    ACHIEVEMENTHOOK_CAN_CHECK_CRITERIA,
    ACHIEVEMENTHOOK_END
};

class AchievementScript : public ScriptObject
{
protected:
    AchievementScript(const char* name, std::vector<uint16> enabledHooks = std::vector<uint16>());

public:
    [[nodiscard]] bool IsDatabaseBound() const override { return false; }

    // After complete global acvievement
    virtual void SetRealmCompleted(AchievementEntry const* /*achievement*/) { }

    [[nodiscard]] virtual bool IsCompletedCriteria(AchievementMgr<Player>* /*mgr*/, AchievementCriteriaEntry const* /*achievementCriteria*/, AchievementEntry const* /*achievement*/, CriteriaProgress const* /*progress*/) { return true; }
    [[nodiscard]] virtual bool IsCompletedCriteria(AchievementMgr<Guild>* /*mgr*/, AchievementCriteriaEntry const* /*achievementCriteria*/, AchievementEntry const* /*achievement*/, CriteriaProgress const* /*progress*/) { return true; }

    [[nodiscard]] virtual bool IsRealmCompleted(AchievementGlobalMgr const* /*globalmgr*/, AchievementEntry const* /*achievement*/, SystemTimePoint /*completionTime*/) { return true; }

    [[nodiscard]] virtual void OnBeforeCheckCriteria(AchievementMgr<Player>* /*mgr*/, AchievementCriteriaEntryList const* /*achievementCriteriaList*/) { }
    [[nodiscard]] virtual void OnBeforeCheckCriteria(AchievementMgr<Guild>* /*mgr*/, AchievementCriteriaEntryList const* /*achievementCriteriaList*/) { }

    [[nodiscard]] virtual bool CanCheckCriteria(AchievementMgr<Player>* /*mgr*/, AchievementCriteriaEntry const* /*achievementCriteria*/) { return true; }
    [[nodiscard]] virtual bool CanCheckCriteria(AchievementMgr<Guild>* /*mgr*/, AchievementCriteriaEntry const* /*achievementCriteria*/) { return true; }
};

#endif
