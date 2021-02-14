/*
 *  RoG_WoW Source 2012-2017 <http://wow.rog.snet>
 *  Do NOT share this source Code
 */

#include "AnticheatMgr.h"
#include "AnticheatScripts.h"
#include "MapManager.h"
#include "AccountMgr.h"

#define CLIMB_ANGLE 1.9f

AnticheatMgr::AnticheatMgr()
{
	m_dataDeleteTimer = 0;
}

AnticheatMgr::~AnticheatMgr()
{
    m_Players.clear();
}

void AnticheatMgr::JumpHackDetection(Player* player, MovementInfo movementInfo,uint32 opcode)
{
    if ((sWorld->getIntConfig(CONFIG_ANTICHEAT_DETECTIONS_ENABLED) & JUMP_HACK_DETECTION) == 0)
        return;

    uint32 key = player->GetGUIDLow();

    if (m_Players[key].GetLastOpcode() == CMSG_MOVE_JUMP && opcode == CMSG_MOVE_JUMP)
    {
        BuildReport(player,JUMP_HACK_REPORT);
    }
}

void AnticheatMgr::WalkOnWaterHackDetection(Player* player, MovementInfo movementInfo)
{
    if ((sWorld->getIntConfig(CONFIG_ANTICHEAT_DETECTIONS_ENABLED) & WALK_WATER_HACK_DETECTION) == 0)
        return;

    uint32 key = player->GetGUIDLow();
    if (!m_Players[key].GetLastMovementInfo().HasMovementFlag(MOVEMENTFLAG_WATERWALKING))
        return;

    // if we are a ghost we can walk on water
    if (!player->isAlive())
        return;

    if (player->HasAuraType(SPELL_AURA_FEATHER_FALL) ||
        player->HasAuraType(SPELL_AURA_SAFE_FALL) ||
        player->HasAuraType(SPELL_AURA_WATER_WALK))
        return;

    float x, y, z;
    player->GetPosition(x, y, z);
    float ground_Z = player->GetMap()->GetHeight(x, y, z);
    float z_diff = fabs(ground_Z - z);
	
	if (player->getClass() == CLASS_PRIEST && z_diff > 0 && z_diff < 10)
		return;
	
    sLog->outInfo(LOG_FILTER_CHARACTER, "AnticheatMgr:: Walk on Water - Hack detected player GUID (low) %u",player->GetGUIDLow());
    BuildReport(player,WALK_WATER_HACK_REPORT);

}

void AnticheatMgr::FlyHackDetection(Player* player, MovementInfo movementInfo)
{
    if ((sWorld->getIntConfig(CONFIG_ANTICHEAT_DETECTIONS_ENABLED) & FLY_HACK_DETECTION) == 0)
        return;

    uint32 key = player->GetGUIDLow();
    if (!m_Players[key].GetLastMovementInfo().HasMovementFlag(MOVEMENTFLAG_FLYING))
        return;

    if (player->HasAuraType(SPELL_AURA_FLY) ||
        player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) ||
        player->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED))
        return;
		
	if (player->isDead() && player->HasAura(55164)) //Swift Spectral Gryphon
		return;

	if (player->HasAura(34480) || player->HasAura(44226))   // Kael'Thas encounters (in The Eye and Magister's Terrace)
		return;

    if (player->HasAura(82724)) //Alakir Eye of the Storm
        return;

    if (player->HasAura(125883)) //Zen Flight (Glyph)  Monk
        return;

    sLog->outInfo(LOG_FILTER_CHARACTER, "AnticheatMgr:: Fly-Hack detected player GUID (low) %u",player->GetGUIDLow());
    BuildReport(player,FLY_HACK_REPORT);
}

void AnticheatMgr::TeleportPlaneHackDetection(Player* player, MovementInfo movementInfo)
{
    if ((sWorld->getIntConfig(CONFIG_ANTICHEAT_DETECTIONS_ENABLED) & TELEPORT_PLANE_HACK_DETECTION) == 0)
        return;

	auto const& data = m_Players[player->GetGUIDLow()];
    uint32 key = player->GetGUIDLow();

	if (movementInfo.GetMovementFlags() != data.GetLastMovementInfo().GetMovementFlags())
		return;

    if (m_Players[key].GetLastMovementInfo().pos.GetPositionZ() != 0 ||
        movementInfo.pos.GetPositionZ() != 0)
        return;

	uint32 distance2D = (uint32)movementInfo.pos.GetExactDist2d(&data.GetLastMovementInfo().pos);
	if (distance2D > sWorld->getIntConfig(CONFIG_ANTICHEAT_TELEPORTHACKDETECTION_DISTANCE) && !player->GetTeleportFlagForAnticheat())
		BuildReport(player, TELEPORT_PLANE_HACK_REPORT);

    if (movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING))
        return;

	player->SetTeleportFlagForAnticheat(false);
		
    //DEAD_FALLING was deprecated
    //if (player->getDeathState() == DEAD_FALLING)
    //    return;
	
    float x, y, z;
    player->GetPosition(x, y, z);
    float ground_Z = player->GetMap()->GetHeight(x, y, z);
    float z_diff = fabs(ground_Z - z);
		
    // we are not really walking there
    if (z_diff > 1.0f)
    {
        //sLog->outInfo(LOG_FILTER_CHARACTER, "AnticheatMgr:: Teleport To Plane - Hack detected player GUID (low) %u",player->GetGUIDLow());
        BuildReport(player,TELEPORT_PLANE_HACK_REPORT);
    }
}

void AnticheatMgr::StartHackDetection(Player* player, MovementInfo movementInfo, uint32 opcode)
{
    if (!sWorld->getBoolConfig(CONFIG_ANTICHEAT_ENABLE))
        return;

	if (player->isGameMaster() || player->GetSession()->GetSecurity() > SEC_PLAYER)
        return;

    uint32 key = player->GetGUIDLow();

	// Deeprun Tram
	if (player->GetMapId() == 369)
		return;

	// Ulduar tram
	Position const& pos = movementInfo.pos;
	if (player->GetMapId() == 603 &&
		pos.GetPositionX() > 2260 && pos.GetPositionX() < 2360 &&
		pos.GetPositionY() > 270 && pos.GetPositionX() < 2590 &&
		pos.GetPositionZ() > 415 && pos.GetPositionZ() < 430)
		return;

	switch (player->GetZoneId())
	{
	case 3430: // Eversong Woods
	case 3433: // Ghostlands
	case 3525: // Bloodmyst Isle
	case 6757: // Timeless Isle
		return;
	default:
		break;
	}

	if (player->HasAura(148537) // Falling Flame
		|| player->HasAura(126389))
		return;

    if (player->isInFlight() || player->GetTransport() || player->GetVehicle())
    {
        m_Players[key].SetLastMovementInfo(movementInfo);
        m_Players[key].SetLastOpcode(opcode);
        return;
    }

    SpeedHackDetection(player,movementInfo);
    FlyHackDetection(player,movementInfo);
    WalkOnWaterHackDetection(player,movementInfo);
    JumpHackDetection(player,movementInfo,opcode);
    TeleportPlaneHackDetection(player, movementInfo);
    ClimbHackDetection(player,movementInfo,opcode);

    m_Players[key].SetLastMovementInfo(movementInfo);
    m_Players[key].SetLastOpcode(opcode);
}

// basic detection
void AnticheatMgr::ClimbHackDetection(Player *player, MovementInfo movementInfo, uint32 opcode)
{
    if ((sWorld->getIntConfig(CONFIG_ANTICHEAT_DETECTIONS_ENABLED) & CLIMB_HACK_DETECTION) == 0)
        return;

    uint32 key = player->GetGUIDLow();

    if (opcode != CMSG_MOVE_HEARTBEAT ||
        m_Players[key].GetLastOpcode() != CMSG_MOVE_HEARTBEAT)
        return;

    // in this case we don't care if they are "legal" flags, they are handled in another parts of the Anticheat Manager.
    if (player->IsInWater() ||
        player->IsFlying() ||
        player->IsFalling())
        return;

    Position playerPos;
    player->GetPosition(&playerPos);

    float deltaZ = fabs(playerPos.GetPositionZ() - movementInfo.pos.GetPositionZ());
    float deltaXY = movementInfo.pos.GetExactDist2d(&playerPos);

	float angle;

#if defined(__WIN__)
	__try
	{
#endif
		angle = Position::NormalizeOrientation(tan(deltaZ / deltaXY));
#if defined(__WIN__)
	}
	__except (EXCEPTION_ILLEGAL_INSTRUCTION)
	{
		return;
	}
#endif

    if (angle > CLIMB_ANGLE)
    {
        //sLog->outInfo(LOG_FILTER_CHARACTER, "AnticheatMgr:: Climb-Hack detected player GUID (low) %u", player->GetGUIDLow());
        BuildReport(player,CLIMB_HACK_REPORT);
    }
}

void AnticheatMgr::SpeedHackDetection(Player* player,MovementInfo movementInfo)
{
    if ((sWorld->getIntConfig(CONFIG_ANTICHEAT_DETECTIONS_ENABLED) & SPEED_HACK_DETECTION) == 0)
        return;

    uint32 key = player->GetGUIDLow();
		
    // We also must check the map because the movementFlag can be modified by the client.
    // If we just check the flag, they could always add that flag and always skip the speed hacking detection.
    // 369 == DEEPRUN TRAM
	// 607 == Strand of The Ancients
    if (m_Players[key].GetLastMovementInfo().HasMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY) && (player->GetMapId() == 369 || player->GetMapId() == 607))
        return;

	if (player->GetSession()->GetSecurity() > 0)
	    return;

	if (!player->isAlive())
	    return;

    uint32 distance2D = (uint32)movementInfo.pos.GetExactDist2d(&m_Players[key].GetLastMovementInfo().pos);

	if (distance2D > 500)
	    return;

	if (distance2D < 100 && player->getClass() == CLASS_PRIEST && player->isInCombat())
	    return;

	if (player->GetAreaId() == 4281 || player->GetAreaId() == 4342 || player->GetAreaId() == 5154 || player->GetAreaId() == 4395 ||
	player->GetAreaId() == 5926 || player->GetAreaId() == 5738 || player->GetAreaId() == 5535 || player->GetAreaId() == 5004 ||
	player->GetAreaId() == 5088 || player->GetAreaId() == 5303 || player->GetAreaId() == 4753 || player->GetAreaId() == 4752)
	    return;

	if (player->HasAura(66601))
	    return;

    if (player->HasAura(125883))
        return;

	if (player->HasAura(66602))
	{
	    player->RemoveAura(66602);
	}

	if (player->HasAura(605))
	    return;

	if (player->HasAura(51690))
	{
	    player->CastSpell(player, 66601, true);
	    return;
	}

    float x, y, z;
    player->GetPosition(x, y, z);
    float ground_Z = player->GetMap()->GetHeight(x, y, z);
    float z_diff = fabs(ground_Z - z);	
	WorldSession* s = player->GetSession();
	if ( player->GetMap()->IsDungeon() || player->GetMap()->IsRaid() || player->GetMap()->IsBattlegroundOrArena())
	    if ((distance2D > 50 && player->GetMapId() == 566) || (distance2D > 40 && player->GetMapId() != 566))
	        if (z_diff > 1.0f && !player->isGameMaster())
				if (!player->HasUnitMovementFlag(MOVEMENTFLAG_FALLING) && player->isAlive() && !player->IsBeingTeleported())
				{
					std::string str = "";
					str = "|[[ANTICHEAT]|cFF00FFFF[|cFF60FF00" + std::string(player->GetName()) + "|cFF00FFFF] found posible speed cheating!";
					WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
					data << str;
					sWorld->SendGlobalGMMessage(&data);
				}

	if ((player->GetMap()->IsDungeon() || player->GetMap()->IsRaid() || player->GetMap()->IsBattlegroundOrArena() || player->GetMapId() == 732 || player->GetMapId() == 861) && (player->HasAura(33943) || player->HasAura(40120)))
	{
	    player->RemoveAura(33943);
		player->RemoveAura(40120);
	}

    uint8 moveType = 0;
	uint32 maxSpeed = 0;

    // we need to know HOW is the player moving
    // TO-DO: Should we check the incoming movement flags?
    if (player->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
    {
        moveType = MOVE_SWIM;
        maxSpeed = 11;
        if (player->HasAura(8326) || player->HasAura(20584)) //Ghost
            maxSpeed += 5;

        if (player->HasAura(86510)) //Epic Swimming Mount
            maxSpeed += 20;

        if (player->HasAura(95664)) //Advanced Swimming Mount
            maxSpeed += 15;

        if (player->HasAura(73701)) //Sea Legs
            maxSpeed += 5;

        if (player->HasAura(98718)) //Subdued Seahorse
            maxSpeed = 41;

        if (player->HasAura(75207)) //Abyssal Seahorse
            maxSpeed = 56;
    }
    else if (player->IsFlying() && player->HasAuraType(SPELL_AURA_MOUNTED))
    {
        moveType = MOVE_FLIGHT;

        if (player->HasSpell(90265)) //master riding
            maxSpeed = 40;
        else if (player->HasSpell(34091)) //artisan riding
            maxSpeed = 33;
        else if (player->HasSpell(34090))
            maxSpeed = 20;

        //extra checks
        if (maxSpeed < 25)
        {
            if (player->HasAura(86459)) //Mount Speed Mod: Standard Flying Mount
                maxSpeed = 36;

            if (player->HasAura(86460)) //Mount Speed Mod: Epic Flying Mount
                maxSpeed = 49;
        }

        if (player->HasAura(32223)) //Crusader Aura
            maxSpeed += 9;

        if (player->HasAura(51983)) //On a Pale Horse Rank 1
            maxSpeed += 5;
        if (player->HasAura(51986)) //On a Pale Horse Rank 2
            maxSpeed += 9;
        
        if (player->HasAura(26023) || player->HasAura(26022)) //Pursuit of Justice
            maxSpeed += 19;
    }
 	else if (player->IsFlying() && !player->HasAuraType(SPELL_AURA_MOUNTED))
	{
	    maxSpeed = 1;

        if (player->HasAura(98619)) //Wings of Flame
            maxSpeed += 11;

        if (player->HasAura(33943)) //Flight Form
            maxSpeed += 20;

        if (player->HasAura(40120)) //Swift Flight Form
            maxSpeed += 35;
			
        if (player->HasAura(59640)) //Underbelly Elixir
            maxSpeed += 20;			
	}
    else if (player->HasAuraType(SPELL_AURA_MOUNTED))
    {
        if (player->HasSpell(33391)) //journeyman riding
            maxSpeed = 25;
        else if (player->HasSpell(33388)) //apprentice riding
            maxSpeed = 21;

        //extra checks
        if (maxSpeed < 17)
        {
            if (player->HasAura(86458)) //Mount Speed Mod: Epic Ground Mount
                maxSpeed = 21;

            if (player->HasAura(86457)) //Mount Speed Mod: Standard Ground Mount
                maxSpeed = 17;
        }

        if (player->HasAura(32223)) //Crusader Aura
            maxSpeed += 5;

        if (player->HasAura(51983)) //On a Pale Horse Rank 1
            maxSpeed += 3;
        if (player->HasAura(51986)) //On a Pale Horse Rank 2
            maxSpeed += 5;
        
        if (player->HasAura(26023) || player->HasAura(26022)) //Pursuit of Justice
            maxSpeed += 5;			
    }
    else if (player->HasUnitMovementFlag(MOVEMENTFLAG_WALKING))
    {
        moveType = MOVE_WALK;
        maxSpeed = 6;
    }
    else
    {
        moveType = MOVE_RUN;
        maxSpeed = 16;
			
		if (z_diff > 1.0f)
		    maxSpeed = 40;
		
        if (player->HasAura(33943)) //Flight Form
            maxSpeed += 20;

        if (player->HasAura(40120)) //Swift Flight Form
            maxSpeed += 33;

        if (player->HasAura(8326) || player->HasAura(20584)) //Ghost
            maxSpeed += 5;

        if (player->HasAura(26023) || player->HasAura(26022)) //Pursuit of Justice
            maxSpeed += 5;

        if (player->HasAura(51721) || player->HasAura(54055)) //Dominion over Acherus
            maxSpeed += 8;

        if (player->HasAura(48265)) //Unholy Presence
            maxSpeed += 2;

        if (player->HasAura(23451) || player->HasAura(23978)) //Speed
            maxSpeed += 10;

        if (player->HasAura(68992)) //Dark flight
            maxSpeed += 4;

        if (player->HasAura(2983) || player->HasAura(1850)) //Sprint, Dash
            maxSpeed += 7;
			
        if (player->HasAura(36554)) //Shadowstep
            maxSpeed += 8;			
    }
    
    if (player->HasUnitMovementFlag(MOVEMENTFLAG_FALLING))
    {
        maxSpeed = 60;
    }
			
    // how many yards the player can do in one sec.

    // how long the player took to move to here.
    uint32 timeDiff = getMSTimeDiff(m_Players[key].GetLastMovementInfo().time,movementInfo.time);
			
    if (!timeDiff)
        timeDiff = 1;
		
    // this is the distance doable by the player in 1 sec, using the time done to move to this point.
    uint32 clientSpeedRate = distance2D * 1000 / timeDiff;

    // we did the (uint32) cast to accept a margin of tolerance
    if (clientSpeedRate > maxSpeed && player->GetSession()->GetLatency() < 500)
    {
		//WorldSession* s = player->GetSession();
	    //s->KickPlayer();
        BuildReport(player,SPEED_HACK_REPORT);
        //sLog->outInfo(LOG_FILTER_CHARACTER, "AnticheatMgr:: Speed-Hack detected player GUID (low) %u",player->GetGUIDLow());
    }				
}

void AnticheatMgr::StartScripts()
{
    new AnticheatScripts();
}

void AnticheatMgr::HandlePlayerLogin(Player* player)
{
    // we must delete this to prevent errors in case of crash
    CharacterDatabase.PExecute("DELETE FROM players_reports_status WHERE guid=%u",player->GetGUIDLow());
    // we initialize the pos of lastMovementPosition var.
    m_Players[player->GetGUIDLow()].SetPosition(player->GetPositionX(),player->GetPositionY(),player->GetPositionZ(),player->GetOrientation());
    QueryResult resultDB = CharacterDatabase.PQuery("SELECT * FROM daily_players_reports WHERE guid=%u;",player->GetGUIDLow());

    if (resultDB)
        m_Players[player->GetGUIDLow()].SetDailyReportState(true);
}

void AnticheatMgr::HandlePlayerLogout(Player* player)
{
    // TO-DO Make a table that stores the cheaters of the day, with more detailed information.

    // We must also delete it at logout to prevent have data of offline players in the db when we query the database (IE: The GM Command)
    CharacterDatabase.PExecute("DELETE FROM players_reports_status WHERE guid=%u",player->GetGUIDLow());
    // Delete not needed data from the memory.
    m_Players.erase(player->GetGUIDLow());
}

void AnticheatMgr::SavePlayerData(Player* player)
{
    CharacterDatabase.PExecute("REPLACE INTO players_reports_status (guid,average,total_reports,speed_reports,fly_reports,jump_reports,waterwalk_reports,teleportplane_reports,climb_reports,creation_time) VALUES (%u,%f,%u,%u,%u,%u,%u,%u,%u,%u);",player->GetGUIDLow(),m_Players[player->GetGUIDLow()].GetAverage(),m_Players[player->GetGUIDLow()].GetTotalReports(), m_Players[player->GetGUIDLow()].GetTypeReports(SPEED_HACK_REPORT),m_Players[player->GetGUIDLow()].GetTypeReports(FLY_HACK_REPORT),m_Players[player->GetGUIDLow()].GetTypeReports(JUMP_HACK_REPORT),m_Players[player->GetGUIDLow()].GetTypeReports(WALK_WATER_HACK_REPORT),m_Players[player->GetGUIDLow()].GetTypeReports(TELEPORT_PLANE_HACK_REPORT),m_Players[player->GetGUIDLow()].GetTypeReports(CLIMB_HACK_REPORT),m_Players[player->GetGUIDLow()].GetCreationTime());
}

uint32 AnticheatMgr::GetTotalReports(uint32 lowGUID)
{
    return m_Players[lowGUID].GetTotalReports();
}

float AnticheatMgr::GetAverage(uint32 lowGUID)
{
    return m_Players[lowGUID].GetAverage();
}

uint32 AnticheatMgr::GetTypeReports(uint32 lowGUID, uint8 type)
{
    return m_Players[lowGUID].GetTypeReports(type);
}

bool AnticheatMgr::MustCheckTempReports(uint8 type)
{
	if (type == JUMP_HACK_REPORT || type == TELEPORT_PLANE_HACK_REPORT)
        return false;

    return true;
}

void AnticheatMgr::BuildReport(Player* player,uint8 reportType)
{
    uint32 key = player->GetGUIDLow();

    if (MustCheckTempReports(reportType))
    {
        uint32 actualTime = getMSTime();

        if (!m_Players[key].GetTempReportsTimer(reportType))
            m_Players[key].SetTempReportsTimer(actualTime,reportType);

        if (getMSTimeDiff(m_Players[key].GetTempReportsTimer(reportType),actualTime) < 3000)
        {
            m_Players[key].SetTempReports(m_Players[key].GetTempReports(reportType)+1,reportType);

            if (m_Players[key].GetTempReports(reportType) < 3)
                return;
        } else
        {
            m_Players[key].SetTempReportsTimer(actualTime,reportType);
            m_Players[key].SetTempReports(1,reportType);
            return;
        }
    }

    // generating creationTime for average calculation
    if (!m_Players[key].GetTotalReports())
        m_Players[key].SetCreationTime(getMSTime());

    // increasing total_reports
    m_Players[key].SetTotalReports(m_Players[key].GetTotalReports()+1);
    // increasing specific cheat report
    m_Players[key].SetTypeReports(reportType,m_Players[key].GetTypeReports(reportType)+1);

    // diff time for average calculation
    uint32 diffTime = getMSTimeDiff(m_Players[key].GetCreationTime(),getMSTime()) / IN_MILLISECONDS;

    if (diffTime > 0)
    {
        // Average == Reports per second
        float average = float(m_Players[key].GetTotalReports()) / float(diffTime);
        m_Players[key].SetAverage(average);
    }

    if (sWorld->getIntConfig(CONFIG_ANTICHEAT_MAX_REPORTS_FOR_DAILY_REPORT) < m_Players[key].GetTotalReports())
    {
        if (!m_Players[key].GetDailyReportState())
        {
            CharacterDatabase.PExecute("REPLACE INTO daily_players_reports (guid,average,total_reports,speed_reports,fly_reports,jump_reports,waterwalk_reports,teleportplane_reports,climb_reports,creation_time) VALUES (%u,%f,%u,%u,%u,%u,%u,%u,%u,%u);",player->GetGUIDLow(),m_Players[player->GetGUIDLow()].GetAverage(),m_Players[player->GetGUIDLow()].GetTotalReports(), m_Players[player->GetGUIDLow()].GetTypeReports(SPEED_HACK_REPORT),m_Players[player->GetGUIDLow()].GetTypeReports(FLY_HACK_REPORT),m_Players[player->GetGUIDLow()].GetTypeReports(JUMP_HACK_REPORT),m_Players[player->GetGUIDLow()].GetTypeReports(WALK_WATER_HACK_REPORT),m_Players[player->GetGUIDLow()].GetTypeReports(TELEPORT_PLANE_HACK_REPORT),m_Players[player->GetGUIDLow()].GetTypeReports(CLIMB_HACK_REPORT),m_Players[player->GetGUIDLow()].GetCreationTime());
            m_Players[key].SetDailyReportState(true);
        }
    }

    if (m_Players[key].GetTotalReports() > sWorld->getIntConfig(CONFIG_ANTICHEAT_REPORTS_INGAME_NOTIFICATION) && m_Players[key].GetAverage() > 0.5f)
    {
        // display warning at the center of the screen, hacky way?
        std::string str = "";
		std::string BanTemp = "0d0h15m0s";
		std::string BanTempString = "15 minutes";
		switch (reportType)
			{
			case 0:
				str = " Speed-Hack!";
				break;
			case 1:
				str = " Fly-Hack!";
				break;
			case 2:
				str = " Walk-Water!";
				break;
			case 3:
				str = " Jump-Hacker!";
				break;
			case 4:
				str = " Teleport-Hack!";
				break;
			case 5:
				str = " Climb-Hack!";
				break;
			default:
				str = " Cheater!";
				break;
			}
		
		if(sWorld->getIntConfig(CONFIG_ANTICHEAT_ACTION) == 2)
		{
			QueryResult resultDB = CharacterDatabase.PQuery("SELECT COUNT(guid) as total FROM character_banned WHERE bannedby='Anticheat' AND guid= '%u'", player->GetGUIDLow());
			if (resultDB)
			{
				Field *fieldsDB = resultDB->Fetch();

				uint32 BanCount = fieldsDB[0].GetUInt32();
				
				if(BanCount == 2)
				{
					BanTemp = "0d1h0m0s";
					BanTempString = "1 hora";
				}else if(BanCount == 3)
				{
					BanTemp = "1d";
					BanTempString = "1 dia";
				}else if(BanCount > 3) 
				{
					BanTemp = "7d";
					BanTempString = "7 dias";
				}
			}
			
			str = str + " - Banido por " + BanTempString; 
				
			sWorld->BanCharacter(player->GetName(), std::string(BanTemp), std::string(str), "Anticheat");
		}
		
		sLog->outInfo(LOG_FILTER_CHARACTER, "AnticheatMgr:: %u player GUID (low) %u",std::string(str),player->GetGUIDLow());
		
		str = " |cFFFF0000[AC]|cFF00FFFF[|cFF60FF00" + std::string(player->GetName()) + "|cFF00FFFF] " + str;
        WorldPacket data(SMSG_NOTIFICATION, (str.size()+1));
        data << str;
        sWorld->SendGlobalGMMessage(&data);
    }
}

void AnticheatMgr::AnticheatGlobalCommand(ChatHandler* handler)
{
    // MySQL will sort all for us, anyway this is not the best way we must only save the anticheat data not whole player's data!.
    sObjectAccessor->SaveAllPlayers();

    QueryResult resultDB = CharacterDatabase.Query("SELECT guid,average,total_reports FROM players_reports_status WHERE total_reports != 0 ORDER BY average ASC LIMIT 3;");
    if (!resultDB)
    {
        handler->PSendSysMessage("No players found.");
        return;
    } else
    {
        handler->SendSysMessage("=============================");
        handler->PSendSysMessage("Players with the lowest averages:");
        do
        {
            Field *fieldsDB = resultDB->Fetch();

            uint32 guid = fieldsDB[0].GetUInt32();
            float average = fieldsDB[1].GetFloat();
            uint32 total_reports = fieldsDB[2].GetUInt32();

            if (Player* player = sObjectMgr->GetPlayerByLowGUID(guid))
                handler->PSendSysMessage("Player: %s Average: %f Total Reports: %u", player->GetName(), average, total_reports);;

        } while (resultDB->NextRow());
    }

    resultDB = CharacterDatabase.Query("SELECT guid,average,total_reports FROM players_reports_status WHERE total_reports != 0 ORDER BY total_reports DESC LIMIT 3;");

    // this should never happen
    if (!resultDB)
    {
        handler->PSendSysMessage("No players found.");
        return;
    } else
    {
        handler->SendSysMessage("=============================");
        handler->PSendSysMessage("Players with the more reports:");
        do
        {
            Field *fieldsDB = resultDB->Fetch();

            uint32 guid = fieldsDB[0].GetUInt32();
            float average = fieldsDB[1].GetFloat();
            uint32 total_reports = fieldsDB[2].GetUInt32();

            if (Player* player = sObjectMgr->GetPlayerByLowGUID(guid))
                handler->PSendSysMessage("Player: %s Average: %f Total Reports: %u", player->GetName(), average, total_reports);

        } while (resultDB->NextRow());
    }
}

void AnticheatMgr::AnticheatDeleteCommand(uint32 guid)
{
    if (!guid)
    {
        for (AnticheatPlayersDataMap::iterator it = m_Players.begin(); it != m_Players.end(); ++it)
        {
            (*it).second.SetTotalReports(0);
            (*it).second.SetAverage(0);
            (*it).second.SetCreationTime(0);
            for (uint8 i = 0; i < MAX_REPORT_TYPES; i++)
            {
                (*it).second.SetTempReports(0,i);
                (*it).second.SetTempReportsTimer(0,i);
                (*it).second.SetTypeReports(i,0);
            }
        }
        CharacterDatabase.PExecute("DELETE FROM players_reports_status;");
    }
    else
    {
        m_Players[guid].SetTotalReports(0);
        m_Players[guid].SetAverage(0);
        m_Players[guid].SetCreationTime(0);
        for (uint8 i = 0; i < MAX_REPORT_TYPES; i++)
        {
            m_Players[guid].SetTempReports(0,i);
            m_Players[guid].SetTempReportsTimer(0,i);
            m_Players[guid].SetTypeReports(i,0);
        }
        CharacterDatabase.PExecute("DELETE FROM players_reports_status WHERE guid=%u;",guid);
    }
}

void AnticheatMgr::ResetDailyReportStates()
{
     for (AnticheatPlayersDataMap::iterator it = m_Players.begin(); it != m_Players.end(); ++it)
         m_Players[(*it).first].SetDailyReportState(false);
}
