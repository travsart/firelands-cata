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

#ifndef MOTIONMASTER_H
#define MOTIONMASTER_H

#include "Common.h"
#include "Errors.h"
#include "ObjectDefines.h"
#include "ObjectGuid.h"
#include "Optional.h"
#include "Position.h"
#include "SharedDefines.h"
#include <vector>

class MovementGenerator;
class Unit;
class PathGenerator;
struct SplineChainLink;
struct SplineChainResumeInfo;
struct WaypointPath;

namespace Movement
{
    class MoveSplineInit;
}

// Creature Entry ID used for waypoints show, visible only for GMs
#define VISUAL_WAYPOINT 1
// assume it is 25 yard per 0.6 second
#define SPEED_CHARGE    42.0f

enum MovementGeneratorType : uint8
{
    IDLE_MOTION_TYPE                = 0,                  // IdleMovementGenerator.h
    RANDOM_MOTION_TYPE              = 1,                  // RandomMovementGenerator.h
    WAYPOINT_MOTION_TYPE            = 2,                  // WaypointMovementGenerator.h
    CYCLIC_SPLINE_MOTION_TYPE       = 3,                  // CyclicMovementGenerator.h
    MAX_DB_MOTION_TYPE              = 4,                  // Below motion types can't be set in DB.
    CONFUSED_MOTION_TYPE            = 4,                  // ConfusedMovementGenerator.h
    CHASE_MOTION_TYPE               = 5,                  // TargetedMovementGenerator.h
    HOME_MOTION_TYPE                = 6,                  // HomeMovementGenerator.h
    FLIGHT_MOTION_TYPE              = 7,                  // WaypointMovementGenerator.h
    POINT_MOTION_TYPE               = 8,                  // PointMovementGenerator.h
    FLEEING_MOTION_TYPE             = 9,                  // FleeingMovementGenerator.h
    DISTRACT_MOTION_TYPE            = 10,                 // IdleMovementGenerator.h
    ASSISTANCE_MOTION_TYPE          = 11,                 // PointMovementGenerator.h
    ASSISTANCE_DISTRACT_MOTION_TYPE = 12,                 // IdleMovementGenerator.h
    TIMED_FLEEING_MOTION_TYPE       = 13,                 // FleeingMovementGenerator.h
    FOLLOW_MOTION_TYPE              = 14,
    ROTATE_MOTION_TYPE              = 15,
    EFFECT_MOTION_TYPE              = 16,
    ESCORT_MOTION_TYPE              = 17,                             // xinef: EscortMovementGenerator.h
    NULL_MOTION_TYPE                = 18



    // SPLINE_CHAIN_MOTION_TYPE        = 17,                 // SplineChainMovementGenerator.h
    // FORMATION_MOTION_TYPE           = 18,                 // FormationMovementGenerator.h
    // MAX_MOTION_TYPE                                       // limit
};

enum MovementSlot : uint8
{
    MOTION_SLOT_IDLE = 0,
    MOTION_SLOT_ACTIVE,
    MOTION_SLOT_CONTROLLED,
    MAX_MOTION_SLOT
};

enum MMCleanFlag
{
    MMCF_NONE   = 0,
    MMCF_UPDATE = 1, // Clear or Expire called from update
    MMCF_RESET  = 2,  // Flag if need top()->Reset()
    MMCF_INUSE  = 4 // pussywizard: Flag if in MotionMaster::UpdateMotion

};

enum RotateDirection
{
    ROTATE_DIRECTION_LEFT,
    ROTATE_DIRECTION_RIGHT
};

struct ChaseRange
{
    ChaseRange(float range);
    ChaseRange(float _minRange, float _maxRange);
    ChaseRange(float _minRange, float _minTolerance, float _maxTolerance, float _maxRange);

    // this contains info that informs how we should path!
    float MinRange;     // we have to move if we are within this range...    (min. attack range)
    float MinTolerance; // ...and if we are, we will move this far away
    float MaxRange;     // we have to move if we are outside this range...   (max. attack range)
    float MaxTolerance; // ...and if we are, we will move into this range
};

struct ChaseAngle
{
    ChaseAngle(float angle, float tol = M_PI_4) : RelativeAngle(Position::NormalizeOrientation(angle)), Tolerance(tol) {}

    float RelativeAngle; // we want to be at this angle relative to the target (0 = front, M_PI = back)
    float Tolerance;     // but we'll tolerate anything within +- this much

    float UpperBound() const { return Position::NormalizeOrientation(RelativeAngle + Tolerance); }
    float LowerBound() const { return Position::NormalizeOrientation(RelativeAngle - Tolerance); }
    bool IsAngleOkay(float relAngle) const
    {
        float const diff = std::abs(relAngle - RelativeAngle);
        return (std::min(diff, float(2 * M_PI) - diff) <= Tolerance);
    }
};

inline bool IsInvalidMovementGeneratorType(uint8 const type) { return type == MAX_DB_MOTION_TYPE || type >= MAX_MOTION_TYPE; }
inline bool IsInvalidMovementSlot(uint8 const slot) { return slot >= MAX_MOTION_SLOT; }

class FC_GAME_API MotionMaster
{
    private:
        typedef std::vector<MovementGenerator*> MovementList;
        typedef MovementGenerator* _Ty;

        void pop()
        {
            if (empty())
                return;

            Impl[_top] = nullptr;
            while (!empty() && !top())
                --_top;
        }

        [[nodiscard]] bool needInitTop() const
        {
            if (empty())
                return false;
            return _needInit[_top];
        }
        void InitTop();
    public:
        explicit MotionMaster(Unit* unit) : _expList(nullptr), _top(-1), _owner(unit), _cleanFlag(MMCF_NONE)
        {
            for (uint8 i = 0; i < MAX_MOTION_SLOT; ++i)
            {
                Impl[i] = nullptr;
                _needInit[i] = true;
            }
        }
        ~MotionMaster();

        void Initialize();
        void InitDefault();

        [[nodiscard]] bool empty() const { return (_top < 0); }
        [[nodiscard]] int size() const { return _top + 1; }

        [[nodiscard]] _Ty topOrNull() const
        {
            return empty() ? nullptr : top();
        }

        [[nodiscard]] _Ty top() const
        {
            ASSERT(!empty());
            return Impl[_top];
        }
        
        [[nodiscard]] _Ty GetMotionSlot(int slot) const
        {
            if (empty() || IsInvalidMovementSlot(slot) || !Impl[slot])
                return nullptr;

            return Impl[slot];
        }

        [[nodiscard]] uint8 GetCleanFlags() const { return _cleanFlag; }

        void DirectDelete(_Ty curr);
        void DelayedDelete(_Ty curr);

        void UpdateMotion(uint32 diff);

        void Clear(bool reset = true)
        {
            if (_cleanFlag & MMCF_UPDATE)
            {
                if (reset)
                    _cleanFlag |= MMCF_RESET;
                else
                    _cleanFlag &= ~MMCF_RESET;
                DelayedClean();
            }
            else
                DirectClean(reset);
        }
        void Clear(MovementSlot slot)
        {
            if (empty() || IsInvalidMovementSlot(slot))
                return;

            if (_cleanFlag & MMCF_UPDATE)
                DelayedClean(slot);
            else
                DirectClean(slot);
        }
        void MovementExpired(bool reset = true)
        {
            if (_cleanFlag & MMCF_UPDATE)
            {
                if (reset)
                    _cleanFlag |= MMCF_RESET;
                else
                    _cleanFlag &= ~MMCF_RESET;
                DelayedExpire();
            }
            else
                DirectExpire(reset);
        }
        void MovementExpiredOnSlot(MovementSlot slot, bool reset = true)
        {
            // xinef: cannot be used during motion update!
            if (!(_cleanFlag & MMCF_UPDATE))
                DirectExpireSlot(slot, reset);
        }

        void MoveIdle();
        void MoveTargetedHome(bool walk = false);
        void MoveRandom(float wanderDistance = 0.0f);
        void MoveFollow(Unit* target, float dist, float angle, MovementSlot slot = MOTION_SLOT_ACTIVE, bool inheritWalkState = true, bool inheritSpeed = true);
        void MoveChase(Unit* target, std::optional<ChaseRange> dist = {}, std::optional<ChaseAngle> angle = {});
        void MoveChase(Unit* target, float dist, float angle) { MoveChase(target, ChaseRange(dist), ChaseAngle(angle)); }
        void MoveChase(Unit* target, float dist) { MoveChase(target, ChaseRange(dist)); }
        void MoveCircleTarget(Unit* target);
        void MoveBackwards(Unit* target, float dist);
        void MoveForwards(Unit* target, float dist);
        void MoveConfused();
        void MoveFleeing(Unit* enemy, uint32 time = 0);
        void MovePoint(uint32 id, const Position& pos, bool generatePath = true, bool forceDestination = true)
        { MovePoint(id, pos.m_positionX, pos.m_positionY, pos.m_positionZ, generatePath, forceDestination, MOTION_SLOT_ACTIVE, pos.GetOrientation()); }
        void MovePoint(uint32 id, float x, float y, float z, bool generatePath = true, bool forceDestination = true, MovementSlot slot = MOTION_SLOT_ACTIVE, float orientation = 0.0f);
        void MoveSplinePath(Movement::PointsArray* path);
        void MoveSplinePath(uint32 path_id);

        // void MoveCloserAndStop(uint32 id, Unit* target, float distance);  TODO replace with MoveFollow(GetCaster(), 0.0f, 0.0f, MOTION_SLOT_CONTROLLED);

        // These two movement types should only be used with creatures having landing/takeoff animations
        void MoveLand(uint32 id, Position const& pos, Optional<float> velocity = { });
        void MoveLand(uint32 id, float x, float y, float z, float speed = 0.0f); // pussywizard: added for easy calling by passing 3 floats x, y, z
        void MoveTakeoff(uint32 id, Position const& pos, Optional<float> velocity = { });
        void MoveTakeoff(uint32 id, float x, float y, float z, float speed = 0.0f, bool skipAnimation = false); // pussywizard: added for easy calling by passing 3 floats x, y, z


        void MoveCharge(float x, float y, float z, float speed = SPEED_CHARGE, uint32 id = EVENT_CHARGE, const Movement::PointsArray* path = nullptr, bool generatePath = false, float orientation = 0.0f, ObjectGuid targetGUID = ObjectGuid::Empty);
        void MoveCharge(PathGenerator const& path, float speed = SPEED_CHARGE, ObjectGuid targetGUID = ObjectGuid::Empty);
        void MoveKnockbackFrom(float srcX, float srcY, float speedXY, float speedZ);
        void MoveJumpTo(float angle, float speedXY, float speedZ);
        
        void MoveJump(Position const& pos, float speedXY, float speedZ, uint32 id = EVENT_JUMP)
        { MoveJump(pos.m_positionX, pos.m_positionY, pos.m_positionZ, speedXY, speedZ, id); };
        void MoveJump(float x, float y, float z, float speedXY, float speedZ, uint32 id = 0, Unit const* target = nullptr);
        void MoveJumpWithGravity(Position const& pos, float speedXY, float gravity, uint32 id = EVENT_JUMP);
        void MoveFall(uint32 id = 0, bool addFlagForNPC = false);

        void MoveCirclePath(float x, float y, float z, float radius, bool clockwise, uint8 stepCount, float velocity = 0.f);
        void MoveCyclicPath(Position const* pathPoints, size_t pathSize, bool walk = false, bool fly = false, float velocity = 0.f);
        void MoveCyclicPath(uint32 pathId);
        void MoveSmoothPath(uint32 pointId, Position const* pathPoints, size_t pathSize, bool walk = false, bool fly = false, float velocity = 0.f);
        // Walk along spline chain stored in DB (script_spline_chain_meta and script_spline_chain_waypoints)
        void MoveAlongSplineChain(uint32 pointId, uint16 dbChainId, bool walk);
        void MoveAlongSplineChain(uint32 pointId, std::vector<SplineChainLink> const& chain, bool walk);
        void ResumeSplineChain(SplineChainResumeInfo const& info);

        void MoveSeekAssistance(float x, float y, float z);
        void MoveSeekAssistanceDistract(uint32 timer);
        void MoveTaxiFlight(uint32 path, uint32 pathnode);
        void MoveDistract(uint32 time);
        void MovePath(uint32 pathId, bool repeatable);
        void MovePath(WaypointPath& path, bool repeatable);
        void MoveRotate(uint32 time, RotateDirection direction);
#ifdef MOD_PLAYERBOTS
        void MoveKnockbackFromForPlayer(float srcX, float srcY, float speedXY, float speedZ);
        void MovePointBackwards(uint32 id, float x, float y, float z, bool generatePath = true, bool forceDestination = true, MovementSlot slot = MOTION_SLOT_ACTIVE, float orientation = 0.0f);
#endif
        MovementSlot GetCurrentSlot() const;
        MovementGeneratorType GetCurrentMovementGeneratorType() const;
        MovementGeneratorType GetMotionSlotType(MovementSlot slot) const;
        [[nodiscard]] uint32 GetCurrentSplineId() const; // Xinef: Escort system

        void propagateSpeedChange();
        void ReinitializeMovement();

        bool GetDestination(float &x, float &y, float &z);

        void LaunchMoveSpline(Movement::MoveSplineInit&& init, uint32 id = 0, MovementSlot slot = MOTION_SLOT_ACTIVE, MovementGeneratorType type = EFFECT_MOTION_TYPE);

private:
    void Mutate(MovementGenerator* m, MovementSlot slot);                  // use Move* functions instead

    void DirectClean(bool reset);
    void DirectClean(MovementSlot slot);
    void DelayedClean();
    void DelayedClean(MovementSlot slot);

    void DirectExpire(bool reset);
    void DirectExpireSlot(MovementSlot slot, bool reset);
    void DelayedExpire();

    typedef std::vector<_Ty> ExpireList;
    ExpireList* _expList;
    _Ty Impl[MAX_MOTION_SLOT];
    int _top;
    Unit* _owner;
    bool _needInit[MAX_MOTION_SLOT];
    uint8 _cleanFlag;
};

#endif // MOTIONMASTER_H
