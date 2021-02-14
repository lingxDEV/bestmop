/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
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

#include "Common.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include "Battlefield.h"
#include "BattlefieldMgr.h"
#include "Opcodes.h"

//This send to player windows for invite player to join the war
//Param1:(m_Guid) the BattleId of Bf
//Param2:(ZoneId) the zone where the battle is (4197 for wg)
//Param3:(time) Time in second that the player have for accept
void WorldSession::SendBfInvitePlayerToWar(uint64 guid, uint32 zoneId, uint32 pTime)
{
    ObjectGuid guidBytes = guid;
    WorldPacket data(SMSG_BATTLEFIELD_MGR_ENTRY_INVITE);

    uint8 bitOrder[8] = {6, 5, 2, 4, 3, 0, 7, 1};
    data.WriteBitInOrder(guidBytes, bitOrder);

    data.FlushBits();

    data.WriteByteSeq(guidBytes[0]);
    data.WriteByteSeq(guidBytes[7]);

    data << uint32(time(NULL) + pTime); // Invite lasts until

    data.WriteByteSeq(guidBytes[6]);
    data.WriteByteSeq(guidBytes[5]);
    data.WriteByteSeq(guidBytes[3]);

    data << uint32(zoneId);             // Zone Id

    data.WriteByteSeq(guidBytes[1]);
    data.WriteByteSeq(guidBytes[4]);   
    data.WriteByteSeq(guidBytes[2]);

    SendPacket(&data);
}

//This send invitation to player to join the queue
//Param1:(guid) the guid of Bf
void WorldSession::SendBfInvitePlayerToQueue(uint64 guid)
{
    ObjectGuid guidBytes = guid;
    bool warmup = true;

    WorldPacket data(SMSG_BATTLEFIELD_MGR_QUEUE_INVITE);
    
    data.WriteBit(1); // !+9
    data.WriteBit(guidBytes[2]);
    data.WriteBit(0); // +48
    data.WriteBit(guidBytes[7]);
    data.WriteBit(guidBytes[6]);
    data.WriteBit(1); // !+8
    data.WriteBit(guidBytes[1]);
    data.WriteBit(1); // !+11
    data.WriteBit(guidBytes[4]);
    data.WriteBit(guidBytes[5]);
    data.WriteBit(!warmup); // warmup
    data.WriteBit(guidBytes[0]);
    data.WriteBit(0); // -(bit != 0) +7 dword
    data.WriteBit(1); // +!6, dword
    data.WriteBit(guidBytes[3]);

    data.FlushBits();

    data.WriteByteSeq(guidBytes[6]);

    //if(-(bit != 0) +7 dword)
    //    data << uint32(0);

    //if(!+11)
    //    data << uint32(0);

    data.WriteByteSeq(guidBytes[5]);

    //if(!+6)
    //    data << uint32(0);

    //if(!+9)
    //    data << uint32(0);

    data.WriteByteSeq(guidBytes[7]);
    data.WriteByteSeq(guidBytes[1]);

    if(warmup)
        data << uint8(1);

    data.WriteByteSeq(guidBytes[2]);

    //if(+!8)
    //  data << uint32(0);

    data.WriteByteSeq(guidBytes[3]);
    data.WriteByteSeq(guidBytes[4]);
    data.WriteByteSeq(guidBytes[0]);

    //Sending packet to player
    SendPacket(&data);
}

//This send packet for inform player that he join queue
//Param1:(guid) the guid of Bf
//Param2:(ZoneId) the zone where the battle is (4197 for wg)
//Param3:(CanQueue) if able to queue
//Param4:(Full) on log in is full
void WorldSession::SendBfQueueInviteResponse(uint64 guid,uint32 ZoneId, bool CanQueue, bool Full)
{
    const bool hasSecondGuid = false;
    const bool warmup = true;
    ObjectGuid bgGuid = guid;

    WorldPacket data(SMSG_BATTLEFIELD_MGR_QUEUE_REQUEST_RESPONSE, 16);

    data.WriteBit(bgGuid[5]);
    data.WriteBit(bgGuid[2]);
    data.WriteBit(bgGuid[7]);

    data.WriteBit(Full);         // Logging In: 0 wg full - 1 queue for upcoming - May be inversed - CHECK!

    data.WriteBit(bgGuid[3]);
    data.WriteBit(bgGuid[1]);
    data.WriteBit(bgGuid[0]);

    data.WriteBit(!hasSecondGuid);

    // if (hasSecondGuid) 7 3 0 4 2 6 1 5

    data.WriteBit(bgGuid[4]);
    data.WriteBit(bgGuid[6]);

    data.FlushBits();

    data << uint8(CanQueue);  // Accepted

    // if (hasSecondGuid) 7 1 2 6 5 3 4 0

    data.WriteByteSeq(bgGuid[1]);
    data.WriteByteSeq(bgGuid[6]);
    data.WriteByteSeq(bgGuid[0]);
    data.WriteByteSeq(bgGuid[5]);
    data.WriteByteSeq(bgGuid[3]);

    data << uint8(warmup);

    data.WriteByteSeq(bgGuid[7]);
    data.WriteByteSeq(bgGuid[4]);

    data << uint32(ZoneId);

    data.WriteByteSeq(bgGuid[5]);

    SendPacket(&data);
}

//This is call when player accept to join war
//Param1:(guid) the guid of Bf
void WorldSession::SendBfEntered(uint64 guid)
{
    ObjectGuid bgGuid = guid;
//    m_PlayerInWar[player->GetTeamId()].insert(player->GetGUID());

    WorldPacket data(SMSG_BATTLEFIELD_MGR_ENTERING);

    data.WriteBit(bgGuid[7]);
    data.WriteBit(bgGuid[3]);
    data.WriteBit(bgGuid[2]);

    data.WriteBit(1); // unk
    data.WriteBit(_player->isAFK() ? 1 : 0); //Clear AFK

    data.WriteBit(bgGuid[6]);
    data.WriteBit(bgGuid[0]);
    data.WriteBit(bgGuid[5]);

    data.WriteBit(bgGuid[1]);
    data.WriteBit(bgGuid[4]);

    data.WriteBit(1); // unk

    data.FlushBits();
    
    uint8 byteOrder[8] = {1, 7, 3, 4, 2, 5, 6, 0};
    data.WriteBytesSeq(bgGuid, byteOrder);

    SendPacket(&data);
}

void WorldSession::SendBfLeaveMessage(uint64 guid, BFLeaveReason reason)
{
    ObjectGuid bgGuid = guid;

    WorldPacket data(SMSG_BATTLEFIELD_MGR_EJECTED);

    data.WriteBit(bgGuid[1]);
    data.WriteBit(bgGuid[6]);
    data.WriteBit(bgGuid[5]);
    data.WriteBit(bgGuid[2]);
    data.WriteBit(bgGuid[0]);
    data.WriteBit(bgGuid[4]);

    data.WriteBit(0); // relocated

    data.WriteBit(bgGuid[7]);
    data.WriteBit(bgGuid[3]);

    data.FlushBits();

    data << uint8(2); // byte BattleStatus

    data.WriteByteSeq(bgGuid[0]);
    data.WriteByteSeq(bgGuid[4]);
    data.WriteByteSeq(bgGuid[7]);
    data.WriteByteSeq(bgGuid[2]);
    data.WriteByteSeq(bgGuid[6]);
    data.WriteByteSeq(bgGuid[1]);

    data << uint8(reason); // byte Reason

    data.WriteByteSeq(bgGuid[5]);
    data.WriteByteSeq(bgGuid[3]);

    SendPacket(&data);
}

//Send by client when he click on accept for queue
void WorldSession::HandleBfQueueInviteResponse(WorldPacket& recvData)
{
    uint8 accepted;
    ObjectGuid guid;

    uint8 bitOrder[8] = {4, 7, 6, 1, 2, 5, 0, 3};
    recvData.ReadBitInOrder(guid, bitOrder);

    accepted = recvData.ReadBit();
    
    recvData.FlushBits();

    uint8 byteOrder[8] = {6, 4, 0, 1, 5, 7, 3, 2};
    recvData.ReadBytesSeq(guid, byteOrder);

    sLog->outError(LOG_FILTER_GENERAL, "HandleQueueInviteResponse: GUID:" UI64FMTD " Accepted:%u", (uint64)guid, accepted);

    if (!accepted)
        return;

    Battlefield* bf = sBattlefieldMgr->GetBattlefieldByGUID(guid);
    if (!bf)
        return;
    
    bf->PlayerAcceptInviteToQueue(_player);
}

//Send by client on clicking in accept or refuse of invitation windows for join game
void WorldSession::HandleBfEntryInviteResponse(WorldPacket& recvData)
{
    uint8 accepted;
    ObjectGuid guid;

    guid[1] = recvData.ReadBit();
    guid[3] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();
    guid[6] = recvData.ReadBit();
    guid[4] = recvData.ReadBit();
    guid[2] = recvData.ReadBit();

    accepted = recvData.ReadBit();

    guid[5] = recvData.ReadBit();
    guid[0] = recvData.ReadBit();

    recvData.FlushBits();

    uint8 byteOrder[8] = {3, 2, 4, 0, 5, 7, 6, 1};
    recvData.ReadBytesSeq(guid, byteOrder);

    sLog->outError(LOG_FILTER_GENERAL, "HandleBattlefieldInviteResponse: GUID:" UI64FMTD " Accepted:%u", uint64(guid), accepted);

    Battlefield* bf = sBattlefieldMgr->GetBattlefieldByGUID(guid);
    if (!bf)
        return;

    if (accepted)
        bf->PlayerAcceptInviteToWar(_player);
    else
        if (_player->GetZoneId() == bf->GetZoneId())
            bf->KickPlayerFromBattlefield(_player->GetGUID());
}

void WorldSession::HandleBfQueueRequest(WorldPacket& recvData)
{
    ObjectGuid guid;

    uint8 bitOrder[8] = {3, 5, 7, 0, 6, 2, 1, 4};
    recvData.ReadBitInOrder(guid, bitOrder);

    recvData.FlushBits();

    uint8 byteOrder[8] = {1, 0, 3, 2, 4, 7, 5, 6};
    recvData.ReadBytesSeq(guid, byteOrder);

    sLog->outError(LOG_FILTER_GENERAL, "HandleBfQueueRequest: GUID:" UI64FMTD " ", (uint64)guid);

    if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldByGUID(guid))
    {
        if (bf->IsWarTime())
            bf->InvitePlayerToWar(_player);
        else
        {
            uint32 timer = bf->GetTimer() / 1000;
            if (timer < 15 * MINUTE)
                bf->InvitePlayerToQueue(_player);
        }
    }
}

void WorldSession::HandleBfPortOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_BATTLEFIELD_PORT Message");
}

void WorldSession::HandleBfExitQueueRequest(WorldPacket& recvData)
{
    ObjectGuid guid;

    uint8 bitOrder[8] = {4, 0, 1, 5, 3, 7, 6, 2};
    recvData.ReadBitInOrder(guid, bitOrder);

    recvData.FlushBits();

    uint8 byteOrder[8] = {0, 6, 2, 5, 4, 1, 7, 3};
    recvData.ReadBytesSeq(guid, byteOrder);

    sLog->outError(LOG_FILTER_GENERAL, "HandleBfExitQueueRequest: GUID:" UI64FMTD " ", (uint64)guid);

    SendBfLeaveMessage(guid);

    if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldByGUID(guid))
        bf->AskToLeaveQueue(_player);
}

void WorldSession::HandleBfExitRequest(WorldPacket& recvData)
{
    sLog->outError(LOG_FILTER_GENERAL, "HandleBfExitRequest");

    Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(_player->GetZoneId());
    if (bf)
        bf->KickPlayerFromBattlefield(_player->GetGUID());
}
