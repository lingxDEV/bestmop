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

#include "Common.h"
#include "TicketMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Chat.h"
#include "World.h"

inline float GetAge(uint64 t) { return float(time(NULL) - t) / DAY; }

///////////////////////////////////////////////////////////////////////////////////////////////////
// GM ticket
GmTicket::GmTicket() { }

GmTicket::GmTicket(Player* player, WorldPacket& recvData) : _createTime(time(NULL)), _lastModifiedTime(time(NULL)), _closedBy(0), _assignedTo(0), _completed(false),
                                                            _escalatedStatus(TICKET_UNASSIGNED), _needResponse(false), _haveTicket(false), _viewed(false)
{
    uint8 UnkByte;
    uint32 fakeStringLength;

    recvData >> _posY >> _posX >> _mapId >> _posZ >> UnkByte;

    recvData >> fakeStringLength;
    std::string fakeString = recvData.ReadString(fakeStringLength);

    /* The fakeString data reading should actually be handled like below or skipped, but for some reason this doesn't work.
    uint32 BufferSize;
    WorldPacket UnkBuffer;
    recvData >> BufferSize;
    UnkBuffer.resize(BufferSize);
    recvData.read((uint8*)UnkBuffer.contents(), BufferSize);*/

    _needResponse = recvData.ReadBit();
    _haveTicket = recvData.ReadBit();

    size_t DescriptionLenght = recvData.ReadBits(11);

    _message = recvData.ReadString(DescriptionLenght);

    _id = sTicketMgr->GenerateTicketId();
    _playerName = player->GetName();
    _playerGuid = player->GetGUID();
}

GmTicket::~GmTicket() { }

bool GmTicket::LoadFromDB(Field* fields)
{
    //     0       1     2      3          4        5      6     7     8           9            10         11         12        13        14        15
    // ticketId, guid, name, message, createTime, mapId, posX, posY, posZ, lastModifiedTime, closedBy, assignedTo, comment, completed, escalated, viewed
    uint8 index = 0;
    _id                 = fields[  index].GetUInt32();
    _playerGuid         = MAKE_NEW_GUID(fields[++index].GetUInt32(), 0, HIGHGUID_PLAYER);
    _playerName         = fields[++index].GetString();
    _message            = fields[++index].GetString();
    _createTime         = fields[++index].GetUInt32();
    _mapId              = fields[++index].GetUInt16();
    _posX               = fields[++index].GetFloat();
    _posY               = fields[++index].GetFloat();
    _posZ               = fields[++index].GetFloat();
    _lastModifiedTime   = fields[++index].GetUInt32();
    _closedBy           = fields[++index].GetInt32();
    _assignedTo         = MAKE_NEW_GUID(fields[++index].GetUInt32(), 0, HIGHGUID_PLAYER);
    _comment            = fields[++index].GetString();
    _completed          = fields[++index].GetBool();
    _escalatedStatus    = GMTicketEscalationStatus(fields[++index].GetUInt8());
    _viewed             = fields[++index].GetBool();
    _haveTicket         = fields[++index].GetBool();
    return true;
}

void GmTicket::SaveToDB(SQLTransaction& trans) const
{
    //     0       1     2      3          4        5      6     7     8           9            10         11         12        13        14        15
    // ticketId, guid, name, message, createTime, mapId, posX, posY, posZ, lastModifiedTime, closedBy, assignedTo, comment, completed, escalated, viewed
    uint8 index = 0;
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_GM_TICKET);
    stmt->setUInt32(  index, _id);
    stmt->setUInt32(++index, GUID_LOPART(_playerGuid));
    stmt->setString(++index, _playerName);
    stmt->setString(++index, _message);
    stmt->setUInt32(++index, uint32(_createTime));
    stmt->setUInt16(++index, _mapId);
    stmt->setFloat (++index, _posX);
    stmt->setFloat (++index, _posY);
    stmt->setFloat (++index, _posZ);
    stmt->setUInt32(++index, uint32(_lastModifiedTime));
    stmt->setInt32 (++index, GUID_LOPART(_closedBy));
    stmt->setUInt32(++index, GUID_LOPART(_assignedTo));
    stmt->setString(++index, _comment);
    stmt->setBool  (++index, _completed);
    stmt->setUInt8 (++index, uint8(_escalatedStatus));
    stmt->setBool  (++index, _viewed);
    stmt->setBool  (++index, _haveTicket);

    CharacterDatabase.ExecuteOrAppend(trans, stmt);
}

void GmTicket::DeleteFromDB()
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GM_TICKET);
    stmt->setUInt32(0, _id);
    CharacterDatabase.Execute(stmt);
}

void GmTicket::WritePacket(WorldPacket& data) const
{
    if (GmTicket* ticket = sTicketMgr->GetOldestOpenTicket())
        data << uint32(GetAge(ticket->GetLastModifiedTime()));
    else
        data << uint32(0);

    data << uint32(GetId());
    data << uint32(GetAge(sTicketMgr->GetLastChange()));
    data << uint8(_viewed ? GMTICKET_OPENEDBYGM_STATUS_OPENED : GMTICKET_OPENEDBYGM_STATUS_NOT_OPENED); // Whether or not it has been viewed.
    data << uint32(GetAge(_lastModifiedTime));

    if (_message.size())
        data.WriteString(_message);

    data << uint32(0); // Time since GM Response (waitTimeOverrideMinutes).
    data << uint8(std::min(_escalatedStatus, TICKET_IN_ESCALATION_QUEUE)); // Escalated data.

    // GM Response (waitTimeOverrideMessage).
    if (_response.size())
        data.WriteString(_response);

    data << uint8(_haveTicket);
}

std::string GmTicket::FormatMessageString(ChatHandler& handler, bool detailed) const
{
    time_t curTime = time(NULL);

    std::stringstream ss;
    std::string nameLink = handler.playerLink(_playerName);

    ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTGUID, _id);
    ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTNAME, nameLink.c_str());
    ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTAGECREATE, (secsToTimeString(curTime - _createTime, true, false)).c_str());
    ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTAGE, (secsToTimeString(curTime - _lastModifiedTime, true, false)).c_str());

    std::string name;
    if (sObjectMgr->GetPlayerNameByGUID(_assignedTo, name))
        ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTASSIGNEDTO, name.c_str());

    if (detailed)
    {
        ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTMESSAGE, _message.c_str());
        if (!_comment.empty())
            ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTCOMMENT, _comment.c_str());
    }
    return ss.str();
}

std::string GmTicket::FormatMessageString(ChatHandler& handler, const char* szClosedName, const char* szAssignedToName, const char* szUnassignedName, const char* szDeletedName) const
{
    std::stringstream ss;
    ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTGUID, _id);
    ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTNAME, _playerName.c_str());
    if (szClosedName)
        ss << handler.PGetParseString(LANG_COMMAND_TICKETCLOSED, szClosedName);
    if (szAssignedToName)
        ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTASSIGNEDTO, szAssignedToName);
    if (szUnassignedName)
        ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTUNASSIGNED, szUnassignedName);
    if (szDeletedName)
        ss << handler.PGetParseString(LANG_COMMAND_TICKETDELETED, szDeletedName);
    return ss.str();
}

void GmTicket::SetUnassigned()
{
    _assignedTo = 0;
    switch (_escalatedStatus)
    {
        case TICKET_ASSIGNED: _escalatedStatus = TICKET_UNASSIGNED; break;
        case TICKET_ESCALATED_ASSIGNED: _escalatedStatus = TICKET_IN_ESCALATION_QUEUE; break;
        case TICKET_UNASSIGNED:
        case TICKET_IN_ESCALATION_QUEUE:
        default:
            break;
    }
}

void GmTicket::TeleportTo(Player* player) const
{
    player->TeleportTo(_mapId, _posX, _posY, _posZ, 0.0f, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Ticket manager
TicketMgr::TicketMgr() : _status(true), _lastTicketId(0), _lastSurveyId(0), _openTicketCount(0), _lastChange(time(NULL)) { }

TicketMgr::~TicketMgr()
{
    for (GmTicketList::const_iterator itr = _ticketList.begin(); itr != _ticketList.end(); ++itr)
        delete itr->second;
}

void TicketMgr::Initialize() { SetStatus(sWorld->getBoolConfig(CONFIG_ALLOW_TICKETS)); }

void TicketMgr::ResetTickets()
{
    for (GmTicketList::const_iterator itr = _ticketList.begin(); itr != _ticketList.end(); ++itr)
        if (itr->second->IsClosed())
            sTicketMgr->RemoveTicket(itr->second->GetId());

    _lastTicketId = 0;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ALL_GM_TICKETS);
    CharacterDatabase.Execute(stmt);
}

void TicketMgr::LoadTickets()
{
    uint32 oldMSTime = getMSTime();

    for (GmTicketList::const_iterator itr = _ticketList.begin(); itr != _ticketList.end(); ++itr)
        delete itr->second;
    
	/// Delete old tickets (> 2 day)
	CharacterDatabase.PQuery("DELETE FROM gm_tickets WHERE createtime < UNIX_TIMESTAMP() - 60*60*24*2");

	_ticketList.clear();

    _lastTicketId = 0;
    _openTicketCount = 0;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GM_TICKETS);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);
    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 GM tickets. DB table `gm_tickets` is empty!");

        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        GmTicket* ticket = new GmTicket();
        if (!ticket->LoadFromDB(fields))
        {
            delete ticket;
            continue;
        }
        if (!ticket->IsClosed())
            ++_openTicketCount;

        // Update max ticket id if necessary
        uint32 id = ticket->GetId();
        if (_lastTicketId < id)
            _lastTicketId = id;

        _ticketList[id] = ticket;
        ++count;
    }
    while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u GM tickets in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

}

void TicketMgr::LoadSurveys()
{
    // we don't actually load anything into memory here as there's no reason to
    _lastSurveyId = 0;

    uint32 oldMSTime = getMSTime();
    if (QueryResult result = CharacterDatabase.Query("SELECT MAX(surveyId) FROM gm_surveys"))
        _lastSurveyId = (*result)[0].GetUInt32();

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded GM Survey count from database in %u ms", GetMSTimeDiffToNow(oldMSTime));

}

void TicketMgr::AddTicket(GmTicket* ticket)
{
    _ticketList[ticket->GetId()] = ticket;
    if (!ticket->IsClosed())
        ++_openTicketCount;
    SQLTransaction trans = SQLTransaction(NULL);
    ticket->SaveToDB(trans);
}

void TicketMgr::CloseTicket(uint32 ticketId, int64 source)
{
    if (GmTicket* ticket = GetTicket(ticketId))
    {
        SQLTransaction trans = SQLTransaction(NULL);
        ticket->SetClosedBy(source);
        if (source)
            --_openTicketCount;
        ticket->SaveToDB(trans);
    }
}

void TicketMgr::RemoveTicket(uint32 ticketId)
{
    if (GmTicket* ticket = GetTicket(ticketId))
    {
        ticket->DeleteFromDB();
        _ticketList.erase(ticketId);
        delete ticket;
    }
}

void TicketMgr::ShowList(ChatHandler& handler, bool onlineOnly) const
{
    handler.SendSysMessage(onlineOnly ? LANG_COMMAND_TICKETSHOWONLINELIST : LANG_COMMAND_TICKETSHOWLIST);
    for (GmTicketList::const_iterator itr = _ticketList.begin(); itr != _ticketList.end(); ++itr)
        if (!itr->second->IsClosed() && !itr->second->IsCompleted())
            if (!onlineOnly || itr->second->GetPlayer())
                handler.SendSysMessage(itr->second->FormatMessageString(handler).c_str());
}

void TicketMgr::ShowClosedList(ChatHandler& handler) const
{
    handler.SendSysMessage(LANG_COMMAND_TICKETSHOWCLOSEDLIST);
    for (GmTicketList::const_iterator itr = _ticketList.begin(); itr != _ticketList.end(); ++itr)
        if (itr->second->IsClosed())
            handler.SendSysMessage(itr->second->FormatMessageString(handler).c_str());
}

void TicketMgr::ShowEscalatedList(ChatHandler& handler) const
{
    handler.SendSysMessage(LANG_COMMAND_TICKETSHOWESCALATEDLIST);
    for (GmTicketList::const_iterator itr = _ticketList.begin(); itr != _ticketList.end(); ++itr)
        if (!itr->second->IsClosed() && itr->second->GetEscalatedStatus() == TICKET_IN_ESCALATION_QUEUE)
            handler.SendSysMessage(itr->second->FormatMessageString(handler).c_str());
}

void TicketMgr::SendTicket(WorldSession* session, GmTicket* ticket) const
{
    uint32 status = GMTICKET_STATUS_DEFAULT;
    std::string message;
    std::string response;

    if (ticket)
    {
        message = ticket->GetMessage();
        response = ticket->GetResponse();

        status = GMTICKET_STATUS_HASTEXT;
    }

    WorldPacket data(SMSG_GM_TICKET_GET_TICKET_RESPONSE);

    data << uint32(status);

    data.WriteBit(status == GMTICKET_STATUS_HASTEXT);

    if (status == GMTICKET_STATUS_HASTEXT)
    {
        data.WriteBits(response.size() ? response.size() : 0, 10); // GM Response size (waitTimeOverrideMessage).
        data.WriteBits(message.size()  ? message.size()  : 0, 11); // Ticket message size.

        data.FlushBits();

        // We've got the easy stuff done by now, now we need to go through the client logic for displaying various levels of ticket load.
        ticket->WritePacket(data);
    }

    session->SendPacket(&data);
}

void TicketMgr::SendTicketStatusUpdate(WorldSession* session, uint8 response) const
{
    WorldPacket data(SMSG_GM_TICKET_STATUS_UPDATE, 1);
    data << uint8(response);
    session->SendPacket(&data);
}
