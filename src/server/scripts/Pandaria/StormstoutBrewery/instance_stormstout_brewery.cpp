/*
* Copyright (C) 2014-2017 RoG_WoW Source  <http://wow.rog.snet
*
* Dont Share The SourceCode
*
*/

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "Player.h"
#include "Unit.h"
#include "ObjectAccessor.h"
#include "Group.h"

#include "stormstout_brewery.h"

/*static BossScenarios const g_BossScenarios[] =
{
    { Data::DATA_OOKOOK,                eScenarioDatas::OokOok },
    { Data::DATA_HOPTALLUS,             eScenarioDatas::Hoptallus },
    { Data::DATA_YANZHU_THE_UNCASKED,   eScenarioDatas::YanZhu },
    { 0, 0 }
};*/

class instance_stormstout_brewery : public InstanceMapScript
{
    public:
        instance_stormstout_brewery() : InstanceMapScript("instance_stormstout_brewery", 961) { }

        struct instance_stormstout_brewery_InstanceMapScript : public InstanceScript
        {
            instance_stormstout_brewery_InstanceMapScript(Map* map) : InstanceScript(map)
            {
                Initialize();
            }

            // Bosses.
            uint64 uiOokOok;
            uint64 uiHoptallus;
            uint64 uiYanzhuTheUncasked;
            // GameObjects
            uint64 uiHoptallusDoor;
            uint64 uiYanzhuDoor;
            uint64 m_HoptallusKegGuid;
            uint64 m_CarrotDoorGuid;
            bool OokOokSummoned;
            bool OokOokInCombat;
            bool HoptallusSummoned;
            bool YanzhuSummoned;
            std::list<Creature*> partyAnimalList;

            // Hozen killed for Ook-Ook summon.
            uint32 HozenKilled;

            uint32 playerTimer;
            Map::PlayerList const &plrList = instance->GetPlayers();

            void Initialize()
            {
                SetBossNumber(MAX_ENCOUNTERS);

                // Bosses.
                uiOokOok = 0;
                uiHoptallus = 0;
                uiYanzhuTheUncasked = 0;

                // GameObjects
                m_HoptallusKegGuid = 0;
                m_CarrotDoorGuid = 0;
                uiHoptallusDoor = 0;
                uiYanzhuDoor = 0;

                OokOokSummoned = false;
                OokOokInCombat = false;
                HoptallusSummoned = false;
                YanzhuSummoned = false;

                // Hozen Party Animal list.
                partyAnimalList.clear();

                // Hozen killed for Ook-Ook summon.
                HozenKilled = 0;

                for (uint32 i = 0; i < MAX_ENCOUNTERS; ++i)
                    SetBossState(i, NOT_STARTED);
            }

            bool IsEncounterInProgress() const
            {
                for (uint32 i = 0; i < MAX_ENCOUNTERS; ++i)
                    if (GetBossState(i) == IN_PROGRESS)
                        return true;

                return false;
            }

            void OnCreatureCreate(Creature* creature)
            {
                switch(creature->GetEntry())
                {
                    // Bosses.
                    case BOSS_OOKOOK:
                        uiOokOok = creature->GetGUID();
                        break;
                    case BOSS_HOPTALLUS:
                        uiHoptallus = creature->GetGUID();
                        break;
                    case BOSS_YANZHU_THE_UNCASKED:
                        uiYanzhuTheUncasked = creature->GetGUID();
                        YanzhuSummoned = true;
                        break;

                    // NPCs
                    case NPC_ANCESTRAL_BREWMASTER_1:
                    case NPC_ANCESTRAL_BREWMASTER_2:
                    case NPC_ANCESTRAL_BREWMASTER_3:
                        creature->AddAura(SPELL_ANCESTRAL_BREWM_V, creature);
                        break;
                    case NPC_HOZEN_PARTY_ANIMAL:
                        partyAnimalList.push_back(creature);
                        break;
                    case NPC_GENERAL_PURPOSE_BUNNY_JMF:
                        if (!creature->isAlive())
                            creature->Respawn(true);
                        break;
                    default: break;
                }
            }

            // Special function to summon Ook-Ook.
            void PlayerSummonOokOok(Player* player, bool hasGroup)
            {
                if (OokOokSummoned)
                    return;

                if (!OokOokInCombat)
                {
                    if (Creature* l_OokOok = instance->GetCreature(GetData64(DATA_OOKOOK)))
                    if (l_OokOok->AI())
                        l_OokOok->AI()->DoAction(ACTION_OOK_OOK_FIGHT);                    

                    OokOokInCombat = true;
                }

                // Remove auras.
                if (hasGroup)
                {
                    for (GroupReference* itr = player->GetGroup()->GetFirstMember(); itr != NULL; itr = itr->next())
                        if (Player* member = itr->getSource())
                            if (member->HasAura(SPELL_BANANA_BAR))
                                member->RemoveAurasDueToSpell(SPELL_BANANA_BAR);
                }
                else
                {
                    if (player->HasAura(SPELL_BANANA_BAR))
                        player->RemoveAurasDueToSpell(SPELL_BANANA_BAR);
                }

                OokOokSummoned = true;
            }

            void OnUnitDeath(Unit* killed)
            {
                if (killed->GetTypeId() == TYPEID_PLAYER) return;

                switch(killed->ToCreature()->GetEntry())
                {
                    // Script for Ook-ook summon bar.
                    case NPC_SODDEN_HOZEN_BRAWLER:
                    case NPC_INFLAMED_HOZEN_BRAWLER:
                    case NPC_SLEEPY_HOZEN_BRAWLER:
                    case NPC_DRUNKEN_HOZEN_BRAWLER:
                    case NPC_HOZEN_BOUNCER:
                    case NPC_HOZEN_PARTY_ANIMAL:
                    case NPC_HOZEN_PARTY_ANIMAL_1:
                        // Remove Party Animals from the list if they died.
                        if (killed->ToCreature()->GetEntry() == NPC_HOZEN_PARTY_ANIMAL)
                            partyAnimalList.remove(killed->ToCreature());
                        if (killed->ToCreature()->GetEntry() == NPC_HOZEN_PARTY_ANIMAL_1)
                            partyAnimalList.remove(killed->ToCreature());
                        // Increase Hozen killed count.
                        if (HozenKilled < HOZEN_KILLS_NEEDED)
                            HozenKilled++;
                        // Check auras and increase player powers / summon Ook-Ook.
                        if (Player* player = killed->FindNearestPlayer(20.0f))
                        {
                            if (player->GetGroup())
                            {
                                if (Player* Leader = ObjectAccessor::FindPlayer(player->GetGroup()->GetLeaderGUID()))
                                {
                                    if (HozenKilled < HOZEN_KILLS_NEEDED) // We check the counter in advance to summon Ook-Ook at right value.
                                    {
                                        // Update Leader power.
                                        if (!Leader->HasAura(SPELL_BANANA_BAR))
                                            Leader->AddAura(SPELL_BANANA_BAR, Leader);
                                        Leader->SetPower(POWER_ALTERNATE_POWER, HozenKilled);

                                        // Update group member bars.
                                        for (GroupReference* itr = Leader->GetGroup()->GetFirstMember(); itr != NULL; itr = itr->next())
                                        {
                                            if (Player* member = itr->getSource())
                                            {
                                                if (member != Leader)
                                                {
                                                    if (!member->HasAura(SPELL_BANANA_BAR))
                                                        member->AddAura(SPELL_BANANA_BAR, member);
                                                    member->SetPower(POWER_ALTERNATE_POWER, HozenKilled);
                                                }
                                            }
                                        }
                                    }
                                    else
                                        PlayerSummonOokOok(Leader, true);
                                }
                            }
                            else // Solo.
                            {
                                if (HozenKilled < HOZEN_KILLS_NEEDED) // We check the counter in advance to summon Ook-Ook at right value.
                                {
                                    if (!player->HasAura(SPELL_BANANA_BAR))
                                        player->AddAura(SPELL_BANANA_BAR, player);
                                    player->SetPower(POWER_ALTERNATE_POWER, HozenKilled); // // Update player power.
                                }
                                else
                                    PlayerSummonOokOok(player, false);
                            }
                        }
                        break;                        

                    default: break;
                }
            }

            void OnGameObjectCreate(GameObject* go)
            {
                switch (go->GetEntry())
                {
                    case GAMEOBJECT_BREWERY_DOOR:
                        if (go->GetPositionX() > -704.0f && go->GetPositionX() < -702.0f && go->GetPositionY() > 1283.0f && go->GetPositionY() < 1286.0f)
                            uiHoptallusDoor = go->GetGUID();
                        if (go->GetPositionX() > -671.0f && go->GetPositionX() < -669.0f && go->GetPositionY() > 1136.0f && go->GetPositionY() < 1139.0f)
                            uiYanzhuDoor = go->GetGUID();
                        break;

                    case GameObjects::GAMEOBJECT_GIANT_BARREL:
                        m_HoptallusKegGuid = go->GetGUID();
                        break;
                    case GameObjects::GAMEOBJECT_GIANT_CARROT:
                        m_CarrotDoorGuid = go->GetGUID();
                        break;

                    default: break;
                }
            }

            /*
            void OnGameObjectRemove(GameObject* go)
            {
                switch (go->GetEntry())
                {
                }
            }
            */

            void SetData(uint32 type, uint32 data)
            {
                if (type > DATA_YANZHU_THE_UNCASKED_EVENT)
                    return;

                SetBossState(type, EncounterState(data));

                if (data == DONE)
                    SaveToDB();
            }

            uint32 GetData(uint32 type)
            {
                // First check for type.
                if (type == DATA_HOZEN_KILLED)
                    return HozenKilled;

                if (type == DATA_OOK_SUMMONED)
                    return OokOokSummoned;

                if (type == DATA_HOPTALLUS_SUMMONED)
                    return HoptallusSummoned;

                if (type == DATA_YANZHU_SUMMONED)
                    return YanzhuSummoned;

                return GetBossState(type);
            }

            uint64 GetData64(uint32 data)
            {
                switch(data)
                {
                    // Bosses.
                    case DATA_OOKOOK:               return uiOokOok;             break;
                    case DATA_HOPTALLUS:            return uiHoptallus;          break;
                    case DATA_YANZHU_THE_UNCASKED:  return uiYanzhuTheUncasked;  break;

                    case DATA_HOPTALLUS_DOOR:       return uiHoptallusDoor;      break;
                    case DATA_YAN_ZHU_DOOR:         return uiYanzhuDoor;         break;

                        // GameObjects
                    case GAMEOBJECT_GIANT_BARREL:
                        return m_HoptallusKegGuid;
                    case GAMEOBJECT_GIANT_CARROT:
                        return m_CarrotDoorGuid;

                    default:                        return 0;                    break;
                }
            }

            bool SetBossState(uint32 data, EncounterState state)
            {
                if (!InstanceScript::SetBossState(data, state))
                    return false;

                if (state == DONE)
                {
                    switch(data)
                    {
                        case DATA_OOKOOK_EVENT:
                        case DATA_HOPTALLUS_EVENT:
                        case DATA_YANZHU_THE_UNCASKED_EVENT:
                        break;

                        default: break;
                    }
                }

                return true;
            }

            bool IsWipe()
            {
                Map::PlayerList const& PlayerList = instance->GetPlayers();

                if (!PlayerList.isEmpty())
                {
                    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                    {
                        if (Player* plr = i->getSource())
                            if (plr->isAlive() && !plr->isGameMaster())
                                return false;
                    }
                }
                return true;
            }

            std::string GetSaveData()
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << "S B " << GetBossSaveData() << ' ' << HozenKilled << ' ' << OokOokSummoned << ' ' << HoptallusSummoned << ' ' << YanzhuSummoned;

                OUT_SAVE_INST_DATA_COMPLETE;
                return saveStream.str();
            }

            void Load(const char* in)
            {
                if (!in)
                {
                    OUT_LOAD_INST_DATA_FAIL;
                    return;
                }

                OUT_LOAD_INST_DATA(in);

                char dataHead1, dataHead2;

                std::istringstream loadStream(in);
                loadStream >> dataHead1 >> dataHead2;

                if (dataHead1 == 'S' && dataHead2 == 'B')
                {
                    for (uint32 i = 0; i < MAX_ENCOUNTERS; ++i)
                    {
                        uint32 tmpState;
                        loadStream >> tmpState;
            
                        if (tmpState == IN_PROGRESS || tmpState > SPECIAL)
                            tmpState = NOT_STARTED;
            
                        // Below makes the player on-instance-entry display of bosses killed shit work (SMSG_RAID_INSTANCE_INFO).
                        // Like, say an unbound player joins the party and he tries to enter the dungeon / raid.
                        // This makes sure binding-to-instance-on-entrance confirmation box will properly display bosses defeated / available.
                        SetBossState(i, EncounterState(tmpState));

                        // Load killed Hozen counter.
                        uint32 hozenDead = 0;
                        loadStream >> hozenDead;
                        HozenKilled = hozenDead;

                        // Load boss data and spawn them if needed.
                        bool OokOokToSummon = false;
                        loadStream >> OokOokToSummon;
                        OokOokSummoned = OokOokToSummon;

                        bool HoptallusToSummon = false;
                        loadStream >> HoptallusToSummon;
                        HoptallusSummoned = HoptallusToSummon;

                        bool YanzhuToSummon = false;
                        loadStream >> YanzhuToSummon;
                        YanzhuSummoned = YanzhuToSummon;

                        if (OokOokToSummon == true && GetData(DATA_OOKOOK_EVENT) != DONE)
                        {                            
                            if (!OokOokInCombat)
                            {
                                if (Creature* l_OokOok = instance->GetCreature(GetData64(DATA_OOKOOK)))
                                if (l_OokOok->AI())
                                    l_OokOok->AI()->DoAction(ACTION_OOK_OOK_FIGHT);

                                OokOokInCombat = true;
                            }
                            OokOokToSummon = false;
                        }

                        if (HoptallusToSummon == true && GetData(DATA_HOPTALLUS_EVENT) != DONE && GetData(DATA_OOKOOK_EVENT) == DONE)
                        {
                            if (Creature* hoptallus = instance->GetCreature(GetData64(DATA_HOPTALLUS)))
                            if (hoptallus->AI())
                                hoptallus->AI()->DoAction(ACTION_HOPTALLUS_APPEARS);                         
                            HoptallusToSummon = false;
                        }
                    }
                }
                else OUT_LOAD_INST_DATA_FAIL;

                OUT_LOAD_INST_DATA_COMPLETE;
            }

            void Update(uint32 diff)
            {

                if (playerTimer <= diff)
                {
                    if (!plrList.isEmpty())
                    for (Map::PlayerList::const_iterator i = plrList.begin(); i != plrList.end(); ++i)
                    if (Player* pPlayer = i->getSource())
                    if (InstanceScript* instance = pPlayer->GetInstanceScript())
                    {
                        if (instance->GetBossState(DATA_OOKOOK_EVENT) != DONE)
                        if (pPlayer->GetPositionZ() < 50.0f && !pPlayer->IsBeingTeleported())
                            pPlayer->NearTeleportTo(TeleportPos.GetPositionX(), TeleportPos.GetPositionY(), TeleportPos.GetPositionZ(), TeleportPos.GetOrientation());
                    }
                    playerTimer = 2000;
                }
                else
                    playerTimer -= diff;
            }

        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_stormstout_brewery_InstanceMapScript(map);
        }
};

void AddSC_instance_stormstout_brewery()
{
    new instance_stormstout_brewery();
}
