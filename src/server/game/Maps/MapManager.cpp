/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "MapManager.h"
#include "InstanceSaveMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Transport.h"
#include "GridDefines.h"
#include "MapInstanced.h"
#include "InstanceScript.h"
#include "Config.h"
#include "World.h"
#include "CellImpl.h"
#include "Corpse.h"
#include "ObjectMgr.h"
#include "Language.h"
#include "WorldPacket.h"
#include "Group.h"
#include "Chat.h"

extern GridState* si_GridStates[];                          // debugging code, should be deleted some day

MapManager::MapManager()
{
    i_gridCleanUpDelay = sWorld->getIntConfig(CONFIG_INTERVAL_GRIDCLEAN);
    i_timer.SetInterval(sWorld->getIntConfig(CONFIG_INTERVAL_MAPUPDATE));
}

MapManager::~MapManager()
{
}

void MapManager::Initialize()
{
    Map::InitStateMachine();

    // debugging code, should be deleted some day
    {
        for (uint8 i = 0; i < MAX_GRID_STATE; ++i)
             i_GridStates[i] = si_GridStates[i];

        i_GridStateErrorCount = 0;
    }
    int num_threads(sWorld->getIntConfig(CONFIG_NUMTHREADS));
    // Start mtmaps if needed.
    if (num_threads > 0 && m_updater.activate(num_threads) == -1)
        abort();
}

void MapManager::InitializeVisibilityDistanceInfo()
{
    for (MapMapType::iterator iter=i_maps.begin(); iter != i_maps.end(); ++iter)
        (*iter).second->InitVisibilityDistance();
}

// debugging code, should be deleted some day
void MapManager::checkAndCorrectGridStatesArray()
{
    bool ok = true;
    for (int i=0; i<MAX_GRID_STATE; i++)
    {
        if (i_GridStates[i] != si_GridStates[i])
        {
            sLog->outError(LOG_FILTER_MAPS, "MapManager::checkGridStates(), GridState: si_GridStates is currupt !!!");
            ok = false;
            si_GridStates[i] = i_GridStates[i];
        }
        #ifdef TRINITY_DEBUG
        // inner class checking only when compiled with debug
        if (!si_GridStates[i]->checkMagic())
        {
            ok = false;
            si_GridStates[i]->setMagic();
        }
        #endif
    }
    if (!ok)
        ++i_GridStateErrorCount;
}

Map* MapManager::CreateBaseMap(uint32 id)
{
    Map* map = FindBaseMap(id);

    if (map == NULL)
    {
        TRINITY_GUARD(ACE_Thread_Mutex, Lock);

        MapEntry const* entry = sMapStore.LookupEntry(id);
        ASSERT(entry);

        if (entry->Instanceable())
            map = new MapInstanced(id, i_gridCleanUpDelay);
        else
        {
            map = new Map(id, i_gridCleanUpDelay, 0, REGULAR_DIFFICULTY);
            map->LoadRespawnTimes();
        }

        i_maps[id] = map;
    }

    ASSERT(map);
    return map;
}

Map* MapManager::FindBaseNonInstanceMap(uint32 mapId) const
{
    Map* map = FindBaseMap(mapId);
    if (map && map->Instanceable())
        return NULL;
    return map;
}

Map* MapManager::CreateMap(uint32 id, Player* player)
{
    Map* m = CreateBaseMap(id);

    if (m && m->Instanceable())
        m = ((MapInstanced*)m)->CreateInstanceForPlayer(id, player);

    return m;
}

Map* MapManager::FindMap(uint32 mapid, uint32 instanceId) const
{
    Map* map = FindBaseMap(mapid);
    if (!map)
        return NULL;

    if (!map->Instanceable())
        return instanceId == 0 ? map : NULL;

    return ((MapInstanced*)map)->FindInstanceMap(instanceId);
}

bool MapManager::CanPlayerEnter(uint32 mapid, Player* player, bool loginCheck)
{
    MapEntry const* entry = sMapStore.LookupEntry(mapid);
    if (!entry)
       return false;

    // Molten Front
    // That's not an instance, so check it here
    if (entry->MapID == 861)
        if (player->getLevel() < 85)
            return false;

    if (!entry->IsDungeon())
        return true;

    InstanceTemplate const* instance = sObjectMgr->GetInstanceTemplate(mapid);
    if (!instance)
        return false;

    Difficulty targetDifficulty = player->GetDifficulty(entry->IsRaid());
	switch (entry->MapID)
	{
	case 169:
	case 409:
	case 469:
	case 531:
		targetDifficulty = RAID_DIFFICULTY_40MAN;
		break;
	}
    // The player has normal / heroic difficulty active and tries to enter into instance which doesn't have a normal / heroic mode.
    MapDifficulty const* mapDiff = GetMapDifficultyData(entry->MapID, targetDifficulty);
    if (!mapDiff)
    {
        // Send aborted message for dungeons / raids.
        if (entry->IsNonRaidDungeon())
        {
            player->SendTransferAborted(mapid, TRANSFER_ABORT_DIFFICULTY, player->GetDungeonDifficulty());
            return false;
        }
        else // mapDiff = GetDownscaledMapDifficultyData(entry->MapID, targetDifficulty); // Attempt to downscale.
        {
            player->SendTransferAborted(mapid, TRANSFER_ABORT_DIFFICULTY, player->GetRaidDifficulty());
            return false;
        }
    }

    // Bypass checks for GMs
    if (player->isGameMaster())
        return true;

    char const* mapName = entry->name;

    if (!player->isAlive())
    {
        if (Corpse* corpse = player->GetCorpse())
        {
            // let enter in ghost mode in instance that connected to inner instance with corpse
            uint32 corpseMap = corpse->GetMapId();
            do
            {
                if (corpseMap == mapid)
                    break;

                InstanceTemplate const* corpseInstance = sObjectMgr->GetInstanceTemplate(corpseMap);
                corpseMap = corpseInstance ? corpseInstance->Parent : 0;
            }
            while (corpseMap);

            if (!corpseMap)
            {
                WorldPacket data(SMSG_CORPSE_NOT_IN_INSTANCE);
                player->GetSession()->SendPacket(&data);
                sLog->outDebug(LOG_FILTER_MAPS, "MAP: Player '%s' does not have a corpse in instance '%s' and cannot enter.", player->GetName(), mapName);
                return false;
            }
            sLog->outDebug(LOG_FILTER_MAPS, "MAP: Player '%s' has corpse in instance '%s' and can enter.", player->GetName(), mapName);
            player->ResurrectPlayer(0.5f, false);
            player->SpawnCorpseBones();
        }
        else
            sLog->outDebug(LOG_FILTER_MAPS, "Map::CanPlayerEnter - player '%s' is dead but does not have a corpse!", player->GetName());
    }
    
    Group* group = player->GetGroup();
    if (entry->IsRaid() && entry->Expansion() >= EXP_PANDARIA)
    {
        // Can only enter in a raid group.
        if ((!group || !group->isRaidGroup()) && !sWorld->getBoolConfig(CONFIG_INSTANCE_IGNORE_RAID))
        {
            // Can also use ChatHandler(player).PSendSysMessage(player->GetSession()->GetTrinityString(LANG_INSTANCE_RAID_GROUP_ONLY), mapName);. Opcode better.
            player->SendTransferAborted(entry->MapID, TRANSFER_ABORT_NEED_GROUP);
            sLog->outDebug(LOG_FILTER_MAPS, "MAP: Player '%s' must be in a raid group to enter instance '%s'", player->GetName(), mapName);
            return false;
        }
    }

    // If the player or group leader defeats any bosses on Normal and goes out, switches diff and tries to enter on Heroic difficulty, he can't. Or the other way around.
    // He can do it only by using Dynamic Difficulty, inside the instance.
    uint32 boundDifficultyToCheck = 0;
    switch (targetDifficulty)
    {
        case RAID_DIFFICULTY_10MAN_NORMAL:
            boundDifficultyToCheck = RAID_DIFFICULTY_10MAN_HEROIC;
            break;

        case RAID_DIFFICULTY_25MAN_NORMAL:
            boundDifficultyToCheck = RAID_DIFFICULTY_25MAN_HEROIC;
            break;

        case RAID_DIFFICULTY_10MAN_HEROIC:
            boundDifficultyToCheck = RAID_DIFFICULTY_10MAN_NORMAL;
            break;

        case RAID_DIFFICULTY_25MAN_HEROIC:
            boundDifficultyToCheck = RAID_DIFFICULTY_25MAN_NORMAL;
            break;

        default: break;
    }

    // Get instance where the player's group is bound & it's map.
    if (group)
    {
        // Check the map entrance and players inside.
        InstanceGroupBind* boundInstance = group->GetBoundInstance(entry);
        if (boundInstance && boundInstance->save)
            if (Map* boundMap = sMapMgr->FindMap(mapid, boundInstance->save->GetInstanceId()))
                if (!loginCheck && !boundMap->CanEnter(player))
                    return false;

        // Player permanently bound to different instance than group leader one.
        if (group->GetLeaderGUID())
        if (Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID()))
        {
            InstancePlayerBind* leaderBoundInstance = leader->GetBoundInstance(mapid, leader->GetDifficulty(entry->IsRaid()));
            InstancePlayerBind* playerBoundInstance = player->GetBoundInstance(mapid, player->GetDifficulty(entry->IsRaid())); // Player inherits leader difficulty.

            // The leader / players cannot enter a raid if a boss is killed and they used Dynamic Difficulty for it, unless they use it from inside.
            if (leader->GetBoundInstance(mapid, Difficulty(boundDifficultyToCheck)) && leader->HasDynamicDifficultyMap(mapid))
            {
                player->SendTransferAborted(entry->MapID, TRANSFER_ABORT_SOLO_PLAYER_SWITCH_DIFFICULTY);
                return false;
            }

            // The leader and the player are both bound to an instance, check if it's the same.
            if (leaderBoundInstance && playerBoundInstance)
            {
                if (playerBoundInstance->perm && playerBoundInstance->save && leaderBoundInstance->perm && leaderBoundInstance->save)
                {
                    // Different save defeated encounters. If the player has more, error. Else he inherits them on entrance.
                    if (playerBoundInstance->save->GetEncounterMask() > leaderBoundInstance->save->GetEncounterMask())
                    {
                        player->SendTransferAborted(entry->MapID, TRANSFER_ABORT_ALREADY_COMPLETED_ENCOUNTER);
                        return false;
                    }

                    // Different save instance id's.
                    if (playerBoundInstance->save->GetInstanceId() != leaderBoundInstance->save->GetInstanceId())
                    {
                        // For Normal raids, if the leader has at least the same number of defeated encounters, update the player bind to his and let him enter.
                        if (leader->GetDifficulty(entry->IsRaid()) == RAID_DIFFICULTY_10MAN_NORMAL || leader->GetDifficulty(entry->IsRaid()) == RAID_DIFFICULTY_25MAN_NORMAL)
                        {
                            player->UnbindInstance(mapid, leader->GetDifficulty(entry->IsRaid()), false);
                            player->BindToInstance(leaderBoundInstance->save, leaderBoundInstance->perm, false);
                            playerBoundInstance = player->GetBoundInstance(mapid, player->GetDifficulty(entry->IsRaid())); // Player inherits leader difficulty.
                        }
                        else // For Heroic raids, the player cannot enter.
                        {
                            player->SendTransferAborted(entry->MapID, TRANSFER_ABORT_LOCKED_TO_DIFFERENT_INSTANCE);
                            return false;
                        }
                    }
                }
            }

            // The player is bound to an instance to which the leader is not (reverse doesn't count as the player gets the leader bind on entrance).
            if (!leaderBoundInstance && playerBoundInstance)
            {
                player->SendTransferAborted(entry->MapID, TRANSFER_ABORT_ALREADY_COMPLETED_ENCOUNTER);
                return false;
            }
		}

        // Group check for max players in instance.
        if (!group->CanEnterInInstance())
        {
            player->SendTransferAborted(entry->MapID, TRANSFER_ABORT_MAX_PLAYERS);
            return false;
        }
    }
    else
    {
        // The leader / players cannot enter a raid if a boss is killed and they used Dynamic Difficulty for it, unless they use it from inside.
        if (player->GetBoundInstance(mapid, Difficulty(boundDifficultyToCheck)) && player->HasDynamicDifficultyMap(mapid))
        {
            player->SendTransferAborted(entry->MapID, TRANSFER_ABORT_SOLO_PLAYER_SWITCH_DIFFICULTY);
            return false;
        }
    }

    // As of Patch 5.4.7, players are limited to entering 10 instances per hour (modifiable by config).
    if (entry->IsDungeon() && (!player->GetGroup() || (player->GetGroup() && !player->GetGroup()->isLFGGroup())))
    {
        uint32 instanceIdToCheck = 0;
        if (InstanceSave* save = player->GetInstanceSave(mapid, entry->IsRaid()))
            instanceIdToCheck = save->GetInstanceId();

        // InstanceId can never be 0 - will not be found.
        if (!player->CheckInstanceCount(instanceIdToCheck) && !player->isDead())
        {
            player->SendTransferAborted(mapid, TRANSFER_ABORT_TOO_MANY_INSTANCES);
            return false;
        }
    }

    // Other requirements.
    return player->Satisfy(sObjectMgr->GetAccessRequirement(mapid, targetDifficulty), mapid, true);
}

void MapManager::Update(uint32 diff)
{
    i_timer.Update(diff);
    if (!i_timer.Passed())
        return;

    MapMapType::iterator iter = i_maps.begin();
    for (; iter != i_maps.end(); ++iter)
    {
        if (m_updater.activated())
            m_updater.schedule_update(*iter->second, uint32(i_timer.GetCurrent()));
        else
            iter->second->Update(uint32(i_timer.GetCurrent()));
    }
    if (m_updater.activated())
        m_updater.wait();

    for (iter = i_maps.begin(); iter != i_maps.end(); ++iter)
        iter->second->DelayedUpdate(uint32(i_timer.GetCurrent()));

    sObjectAccessor->Update(uint32(i_timer.GetCurrent()));
    for (TransportSet::iterator itr = m_Transports.begin(); itr != m_Transports.end(); ++itr)
        (*itr)->Update(uint32(i_timer.GetCurrent()));

    i_timer.SetCurrent(0);
}

void MapManager::DoDelayedMovesAndRemoves()
{
}

bool MapManager::ExistMapAndVMap(uint32 mapid, float x, float y)
{
    GridCoord p = WoWSource::ComputeGridCoord(x, y);

    int gx=63-p.x_coord;
    int gy=63-p.y_coord;

    return Map::ExistMap(mapid, gx, gy) && Map::ExistVMap(mapid, gx, gy);
}

bool MapManager::IsValidMAP(uint32 mapid, bool startUp)
{
    MapEntry const* mEntry = sMapStore.LookupEntry(mapid);

    if (startUp)
        return mEntry ? true : false;
    else
        return mEntry && (!mEntry->IsDungeon() || sObjectMgr->GetInstanceTemplate(mapid));

    // TODO: add check for battleground template
}

void MapManager::UnloadAll()
{
    if (!m_Transports.empty())
    {
        for (TransportSet::iterator i = m_Transports.begin(); i != m_Transports.end(); ++i)
        {
            if (*i)
            {
                (*i)->RemoveFromWorld();
                delete *i;
            }
        }
    }

    for (MapMapType::iterator iter = i_maps.begin(); iter != i_maps.end();)
    {
        iter->second->UnloadAll();
        delete iter->second;
        i_maps.erase(iter++);
    }

    if (m_updater.activated())
        m_updater.deactivate();

    Map::DeleteStateMachine();
}

uint32 MapManager::GetNumInstances()
{
    TRINITY_GUARD(ACE_Thread_Mutex, Lock);

    uint32 ret = 0;
    for (MapMapType::iterator itr = i_maps.begin(); itr != i_maps.end(); ++itr)
    {
        Map* map = itr->second;
        if (!map->Instanceable())
            continue;
        MapInstanced::InstancedMaps &maps = ((MapInstanced*)map)->GetInstancedMaps();
        for (MapInstanced::InstancedMaps::iterator mitr = maps.begin(); mitr != maps.end(); ++mitr)
            if (mitr->second->IsDungeon()) ret++;
    }
    return ret;
}

uint32 MapManager::GetNumPlayersInInstances()
{
    TRINITY_GUARD(ACE_Thread_Mutex, Lock);

    uint32 ret = 0;
    for (MapMapType::iterator itr = i_maps.begin(); itr != i_maps.end(); ++itr)
    {
        Map* map = itr->second;
        if (!map->Instanceable())
            continue;
        MapInstanced::InstancedMaps &maps = ((MapInstanced*)map)->GetInstancedMaps();
        for (MapInstanced::InstancedMaps::iterator mitr = maps.begin(); mitr != maps.end(); ++mitr)
            if (mitr->second->IsDungeon())
                ret += ((InstanceMap*)mitr->second)->GetPlayers().getSize();
    }
    return ret;
}

void MapManager::InitInstanceIds()
{
    _nextInstanceId = 1;

    QueryResult result = CharacterDatabase.Query("SELECT MAX(id) FROM instance");
    if (result)
    {
        uint32 maxId = (*result)[0].GetUInt32();

        // Resize to multiples of 32 (vector<bool> allocates memory the same way)
        _instanceIds.resize((maxId / 32) * 32 + (maxId % 32 > 0 ? 32 : 0));
    }
}

void MapManager::RegisterInstanceId(uint32 instanceId)
{
    // Allocation and sizing was done in InitInstanceIds()
    _instanceIds[instanceId] = true;
}

uint32 MapManager::GenerateInstanceId()
{
    uint32 newInstanceId = _nextInstanceId;

    // Find the lowest available id starting from the current NextInstanceId (which should be the lowest according to the logic in FreeInstanceId()
    for (uint32 i = ++_nextInstanceId; i < 0xFFFFFFFF; ++i)
    {
        if ((i < _instanceIds.size() && !_instanceIds[i]) || i >= _instanceIds.size())
        {
            _nextInstanceId = i;
            break;
        }
    }

    if (newInstanceId == _nextInstanceId)
    {
        sLog->outError(LOG_FILTER_MAPS, "Instance ID overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }

    // Allocate space if necessary
    if (newInstanceId >= uint32(_instanceIds.size()))
    {
        // Due to the odd memory allocation behavior of vector<bool> we match size to capacity before triggering a new allocation
        if (_instanceIds.size() < _instanceIds.capacity())
        {
            _instanceIds.resize(_instanceIds.capacity());
        }
        else
            _instanceIds.resize((newInstanceId / 32) * 32 + (newInstanceId % 32 > 0 ? 32 : 0));
    }

    _instanceIds[newInstanceId] = true;

    return newInstanceId;
}

void MapManager::FreeInstanceId(uint32 instanceId)
{
    // If freed instance id is lower than the next id available for new instances, use the freed one instead
    if (instanceId < _nextInstanceId)
        SetNextInstanceId(instanceId);

    _instanceIds[instanceId] = false;
}
