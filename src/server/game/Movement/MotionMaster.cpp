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

#include "MotionMaster.h"
#include "ChaseMovementGenerator.h"
#include "ConfusedMovementGenerator.h"
#include "Creature.h"
#include "CreatureAISelector.h"
#include "CyclicMovementGenerator.h"
#include "DBCStores.h"
#include "G3DPosition.hpp"
#include "FleeingMovementGenerator.h"
#include "FlightPathMovementGenerator.h"
#include "FormationMovementGenerator.h"
#include "GenericMovementGenerator.h"
#include "HomeMovementGenerator.h"
#include "IdleMovementGenerator.h"
#include "Log.h"
#include "Map.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "PathGenerator.h"
#include "Player.h"
#include "PointMovementGenerator.h"
#include "RandomMovementGenerator.h"
#include "ScriptSystem.h"
#include "SplineChainMovementGenerator.h"
#include "TargetedMovementGenerator.h"
#include "Unit.h"
#include "WaypointDefines.h"
#include "WaypointMovementGenerator.h"
#include "WaypointManager.h"

inline MovementGenerator* GetIdleMovementGenerator()
{
    return sMovementGeneratorRegistry->GetRegistryItem(IDLE_MOTION_TYPE)->Create();
}

inline bool isStatic(MovementGenerator* movement)
{
    return (movement == GetIdleMovementGenerator());
}


 // ---- ChaseRange ---- //

ChaseRange::ChaseRange(float range) : MinRange(range > CONTACT_DISTANCE ? 0 : range - CONTACT_DISTANCE), MinTolerance(range), MaxRange(range + CONTACT_DISTANCE), MaxTolerance(range) { }
ChaseRange::ChaseRange(float _minRange, float _maxRange) : MinRange(_minRange), MinTolerance(std::min(_minRange + CONTACT_DISTANCE, (_minRange + _maxRange) / 2)), MaxRange(_maxRange), MaxTolerance(std::max(_maxRange - CONTACT_DISTANCE, MinTolerance)) { }
ChaseRange::ChaseRange(float _minRange, float _minTolerance, float _maxTolerance, float _maxRange) : MinRange(_minRange), MinTolerance(_minTolerance), MaxRange(_maxRange), MaxTolerance(_maxTolerance) { }

// ---- ChaseAngle ---- //

ChaseAngle::ChaseAngle(float angle, float _tolerance/* = M_PI_4*/) : RelativeAngle(Position::NormalizeOrientation(angle)), Tolerance(_tolerance) { }

float ChaseAngle::UpperBound() const
{
    return Position::NormalizeOrientation(RelativeAngle + Tolerance);
}

float ChaseAngle::LowerBound() const
{
    return Position::NormalizeOrientation(RelativeAngle - Tolerance);
}

bool ChaseAngle::IsAngleOkay(float relativeAngle) const
{
    float const diff = std::abs(relativeAngle - RelativeAngle);

    return (std::min(diff, float(2 * M_PI) - diff) <= Tolerance);
}


MotionMaster::~MotionMaster()
{
    // clear ALL movement generators (including default)
    while (!empty())
    {
        MovementGenerator* movement = top();
        pop();
        if (movement && !isStatic(movement))
            delete movement;
    }

    InitDefault();
}

void MotionMaster::Initialize()
{
    // clear ALL movement generators (including default)
    while (!empty())
    {
        MovementGenerator* curr = top();
        pop();
        if (curr)
            DirectDelete(curr);
    }

    InitDefault();
}

// set new default movement generator
void MotionMaster::InitDefault()
{
    Mutate(FactorySelector::SelectMovementGenerator(_owner), MOTION_SLOT_IDLE);
}

void MotionMaster::UpdateMotion(uint32 diff)
{
    if (!_owner)
        return;

    ASSERT(!empty());

    _cleanFlag |= MMCF_INUSE;

    _cleanFlag |= MMCF_UPDATE;
    if (!top()->Update(_owner, diff))
    {
        _cleanFlag &= ~MMCF_UPDATE;
        MovementExpired();
    }
    else
        _cleanFlag &= ~MMCF_UPDATE;

    if (_expList)
    {
        for (std::size_t i = 0; i < _expList->size(); ++i)
        {
            MovementGenerator* mg = (*_expList)[i];
            DirectDelete(mg);
        }

        delete _expList;
        _expList = nullptr;

        if (empty())
            Initialize();
        else if (needInitTop())
            InitTop();
        else if (_cleanFlag & MMCF_RESET)
            top()->Reset(_owner);

        _cleanFlag &= ~MMCF_RESET;
    }

    _cleanFlag &= ~MMCF_INUSE;
}

void MotionMaster::DirectClean(bool reset)
{
    while (size() > 1)
    {
        MovementGenerator* curr = top();
        pop();
        if (curr) DirectDelete(curr);
    }

    if (empty())
        return;

    if (needInitTop())
        InitTop();
    else if (reset)
        top()->Reset(_owner);
}

void MotionMaster::DelayedClean()
{
    while (size() > 1)
    {
        MovementGenerator* curr = top();
        pop();
        if (curr)
            DelayedDelete(curr);
    }
}

void MotionMaster::DirectClean(MovementSlot slot)
{
    if (MovementGenerator* motion = GetMotionSlot(slot))
    {
        Impl[slot] = nullptr;
        DirectDelete(motion);
    }

    while (!empty() && !top())
        --_top;

    if (empty())
        Initialize();
    else if (needInitTop())
        InitTop();
}

void MotionMaster::DelayedClean(MovementSlot slot)
{
    if (MovementGenerator* motion = GetMotionSlot(slot))
    {
        Impl[slot] = nullptr;
        DelayedDelete(motion);
    }

    while (!empty() && !top())
        --_top;
}

void MotionMaster::DirectExpire(bool reset)
{
    if (size() > 1)
    {
        MovementGenerator* curr = top();
        pop();
        DirectDelete(curr);
    }

    while (!empty() && !top())
        --_top;

    if (empty())
        Initialize();
    else if (needInitTop())
        InitTop();
    else if (reset)
        top()->Reset(_owner);
}

void MotionMaster::DelayedExpire()
{
    if (size() > 1)
    {
        MovementGenerator* curr = top();
        pop();
        DelayedDelete(curr);
    }

    while (!empty() && !top())
        --_top;
}

void MotionMaster::DirectExpireSlot(MovementSlot slot, bool reset)
{
    if (size() > 1)
    {
        MovementGenerator* curr = Impl[slot];

        // pussywizard: clear slot AND decrease top immediately to avoid crashes when referencing null top in DirectDelete
        Impl[slot] = nullptr;
        while (!empty() && !top())
            --_top;

        if (curr) DirectDelete(curr);
    }

    while (!empty() && !top())
        --_top;

    if (empty())
        Initialize();
    else if (needInitTop())
        InitTop();
    else if (reset)
        top()->Reset(_owner);
}

void MotionMaster::DirectDelete(_Ty curr)
{
    if (isStatic(curr))
        return;


    curr->Finalize(_owner);
    curr->NotifyAIOnFinalize(_owner);
    delete curr;
}

void MotionMaster::DelayedDelete(_Ty curr)
{
    LOG_DEBUG("movement.motionmaster", "MotionMaster::DelayedDelete: '%s', delayed deleting movement generator (type: %u)", _owner->GetGUID().ToString().c_str(), curr->GetMovementGeneratorType());
    if (isStatic(curr))
        return;

    if (!_expList)
        _expList = new ExpireList();
    _expList->push_back(curr);
}

void MotionMaster::MoveIdle()
{
    //! Should be preceded by MovementExpired or Clear if there's an overlying movementgenerator active
    if (empty() || !isStatic(top()))
        Mutate(GetIdleMovementGenerator(), MOTION_SLOT_IDLE);
}


/**
 * @brief Enable a random movement in desired range around the unit. Doesn't work with UNIT_FLAG_DISABLE_MOVE
 */
void MotionMaster::MoveRandom(float wanderDistance)
{
    if (_owner->HasUnitFlag(UNIT_FLAG_DISABLE_MOVE))
        return;

    if (_owner->IsCreature())
    {
        LOG_DEBUG("movement.motionmaster", "Creature ({}) start moving random", _owner->GetGUID().ToString());
        Mutate(new RandomMovementGenerator<Creature>(wanderDistance), MOTION_SLOT_IDLE);
    }
}

/**
 * @brief The unit will return this initial position (owner for pets and summoned creatures). Doesn't work with UNIT_FLAG_DISABLE_MOVE
 *
 * @param walk The unit will run by default, but you can set it to walk
 */
void MotionMaster::MoveTargetedHome(bool walk /*= false*/)
{
    Clear(false);

    if (_owner->IsCreature() && !_owner->ToCreature()->GetCharmerOrOwnerGUID())
    {
        LOG_DEBUG("movement.motionmaster", "Creature ({}) targeted home", _owner->GetGUID().ToString());
        Mutate(new HomeMovementGenerator<Creature>(walk), MOTION_SLOT_ACTIVE);
    }
    else if (_owner->IsCreature() && _owner->ToCreature()->GetCharmerOrOwnerGUID())
    {
        _owner->ClearUnitState(UNIT_STATE_EVADE);

        if (_owner->HasUnitFlag(UNIT_FLAG_DISABLE_MOVE))
            return;

        LOG_DEBUG("movement.motionmaster", "Pet or controlled creature ({}) targeting home", _owner->GetGUID().ToString());
        Unit* target = _owner->ToCreature()->GetCharmerOrOwner();
        if (target)
        {
            LOG_DEBUG("movement.motionmaster", "Following {} ({})", target->IsPlayer() ? "player" : "creature", target->GetGUID().ToString());
            Mutate(new FollowMovementGenerator<Creature>(target, PET_FOLLOW_DIST, _owner->GetFollowAngle(), true, true), MOTION_SLOT_ACTIVE);
        }
    }
    else
    {
        LOG_ERROR("movement.motionmaster", "Player ({}) attempt targeted home", _owner->GetGUID().ToString());
    }
}

/**
 * @brief Enable the confusion movement. Doesn't work with UNIT_FLAG_DISABLE_MOVE
 */
void MotionMaster::MoveConfused()
{
    // Xinef: do not allow to move with UNIT_FLAG_DISABLE_MOVE
    if (_owner->HasUnitFlag(UNIT_FLAG_DISABLE_MOVE))
        return;

    if (_owner->IsPlayer())
    {
        LOG_DEBUG("movement.motionmaster", "Player ({}) move confused", _owner->GetGUID().ToString());
        Mutate(new ConfusedMovementGenerator<Player>(), MOTION_SLOT_CONTROLLED);
    }
    else
    {
        LOG_DEBUG("movement.motionmaster", "Creature ({}) move confused", _owner->GetGUID().ToString());
        Mutate(new ConfusedMovementGenerator<Creature>(), MOTION_SLOT_CONTROLLED);
    }
}

/**
 * @brief Force the unit to chase this target. Doesn't work with UNIT_FLAG_DISABLE_MOVE
 */
void MotionMaster::MoveChase(Unit* target,  std::optional<ChaseRange> dist, std::optional<ChaseAngle> angle)
{
    // ignore movement request if target not exist
    if (!target || target == _owner || _owner->HasUnitFlag(UNIT_FLAG_DISABLE_MOVE))
        return;

    //_owner->ClearUnitState(UNIT_STATE_FOLLOW);
    if (_owner->IsPlayer())
    {
        LOG_DEBUG("movement.motionmaster", "Player ({}) chase to {} ({})",
            _owner->GetGUID().ToString(), target->IsPlayer() ? "player" : "creature", target->GetGUID().ToString());
        Mutate(new ChaseMovementGenerator<Player>(target, dist, angle), MOTION_SLOT_ACTIVE);
    }
    else
    {
        LOG_DEBUG("movement.motionmaster", "Creature ({}) chase to {} ({})",
            _owner->GetGUID().ToString(), target->IsPlayer() ? "player" : "creature", target->GetGUID().ToString());
        Mutate(new ChaseMovementGenerator<Creature>(target, dist, angle), MOTION_SLOT_ACTIVE);
    }
}

void MotionMaster::MoveBackwards(Unit* target, float dist)
{
    if (!target)
    {
        return;
    }

    Position const& pos = target->GetPosition();
    float angle = target->GetAngle(_owner);
    G3D::Vector3 point;
    point.x = pos.m_positionX + dist * cosf(angle);
    point.y = pos.m_positionY + dist * sinf(angle);
    point.z = pos.m_positionZ;

    if (!_owner->GetMap()->CanReachPositionAndGetValidCoords(_owner, point.x, point.y, point.z, true, true))
    {
        return;
    }

    Movement::MoveSplineInit init(_owner);
    init.MoveTo(point.x, point.y, point.z, false);
    init.SetFacing(target);
    init.SetOrientationInversed();
    init.Launch();
}

void MotionMaster::MoveForwards(Unit* target, float dist)
{
    //like movebackwards, but without the inversion
    if (!target)
    {
        return;
    }

    Position const& pos = target->GetPosition();
    float angle = target->GetAngle(_owner);
    G3D::Vector3 point;
    point.x = pos.m_positionX + dist * cosf(angle);
    point.y = pos.m_positionY + dist * sinf(angle);
    point.z = pos.m_positionZ;

    if (!_owner->GetMap()->CanReachPositionAndGetValidCoords(_owner, point.x, point.y, point.z, true, true))
    {
        return;
    }

    Movement::MoveSplineInit init(_owner);
    init.MoveTo(point.x, point.y, point.z, false);
    init.SetFacing(target);
    init.Launch();
}

void MotionMaster::MoveCircleTarget(Unit* target)
{
    if (!target)
    {
        return;
    }

    Position pos;
    if (!target->GetMeleeAttackPoint(_owner, pos))
    {
        return;
    }

    Movement::MoveSplineInit init(_owner);
    init.MoveTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), false);
    init.SetWalk(true);
    init.SetFacing(target);
    init.Launch();
}

void MotionMaster::MoveFleeing(Unit* enemy, uint32 time)
{
    if (!enemy)
        return;

    LOG_DEBUG("movement.motionmaster", "MotionMaster::MoveFleeing: '%s', flees from '%s' (time: %u)", _owner->GetGUID().ToString().c_str(), enemy->GetGUID().ToString().c_str(), time);
    if (_owner->GetTypeId() == TYPEID_UNIT)
    {
        if (time)
            Mutate(new TimedFleeingMovementGenerator(enemy->GetGUID(), time), MOTION_SLOT_CONTROLLED);
        else
            Mutate(new FleeingMovementGenerator<Creature>(enemy->GetGUID()), MOTION_SLOT_CONTROLLED);
    }
    else
        Mutate(new FleeingMovementGenerator<Player>(enemy->GetGUID()), MOTION_SLOT_CONTROLLED);
}

void MotionMaster::MovePoint(uint32 id, float x, float y, float z, bool generatePath, float speed)
{
    if (_owner->GetTypeId() == TYPEID_PLAYER)
    {
        LOG_DEBUG("movement.motionmaster", "MotionMaster::MovePoint: '%s', targeted point Id: %u (X: %f, Y: %f, Z: %f)", _owner->GetGUID().ToString().c_str(), id, x, y, z);
        Mutate(new PointMovementGenerator<Player>(id, x, y, z, generatePath, speed), MOTION_SLOT_ACTIVE);
    }
    else
    {
        LOG_DEBUG("movement.motionmaster", "MotionMaster::MovePoint: '%s', targeted point Id: %u (X: %f, Y: %f, Z: %f)", _owner->GetGUID().ToString().c_str(), id, x, y, z);
        Mutate(new PointMovementGenerator<Creature>(id, x, y, z, generatePath, speed), MOTION_SLOT_ACTIVE);
    }
}

void MotionMaster::MoveCloserAndStop(uint32 id, Unit* target, float distance)
{
    float distanceToTravel = _owner->GetExactDist2d(target) - distance;
    if (distanceToTravel > 0.0f)
    {
        float angle = _owner->GetAngle(target);
        float destx = _owner->GetPositionX() + distanceToTravel * std::cos(angle);
        float desty = _owner->GetPositionY() + distanceToTravel * std::sin(angle);
        MovePoint(id, destx, desty, target->GetPositionZ());
    }
    else
    {
        // we are already close enough. We just need to turn toward the target without changing position.
        Movement::MoveSplineInit init(_owner);
        init.MoveTo(_owner->GetPositionX(), _owner->GetPositionY(), _owner->GetPositionZ());
        init.SetFacing(target);
        Mutate(new GenericMovementGenerator(std::move(init), EFFECT_MOTION_TYPE, id), MOTION_SLOT_ACTIVE);
    }
}

void MotionMaster::MoveLand(uint32 id, Position const& pos, Optional<float> velocity /*= { }*/)
{
    LOG_DEBUG("movement.motionmaster", "MotionMaster::MoveLand: '%s', landing point Id: %u (X: %f, Y: %f, Z: %f)", _owner->GetGUID().ToString().c_str(), id, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());

    Movement::MoveSplineInit init(_owner);
    init.MoveTo(PositionToVector3(pos), false);
    init.SetSmooth();
    init.SetFly();
    init.SetAnimation(AnimationTier::Ground);
    if (velocity)
        init.SetVelocity(velocity.value());
    Mutate(new GenericMovementGenerator(std::move(init), EFFECT_MOTION_TYPE, id), MOTION_SLOT_ACTIVE);
}

void MotionMaster::MoveTakeoff(uint32 id, Position const& pos, Optional<float> velocity /*= { }*/)
{
    LOG_DEBUG("movement.motionmaster", "MotionMaster::MoveTakeoff: '%s', landing point Id: %u (X: %f, Y: %f, Z: %f)", _owner->GetGUID().ToString().c_str(), id, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());

    Movement::MoveSplineInit init(_owner);
    init.MoveTo(PositionToVector3(pos), false);
    init.SetSmooth();
    init.SetFly();
    init.SetAnimation(AnimationTier::Hover);
    if (velocity)
        init.SetVelocity(velocity.value());
    Mutate(new GenericMovementGenerator(std::move(init), EFFECT_MOTION_TYPE, id), MOTION_SLOT_ACTIVE);
}

void MotionMaster::MoveCharge(float x, float y, float z, float speed /*= SPEED_CHARGE*/, uint32 id /*= EVENT_CHARGE*/, bool generatePath /*= false*/)
{
    if (Impl[MOTION_SLOT_CONTROLLED] && Impl[MOTION_SLOT_CONTROLLED]->GetMovementGeneratorType() != DISTRACT_MOTION_TYPE)
        return;

    if (_owner->GetTypeId() == TYPEID_PLAYER)
    {
        LOG_DEBUG("movement.motionmaster", "MotionMaster::MoveCharge: '%s', charging point Id: %u (X: %f, Y: %f, Z: %f)", _owner->GetGUID().ToString().c_str(), id, x, y, z);
        Mutate(new PointMovementGenerator<Player>(id, x, y, z, generatePath, speed), MOTION_SLOT_CONTROLLED);
    }
    else
    {
        LOG_DEBUG("movement.motionmaster", "MotionMaster::MoveCharge: '%s', charging point Id: %u (X: %f, Y: %f, Z: %f)", _owner->GetGUID().ToString().c_str(), id, x, y, z);
        Mutate(new PointMovementGenerator<Creature>(id, x, y, z, generatePath, speed), MOTION_SLOT_CONTROLLED);
    }
}

void MotionMaster::MoveCharge(PathGenerator const& path, float speed /*= SPEED_CHARGE*/)
{
    G3D::Vector3 dest = path.GetActualEndPosition();

    MoveCharge(dest.x, dest.y, dest.z, speed, EVENT_CHARGE_PREPATH);

    // Charge movement is not started when using EVENT_CHARGE_PREPATH
    Movement::MoveSplineInit init(_owner);
    init.MovebyPath(path.GetPath());
    init.SetVelocity(speed);
    init.Launch();
}

void MotionMaster::MoveKnockbackFrom(float srcX, float srcY, float speedXY, float speedZ)
{
    //this function may make players fall below map
    if (_owner->GetTypeId() == TYPEID_PLAYER)
        return;

    if (speedXY < 0.01f)
        return;

    Position dest = _owner->GetPosition();
    float moveTimeHalf = speedZ / Movement::gravity;
    float dist = 2 * moveTimeHalf * speedXY;
    float max_height = -Movement::computeFallElevation(moveTimeHalf, false, -speedZ);

    // Use a mmap raycast to get a valid destination.
    _owner->MovePositionToFirstCollision(dest, dist, _owner->GetRelativeAngle(srcX, srcY) + float(M_PI));

    Movement::MoveSplineInit init(_owner);
    init.MoveTo(dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ(), false);
    init.SetParabolic(max_height, 0);
    init.SetOrientationFixed(true);
    init.SetVelocity(speedXY);
    Mutate(new GenericMovementGenerator(std::move(init), EFFECT_MOTION_TYPE, 0), MOTION_SLOT_CONTROLLED);
}

void MotionMaster::MoveJumpTo(float angle, float speedXY, float speedZ)
{
    //this function may make players fall below map
    if (_owner->GetTypeId() == TYPEID_PLAYER)
        return;

    float x, y, z = _owner->GetPositionZ();

    float moveTimeHalf = speedZ / Movement::gravity;
    float dist = 2 * moveTimeHalf * speedXY;
    _owner->GetNearPoint2D(nullptr, x, y, dist, _owner->GetOrientation() + angle);
    _owner->UpdateAllowedPositionZ(x, y, z);
    MoveJump(x, y, z, 0.0f, speedXY, speedZ);
}

void MotionMaster::MoveJump(float x, float y, float z, float o, float speedXY, float speedZ, uint32 id, bool hasOrientation /* = false*/)
{
    LOG_DEBUG("movement.motionmaster", "MotionMaster::MoveJump: '%s', jumps to point Id: %u (X: %f, Y: %f, Z: %f)", _owner->GetGUID().ToString().c_str(), id, x, y, z);
    if (speedXY < 0.01f)
        return;

    float moveTimeHalf = speedZ / Movement::gravity;
    float max_height = -Movement::computeFallElevation(moveTimeHalf, false, -speedZ);

    Movement::MoveSplineInit init(_owner);
    init.MoveTo(x, y, z, false);
    init.SetParabolic(max_height, 0);
    init.SetVelocity(speedXY);
    if (hasOrientation)
        init.SetFacing(o);

    Mutate(new GenericMovementGenerator(std::move(init), EFFECT_MOTION_TYPE, id), MOTION_SLOT_CONTROLLED);
}

void MotionMaster::MoveJumpWithGravity(Position const& pos, float speedXY, float gravity, uint32 id, bool hasOrientation /* = false*/)
{
    LOG_DEBUG("movement.motionmaster", "MotionMaster::MoveJump: '%s', jumps to point Id: %u (%s)", _owner->GetGUID().ToString().c_str(), id, pos.ToString().c_str());
    if (speedXY < 0.01f)
        return;

    Movement::MoveSplineInit init(_owner);
    init.MoveTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), false);
    init.SetParabolicVerticalAcceleration(gravity, 0);
    init.SetVelocity(speedXY);
    if (hasOrientation)
        init.SetFacing(pos.GetOrientation());

    Mutate(new GenericMovementGenerator(std::move(init), EFFECT_MOTION_TYPE, id), MOTION_SLOT_CONTROLLED);
}

void MotionMaster::MoveCirclePath(float x, float y, float z, float radius, bool clockwise, uint8 stepCount, float velocity /*= 0.f*/)
{
    float step = 2 * float(M_PI) / stepCount * (clockwise ? -1.0f : 1.0f);
    Position const& pos = { x, y, z, 0.0f };
    float angle = pos.GetAngle(_owner->GetPositionX(), _owner->GetPositionY());

    Movement::MoveSplineInit init(_owner);

    // add the owner's current position as starting point as it gets removed after entering the cycle
    init.Path().push_back(G3D::Vector3(_owner->GetPositionX(), _owner->GetPositionY(), _owner->GetPositionZ()));

    for (uint8 i = 0; i < stepCount; angle += step, ++i)
    {
        G3D::Vector3 point;
        point.x = x + radius * cosf(angle);
        point.y = y + radius * sinf(angle);

        if (_owner->IsFlying())
            point.z = z;
        else
            point.z = _owner->GetMapHeight(point.x, point.y, z) + _owner->GetHoverOffset();

        init.Path().push_back(point);
    }

    if (_owner->IsFlying())
    {
        init.SetFly();
        init.SetCyclic();
        init.SetAnimation(AnimationTier::Hover);
        init.SetUncompressed();
    }
    else
    {
        init.SetWalk(true);
        init.SetCyclic();
    }

    init.SetSmooth();
    if (velocity > 0.f)
        init.SetVelocity(velocity);

    Mutate(new GenericMovementGenerator(std::move(init), CYCLIC_SPLINE_MOTION_TYPE, 0), MOTION_SLOT_ACTIVE);
}

void MotionMaster::MoveCyclicPath(Position const* pathPoints, size_t pathSize, bool walk /*= false*/, bool fly /*= false*/, float velocity /*= 0.0f*/)
{
    Movement::MoveSplineInit init(_owner);
    Movement::PointsArray path;
    path.reserve(pathSize);
    std::transform(pathPoints, pathPoints + pathSize, std::back_inserter(path), [](Position const& point)
    {
        return G3D::Vector3(point.GetPositionX(), point.GetPositionY(), point.GetPositionZ());
    });

    if (fly)
    {
        init.SetFly();
        init.SetUncompressed();
    }

    if (velocity > 0.0f)
        init.SetVelocity(velocity);

    init.MovebyPath(path);
    init.SetSmooth();
    init.SetWalk(walk);
    init.SetCyclic();
    Mutate(new GenericMovementGenerator(std::move(init), CYCLIC_SPLINE_MOTION_TYPE, 0), MOTION_SLOT_IDLE);
}

void MotionMaster::MoveCyclicPath(uint32 pathId)
{
    if (WaypointPath const* splinePath = sWaypointMgr->GetPath(pathId))
        Mutate(new CyclicMovementGenerator<Creature>(splinePath), MOTION_SLOT_IDLE);
}

void MotionMaster::MoveSmoothPath(uint32 pointId, Position const* pathPoints, size_t pathSize, bool walk /*= false*/, bool fly /*= false*/, float velocity /*= 0.0f*/)
{
    Movement::MoveSplineInit init(_owner);
    Movement::PointsArray path;
    path.reserve(pathSize);
    std::transform(pathPoints, pathPoints + pathSize, std::back_inserter(path), [](Position const& point)
    {
        return G3D::Vector3(point.GetPositionX(), point.GetPositionY(), point.GetPositionZ());
    });

    if (fly)
    {
        init.SetFly();
        init.SetSmooth();
        init.SetUncompressed();
    }
    else
    {
        if (pathSize > 1)
        {
            G3D::Vector3 middle = (path[0] + path[pathSize]) / 2.f;
            for (uint32 i = 1; i < pathSize; ++i)
            {
                G3D::Vector3 delta = middle - path[i];
                if (delta.x > 400 || delta.y > 400 || delta.z > 400)
                {
                    init.SetUncompressed();
                    break;
                }
            }
        }
    }

    if (velocity > 0.0f)
        init.SetVelocity(velocity);

    init.MovebyPath(path);
    init.SetWalk(walk);

    // This code is not correct
    // GenericMovementGenerator does not affect UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE
    // need to call PointMovementGenerator with various pointIds
    Mutate(new GenericMovementGenerator(std::move(init), EFFECT_MOTION_TYPE, pointId), MOTION_SLOT_ACTIVE);
}

void MotionMaster::MoveAlongSplineChain(uint32 pointId, uint16 dbChainId, bool walk)
{
    Creature* owner = _owner->ToCreature();
    if (!owner)
    {
        LOG_ERROR("movement.motionmaster", "MotionMaster::MoveAlongSplineChain: '%s', tried to walk along DB spline chain. Ignoring.", _owner->GetGUID().ToString().c_str());
        return;
    }
    std::vector<SplineChainLink> const* chain = sScriptSystemMgr->GetSplineChain(owner, dbChainId);
    if (!chain)
    {
        LOG_ERROR("movement.motionmaster", "MotionMaster::MoveAlongSplineChain: '%s', tried to walk along non-existing spline chain with DB Id: %u.", _owner->GetGUID().ToString().c_str(), dbChainId);
        return;
    }
    MoveAlongSplineChain(pointId, *chain, walk);
}

void MotionMaster::MoveAlongSplineChain(uint32 pointId, std::vector<SplineChainLink> const& chain, bool walk)
{
    Mutate(new SplineChainMovementGenerator(pointId, chain, walk), MOTION_SLOT_ACTIVE);
}

void MotionMaster::ResumeSplineChain(SplineChainResumeInfo const& info)
{
    if (info.Empty())
    {
        LOG_ERROR("movement.motionmaster", "MotionMaster::ResumeSplineChain: '%s', tried to resume a spline chain from empty info.", _owner->GetGUID().ToString().c_str());
        return;
    }
    Mutate(new SplineChainMovementGenerator(info), MOTION_SLOT_ACTIVE);
}

void MotionMaster::MoveFall(uint32 id /*=0*/)
{
    // use larger distance for vmap height search than in most other cases
    float tz = _owner->GetMapHeight(_owner->GetPositionX(), _owner->GetPositionY(), _owner->GetPositionZ(), true, MAX_FALL_DISTANCE);
    if (tz <= INVALID_HEIGHT)
    {
        LOG_DEBUG("movement.motionmaster", "MotionMaster::MoveFall: '%s', unable to retrieve a proper height at map Id: %u (X: %f, Y: %f, Z: %f)",
            _owner->GetGUID().ToString().c_str(), _owner->GetMap()->GetId(), _owner->GetPositionX(), _owner->GetPositionY(), _owner->GetPositionZ());
        return;
    }

    // Abort too if the ground is very near
    if (std::fabs(_owner->GetPositionZ() - tz) < 0.1f)
        return;

    // rooted units don't move (also setting falling+root flag causes client freezes)
    if (_owner->HasUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED))
        return;

    _owner->AddUnitMovementFlag(MOVEMENTFLAG_FALLING);
    _owner->m_movementInfo.SetFallTime(0);

    if (_owner->GetTypeId() == TYPEID_PLAYER)
        _owner->SetFall(true);

    Movement::MoveSplineInit init(_owner);
    init.MoveTo(_owner->GetPositionX(), _owner->GetPositionY(), tz + _owner->GetHoverOffset(), false);
    init.SetFall();
    Mutate(new GenericMovementGenerator(std::move(init), EFFECT_MOTION_TYPE, id), MOTION_SLOT_CONTROLLED);
}

void MotionMaster::MoveFall(uint32 id /*=0*/)
{
    // use larger distance for vmap height search than in most other cases
    float tz = _owner->GetMapHeight(_owner->GetPositionX(), _owner->GetPositionY(), _owner->GetPositionZ(), true, MAX_FALL_DISTANCE);
    if (tz <= INVALID_HEIGHT)
    {
        LOG_DEBUG("movement.motionmaster", "MotionMaster::MoveFall: '%s', unable to retrieve a proper height at map Id: %u (X: %f, Y: %f, Z: %f)", _owner->GetGUID().ToString().c_str(),
            _owner->GetMap()->GetId(), _owner->GetPositionX(), _owner->GetPositionY(), _owner->GetPositionZ());
        return;
    }

    // Abort too if the ground is very near
    if (std::fabs(_owner->GetPositionZ() - tz) < 0.1f)
        return;

    // rooted units don't move (also setting falling+root flag causes client freezes)
    if (_owner->HasUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED))
        return;

    _owner->AddUnitMovementFlag(MOVEMENTFLAG_FALLING);
    _owner->m_movementInfo.SetFallTime(0);

    // We run spline movement for players, this is not a blizzlike
    if (_owner->GetTypeId() == TYPEID_PLAYER)
    {
        _owner->ToPlayer()->SetFallInformation(0, _owner->GetPositionZ());
    }

    if (_owner->GetTypeId() == TYPEID_PLAYER)
        _owner->SetFall(true);

    Movement::MoveSplineInit init(_owner);
    init.MoveTo(_owner->GetPositionX(), _owner->GetPositionY(), tz + _owner->GetHoverOffset(), false);
    init.SetFall();
    Mutate(new GenericMovementGenerator(std::move(init), EFFECT_MOTION_TYPE, id), MOTION_SLOT_CONTROLLED);
}

void MotionMaster::MoveSeekAssistance(float x, float y, float z)
{
    if (Creature* creature = _owner->ToCreature())
    {
        LOG_DEBUG("movement.motionmaster", "MotionMaster::MoveSeekAssistance: '%s', seeks assistance (X: %f, Y: %f, Z: %f)", creature->GetGUID().ToString().c_str(), x, y, z);
        creature->AttackStop();
        creature->CastStop();
        creature->ResetSpellFocusInfo();
        creature->SetReactState(REACT_PASSIVE);
        Mutate(new AssistanceMovementGenerator(x, y, z), MOTION_SLOT_ACTIVE);
    }
    else
        LOG_ERROR("movement.motionmaster", "MotionMaster::MoveSeekAssistance: '%s', attempted to seek assistance.", _owner->GetGUID().ToString().c_str());
}

void MotionMaster::MoveSeekAssistanceDistract(uint32 time)
{
    if (_owner->GetTypeId() == TYPEID_UNIT)
    {
        LOG_DEBUG("movement.motionmaster", "MotionMaster::MoveSeekAssistanceDistract: '%s', is distracted after assistance call (Time: %u)", _owner->GetGUID().ToString().c_str(), time);
        Mutate(new AssistanceDistractMovementGenerator(time), MOTION_SLOT_ACTIVE);
    }
    else
        LOG_ERROR("movement.motionmaster", "MotionMaster::MoveSeekAssistanceDistract: '%s', attempted to call distract assistance.", _owner->GetGUID().ToString().c_str());
}

void MotionMaster::MoveTaxiFlight(uint32 path, uint32 pathnode)
{
    if (_owner->GetTypeId() == TYPEID_PLAYER)
    {
        if (path < sTaxiPathNodesByPath.size())
        {
            LOG_DEBUG("movement.motionmaster", "MotionMaster::MoveTaxiFlight: '%s', taxi to path Id: %u (node %u)", _owner->GetGUID().ToString().c_str(), path, pathnode);
            FlightPathMovementGenerator* movement = new FlightPathMovementGenerator(pathnode);
            movement->LoadPath(_owner->ToPlayer());
            Mutate(movement, MOTION_SLOT_CONTROLLED);
        }
        else
            LOG_ERROR("movement.motionmaster", "MotionMaster::MoveTaxiFlight: '%s', attempted taxi to non-existing path Id: %u (node: %u)", _owner->GetGUID().ToString().c_str(), path, pathnode);
    }
    else
        LOG_ERROR("movement.motionmaster", "MotionMaster::MoveTaxiFlight: '%s', attempted taxi to path Id: %u (node: %u)", _owner->GetGUID().ToString().c_str(), path, pathnode);
}

void MotionMaster::MoveDistract(uint32 timer)
{
    if (Impl[MOTION_SLOT_CONTROLLED])
        return;

    if (_owner->GetTypeId() == TYPEID_PLAYER)
        LOG_DEBUG("movement.motionmaster", "MotionMaster::MoveDistract: '%s', distracted (timer: %u)", _owner->GetGUID().ToString().c_str(), timer);
    else
        LOG_DEBUG("movement.motionmaster", "MotionMaster::MoveDistract: '%s', distracted (timer: %u)", _owner->GetGUID().ToString().c_str(), timer);

    Mutate(new DistractMovementGenerator(timer), MOTION_SLOT_CONTROLLED);
}

void MotionMaster::MovePath(uint32 pathId, bool repeatable)
{
    if (!pathId)
        return;

    LOG_DEBUG("movement.motionmaster", "MotionMaster::MovePath: '%s', starts moving over path Id: %u (repeatable: %s)", _owner->GetGUID().ToString().c_str(), pathId, repeatable ? "YES" : "NO");
    Mutate(new WaypointMovementGenerator<Creature>(pathId, repeatable), MOTION_SLOT_IDLE);
}

void MotionMaster::MovePath(WaypointPath& path, bool repeatable)
{
    LOG_DEBUG("movement.motionmaster", "MotionMaster::MovePath: '%s', starts moving over path Id: %u (repeatable: %s)", _owner->GetGUID().ToString().c_str(), path.Id, repeatable ? "YES" : "NO");
    Mutate(new WaypointMovementGenerator<Creature>(path, repeatable), MOTION_SLOT_IDLE);
}

void MotionMaster::MoveRotate(uint32 time, RotateDirection direction)
{
    if (!time)
        return;

    LOG_DEBUG("movement.motionmaster", "MotionMaster::MoveRotate: '%s', starts rotate (time: %u, direction: %u)", _owner->GetGUID().ToString().c_str(), time, direction);
    Mutate(new RotateMovementGenerator(time, direction), MOTION_SLOT_ACTIVE);
}

#ifdef MOD_PLAYERBOTS
void MotionMaster::MoveKnockbackFromForPlayer(float srcX, float srcY, float speedXY, float speedZ)
{
    if (speedXY <= 0.1f)
        return;

    Position dest = _owner->GetPosition();
    float moveTimeHalf = speedZ / Movement::gravity;
    float dist = 2 * moveTimeHalf * speedXY;
    float max_height = -Movement::computeFallElevation(moveTimeHalf, false, -speedZ);

    // Use a mmap raycast to get a valid destination.
    _owner->MovePositionToFirstCollision(dest, dist, _owner->GetRelativeAngle(srcX, srcY) + float(M_PI));

    Movement::MoveSplineInit init(_owner);
    init.MoveTo(dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ());
    init.SetParabolic(max_height, 0);
    init.SetOrientationFixed(true);
    init.SetVelocity(speedXY);
    init.Launch();

    Mutate(new GenericMovementGenerator(std::move(init), EFFECT_MOTION_TYPE, 0), MOTION_SLOT_CONTROLLED);
}

// Similar to MovePoint except setting orientationInversed
void MotionMaster::MovePointBackwards(uint32 id, float x, float y, float z, bool generatePath, bool forceDestination, MovementSlot slot, float orientation /* = 0.0f*/)
{
    if (_owner->HasFlag(UNIT_FLAG_DISABLE_MOVE))
        return;

    if (_owner->IsPlayer())
    {
        LOG_DEBUG("movement.motionmaster", "Player ({}) targeted point (Id: {} X: {} Y: {} Z: {})", _owner->GetGUID().ToString(), id, x, y, z);
        Mutate(new PointMovementGenerator<Player>(id, x, y, z, generatePath, 0.0f, true), slot);
    }
    else
    {
        LOG_DEBUG("movement.motionmaster", "Creature ({}) targeted point (ID: {} X: {} Y: {} Z: {})", _owner->GetGUID().ToString(), id, x, y, z);
        Mutate(new PointMovementGenerator<Creature>(id, x, y, z, generatePath, 0.0f, true), slot);
    }
}

#endif

void MotionMaster::LaunchMoveSpline(Movement::MoveSplineInit&& init, uint32 id/*= 0*/, MovementSlot slot/*= MOTION_SLOT_ACTIVE*/, MovementGeneratorType type/*= EFFECT_MOTION_TYPE*/)
{
    if (IsInvalidMovementGeneratorType(type))
    {
        LOG_DEBUG("movement.motionmaster", "MotionMaster::LaunchMoveSpline: '%s', tried to launch a spline with an invalid MovementGeneratorType: %u (Id: %u, Slot: %u)", _owner->GetGUID().ToString().c_str(), type, id, slot);
        return;
    }

    LOG_DEBUG("movement.motionmaster", "MotionMaster::LaunchMoveSpline: '%s', initiates spline Id: %u (Type: %u, Slot: %u)", _owner->GetGUID().ToString().c_str(), id, type, slot);
    Mutate(new GenericMovementGenerator(std::move(init), type, id), slot);
}

/******************** Private methods ********************/

void MotionMaster::InitTop()
{
    top()->Initialize(_owner);
    _needInit[_top] = false;
}

void MotionMaster::Mutate(MovementGenerator *m, MovementSlot slot)
{
    if (MovementGenerator* curr = Impl[slot])
    {
        Impl[slot] = nullptr; // in case a new one is generated in this slot during directdelete
        if (_top == slot && (_cleanFlag & MMCF_UPDATE))
            DelayedDelete(curr);
        else
            DirectDelete(curr);
    }
    else if (_top < slot)
    {
        _top = slot;
    }

    Impl[slot] = m;
    if (_top > slot)
        _needInit[slot] = true;
    else
    {
        _needInit[slot] = false;
        m->Initialize(_owner);
    }
}

MovementSlot MotionMaster::GetCurrentSlot() const
{
    if (empty() || (Impl[MOTION_SLOT_IDLE] && !Impl[MOTION_SLOT_ACTIVE]))
        return MOTION_SLOT_IDLE;

    if (Impl[MOTION_SLOT_ACTIVE])
        return MOTION_SLOT_ACTIVE;

    if (Impl[MOTION_SLOT_CONTROLLED])
        return MOTION_SLOT_CONTROLLED;

    return MAX_MOTION_SLOT;
}

MovementGeneratorType MotionMaster::GetCurrentMovementGeneratorType() const
{
    if (empty())
        return IDLE_MOTION_TYPE;

    if (topOrNull())
        return top()->GetMovementGeneratorType();
    else
        return IDLE_MOTION_TYPE;
}

MovementGeneratorType MotionMaster::GetMotionSlotType(MovementSlot slot) const
{
    if (empty() || IsInvalidMovementSlot(slot) || !Impl[slot])
        return MAX_MOTION_TYPE;

    return Impl[slot]->GetMovementGeneratorType();
}

void MotionMaster::propagateSpeedChange()
{
    if (empty())
        return;

    MovementGenerator* movement = top();
    if (!movement)
        return;

    movement->UnitSpeedChanged();
}

bool MotionMaster::GetDestination(float &x, float &y, float &z)
{
    if (_owner->movespline->Finalized())
        return false;

    G3D::Vector3 const& dest = _owner->movespline->FinalDestination();
    x = dest.x;
    y = dest.y;
    z = dest.z;
    return true;
}
