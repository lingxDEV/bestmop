/*
 * Copyright (C) 2012 - 2016 WoWSource
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Creature.h"
#include "CreatureMovement.h"
#include "BattlegroundMgr.h"
#include "CellImpl.h"
#include "Common.h"
#include "CreatureAI.h"
#include "CreatureAISelector.h"
#include "CreatureGroups.h"
#include "DatabaseEnv.h"
#include "Formulas.h"
#include "GameEventMgr.h"
#include "GossipDef.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "GroupMgr.h"
#include "InstanceScript.h"
#include "Log.h"
#include "LootMgr.h"
#include "MapManager.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "OutdoorPvPMgr.h"
#include "Player.h"
#include "PlayerMovement.h"
#include "UnitMovement.h"
#include "ObjectMovement.h"
#include "PoolMgr.h"
#include "QuestDef.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include "TemporarySummon.h"
#include "Util.h"
#include "Vehicle.h"
#include "WaypointMovementGenerator.h"
#include "World.h"
#include "WorldPacket.h"

#include "Transport.h"

    /*** Movement functions - Handled by CreatureMovement. ***/

// Movement information.

/*
void Creature::UpdateMovementFlags()
{
    // Do not update movement flags if creature is controlled by a player (charm/vehicle)
    if (m_movedPlayer)
        return;

    // Set the movement flags if the creature is in that mode. (Only fly if actually in air, only swim if in water, etc)
    float ground = GetMap()->GetHeight(GetPositionX(), GetPositionY(), GetPositionZMinusOffset());

    bool isInAir = (G3D::fuzzyGt(GetPositionZMinusOffset(), ground + 0.05f) || G3D::fuzzyLt(GetPositionZMinusOffset(), ground - 0.05f)); // Can be underground too, prevent the falling

    if (GetCreatureTemplate()->InhabitType & INHABIT_AIR && isInAir && !IsFalling())
    {
        if (GetCreatureTemplate()->InhabitType & INHABIT_GROUND)
            SetCanFly(true);
        else
            SetDisableGravity(true);
    }
    else
    {
        SetCanFly(false);
        SetDisableGravity(false);
    }

    if (!isInAir)
        SetFall(false);

    SetSwim(GetCreatureTemplate()->InhabitType & INHABIT_WATER && IsInWater());
}
*/

    /*** Positions functions - Handled by CreatureMovement. ***/

// Positions information.

void Creature::SetPosition(float x, float y, float z, float o)
{
    // Prevent crash when a bad coord is sent by the client.
    if (!WoWSource::IsValidMapCoord(x, y, z, o))
    {
        sLog->outDebug(LOG_FILTER_UNITS, "Creature::SetPosition(%f, %f, %f) .. bad coordinates!", x, y, z);
        return;
    }

    GetMap()->CreatureRelocation(ToCreature(), x, y, z, o);
    if (IsVehicle())
        GetVehicleKit(); // ->RelocatePassengers();
}

void Creature::GetRespawnPosition(float &x, float &y, float &z, float* ori, float* dist) const
{
    if (m_DBTableGuid)
    {
        if (CreatureData const* data = sObjectMgr->GetCreatureData(GetDBTableGUIDLow()))
        {
            x = data->posX;
            y = data->posY;
            z = data->posZ;
            if (ori)
                *ori = data->orientation;
            if (dist)
                *dist = data->spawndist;

            return;
        }
    }

    x = GetPositionX();
    y = GetPositionY();
    z = GetPositionZ();

    if (ori)
        *ori = GetOrientation();

    if (dist)
        *dist = 0;
}
