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

#include "SpellMgr.h"
#include "SpellInfo.h"
#include "ObjectMgr.h"
#include "SpellAuras.h"
#include "SpellAuraDefines.h"
#include "SharedDefines.h"
#include "DBCStores.h"
#include "World.h"
#include "Chat.h"
#include "Spell.h"
#include "BattlegroundMgr.h"
#include "CreatureAI.h"
#include "MapManager.h"
#include "BattlegroundIC.h"
#include "BattlefieldWG.h"
#include "BattlefieldMgr.h"
#include "InstanceScript.h"

bool IsPrimaryProfessionSkill(uint32 skill)
{
    SkillLineEntry const* pSkill = sSkillLineStore.LookupEntry(skill);
    if (!pSkill)
        return false;

    if (pSkill->categoryId != SKILL_CATEGORY_PROFESSION)
        return false;

    return true;
}

bool IsPartOfSkillLine(uint32 skillId, uint32 spellId)
{
    SkillLineAbilityMapBounds skillBounds = sSpellMgr->GetSkillLineAbilityMapBounds(spellId);
    for (SkillLineAbilityMap::const_iterator itr = skillBounds.first; itr != skillBounds.second; ++itr)
    if (itr->second->skillId == skillId)
        return true;

    return false;
}

DiminishingGroup GetDiminishingReturnsGroupForSpell(SpellInfo const* spellproto, bool triggered)
{
    if (spellproto->IsPositive())
        return DIMINISHING_NONE;

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (spellproto->Effects[i].ApplyAuraName == SPELL_AURA_MOD_TAUNT)
            return DIMINISHING_TAUNT;
    }

    // Explicit Diminishing Groups
    switch (spellproto->SpellFamilyName)
    {
    case SPELLFAMILY_GENERIC:
    {
                                switch (spellproto->Id)
                                {
                                case 20549:  // War Stomp
                                case 47481:  // Gnaw
                                case 96201:  // Web Wrap
                                    return DIMINISHING_CONTROLLED_STUN;
                                case 25046:  // Arcane Torrent (Energy)
                                case 28730:  // Arcane Torrent (Mana)
                                case 50613:  // Arcane Torrent (Death Knight)
                                case 69179:  // Arcane Torrent (Warrior)
                                case 80483:  // Arcane Torrent (Hunter)
                                case 129597: // Arcane Torrent (Monk)
                                    return DIMINISHING_SILENCE;
                                case 107079: // Quaking Palm
                                    return DIMINISHING_DISORIENT;
                                case 113506: // Cyclone (Symbiosis)
                                    return DIMINISHING_CYCLONE;
                                }

                                // Pet charge effects (Infernal Awakening, Demon Charge)
                                if (spellproto->SpellVisual[0] == 2816 && spellproto->SpellIconID == 15)
                                    return DIMINISHING_CONTROLLED_STUN;
                                // Frost Tomb
                                else if (spellproto->Id == 48400)
                                    return DIMINISHING_NONE;
                                // Earthquake (Trash, Ulduar)
                                else if (spellproto->Id == 64697)
                                    return DIMINISHING_NONE;
                                // Summoning Disorientation
                                else if (spellproto->Id == 32752)
                                    return DIMINISHING_NONE;
                                else if (spellproto->SpellVisual[0] == 14153)
                                    return DIMINISHING_NONE;
                                // Silence, Asira Dawnslayer, Hour of Twilight
                                else if (spellproto->Id == 103587)
                                    return DIMINISHING_NONE;
                                // Glyph of Intimidating Shout
                                else if (spellproto->Id == 95199)
                                    return DIMINISHING_LIMITONLY;
                                break;
    }
        // Event spells
    case SPELLFAMILY_EVENTS:
        return DIMINISHING_NONE;
    case SPELLFAMILY_MAGE:
    {
                             switch (spellproto->Id)
                             {
                             case 122:    // Frost Nova
                             case 33395:  // Freeze
                                 return DIMINISHING_CONTROLLED_ROOT;
                             case 44572:  // Deep Freeze
                             case 118271: // Combustion Impact
                                 return DIMINISHING_CONTROLLED_STUN;
                             case 118:    // Polymorph
                             case 82691:  // Ring of Frost
                                 return DIMINISHING_DISORIENT;
                             case 55021:  // Improved Counter Spell
                             case 102051: // Frostjaw
                                 return DIMINISHING_SILENCE;
                             case 111340: // Ice Ward
                                 return DIMINISHING_ICE_WARD;
                             case 31589:  // Slow
                                 return DIMINISHING_LIMITONLY;
                             case 31661: // Dragon's Breath
                                 return DIMINISHING_DRAGONS_BREATH;
                             }

                             // Frost Nova / Freeze (Water Elemental)
                             if (spellproto->SpellIconID == 193)
                                 return DIMINISHING_CONTROLLED_ROOT;

                             break;
    }
    case SPELLFAMILY_WARRIOR:
    {
                                switch (spellproto->Id)
                                {
                                case 107566: // Staggering Shout
                                    return DIMINISHING_CONTROLLED_ROOT;
                                case 132168: // Shockwave
                                case 105771: // Warbringer
                                case 145585: // Storm Bolt
                                    return DIMINISHING_CONTROLLED_STUN;
                                case 676:    // Disarm
                                    return DIMINISHING_DISARM;
                                case 5246:   // Intimidating Shout
                                    return DIMINISHING_FEAR;
                                case 118895: // Dragon Roar
                                    return DIMINISHING_RANDOM_STUN;
                                case 6552:   // Pummel
                                case 18498:  // Glyph of Gag Order
                                    return DIMINISHING_SILENCE;
                                case 12323:  // Piercing Howl
                                    return DIMINISHING_LIMITONLY;
                                default:
                                    break;
                                }

                                // Hamstring - limit duration to 10s in PvP
                                if (spellproto->SpellFamilyFlags[0] & 0x2)
                                    return DIMINISHING_LIMITONLY;
                                // Charge Stun (own diminishing)
                                else if (spellproto->SpellFamilyFlags[0] & 0x01000000)
                                    return DIMINISHING_CHARGE;
                                break;
    }
    case SPELLFAMILY_WARLOCK:
    {
                                switch (spellproto->Id)
                                {
                                case 22703:  // Infernal Awakening
                                case 30283:  // Shadowfury
                                case 89766:  // Axe Toss
                                    return DIMINISHING_CONTROLLED_STUN;
                                case 118093: // Voidwalker: Disarm
                                    return DIMINISHING_DISARM;
                                case 6358:   // Seduction
                                case 115268: // Mesmerize
                                case 132412: // Seduction
                                    //return DIMINISHING_DISORIENT;
                                case 118699: // Fear
                                case 5484:   // Howl of Terror
                                case 104045: // Sleep
                                    return DIMINISHING_FEAR;
                                case 6789:   // Mortal Coil
                                    return DIMINISHING_HORROR;
                                case 24259:  // Spell Lock
                                case 115782: // Optical Blast
                                    return DIMINISHING_SILENCE;
                                case 710:    // Banish
                                    return DIMINISHING_BANISH;
                                case 31117:  // Unstable Affliction
                                    return DIMINISHING_LIMITONLY;
                                }

                                // Curses/etc
                                if ((spellproto->SpellFamilyFlags[0] & 0x80000000) || (spellproto->SpellFamilyFlags[1] & 0x200))
                                    return DIMINISHING_LIMITONLY;
                                // Curse of Exhaustion
                                else if (spellproto->Id == 18223)
                                    return DIMINISHING_LIMITONLY;

                                break;
    }
    case SPELLFAMILY_DRUID:
    {
                              switch (spellproto->Id)
                              {
                              case 9005:  // Pounce
                              case 22570: // Maim
                              case 102456:// Pounce (Incarnation)
                              case 5211:  // Mighty Bash
                              case 102795:// Bear Hug
                              case 113801:// Bash
                                  return DIMINISHING_CONTROLLED_STUN;
                              case 33786: // Cyclone
                                  return DIMINISHING_CYCLONE;
                              case 339:   // Entangling Roots
                              case 19975: // Entangling Roots (Nature's Grasp)
                              case 102359:// Mass Entanglement
                                  return DIMINISHING_CONTROLLED_ROOT;
                              case 81261: // Solar Beam
                                  return DIMINISHING_SILENCE;
                              case 770:   // Faerie Fire
                              case 102355:// Faerie Swarm
                                  return DIMINISHING_LIMITONLY;
                              case 45334: // Feral Charge: Bear Effect
                                  return DIMINISHING_RANDOM_ROOT;
                              case 2637:  // Hibernate
                                  return DIMINISHING_DISORIENT;
                              default:
                                  break;
                              }

                              break;
    }
    case SPELLFAMILY_ROGUE:
    {
                              switch (spellproto->Id)
                              {
                              case 115197: // Partial Paralysis
                                  return DIMINISHING_CONTROLLED_ROOT;
                              case 1833:   // Cheap Shot
                              case 408:    // Kidney Shot
                                  return DIMINISHING_CONTROLLED_STUN;
                              case 51722:  // Dismantle
                                  return DIMINISHING_DISARM;
                              case 1776:   // Gouge
                              case 6770:   // Sap
                                  return DIMINISHING_DISORIENT;
                              case 2094:   // Blind
                                  return DIMINISHING_FEAR;
                              case 113953: // Paralytic Poison
                                  return DIMINISHING_RANDOM_STUN;
                              case 1330:   // Garrote
                                  return DIMINISHING_SILENCE;
                              }

                              // Crippling poison - Limit to 10 seconds in PvP (No SpellFamilyFlags)
                              if (spellproto->SpellIconID == 163)
                                  return DIMINISHING_LIMITONLY;

                              break;
    }
    case SPELLFAMILY_HUNTER:
    {
                               switch (spellproto->Id)
                               {
                               case 4167:   // Web
                               case 50245:  // Pin
                               case 54706:  // Venom Web Spray
                               case 90327:  // Lock Jaw
                               case 136634: // Narrow Escape
                                   return DIMINISHING_CONTROLLED_ROOT;
                               case 24394:  // Intimidation
                               case 50519:  // Sonic Blast
                               case 56626:  // Sting
                               case 90337:  // Bad Manner
                               case 117526: // Binding Shot
                                   return DIMINISHING_CONTROLLED_STUN;
                               case 50541:  // Clench
                               case 91644:  // Snatch
                                   return DIMINISHING_DISARM;
                               case 19386:  // Wyvern Sting
                               case 3355:   // Freezing Trap
                                   return DIMINISHING_DISORIENT;
                               case 1513:   // Scare Beast
                                   return DIMINISHING_FEAR;
                               case 34490:  // Silencing Shot
                                   return DIMINISHING_SILENCE;
                               case 64803:  // Entrapment
                                   return DIMINISHING_ENTRAPMENT;
                               }

                               // Hunter's Mark
                               if ((spellproto->SpellFamilyFlags[0] & 0x400) && spellproto->SpellIconID == 538)
                                   return DIMINISHING_LIMITONLY;

                               break;
    }
    case SPELLFAMILY_PALADIN:
    {
                                switch (spellproto->Id)
                                {
                                case 853:    // Hammer of Justice
                                case 105593: // Fist of Justice
                                case 115752: // Blinding Light (Glyphed)
                                case 119072: // Holy Wrath
                                    return DIMINISHING_CONTROLLED_STUN;
                                case 20066:  // Repentance
                                    return DIMINISHING_DISORIENT;
                                case 10326:  // Turn Evil
                                case 145067: // Turn Evil
                                case 105421: // Blinding Light
                                    return DIMINISHING_FEAR;
                                case 31935:  // Avenger's Shield
                                    return DIMINISHING_SILENCE;
                                }

                                // Judgement of Justice - limit duration to 10s in PvP
                                if (spellproto->SpellFamilyFlags[0] & 0x100000)
                                    return DIMINISHING_LIMITONLY;

                                break;
    }
    case SPELLFAMILY_DEATHKNIGHT:
    {
                                    switch (spellproto->Id)
                                    {
                                    case 96294:  // Chains of Ice
                                        return DIMINISHING_CONTROLLED_ROOT;
                                    case 91797:  // Monstrous Blow
                                    case 91800:  // Gnaw
                                    case 108194: // Asphyxiate
                                    case 115001: // Remorseless Winter
                                        return DIMINISHING_CONTROLLED_STUN;
                                    case 47476:  // Strangulate
                                        return DIMINISHING_SILENCE;
                                    }

                                    // Hungering Cold (no flags)
                                    if (spellproto->SpellIconID == 2797)
                                        return DIMINISHING_DISORIENT;
                                    // Mark of Blood
                                    else if ((spellproto->SpellFamilyFlags[0] & 0x10000000) && spellproto->SpellIconID == 2285)
                                        return DIMINISHING_LIMITONLY;
                                    break;
    }
    case SPELLFAMILY_MONK:
    {
                             switch (spellproto->Id)
                             {
                             case 116706: // Disable
                                 return DIMINISHING_CONTROLLED_ROOT;
                             case 120086: // Fists of Fury
                             case 119381: // Leg Sweep
                             case 119392: // Charging Ox Wave
                             case 122242: // Clash
                                 return DIMINISHING_CONTROLLED_STUN;
                             case 117368: // Grapple Weapon
                                 return DIMINISHING_DISARM;
                             case 115078: // Disable
                                 return DIMINISHING_DISORIENT;
                             case 116709: // Spear Hand Strike
                                 return DIMINISHING_SILENCE;
                             case 116095: // Disable (reduce movement speed)
                                 return DIMINISHING_LIMITONLY;
                             }

                             break;
    }
    case SPELLFAMILY_PRIEST:
    {
                               switch (spellproto->Id)
                               {
                               case 87194:  // Glyph of Mind Blast
                               case 114404: // Void Tendril's Grasp
                                   return DIMINISHING_CONTROLLED_ROOT;
                               case 64058:  // Psychic Horror
                                   return DIMINISHING_DISARM;
                               case 9484:   // Shackle Undead
                                   return DIMINISHING_DISORIENT;
                               case 8122:   // Psychic Scream
                               case 113792: // Psychic Terror
                                   return DIMINISHING_FEAR;
                               case 64044:  // Psychic Horror
                               case 87204:  // Sin and Punishment
                                   return DIMINISHING_HORROR;
                               case 15487:  // Silence
                                   return DIMINISHING_SILENCE;
                               case 88625:  // Holy Word: Chastise
                                   return DIMINISHING_NONE;
                               }

                               break;
    }
    case SPELLFAMILY_SHAMAN:
    {
                               switch (spellproto->Id)
                               {
                               case 63685:  // Freeze
                               case 64695:  // Earthgrab
                                   return DIMINISHING_CONTROLLED_ROOT;
                               case 118905: // Static Charge
                                   return DIMINISHING_CONTROLLED_STUN;
                               case 51514:  // Hex
                                   return DIMINISHING_DISORIENT;
                               case 77505:  // Earthquake
                                   return DIMINISHING_RANDOM_STUN;
                               case 76780:  // Bind Elemental
                                   return DIMINISHING_BIND_ELEMENTAL;
                               }

                               break;
    }
    default:
        break;
    }

    // Lastly - Set diminishing depending on mechanic
    uint32 mechanic = spellproto->GetAllEffectsMechanicMask();
    if (mechanic & (1 << MECHANIC_CHARM))
        return DIMINISHING_MIND_CONTROL;
    if (mechanic & (1 << MECHANIC_SILENCE))
        return DIMINISHING_SILENCE;
    if (mechanic & (1 << MECHANIC_SLEEP))
        return DIMINISHING_SLEEP;
    if (mechanic & ((1 << MECHANIC_SAPPED) | (1 << MECHANIC_POLYMORPH) | (1 << MECHANIC_SHACKLE)))
        return DIMINISHING_DISORIENT;
    // Mechanic Knockout, except Blast Wave
    if (mechanic & (1 << MECHANIC_KNOCKOUT) && spellproto->SpellIconID != 292)
        return DIMINISHING_DISORIENT;
    if (mechanic & (1 << MECHANIC_DISARM))
        return DIMINISHING_DISARM;
    if (mechanic & (1 << MECHANIC_FEAR))
        return DIMINISHING_FEAR;
    if (mechanic & (1 << MECHANIC_STUN))
        return triggered ? DIMINISHING_RANDOM_STUN : DIMINISHING_CONTROLLED_STUN;
    if (mechanic & (1 << MECHANIC_BANISH))
        return DIMINISHING_BANISH;
    if (mechanic & (1 << MECHANIC_ROOT))
        return triggered ? DIMINISHING_RANDOM_ROOT : DIMINISHING_CONTROLLED_ROOT;
    if (mechanic & (1 << MECHANIC_HORROR))
        return DIMINISHING_HORROR;

    return DIMINISHING_NONE;
}

DiminishingReturnsType GetDiminishingReturnsGroupType(DiminishingGroup group)
{
    switch (group)
    {
    case DIMINISHING_TAUNT:
    case DIMINISHING_CONTROLLED_STUN:
    case DIMINISHING_RANDOM_STUN:
    case DIMINISHING_OPENING_STUN:
    case DIMINISHING_CYCLONE:
    case DIMINISHING_CHARGE:
        return DRTYPE_ALL;
    case DIMINISHING_LIMITONLY:
    case DIMINISHING_NONE:
        return DRTYPE_NONE;
    default:
        return DRTYPE_PLAYER;
    }
}

DiminishingLevels GetDiminishingReturnsMaxLevel(DiminishingGroup group)
{
    switch (group)
    {
    case DIMINISHING_TAUNT:
        return DIMINISHING_LEVEL_TAUNT_IMMUNE;
    default:
        return DIMINISHING_LEVEL_IMMUNE;
    }
}

int32 GetDiminishingReturnsLimitDuration(DiminishingGroup group, SpellInfo const* spellproto)
{
    if (!IsDiminishingReturnsGroupDurationLimited(group))
        return 0;

    // Explicit diminishing duration
    switch (spellproto->SpellFamilyName)
    {
    case SPELLFAMILY_DRUID:
    {
                              switch (spellproto->Id)
                              {
                              case 770:   // Faerie Fire - limit to 20 seconds in PvP (5.4)
                                  return 20 * IN_MILLISECONDS;
                              case 102355:// Faerie Swarm - limit to 40 seconds in PvP (3.1)
                                  return 40 * IN_MILLISECONDS;
                              case 113770:// Entangling Roots - limit to 8 seconds in PvP (Force of Nature treant spell)
                                  return 8 * IN_MILLISECONDS;
                              default:
                                  break;
                              }

                              break;
    }
    case SPELLFAMILY_WARRIOR:
    {
                                switch (spellproto->Id)
                                {
                                case 12323:  // Piercing Howl 
                                case 137637: // Warbringer
                                    return 8 * IN_MILLISECONDS;
                                default:
                                    break;
                                }

                                break;
    }
    case SPELLFAMILY_HUNTER:
    {
                               // Wyvern Sting
                               if (spellproto->SpellFamilyFlags[1] & 0x1000)
                                   return 6 * IN_MILLISECONDS;
                               // Hunter's Mark
                               else if (spellproto->SpellFamilyFlags[0] & 0x400)
                                   return 20 * IN_MILLISECONDS;
                               // Widow Venom
                               else if (spellproto->Id == 82654)
                                   return 10 * IN_MILLISECONDS;
                               // Binding Shot dot
                               else if (spellproto->Id == 117526)
                                   return 3 * IN_MILLISECONDS;
                               break;
    }
    case SPELLFAMILY_PALADIN:
    {
                                // Repentance - limit to 6 seconds in PvP
                                if (spellproto->SpellFamilyFlags[0] & 0x4)
                                    return 6 * IN_MILLISECONDS;
                                // Turn Evil - limit to 8 seconds in PvP
                                else if (spellproto->Id == 145067 || spellproto->Id == 10326)
                                    return 8 * IN_MILLISECONDS;
                                break;
    }
    case SPELLFAMILY_WARLOCK:
    {
                                // Banish - limit to 6 seconds in PvP
                                if (spellproto->SpellFamilyFlags[1] & 0x8000000)
                                    return 6 * IN_MILLISECONDS;
                                // Curse of Tongues - limit to 12 seconds in PvP
                                else if (spellproto->SpellFamilyFlags[2] & 0x800)
                                    return 12 * IN_MILLISECONDS;
                                // Curse of Elements - limit to 120 seconds in PvP
                                else if (spellproto->SpellFamilyFlags[1] & 0x200)
                                    return 120 * IN_MILLISECONDS;
                                // Curse of Exhaustion
                                else if (spellproto->Id == 18223)
                                    return 10 * IN_MILLISECONDS;
                                break;
    }
    case SPELLFAMILY_MONK:
    {
                             // Disable (reduce movement speed) - limit to 8 seconds in PvP
                             if (spellproto->Id == 116095)
                                 return 8 * IN_MILLISECONDS;
                             // Disable (root) - limit to 4 seconds in PvP
                             else if (spellproto->Id == 116706)
                                 return 4 * IN_MILLISECONDS;
                             break;
    }
    case SPELLFAMILY_PRIEST:
    {
                               // Dominate Mind - limit to 8 seconds in PvP
                               if (spellproto->Id == 605)
                                   return 8 * IN_MILLISECONDS;
                               break;
    }
    default:
        break;
    }

    return 8 * IN_MILLISECONDS;
}

bool IsDiminishingReturnsGroupDurationLimited(DiminishingGroup group)
{
    switch (group)
    {
    case DIMINISHING_BANISH:
    case DIMINISHING_CONTROLLED_STUN:
    case DIMINISHING_CONTROLLED_ROOT:
    case DIMINISHING_CYCLONE:
    case DIMINISHING_DISORIENT:
    case DIMINISHING_ENTRAPMENT:
    case DIMINISHING_FEAR:
    case DIMINISHING_HORROR:
    case DIMINISHING_MIND_CONTROL:
    case DIMINISHING_OPENING_STUN:
    case DIMINISHING_RANDOM_ROOT:
    case DIMINISHING_RANDOM_STUN:
    case DIMINISHING_SLEEP:
    case DIMINISHING_LIMITONLY:
        return true;
    default:
        return false;
    }
}

SpellMgr::SpellMgr()
{
}

SpellMgr::~SpellMgr()
{
    UnloadSpellInfoStore();
}

/// Some checks for spells, to prevent adding deprecated/broken spells for trainers, spell book, etc
bool SpellMgr::IsSpellValid(SpellInfo const* spellInfo, Player* player, bool msg)
{
    // not exist
    if (!spellInfo)
        return false;

    bool need_check_reagents = false;

    // check effects
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        switch (spellInfo->Effects[i].Effect)
        {
        case 0:
            continue;

            // craft spell for crafting non-existed item (break client recipes list show)
        case SPELL_EFFECT_CREATE_ITEM:
        case SPELL_EFFECT_CREATE_ITEM_2:
        {
                                           if (spellInfo->Effects[i].ItemType == 0)
                                           {
                                               // skip auto-loot crafting spells, its not need explicit item info (but have special fake items sometime)
                                               if (!spellInfo->IsLootCrafting())
                                               {
                                                   if (msg)
                                                   {
                                                       if (player)
                                                           ChatHandler(player).PSendSysMessage("Craft spell %u not have create item entry.", spellInfo->Id);
                                                       else
                                                           sLog->outError(LOG_FILTER_SQL, "Craft spell %u not have create item entry.", spellInfo->Id);
                                                   }
                                                   return false;
                                               }

                                           }
                                           // also possible IsLootCrafting case but fake item must exist anyway
                                           else if (!sObjectMgr->GetItemTemplate(spellInfo->Effects[i].ItemType))
                                           {
                                               if (msg)
                                               {
                                                   if (player)
                                                       ChatHandler(player).PSendSysMessage("Craft spell %u create not-exist in DB item (Entry: %u) and then...", spellInfo->Id, spellInfo->Effects[i].ItemType);
                                                   else
                                                       sLog->outError(LOG_FILTER_SQL, "Craft spell %u create not-exist in DB item (Entry: %u) and then...", spellInfo->Id, spellInfo->Effects[i].ItemType);
                                               }
                                               return false;
                                           }

                                           need_check_reagents = true;
                                           break;
        }
        case SPELL_EFFECT_LEARN_SPELL:
        {
                                         SpellInfo const* spellInfo2 = sSpellMgr->GetSpellInfo(spellInfo->Effects[i].TriggerSpell);
                                         if (!IsSpellValid(spellInfo2, player, msg))
                                         {
                                             if (msg)
                                             {
                                                 if (player)
                                                     ChatHandler(player).PSendSysMessage("Spell %u learn to broken spell %u, and then...", spellInfo->Id, spellInfo->Effects[i].TriggerSpell);
                                                 else
                                                     sLog->outError(LOG_FILTER_SQL, "Spell %u learn to invalid spell %u, and then...", spellInfo->Id, spellInfo->Effects[i].TriggerSpell);
                                             }
                                             return false;
                                         }
                                         break;
        }
        }
    }

    if (need_check_reagents)
    {
        for (uint8 j = 0; j < MAX_SPELL_REAGENTS; ++j)
        {
            if (spellInfo->Reagent[j] > 0 && !sObjectMgr->GetItemTemplate(spellInfo->Reagent[j]))
            {
                if (msg)
                {
                    if (player)
                        ChatHandler(player).PSendSysMessage("Craft spell %u have not-exist reagent in DB item (Entry: %u) and then...", spellInfo->Id, spellInfo->Reagent[j]);
                    else
                        sLog->outError(LOG_FILTER_SQL, "Craft spell %u have not-exist reagent in DB item (Entry: %u) and then...", spellInfo->Id, spellInfo->Reagent[j]);
                }
                return false;
            }
        }
    }

    return true;
}

bool SpellMgr::IsSpellForbidden(uint32 spellid)
{
    std::list<uint32>::iterator Itr;

    for (Itr = mForbiddenSpells.begin(); Itr != mForbiddenSpells.end(); Itr++)
    if ((*Itr) == spellid)
        return true;

    return false;
}

uint32 SpellMgr::GetSpellDifficultyId(uint32 spellId) const
{
    SpellDifficultySearcherMap::const_iterator i = mSpellDifficultySearcherMap.find(spellId);
    return i == mSpellDifficultySearcherMap.end() ? 0 : (*i).second;
}

void SpellMgr::SetSpellDifficultyId(uint32 spellId, uint32 id)
{
    mSpellDifficultySearcherMap[spellId] = id;
}

uint32 SpellMgr::GetSpellIdForDifficulty(uint32 spellId, Unit const* caster) const
{
    // Dbc supprim閑 au passage a MoP
    return spellId;
    /*if (!GetSpellInfo(spellId))
    return spellId;

    if (!caster || !caster->GetMap() || !caster->GetMap()->IsDungeon())
    return spellId;

    uint32 mode = uint32(caster->GetMap()->GetSpawnMode());
    if (mode >= MAX_DIFFICULTY)
    {
    sLog->outError(LOG_FILTER_SPELLS_AURAS, "SpellMgr::GetSpellIdForDifficulty: Incorrect Difficulty for spell %u.", spellId);
    return spellId; //return source spell
    }

    uint32 difficultyId = GetSpellDifficultyId(spellId);
    if (!difficultyId)
    return spellId; //return source spell, it has only REGULAR_DIFFICULTY

    SpellDifficultyEntry const* difficultyEntry = sSpellDifficultyStore.LookupEntry(difficultyId);
    if (!difficultyEntry)
    {
    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SpellMgr::GetSpellIdForDifficulty: SpellDifficultyEntry not found for spell %u. This should never happen.", spellId);
    return spellId; //return source spell
    }

    if (difficultyEntry->SpellID[mode] <= 0 && mode > HEROIC_DIFFICULTY)
    {
    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SpellMgr::GetSpellIdForDifficulty: spell %u mode %u spell is NULL, using mode %u", spellId, mode, mode - 2);
    mode -= 2;
    }

    if (difficultyEntry->SpellID[mode] <= 0)
    {
    sLog->outError(LOG_FILTER_SQL, "SpellMgr::GetSpellIdForDifficulty: spell %u mode %u spell is 0. Check spelldifficulty_dbc!", spellId, mode);
    return spellId;
    }

    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "SpellMgr::GetSpellIdForDifficulty: spellid for spell %u in mode %u is %d", spellId, mode, difficultyEntry->SpellID[mode]);
    return uint32(difficultyEntry->SpellID[mode]);*/
}

SpellInfo const* SpellMgr::GetSpellForDifficultyFromSpell(SpellInfo const* spell, Unit const* caster) const
{
    if (!spell)
        return NULL;

    if (!caster || !caster->GetMap() || !caster->GetMap()->IsDungeon())
        return spell;

    uint32 mode = uint32(caster->GetMap()->GetSpawnMode());
    return GetSpellInfo(spell->Id, (Difficulty)mode);
}

SpellChainNode const* SpellMgr::GetSpellChainNode(uint32 spellId) const
{
    SpellChainMap::const_iterator itr = mSpellChains.find(spellId);
    if (itr == mSpellChains.end())
        return NULL;

    return &itr->second;
}

uint32 SpellMgr::GetFirstSpellInChain(uint32 spellId) const
{
    if (SpellChainNode const* node = GetSpellChainNode(spellId))
        return node->first->Id;

    return spellId;
}

uint32 SpellMgr::GetLastSpellInChain(uint32 spellId) const
{
    if (SpellChainNode const* node = GetSpellChainNode(spellId))
        return node->last->Id;

    return spellId;
}

uint32 SpellMgr::GetNextSpellInChain(uint32 spellId) const
{
    if (SpellChainNode const* node = GetSpellChainNode(spellId))
    if (node->next)
        return node->next->Id;

    return 0;
}

uint32 SpellMgr::GetPrevSpellInChain(uint32 spellId) const
{
    if (SpellChainNode const* node = GetSpellChainNode(spellId))
    if (node->prev)
        return node->prev->Id;

    return 0;
}

uint8 SpellMgr::GetSpellRank(uint32 spellId) const
{
    if (SpellChainNode const* node = GetSpellChainNode(spellId))
        return node->rank;

    return 0;
}

uint32 SpellMgr::GetSpellWithRank(uint32 spellId, uint32 rank, bool strict) const
{
    if (SpellChainNode const* node = GetSpellChainNode(spellId))
    {
        if (rank != node->rank)
            return GetSpellWithRank(node->rank < rank ? node->next->Id : node->prev->Id, rank, strict);
    }
    else if (strict && rank > 1)
        return 0;
    return spellId;
}

SpellRequiredMapBounds SpellMgr::GetSpellsRequiredForSpellBounds(uint32 spellId) const
{
    return SpellRequiredMapBounds(mSpellReq.lower_bound(spellId), mSpellReq.upper_bound(spellId));
}

SpellsRequiringSpellMapBounds SpellMgr::GetSpellsRequiringSpellBounds(uint32 spellId) const
{
    return SpellsRequiringSpellMapBounds(mSpellsReqSpell.lower_bound(spellId), mSpellsReqSpell.upper_bound(spellId));
}

bool SpellMgr::IsSpellRequiringSpell(uint32 spellid, uint32 req_spellid) const
{
    SpellsRequiringSpellMapBounds spellsRequiringSpell = GetSpellsRequiringSpellBounds(req_spellid);
    for (SpellsRequiringSpellMap::const_iterator itr = spellsRequiringSpell.first; itr != spellsRequiringSpell.second; ++itr)
    {
        if (itr->second == spellid)
            return true;
    }
    return false;
}

const SpellsRequiringSpellMap SpellMgr::GetSpellsRequiringSpell()
{
    return this->mSpellsReqSpell;
}

uint32 SpellMgr::GetSpellRequired(uint32 spellId) const
{
    SpellRequiredMap::const_iterator itr = mSpellReq.find(spellId);

    if (itr == mSpellReq.end())
        return 0;

    return itr->second;
}

SpellLearnSkillNode const* SpellMgr::GetSpellLearnSkill(uint32 spellId) const
{
    SpellLearnSkillMap::const_iterator itr = mSpellLearnSkills.find(spellId);
    if (itr != mSpellLearnSkills.end())
        return &itr->second;
    else
        return NULL;
}

SpellLearnSpellMapBounds SpellMgr::GetSpellLearnSpellMapBounds(uint32 spellId) const
{
    return SpellLearnSpellMapBounds(mSpellLearnSpells.lower_bound(spellId), mSpellLearnSpells.upper_bound(spellId));
}

bool SpellMgr::IsSpellLearnSpell(uint32 spellId) const
{
    return mSpellLearnSpells.find(spellId) != mSpellLearnSpells.end();
}

bool SpellMgr::IsSpellLearnToSpell(uint32 spellId1, uint32 spellId2) const
{
    SpellLearnSpellMapBounds bounds = GetSpellLearnSpellMapBounds(spellId1);
    for (SpellLearnSpellMap::const_iterator i = bounds.first; i != bounds.second; ++i)
    if (i->second.spell == spellId2)
        return true;
    return false;
}

SpellTargetPosition const* SpellMgr::GetSpellTargetPosition(uint32 spellId, SpellEffIndex effIndex) const
{
    SpellTargetPositionMap::const_iterator itr = mSpellTargetPositions.find(std::make_pair(spellId, effIndex));
    if (itr != mSpellTargetPositions.end())
        return &itr->second;
    return NULL;
}

SpellSpellGroupMapBounds SpellMgr::GetSpellSpellGroupMapBounds(uint32 spellId) const
{
    spellId = GetFirstSpellInChain(spellId);
    return SpellSpellGroupMapBounds(mSpellSpellGroup.lower_bound(spellId), mSpellSpellGroup.upper_bound(spellId));
}

uint32 SpellMgr::IsSpellMemberOfSpellGroup(uint32 spellid, SpellGroup groupid) const
{
    SpellSpellGroupMapBounds spellGroup = GetSpellSpellGroupMapBounds(spellid);
    for (SpellSpellGroupMap::const_iterator itr = spellGroup.first; itr != spellGroup.second; ++itr)
    {
        if (itr->second == groupid)
            return true;
    }
    return false;
}

SpellGroupSpellMapBounds SpellMgr::GetSpellGroupSpellMapBounds(SpellGroup group_id) const
{
    return SpellGroupSpellMapBounds(mSpellGroupSpell.lower_bound(group_id), mSpellGroupSpell.upper_bound(group_id));
}

void SpellMgr::GetSetOfSpellsInSpellGroup(SpellGroup group_id, std::set<uint32>& foundSpells) const
{
    std::set<SpellGroup> usedGroups;
    GetSetOfSpellsInSpellGroup(group_id, foundSpells, usedGroups);
}

void SpellMgr::GetSetOfSpellsInSpellGroup(SpellGroup group_id, std::set<uint32>& foundSpells, std::set<SpellGroup>& usedGroups) const
{
    if (usedGroups.find(group_id) != usedGroups.end())
        return;
    usedGroups.insert(group_id);

    SpellGroupSpellMapBounds groupSpell = GetSpellGroupSpellMapBounds(group_id);
    for (SpellGroupSpellMap::const_iterator itr = groupSpell.first; itr != groupSpell.second; ++itr)
    {
        if (itr->second < 0)
        {
            SpellGroup currGroup = (SpellGroup)abs(itr->second);
            GetSetOfSpellsInSpellGroup(currGroup, foundSpells, usedGroups);
        }
        else
        {
            foundSpells.insert(itr->second);
        }
    }
}

bool SpellMgr::AddSameEffectStackRuleSpellGroups(SpellInfo const* spellInfo, int32 amount, std::map<SpellGroup, int32>& groups, AuraType auraType) const
{
    uint32 spellId = spellInfo->GetFirstRankSpell()->Id;
    SpellSpellGroupMapBounds spellGroup = GetSpellSpellGroupMapBounds(spellId);
    // Find group with SPELL_GROUP_STACK_RULE_EXCLUSIVE_SAME_EFFECT if it belongs to one
    for (SpellSpellGroupMap::const_iterator itr = spellGroup.first; itr != spellGroup.second; ++itr)
    {
        SpellGroup group = itr->second;
        SpellGroupStackMap::const_iterator found = mSpellGroupStack.find(group);
        if (found != mSpellGroupStack.end())
        {
            if (found->second.stackRule == SPELL_GROUP_STACK_RULE_EXCLUSIVE_SAME_EFFECT &&
                (!found->second.auraType || found->second.auraType == auraType))
            {
                // Put the highest amount in the map
                if (groups.find(group) == groups.end())
                    groups[group] = amount;
                else
                {
                    int32 curr_amount = groups[group];
                    // Take absolute value because this also counts for the highest negative aura
                    if (abs(curr_amount) < abs(amount))
                        groups[group] = amount;
                }
                // return because a spell should be in only one SPELL_GROUP_STACK_RULE_EXCLUSIVE_SAME_EFFECT group
                return true;
            }
        }
    }
    // Not in a SPELL_GROUP_STACK_RULE_EXCLUSIVE_SAME_EFFECT group, so return false
    return false;
}

SpellGroupStackRule SpellMgr::CheckSpellGroupStackRules(SpellInfo const* spellInfo1, SpellInfo const* spellInfo2) const
{
    uint32 spellid_1 = spellInfo1->GetFirstRankSpell()->Id;
    uint32 spellid_2 = spellInfo2->GetFirstRankSpell()->Id;
    if (spellid_1 == spellid_2)
        return SPELL_GROUP_STACK_RULE_DEFAULT;
    // find SpellGroups which are common for both spells
    SpellSpellGroupMapBounds spellGroup1 = GetSpellSpellGroupMapBounds(spellid_1);
    std::set<SpellGroup> groups;
    for (SpellSpellGroupMap::const_iterator itr = spellGroup1.first; itr != spellGroup1.second; ++itr)
    {
        if (IsSpellMemberOfSpellGroup(spellid_2, itr->second))
        {
            bool add = true;
            SpellGroupSpellMapBounds groupSpell = GetSpellGroupSpellMapBounds(itr->second);
            for (SpellGroupSpellMap::const_iterator itr2 = groupSpell.first; itr2 != groupSpell.second; ++itr2)
            {
                if (itr2->second < 0)
                {
                    SpellGroup currGroup = (SpellGroup)abs(itr2->second);
                    if (IsSpellMemberOfSpellGroup(spellid_1, currGroup) && IsSpellMemberOfSpellGroup(spellid_2, currGroup))
                    {
                        add = false;
                        break;
                    }
                }
            }
            if (add)
                groups.insert(itr->second);
        }
    }

    SpellGroupStackRule rule = SPELL_GROUP_STACK_RULE_DEFAULT;

    for (std::set<SpellGroup>::iterator itr = groups.begin(); itr != groups.end(); ++itr)
    {
        SpellGroupStackMap::const_iterator found = mSpellGroupStack.find(*itr);
        if (found != mSpellGroupStack.end())
            rule = found->second.stackRule;
        if (rule)
            break;
    }
    return rule;
}

SpellGroupStackRule SpellMgr::GetSpellGroupStackRule(SpellGroup group) const
{
    SpellGroupStackMap::const_iterator itr = mSpellGroupStack.find(group);
    if (itr != mSpellGroupStack.end())
        return itr->second.stackRule;

    return SPELL_GROUP_STACK_RULE_DEFAULT;
}

SpellProcEventEntry const* SpellMgr::GetSpellProcEvent(uint32 spellId) const
{
    SpellProcEventMap::const_iterator itr = mSpellProcEventMap.find(spellId);
    if (itr != mSpellProcEventMap.end())
        return &itr->second;
    return NULL;
}

bool SpellMgr::IsSpellProcEventCanTriggeredBy(SpellProcEventEntry const* spellProcEvent, uint32 EventProcFlag, SpellInfo const* procSpell, uint32 procFlags, uint32 procExtra, bool active)
{
    // No extra req need
    uint32 procEvent_procEx = PROC_EX_NONE;

    // check prockFlags for condition
    if ((procFlags & EventProcFlag) == 0)
        return false;

    bool hasFamilyMask = false;

    /* Check Periodic Auras

    *Dots can trigger if spell has no PROC_FLAG_SUCCESSFUL_NEGATIVE_MAGIC_SPELL
    nor PROC_FLAG_TAKEN_POSITIVE_MAGIC_SPELL

    *Only Hots can trigger if spell has PROC_FLAG_TAKEN_POSITIVE_MAGIC_SPELL

    *Only dots can trigger if spell has both positivity flags or PROC_FLAG_SUCCESSFUL_NEGATIVE_MAGIC_SPELL

    *Aura has to have PROC_FLAG_TAKEN_POSITIVE_MAGIC_SPELL or spellfamily specified to trigger from Hot

    */

    if (procFlags & PROC_FLAG_DONE_PERIODIC)
    {
        if (EventProcFlag & PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG)
        {
            if (!(procExtra & PROC_EX_INTERNAL_DOT))
                return false;
        }
        else if (procExtra & PROC_EX_INTERNAL_HOT)
            procExtra |= PROC_EX_INTERNAL_REQ_FAMILY;
        else if (EventProcFlag & PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS)
            return false;
    }

    if (procFlags & PROC_FLAG_TAKEN_PERIODIC)
    {
        if (EventProcFlag & PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_POS)
        {
            if (!(procExtra & PROC_EX_INTERNAL_HOT))
                return false;
        }
        else if (procExtra & PROC_EX_INTERNAL_HOT)
            procExtra |= PROC_EX_INTERNAL_REQ_FAMILY;
        else if (EventProcFlag & PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_POS)
            return false;
    }
    // Trap casts are active by default
    if (procFlags & PROC_FLAG_DONE_TRAP_ACTIVATION)
        active = true;

    // Always trigger for this
    if (procFlags & (PROC_FLAG_KILLED | PROC_FLAG_KILL | PROC_FLAG_DEATH))
        return true;

    if (spellProcEvent)     // Exist event data
    {
        // Store extra req
        procEvent_procEx = spellProcEvent->procEx;

        // For melee triggers
        if (procSpell == NULL)
        {
            // Check (if set) for school (melee attack have Normal school)
            if (spellProcEvent->schoolMask && (spellProcEvent->schoolMask & SPELL_SCHOOL_MASK_NORMAL) == 0)
                return false;
        }
        else // For spells need check school/spell family/family mask
        {
            // Check (if set) for school
            if (spellProcEvent->schoolMask && (spellProcEvent->schoolMask & procSpell->SchoolMask) == 0)
                return false;

            // Check (if set) for spellFamilyName
            if (spellProcEvent->spellFamilyName && (spellProcEvent->spellFamilyName != procSpell->SpellFamilyName))
                return false;

            // spellFamilyName is Ok need check for spellFamilyMask if present
            if (spellProcEvent->spellFamilyMask)
            {
                if (!(spellProcEvent->spellFamilyMask & procSpell->SpellFamilyFlags))
                    return false;
                hasFamilyMask = true;
                // Some spells are not considered as active even with have spellfamilyflags
                if (!(procEvent_procEx & PROC_EX_ONLY_ACTIVE_SPELL))
                    active = true;
            }
        }
    }

    if (procExtra & (PROC_EX_INTERNAL_REQ_FAMILY))
    {
        if (!hasFamilyMask)
            return false;
    }

    // Check for extra req (if none) and hit/crit
    if (procEvent_procEx == PROC_EX_NONE)
    {
        // No extra req, so can trigger only for hit/crit - spell has to be active
        if ((procExtra & (PROC_EX_NORMAL_HIT | PROC_EX_CRITICAL_HIT)) && active)
            return true;
    }
    else // Passive spells hits here only if resist/reflect/immune/evade
    {
        if (procExtra & AURA_SPELL_PROC_EX_MASK)
        {
            // if spell marked as procing only from not active spells
            if (active && procEvent_procEx & PROC_EX_NOT_ACTIVE_SPELL)
                return false;
            // if spell marked as procing only from active spells
            if (!active && procEvent_procEx & PROC_EX_ONLY_ACTIVE_SPELL)
                return false;
            // Exist req for PROC_EX_EX_TRIGGER_ALWAYS
            if (procEvent_procEx & PROC_EX_EX_TRIGGER_ALWAYS)
                return true;
            // PROC_EX_NOT_ACTIVE_SPELL and PROC_EX_ONLY_ACTIVE_SPELL flags handle: if passed checks before
            if ((procExtra & (PROC_EX_NORMAL_HIT | PROC_EX_CRITICAL_HIT)) && ((procEvent_procEx & (AURA_SPELL_PROC_EX_MASK)) == 0))
                return true;
        }
        // Check Extra Requirement like (hit/crit/miss/resist/parry/dodge/block/immune/reflect/absorb and other)
        if (procEvent_procEx & procExtra)
            return true;
    }
    return false;
}

SpellProcEntry const* SpellMgr::GetSpellProcEntry(uint32 spellId) const
{
    SpellProcMap::const_iterator itr = mSpellProcMap.find(spellId);
    if (itr != mSpellProcMap.end())
        return &itr->second;
    return NULL;
}

bool SpellMgr::CanSpellTriggerProcOnEvent(SpellProcEntry const& procEntry, ProcEventInfo& eventInfo)
{
    // proc type doesn't match
    if (!(eventInfo.GetTypeMask() & procEntry.typeMask))
        return false;

    // check XP or honor target requirement
    if (procEntry.attributesMask & PROC_ATTR_REQ_EXP_OR_HONOR)
    if (Player* actor = eventInfo.GetActor()->ToPlayer())
    if (eventInfo.GetActionTarget() && !actor->isHonorOrXPTarget(eventInfo.GetActionTarget()))
        return false;

    // always trigger for these types
    if (eventInfo.GetTypeMask() & (PROC_FLAG_KILLED | PROC_FLAG_KILL | PROC_FLAG_DEATH))
        return true;

    // check school mask (if set) for other trigger types
    if (procEntry.schoolMask && !(eventInfo.GetSchoolMask() & procEntry.schoolMask))
        return false;

    // check spell family name/flags (if set) for spells
    if (eventInfo.GetTypeMask() & (PERIODIC_PROC_FLAG_MASK | SPELL_PROC_FLAG_MASK | PROC_FLAG_DONE_TRAP_ACTIVATION))
    {
        if (procEntry.spellFamilyName && (procEntry.spellFamilyName != eventInfo.GetSpellInfo()->SpellFamilyName))
            return false;

        if (procEntry.spellFamilyMask && !(procEntry.spellFamilyMask & eventInfo.GetSpellInfo()->SpellFamilyFlags))
            return false;
    }

    // check spell type mask (if set)
    if (eventInfo.GetTypeMask() & (SPELL_PROC_FLAG_MASK | PERIODIC_PROC_FLAG_MASK))
    {
        if (procEntry.spellTypeMask && !(eventInfo.GetSpellTypeMask() & procEntry.spellTypeMask))
            return false;
    }

    // check spell phase mask
    if (eventInfo.GetTypeMask() & REQ_SPELL_PHASE_PROC_FLAG_MASK)
    {
        if (!(eventInfo.GetSpellPhaseMask() & procEntry.spellPhaseMask))
            return false;
    }

    // check hit mask (on taken hit or on done hit, but not on spell cast phase)
    if ((eventInfo.GetTypeMask() & TAKEN_HIT_PROC_FLAG_MASK) || ((eventInfo.GetTypeMask() & DONE_HIT_PROC_FLAG_MASK) && !(eventInfo.GetSpellPhaseMask() & PROC_SPELL_PHASE_CAST)))
    {
        uint32 hitMask = procEntry.hitMask;
        // get default values if hit mask not set
        if (!hitMask)
        {
            // for taken procs allow normal + critical hits by default
            if (eventInfo.GetTypeMask() & TAKEN_HIT_PROC_FLAG_MASK)
                hitMask |= PROC_HIT_NORMAL | PROC_HIT_CRITICAL;
            // for done procs allow normal + critical + absorbs by default
            else
                hitMask |= PROC_HIT_NORMAL | PROC_HIT_CRITICAL | PROC_HIT_ABSORB;
        }
        if (!(eventInfo.GetHitMask() & hitMask))
            return false;
    }

    return true;
}

SpellBonusEntry const* SpellMgr::GetSpellBonusData(uint32 spellId) const
{
    // Lookup data
    SpellBonusMap::const_iterator itr = mSpellBonusMap.find(spellId);
    if (itr != mSpellBonusMap.end())
        return &itr->second;
    // Not found, try lookup for 1 spell rank if exist
    if (uint32 rank_1 = GetFirstSpellInChain(spellId))
    {
        SpellBonusMap::const_iterator itr2 = mSpellBonusMap.find(rank_1);
        if (itr2 != mSpellBonusMap.end())
            return &itr2->second;
    }
    return NULL;
}

SpellThreatEntry const* SpellMgr::GetSpellThreatEntry(uint32 spellID) const
{
    SpellThreatMap::const_iterator itr = mSpellThreatMap.find(spellID);
    if (itr != mSpellThreatMap.end())
        return &itr->second;
    else
    {
        uint32 firstSpell = GetFirstSpellInChain(spellID);
        itr = mSpellThreatMap.find(firstSpell);
        if (itr != mSpellThreatMap.end())
            return &itr->second;
    }
    return NULL;
}

SkillLineAbilityMapBounds SpellMgr::GetSkillLineAbilityMapBounds(uint32 spellId) const
{
    return SkillLineAbilityMapBounds(mSkillLineAbilityMap.lower_bound(spellId), mSkillLineAbilityMap.upper_bound(spellId));
}

PetAura const* SpellMgr::GetPetAura(uint32 spellId, uint8 eff)
{
    SpellPetAuraMap::const_iterator itr = mSpellPetAuraMap.find((spellId << 8) + eff);
    if (itr != mSpellPetAuraMap.end())
        return &itr->second;
    else
        return NULL;
}

SpellEnchantProcEntry const* SpellMgr::GetSpellEnchantProcEvent(uint32 enchId) const
{
    SpellEnchantProcEventMap::const_iterator itr = mSpellEnchantProcEventMap.find(enchId);
    if (itr != mSpellEnchantProcEventMap.end())
        return &itr->second;
    return NULL;
}

bool SpellMgr::IsArenaAllowedEnchancment(uint32 ench_id) const
{
    return mEnchantCustomAttr[ench_id];
}

const std::vector<int32>* SpellMgr::GetSpellLinked(int32 spellId) const
{
    SpellLinkedMap::const_iterator itr = mSpellLinkedMap.find(spellId);
    return itr != mSpellLinkedMap.end() ? &(itr->second) : NULL;
}

PetLevelupSpellSet const* SpellMgr::GetPetLevelupSpellList(uint32 petFamily) const
{
    PetLevelupSpellMap::const_iterator itr = mPetLevelupSpellMap.find(petFamily);
    if (itr != mPetLevelupSpellMap.end())
        return &itr->second;
    else
        return NULL;
}

PetDefaultSpellsEntry const* SpellMgr::GetPetDefaultSpellsEntry(int32 id) const
{
    PetDefaultSpellsMap::const_iterator itr = mPetDefaultSpellsMap.find(id);
    if (itr != mPetDefaultSpellsMap.end())
        return &itr->second;
    return NULL;
}

SpellAreaMapBounds SpellMgr::GetSpellAreaMapBounds(uint32 spellId) const
{
    return mSpellAreaMap.equal_range(spellId);
}

SpellAreaForQuestMapBounds SpellMgr::GetSpellAreaForQuestMapBounds(uint32 quest_id) const
{
    return mSpellAreaForQuestMap.equal_range(quest_id);
}

SpellAreaForQuestMapBounds SpellMgr::GetSpellAreaForQuestEndMapBounds(uint32 quest_id) const
{
    return mSpellAreaForQuestEndMap.equal_range(quest_id);
}

SpellAreaForAuraMapBounds SpellMgr::GetSpellAreaForAuraMapBounds(uint32 spellId) const
{
    return mSpellAreaForAuraMap.equal_range(spellId);
}

SpellAreaForAreaMapBounds SpellMgr::GetSpellAreaForAreaMapBounds(uint32 area_id) const
{
    return mSpellAreaForAreaMap.equal_range(area_id);
}

bool SpellArea::IsFitToRequirements(Player const* player, uint32 newZone, uint32 newArea) const
{
    if (gender != GENDER_NONE)                   // not in expected gender
    if (!player || gender != player->getGender())
        return false;

    if (raceMask)                                // not in expected race
    if (!player || !(raceMask & player->getRaceMask()))
        return false;

    if (areaId)                                  // not in expected zone
    if (newZone != areaId && newArea != areaId)
        return false;

    if (questStart)                              // not in expected required quest state
    if (!player || ((questStartStatus & (1 << player->GetQuestStatus(questStart))) == 0))
        return false;

    if (questEnd)                                // not in expected forbidden quest state
    if (!player || (questEndStatus & (1 << player->GetQuestStatus(questEnd))))
        return false;

    if (auraSpell)                               // not have expected aura
    if (!player || (auraSpell > 0 && !player->HasAura(auraSpell)) || (auraSpell < 0 && player->HasAura(-auraSpell)))
        return false;

    // Extra conditions -- leaving the possibility add extra conditions...
    switch (spellId)
    {
    case 58600: // No fly Zone - Dalaran
    {
                    if (!player)
                        return false;

                    AreaTableEntry const* pArea = GetAreaEntryByAreaID(player->GetAreaId());
                    if (!(pArea && pArea->flags & AREA_FLAG_NO_FLY_ZONE))
                        return false;
                    if (!player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) && !player->HasAuraType(SPELL_AURA_FLY))
                        return false;
                    break;
    }
    case 58730: // No fly Zone - Wintergrasp
    case 91604: // No fly Zone - Wintergrasp
    {
                    if (!player)
                        return false;

                    Battlefield* Bf = sBattlefieldMgr->GetBattlefieldToZoneId(player->GetZoneId());
                    if (!Bf || Bf->CanFlyIn() || (!player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) && !player->HasAuraType(SPELL_AURA_FLY)))
                        return false;
                    break;
    }
    case 68719: // Oil Refinery - Isle of Conquest.
    case 68720: // Quarry - Isle of Conquest.
    {
                    if (!player || player->GetBattlegroundTypeId() != BATTLEGROUND_IC || !player->GetBattleground())
                        return false;

                    uint8 nodeType = spellId == 68719 ? NODE_TYPE_REFINERY : NODE_TYPE_QUARRY;
                    uint8 nodeState = player->GetTeamId() == TEAM_ALLIANCE ? NODE_STATE_CONTROLLED_A : NODE_STATE_CONTROLLED_H;

                    BattlegroundIC* pIC = static_cast<BattlegroundIC*>(player->GetBattleground());
                    if (pIC->GetNodeState(nodeType) == nodeState)
                        return true;

                    return false;
    }
    case 56618: // Horde Controls Factory Phase Shift
    case 56617: // Alliance Controls Factory Phase Shift
    {
                    if (!player)
                        return false;

                    Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(player->GetZoneId());

                    if (!bf || bf->GetTypeId() != BATTLEFIELD_WG)
                        return false;

                    // team that controls the workshop in the specified area
                    uint32 team = bf->GetData(newArea);

                    if (team == TEAM_HORDE)
                        return spellId == 56618;
                    else if (team == TEAM_ALLIANCE)
                        return spellId == 56617;
                    break;
    }
    case 57940: // Essence of Wintergrasp - Northrend
    case 58045: // Essence of Wintergrasp - Wintergrasp
    {
                    if (!player)
                        return false;

                    if (Battlefield* battlefieldWG = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG))
                        return battlefieldWG->IsEnabled() && (player->GetTeamId() == battlefieldWG->GetDefenderTeam()) && !battlefieldWG->IsWarTime();
                    break;
    }
    case 74411: // Battleground - Dampening
    {
                    if (!player)
                        return false;

                    if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(player->GetZoneId()))
                        return bf->IsWarTime();
                    break;
    }
    case 73822: // Hellscream's Warsong
    case 73828: // Strength of Wrynn
    {
                    if (!player)
                        return false;

                    InstanceScript* instanceScript = ((Player*)player)->GetInstanceScript();
                    if (!instanceScript)
                        return false;

                    if (instanceScript->GetData(41) == 3) // 41 - DATA_BUFF_REMOVED
                        return false;
                    break;
    }
    }

    return true;
}

void SpellMgr::LoadSpellRanks()
{
    uint32 oldMSTime = getMSTime();

    // cleanup core data before reload - remove reference to ChainNode from SpellInfo
    for (SpellChainMap::iterator itr = mSpellChains.begin(); itr != mSpellChains.end(); ++itr)
    {
        for (int difficulty = 0; difficulty < MAX_DIFFICULTY; difficulty++)
        {
            if (mSpellInfoMap[difficulty][itr->first])
                mSpellInfoMap[difficulty][itr->first]->ChainEntry = NULL;
        }
    }
    mSpellChains.clear();
    //                                                     0             1      2
    QueryResult result = WorldDatabase.Query("SELECT first_spellId, spellId, spell_rank from spell_ranks ORDER BY first_spellId, spell_rank");

    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 spell rank records. DB table `spell_ranks` is empty.");

        return;
    }

    uint32 count = 0;
    bool finished = false;

    do
    {
        // spellid, rank
        std::list < std::pair < int32, int32 > > rankChain;
        int32 currentSpell = -1;
        int32 lastSpell = -1;

        // fill one chain
        while (currentSpell == lastSpell && !finished)
        {
            Field* fields = result->Fetch();

            currentSpell = fields[0].GetUInt32();
            if (lastSpell == -1)
                lastSpell = currentSpell;
            uint32 spellId = fields[1].GetUInt32();
            uint32 rank = fields[2].GetUInt8();

            // don't drop the row if we're moving to the next rank
            if (currentSpell == lastSpell)
            {
                rankChain.push_back(std::make_pair(spellId, rank));
                if (!result->NextRow())
                    finished = true;
            }
            else
                break;
        }
        // check if chain is made with valid first spell
        SpellInfo const* first = GetSpellInfo(lastSpell);
        if (!first)
        {
            sLog->outError(LOG_FILTER_SQL, "Spell rank identifier(first_spellId) %u listed in `spell_ranks` does not exist!", lastSpell);
            continue;
        }
        // check if chain is long enough
        if (rankChain.size() < 2)
        {
            sLog->outError(LOG_FILTER_SQL, "There is only 1 spell rank for identifier(first_spellId) %u in `spell_ranks`, entry is not needed!", lastSpell);
            continue;
        }
        int32 curRank = 0;
        bool valid = true;
        // check spells in chain
        for (std::list<std::pair<int32, int32> >::iterator itr = rankChain.begin(); itr != rankChain.end(); ++itr)
        {
            SpellInfo const* spell = GetSpellInfo(itr->first);
            if (!spell)
            {
                sLog->outError(LOG_FILTER_SQL, "Spell %u (rank %u) listed in `spell_ranks` for chain %u does not exist!", itr->first, itr->second, lastSpell);
                valid = false;
                break;
            }
            ++curRank;
            if (itr->second != curRank)
            {
                sLog->outError(LOG_FILTER_SQL, "Spell %u (rank %u) listed in `spell_ranks` for chain %u does not have proper rank value(should be %u)!", itr->first, itr->second, lastSpell, curRank);
                valid = false;
                break;
            }
        }
        if (!valid)
            continue;
        int32 prevRank = 0;
        // insert the chain
        std::list<std::pair<int32, int32> >::iterator itr = rankChain.begin();
        do
        {
            ++count;
            int32 addedSpell = itr->first;
            mSpellChains[addedSpell].first = GetSpellInfo(lastSpell);
            mSpellChains[addedSpell].last = GetSpellInfo(rankChain.back().first);
            mSpellChains[addedSpell].rank = itr->second;
            mSpellChains[addedSpell].prev = GetSpellInfo(prevRank);
            for (int difficulty = 0; difficulty < MAX_DIFFICULTY; difficulty++)
            if (mSpellInfoMap[difficulty][addedSpell])
                mSpellInfoMap[difficulty][addedSpell]->ChainEntry = &mSpellChains[addedSpell];
            prevRank = addedSpell;
            ++itr;
            if (itr == rankChain.end())
            {
                mSpellChains[addedSpell].next = NULL;
                break;
            }
            else
                mSpellChains[addedSpell].next = GetSpellInfo(itr->first);
        } while (true);
    } while
        (!finished);

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u spell rank records in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

}

void SpellMgr::LoadSpellRequired()
{
    uint32 oldMSTime = getMSTime();

    mSpellsReqSpell.clear();                                   // need for reload case
    mSpellReq.clear();                                         // need for reload case

    //                                                   0        1
    QueryResult result = WorldDatabase.Query("SELECT spellId, req_spell from spell_required");

    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 spell required records. DB table `spell_required` is empty.");

        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 spellId = fields[0].GetUInt32();
        uint32 spell_req = fields[1].GetUInt32();

        // check if chain is made with valid first spell
        SpellInfo const* spell = GetSpellInfo(spellId);
        if (!spell)
        {
            sLog->outError(LOG_FILTER_SQL, "spellId %u in `spell_required` table is not found in dbcs, skipped", spellId);
            continue;
        }

        SpellInfo const* req_spell = GetSpellInfo(spell_req);
        if (!req_spell)
        {
            sLog->outError(LOG_FILTER_SQL, "req_spell %u in `spell_required` table is not found in dbcs, skipped", spell_req);
            continue;
        }

        if (GetFirstSpellInChain(spellId) == GetFirstSpellInChain(spell_req))
        {
            sLog->outError(LOG_FILTER_SQL, "req_spell %u and spellId %u in `spell_required` table are ranks of the same spell, entry not needed, skipped", spell_req, spellId);
            continue;
        }

        if (IsSpellRequiringSpell(spellId, spell_req))
        {
            sLog->outError(LOG_FILTER_SQL, "duplicated entry of req_spell %u and spellId %u in `spell_required`, skipped", spell_req, spellId);
            continue;
        }

        mSpellReq.insert(std::pair<uint32, uint32>(spellId, spell_req));
        mSpellsReqSpell.insert(std::pair<uint32, uint32>(spell_req, spellId));
        ++count;
    } while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u spell required records in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

}

void SpellMgr::LoadSpellLearnSkills()
{
    uint32 oldMSTime = getMSTime();

    mSpellLearnSkills.clear();                              // need for reload case

    // search auto-learned skills and add its to map also for use in unlearn spells/talents
    uint32 dbc_count = 0;
    for (uint32 spell = 0; spell < GetSpellInfoStoreSize(); ++spell)
    {
        SpellInfo const* entry = GetSpellInfo(spell);

        if (!entry)
            continue;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (entry->Effects[i].Effect == SPELL_EFFECT_SKILL)
            {
                SpellLearnSkillNode dbc_node;
                dbc_node.skill = entry->Effects[i].MiscValue;
                dbc_node.step = entry->Effects[i].CalcValue();
                if (dbc_node.skill != SKILL_RIDING)
                    dbc_node.value = 1;
                else
                    dbc_node.value = dbc_node.step * 75;
                dbc_node.maxvalue = dbc_node.step * 75;
                mSpellLearnSkills[spell] = dbc_node;
                ++dbc_count;
                break;
            }
        }
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u Spell Learn Skills from DBC in %u ms", dbc_count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellLearnSpells()
{
    uint32 oldMSTime = getMSTime();

    mSpellLearnSpells.clear();                              // need for reload case

    //                                                  0      1        2
    QueryResult result = WorldDatabase.Query("SELECT entry, SpellID, Active FROM spell_learn_spell");
    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 spell learn spells. DB table `spell_learn_spell` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 spellId = fields[0].GetUInt32();

        SpellLearnSpellNode node;
        node.spell = fields[1].GetUInt32();
        node.active = fields[2].GetBool();
        node.autoLearned = false;

        if (!GetSpellInfo(spellId))
        {
            sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_learn_spell` does not exist", spellId);
            continue;
        }

        if (!GetSpellInfo(node.spell))
        {
            sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_learn_spell` learning not existed spell %u", spellId, node.spell);
            continue;
        }

        /*if (GetTalentSpellCost(node.spell))
        {
        sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_learn_spell` attempt learning talent spell %u, skipped", spellId, node.spell);
        continue;
        }*/

        mSpellLearnSpells.insert(SpellLearnSpellMap::value_type(spellId, node));

        ++count;
    } while (result->NextRow());

    // search auto-learned spells and add its to map also for use in unlearn spells/talents
    uint32 dbc_count = 0;
    for (uint32 spell = 0; spell < GetSpellInfoStoreSize(); ++spell)
    {
        SpellInfo const* entry = GetSpellInfo(spell);

        if (!entry)
            continue;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (entry->Effects[i].Effect == SPELL_EFFECT_LEARN_SPELL)
            {
                SpellLearnSpellNode dbc_node;
                dbc_node.spell = entry->Effects[i].TriggerSpell;
                dbc_node.active = true;                     // all dbc based learned spells is active (show in spell book or hide by client itself)

                // ignore learning not existed spells (broken/outdated/or generic learnig spell 483
                if (!GetSpellInfo(dbc_node.spell))
                    continue;

                // talent or passive spells or skill-step spells auto-casted and not need dependent learning,
                // pet teaching spells must not be dependent learning (casted)
                // other required explicit dependent learning
                dbc_node.autoLearned = entry->Effects[i].TargetA.GetTarget() == TARGET_UNIT_PET || /*GetTalentSpellCost(spell) > 0 ||*/ entry->IsPassive() || entry->HasEffect(SPELL_EFFECT_SKILL_STEP);

                SpellLearnSpellMapBounds db_node_bounds = GetSpellLearnSpellMapBounds(spell);

                bool found = false;
                for (SpellLearnSpellMap::const_iterator itr = db_node_bounds.first; itr != db_node_bounds.second; ++itr)
                {
                    if (itr->second.spell == dbc_node.spell)
                    {
                        sLog->outError(LOG_FILTER_SQL, "Spell %u auto-learn spell %u in spell.dbc then the record in `spell_learn_spell` is redundant, please fix DB.",
                            spell, dbc_node.spell);
                        found = true;
                        break;
                    }
                }

                if (!found)                                  // add new spell-spell pair if not found
                {
                    mSpellLearnSpells.insert(SpellLearnSpellMap::value_type(spell, dbc_node));
                    ++dbc_count;
                }
            }
        }
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u spell learn spells + %u found in DBC in %u ms", count, dbc_count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellTargetPositions()
{
    uint32 oldMSTime = getMSTime();

    mSpellTargetPositions.clear(); // need for reload case

    //                                                0    1         2           3                  4                     5                  6
    QueryResult result = WorldDatabase.Query("SELECT id, effIndex, target_map, target_position_x, target_position_y, target_position_z, target_orientation FROM spell_target_position");
    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 spell target coordinates. DB table `spell_target_position` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 spellId = fields[0].GetUInt32();
        SpellEffIndex effIndex = SpellEffIndex(fields[1].GetUInt8());

        SpellTargetPosition st;

        st.target_mapId = fields[2].GetUInt16();
        st.target_X = fields[3].GetFloat();
        st.target_Y = fields[4].GetFloat();
        st.target_Z = fields[5].GetFloat();
        st.target_Orientation = fields[6].GetFloat();

        MapEntry const* mapEntry = sMapStore.LookupEntry(st.target_mapId);
        if (!mapEntry)
        {
            sLog->outError(LOG_FILTER_SQL, "Spell (Id: %u, EffectIndex: %u) target map (Id: %u) does not exist in `Map.dbc`.", spellId, effIndex, st.target_mapId);
            continue;
        }

        if (st.target_X == 0 && st.target_Y == 0 && st.target_Z == 0)
        {
            sLog->outError(LOG_FILTER_SQL, "Spell (Id: %u, EffectIndex: %u) target coordinates not provided.", spellId, effIndex);
            continue;
        }

        SpellInfo const* spellInfo = GetSpellInfo(spellId);
        if (!spellInfo)
        {
            sLog->outError(LOG_FILTER_SQL, "Spell (Id:%u) listed in `spell_target_position` does not exist.", spellId);
            continue;
        }

        if (spellInfo->Effects[effIndex].TargetA.GetTarget() == TARGET_DEST_DB || spellInfo->Effects[effIndex].TargetB.GetTarget() == TARGET_DEST_DB)
        {
            std::pair<uint32, SpellEffIndex> key = std::make_pair(spellId, effIndex);
            mSpellTargetPositions[key] = st;
            ++count;
        }
        else
        {
            sLog;
        }

    } while (result->NextRow());


    /*
    // Check all spells
    for (uint32 i = 1; i < GetSpellInfoStoreSize(); ++i)
    {
    SpellInfo const* spellInfo = GetSpellInfo(i);
    if (!spellInfo)
    continue;

    for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
    {
    SpellEffectInfo const* effect = spellInfo->GetEffect(j);
    if (!effect)
    continue;

    if (effect->TargetA.GetTarget() != TARGET_DEST_DB && effect->TargetB.GetTarget() != TARGET_DEST_DB)
    continue;

    if (!GetSpellTargetPosition(i, SpellEffIndex(j)))
    TC_LOG_DEBUG("spells", "Spell (Id: %u, EffectIndex: %u) does not have record in `spell_target_position`.", i, j);
    }
    }
    */

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u spell teleport coordinates in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellGroups()
{
    uint32 oldMSTime = getMSTime();

    mSpellSpellGroup.clear();                                  // need for reload case
    mSpellGroupSpell.clear();

    //                                                0     1
    QueryResult result = WorldDatabase.Query("SELECT id, spellId FROM spell_group");
    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 spell group definitions. DB table `spell_group` is empty.");
        return;
    }

    std::set<uint32> groups;
    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 group_id = fields[0].GetUInt32();
        if (group_id <= SPELL_GROUP_DB_RANGE_MIN && group_id >= SPELL_GROUP_CORE_RANGE_MAX)
        {
            sLog->outError(LOG_FILTER_SQL, "SpellGroup id %u listed in `spell_group` is in core range, but is not defined in core!", group_id);
            continue;
        }
        int32 spellId = fields[1].GetInt32();

        groups.insert(std::set<uint32>::value_type(group_id));
        mSpellGroupSpell.insert(SpellGroupSpellMap::value_type((SpellGroup)group_id, spellId));

    } while (result->NextRow());

    for (SpellGroupSpellMap::iterator itr = mSpellGroupSpell.begin(); itr != mSpellGroupSpell.end();)
    {
        if (itr->second < 0)
        {
            if (groups.find(abs(itr->second)) == groups.end())
            {
                sLog->outError(LOG_FILTER_SQL, "SpellGroup id %u listed in `spell_group` does not exist", abs(itr->second));
                mSpellGroupSpell.erase(itr++);
            }
            else
                ++itr;
        }
        else
        {
            SpellInfo const* spellInfo = GetSpellInfo(itr->second);

            if (!spellInfo)
            {
                sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_group` does not exist", itr->second);
                mSpellGroupSpell.erase(itr++);
            }
            else if (spellInfo->GetRank() > 1)
            {
                sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_group` is not first rank of spell", itr->second);
                mSpellGroupSpell.erase(itr++);
            }
            else
                ++itr;
        }
    }

    for (std::set<uint32>::iterator groupItr = groups.begin(); groupItr != groups.end(); ++groupItr)
    {
        std::set<uint32> spells;
        GetSetOfSpellsInSpellGroup(SpellGroup(*groupItr), spells);

        for (std::set<uint32>::iterator spellItr = spells.begin(); spellItr != spells.end(); ++spellItr)
        {
            ++count;
            mSpellSpellGroup.insert(SpellSpellGroupMap::value_type(*spellItr, SpellGroup(*groupItr)));
        }
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u spell group definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellGroupStackRules()
{
    uint32 oldMSTime = getMSTime();

    mSpellGroupStack.clear();                                  // need for reload case

    //                                                       0         1      2
    QueryResult result = WorldDatabase.Query("SELECT group_id, stack_rule, auraType FROM spell_group_stack_rules");
    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 spell group stack rules. DB table `spell_group_stack_rules` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 group_id = fields[0].GetUInt32();
        uint8 stack_rule = fields[1].GetInt8();
        uint32 auraType = fields[2].GetUInt32();
        if (stack_rule >= SPELL_GROUP_STACK_RULE_MAX)
        {
            sLog->outError(LOG_FILTER_SQL, "SpellGroupStackRule %u listed in `spell_group_stack_rules` does not exist", stack_rule);
            continue;
        }

        SpellGroupSpellMapBounds spellGroup = GetSpellGroupSpellMapBounds((SpellGroup)group_id);

        if (spellGroup.first == spellGroup.second)
        {
            sLog->outError(LOG_FILTER_SQL, "SpellGroup id %u listed in `spell_group_stack_rules` does not exist", group_id);
            continue;
        }

        if (auraType >= TOTAL_AURAS)
        {
            sLog->outError(LOG_FILTER_SQL, "SpellGroup id %u listed in `spell_group_stack_rules` has wrong auraeffect id", group_id);
            continue;
        }

        SpellGroupStack stack;
        stack.stackRule = (SpellGroupStackRule)stack_rule;
        stack.auraType = auraType;
        mSpellGroupStack[(SpellGroup)group_id] = stack;

        ++count;
    }
    while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u spell group stack rules in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadForbiddenSpells()
{
    uint32 oldMSTime = getMSTime();

    mForbiddenSpells.clear();

    uint32 count = 0;

    QueryResult result = WorldDatabase.Query("SELECT spellId FROM spell_forbidden");
    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u spell group definitions", count);
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        mForbiddenSpells.push_back(fields[0].GetUInt32());

        ++count;
    } while (result->NextRow());


    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u forbidden spells in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::InitializeItemUpgradeDatas()
{
    uint32 oldMSTime = getMSTime();

    uint16 spTable[71][2] =
    {
        { 458, 4914 }, { 463, 5152 }, { 466, 5293 }, { 470, 5497 }, { 471, 5552 }, { 474, 5704 }, { 476, 5812 }, { 478, 5920 }, { 480, 6037 }, { 483, 6206 },
        { 484, 6262 }, { 487, 6445 }, { 489, 6564 }, { 490, 6628 }, { 491, 6684 }, { 493, 6810 }, { 494, 6874 }, { 496, 7007 }, { 497, 7070 }, { 498, 7140 },
        { 500, 7272 }, { 501, 7337 }, { 502, 7410 }, { 503, 7478 }, { 504, 7548 }, { 505, 7619 }, { 506, 7690 }, { 507, 7759 }, { 508, 7836 }, { 509, 7907 },
        { 510, 7982 }, { 511, 8054 }, { 512, 8132 }, { 513, 8209 }, { 514, 8286 }, { 515, 8364 }, { 516, 8441 }, { 517, 8521 }, { 518, 8603 }, { 519, 8680 },
        { 520, 8764 }, { 521, 8841 }, { 522, 8925 }, { 524, 9093 }, { 525, 9179 }, { 526, 9265 }, { 528, 9440 }, { 530, 9618 }, { 532, 9797 }, { 535, 10078 },
        { 536, 10169 }, { 539, 10458 }, { 540, 10557 }, { 541, 10655 }, { 543, 10859 }, { 544, 10957 }, { 545, 11060 }, { 548, 11372 }, { 549, 11479 }, { 553, 11916 },
        { 557, 12370 }, { 559, 12602 }, { 561, 12841 }, { 563, 13079 }, { 566, 13452 }, { 567, 13578 }, { 570, 13961 }, { 572, 14225 }, { 574, 14492 }, { 576, 14766 }, { 580, 15321 }
    };

    for (uint8 i = 0; i < 71; ++i)
        mItemUpgradeDatas.insert(std::make_pair(spTable[i][0], spTable[i][1]));

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 71 item upgrade datas in %u ms", GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellProcEvents()
{
    uint32 oldMSTime = getMSTime();

    mSpellProcEventMap.clear();                             // need for reload case

    //                                                0      1           2                3                 4                 5                 6                   7           8        9         10         11
    QueryResult result = WorldDatabase.Query("SELECT entry, SchoolMask, SpellFamilyName, SpellFamilyMask0, SpellFamilyMask1, SpellFamilyMask2, spellFamilyMask3, procFlags, procEx, ppmRate, CustomChance, Cooldown FROM spell_proc_event");
    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 spell proc event conditions. DB table `spell_proc_event` is empty.");
        return;
    }

    uint32 count = 0;
    uint32 customProc = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        SpellInfo const* spell = GetSpellInfo(entry);
        if (!spell)
        {
            WorldDatabase.PExecute("DELETE FROM spell_proc_event WHERE entry = %u;", entry);
            sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_proc_event` does not exist", entry);
            continue;
        }

        SpellProcEventEntry spe;

        spe.schoolMask = fields[1].GetInt8();
        spe.spellFamilyName = fields[2].GetUInt16();
        spe.spellFamilyMask[0] = fields[3].GetUInt32();
        spe.spellFamilyMask[1] = fields[4].GetUInt32();
        spe.spellFamilyMask[2] = fields[5].GetUInt32();
        spe.spellFamilyMask[3] = fields[6].GetUInt32();
        spe.procFlags = fields[7].GetUInt32();
        spe.procEx = fields[8].GetUInt32();
        spe.ppmRate = fields[9].GetFloat();
        spe.customChance = fields[10].GetFloat();
        spe.cooldown = fields[11].GetUInt32();

        mSpellProcEventMap[entry] = spe;

        if (spell->ProcFlags == 0)
        {
            if (spe.procFlags == 0)
            {
                sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_proc_event` probally not triggered spell", entry);
                continue;
            }
            customProc++;
        }
        ++count;
    } while (result->NextRow());

    if (customProc)
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u extra and %u custom spell proc event conditions in %u ms", count, customProc, GetMSTimeDiffToNow(oldMSTime));
    else
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u extra spell proc event conditions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

}

void SpellMgr::LoadSpellProcs()
{
    uint32 oldMSTime = getMSTime();

    mSpellProcMap.clear();                             // need for reload case

    //                                                 0        1           2                3                 4                 5                 6         7              8               9        10              11             12      13        14
    QueryResult result = WorldDatabase.Query("SELECT spellId, schoolMask, spellFamilyName, spellFamilyMask0, spellFamilyMask1, spellFamilyMask2, typeMask, spellTypeMask, spellPhaseMask, hitMask, attributesMask, ratePerMinute, chance, cooldown, charges FROM spell_proc");
    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 spell proc conditions and data. DB table `spell_proc` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        int32 spellId = fields[0].GetInt32();

        bool allRanks = false;
        if (spellId <= 0)
        {
            allRanks = true;
            spellId = -spellId;
        }

        SpellInfo const* spellEntry = GetSpellInfo(spellId);
        if (!spellEntry)
        {
            sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_proc` does not exist", spellId);
            continue;
        }

        if (allRanks)
        {
            if (GetFirstSpellInChain(spellId) != uint32(spellId))
            {
                sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_proc` is not first rank of spell.", fields[0].GetInt32());
                continue;
            }
        }

        SpellProcEntry baseProcEntry;

        baseProcEntry.schoolMask = fields[1].GetInt8();
        baseProcEntry.spellFamilyName = fields[2].GetUInt16();
        baseProcEntry.spellFamilyMask[0] = fields[3].GetUInt32();
        baseProcEntry.spellFamilyMask[1] = fields[4].GetUInt32();
        baseProcEntry.spellFamilyMask[2] = fields[5].GetUInt32();
        baseProcEntry.typeMask = fields[6].GetUInt32();
        baseProcEntry.spellTypeMask = fields[7].GetUInt32();
        baseProcEntry.spellPhaseMask = fields[8].GetUInt32();
        baseProcEntry.hitMask = fields[9].GetUInt32();
        baseProcEntry.attributesMask = fields[10].GetUInt32();
        baseProcEntry.ratePerMinute = fields[11].GetFloat();
        baseProcEntry.chance = fields[12].GetFloat();
        float cooldown = fields[13].GetFloat();
        baseProcEntry.cooldown = uint32(cooldown);
        baseProcEntry.charges = fields[14].GetUInt32();

        while (true)
        {
            if (mSpellProcMap.find(spellId) != mSpellProcMap.end())
            {
                sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_proc` has duplicate entry in the table", spellId);
                break;
            }
            SpellProcEntry procEntry = SpellProcEntry(baseProcEntry);

            // take defaults from dbcs
            if (!procEntry.typeMask)
                procEntry.typeMask = spellEntry->ProcFlags;
            if (!procEntry.charges)
                procEntry.charges = spellEntry->ProcCharges;
            if (!procEntry.chance && !procEntry.ratePerMinute)
                procEntry.chance = float(spellEntry->ProcChance);

            // validate data
            if (procEntry.schoolMask & ~SPELL_SCHOOL_MASK_ALL)
                sLog->outError(LOG_FILTER_SQL, "`spell_proc` table entry for spellId %u has wrong `schoolMask` set: %u", spellId, procEntry.schoolMask);
            if (procEntry.spellFamilyName && (procEntry.spellFamilyName < 3 || procEntry.spellFamilyName > 17 || procEntry.spellFamilyName == 14 || procEntry.spellFamilyName == 16))
                sLog->outError(LOG_FILTER_SQL, "`spell_proc` table entry for spellId %u has wrong `spellFamilyName` set: %u", spellId, procEntry.spellFamilyName);
            if (procEntry.chance < 0)
            {
                sLog->outError(LOG_FILTER_SQL, "`spell_proc` table entry for spellId %u has negative value in `chance` field", spellId);
                procEntry.chance = 0;
            }
            if (procEntry.ratePerMinute < 0)
            {
                sLog->outError(LOG_FILTER_SQL, "`spell_proc` table entry for spellId %u has negative value in `ratePerMinute` field", spellId);
                procEntry.ratePerMinute = 0;
            }
            if (cooldown < 0)
            {
                sLog->outError(LOG_FILTER_SQL, "`spell_proc` table entry for spellId %u has negative value in `cooldown` field", spellId);
                procEntry.cooldown = 0;
            }
            if (procEntry.chance == 0 && procEntry.ratePerMinute == 0)
                sLog->outError(LOG_FILTER_SQL, "`spell_proc` table entry for spellId %u doesn't have `chance` and `ratePerMinute` values defined, proc will not be triggered", spellId);
            if (procEntry.charges > 99)
            {
                sLog->outError(LOG_FILTER_SQL, "`spell_proc` table entry for spellId %u has too big value in `charges` field", spellId);
                procEntry.charges = 99;
            }
            if (!procEntry.typeMask)
                sLog->outError(LOG_FILTER_SQL, "`spell_proc` table entry for spellId %u doesn't have `typeMask` value defined, proc will not be triggered", spellId);
            if (procEntry.spellTypeMask & ~PROC_SPELL_PHASE_MASK_ALL)
                sLog->outError(LOG_FILTER_SQL, "`spell_proc` table entry for spellId %u has wrong `spellTypeMask` set: %u", spellId, procEntry.spellTypeMask);
            if (procEntry.spellTypeMask && !(procEntry.typeMask & (SPELL_PROC_FLAG_MASK | PERIODIC_PROC_FLAG_MASK)))
                sLog->outError(LOG_FILTER_SQL, "`spell_proc` table entry for spellId %u has `spellTypeMask` value defined, but it won't be used for defined `typeMask` value", spellId);
            if (!procEntry.spellPhaseMask && procEntry.typeMask & REQ_SPELL_PHASE_PROC_FLAG_MASK)
                sLog->outError(LOG_FILTER_SQL, "`spell_proc` table entry for spellId %u doesn't have `spellPhaseMask` value defined, but it's required for defined `typeMask` value, proc will not be triggered", spellId);
            if (procEntry.spellPhaseMask & ~PROC_SPELL_PHASE_MASK_ALL)
                sLog->outError(LOG_FILTER_SQL, "`spell_proc` table entry for spellId %u has wrong `spellPhaseMask` set: %u", spellId, procEntry.spellPhaseMask);
            if (procEntry.spellPhaseMask && !(procEntry.typeMask & REQ_SPELL_PHASE_PROC_FLAG_MASK))
                sLog->outError(LOG_FILTER_SQL, "`spell_proc` table entry for spellId %u has `spellPhaseMask` value defined, but it won't be used for defined `typeMask` value", spellId);
            if (procEntry.hitMask & ~PROC_HIT_MASK_ALL)
                sLog->outError(LOG_FILTER_SQL, "`spell_proc` table entry for spellId %u has wrong `hitMask` set: %u", spellId, procEntry.hitMask);
            if (procEntry.hitMask && !(procEntry.typeMask & TAKEN_HIT_PROC_FLAG_MASK || (procEntry.typeMask & DONE_HIT_PROC_FLAG_MASK && (!procEntry.spellPhaseMask || procEntry.spellPhaseMask & (PROC_SPELL_PHASE_HIT | PROC_SPELL_PHASE_FINISH)))))
                sLog->outError(LOG_FILTER_SQL, "`spell_proc` table entry for spellId %u has `hitMask` value defined, but it won't be used for defined `typeMask` and `spellPhaseMask` values", spellId);

            mSpellProcMap[spellId] = procEntry;

            if (allRanks)
            {
                spellId = GetNextSpellInChain(spellId);
                spellEntry = GetSpellInfo(spellId);
            }
            else
                break;
        }
        ++count;
    } while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u spell proc conditions and data in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellBonusess()
{
    uint32 oldMSTime = getMSTime();

    mSpellBonusMap.clear();                             // need for reload case

    //                                                0      1             2          3         4
    QueryResult result = WorldDatabase.Query("SELECT entry, direct_bonus, dot_bonus, ap_bonus, ap_dot_bonus FROM spell_bonus_data");
    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 spell bonus data. DB table `spell_bonus_data` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 entry = fields[0].GetUInt32();

        SpellInfo const* spell = GetSpellInfo(entry);
        if (!spell)
        {
            WorldDatabase.PExecute("DELETE FROM spell_bonus_data WHERE entry = %u;", entry);
            sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_bonus_data` does not exist", entry);
            continue;
        }

        SpellBonusEntry& sbe = mSpellBonusMap[entry];
        sbe.direct_damage = fields[1].GetFloat();
        sbe.dot_damage = fields[2].GetFloat();
        sbe.ap_bonus = fields[3].GetFloat();
        sbe.ap_dot_bonus = fields[4].GetFloat();

        ++count;
    } while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u extra spell bonus data in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellThreats()
{
    uint32 oldMSTime = getMSTime();

    mSpellThreatMap.clear();                                // need for reload case

    //                                                0      1        2       3
    QueryResult result = WorldDatabase.Query("SELECT entry, flatMod, pctMod, apPctMod FROM spell_threat");
    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 aggro generating spells. DB table `spell_threat` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        if (!GetSpellInfo(entry))
        {
            sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_threat` does not exist", entry);
            continue;
        }

        SpellThreatEntry ste;
        ste.flatMod = fields[1].GetInt32();
        ste.pctMod = fields[2].GetFloat();
        ste.apPctMod = fields[3].GetFloat();

        mSpellThreatMap[entry] = ste;
        ++count;
    } while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u SpellThreatEntries in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSkillLineAbilityMap()
{
    uint32 oldMSTime = getMSTime();

    mSkillLineAbilityMap.clear();

    uint32 count = 0;

    for (uint32 i = 0; i < sSkillLineAbilityStore.GetNumRows(); ++i)
    {
        SkillLineAbilityEntry const* SkillInfo = sSkillLineAbilityStore.LookupEntry(i);
        if (!SkillInfo)
            continue;

        mSkillLineAbilityMap.insert(SkillLineAbilityMap::value_type(SkillInfo->spellId, SkillInfo));
        ++count;
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u SkillLineAbility MultiMap Data in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellPetAuras()
{
    uint32 oldMSTime = getMSTime();

    mSpellPetAuraMap.clear();                                  // need for reload case

    //                                                  0       1       2    3
    QueryResult result = WorldDatabase.Query("SELECT spell, effectId, pet, aura FROM spell_pet_auras");
    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 spell pet auras. DB table `spell_pet_auras` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 spell = fields[0].GetUInt32();
        uint8 eff = fields[1].GetUInt8();
        uint32 pet = fields[2].GetUInt32();
        uint32 aura = fields[3].GetUInt32();

        SpellPetAuraMap::iterator itr = mSpellPetAuraMap.find((spell << 8) + eff);
        if (itr != mSpellPetAuraMap.end())
            itr->second.AddAura(pet, aura);
        else
        {
            SpellInfo const* spellInfo = GetSpellInfo(spell);
            if (!spellInfo)
            {
                sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_pet_auras` does not exist", spell);
                continue;
            }
            if (spellInfo->Effects[eff].Effect != SPELL_EFFECT_DUMMY &&
                (spellInfo->Effects[eff].Effect != SPELL_EFFECT_APPLY_AURA ||
                spellInfo->Effects[eff].Effect != SPELL_EFFECT_APPLY_AURA_ON_PET ||
                spellInfo->Effects[eff].ApplyAuraName != SPELL_AURA_DUMMY))
            {
                sLog->outError(LOG_FILTER_SPELLS_AURAS, "Spell %u listed in `spell_pet_auras` does not have dummy aura or dummy effect", spell);
                continue;
            }

            SpellInfo const* spellInfo2 = GetSpellInfo(aura);
            if (!spellInfo2)
            {
                sLog->outError(LOG_FILTER_SQL, "Aura %u listed in `spell_pet_auras` does not exist", aura);
                continue;
            }

            PetAura pa(pet, aura, spellInfo->Effects[eff].TargetA.GetTarget() == TARGET_UNIT_PET, spellInfo->Effects[eff].CalcValue());
            mSpellPetAuraMap[(spell << 8) + eff] = pa;
        }

        ++count;
    } while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u spell pet auras in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

// Fill custom data about enchancments
void SpellMgr::LoadEnchantCustomAttr()
{
    uint32 oldMSTime = getMSTime();

    uint32 size = sSpellItemEnchantmentStore.GetNumRows();
    mEnchantCustomAttr.resize(size);

    for (uint32 i = 0; i < size; ++i)
        mEnchantCustomAttr[i] = 0;

    uint32 count = 0;
    for (uint32 i = 0; i < GetSpellInfoStoreSize(); ++i)
    {
        SpellInfo const* spellInfo = GetSpellInfo(i);
        if (!spellInfo)
            continue;

        // TODO: find a better check
        if (!(spellInfo->AttributesEx2 & SPELL_ATTR2_PRESERVE_ENCHANT_IN_ARENA) || !(spellInfo->Attributes & SPELL_ATTR0_NOT_SHAPESHIFT))
            continue;

        for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            if (spellInfo->Effects[j].Effect == SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY)
            {
                uint32 enchId = spellInfo->Effects[j].MiscValue;
                SpellItemEnchantmentEntry const* ench = sSpellItemEnchantmentStore.LookupEntry(enchId);
                if (!ench)
                    continue;
                mEnchantCustomAttr[enchId] = true;
                ++count;
                break;
            }
        }
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u custom enchant attributes in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellEnchantProcData()
{
    uint32 oldMSTime = getMSTime();

    mSpellEnchantProcEventMap.clear();                             // need for reload case

    //                                                  0         1           2         3
    QueryResult result = WorldDatabase.Query("SELECT entry, customChance, PPMChance, procEx FROM spell_enchant_proc_data");
    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 spell enchant proc event conditions. DB table `spell_enchant_proc_data` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 enchantId = fields[0].GetUInt32();

        SpellItemEnchantmentEntry const* ench = sSpellItemEnchantmentStore.LookupEntry(enchantId);
        if (!ench)
        {
            sLog->outError(LOG_FILTER_SQL, "Enchancment %u listed in `spell_enchant_proc_data` does not exist", enchantId);
            continue;
        }

        SpellEnchantProcEntry spe;

        spe.customChance = fields[1].GetUInt32();
        spe.PPMChance = fields[2].GetFloat();
        spe.procEx = fields[3].GetUInt32();

        mSpellEnchantProcEventMap[enchantId] = spe;

        ++count;
    } while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u enchant proc data definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellLinked()
{
    uint32 oldMSTime = getMSTime();

    mSpellLinkedMap.clear();    // need for reload case

    //                                                0              1             2
    QueryResult result = WorldDatabase.Query("SELECT spell_trigger, spell_effect, type FROM spell_linked_spell");
    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 linked spells. DB table `spell_linked_spell` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        int32 trigger = fields[0].GetInt32();
        int32 effect = fields[1].GetInt32();
        int32 type = fields[2].GetUInt8();

        SpellInfo const* spellInfo = GetSpellInfo(abs(trigger));
        if (!spellInfo)
        {
            sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_linked_spell` does not exist", abs(trigger));
            continue;
        }
        spellInfo = GetSpellInfo(abs(effect));
        if (!spellInfo)
        {
            sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_linked_spell` does not exist", abs(effect));
            continue;
        }

        if (type) //we will find a better way when more types are needed
        {
            if (trigger > 0)
                trigger += SPELL_LINKED_MAX_SPELLS * type;
            else
                trigger -= SPELL_LINKED_MAX_SPELLS * type;
        }
        mSpellLinkedMap[trigger].push_back(effect);

        ++count;
    } while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u linked spells in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadPetLevelupSpellMap()
{
    uint32 oldMSTime = getMSTime();

    mPetLevelupSpellMap.clear();                                   // need for reload case

    uint32 count = 0;
    uint32 family_count = 0;

    for (uint32 i = 0; i < sCreatureFamilyStore.GetNumRows(); ++i)
    {
        CreatureFamilyEntry const* creatureFamily = sCreatureFamilyStore.LookupEntry(i);
        if (!creatureFamily)                                     // not exist
            continue;

        for (uint8 j = 0; j < 2; ++j)
        {
            if (!creatureFamily->skillLine[j])
                continue;

            for (uint32 k = 0; k < sSkillLineAbilityStore.GetNumRows(); ++k)
            {
                SkillLineAbilityEntry const* skillLine = sSkillLineAbilityStore.LookupEntry(k);
                if (!skillLine)
                    continue;

                //if (skillLine->skillId != creatureFamily->skillLine[0] &&
                //    (!creatureFamily->skillLine[1] || skillLine->skillId != creatureFamily->skillLine[1]))
                //    continue;

                if (skillLine->skillId != creatureFamily->skillLine[j])
                    continue;

                if (skillLine->learnOnGetSkill != ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL)
                    continue;

                SpellInfo const* spell = GetSpellInfo(skillLine->spellId);
                if (!spell) // not exist or triggered or talent
                    continue;

                if (!spell->SpellLevel)
                    continue;

                PetLevelupSpellSet& spellSet = mPetLevelupSpellMap[creatureFamily->ID];
                if (spellSet.empty())
                    ++family_count;

                spellSet.insert(PetLevelupSpellSet::value_type(spell->SpellLevel, spell->Id));
                ++count;
            }
        }
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u pet levelup and default spells for %u families in %u ms", count, family_count, GetMSTimeDiffToNow(oldMSTime));
}

bool LoadPetDefaultSpells_helper(CreatureTemplate const* cInfo, PetDefaultSpellsEntry& petDefSpells)
{
    // skip empty list;
    bool have_spell = false;
    for (uint8 j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
    {
        if (petDefSpells.spellid[j])
        {
            have_spell = true;
            break;
        }
    }
    if (!have_spell)
        return false;

    // remove duplicates with levelupSpells if any
    if (PetLevelupSpellSet const* levelupSpells = cInfo->family ? sSpellMgr->GetPetLevelupSpellList(cInfo->family) : NULL)
    {
        for (uint8 j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
        {
            if (!petDefSpells.spellid[j])
                continue;

            for (PetLevelupSpellSet::const_iterator itr = levelupSpells->begin(); itr != levelupSpells->end(); ++itr)
            {
                if (itr->second == petDefSpells.spellid[j])
                {
                    petDefSpells.spellid[j] = 0;
                    break;
                }
            }
        }
    }

    // skip empty list;
    have_spell = false;
    for (uint8 j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
    {
        if (petDefSpells.spellid[j])
        {
            have_spell = true;
            break;
        }
    }

    return have_spell;
}

void SpellMgr::LoadPetDefaultSpells()
{
    uint32 oldMSTime = getMSTime();

    mPetDefaultSpellsMap.clear();

    uint32 countCreature = 0;
    uint32 countData = 0;

    CreatureTemplateContainer const* ctc = sObjectMgr->GetCreatureTemplates();
    for (CreatureTemplateContainer::const_iterator itr = ctc->begin(); itr != ctc->end(); ++itr)
    {

        if (!itr->second.PetSpellDataId)
            continue;

        // for creature with PetSpellDataId get default pet spells from dbc
        CreatureSpellDataEntry const* spellDataEntry = sCreatureSpellDataStore.LookupEntry(itr->second.PetSpellDataId);
        if (!spellDataEntry)
            continue;

        int32 petSpellsId = -int32(itr->second.PetSpellDataId);
        PetDefaultSpellsEntry petDefSpells;
        for (uint8 j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
            petDefSpells.spellid[j] = spellDataEntry->spellId[j];

        if (LoadPetDefaultSpells_helper(&itr->second, petDefSpells))
        {
            mPetDefaultSpellsMap[petSpellsId] = petDefSpells;
            ++countData;
        }
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded addition spells for %u pet spell data entries in %u ms", countData, GetMSTimeDiffToNow(oldMSTime));

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, "Loading summonable creature templates...");
    oldMSTime = getMSTime();

    // different summon spells
    for (uint32 i = 0; i < GetSpellInfoStoreSize(); ++i)
    {
        SpellInfo const* spellEntry = GetSpellInfo(i);
        if (!spellEntry)
            continue;

        for (uint8 k = 0; k < MAX_SPELL_EFFECTS; ++k)
        {
            if (spellEntry->Effects[k].Effect == SPELL_EFFECT_SUMMON || spellEntry->Effects[k].Effect == SPELL_EFFECT_SUMMON_PET)
            {
                uint32 creature_id = spellEntry->Effects[k].MiscValue;
                CreatureTemplate const* cInfo = sObjectMgr->GetCreatureTemplate(creature_id);
                if (!cInfo)
                    continue;

                // already loaded
                if (cInfo->PetSpellDataId)
                    continue;

                // for creature without PetSpellDataId get default pet spells from creature_template
                int32 petSpellsId = cInfo->Entry;
                if (mPetDefaultSpellsMap.find(cInfo->Entry) != mPetDefaultSpellsMap.end())
                    continue;

                PetDefaultSpellsEntry petDefSpells;
                for (uint8 j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
                    petDefSpells.spellid[j] = cInfo->spells[j];

                if (LoadPetDefaultSpells_helper(cInfo, petDefSpells))
                {
                    mPetDefaultSpellsMap[petSpellsId] = petDefSpells;
                    ++countCreature;
                }
            }
        }
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u summonable creature templates in %u ms", countCreature, GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadSpellAreas()
{
    uint32 oldMSTime = getMSTime();

    mSpellAreaMap.clear();                                  // need for reload case
    mSpellAreaForQuestMap.clear();
    mSpellAreaForActiveQuestMap.clear();
    mSpellAreaForQuestEndMap.clear();
    mSpellAreaForAuraMap.clear();

    //                                                  0     1         2              3               4                 5          6          7       8         9
    QueryResult result = WorldDatabase.Query("SELECT spell, area, quest_start, quest_start_status, quest_end_status, quest_end, aura_spell, racemask, gender, autocast FROM spell_area");
    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 spell area requirements. DB table `spell_area` is empty.");

        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 spell = fields[0].GetUInt32();
        SpellArea spellArea;
        spellArea.spellId = spell;
        spellArea.areaId = fields[1].GetUInt32();
        spellArea.questStart = fields[2].GetUInt32();
        spellArea.questStartStatus = fields[3].GetUInt32();
        spellArea.questEndStatus = fields[4].GetUInt32();
        spellArea.questEnd = fields[5].GetUInt32();
        spellArea.auraSpell = fields[6].GetInt32();
        spellArea.raceMask = fields[7].GetUInt32();
        spellArea.gender = Gender(fields[8].GetUInt8());
        spellArea.autocast = fields[9].GetBool();

        if (SpellInfo const* spellInfo = GetSpellInfo(spell))
        {
            if (spellArea.autocast)
                const_cast<SpellInfo*>(spellInfo)->Attributes |= SPELL_ATTR0_CANT_CANCEL;
        }
        else
        {
            sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_area` does not exist", spell);
            continue;
        }

        {
            bool ok = true;
            SpellAreaMapBounds sa_bounds = GetSpellAreaMapBounds(spellArea.spellId);
            for (SpellAreaMap::const_iterator itr = sa_bounds.first; itr != sa_bounds.second; ++itr)
            {
                if (spellArea.spellId != itr->second.spellId)
                    continue;
                if (spellArea.areaId != itr->second.areaId)
                    continue;
                if (spellArea.questStart != itr->second.questStart)
                    continue;
                if (spellArea.auraSpell != itr->second.auraSpell)
                    continue;
                if ((spellArea.raceMask & itr->second.raceMask) == 0)
                    continue;
                if (spellArea.gender != itr->second.gender)
                    continue;

                // duplicate by requirements
                ok = false;
                break;
            }

            if (!ok)
            {
                sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_area` already listed with similar requirements.", spell);
                continue;
            }
        }

        if (spellArea.areaId && !GetAreaEntryByAreaID(spellArea.areaId))
        {
            sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_area` have wrong area (%u) requirement", spell, spellArea.areaId);
            continue;
        }

        if (spellArea.questStart && !sObjectMgr->GetQuestTemplate(spellArea.questStart))
        {
            sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_area` have wrong start quest (%u) requirement", spell, spellArea.questStart);
            continue;
        }

        if (spellArea.questEnd)
        {
            if (!sObjectMgr->GetQuestTemplate(spellArea.questEnd))
            {
                sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_area` have wrong end quest (%u) requirement", spell, spellArea.questEnd);
                continue;
            }
        }

        if (spellArea.auraSpell)
        {
            SpellInfo const* spellInfo = GetSpellInfo(abs(spellArea.auraSpell));
            if (!spellInfo)
            {
                sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_area` have wrong aura spell (%u) requirement", spell, abs(spellArea.auraSpell));
                continue;
            }

            if (uint32(abs(spellArea.auraSpell)) == spellArea.spellId)
            {
                sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_area` have aura spell (%u) requirement for itself", spell, abs(spellArea.auraSpell));
                continue;
            }

            // not allow autocast chains by auraSpell field (but allow use as alternative if not present)
            if (spellArea.autocast && spellArea.auraSpell > 0)
            {
                bool chain = false;
                SpellAreaForAuraMapBounds saBound = GetSpellAreaForAuraMapBounds(spellArea.spellId);
                for (SpellAreaForAuraMap::const_iterator itr = saBound.first; itr != saBound.second; ++itr)
                {
                    if (itr->second->autocast && itr->second->auraSpell > 0)
                    {
                        chain = true;
                        break;
                    }
                }

                if (chain)
                {
                    sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_area` have aura spell (%u) requirement that itself autocast from aura", spell, spellArea.auraSpell);
                    continue;
                }

                SpellAreaMapBounds saBound2 = GetSpellAreaMapBounds(spellArea.auraSpell);
                for (SpellAreaMap::const_iterator itr2 = saBound2.first; itr2 != saBound2.second; ++itr2)
                {
                    if (itr2->second.autocast && itr2->second.auraSpell > 0)
                    {
                        chain = true;
                        break;
                    }
                }

                if (chain)
                {
                    sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_area` have aura spell (%u) requirement that itself autocast from aura", spell, spellArea.auraSpell);
                    continue;
                }
            }
        }

        if (spellArea.raceMask && (spellArea.raceMask & RACEMASK_ALL_PLAYABLE) == 0)
        {
            sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_area` have wrong race mask (%u) requirement", spell, spellArea.raceMask);
            continue;
        }

        if (spellArea.gender != GENDER_NONE && spellArea.gender != GENDER_FEMALE && spellArea.gender != GENDER_MALE)
        {
            sLog->outError(LOG_FILTER_SQL, "Spell %u listed in `spell_area` have wrong gender (%u) requirement", spell, spellArea.gender);
            continue;
        }

        SpellArea const* sa = &mSpellAreaMap.insert(SpellAreaMap::value_type(spell, spellArea))->second;

        // for search by current zone/subzone at zone/subzone change
        if (spellArea.areaId)
            mSpellAreaForAreaMap.insert(SpellAreaForAreaMap::value_type(spellArea.areaId, sa));

        // for search at quest start/reward
        if (spellArea.questStart)
            mSpellAreaForQuestMap.insert(SpellAreaForQuestMap::value_type(spellArea.questStart, sa));

        // for search at quest start/reward
        if (spellArea.questEnd)
            mSpellAreaForQuestEndMap.insert(SpellAreaForQuestMap::value_type(spellArea.questEnd, sa));

        // for search at aura apply
        if (spellArea.auraSpell)
            mSpellAreaForAuraMap.insert(SpellAreaForAuraMap::value_type(abs(spellArea.auraSpell), sa));

        ++count;
    } while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u spell area requirements in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

static const uint32 SkillClass[MAX_CLASSES] = { 0, 840, 800, 795, 921, 804, 796, 924, 904, 849, 829, 798 };

void SpellMgr::LoadSpellClassInfo()
{
    mSpellClassInfo.resize(MAX_CLASSES);
    for (int ClassID = 0; ClassID < MAX_CLASSES; ClassID++)
    {
        ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(ClassID);
        if (!classEntry)
            continue;
        // Tome of the Clear Mind
        mSpellClassInfo[ClassID].insert(89964);
        // Dust of Disappearance
        mSpellClassInfo[ClassID].insert(90647);
        // Vanishing Powder
        mSpellClassInfo[ClassID].insert(111621);
        // Remove Talent
        mSpellClassInfo[ClassID].insert(127650);
        mSpellClassInfo[ClassID].insert(127649);
        mSpellClassInfo[ClassID].insert(113873);
        // Player damage reduction (72% base resilience)
        mSpellClassInfo[ClassID].insert(115043);
        mSpellClassInfo[ClassID].insert(142689);
        // Player mastery activation
        mSpellClassInfo[ClassID].insert(114585);
        // Battle Fatigue
        mSpellClassInfo[ClassID].insert(134732);

        // All Rune for DK
        if (ClassID == CLASS_DEATH_KNIGHT)
        {
            mSpellClassInfo[ClassID].insert(53323);
            mSpellClassInfo[ClassID].insert(54447);
            mSpellClassInfo[ClassID].insert(53342);
            mSpellClassInfo[ClassID].insert(53331);
            mSpellClassInfo[ClassID].insert(54446);
            mSpellClassInfo[ClassID].insert(53323);
            mSpellClassInfo[ClassID].insert(53344);
            mSpellClassInfo[ClassID].insert(70164);
            mSpellClassInfo[ClassID].insert(62158);
        }

        // Swift Flight Form
        if (ClassID == CLASS_DRUID)
            mSpellClassInfo[ClassID].insert(40120);

        // Dark Soul
        if (ClassID == CLASS_WARLOCK)
            mSpellClassInfo[ClassID].insert(77801);

        // All portals and teleports for mages and Mana Attunement
        if (ClassID == CLASS_MAGE)
        {
            mSpellClassInfo[ClassID].insert(3561);
            mSpellClassInfo[ClassID].insert(3562);
            mSpellClassInfo[ClassID].insert(3563);
            mSpellClassInfo[ClassID].insert(3565);
            mSpellClassInfo[ClassID].insert(3566);
            mSpellClassInfo[ClassID].insert(3567);
            mSpellClassInfo[ClassID].insert(32271);
            mSpellClassInfo[ClassID].insert(32272);
            mSpellClassInfo[ClassID].insert(49359);
            mSpellClassInfo[ClassID].insert(49360);
            mSpellClassInfo[ClassID].insert(32266);
            mSpellClassInfo[ClassID].insert(32267);
            mSpellClassInfo[ClassID].insert(10059);
            mSpellClassInfo[ClassID].insert(11416);
            mSpellClassInfo[ClassID].insert(11417);
            mSpellClassInfo[ClassID].insert(11418);
            mSpellClassInfo[ClassID].insert(11419);
            mSpellClassInfo[ClassID].insert(11420);
            mSpellClassInfo[ClassID].insert(49358);
            mSpellClassInfo[ClassID].insert(49361);
            mSpellClassInfo[ClassID].insert(35715);
            mSpellClassInfo[ClassID].insert(33690);
            mSpellClassInfo[ClassID].insert(33691);
            mSpellClassInfo[ClassID].insert(35717);
            mSpellClassInfo[ClassID].insert(53140);
            mSpellClassInfo[ClassID].insert(53142);
            mSpellClassInfo[ClassID].insert(88342);
            mSpellClassInfo[ClassID].insert(88344);
            mSpellClassInfo[ClassID].insert(88345);
            mSpellClassInfo[ClassID].insert(88346);
            mSpellClassInfo[ClassID].insert(121039);
            mSpellClassInfo[ClassID].insert(132626);
            mSpellClassInfo[ClassID].insert(132627);
            mSpellClassInfo[ClassID].insert(132621);
            mSpellClassInfo[ClassID].insert(132620);
        }

        // Ancestral Focus
        if (ClassID == CLASS_SHAMAN)
            mSpellClassInfo[ClassID].insert(89920);

        // Plate Mail skill
        if (ClassID == CLASS_PALADIN || ClassID == CLASS_WARRIOR)
            mSpellClassInfo[ClassID].insert(750);

        // Mail skill
        if (ClassID == CLASS_SHAMAN || ClassID == CLASS_HUNTER)
            mSpellClassInfo[ClassID].insert(8737);

        // Dual Wield
        if (ClassID == CLASS_WARRIOR || ClassID == CLASS_HUNTER || ClassID == CLASS_ROGUE || ClassID == CLASS_DEATH_KNIGHT || ClassID == CLASS_MONK)
            mSpellClassInfo[ClassID].insert(674);

        // Natural Insight druid
        if (ClassID == CLASS_DRUID)
            mSpellClassInfo[ClassID].insert(112857);

        // Sinister Strike Enabler
        if (ClassID == CLASS_ROGUE)
            mSpellClassInfo[ClassID].insert(79327);

        // Opening gameobject
        if (ClassID == CLASS_MONK)
        {
            mSpellClassInfo[ClassID].insert(3365);
            mSpellClassInfo[ClassID].insert(6247);
            mSpellClassInfo[ClassID].insert(6477);
            mSpellClassInfo[ClassID].insert(6478);
            mSpellClassInfo[ClassID].insert(21651);
            mSpellClassInfo[ClassID].insert(22810);
            mSpellClassInfo[ClassID].insert(61437);
            mSpellClassInfo[ClassID].insert(68398);
            mSpellClassInfo[ClassID].insert(96220);
        }

        for (uint32 i = 0; i < sSkillLineAbilityStore.GetNumRows(); ++i)
        {
            SkillLineAbilityEntry const* skillLine = sSkillLineAbilityStore.LookupEntry(i);
            if (!skillLine)
                continue;

            SpellInfo const* spellEntry = sSpellMgr->GetSpellInfo(skillLine->spellId);
            if (!spellEntry)
                continue;

            if (spellEntry->SpellLevel == 0)
                continue;

            if (skillLine->skillId != SkillClass[ClassID] || skillLine->learnOnGetSkill != ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL)
                continue;

            // See CGSpellBook::InitFutureSpells in client
            if (spellEntry->Attributes & SPELL_ATTR0_TRADESPELL || spellEntry->Attributes & SPELL_ATTR0_HIDDEN_CLIENTSIDE
                || spellEntry->AttributesEx8 & SPELL_ATTR8_UNK13 || spellEntry->AttributesEx4 & SPELL_ATTR4_UNK15)
                continue;

            if (sSpellMgr->IsTalent(spellEntry->Id))
                continue;

            mSpellClassInfo[ClassID].insert(spellEntry->Id);
        }

        for (uint32 i = 0; i < sSpecializationSpellStore.GetNumRows(); ++i)
        {
            SpecializationSpellEntry const* specializationInfo = sSpecializationSpellStore.LookupEntry(i);
            if (!specializationInfo)
                continue;

            ChrSpecializationsEntry const* chrSpec = sChrSpecializationsStore.LookupEntry(specializationInfo->SpecializationEntry);
            if (!chrSpec)
                continue;

            mSpellClassInfo[chrSpec->classId].insert(specializationInfo->LearnSpell);
        }
    }
}

struct spellDifficultyLoadInfo
{
    uint32 id;
    std::list<uint32> difficultyList;
};

void SpellMgr::LoadSpellInfoStore()
{
    uint32 oldMSTime = getMSTime();

    std::map<uint32, std::set<uint32> > spellDifficultyList;

    for (uint32 i = 0; i < sSpellEffectStore.GetNumRows(); ++i)
    if (SpellEffectEntry const* spellEffect = sSpellEffectStore.LookupEntry(i))
        spellDifficultyList[spellEffect->EffectSpellId].insert(spellEffect->EffectDifficulty);


    UnloadSpellInfoStore();
    for (int difficulty = 0; difficulty < MAX_DIFFICULTY; difficulty++)
        mSpellInfoMap[difficulty].resize(sSpellStore.GetNumRows(), NULL);

    for (uint32 i = 0; i < sSpellStore.GetNumRows(); ++i)
    {
        if (SpellEntry const* spellEntry = sSpellStore.LookupEntry(i))
        {
            std::set<uint32> difficultyInfo = spellDifficultyList[i];
            for (std::set<uint32>::iterator itr = difficultyInfo.begin(); itr != difficultyInfo.end(); itr++)
                mSpellInfoMap[(*itr)][i] = new SpellInfo(spellEntry, (*itr));
        }
    }

    std::set<uint32> alreadySet;
    for (uint32 i = 0; i < sSpellPowerStore.GetNumRows(); i++)
    {
        SpellPowerEntry const* spellPower = sSpellPowerStore.LookupEntry(i);
        if (!spellPower)
            continue;

        if (alreadySet.find(spellPower->SpellId) != alreadySet.end())
            continue;

        for (int difficulty = 0; difficulty < MAX_DIFFICULTY; difficulty++)
        {
            SpellInfo* spell = mSpellInfoMap[difficulty][spellPower->SpellId];
            if (!spell)
                continue;

            spell->ManaCost = spellPower->manaCost;
            spell->ManaCostPercentage = spellPower->ManaCostPercentage;
            spell->ManaPerSecond = spellPower->manaPerSecond;
            spell->PowerType = spellPower->powerType;

            spell->spellPower->manaCost = spellPower->manaCost;
            spell->spellPower->ManaCostPercentage = spellPower->ManaCostPercentage;
            spell->spellPower->manaPerSecond = spellPower->manaPerSecond;
            spell->spellPower->powerType = spellPower->powerType;
        }

        alreadySet.insert(spellPower->SpellId);
    }

    for (uint32 i = 0; i < sTalentStore.GetNumRows(); i++)
    {
        TalentEntry const* talentInfo = sTalentStore.LookupEntry(i);
        if (!talentInfo)
            continue;

        SpellInfo * spellEntry = mSpellInfoMap[REGULAR_DIFFICULTY][talentInfo->spellId];
        if (spellEntry)
            spellEntry->talentId = talentInfo->Id;
    }

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded spell info store in %u ms", GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::UnloadSpellInfoStore()
{
    for (int difficulty = 0; difficulty < MAX_DIFFICULTY; difficulty++)
    {
        for (uint32 i = 0; i < mSpellInfoMap[difficulty].size(); ++i)
        {
            if (mSpellInfoMap[difficulty][i])
                delete mSpellInfoMap[difficulty][i];
        }
        mSpellInfoMap[difficulty].clear();
    }
}

void SpellMgr::UnloadSpellInfoImplicitTargetConditionLists()
{
    for (int difficulty = 0; difficulty < MAX_DIFFICULTY; difficulty++)
    {
        for (uint32 i = 0; i < mSpellInfoMap[difficulty].size(); ++i)
        {
            if (mSpellInfoMap[difficulty][i])
                mSpellInfoMap[difficulty][i]->_UnloadImplicitTargetConditionLists();
        }
    }
}

void SpellMgr::LoadSpellCustomAttr()
{
    uint32 oldMSTime = getMSTime();

    SpellInfo* spellInfo = NULL;
    for (uint32 i = 0; i < GetSpellInfoStoreSize(); ++i)
    {
        for (int difficulty = 0; difficulty < MAX_DIFFICULTY; difficulty++)
        {
            spellInfo = mSpellInfoMap[difficulty][i];
            if (!spellInfo)
                continue;

            for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
            {
                switch (spellInfo->Effects[j].ApplyAuraName)
                {
                case SPELL_AURA_MOD_POSSESS:
                case SPELL_AURA_MOD_CONFUSE:
                case SPELL_AURA_MOD_CHARM:
                case SPELL_AURA_AOE_CHARM:
                case SPELL_AURA_MOD_FEAR:
                case SPELL_AURA_MOD_FEAR_2:
                case SPELL_AURA_MOD_STUN:
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_AURA_CC;
                    break;
                case SPELL_AURA_PERIODIC_HEAL:
                case SPELL_AURA_PERIODIC_DAMAGE:
                case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
                case SPELL_AURA_PERIODIC_LEECH:
                case SPELL_AURA_PERIODIC_MANA_LEECH:
                case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
                case SPELL_AURA_PERIODIC_ENERGIZE:
                case SPELL_AURA_OBS_MOD_HEALTH:
                case SPELL_AURA_OBS_MOD_POWER:
                case SPELL_AURA_POWER_BURN:
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_NO_INITIAL_THREAT;
                    break;
                }

                switch (spellInfo->Effects[j].Effect)
                {
                case SPELL_EFFECT_SCHOOL_DAMAGE:
                case SPELL_EFFECT_WEAPON_DAMAGE:
                case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                case SPELL_EFFECT_HEAL:
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_DIRECT_DAMAGE;
                    break;
                case SPELL_EFFECT_POWER_DRAIN:
                case SPELL_EFFECT_POWER_BURN:
                case SPELL_EFFECT_HEAL_MAX_HEALTH:
                case SPELL_EFFECT_HEALTH_LEECH:
                case SPELL_EFFECT_HEAL_PCT:
                case SPELL_EFFECT_ENERGIZE_PCT:
                case SPELL_EFFECT_ENERGIZE:
                case SPELL_EFFECT_HEAL_MECHANICAL:
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_NO_INITIAL_THREAT;
                    break;
                case SPELL_EFFECT_CHARGE:
                case SPELL_EFFECT_CHARGE_DEST:
                case SPELL_EFFECT_JUMP:
                case SPELL_EFFECT_JUMP_DEST:
                case SPELL_EFFECT_LEAP_BACK:
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_CHARGE;
                    break;
                case SPELL_EFFECT_PICKPOCKET:
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_PICKPOCKET;
                    break;
                case SPELL_EFFECT_CREATE_ITEM:
                case SPELL_EFFECT_CREATE_ITEM_2:
                    mSpellCreateItemList.push_back(i);
                    break;
                case SPELL_EFFECT_ENCHANT_ITEM:
                case SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY:
                case SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC:
                case SPELL_EFFECT_ENCHANT_HELD_ITEM:
                {
                                                       // only enchanting profession enchantments procs can stack
                                                       if (IsPartOfSkillLine(SKILL_ENCHANTING, i))
                                                       {
                                                           uint32 enchantId = spellInfo->Effects[j].MiscValue;
                                                           SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
                                                           for (uint8 s = 0; s < MAX_ITEM_ENCHANTMENT_EFFECTS; ++s)
                                                           {
                                                               if (enchant->type[s] != ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL)
                                                                   continue;

                                                               SpellInfo* procInfo = (SpellInfo*)GetSpellInfo(enchant->spellid[s]);
                                                               if (!procInfo)
                                                                   continue;

                                                               // if proced directly from enchantment, not via proc aura
                                                               // NOTE: Enchant Weapon - Blade Ward also has proc aura spell and is proced directly
                                                               // however its not expected to stack so this check is good
                                                               if (procInfo->HasAura(SPELL_AURA_PROC_TRIGGER_SPELL))
                                                                   continue;

                                                               procInfo->AttributesCu |= SPELL_ATTR0_CU_ENCHANT_STACK;
                                                           }
                                                       }
                                                       else if (IsPartOfSkillLine(SKILL_RUNEFORGING, i))
                                                       {
                                                           uint32 enchantId = spellInfo->Effects[j].MiscValue;
                                                           SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
                                                           for (uint8 s = 0; s < MAX_ITEM_ENCHANTMENT_EFFECTS; ++s)
                                                           {
                                                               SpellInfo* procInfo = (SpellInfo*)GetSpellInfo(enchant->spellid[s]);
                                                               if (!procInfo)
                                                                   continue;

                                                               switch (procInfo->Id)
                                                               {
                                                               case 53365: // Rune of the Fallen Crusader
                                                               case 51714: // Rune of Razorice
                                                                   continue;
                                                               }

                                                               procInfo->AttributesCu |= SPELL_ATTR0_CU_ENCHANT_STACK;
                                                           }
                                                       }
                                                       break;
                }
                }
            }

            if (!spellInfo->_IsPositiveEffect(EFFECT_0, false))
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE_EFF0;

            if (!spellInfo->_IsPositiveEffect(EFFECT_1, false))
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE_EFF1;

            if (!spellInfo->_IsPositiveEffect(EFFECT_2, false))
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE_EFF2;

            if (spellInfo->SpellVisual[0] == 3879 || spellInfo->Id == 74117)
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_CONE_BACK;

            ////////////////////////////////////
            ///      DEFINE BINARY SPELLS   ////
            ////////////////////////////////////
            for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
            {
                switch (spellInfo->Effects[j].Effect)
                {
                case SPELL_EFFECT_DISPEL:
                case SPELL_EFFECT_STEAL_BENEFICIAL_BUFF:
                case SPELL_EFFECT_POWER_DRAIN:
                case SPELL_EFFECT_POWER_BURN:
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_BINARY;
                    break;
                }

                switch (spellInfo->Effects[j].Mechanic)
                {
                case MECHANIC_FEAR:
                case MECHANIC_CHARM:
                case MECHANIC_SNARE:
                    // Frost Bolt is not binary
                    if (spellInfo->Id == 116)
                        break;
                case MECHANIC_FREEZE:
                case MECHANIC_BANISH:
                case MECHANIC_POLYMORPH:
                case MECHANIC_ROOT:
                case MECHANIC_INTERRUPT:
                case MECHANIC_SILENCE:
                case MECHANIC_HORROR:
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_BINARY;
                    break;
                }
            }

            switch (spellInfo->Mechanic)
            {
            case MECHANIC_FEAR:
            case MECHANIC_CHARM:
            case MECHANIC_SNARE:
            case MECHANIC_FREEZE:
            case MECHANIC_BANISH:
            case MECHANIC_POLYMORPH:
            case MECHANIC_ROOT:
            case MECHANIC_INTERRUPT:
            case MECHANIC_SILENCE:
            case MECHANIC_HORROR:
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_BINARY;
                break;
            }

            ///////////////////////////
            //////   END BINARY  //////
            ///////////////////////////

            auto itr = sSpellEnchantScalingData.find(spellInfo->Id);
            if (itr != sSpellEnchantScalingData.end())
            {
                auto const ench = itr->second;
                spellInfo->ScalingClass = ench->ScalingClass;
            }

            switch (spellInfo->Id)
            {
            case 135704: // Grand Commendation of the Anglers
                spellInfo->Effects[EFFECT_0].BasePoints = 1302;
                break;
            case 135719: // Grand Commendation of the Klaxxi
                spellInfo->Effects[EFFECT_0].BasePoints = 1337;
                break;
            case 135717: // Grand Commendation of the Golden Lotus
                spellInfo->Effects[EFFECT_0].BasePoints = 1269;
                break;
            case 135715: // Grand Commendation of the Shado-Pan
                spellInfo->Effects[EFFECT_0].BasePoints = 1270;
                break;
            case 135716: // Grand Commendation of the August Celestials
                spellInfo->Effects[EFFECT_0].BasePoints = 1341;
                break;
            case 135714: // Grand Commendation of the Tillers
                spellInfo->Effects[EFFECT_0].BasePoints = 1272;
                break;
            case 135713: // Grand Commendation of the Order of the Cloud Serpent
                spellInfo->Effects[EFFECT_0].BasePoints = 1271;
                break;
            case 135710: // Grand Commendation of Operation: Shieldwall
                spellInfo->Effects[EFFECT_0].BasePoints = 1376;
                break;
            case 135711: // Grand Commendation of the Dominance Offensive
                spellInfo->Effects[EFFECT_0].BasePoints = 1375;
                break;
            case 140226: // Grand Commendation of the Kirin Tor Offensive
                spellInfo->Effects[EFFECT_0].BasePoints = 1387;
                break;
            case 140228: // Grand Commendation of the Sunreaver Onslaught
                spellInfo->Effects[EFFECT_0].BasePoints = 1388;
                break;
            case 135712: // Grand Commendation of the Lorewalkers
                spellInfo->Effects[EFFECT_0].BasePoints = 1345;
                break;
            case 138759: //Feathers of Fury
            case 138756: //Blades of Renataki
            case 138728: //Change of Tactics
            case 138786: //Wushoolay's Lightning 
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_REDUCE_33;
                break;
            case 138864: //Blood of Power 
            case 139170: //Eye of Brutality
            case 138870: //Rampage
            case 138895: //Frenzy
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_REDUCE_75;
                break;
            case 36327:  // 
            case 124995: // Great Wall Corner A Destroyed
            case 124656: // Great Wall Corner B Destroyed
            case 124999: // Great Wall Corner C Destroyed
            case 124993: // Hurl Boulder -> 124995 Great Wall Corner A Destroyed 
            case 114906: // Hurl Boulder -> 124656 Great Wall Corner B Destroyed
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ANY);
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo(0);
                break;
            case 125000: // Hurl Boulder -> 124999 Great Wall Corner C Destroyed. 
                spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ANY);
                spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo(0);
                spellInfo->Effects[EFFECT_1].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ANY);
                spellInfo->Effects[EFFECT_1].TargetB = SpellImplicitTargetInfo(0);
                break;
            case 106933:  // Prey Time
                spellInfo->AttributesEx &= ~SPELL_ATTR1_CHANNELED_1;
                break;
            case 89964:  // Tome of the Clear Mind
            case 127650: // Remove Talent
                spellInfo->SpellLevel = 15;
                break;
            case 90647:  // Dust of Disappearance
            case 127649: // Remove Talent
                spellInfo->SpellLevel = 81;
                break;
            case 111621: // Vanishing Powder
            case 113873: // Remove Talent
                spellInfo->SpellLevel = 86;
                break;
			case 134735:
				spellInfo->Effects[EFFECT_0].BasePoints = -60;
				break;
			case 116033:// Sparring aura
				spellInfo->Effects[EFFECT_1].Effect = 0;
				break;
			case 116023: // Sparring
				spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_APPLY_AURA;
				spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_PROC_TRIGGER_SPELL;
				spellInfo->Effects[EFFECT_0].TriggerSpell = 116033; // Sparring
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
				spellInfo->Effects[EFFECT_0].TargetB = 0;
				spellInfo->AuraInterruptFlags = 0;
				break;
			case 143268: // Unbridled Wrath
				spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_APPLY_AURA;
				spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_PROC_TRIGGER_SPELL;
				spellInfo->Effects[EFFECT_0].TriggerSpell = 29842; // Unbridled Wrath
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
				spellInfo->Effects[EFFECT_0].TargetB = 0;
				break;
				// Fiery Boulder, Support for gilneas start zone
			case 68591:
				spellInfo->MaxAffectedTargets = 1;
				spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_SRC_AREA_ENTRY);
				spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo();
				break;
				// Fan the Flames, Support for: Quest [29422] Huo, the Spirit of Fire
			case 109090: // Fan the Flames: Throw Wood
			case 109095: // Fan the Flames: Blow Air
			case 109105: // Fan the Flames: Blow Air Big
			case 109109: // Fan the Flames: Blow Air Bigger
				spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_10_YARDS);
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CONE_ENTRY;
				spellInfo->Effects[EFFECT_0].TargetB = 0;
				break;
				// Flying Shadow Kick, Support for: Quest [29421] Only the Worthy Shall Pass
			case 108936:
				spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(7); // 2 yards
				break;
				// Feet of Fury, Support for: Quest [29421] Only the Worthy Shall Pass
			case 108958:
				spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ENEMY);
				spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ENEMY);
				break;
				// Summon Living Air, Support for: Quest [29523] Fanning the Flames 
			case 106299:
				spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_DEST_AREA_ENTRY);
				spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo(0);
				spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(48);
				break;
            case 115176: // Zen meditation
                spellInfo->ChannelInterruptFlags = 0x0;
                break;
            case 137143: // Blood Horror
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE;
                break;
            case 19503: // Scatter Shot
            case 3355: // Freezing Trap
            case 118: // Polymorph
                spellInfo->ProcFlags = 0x001A22A8;
            break;
            case 51128: // Killing Machine
                spellInfo->ProcChance = 0;
                break;
            case 14161: // Ruthlessness
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_PROC_TRIGGER_SPELL;
                spellInfo->Effects[0].TriggerSpell = 139546;
                spellInfo->ProcFlags = 0x00015550;
                spellInfo->ProcChance = 100;
                break;
            case 1329:  // Mutilate
                spellInfo->AttributesEx3 &= ~SPELL_ATTR3_CANT_TRIGGER_PROC;
                break;
            case 20740:
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(9); // 8s
                break;
            case 53651: // Beacon of Light (dummy)
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(39); // 2s
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER_AREA_RAID;
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(48); // 60 yards
                spellInfo->ProcFlags = 0x8A20;
                break;
            case 53563: // Beacon of Light
                spellInfo->Effects[1].Effect = 0;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_PERIODIC_TRIGGER_SPELL;
                spellInfo->Effects[0].Amplitude = 1500;
                break;
            case 143333:
                spellInfo->AuraInterruptFlags |= AURA_INTERRUPT_FLAG_NOT_MOUNTED;
                break;
            case 49575: // Death Grip
                spellInfo->Effects[0].Effect = SPELL_EFFECT_JUMP_DEST;
                spellInfo->Effects[0].MiscValueB = 50;
                break;
            case 102401: // Wild Charge (No Form)
                spellInfo->Effects[1].MiscValueB = 75;
                break;
            case 73325: // Leap of Faith
                spellInfo->Effects[1].TriggerSpell = 0;
                break;
            case 15286: // Vampiric Embrace
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 78832: // Arcane Blast
                spellInfo->CasterAuraSpell = 0;
                break;
            case 52752: // Ancestral Awakening
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                break;
            case 95540: // Fungalophobia Achievement Check
                spellInfo->Attributes |= (SPELL_ATTR0_PASSIVE | SPELL_ATTR0_HIDDEN_CLIENTSIDE);
                break;
            case 136340:
                spellInfo->ExcludeTargetAuraSpell = 0;
                break;
            case 137639:// Storm, Earth and Fire
                spellInfo->AttributesCu &= ~(SPELL_ATTR0_CU_NEGATIVE_EFF1 | SPELL_ATTR0_CU_NEGATIVE_EFF0);
                break;
            case 138130:// Storm, Earth and Fire (for spirits)
                spellInfo->Effects[0].Effect = 0;
                break;
            case 108501: // Grimmuar of service
                spellInfo->Attributes &= ~(SPELL_ATTR0_HIDDEN_CLIENTSIDE);
                break;
            case 123099: // Spiritual Insight
            case 112857: // Natural Insight
            case 112858: // Spiritual Insight
            case 112859: // Holy Insight
                spellInfo->Effects[1].Effect = 0;
                break;
            case 135299:// Ice Trap (snare)
            case 140023:// Ring of Peace (dummy)
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(285); // 1s
                break;
            case 5740:  // Rain of Fire
            case 10: // Blizzard
            case 104232:// Rain of Fire (Aftermath)
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 116257: // Invoker's Energy
                spellInfo->Effects[2].ApplyAuraName = SPELL_AURA_INCREASE_MANA_REGEN;
                break;
            case 81297: // Consecration Damages
            case 81298: // Consecration Visual
                spellInfo->Effects[0].TargetA = TARGET_DEST_DEST;
                break;
            case 119414:// Breath of Fear
                spellInfo->Effects[2].Effect = 0;
                break;
            case 89646:// Glyph of Healing Storm
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 107145:// Wall of Light activation
                spellInfo->Effects[0].TargetA = TARGET_SRC_CASTER;
                break;
            case 119312:// Conjure Terror Spawn (01)
            case 119370:// Conjure Terror Spawn (02)
            case 119371:// Conjure Terror Spawn (03)
            case 119372:// Conjure Terror Spawn (04)
                spellInfo->Effects[0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 117866:// Champion of Light
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(65); // 1,5s
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(15); // 3 yards
                break;
            case 82366:
            case 110704:
                spellInfo->Effects[2].Effect = SPELL_EFFECT_DUMMY;
                break;
            case 124009:
            case 130793:
                spellInfo->StartRecoveryTime = 1500;
                spellInfo->StartRecoveryCategory = 133;
                break;
            case 123996:// Crackling Tiger Lightning
                spellInfo->StartRecoveryTime = 1000;
                spellInfo->StartRecoveryCategory = 133;
                break;
            case 126848:// Ritual of Purification
                spellInfo->Effects[0].BasePoints = -10;
                spellInfo->Effects[1].BasePoints = -10;
                break;
            case 125706:// Channeling Corruption
                spellInfo->Effects[0].TriggerSpell = 0;
                break;
            case 125713:// Channeling Corruption (triggered)
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 125736:// Night Terrors (missile)
                spellInfo->Effects[0].Effect = SPELL_EFFECT_DUMMY;
                break;
            case 117230:// Overpowered
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                break;
            case 117988:// Defiled Ground
                spellInfo->Effects[0].Effect = 0;
                spellInfo->Effects[1].Mechanic = MECHANIC_NONE;
                break;
            case 91264:
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(39); // 2s
                break;
            case 121039: // Mana Attunement (Passive)
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 117052:// Corruption Sha
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[0].TargetB = 0;
                spellInfo->Effects[1].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[1].TargetB = 0;
                spellInfo->Effects[2].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[2].TargetB = 0;
                break;
            case 127731:// Corruption Sha (triggered)
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 123244:// Hide
                spellInfo->Effects[0].Effect = 0;
                spellInfo->Effects[2].Effect = 0;
                break;
            case 127424:
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CONE_ENEMY_54;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 101840: // Molten Axe, Echo of Baine
            case 101867: // Molten Fists, Echo of Baine
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE;
                break;
            case 45257: // Using Steam Tonk Controller
            case 45440: // Steam Tonk Controller
            case 49352: // Crashin' Thrashin' Racer Controller
            case 75111: // Blue Crashin' Thrashin' Racer Controller
            case 60256: // Collect Sample
                //Crashes client on pressing ESC (Maybe because of ReqSpellFocus and GameObject)
                spellInfo->AttributesEx4 &= ~SPELL_ATTR4_TRIGGERED;
                break;
            case 8629:
            case 11285:
            case 11286:
            case 12540:
            case 13579:
            case 24698:
            case 28456:
            case 29425:
            case 34940:
            case 36862:
            case 38764:
            case 38863:
            case 52743: // Head Smack
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_REQ_TARGET_FACING_CASTER;
                break;
            case 53: // Backstab
            case 2589:
            case 2590:
            case 2591:
            case 8721:
            case 11279:
            case 11280:
            case 11281:
            case 25300:
            case 26863:
            case 48656:
            case 48657:
            case 8631:
            case 8632:
            case 8633:
            case 11289:
            case 11290:
            case 26839:
            case 26884:
            case 48675:
            case 48676:
            case 5221: // Shred
            case 6800:
            case 8992:
            case 9829:
            case 9830:
            case 27001:
            case 27002:
            case 48571:
            case 48572:
            case 8676: // Ambush
            case 8724:
            case 8725:
            case 11267:
            case 11268:
            case 11269:
            case 27441:
            case 48689:
            case 48690:
            case 48691:
            case 6787:
            case 9866:
            case 9867:
            case 27005:
            case 48578:
            case 48579:
            case 21987: // Lash of Pain
            case 23959: // Test Stab R50
            case 24825: // Test Backstab
            case 58563: // Assassinate Restless Lookout
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_REQ_CASTER_BEHIND_TARGET;
                break;
                // Shado-Pan Dragon Gun
            case 120751:
            case 120876:
            case 120964:
            case 124347:
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CONE_ENEMY_54;
                break;
            case 26029: // Dark Glare
            case 37433: // Spout
            case 43215: // Flame Breath
            case 70461: // Coldflame Trap
            case 72133: // Pain and Suffering
            case 73788: // Pain and Suffering
            case 73789: // Pain and Suffering
            case 73790: // Pain and Suffering
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_CONE_LINE;
                break;
            case 24340: // Meteor
            case 26558: // Meteor
            case 28884: // Meteor
            case 36837: // Meteor
            case 38903: // Meteor
            case 41276: // Meteor
            case 57467: // Meteor
            case 26789: // Shard of the Fallen Star
            case 31436: // Malevolent Cleave
            case 35181: // Dive Bomb
            case 40810: // Saber Lash
            case 43267: // Saber Lash
            case 43268: // Saber Lash
            case 42384: // Brutal Swipe
            case 45150: // Meteor Slash
            case 64688: // Sonic Screech
            case 72373: // Shared Suffering
            case 71904: // Chaos Bane
            case 70492: // Ooze Eruption
            case 72505: // Ooze Eruption
            case 72624: // Ooze Eruption
            case 72625: // Ooze Eruption
            case 88942: // Meteor Slash
            case 95172: // Meteor Slash
            case 82935: // Caustic Slime
            case 88915: // Caustic Slime
            case 88916: // Caustic Slime
            case 88917: // Caustic Slime
            case 77679: // Scorching Blast
            case 92968: // Scorching Blast
            case 92969: // Scorching Blast
            case 92970: // Scorching Blast
            case 98474: // Flame Scythe
            case 100212: // Flame Scythe
            case 100213: // Flame Scythe
            case 100214: // Flame Scythe
            case 103414: // Stomp
            case 108571: // Stomp
            case 106375: // Unstable Twilight
            case 109182: // Unstable Twilight
            case 109183: // Unstable Twilight
            case 109184: // Unstable Twilight
            case 107439: // Twilight Barrage
            case 109203: // Twilight Barrage
                //case 109204: // Twilight Barrage
                //case 109205: // Twilight Barrage
            case 106401: // Twilight Onslaught
            case 108862: // Twilight Onslaught
            case 109226: // Twilight Onslaught
            case 109227: // Twilight Onslaught
            case 144688: // Magma Crush Ordos
                // ONLY SPELLS WITH SPELLFAMILY_GENERIC and EFFECT_SCHOOL_DAMAGE
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_SHARE_DAMAGE;
                break;
            case 18500: // Wing Buffet
            case 33086: // Wild Bite
            case 49749: // Piercing Blow
            case 52890: // Penetrating Strike
            case 53454: // Impale
            case 59446: // Impale
            case 64777: // Machine Gun
            case 65239: // Machine Gun
            case 65919: // Impale
            case 67858: // Impale
            case 67859: // Impale
            case 67860: // Impale
            case 69293: // Wing Buffet
            case 74439: // Machine Gun
            case 63278: // Mark of the Faceless (General Vezax)
            case 62544: // Thrust (Argent Tournament)
            case 64588: // Thrust (Argent Tournament)
            case 66479: // Thrust (Argent Tournament)
            case 68505: // Thrust (Argent Tournament)
            case 62709: // Counterattack! (Argent Tournament)
            case 62626: // Break-Shield (Argent Tournament, Player)
            case 64590: // Break-Shield (Argent Tournament, Player)
            case 64342: // Break-Shield (Argent Tournament, NPC)
            case 64686: // Break-Shield (Argent Tournament, NPC)
            case 65147: // Break-Shield (Argent Tournament, NPC)
            case 68504: // Break-Shield (Argent Tournament, NPC)
            case 62874: // Charge (Argent Tournament, Player)
            case 68498: // Charge (Argent Tournament, Player)
            case 64591: // Charge (Argent Tournament, Player)
            case 63003: // Charge (Argent Tournament, NPC)
            case 63010: // Charge (Argent Tournament, NPC)
            case 68321: // Charge (Argent Tournament, NPC)
            case 72255: // Mark of the Fallen Champion (Deathbringer Saurfang)
            case 72444: // Mark of the Fallen Champion (Deathbringer Saurfang)
            case 72445: // Mark of the Fallen Champion (Deathbringer Saurfang)
            case 72446: // Mark of the Fallen Champion (Deathbringer Saurfang)
            case 62775: // Tympanic Tantrum (XT-002 encounter)
            case 102598: // Void Strike, Infinite Warden
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
                break;
            case 64422: // Sonic Screech (Auriaya)
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_SHARE_DAMAGE;
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
                break;
            case 72293: // Mark of the Fallen Champion (Deathbringer Saurfang)
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE_EFF0;
                break;
            case 81744:
            case 81748:
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_2].Effect = SPELL_EFFECT_APPLY_AURA;
                break;
                // Custom MoP Script
                // FENG
            case 116711:// Draw Flame
                spellInfo->ChannelInterruptFlags = 0x0;
                spellInfo->AuraInterruptFlags = 0x0;
                break;
                // GARAJAL
            case 118443:// Instantly Death
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[0].TargetB = 0;
                break;
                // ELEGON
            case 116994:
                spellInfo->Effects[1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[1].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_DAMAGE_IMMUNITY;
                spellInfo->Effects[1].MiscValue = 127;
                break;
            case 116989:// Overloaded Missile
            case 117220:// Overloaded Triggered
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 118430:// Core Beam
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->AttributesEx &= ~SPELL_ATTR1_CHANNELED_1;
                break;
            case 129724:// Grasping Energy Tendrils
                spellInfo->Effects[0].Effect = SPELL_EFFECT_DUMMY;
                break;
            case 147362:// Counter Shot 
                spellInfo->AttributesEx7 &= ~SPELL_ATTR7_INTERRUPT_ONLY_NONPLAYER;
                break;
            case 117960:// Celestial Breathe
                spellInfo->AttributesEx5 &= ~SPELL_ATTR5_START_PERIODIC_AT_APPLY;
                break;
            case 118018:// Draw Power (Summons)
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[1].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[2].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[3].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[4].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[5].TargetA = TARGET_UNIT_CASTER;
                break;
                // SPIRIT KINGS
            case 117558:
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(13); // 10 yards
                break;
            case 117829:// Cowardice (DoT)
            case 117006:// Devastating Arc
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE;
                break;
            case 117833:// Crazy Thought
            case 117708:// Maddening Shout
                spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_FEARED;
                break;
            case 118047:// Pillage
                spellInfo->Effects[0].Effect = SPELL_EFFECT_CHARGE;
                spellInfo->Effects[0].TriggerSpell = 0;
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                break;
                // SPELLS
            case 688:   // Summon Imp
                spellInfo->OverrideSpellList.push_back(112866); // Summon Fel Imp
                break;
            case 697:   // Summon Voidwalker
                spellInfo->OverrideSpellList.push_back(112867); // Summon Voidlord
                break;
            case 712:   // Summon Succubus
                spellInfo->OverrideSpellList.push_back(112868); // Summon Shivarra
                break;
            case 691:   // Summon Felhunter
                spellInfo->OverrideSpellList.push_back(112869); // Summon Observer
                break;
            case 30146: // Summon Felguard
                spellInfo->OverrideSpellList.push_back(112870); // Summon Wrathguard
                break;
            case 1122:  // Summon Infernal
                spellInfo->OverrideSpellList.push_back(112921); // Summon Abyssal
                break;
            case 18540: // Summon Doomguard
                spellInfo->OverrideSpellList.push_back(112927); // Summon Terrorguard
                break;
            case 115422: // Void Ray
            case 103964: // Touch of Chaos
            case 124915: // Chaos Wave
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
                break;
            case 114255: // Surge of Light
                spellInfo->ProcCharges = 2;
                break;
            case 120517:// Halo (Holy)
            case 120644:// Halo (Shadow)
                spellInfo->Speed = 10;
                spellInfo->AttributesCu &= ~SPELL_ATTR0_CU_NEGATIVE;
                spellInfo->Effects[1].Effect = SPELL_EFFECT_SCRIPT_EFFECT;
                spellInfo->Effects[1].BasePoints = 0;
                spellInfo->Effects[1].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[1].TargetB = TARGET_UNIT_SRC_AREA_ALLY;
                spellInfo->Effects[1].RadiusEntry = sSpellRadiusStore.LookupEntry(10);
                spellInfo->Effects[0].Effect = SPELL_EFFECT_SCHOOL_DAMAGE;
                spellInfo->Effects[0].BasePoints = 1;
                spellInfo->Effects[0].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(10);
                break;
            case 74434:// Soulburn
                spellInfo->Effects[3].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[3].ApplyAuraName = SPELL_AURA_CAST_WHILE_WALKING;
                spellInfo->Effects[3].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[3].SpellClassMask[1] = 0x100;
                break;
            case 77799: // Fel Flame
                spellInfo->ManaCost = 0;
                spellInfo->ManaCostPercentage = 6;
                break;
            case 48108: // Hot Streak
            case 12043: // Presence of Mind
            case 53817: // Maelstrom Weapon
                spellInfo->ProcCharges = 0;
                break;
            case 128997:// Spirit Beast Blessing
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->AttributesEx |= SPELL_ATTR1_NOT_BREAK_STEALTH;
                break;
            case 127830:// Spirit Beast Blessing (Mastery Rating)
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AREA_AURA_RAID;
                break;
            case 121129:// Daybreak (heal)
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_TARGET_SELF;
                break;
            case 88819: // Daybreak buff
                spellInfo->StackAmount = 2;
                spellInfo->ProcFlags = 0;
                break;
            case 73981: // Redirect
            case 110730:// Redirect
            case 51699: // Honor Among Thieves (triggered)
            case 57934: // Tricks of the Trade
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                break;
            case 105770: // Item - Druid T13 Restoration 4P Bonus (Rejuvenation)
                spellInfo->Effects[0].SpellClassMask = flag128(0x00000050, 0, 0);
                break;
            case 105786: // Temporal Ruin Warlock 4P T13 bonus
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
                // Item - Druid T12 Feral 4P Bonus
            case 99009:
                spellInfo->ProcChance = 100;
                spellInfo->ProcFlags = 16;
                break;
                // Item - Shaman T12 Enhancement 2P Bonus
            case 99209:
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
                // Item - Shaman T12 Enhancement 4P Bonus
            case 99213:
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
                spellInfo->ProcChance = 100;
                spellInfo->ProcFlags = 16;
                break;
                // Stormfire, Item - Shaman T12 Enhancement 4P Bonus
            case 99212:
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
                // Item - Shaman T12 Elemental 4P Bonus
            case 99206:
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
                // Item - Rogue T12 4P Bonus
            case 99175:
                spellInfo->ProcChance = 0;
                spellInfo->ProcFlags = 0;
                break;
                // Item - Warrior T12 DPS 2P Bonus
            case 99234:
                spellInfo->ProcChance = 0;
                spellInfo->ProcFlags = 0;
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
                spellInfo->Effects[EFFECT_0].TriggerSpell = 0;
                break;
            case 81662: // Will of the Necropolis
                spellInfo->Effects[0].BasePoints = 25;
                break;
            case 127538:// Savage Roar (Glyphed)
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(636); // 12s / 0s / 42s
                break;
            case 15473: // Shadowform - hotfix 5.4.2
                //spellInfo->Effects[6].BasePoints = 100;
                spellInfo->SchoolMask = SPELL_SCHOOL_MASK_SHADOW;
                break;
            case 91107: // Unholy Might
                //spellInfo->Effects[0].BasePoints = 35;
                spellInfo->OverrideSpellList.push_back(109260); // Add Aspect of the Iron Hack to override spell list of Aspect of the Hawk
                break;
            case 774:  // Rejuvenation - hotfix 5.4.2 (idk why they have 2 healing effects, so 2 ticks when must be one)
                spellInfo->Effects[2].Effect = 0;
                break;
                /*case 49020: // Obliterate - hotfix 5.4.2
                spellInfo->Effects[1].BasePoints = 250;
                break;
                case 24858: // Moonkin form - hotfix 5.4.2
                spellInfo->Effects[2].BasePoints = 100;
                break;
                case 50887: // Icy Talons - hotfix 5.4.2
                spellInfo->Effects[0].BasePoints = 45;
                break;
                case 12294: // Mortal strike - hotfix 5.4.2
                spellInfo->Effects[2].BasePoints = 215;
                break;
                case 109260:// Aspect of the Iron Hawk - hotfix 5.4.2
                spellInfo->Effects[0].BasePoints = 35;
                break;
                case 48181: // Haunt - hotfix 5.4.2
                spellInfo->Effects[3].BasePoints = 35;
                break;
                case 16246: // Clearcasting - hotfix 5.4.2
                spellInfo->Effects[1].BasePoints = 20;
                break;
                case 105284:// Ancestral Vigor - fix 5.4.1
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(9); // 30s
                break;
                case 982:   // Revive Pet - hotfix 5.4.2
                spellInfo->CastTimeEntry = sSpellCastTimesStore.LookupEntry(5); // 2s
                break;
                case 23881: // Bloodthirst - fix 5.4
                spellInfo->Effects[1].BasePoints = 108;
                break;
                case 12328: // Sweeping Strikes - hotfix 5.4.2
                spellInfo->Effects[0].BasePoints = 50;
                break;*/
            case 115460:// Healing Sphere
                spellInfo->PreventionType = SPELL_PREVENTION_TYPE_SILENCE;
                spellInfo->SchoolMask = SPELL_SCHOOL_MASK_NATURE;
                break;
            case 124682:// Enveloping Mist
                spellInfo->PreventionType = SPELL_PREVENTION_TYPE_SILENCE;
            case 115072:// Expel Harm
            case 115310:// Revival
            case 116670:// Uplift
                spellInfo->SchoolMask = SPELL_SCHOOL_MASK_NATURE;
                break;
				// Mist of Pandaria Jewelcrafting Stones That Doesn't give Skill Points
			case 107624: // Red Stone
			case 107625:
			case 107626:
			case 107617: // Blue Stones
			case 107619:
			case 107620:
			case 107640: // Yellow Stones
			case 107641:
			case 107645: // Orange Stones
			case 107646:
			case 107647:
			case 107649:
			case 107650:
			case 107652:
			case 107654:
			case 107655:
			case 107658:
			case 107656:
			case 107667:
			case 107665:
			case 107599: // Green Stones
			case 107598:
			case 107600:
			case 107601:
			case 107602:
			case 107604:
			case 107605:
			case 107606:
			case 107607:
			case 107608:
			case 107609:
			case 107610:
			case 107611:
			case 107612:
			case 107613:
			case 107614:
			case 107615:
			case 107616:
			case 107627: // Purple Stones
			case 107628:
			case 107630:
			case 107631:
			case 107632:
			case 107633:
			case 107634:
			case 107635:
			case 107636:
			case 107637:
			case 107638:
			case 107639:
			case 122663:
			case 122665:
			case 130655:
			case 130656:
				spellInfo->Effects[0].Effect = SPELL_EFFECT_CREATE_ITEM;
				break;
            case 110744:// Divine Star (holy)
            case 122121:// Divine Star (shadow)
                spellInfo->Speed = 25.f;
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                spellInfo->Effects[0].TargetA = TARGET_UNIT_PROYECTILE;
                spellInfo->Effects[0].Effect = SPELL_EFFECT_DUMMY;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_DUMMY;
                spellInfo->Effects[0].TriggerSpell = 0;
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_25_YARDS);
                spellInfo->Effects[1].Effect = 0;
                spellInfo->Effects[1].TriggerSpell = 0;
                break;
            case 110745:// Divine Star heal
                spellInfo->Effects[0].Effect = SPELL_EFFECT_DUMMY;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_DUMMY;
                spellInfo->Effects[0].TargetA = 0;
                spellInfo->Effects[0].TargetB = 0;
                spellInfo->Effects[1].TargetA = TARGET_UNIT_TARGET_ALLY;
                spellInfo->Effects[1].TargetB = 0;
                break;
            case 122128:// Divine Star damage
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[0].TargetB = 0;
                spellInfo->Effects[1].Effect = SPELL_EFFECT_DUMMY;
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_DUMMY;
                spellInfo->Effects[1].TargetA = 0;
                spellInfo->Effects[1].TargetB = 0;
                break;
            case 123986:// Chi Burst
                spellInfo->Effects[0].TargetA = TARGET_UNIT_PROYECTILE;
                spellInfo->Effects[0].Effect = SPELL_EFFECT_SCHOOL_DAMAGE;
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_40_YARDS);
                spellInfo->Effects[1].TargetA = TARGET_UNIT_PROYECTILE;
                spellInfo->Effects[1].Effect = SPELL_EFFECT_HEAL;
                spellInfo->Effects[1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_40_YARDS);
                break;
            case 115396:// Ascension
                spellInfo->Effects[0].BasePoints = 0;
                spellInfo->Effects[1].BasePoints = 15;
                spellInfo->Effects[2].BasePoints = 15;
                break;
            case 81333: // Might of the Frozen Wastes
                //spellInfo->Effects[0].Effect = 0;
                spellInfo->Effects[1].Effect = 0;
                //spellInfo->Effects[2].Effect = 0;
                break;
            case 78976: // Solar Beam
                spellInfo->Effects[2].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 26654: // Sweeping Strikes damage spell
                spellInfo->AttributesEx |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
                spellInfo->Effects[0].Effect = SPELL_EFFECT_SCHOOL_DAMAGE;
                spellInfo->SchoolMask = SPELL_SCHOOL_MASK_NORMAL;
                break;
            case 144864:// Item - Druid T16 Feral 2P Bonus
                spellInfo->ProcFlags = 0;
                break;
                // Lava lash
            case 60103:
                spellInfo->Effects[2].Effect = SPELL_EFFECT_DUMMY;
                break;
            case 42292: // PvP trinket
            case 59752: // Every Man for Himself
            case 65547: // PvP trinket (Trial of Crusader)
                spellInfo->AttributesEx6 |= SPELL_ATTR6_IGNORE_CASTER_AURAS;
                break;
            case 125676:// Glyph of Afterlife
                spellInfo->Effects[0].BasePoints = 50;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_ADD_FLAT_MODIFIER;
                spellInfo->Effects[0].ValueMultiplier = 50;
                break;
            case 88852: // Tower of Radiance
                spellInfo->Effects[0].Effect = 0;
                break;
            case 24275: // Hammer of Wrath
                spellInfo->Attributes |= SPELL_ATTR0_IMPOSSIBLE_DODGE_PARRY_BLOCK;
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_BE_REFLECTED;
                spellInfo->Effects[0].Effect = SPELL_EFFECT_SCHOOL_DAMAGE;
                break;
            case 53592: // Guarded by the Light
                spellInfo->Attributes |= SPELL_ATTR0_ABILITY;
                spellInfo->Attributes |= SPELL_ATTR0_PASSIVE;
                spellInfo->Attributes |= SPELL_ATTR0_HIDE_IN_COMBAT_LOG;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_DEATH_PERSISTENT;
                break;
            case 29144: // Unwavering Sentinel
                spellInfo->Attributes |= SPELL_ATTR0_ABILITY;
                spellInfo->Attributes |= SPELL_ATTR0_PASSIVE;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_DEATH_PERSISTENT;
                break;
            case 118314:// Enchant Weapon - Colossus
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_PROC_TRIGGER_SPELL;
                spellInfo->Effects[0].TriggerSpell = 116631;
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 120033:// Enchant Weapon - Jade Spirit
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_PROC_TRIGGER_SPELL;
                spellInfo->Effects[0].TriggerSpell = 104993;
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 104441:// Enchant Weapon - River's Song
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_PROC_TRIGGER_SPELL;
                spellInfo->Effects[0].TriggerSpell = 116660;
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 118333:// Enchant Weapon - Dancing Steel
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_PROC_TRIGGER_SPELL;
                spellInfo->Effects[0].TriggerSpell = 120032;
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 104561:// Enchant Weapon - Windsong
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_PROC_TRIGGER_SPELL;
                spellInfo->Effects[0].TriggerSpell = 104562;
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 105708:// Master Health Potion
                spellInfo->Effects[0].BasePoints = 120000;
                break;
            case 5246:  // Intimidating Shout
                spellInfo->MaxAffectedTargets |= 5;
                break;
            case 147838:// Glyph of Impaling Throws
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_PERIODIC_DUMMY;
                spellInfo->Effects[0].Amplitude = 1000;
                break;
            case 33110: // Prayer of Mending
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ALLY;
                break;
            case 123259: // Prayer of Mending
                spellInfo->Effects[0].TriggerSpell = 0;
                break;
            case 67201: // Item - Priest T9 Healing 2P Bonus
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_ADD_PCT_MODIFIER;
                spellInfo->Effects[0].MiscValue = SPELLMOD_DAMAGE;
                break;
            case 87336: // Spiritual Healing
                spellInfo->Effects[0].SpellClassMask[1] = 0x24;
                break;
            case 118522:// Elemental Blast
                spellInfo->Effects[0].BasePoints = 3500;
                spellInfo->Effects[1].BasePoints = 3500;
                spellInfo->Effects[2].BasePoints = 3500;
                spellInfo->Effects[3].BasePoints = 3500;
                break;
            case 112071:// Celestial Alignment
                spellInfo->Effects[0].BasePoints = 0;
                break;
            case 116:
                spellInfo->PreventionType = 1;
                spellInfo->SchoolMask = SPELL_SCHOOL_MASK_FROST;
                break;
            case 24378: // Berserking
            case 23505: // Berserking
            case 132958:// Restoration
            case 24379: // Restoration
            case 23493: // Restoration
            case 23978: // Speed
            case 23451: // Speed
                spellInfo->AttributesEx |= SPELL_ATTR1_NOT_BREAK_STEALTH;
                break;
            case 45284: // Lightning Bolt (Mastery)
                //spellInfo->SpellFamilyFlags.Set(1, 0, 0, 0);
                spellInfo->InterruptFlags = 0; // to prevent a bug when they don't work in movement
                break;
            case 45297: // Chain Lighting (Mastery)
                //spellInfo->SpellFamilyFlags.Set(2, 0, 0, 0);
                spellInfo->InterruptFlags = 0; // to prevent a bug when they don't work in movement
                break;
            case 120588:// Lava Burst (Mastery)
            case 77451: // Lava Burst (Mastery)
                spellInfo->InterruptFlags = 0; // to prevent a bug when they don't work in movement
                break;
            case 120687:// Stormslash totem
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
                spellInfo->AttributesEx6 |= SPELL_ATTR6_IGNORE_CASTER_AURAS;
                break;
            case 96103: // Raging Blow (main-hand) - hotfix 5.4
            case 85384: // Raging Blow (off-hand)
                spellInfo->Effects[1].BasePoints = 228;
                break;
            case 115767:// Deep Wounds
                spellInfo->AttributesEx8 |= SPELL_ATTR8_DONT_RESET_PERIODIC_TIMER;
                break;
            case 51963: // Gargoyle Strike
                spellInfo->InterruptFlags = SPELL_INTERRUPT_FLAG_INTERRUPT;
                break;
            case 145674:// Riposte
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_RATING;
                break;
            case 32379: // Shadow Word : Death - Glyph of Shadow Word: Death
                spellInfo->OverrideSpellList.push_back(129176);
                break;
            case 120692:// Halo (Heal, Holy)
            case 120696:// Halo (Heal, Shadow);
                spellInfo->Effects[0].Effect = SPELL_EFFECT_HEAL;
                spellInfo->Effects[0].BasePoints = 1;
                break;
            case 126133: // Glyph of Lightwell (Holy)
                spellInfo->Effects[2].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[2].ApplyAuraName = SPELL_AURA_ADD_PCT_MODIFIER;
                spellInfo->Effects[2].MiscValue = SPELLMOD_DOT;
                spellInfo->Effects[2].BasePoints = 50;
                spellInfo->Effects[2].SpellClassMask[2] = 0x00004000; // Only work with Spell 126154
                break;
            case 48045: // Mind Sear
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_TARGET_SELF;
                break;
            case 79808: // Arcane Missiles Visual
                spellInfo->ProcFlags = 0;
                break;
            case 91724: // Spell Warding
                spellInfo->Effects[0].BasePoints = -6;
                break;
            case 116481:// Tiger's lust
                spellInfo->Attributes |= SPELL_ATTR0_ABILITY;
                spellInfo->AttributesEx5 &= ~SPELL_ATTR5_HASTE_AFFECT_DURATION;
                break;
            case 879:   // Exorcism
                spellInfo->AttributesEx2 |= SPELL_ATTR2_NOT_RESET_AUTO_ACTIONS;
                spellInfo->Effects[0].Effect = SPELL_EFFECT_SCHOOL_DAMAGE;
                break;
            case 20271: // Judgement
                spellInfo->Effects[0].Effect = SPELL_EFFECT_SCHOOL_DAMAGE;
                break;
            case 86704: // Ancient Fury
                spellInfo->Effects[0].Effect = SPELL_EFFECT_SCHOOL_DAMAGE;
                break;
            case 77613: // Grace
                spellInfo->Dispel = DISPEL_NONE;
                spellInfo->SpellIconID = 2819;
                break;
            case 120699:// Lynx Crush damage
                spellInfo->Effects[1].BasePoints = 100;
                spellInfo->Effects[1].Effect = SPELL_EFFECT_WEAPON_PERCENT_DAMAGE;
                break;
            case 128432:// Cackling Howl
            case 128433:// Serpent's Swiftness
            case 97229: // Bellowing Roal
            case 24604: // Furious Howl
            case 90309: // Terrifying Roar
            case 126373:// Fearless Roar
            case 93435: // Roar of Courage
                spellInfo->AttributesEx |= SPELL_ATTR1_NOT_BREAK_STEALTH;
                break;
            case 124271:// Sanguinary Vein
                spellInfo->Effects[0].BasePoints = 35;
                break;
            case 84601: // Assassin's Resolve
                spellInfo->Effects[1].BasePoints = 20;
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_MOD_DAMAGE_PERCENT_DONE;
                break;
            case 2818:  // Deadly Poison
                spellInfo->Attributes |= SPELL_ATTR0_IMPOSSIBLE_DODGE_PARRY_BLOCK;
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_BE_REDIRECTED;
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_BE_REFLECTED;
                break;
            case 108212:// Burst of Speed
                spellInfo->AttributesEx |= SPELL_ATTR1_NOT_BREAK_STEALTH;
                spellInfo->Effects[0].BasePoints = 70;
                break;
            case 31224: // Cloak of Shadows
                spellInfo->Effects[0].BasePoints = -200;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE;
                spellInfo->Effects[0].ValueMultiplier = -200;
                break;
            case 51690: // Killing Spree
                spellInfo->Effects[3].Effect = SPELL_EFFECT_FORCE_DESELECT;
                break;
            case 137619:// Marked for Death
                spellInfo->AttributesEx |= SPELL_ATTR1_NO_THREAT;
                break;
            case 8680:  // Wound Poison
            case 3409:  // Crippling Poison
            case 5760:  // Mind-numbing Poison
                spellInfo->Attributes |= SPELL_ATTR0_IMPOSSIBLE_DODGE_PARRY_BLOCK;
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_BE_REDIRECTED;
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_BE_REFLECTED;
                break;
            case 115098:// Chi Wave - Add Aura for bounce counting
                spellInfo->SchoolMask = SPELL_SCHOOL_MASK_NATURE;
                spellInfo->Effects[1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_DUMMY;
                spellInfo->Effects[1].BasePoints = 1;
                spellInfo->Effects[1].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[1].TargetB = 0;
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(8); // 15s
                spellInfo->Speed = 100.0f;
                break;
            case 125195:// Tigereye brew stacks
                spellInfo->Effects[2].BasePoints = 6;
            case 115867:// Mana Tea stacks
                spellInfo->StackAmount = 20;
                break;
            case 128939:// Elusive brew stacks
                spellInfo->StackAmount = 15;
                break;
            case 7376: // Defensive Stance
                spellInfo->Effects[3].ApplyAuraName = SPELL_AURA_PERIODIC_DUMMY;
                spellInfo->Effects[3].Amplitude = 3000;
                break;
            case 132169: // Storm Bolt
                spellInfo->Speed = 0;
                break;
            case 120267:// Vengeance
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                break;
			case 24907: // Moonkin Aura
				spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
				spellInfo->Effects[0].TargetB = TARGET_UNIT_CASTER_AREA_RAID;
			case 113860: // Dark Soul: Misery
				spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK;
				break;
            case 114168:// Dark Apotheosis (Glyph)
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_IGNORE_SHAPESHIFT;
                spellInfo->Effects[0].MiscValue = 22;
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[0].SpellClassMask[0] = 0x00000040;
                spellInfo->Effects[0].SpellClassMask[1] = 0x10000000;
                spellInfo->Effects[0].SpellClassMask[2] = 0x00000800;
                spellInfo->Effects[7].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[7].ApplyAuraName = SPELL_AURA_MOD_IGNORE_SHAPESHIFT;
                spellInfo->Effects[7].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[7].SpellClassMask[0] = 0x837FCFFF;
                spellInfo->Effects[7].SpellClassMask[1] = 0xED8FFFFF;
                spellInfo->Effects[7].SpellClassMask[2] = 0x828593FF;
                spellInfo->Effects[8].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[8].ApplyAuraName = SPELL_AURA_MOD_IGNORE_SHAPESHIFT;
                spellInfo->Effects[8].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[8].SpellClassMask[0] = 0x0000007F;
                spellInfo->Effects[9].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[9].ApplyAuraName = SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS;
                spellInfo->Effects[9].BasePoints = 114175;
                spellInfo->Effects[9].ValueMultiplier = 114175;
                break;
            case 145955: //Readiness trinket Evil Eye of Galakras 
            case 146019: //Readiness trinket Assurance of Consequence 
            case 146025: //Readiness trinket Vial of Living Corruption 
                spellInfo->AttributesEx3 |= SPELL_ATTR3_DEATH_PERSISTENT;
                break;
            case 88423: // Nature's Cure
                spellInfo->Effects[1].Effect = 0;
                spellInfo->Effects[2].Effect = 0;
                break;
            case 686:   // Shadow bolt override with Demonic Slash
                spellInfo->OverrideSpellList.push_back(114175);
                break;
            /*case 145518:// Genesis
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[0].TargetB = 0;*/

            case 29858: // Soulshatter
                spellInfo->OverrideSpellList.push_back(97827); // Add Taunt (Metamorphosis)
                break;
            case 103965:// Metamorphosis (override auras)
                spellInfo->Effects[2].SpellClassMask[0] = 64;
                spellInfo->Effects[7].SpellClassMask[1] = 0x400;
                spellInfo->Effects[15].ApplyAuraName = SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS;
                break;
            case 104025:// Immolation Aura
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_PERIODIC_TRIGGER_SPELL;
                spellInfo->Effects[0].TriggerSpell = 129476;
                break;
            case 1949:// Hellfire
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_PERIODIC_TRIGGER_SPELL;
                spellInfo->Effects[0].TriggerSpell = 5857;
                spellInfo->OverrideSpellList.push_back(104025);
                break;
            case 108686:// Immolate
            case 108685:// Conflagrate
            case 114654:// Incinerate
            case 109468:// Curse of Enfeeblement
            case 104225:// Curse of Elements
                spellInfo->AttributesEx4 |= SPELL_ATTR4_NOT_CHECK_SELFCAST_POWER;
                break;
            case 86213: // Soul Swap Exhale
                spellInfo->ManaCostPercentage = 0;
                spellInfo->ManaCost = 0;
                spellInfo->SpellLevel = 1;
				spellInfo->Speed = 0;
                break;
            case 105174:// Hand of Gul'dan
                spellInfo->Effects[0].Effect = SPELL_EFFECT_TRIGGER_MISSILE;
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[0].TriggerSpell = 86040;
                break;
            case 114790:// Seed of Corruption
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(85); // 18s
                break;
            case 102352:// Cenarion Ward (aura)
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 5421:  // Aquatic Form
                spellInfo->Effects[0].BasePoints = 50;
                break;
            case 1850:  // Dash
                 spellInfo->Effects[0].BasePoints = 56;
                 break;
            case 137452:  // Displacer Beast
                spellInfo->Effects[0].BasePoints = 75;
                break;
			case 145205: //Wild Mushroom Summon
				spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(21); // Infinite
				break;
            case 81262: // Efflorescence
                spellInfo->AttributesEx5 &= ~SPELL_ATTR5_START_PERIODIC_AT_APPLY;
                break;
			case 142423: //Efflorescence Force of Nature
				spellInfo->Effects[0].TargetA = 132;
				spellInfo->ExplicitTargetMask = TARGET_FLAG_NONE;
				break;
            case 114300:// Glyph of Stampede
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_IGNORE_SHAPESHIFT;
                spellInfo->Effects[0].SpellClassMask[0] = 0x8000000;
                break;
            case 115070:// Stance of the Wise Serpent
				spellInfo->Effects[6].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[6].ApplyAuraName = SPELL_AURA_INCREASE_HASTE_FROM_ITEMS_BY_PCT;
				spellInfo->Effects[6].BasePoints = 50;
                break;
            case 145109:// Ysera's Gift
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ALLY;
                break;
            case 145110: // Ysera's Gift (Party Heal)
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_100_YARDS);
                break;
            case 117050:// Glaive Toss (talent)
                spellInfo->Effects[1].TargetA = TARGET_UNIT_TARGET_ENEMY;
                break;
            case 120755:// Glaive Toss (Glaive right)
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 94528: // Flare
                spellInfo->AttributesEx6 |= SPELL_ATTR6_CAN_TARGET_INVISIBLE;
                break;
            case 148017:
            case 148018:
            case 148019:
            case 148020:
            case 148021:
            case 148022:
                spellInfo->AttributesEx6 &= ~SPELL_ATTR6_CANT_TARGET_CROWD_CONTROLLED;
                break;
            case 118694:// Spirit Bond (heal effect)
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AREA_AURA_PET;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_OBS_MOD_HEALTH;
                spellInfo->Effects[0].BasePoints = 3;
                spellInfo->Effects[0].Amplitude = 2000;
                break;
            case 50842: // Pestilence
                spellInfo->Effects[2].RadiusEntry = sSpellRadiusStore.LookupEntry(13); // 10 Yards
                break;
            case 43265: // Death and Decay
                spellInfo->Effects[3].Effect = SPELL_EFFECT_PERSISTENT_AREA_AURA;
                spellInfo->Effects[3].ApplyAuraName = SPELL_AURA_MOD_DECREASE_SPEED;
                //spellInfo->Effects[3].TargetA = TARGET_DEST_DEST;
                //spellInfo->Effects[3].TargetB = TARGET_DEST_DYNOBJ_ENEMY;
                break;
            case 108482:// Unbound Will
                // Crashes client on pressing ESC
				spellInfo->Effects[0].Effect = SPELL_EFFECT_DISPEL;
                spellInfo->AttributesEx4 &= ~SPELL_ATTR4_TRIGGERED;
				spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_STUNNED;
                break;
            case 66:    // Invisibility
                spellInfo->OverrideSpellList.push_back(110959); // Greater Invisibility
                spellInfo->Dispel = DISPEL_INVISIBILITY;
                break;
            case 110959:// Greater Invisibility
                spellInfo->Dispel = DISPEL_INVISIBILITY;
                break;
            case 47476: // Strangulate
                spellInfo->OverrideSpellList.push_back(108194); // Asphyxiate
                break;
            case 108194: // Asphyxiate
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_STUN;
                break;
            case 56131: // Glyph of Purify
                spellInfo->Effects[0].Effect = SPELL_EFFECT_HEAL_PCT;
                break;
            case 34428: // Victory Rush
                spellInfo->OverrideSpellList.push_back(103840); // Impending Victory
                break;
            case 91342: // Shadow Infusion
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 27285: // Seed of Corruption
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(13); // 10 Yards
                break;
            case 87385: // Soulburn : Seed of Corruption - damage
                spellInfo->Effects[0].TargetA = TARGET_UNIT_DEST_AREA_ENEMY;
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(13); // 10 Yards
                break;
            case 86664: // Soulburn: Seed of Corruption
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_DUMMY;
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(85);
                break;
            case 89523: // Glyph of Grounding Totem
                spellInfo->SpellFamilyName = SPELLFAMILY_SHAMAN;
                break;
            case 1856:  // Vanish
                spellInfo->Effects[1].TriggerSpell = 131368;
                spellInfo->Effects[0].Effect = SPELL_EFFECT_SANCTUARY;
                break;
            case 131369:// Vanish - Improved Stealth
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(27); // 3s
                break;
            case 116784:// Wildfire Spark - Boss Feng
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[0].TargetB = 0;
                spellInfo->Effects[1].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[1].TargetB = 0;
                break;
            case 67801: // Deterrence
                spellInfo->ProcCharges = 100;
                break;
            case 114234:// Glyph of Shred
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 120954:// Fortifying Brew
                spellInfo->Effects[4].ApplyAuraName = SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT;
                break;
            case 114871:// Holy Prism (Allies)
            case 114852:// Holy Prism (ennemies)
                spellInfo->Effects[2].Effect = 0;
                break;
            case 123154:// Fists of Fury Visual Target
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(35); // 4s
                break;
            case 53479: // Last Stand (Tenacity)
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT;
                break;
            case 104316:// Imp Swarm
                spellInfo->Effects[1].TargetA = TARGET_UNIT_CASTER;
                break;
            case 114736:// Disrupted Nether
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(4); // 120s
                break;
            case 126309:// Still Water
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[1].TargetA = TARGET_UNIT_CASTER;
                break;
            case 126311:// Surface Trot
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AREA_AURA_PET;
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 88767: // Fulmination (triggered)
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                break;
            case 20167: // Seal of insight
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[1].Effect = SPELL_EFFECT_DUMMY;
                break;
            case 146585:// Execution Sentence
            case 146586:// Stay of Execution
            case 53652: // Beacon of Light (heal)
            case 86273: // Illuminated Heal
            case 47753: // Divine Aegis (absorb aura)
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
                spellInfo->AttributesEx6 |= SPELL_ATTR6_IGNORE_CASTER_AURAS;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                break;
            case 49016: // Unholy Frenzy
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_PERIODIC_DUMMY;
            case 87023: // Cauterize
            case 110914:// Dark Bargain (DoT)
            case 113344:// Bloodbath (DoT)
            case 12654: // Mastery: Ignite
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
                break;
            case 124280:// Touch of Karma
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
                spellInfo->AttributesEx6 |= SPELL_ATTR6_IGNORE_CASTER_AURAS;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_BE_REFLECTED;
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
                break;
            case 101545: // Flying Serpent Kick
                spellInfo->Effects[6].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[6].ApplyAuraName = SPELL_AURA_PERIODIC_DUMMY;
                spellInfo->Effects[6].Amplitude = 200;
                break;
            case 130616:// Glyph of Fear effect
                spellInfo->Dispel = DISPEL_MAGIC;
                break;
            case 113792:// Pyschic Horror - Psyfiend
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[0].TargetB = 0;
                spellInfo->Effects[1].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[1].TargetB = 0;
                spellInfo->Effects[2].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[2].TargetB = 0;
                break;
            case 125972:// Felin Grace
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_SAFE_FALL;
                break;
            case 83968: // Mass Resurrect
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_DEAD;
                break;
            case 108507:// Kil'Jaeden's Cunning (passive)
                spellInfo->Effects[1].SpellClassMask[0] = 0x7C01487D;
                spellInfo->Effects[1].SpellClassMask[1] = 0x48022DF0;
                spellInfo->Effects[1].SpellClassMask[2] = 0x04000010;
                break;
            case 119049:// Kil'Jaeden's Cunning
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 116833:// Cosmetic Spirit Totem - Gara'Jal
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 130774:// Amethyst Pool - Periodic damage
                spellInfo->Attributes |= SPELL_ATTR0_CANT_CANCEL;
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
                break;
            case 123811:// Pheromones of Zeal
                spellInfo->Effects[0].TargetA = TARGET_UNIT_DEST_AREA_ENTRY;
                break;
            case 108503:// Grimoire of Sacrifice
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
				spellInfo->Effects[10].Effect = 0;
				spellInfo->Effects[9].Effect = 0;
				spellInfo->Effects[6].Effect = 0;
				spellInfo->Effects[8].Effect = SPELL_EFFECT_APPLY_AURA;
				spellInfo->Effects[8].ApplyAuraName = SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT;
				spellInfo->Effects[8].BasePoints = 20;
				spellInfo->Effects[8].TargetA = TARGET_UNIT_CASTER;
				spellInfo->Effects[8].TargetB = 0;
				spellInfo->Effects[1].BasePoints = 2;
				spellInfo->Effects[1].Effect = SPELL_EFFECT_APPLY_AURA;
				spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_OBS_MOD_HEALTH;
				spellInfo->Effects[1].TargetA = TARGET_UNIT_CASTER;			
				spellInfo->Effects[1].TargetB = 0;				
                break;
			// Abyssal SeaHorse ^ Sea Turtle Summon underwater Fix
			case 75207:
			case 64731:
			case 98718: /* Subdued Sea Horse Rare Underwater Mount*/
				spellInfo->AttributesEx8 = SPELL_ATTR8_WATER_MOUNT;
				break;
            case 119905:// Cauterize (Command Demon)
            case 119907:// Disarm (Command Demon)
            case 119909:// Whiplash (Command Demon)
            case 119910:// Spell Lock (Command Demon)
            case 119911:// Optical Blast (Command Demon)
            case 119913:// Fellash (Command Demon)
            case 119914:// Felstorm (Command Demon)
            case 119915:// Wrathstorm (Command Demon)
                spellInfo->AttributesEx6 &= ~SPELL_ATTR6_DONT_DISPLAY_COOLDOWN;
                break;
            case 119904:// Supplant Command Demon
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[1].Effect = 0;
                break;
            case 24259: // Spell Lock (Felhunter)
            case 115782:// Optical Blast (Observer)
                spellInfo->Speed = 80.0f;
                break;
            case 108199:// Gorefiend's Grasp
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 49206: // Summon Gargoyle
                spellInfo->AttributesEx5 |= SPELL_ATTR5_SINGLE_TARGET_SPELL;
                break;
            case 82691: // Ring of Frost
                spellInfo->AttributesEx5 &= ~SPELL_ATTR5_SINGLE_TARGET_SPELL;
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_BE_REFLECTED;
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(13);
                break;
            case 91021: // Find Weakness
                spellInfo->Effects[0].BasePoints = 100;
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 76577: // Smoke Bomb
                spellInfo->SpellVisual[1] = 20733;
                break;
            case 1833:// Cheap Shot
                spellInfo->Effects[3].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[3].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 55078:// Blood Plague
            case 55095:// Frost Fever
                spellInfo->Dispel = DISPEL_DISEASE;
                break;
            case 119611:// Renewing Mist
                spellInfo->ProcCharges = 3;
                spellInfo->SchoolMask = SPELL_SCHOOL_MASK_NATURE;
                break;
            case 118699:// Fear Effect
                spellInfo->Dispel = DISPEL_MAGIC;
                spellInfo->AttributesEx3 &= ~SPELL_ATTR3_IGNORE_HIT_RESULT;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_FEAR;
                break;
            case 124991:// Nature's Vigil (Damage)
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 124988:// Nature's Vigil (Heal)
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ALLY;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 122355:// Molten Core
                spellInfo->StackAmount = 255;				
                break;
			case 112965:
				spellInfo->Effects[3].BasePoints = 0;
				break;
            case 6203:  // Soulstone
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_DEAD;
                break;
            case 106707:// Faerie Swarm (talent)
                spellInfo->AttributesEx8 |= SPELL_ATTR8_AURA_SEND_AMOUNT;
                break;
            case 118291:// Greater Fire Elemental
            case 118323:// Greater Earth Elemental
                spellInfo->Effects[0].Effect = SPELL_EFFECT_SUMMON_PET;
                break;
            case 114942:// Healing Tide
                spellInfo->MaxAffectedTargets = 5;
                break;
            case 108283:// Echo of the Elements
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 116943:// Earthgrab
                spellInfo->AttributesEx5 |= SPELL_ATTR5_START_PERIODIC_AT_APPLY;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 8145:  // Tremor Totem effect
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_DUMMY;
            case 6474:  // Earthbind Totem
                spellInfo->AttributesEx5 |= SPELL_ATTR5_START_PERIODIC_AT_APPLY;
                break;
            case 8143: // Tremor Totem
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_USABLE_WHILE_SLEEPED;
                spellInfo->AttributesEx5 |= SPELL_ATTR5_START_PERIODIC_AT_APPLY;
                spellInfo->Effects[1].TargetA = TARGET_UNIT_CASTER_AREA_RAID;
                spellInfo->Effects[1].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_30_YARDS);
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_PERIODIC_DUMMY;
                spellInfo->Effects[1].Amplitude = 3000;
                break;
            case 2484:  // Earthbind Totem
                spellInfo->OverrideSpellList.push_back(51485);
                break;
            case 6544:  // Heroic Leap
                spellInfo->Effects[2].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[2].ApplyAuraName = SPELL_AURA_DUMMY;
                spellInfo->Effects[2].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[2].TargetB = 0;
                spellInfo->Effects[2].BasePoints = 0;
                break;
            case 116198:// Enfeeblement Aura
                spellInfo->AttributesCu &= ~SPELL_ATTR0_CU_NEGATIVE_EFF0;
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].TargetA = TARGET_DEST_TARGET_ENEMY;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_DEST_AREA_ENEMY;
                spellInfo->Effects[1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[1].TargetA = TARGET_DEST_TARGET_ENEMY;
                spellInfo->Effects[1].TargetB = TARGET_UNIT_DEST_AREA_ENEMY;
                spellInfo->Effects[2].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[2].TargetA = TARGET_DEST_TARGET_ENEMY;
                spellInfo->Effects[2].TargetB = TARGET_UNIT_DEST_AREA_ENEMY;
                spellInfo->Effects[4].TargetA = TARGET_DEST_TARGET_ENEMY;
                spellInfo->Effects[4].TargetB = TARGET_UNIT_DEST_AREA_ENEMY;
                break;
            case 116202:// Elements Aura
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].TargetA = TARGET_DEST_TARGET_ENEMY;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_DEST_AREA_ENEMY;
                spellInfo->Effects[2].TargetA = TARGET_DEST_TARGET_ENEMY;
                spellInfo->Effects[2].TargetB = TARGET_UNIT_DEST_AREA_ENEMY;
                break;
            case 853:   // Hammer of Justice
                spellInfo->OverrideSpellList.push_back(105593); // Replace Hammer of Justice by Fist of Justice
                break;
            case 94339: // Fungal Area
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(18); // 20s
                break;
            case 81282: // Fungal Growth
				spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(21); // Infinite
                break;
            case 108446:// Soul Link
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_DUMMY;
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[1].TargetA = TARGET_UNIT_PET;
                break;
            case 6785:  // Ravage
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_REQ_CASTER_BEHIND_TARGET;
                spellInfo->OverrideSpellList.push_back(102545); // Replace Ravage by Ravage (Incarnation)
                break;
            case 5212:  // Prowl
                spellInfo->OverrideSpellList.push_back(102547); // Replace Prowl by Prowl (Incarnation)
                break;
            case 16914: // Hurricane
                spellInfo->Effects[0].TargetB = 0;
                spellInfo->Effects[0].BasePoints = -50;
                spellInfo->Effects[0].Effect = SPELL_EFFECT_PERSISTENT_AREA_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_DECREASE_SPEED;
                spellInfo->Effects[0].Mechanic = MECHANIC_SNARE;
                spellInfo->OverrideSpellList.push_back(106996); // Replace Hurricane by Astral Storm
                break;
            case 80240: // Havoc
				spellInfo->StackAmount = 3;
				spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_DUMMY;	
				spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_DUMMY;
				spellInfo->Effects[1].TargetA = TARGET_UNIT_CASTER;		
				spellInfo->Effects[1].TargetB = 0;
                break;
				
            case 5487:  // Bear Form
                spellInfo->Effects[2].BasePoints = 120;
                break;
            case 145152:// Dream of Cenarius (cat)
                spellInfo->StackAmount = 2;
                break;
            case 145153:// Dream of Cenarius (restoration)
            case 94289: // Glyph of Protector of the Innocent
                    spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
                    spellInfo->AttributesEx6 |= SPELL_ATTR6_IGNORE_CASTER_AURAS;
                    spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                    spellInfo->Effects[EFFECT_0].RealPointsPerLevel = 0.f;
                    break;
            case 106996:// Astral Storm
                spellInfo->Effects[2].TargetB = 0;
                spellInfo->Effects[0].BasePoints = -50;
                spellInfo->Effects[0].Effect = SPELL_EFFECT_PERSISTENT_AREA_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_DECREASE_SPEED;
                spellInfo->Effects[0].Mechanic = MECHANIC_SNARE;
                break;
            case 85673: // Word of Glory
                spellInfo->OverrideSpellList.push_back(114163); // Replace World of glory by Eternal Flame
                break;
            case 86529: // Mail Specialization (Shaman)
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 61999: // Raise Ally
                spellInfo->Effects[1].TargetA = TARGET_UNIT_TARGET_ALLY;
                break;
            case 31935: // Avenger's Shield
                spellInfo->DmgClass = SPELL_DAMAGE_CLASS_MAGIC;
                break;
            case 498:   // Divine Protection
            case 30823: // Shamanistic Rage
            case 51490: // Thunderstorm
            case 108416:// Sacrificial Pact
            case 137562:// Nimble Brew
            case 33206: // Pain Suppression
            case 116849:// Life Cocoon
                spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_STUNNED;
                break;
            case 115610:// Temporal Shield
                spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_STUNNED;
                spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_FEARED;
                break;
            case 44203: // Tranquility (triggered)
                spellInfo->MaxAffectedTargets = 5;
                break;
            case 121118:// Dire Beast summons
            case 122802:
            case 122804:
            case 122806:
            case 122807:
            case 122809:
            case 122811:
            case 126213:
            case 126214:
            case 126215:
            case 126216:
            case 132764:
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 19574: // Bestial Wrath
                spellInfo->Effects[3].Effect = 0;
                spellInfo->Effects[3].ApplyAuraName = 0;
                break;
            case 87935: // Serpent Spread
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_DUMMY;
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(21); // -1s
                break;
            case 53257: // Cobra Strikes
                spellInfo->Effects[0].BasePoints = 0;
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_MOD_CRIT_PCT;
                break;
            case 124974: // Nature's Vigil
                spellInfo->Effects[2].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 21562: // Power Word : Fortitude
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_CASTER_AREA_RAID;
                break;
            case 1126:  // Mark of the Wild
            case 19740: // Blessing of Might
            case 20217: // Blessing of Kings
            case 116781:// Legacy of the White Tiger
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_CASTER_AREA_RAID;
                break;
            case 109773:// Dark Intent
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_CASTER_AREA_RAID;
                spellInfo->Effects[1].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[1].TargetB = TARGET_UNIT_CASTER_AREA_RAID;
                spellInfo->Effects[1].MiscValue = 2;
                break;
            case 1459:  // Arcane Illumination
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER_AREA_RAID;
                spellInfo->Effects[1].TargetA = TARGET_UNIT_CASTER_AREA_RAID;
                break;
            case 61316: // Dalaran Illumination
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER_AREA_RAID;
                spellInfo->Effects[2].TargetA = TARGET_UNIT_CASTER_AREA_RAID;
                break;
            case 11958: // Cold Snap
				spellInfo->AttributesEx6 |= SPELL_ATTR6_IGNORE_CASTER_AURAS;
              /*  spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_FEARED;
                spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_STUNNED;
                spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_CONFUSED;*/
                break;
            case 112023:// Sparring (dummy)
                spellInfo->AuraInterruptFlags = 0x0;
                spellInfo->ChannelInterruptFlags = 0x0;
                spellInfo->InterruptFlags = 0x0;
                break;
            case 115073:// Spinning Fire Blossom
                spellInfo->Effects[0].TargetA = 0;
                spellInfo->Effects[0].Effect = 0;
                spellInfo->Effects[0].BasePoints = 0;
                spellInfo->OverrideSpellList.push_back(123408); // Add Spinning Fire Blossom (Overrided by Glyph) to override spell list of Spinning Fire Blossom
                break;
            case 86698: // Guardian of Ancient Kings
                spellInfo->Effects[0].TargetA = TARGET_CHECK_ENTRY;
                break;
            case 86674: // Ancient Healer
                spellInfo->ProcCharges = 5;
                break;
            case 86657: // Ancient Guardian
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN;
                break;
            case 5782:  // Fear
                spellInfo->Dispel = DISPEL_MAGIC;
                spellInfo->Mechanic = 0;
                spellInfo->Effects[0].Mechanic = MECHANIC_NONE;
                spellInfo->OverrideSpellList.push_back(104045);
                break;
            case 124081: // Zen Sphere
                spellInfo->SchoolMask = SPELL_SCHOOL_MASK_NATURE;
                spellInfo->AttributesEx5 &= ~SPELL_ATTR5_SINGLE_TARGET_SPELL;
                break;
            case 51460: // Runic Corruption
                spellInfo->Effects[EFFECT_1].Effect = 0;
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_MOD_POWER_REGEN_PERCENT;
                spellInfo->Effects[EFFECT_0].MiscValue = 5;
                spellInfo->Effects[EFFECT_0].MiscValueB = NUM_RUNE_TYPES;
                break;
            case 45204: // Mirror Image - Clone Me!
                spellInfo->AttributesEx6 |= SPELL_ATTR6_CAN_TARGET_INVISIBLE;
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 41055: // Copy Weapon Spells
            case 45206:
            case 63416:
            case 69891:
            case 69892:
                spellInfo->Effects[0].Effect = SPELL_EFFECT_DUMMY;
                spellInfo->Mechanic = 0;
                break;
            case 20066: // Repentence
                spellInfo->PreventionType = SPELL_PREVENTION_TYPE_SILENCE;
                break;
            case 115175:// Soothing Mists
            case 116694:// Surging Mists
            case 117952:// Crackling Jade Lightning
                spellInfo->SchoolMask = SPELL_SCHOOL_MASK_NATURE;
                spellInfo->PreventionType = SPELL_PREVENTION_TYPE_SILENCE;
                break;
            case 102794://Ursol's Vortex Inmune
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_DUMMY;
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(1); // 10s
                spellInfo->Attributes |= (SPELL_ATTR0_PASSIVE | SPELL_ATTR0_HIDDEN_CLIENTSIDE);
                break;
            case 172:   // Corruption
            case 30108: // Unstable Affliction
                spellInfo->Effects[2].Effect = 0;
                spellInfo->Effects[2].ApplyAuraName = 0;
                spellInfo->Effects[2].BasePoints = 0;
                break;
            case 34433: // Shadowfiend
                spellInfo->Effects[EFFECT_0].MiscValueB = 1561;
                spellInfo->OverrideSpellList.push_back(123040); // Add Mindbender to override spell list of Shadowfiend
                break;
            case 64904: // Hymn of Hope
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_MOD_INCREASE_ENERGY_PERCENT;
                break;
            case 59907: // Lightwell Charges
                spellInfo->Effects[0].BasePoints = 15;
                spellInfo->ProcCharges = 15;
                break;
            case 126135:// Lightwell
                spellInfo->OverrideSpellList.push_back(724); // Add old Lightwell to override list
                break;
            case 81751: // Atonement
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ALLY;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 47515: // Divine Aegis
                spellInfo->Effects[0].BasePoints = 100;
                break;
            case 88345: // Portal : Tol Barad(SPELL_ATTR7_ALLIANCE_ONLY)
            case 88346: // Portal: Tol Barad (SPELL_ATTR7_HORDE_ONLY)
                spellInfo->Effects[0].Effect = SPELL_EFFECT_DUMMY;
                break;
            case 108201:// Desecrated Ground
                spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_FEARED;
                spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_STUNNED;
                spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_CONFUSED;
                spellInfo->AttributesEx |= SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY;
                break;
            case 9005:  // Pounce
                spellInfo->OverrideSpellList.push_back(102546); // Replace Pounce by Pounce (Incarnation)
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(35); // 4s
                break;
            case 102359:// Mass Entanglement
                spellInfo->AttributesEx5 &= ~SPELL_ATTR5_SINGLE_TARGET_SPELL;
                break;
            case 33763: // Lifebloom
                spellInfo->AttributesEx5 |= SPELL_ATTR5_SINGLE_TARGET_SPELL;
                break;
            case 3411:  // Intervene
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_RAID;
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_TARGET_SELF;
                spellInfo->AttributesEx7 |= SPELL_ATTR7_HAS_CHARGE_EFFECT;
                spellInfo->OverrideSpellList.push_back(114029); // Add Safeguard to override spell list of Intervene
                break;
            case 114029:// Safeguard
                spellInfo->Effects[2].BasePoints = 100;
                break;
            case 116768:// Combo Breaker: Blackout Kick
               spellInfo->ProcFlags = 0;
                break;
            case 1784:  // Stealth
                spellInfo->OverrideSpellList.push_back(115191); // Add Stealth (talent) to override spell list of Stealth
                break;
            case 115191:// Subterfuge
                spellInfo->Attributes &= ~SPELL_ATTR0_BREAKABLE_BY_DAMAGE;
                spellInfo->AttributesEx |= SPELL_ATTR0_DISABLED_WHILE_ACTIVE;
                spellInfo->Attributes |= SPELL_ATTR0_STOP_ATTACK_TARGET;
                spellInfo->AttributesEx8 |= SPELL_ATTR8_AURA_SEND_AMOUNT;
                spellInfo->ProcFlags &= ~PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS;
                spellInfo->ProcFlags &= ~PROC_FLAG_DONE_SPELL_RANGED_DMG_CLASS;
                spellInfo->ProcFlags &= ~PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS;
                spellInfo->ProcFlags &= ~PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG;
                spellInfo->ProcFlags &= ~PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS;
                spellInfo->ProcFlags &= ~PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG;
                spellInfo->ProcFlags &= ~PROC_FLAG_DONE_PERIODIC;
                spellInfo->ProcFlags &= ~PROC_FLAG_DONE_MELEE_AUTO_ATTACK;
                spellInfo->ProcFlags &= ~PROC_FLAG_DONE_RANGED_AUTO_ATTACK;
                break;
            case 128766:// Shadowstep teleport only (talent)
            case 36554: // Shadowstep
            case 36563: // Shadowstep (trigger)
                spellInfo->AttributesEx |= SPELL_ATTR1_NOT_BREAK_STEALTH;
                break;
            case 140376: // Ring of frost (presence of mind)
                spellInfo->PreventionType = SPELL_PREVENTION_TYPE_SILENCE;
                spellInfo->AttributesEx &= ~SPELL_ATTR1_NOT_BREAK_STEALTH;
                spellInfo->AttributesEx5 &= ~SPELL_ATTR5_USABLE_WHILE_FEARED;
                spellInfo->AttributesEx5 &= ~SPELL_ATTR5_USABLE_WHILE_STUNNED;
                spellInfo->AttributesEx5 &= ~SPELL_ATTR5_USABLE_WHILE_CONFUSED;
                spellInfo->AttributesEx6 &= ~SPELL_ATTR6_CASTABLE_WHILE_ON_VEHICLE;
                spellInfo->Effects[1].Effect = SPELL_EFFECT_SUMMON;
                spellInfo->Effects[1].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[1].MiscValue = 44199;
                spellInfo->Effects[1].MiscValueB = 3018;
                spellInfo->Effects[2].Effect = SPELL_EFFECT_TRIGGER_SPELL;
                spellInfo->Effects[2].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[2].BasePoints = 10;
                spellInfo->Effects[2].TriggerSpell = 136511;
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(1); // 10s
				spellInfo->SpellFamilyName = SPELLFAMILY_MAGE;
				spellInfo->SpellFamilyFlags = flag128(1933836341, 6144, 4460608); // Same of presence of mind
                break;
            case 115192:// Subterfuge
                spellInfo->Attributes |= SPELL_ATTR0_DONT_AFFECT_SHEATH_STATE;
                spellInfo->Attributes |= SPELL_ATTR0_NOT_SHAPESHIFT;
                spellInfo->AttributesEx |= SPELL_ATTR1_NOT_BREAK_STEALTH;
                break;
            case 130493:// Nightstalker
                spellInfo->Effects[1].Effect = 0;
                spellInfo->Effects[1].ApplyAuraName = 0;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_DAMAGE_PERCENT_DONE;
                spellInfo->Effects[0].MiscValue = SPELL_SCHOOL_MASK_ALL;
                break;
            case 84745: // Shallow Insight
            case 84746: // Moderate Insight
            case 84747: // Deep Insight
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_DAMAGE_PERCENT_DONE;
                spellInfo->Effects[0].MiscValue = SPELL_SCHOOL_MASK_ALL;
                break;
            case 121733:// Throw
                spellInfo->OverrideSpellList.push_back(114014); // Add Shuriken Toss to override spell list of Throw
                break;
            case 105361:// Seal of Command
                spellInfo->OverrideSpellList.push_back(31801); // Replace Seal of Command with Seal of Truth
                break;
            case 117895:// Eminence (statue)
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ALLY;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 115294:// Mana Tea
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(36); // 1s
                break;
            case 44457: // Living Bomb
                spellInfo->AttributesEx5 &= ~SPELL_ATTR5_SINGLE_TARGET_SPELL;
                spellInfo->MaxAffectedTargets = 3;
                break;
            case 23691: // Berzerker Rage Effect
                spellInfo->Effects[0].BasePoints = 100;
                break;
            case 114205:// Demoralizing Banner
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(8); // 15s
                spellInfo->AttributesEx |= SPELL_ATTR1_NOT_BREAK_STEALTH;
                break;
            case 77762: // Lava Surge (cast time)
				// we will remove it in spellscript, to prevent bug
                spellInfo->ProcFlags = 0; 
				spellInfo->ProcCharges = 0;
                break;
            case 78675: // Sunbeam
            case 113286:
                
				spellInfo->Effects[2].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 81261: // Sunbeam
            case 113287:
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(285); // 1s
                break;
            case 29166: // Innervate
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_PERIODIC_DUMMY;
                break;
            case 144442:// Death Sentence (proc from Item - Warrior T16 DPS 4P Bonus)
                spellInfo->AttributesEx4 |= SPELL_ATTR4_TRIGGERED;
                spellInfo->ProcFlags = 0;
                break;
            case 127630:// Cascade - damage trigger
            case 120786:// Cascade - heal trigger
                spellInfo->Effects[1].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[1].TargetB = 0;
                break;
            case 12975:  // Last Stand
            case 17767:  // Shadow bulwark
            case 132413: // Shadow bulwark
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT;
                break;
            case 107566: // Staggering Shout
                spellInfo->ProcCharges = 1;
                break;
            case 122507:// Rallying Cry
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT;
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(1);
                break;
            case 106922:// Might of Ursoc
            case 113072:// Might of Ursoc (Symbiosis)
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT;
                break;
            case 119032: // Spectral Guise
                spellInfo->ProcFlags = 0;
                break;
            case 113890:
                spellInfo->Effects[0].TargetA = TARGET_DEST_DEST;
                break;
            case 113886:
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 143646: // Balanced Trillium Ingot and Its Uses
                spellInfo->Effects[1].Effect = SPELL_EFFECT_LEARN_SPELL;
                spellInfo->Effects[1].TriggerSpell = 143255;
                break;
            case 143626: // Celestial Cloth and Its Uses
                spellInfo->Effects[1].Effect = SPELL_EFFECT_LEARN_SPELL;
                spellInfo->Effects[1].TriggerSpell = 143011;
                break;
            case 143644: // Hardened Magnificent Hide and Its Uses
                spellInfo->Effects[1].Effect = SPELL_EFFECT_LEARN_SPELL;
                spellInfo->Effects[1].TriggerSpell = 142976;
                break;
            case 117828:// Backdraft
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_ADD_PCT_MODIFIER;
                spellInfo->Effects[0].MiscValue = SPELLMOD_CASTING_TIME;
                spellInfo->Effects[0].BasePoints = -30;
                spellInfo->Effects[0].SpellClassMask[1] |= 0x2000;
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_ADD_PCT_MODIFIER;
                spellInfo->Effects[1].MiscValue = SPELLMOD_COST;
                spellInfo->Effects[1].BasePoints = -30;
                spellInfo->Effects[1].SpellClassMask[1] |= 0x80;
                spellInfo->Effects[1].SpellClassMask[0] |= 0x1000;
                spellInfo->Effects[2].ApplyAuraName = SPELL_AURA_ADD_PCT_MODIFIER;
                spellInfo->Effects[2].MiscValue = SPELLMOD_CASTING_TIME;
                spellInfo->Effects[2].BasePoints = -30;
                spellInfo->Effects[2].SpellClassMask[1] |= 0x80;
                spellInfo->Effects[2].SpellClassMask[0] |= 0x1000;
                break;
            case 109259:// Powershot
                spellInfo->Effects[1].BasePoints = 70;
                spellInfo->Effects[2].BasePoints = 90;
                spellInfo->Effects[2].Effect = SPELL_EFFECT_WEAPON_PERCENT_DAMAGE;
                break;
            case 7922: // Charge Stun
                spellInfo->Speed = 0.2f;
                spellInfo->AttributesEx9 |= SPELL_ATTR9_SPECIAL_DELAY_CALCULATION;
                break;
            case 82926: // Fire ! (for Master Marksman)
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_ADD_PCT_MODIFIER;
                spellInfo->Effects[0].MiscValue = SPELLMOD_CASTING_TIME;
                spellInfo->Effects[0].BasePoints = -100;
                spellInfo->Effects[0].SpellClassMask[0] |= 0x20000;
                break;
            case 7384: // Overpower
                spellInfo->AttributesEx |= SPELL_ATTR1_ENABLE_AT_DODGE;
                break;
            case 114695:// Pursuit of Justice
                spellInfo->Effects[0].BasePoints = 0;
                break;
            case 6262:  // Healthstone
                spellInfo->Effects[0].Effect = SPELL_EFFECT_HEAL_PCT;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_OBS_MOD_HEALTH;
                spellInfo->Effects[1].BasePoints = 4;
                break;
            case 56224: // Glyph of Healthstone
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_ADD_PCT_MODIFIER;
                spellInfo->Effects[0].SpellClassMask[0] = 0x10000;
                spellInfo->Effects[0].BasePoints = 100;
                break;
            case 53390:// Tidal Waves
                spellInfo->Effects[0].BasePoints = -30;
                spellInfo->Effects[1].BasePoints = 30;
                break;
            case 116218:// Glyph of Regrowth
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_ADD_FLAT_MODIFIER;
                spellInfo->Effects[0].MiscValue = SPELLMOD_CRITICAL_CHANCE;
                break;
                /*case 63304: // Glyph of Ember Trap
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_ADD_PCT_MODIFIER;
                spellInfo->Effects[0].SpellClassMask[0] = 0x20000;
                spellInfo->Effects[0].BasePoints = 5;
                break;*/
            case 34299: // Leader of the pack - healing
            case 81280: // Blood Burst
                spellInfo->Effects[0].Effect = SPELL_EFFECT_HEAL_PCT;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                break;
            case 90259: // Glyph of Frost Pillar (Root Aura)
                spellInfo->Effects[0].MiscValue = 0;
                spellInfo->Effects[0].MiscValueB = 0;
                spellInfo->Effects[EFFECT_1].BasePoints = -70;
                break;
            case 49821: // Mind Sear
                spellInfo->Effects[0].TargetA = TARGET_DEST_CHANNEL_TARGET;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_DEST_AREA_ENEMY;
                break;
            case 118473:// Unleashed Fury
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ALLY;
                spellInfo->Effects[0].TargetB = 0;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_HEALING_RECEIVED;
                spellInfo->Effects[0].SpellClassMask = flag128(0xC0, 0, 0x10010);
                spellInfo->Effects[0].MiscValue = 11;
                break;
            case 131567:// Holy Spark
                spellInfo->Effects[0].MiscValue = 11;
                spellInfo->ProcCharges = 1;
                break;
            case 974:// Earth Shield
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_MOD_HEALING_RECEIVED;
                spellInfo->Effects[1].MiscValue = 1;
                break;
            case 73920: // Healing Rain
                spellInfo->Effects[3].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[3].TargetB = 0;
                spellInfo->Effects[3].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[3].ApplyAuraName = SPELL_AURA_MOD_HEALING_DONE_PERCENT;
                spellInfo->Effects[3].SpellClassMask = flag128(0, 0, 0x800000);
                spellInfo->MaxAffectedTargets = 10;
                break;
            case 10326: // Turn Evil
                spellInfo->OverrideSpellList.push_back(145067); // Evil is a point of view
                spellInfo->Mechanic = MECHANIC_TURN;
                spellInfo->SchoolMask = SPELL_SCHOOL_MASK_HOLY;
                spellInfo->PreventionType = SPELL_PREVENTION_TYPE_SILENCE;
                break;
            case 145067:// Turn Evil with talent
                spellInfo->Mechanic = MECHANIC_TURN;
                spellInfo->SchoolMask = SPELL_SCHOOL_MASK_HOLY;
                spellInfo->PreventionType = SPELL_PREVENTION_TYPE_SILENCE;
                break;
            case 113656:// Fists of Fury
                spellInfo->PreventionType = 6;
                break;
            case 115315:// Summon Black Ox Statue
                spellInfo->Effects[1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_DUMMY;
                spellInfo->Effects[1].TargetA = TARGET_UNIT_CASTER;
                break;
            case 13165: // Aspect of the Hawk
                spellInfo->OverrideSpellList.push_back(109260); // Add Aspect of the Iron Hack to override spell list of Aspect of the Hawk
                spellInfo->Effects[0].BasePoints = 35;
                break;
            case 34936: // Backlash
                spellInfo->ProcFlags = 0;
                break;
            case 113770://Root (Force of Nature balance treant)
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(31);
                break;
            case 86172: // Divine Purpose
                spellInfo->Effects[0].TriggerSpell = 0;
                break;
            case 132158:// Nature's Swiftness
                spellInfo->Effects[4].SpellClassMask = flag128(0x00000240, 0, 0x00008000);
            case 6346:  // Fear Ward
            case 110717:// Fear Ward (Symbiosis)
            case 57761: // Brain Freeze
            case 23920: // Spell Reflection
            case 113002:// Spell Reflection (Symbiosis)
            case 124430:// Divine Insight (Shadow)
            case 81292: // Glyph of Mind Spike
            case 114250:// Selfless Healer
            case 90174: // Divine Purpose
            case 69369: // Predator Swiftness
            case 108382:// Dream of Cenarius (second proc)
            case 113853:// Blazing Speed aurastate
            case 114028:// Mass Spell Reflection
            case 93400: //  Shooting Stars
            case 122510:// Ultimatum
            case 34784: // Intervene (triggered)
                spellInfo->ProcCharges = 1;
                break;
            case 110600:// Ice Trap (Symbiosis)
                spellInfo->Effects[0].MiscValue = 164639;
                break;
            case 110588:// Misdirection (Symbiosis)
                spellInfo->Effects[2].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[2].ApplyAuraName = SPELL_AURA_MOD_SCALE;
                spellInfo->Effects[2].BasePoints = 30;
                break;
            case 122292:// Intervene (Symbiosis)
                spellInfo->Effects[1].BasePoints = 100;
                break;
            case 6358:  // Seduce (succubus)
            case 115268:// Mesmerize
                spellInfo->SpellFamilyName = SPELLFAMILY_WARLOCK;
                break;
            case 980:   // Agony
                spellInfo->StackAmount = 10;
                break;
            case 131740:// Corruption (Malefic Grasp)
            case 131736:// Unstable Affliction (Malefic Grasp)
            case 132566:// Seed of Corruption (Malefic Grasp)
            case 131737:// Agony (Malefic Grasp)
            case 42463: // Seal of Truth
            case 124464:// Shadow Word: Pain (Mastery instant damage)
            case 124465:// Vampiric Touch (Mastery instant damage)
            case 124467:// Devouring Plague (Mastery instant damage)
            case 124468:// Mind Flay (Mastery instant damage)
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                break;
            case 124495: // Devouring Plague Visual
                spellInfo->ProcFlags = 0;
                break;
            case 131116:// Allow to use Raging Blow
            case 108381:// Dream of Cenarius (first proc)
                spellInfo->ProcCharges = 2;
                break;
            case 44544: // Fingers of Frost
				spellInfo->StackAmount = 2;
				spellInfo->Effects[2].TriggerSpell = 0;
				break;
            case 126084:// Fingers of Frost - visual
                spellInfo->StackAmount = 2;
                break;
            case 85222: // Light of Dawn
                spellInfo->MaxAffectedTargets = 6;
                break;
            case 2641:  // Dismiss Pet
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_DEAD;
                break;
            case 52042: // Healing Stream - Totem
                spellInfo->Effects[0].BasePoints = 31;
                break;
            case 324:   // Lightning Shield
                spellInfo->ProcCharges = 0;
                break;
            case 116740:// Tigereye Brew
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(1);
                break;
            case 117993:// Chi Torpedo : Heal
            case 124040:// Chi Torpedo : Damage
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(9);
                break;
            case 80325: // Camouflage
            case 119450:// Glyph of Camouflage
                spellInfo->Effects[1].Effect = 0;
                break;
            case 115008:// Chi Torpedo
                spellInfo->OverrideSpellList.push_back(121828); // Override List with Chi Torpedo - Talent
                spellInfo->Effects[7].TargetA = TARGET_UNIT_PROYECTILE;
                spellInfo->Effects[7].Effect = SPELL_EFFECT_DUMMY;
                spellInfo->Effects[7].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_21_YARDS);
                break;
            case 109132:// Roll
                spellInfo->OverrideSpellList.push_back(115008); // Override List with Chi Torpedo
                spellInfo->OverrideSpellList.push_back(121827); // Override List with Roll - Talent
                break;
            case 115295:// Guard
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_MOD_HEALING_DONE_PERCENT;
                spellInfo->Effects[1].BasePoints = 30;
                break;
            case 124273:// Heavy Stagger
            case 124274:// Moderate Stagger
            case 124275:// Light Stagger
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_PERIODIC_DAMAGE;
                spellInfo->Effects[0].Amplitude = 1000;
                spellInfo->Effects[0].BasePoints = 0;
                break;
            case 108937:// Baby Elephant Takes a Bath
                spellInfo->Effects[1].BasePoints = 40;
                spellInfo->Effects[1].MiscValue = 100;
                break;
            case 126451:// Clash - Impact
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(29);
                break;
            case 121253:// Keg Smash
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(14);
                spellInfo->MaxAffectedTargets = 3;
                break;
            case 115276:// Sear magic
                spellInfo->MaxAffectedTargets = 3;
                break;
            case 115308:// Elusive Brew
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(1);
                break;
            case 115129:// Expel Harm - Damage to a nearby ennemy within 10 yards
                spellInfo->Effects[0].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_NEARBY_ENEMY;
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(13);
                break;
            case 102792: // Wild Mushroom: Bloom			
                spellInfo->Effects[0].SetRadiusIndex(EFFECT_RADIUS_10_YARDS);
                break;
            case 126892:// Zen Pilgrimage
            case 126895:// Zen Pilgrimage : Return
                spellInfo->Effects[0].Effect = SPELL_EFFECT_DUMMY;
                break;
            case 130320:// Rising Sun Kick - Monks abilities deal 10% more damage
                spellInfo->Effects[0].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(14);
                break;
            case 101546:// Spinning Crane Kick - Decrease Speed
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_MOD_DECREASE_SPEED;
                spellInfo->Effects[1].BasePoints = -30;
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(14);
                spellInfo->OverrideSpellList.push_back(116847);
                break;
            case 125084: // Charging Ox Wave
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(10); // radius 30
                break;
            case 107270:// Spinning Crane Kick - Radius
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(14);
                break;
            case 107223:
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                break;
            case 113315:
            {
                           SpellInfo const* spell = sSpellMgr->GetSpellInfo(113379);
                           if (!spell)
                               break;
                           spellInfo->DurationEntry = spell->DurationEntry;
            }
                break;
            case 106736:
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_TARGET_ENEMY;
                break;
            case 106113:
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_TARGET_ENEMY;
                break;
            case 119922: //Shockwave
            case 119929:
            case 119930:
            case 119931:
            case 119932:
            case 119933:
                spellInfo->Speed = 5.0f;
                break;
            case 106847: // Barrel Toss
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;         
            case 138391:// Alpha Male Oondasta
                spellInfo->AttributesEx3 &= ~SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
            case 144699:// Ancient Flame Ordos
			case 97079: // Firelands Bethilac Seeping Venom
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 120552:// Mantid Munition Explosion
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(16);
                break;
            case 119684:// Ground Slam
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CONE_ENEMY_24;
                spellInfo->Effects[0].TargetB = 0;
                spellInfo->Effects[1].TargetA = TARGET_UNIT_CONE_ENEMY_24;
                spellInfo->Effects[1].TargetB = 0;
                break;
            case 106853:// Fists of Fury
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 112060:// Apparitions
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 118685:// Battering Headbutt
                spellInfo->RangeEntry = sSpellRangeStore.LookupEntry(5);
                break;
            case 60670: // Malygos Enrage
                spellInfo->Effects[1].TriggerSpell = 0;
                spellInfo->Effects[2].TriggerSpell = 0;
                break;
            case 114746:// Snatch Master's Flame
                spellInfo->Effects[2].TargetA = TARGET_UNIT_TARGET_ALLY;
                spellInfo->Effects[2].TargetB = 0;
                break;
            case 104855:// Overpacked Firework
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
                // Add Server-Side dummy spell for Fishing
                // TODO : Add more generic system to load server-side spell
            case 7733:  // Apprentice Fishing
            case 7734:  // Journeyman Fishing
            case 18249: // Artisan Fishing
            case 54083: // Expert Fishing
            case 54084: // Master Fishing
            case 51293: // Grand Master Fishing
            case 88869: // Illustrious Grand Master Fishing
            case 110412:// Zen Master Fishing
            {
                            SpellInfo* fishingDummy = new SpellInfo(sSpellStore.LookupEntry(131474), difficulty);
                            fishingDummy->Id = spellInfo->Effects[0].TriggerSpell;
                            mSpellInfoMap[difficulty][spellInfo->Effects[0].TriggerSpell] = fishingDummy;
                            break;
            }
                // Mogu'shan Vault
            case 116778:
            case 116525:
                spellInfo->AttributesEx &= ~SPELL_ATTR1_CHANNELED_1;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
            case 116227:
                spellInfo->Effects[EFFECT_0].Effect = 0;
                break;
            case 115911:
                spellInfo->ChannelInterruptFlags = 0x0;
                spellInfo->AuraInterruptFlags = 0x0;
                break;
            case 118530:
                spellInfo->MaxAffectedTargets = 3;
                break;
            case 121224:
                spellInfo->MaxAffectedTargets = 8;
                break;
            case 116060:
            case 115861:
            case 116008:
            case 116038:
            case 116044:
                spellInfo->Attributes &= ~SPELL_ATTR0_HIDE_IN_COMBAT_LOG;
                spellInfo->AttributesEx &= ~SPELL_ATTR1_DONT_DISPLAY_IN_AURA_BAR;
                break;
            case 116000:// Voodoo Dolls
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 116161:// Crossed Over
                spellInfo->Effects[EFFECT_1].MiscValue = 2; // Set Phase to 2
                spellInfo->Effects[EFFECT_3].Effect = 0; // No need to summon
                break;
            case 116272:// Banishment
                spellInfo->Effects[EFFECT_0].MiscValue = 2; // Set Phase to 2
                break;
            case 116606:// Troll Rush
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 118303:// Fixate
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 15850: // Chilled
            case 16927: // Chilled
            case 20005: // Chilled
                spellInfo->Mechanic = MECHANIC_SNARE;
                break;
            case 89640: // Water Spirit
                spellInfo->Effects[EFFECT_0].BasePoints = 5000;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                break;
            case 130649:// Life Spirit
                spellInfo->Effects[EFFECT_0].BasePoints = 20000;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                break;
			case 83991:/*The black bishop Teleport*/
				spellInfo->MaxAffectedTargets = 1;
				spellInfo->Effects[EFFECT_0].TargetA = SpellImplicitTargetInfo(TARGET_UNIT_TARGET_ANY);
				spellInfo->Effects[EFFECT_0].TargetB = SpellImplicitTargetInfo();
				break;	
            case 127362:
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
                // Isle of Conquest Gunship Portal
            case 66630:
            case 66637:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 66550: // teleports outside (Isle of Conquest)
            case 66551: // teleports inside (Isle of Conquest)
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_DUMMY;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
                // Tree of Life
            case 65139:
                spellInfo->Effects[1].Effect = 0;
                spellInfo->Effects[1].ApplyAuraName = 0;
                break;
            case 60206: // Ram
                spellInfo->Effects[2].RadiusEntry = sSpellRadiusStore.LookupEntry(13);
                break;
            case 96172: // Hand of Light
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                spellInfo->AttributesEx6 |= SPELL_ATTR6_IGNORE_CASTER_AURAS;
                spellInfo->AttributesEx6 |= SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS;
                break;
            case 70890: // Scourge Strike triggered part
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
                spellInfo->AttributesEx6 &= ~SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS;
                break;
            case 101085: // Wrath of Tarecgosa
            case 98021: // Spirit Link
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
                break;
            case 20711: // Spirit of Redemption
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 77220: // Mastery: Emberstorm
                spellInfo->Effects[4].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[4].ApplyAuraName = SPELL_AURA_ADD_PCT_MODIFIER;
                spellInfo->Effects[4].MiscValue = SPELLMOD_DAMAGE;
                spellInfo->Effects[4].BasePoints = 0;
                spellInfo->Effects[4].SpellClassMask[0] = 0x80;
                spellInfo->Effects[4].SpellClassMask[1] = 0x2000;
                break;
            case 77219: // Mastery: Master Demonologist
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_ADD_FLAT_MODIFIER;
                spellInfo->Effects[0].MiscValue = SPELLMOD_EFFECT2;
                spellInfo->Effects[0].BasePoints = 0;
                spellInfo->Effects[0].SpellClassMask[2] = 0x2000;
                break;
            case 146976: // Glyph of the Unbound Elemental
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA_ON_PET;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_TRANSFORM;
                spellInfo->Effects[0].MiscValue = 73369;
                spellInfo->Effects[1].Effect = SPELL_EFFECT_APPLY_AURA_ON_PET;
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_MOD_SCALE;
                spellInfo->Effects[1].BasePoints = -20;
                break;
            case 57870: // Glyph of Lesser Proportion
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA_ON_PET;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_SCALE;
                spellInfo->Effects[0].BasePoints = -20;
                break;
            case 58640: // Glyph of the Geist
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA_ON_PET;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_TRANSFORM;
                spellInfo->Effects[0].MiscValue = 69220;
                spellInfo->Effects[1].Effect = SPELL_EFFECT_APPLY_AURA_ON_PET;
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_MOD_SCALE;
                spellInfo->Effects[1].BasePoints = 20;
                break;
            case 146652: // Glyph of the Skeleton
                spellInfo->Effects[0].Effect = SPELL_EFFECT_APPLY_AURA_ON_PET;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_TRANSFORM;
                spellInfo->Effects[0].MiscValue = 73290;
                spellInfo->Effects[1].Effect = SPELL_EFFECT_APPLY_AURA_ON_PET;
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_MOD_SCALE;
                spellInfo->Effects[1].BasePoints = -45;
                break;
            case 89485: // Inner Focus
                spellInfo->ProcChance = 100;
                spellInfo->ProcCharges = 1;
                spellInfo->ProcFlags = PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS;
                spellInfo->Effects[1].SpellClassMask = spellInfo->Effects[0].SpellClassMask;
                break;
            case 54785: // Demonic Leap
				spellInfo->Attributes |= SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY;
				spellInfo->Effects[0].Effect = SPELL_EFFECT_LEAP_BACK;
				spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
				spellInfo->Effects[0].BasePoints = 80;
				spellInfo->Effects[0].MiscValue = 275;
				spellInfo->Effects[0].MiscValueB = 0;
                break;
            case 102417: // Wild Charge (Travel Form)
                spellInfo->Effects[0].Effect = SPELL_EFFECT_LEAP_BACK;
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[0].BasePoints = 100;
                spellInfo->Effects[0].MiscValue = 210;
                break;
            case 5176:  // Wrath
            case 2912:  // Starfire
            case 78674: // Starsurge
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_DUMMY;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[EFFECT_1].Mechanic = MECHANIC_NONE;
                break;
                // Silencing Shot
            case 34490:
                spellInfo->Speed = 0.0f;
                break;
            case 114157:
                spellInfo->Speed = 0.0f;
                break;
            case 132626: // Alliance Portal - Mage
                spellInfo->AttributesEx7 |= SPELL_ATTR7_ALLIANCE_ONLY;
                break;
            case 132620: // Horde Portal - Mage
                spellInfo->AttributesEx7 |= SPELL_ATTR7_HORDE_ONLY;
                break;
                // Magic Barrier, Lady Vashj
            case 38112:
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 36819: // Pyroblast (Kael'thas)
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_BE_REFLECTED;
                break;
                // Snowball
            case 25677:
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_DUMMY;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 54402: // Land Mine Knockback
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_DEST_AREA_ALLY;
                spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_DEST_AREA_ALLY;
                break;
                // Improved Frost Presence (Rank 1/2)
            case 50384:
            case 50385:
                spellInfo->Effects[EFFECT_1].SpellClassMask = 0;
                break;
            case 88685: // Holy World: Sanctuary
                spellInfo->AttributesEx5 &= ~SPELL_ATTR5_START_PERIODIC_AT_APPLY;
                break;
            case 47788: // Guardian Spirit
                spellInfo->Effects[2].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[2].ApplyAuraName = SPELL_AURA_SCHOOL_ABSORB;
                break;
                // Detonate Mana, Tyrande's Favorite Doll
            case 92601:
                spellInfo->CasterAuraSpell = 92596;
                break;
                // Shard of the Defiler - Echo of Archimonde
            case 21079:
                spellInfo->Effects[EFFECT_1].Effect = 0;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = 0;
                spellInfo->Effects[EFFECT_2].Effect = 0;
                spellInfo->Effects[EFFECT_2].ApplyAuraName = 0;
                break;
                // Burn Hay, Braizer Torch
            case 88646:
                spellInfo->Effects[EFFECT_1].Effect = 0;
                spellInfo->Effects[EFFECT_1].TargetA = 0;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                break;
                // Cauterizing Flame
            case 99152:
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_TARGET_SELF;
                break;
                // Glyph of Rejuvernation
            case 131618:
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_ADD_FLAT_MODIFIER;
                break;
                // Moonwell Chalice
            case 100403:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
                // Arest, Sturdy Manacles, Seize the Ambassador
            case 78628:
                spellInfo->Effects[EFFECT_0].Effect = 0;
                break;
                // Flamebreaker, Flameseer's Staff, Flamebreaker quest
            case 75206:
                spellInfo->Effects[EFFECT_0].Effect = 0;
                spellInfo->Effects[EFFECT_0].ApplyAuraName = 0;
                spellInfo->Effects[EFFECT_0].TriggerSpell = 0;
                break;
                // Summon Unbound Flamesparks, Flameseer's Staff, Flamebreaker quest
            case 74723:
                spellInfo->Effects[EFFECT_0].MiscValue = 40065;
                break;
                // Inflate Air Balloon, Undersea Inflation quest
            case 75346:
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_KILL_CREDIT;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_0].MiscValue = 40399;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = 0;
                break;
                // Hercular's Rod
            case 89821:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
                // Fortune Cookie
            case 87604:
                spellInfo->Effects[EFFECT_2].Effect = SPELL_EFFECT_SCRIPT_EFFECT;
                break;
                // Heart's Judgment, Heart of Ignacious trinket
            case 91041:
                spellInfo->CasterAuraSpell = 91027;
                break;
                // Heart's Judgment, Heart of Ignacious trinket (heroic)
            case 92328:
                spellInfo->CasterAuraSpell = 92325;
                break;
            case 53182:
            case 53183:
            case 53184:
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_ADD_PCT_MODIFIER;
                break;
                // Glyph of Fear
            case 56244:
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_ADD_FLAT_MODIFIER;
                spellInfo->Effects[EFFECT_0].BasePoints = 5000;
                spellInfo->Effects[EFFECT_0].MiscValue = SPELLMOD_COOLDOWN;
                break;
                // Eclipse markers
            case 67484:
            case 67483:
            case 48517:
            case 48518:
                spellInfo->AuraInterruptFlags = 0;
                spellInfo->Attributes |= SPELL_ATTR0_CANT_CANCEL;
                break;
                // Cheat Death
            case 45182:
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN;
                break;
                //
                // RUBY SANCTUM SPELLS
                //
            case 74769: // Twilight Cutter
            case 77844: // Twilight Cutter
            case 77845: // Twilight Cutter
            case 77846: // Twilight Cutter
            {
                            const SpellRadiusEntry* radius = sSpellRadiusStore.LookupEntry(12);
                            if (!radius)
                                break;

                            spellInfo->Effects[0].RadiusEntry = radius; //200yards.
            }
                break;
            case 75509: // Twilight Mending
                spellInfo->AttributesEx6 |= SPELL_ATTR6_CAN_TARGET_INVISIBLE;
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
            case 75888: // Awaken Flames
            case 75889: // Awaken Flames
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_TARGET_SELF;
                break;
            case 74799: // Soul Consumption
            {
                            const SpellRadiusEntry* radius = sSpellRadiusStore.LookupEntry(32);
                            if (!radius)
                                break;

                            spellInfo->Effects[1].RadiusEntry = radius; //200yards.
            }
                break;
            case 74802: // Consumption (10man normal)
            case 75874: // Consumption (25man normal)
            case 74630: // Combustion (10man normal)
            case 75882: // Combustion (25man normal)
            {
                            const SpellRadiusEntry* radius = sSpellRadiusStore.LookupEntry(22);
                            if (!radius)
                                break;

                            spellInfo->Effects[0].RadiusEntry = radius; //200yards.
            }
                break;
            case 75875: // Consumption (10man heroic)
            case 75876: // Consumption (25man heroic)
            case 75883: // Combustion (10man heroic)
            case 75884: // Combustion (25man heroic)
            {
                            const SpellRadiusEntry* radius = sSpellRadiusStore.LookupEntry(22);
                            if (!radius)
                                break;

                            spellInfo->Effects[0].RadiusEntry = radius; //200yards.
                            spellInfo->Effects[1].RadiusEntry = radius; //200yards.
            }
                break;
                break;
            case 74562: // Fiery Combustion
            case 74792: // Soul Consumption
                spellInfo->AttributesEx |= (SPELL_ATTR1_CANT_BE_REFLECTED | SPELL_ATTR1_CANT_BE_REDIRECTED);
                break;
                // Bad Manner
            case 90337:
                spellInfo->InterruptFlags |= SPELL_INTERRUPT_FLAG_INTERRUPT;
                break;
                // ENDOF RUBY SANCTUM SPELLS
                //
                // GRIM BATOL SPELLS
                //
                // Trash
            case 76517: // Eruption Fire
            case 90693:
                spellInfo->Effects[0].SetRadiusIndex(15);
                break;
            case 76782: // Rock Smash dmg
            case 90862:
                spellInfo->Effects[0].SetRadiusIndex(8);
                break;
            case 76786: // Fissure dmg
            case 90863:
                spellInfo->Effects[0].SetRadiusIndex(8);
                break;
            case 76101: // Lightning Strike dmg
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 76392: // Arcane Slash
            case 90660:
                spellInfo->Effects[0].SetRadiusIndex(15);
                break;
            case 76370: // Warped Twilight
            case 90300:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 76620: // Azure Blast
            case 90697:
                spellInfo->Effects[0].SetRadiusIndex(14);
                break;
            case 76404: // Crimson Charge
                spellInfo->EquippedItemClass = 0;
                spellInfo->EquippedItemSubClassMask = 0;
                break;
            case 76409: // Crimson Shockwave
            case 90312:
                spellInfo->Effects[0].SetRadiusIndex(8);
                spellInfo->Effects[1].SetRadiusIndex(8);
                break;
            case 76327: // Blaze
            case 90307:
                spellInfo->Effects[0].SetRadiusIndex(8);
                break;
            case 76693: // Empowering Twilight dmg
            case 90707:
                spellInfo->Effects[0].SetRadiusIndex(8);
                spellInfo->Effects[1].SetRadiusIndex(8);
                break;
            case 76627: // Mortal Strike
                spellInfo->EquippedItemClass = 0;
                spellInfo->EquippedItemSubClassMask = 0;
                break;
            case 76603: // Earth Spike
            case 90487:
                spellInfo->Effects[0].SetRadiusIndex(15);
                break;
            case 76411: // Meat Grinder
            case 90665:
                spellInfo->EquippedItemClass = 0;
                spellInfo->EquippedItemSubClassMask = 0;
                break;
            case 76413: // Meat Grinder dmg
            case 90664:
                spellInfo->Effects[0].SetRadiusIndex(8);
                break;
            case 76668: // Flame Conduit
            case 90850:
                spellInfo->Effects[0].SetRadiusIndex(29);
                break;
            case 76578: // Chain Lightning
            case 90856:
                spellInfo->Effects[0].SetRadiusIndex(8);
                break;
                // General Umbriss
            case 74675: // Blitz dmg
            case 90251:
                spellInfo->Effects[0].SetRadiusIndex(14);
                spellInfo->Effects[1].SetRadiusIndex(14);
                break;
            case 74837: // Modgud Malady
            case 90179:
                spellInfo->Effects[0].SetRadiusIndex(14);
                spellInfo->Effects[1].SetRadiusIndex(14);
                break;
            case 90170: // Modgud Malice aura
                spellInfo->Effects[0].SetRadiusIndex(14);
                spellInfo->Effects[1].SetRadiusIndex(14);
                spellInfo->Effects[2].Effect = 0;
                break;
                // Forgemaster Throngus
            case 74976: // Disorenting Roar
            case 90737:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 90754: // Lava Patch dmg
                spellInfo->Effects[0].SetRadiusIndex(8);
                break;
            case 74986: // Cave In dmg
            case 90722:
                spellInfo->Effects[0].SetRadiusIndex(8);
                break;
            case 74984: // Mighty Stomp
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
                // Drahga Shadowburner
            case 75238: // Supernova
            case 90972:
                spellInfo->Effects[0].SetRadiusIndex(14);
                spellInfo->Effects[2].SetRadiusIndex(14);
                break;
            case 75245: // Burning Shadowbolt
            case 90915:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 75317: // Seeping Twilight dmg
            case 90964:
                spellInfo->Effects[0].SetRadiusIndex(14);
                spellInfo->Effects[0].SetRadiusIndex(14);
                break;
                // Erudax
            case 75861: // Binding Shadows aura
            case 91079:
                spellInfo->Effects[0].SetRadiusIndex(14);
                spellInfo->Effects[1].SetRadiusIndex(14);
                spellInfo->Effects[2].SetRadiusIndex(14);
                break;
            case 75520: // Twilight Corruption
            case 91049:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 75694: // Shadow Gale Speed
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 75664: // Shadow Gale
            case 91086:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 75692: // Shadow Gale dmg
            case 91087:
                spellInfo->Effects[0].SetRadiusIndex(28);
                spellInfo->Effects[1].SetRadiusIndex(28);
                break;
            case 76194: // Twilight Blast dmg
            case 91042:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 75809: // Shield of Nightmare
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 75763: // Umbral Mending
            case 91040:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
                // ENDOF GRIM BATOL SPELLS
                //
                // THRONE OF THE FOUR WINDS SPELLS
                //
			case 87740: // Slipstream buff
				spellInfo->SetDurationIndex(566); // 8 seconds
				break;
                // Anshal
            case 85425: // Nurture summon trigger
                spellInfo->AttributesEx &= ~SPELL_ATTR1_CHANNELED_2;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(0);
                break;
            case 85428: // Nurture dummy
            case 85429: // Nurture summon
                spellInfo->AttributesEx &= ~SPELL_ATTR1_CHANNELED_2;
                break;
            case 84638: // Zephyr
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_DUMMY;
                break;				
			/*case 86282: // Toxic Spores				
				spellInfo->StackAmount = 25;
				spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
				break;*/
                // Nezir
            /*case 86367: // Sleet Storm dmg
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_SHARE_DAMAGE;
                break;*/
            case 84644: // Sleet Storm
				spellInfo->Effects[EFFECT_0].Effect = 0;
				spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
                // Rohash
			case 93059: // Storm Shield
				spellInfo->Effects[1].Amplitude = 1000;
				break;
            case 84643: // Hurricane
                spellInfo->Effects[EFFECT_2].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 86492: // Hurricane vehicle
                spellInfo->Effects[EFFECT_1].Effect = 0;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = 0;
                break;
				//Alakir
			case 91020: //91020 Ice Storm
				spellInfo->Effects[0].BasePoints = 10000;
				switch (difficulty){
						case RAID_DIFFICULTY_10MAN_HEROIC:
						case RAID_DIFFICULTY_25MAN_HEROIC:
							spellInfo->Effects[0].BasePoints = 30000;
							break;
						}
			    break;
                // THRONE OF THE FOUR WINDS SPELLS
                //
                // BLACKWING DESCENT SPELLS
                //
                // Trash
            case 79604: // Thunderclap
            case 91905:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(13);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(13);
                break;
			case 81031:
				spellInfo->Effects[EFFECT_0].TriggerSpell = 0;
				break;
			case 77827: // Blackwing Descent > Nefarian > Tail Lash
				spellInfo->MaxAffectedTargets = 25;
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_SRC_CASTER;
				spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
				spellInfo->Effects[EFFECT_1].TargetA = TARGET_SRC_CASTER;
				spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
				break;
			case 81007:
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(26);
				spellInfo->Effects[EFFECT_1].SetRadiusIndex(26);
				break;
			case 66320:
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
				spellInfo->Effects[EFFECT_1].SetRadiusIndex(8);				
				break;
			case 79396:
				spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_PERIODIC_TRIGGER_SPELL;
				spellInfo->Effects[EFFECT_0].TriggerSpell = 0;
				break;
			case 66318:
				spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_PERIODIC_TRIGGER_SPELL;
				break;
			case 77944:
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_SRC_CASTER;
				spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;					
				break;
            case 80035: // Vengeful Rage
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(12);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(12);
                spellInfo->Effects[EFFECT_2].SetRadiusIndex(12);
                break;
            case 79589: // Constricting Chains
            case 91911:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(27);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(27);
                break;
            case 80336: // Frost Burn
            case 91896:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(9);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(9);
                spellInfo->Effects[EFFECT_2].SetRadiusIndex(9);
                break;
            case 80878: // Bestowal of Angerforge
            case 80871: // Bestowal of Thaurissan
            case 80875: // Bestowal of Ironstar
            case 80872: // Bestowal of Burningeye
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(12);
                break;
            case 80638: // Stormbolt
            case 91890:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(8);
                break;
            case 80646: // Chain Lightning
            case 91891:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
                break;
            case 91849: // Grip of Death
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(12);
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 92048: // Shadow Infusion
                spellInfo->TargetAuraSpell = 0;
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                break;
            case 92051: // Shadow Conductor
            case 92135:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                break;
            case 92023: // Encasing Shadows
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                spellInfo->TargetAuraSpell = 0;
                break;
            case 92153: // Blazing Inferno missile
                spellInfo->Speed = 6.0f;
                break;
            case 92154: // Blazing Inferno dmg
            case 92190:
            case 92191:
            case 92192:
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(8);
                break;
            case 92173: // Shadow Breath
            case 92193:
            case 92194:
            case 92195:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
                break;
                // Magmaw
            case 89773: // Mangle
            case 91912:
            case 94616:
            case 94617:
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
			case 78359: // Magma Split 1
            case 91916:
            case 91925:
            case 91926:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 78068: // Magma Split 2
            case 91917:
            case 91927:
            case 91928:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
                // Omnotron Defence System
            case 78697: // Recharging blue
            case 95019:
            case 95020:
            case 95021:
            case 78698: // Recharging orange
            case 95025:
            case 95026:
            case 95027:
            case 78699: // Recharging purple
            case 95028:
            case 95029:
            case 95030:
            case 78700: // Recharging green
            case 95022:
            case 95023:
            case 95024:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 91540: // Arcane Annihilator
            case 91542:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                spellInfo->MaxAffectedTargets = 3;
                break;
            case 79629: // Power Generator aoe
            case 91555:
            case 91556:
            case 91557:
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(8);
				spellInfo->Effects[EFFECT_1].TargetA = TARGET_SRC_CASTER;
				spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_SRC_AREA_ENTRY;
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_SRC_CASTER;
				spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENTRY;
                break;
            case 91858: // Overcharged Power Generator aoe
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(8);
                break;
            case 91879: // Arcane Blowback
            case 91880:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(13);
                break;
            case 79912: // Static Shock dmg
            case 91456:
            case 91457:
            case 91458:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
                break;
            case 79035: // Inseneration Security Missure
            case 91523:
            case 91524:
            case 91525:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                break;
            case 79501: // Acquiring Target
            case 92035:
            case 92036:
            case 92037:
                spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_STUNNED;
                break;
            case 79504: // Flamethrower
            case 91535:
            case 91536:
            case 91537:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CONE_ENEMY_24;
                spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_STUNNED;
                break;
            case 79617: // Backdraft
            case 91528:
            case 91529:
            case 91530:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                break;
            case 80092: // Poison Bomb
            case 91498:
            case 91499:
            case 91500:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(13);
                break;
            case 80164: // Chemical Cloud aoe a
            case 91478:
            case 91479:
            case 91480:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(18);
                break;
            case 80161: // Chemocal Cloud aoe b
            case 91471:
            case 91472:
            case 91473:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(18);
                break;
            case 80097: // Poison Pubble aoe
            case 91488:
            case 91489:
            case 91490:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(13);
                break;
                // Maloriak
            case 89798: // Master Adventurer Award
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                break;
            case 78194: // Magma Jets summon
                spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_STUNNED;
                break;
            case 77569: // Release Aberrations
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_GAMEOBJECT_SRC_AREA;
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                break;
            case 77991: // Release All
                spellInfo->InterruptFlags &= ~SPELL_INTERRUPT_FLAG_INTERRUPT;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_GAMEOBJECT_SRC_AREA;
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 77699: // Flash Freeze dmg
            case 92978:
            case 92979:
            case 92980:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(8);
                break;
            case 77763: // Biting Chill dmg
            case 92975:
            case 92976:
            case 92977:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(15);
                break;
            case 77615: // Debilitating Slime
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                spellInfo->Effects[EFFECT_2].SetRadiusIndex(28);
                break;
            case 77948: // Debilitating Slime knock back
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(10);
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                spellInfo->Effects[EFFECT_2].SetRadiusIndex(10);
                break;
            case 77908: // Arcane Storm dmg
            case 92961:
            case 92962:
            case 92963:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(31);
                break;
            case 78095: // Magma Jets dmg
            case 93014:
            case 93015:
            case 93016:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(15);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(15);
                break;
            case 78225: // Acid Nova
            case 93011:
            case 93012:
            case 93013:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                break;
            case 77987: // Grown Catalyst
            case 101440:
            case 101441:
            case 101442:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(13);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(13);
                spellInfo->Effects[EFFECT_2].SetRadiusIndex(13);
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 78208: // Absolute Zero
            case 93041:
            case 93042:
            case 93043:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(8);
                break;
            case 77715: // Shatter
            case 95655:
            case 95656:
            case 95657:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(8);
                break;
            case 77925: // Throw Red Bottle
            case 77928: // Throw Red Bottle
            case 77932: // Throw Blue Bottle
            case 77934: // Throw Blue Bottle
            case 77937: // Throw Green Bottle
            case 77938: // Throw Green Bottle
            case 92831: // Throw Black Bottle
            case 92837: // Throw Black Bottle
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_GAMEOBJECT_TARGET;
                break;
                // Chimaeron
            case 82848: // Massacre
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                spellInfo->InterruptFlags &= ~SPELL_INTERRUPT_FLAG_INTERRUPT;
                break;
            case 88872: // Feud
                spellInfo->InterruptFlags &= ~SPELL_INTERRUPT_FLAG_INTERRUPT;
                break;
            case 82881: // Break
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE;
                break;
            case 91304: // Shadow Whip
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
                // Atramedes
            case 77611: // Resonating Clash interrupt
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(28);
                break;
            case 77709: // Resonating Clash energize
                spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_SRC_AREA_ALLY;
                spellInfo->Effects[EFFECT_2].TargetB = TARGET_UNIT_SRC_AREA_ALLY;
                break;
            case 77782: // Sonic Flames dmg
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 77672: // Sinar Pulse target
            case 92411:
            case 92412:
            case 92413:
                spellInfo->MaxAffectedTargets = 3;
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                break;
            case 77612: // Modulation
            case 92451:
            case 92452:
            case 92453:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                break;
            case 77675: // Sonar Pulse dmg
            case 92417:
            case 92418:
            case 92419:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
                break;
            case 77982: // Searing Flame dmg
            case 92421:
            case 92422:
            case 92423:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(28);
                break;
            case 78115: // Sonar Fireball
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                break;
            case 92553: // Sonar Bomb dmg
            case 92554:
            case 92555:
            case 92556:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(8);
                break;
            case 77966: // Searing Flame Missile
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(23);
                break;
            case 78353: // Roaring Flame dmg
            case 92445:
            case 92446:
            case 92447:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(8);
                break;
            case 78023: // Roaring Flame aura dmg
            case 92483:
            case 92484:
            case 92485:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(8);
                spellInfo->Effects[EFFECT_2].SetRadiusIndex(8);
                break;
            case 78875: // Devastation
				spellInfo->TargetAuraSpell = 0;
				spellInfo->Effects[EFFECT_0].TriggerSpell = 0;	
				spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_PERIODIC_DAMAGE;				
 				switch (difficulty){
					case RAID_DIFFICULTY_10MAN_HEROIC:
					case RAID_DIFFICULTY_25MAN_HEROIC:
						spellInfo->Effects[EFFECT_0].BasePoints = irand(204750, 215250);
						break;
					default:
						spellInfo->Effects[EFFECT_0].BasePoints = irand(170625, 179375);
						break;
				}
                break;
            case 78868: // Devastation dmg
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);				
                break;
			case 78100:
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
				spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_SCHOOL_DAMAGE;
				switch (difficulty){
				case RAID_DIFFICULTY_10MAN_HEROIC:
					spellInfo->Effects[EFFECT_0].BasePoints = irand(29250, 30750);
					break;
				case RAID_DIFFICULTY_25MAN_HEROIC:
					spellInfo->Effects[EFFECT_0].BasePoints = irand(29250, 30750);
					break;
				default:
					spellInfo->Effects[EFFECT_0].BasePoints = irand(19500, 20500);
					break;
				}
				spellInfo->Effects[EFFECT_1].SetRadiusIndex(28);
				break;
			case 82890:
				spellInfo->AttributesEx3 = 0;
				break;
                // ENDOF BLACKWING DESCENT SPELLS
                //
                // FIRELANDS SPELLS spellInfo->AttributesEx3 &= ~SPELL_ATTR3_DEATH_PERSISTENT
                //
                // Trash
			
            case 101093:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 99692: // Terrifying Roar
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(33);
                break;
            case 97552: // Lava Shower dmg
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(26);
                break;
            case 99993: // Fiery Blood
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(18);
                spellInfo->Effects[EFFECT_2].SetRadiusIndex(18);
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 100273: // Shell Spin dmg
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(8);
                break;
            case 100799: // Fire Torment dmg
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(12);
                break;
            case 99530: // Flame Stomp
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(13);
                break;
            case 99555: // Summon Lava Jest
                spellInfo->MaxAffectedTargets = 4;
                break;
                // Shannox
            case 100002: // Hurl Spear dmg
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(15);
                break;
            case 100031: // Hurl Spear aoe
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 100495: // Magma Flare
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(27);
                break;
            case 99945: // Face Rage jump
                spellInfo->Effects[EFFECT_0].TriggerSpell = 0;
                spellInfo->Attributes |= SPELL_ATTR0_IMPOSSIBLE_DODGE_PARRY_BLOCK;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 99947: // Face Rage aura
                spellInfo->Attributes |= SPELL_ATTR0_IMPOSSIBLE_DODGE_PARRY_BLOCK;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 100415: // Rage
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 99937: // Jagged Tear
            case 101218:
            case 101219:
            case 101220:
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
                // Bethtilac
            case 99052: // Smoldering Devastation
                spellInfo->InterruptFlags = 0;
                break;
            case 98471: // Burning Acid
            case 100826:
            case 100827:
            case 100828:
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 97202: // Fiery Web Spin
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->ChannelInterruptFlags |= CHANNEL_INTERRUPT_FLAG_INTERRUPT;
                break;
            case 98997: // Clear Spiderweb Filament
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 99304: // Consume (drone)
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 99476: // The Widow Kiss
                spellInfo->Effects[EFFECT_0].TriggerSpell = 99506;
                break;
            case 99526: // Fixate
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                break;
            case 99999: // Fixate (broodling)
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                break;
                // Alysrazor
            case 102111: // Cataclysm
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_SCRIPT_EFFECT;
                break;
            case 99208: // Ritual of the Flame dmg
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 99558: // Meteoric Impact
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 99274: // Molten Meteor dmg
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 99844: // Blazing Claw dmg
            case 101729:
            case 101730:
            case 101731:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CONE_ENEMY_104;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_CONE_ENEMY_104;
                break;
            case 99794: // Fiery Vortex dmg
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 100640: // Harsh Winds
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 99816: // Fiery Tornado dmg
            case 100733:
            case 100734:
            case 100735:
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 99747: // Blazing Buffet dmg
            case 100739:
            case 100740:
            case 100741:
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 99461: // Blazing Power dmg
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 99427: // Incindiary Cloud dmg
            case 100729:
            case 100730:
            case 100731:
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 98885: // Brashfire
            case 100715:
            case 100716:
            case 100717:
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 99919: // Ignition
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 99529: // Incindiary Cloud sum
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
                break;
            case 99528: // Blazing Power sum
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
                break;
            case 99605: // Firestorm
            case 101658:
            case 101659:
            case 101660:
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                spellInfo->Effects[EFFECT_2].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 99606: // Firestorm dot
            case 101661:
            case 101662:
            case 101663:
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 101410: // Molten Feather bar
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 100744: // Firestorm heroic
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 100745: // Firestorm heroic dmg
            case 101664:
            case 101665:
            case 101666:
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
                // Lord Rhyolith
            case 98843: // Ride Vehicle
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 98010: // Volcanic Birth
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                break;
            case 98493: // Heated Volcano
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
            case 98255: // Molten Armor
            case 101157:
            case 101158:
            case 101159:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = 0;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                spellInfo->Effects[EFFECT_2].Effect = 0;
                spellInfo->Effects[EFFECT_2].ApplyAuraName = 0;
                spellInfo->Effects[EFFECT_2].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_2].TargetB = 0;
                break;
            case 97230: // Magma Flow area
                spellInfo->SetDurationIndex(1); // 10 seconds
                break;
            case 99875: // Fuse
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_2].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_2].TargetB = 0;
                break;
            case 98135: // Summon Fragment of Rhyolith
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                break;
            case 98553: // Summon Spark of Rhyolith
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                break;
            case 98136: // Summon Fragment of Rhyolith sum
            case 100392:
            case 98552:
                spellInfo->SetDurationIndex(21);
                break;
            case 98649: // Meltdown dmg
            case 101646:
            case 101647:
            case 101648:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                break;
                // Baleroc
            case 99351: // Inferno Blade
                spellInfo->Attributes &= ~SPELL_ATTR0_ON_NEXT_SWING;
                break;
            case 99262: // Vital Spark
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
            case 99515: // Final Countdown
                spellInfo->MaxAffectedTargets = 2;
                break;
            case 99254: // Torment visual
                spellInfo->Effects[EFFECT_0].TriggerSpell = 0;
                break;
            case 99256: // Torment
            case 100230:
            case 100231:
            case 100232:
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
                // Majordomo Staghelm
            case 98565: // Burning Orbs summon
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
            case 98229: // Concentration
                spellInfo->Effects[EFFECT_2].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
                // Ragnaros
            case 98159: // Magma Trap aoe
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
            case 98710: // Sulfuras Smash
            case 100890:
            case 100891:
            case 100892:
                spellInfo->AttributesEx3 &= ~SPELL_ATTR5_DONT_TURN_DURING_CAST;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 98708: // Sulfuras Smash
            case 100256:
            case 100257:
            case 100258:
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                spellInfo->Effects[EFFECT_2].Effect = 0;
                break;
            case 101088: // Lavalogged
                spellInfo->ExcludeTargetAuraSpell = 101008;
                break;
            case 101102: // Lavalogged
                spellInfo->ExcludeTargetAuraSpell = 101102;
                break;
                // Splitting Blow
            case 98951: case 100883: case 100884: case 100885:
            case 98952: case 100877: case 100878: case 100879:
            case 98953: case 100880: case 100881: case 100882:
                spellInfo->AttributesEx3 &= ~SPELL_ATTR5_DONT_TURN_DURING_CAST;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 99012: // Splitting Blow script
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_CASTER;
                break;
            case 98497: // Molten Seed aoe 2
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
            case 98498: // Molten Seed dmg
            case 100579:
            case 100580:
            case 100581:
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_DUMMY;
                break;
            case 100158: // Molten Power
            case 100302:
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 99125: // Blazing Heat dummy
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
            case 99129: // Blazing Heat summon
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
            case 99267: // Living Meteor aoe 1
            case 101387:
            case 101388:
            case 101389:
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
            case 100171: // World in Flame
            case 100190:
                spellInfo->SetDurationIndex(566); // 8 seconds
                break;
            case 98981: // Lava Bolt
            case 100290:
                spellInfo->MaxAffectedTargets = 4;
                break;
            case 61882:
            case 100289:
            case 100291:
            case 114923:
                spellInfo->MaxAffectedTargets = 10;
                break;
            case 100476: // Breadth of Frost summon
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->SetDurationIndex(23);
                break;
            case 100567: // Breadth of Frost dmg
                spellInfo->ExcludeTargetAuraSpell = 100567;
                break;
            case 100679: // Dreadflame summon
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                break;
            case 100714: // Cloudburst missile
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                break;
            case 100644: // Entrapping Roots summon
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->SetDurationIndex(23);
                break;
            case 100653: // Entrapping Roots dmg
                spellInfo->ExcludeTargetAuraSpell = 100653;
                break;
            case 101237: // Entrapping Roots dmg
                spellInfo->ExcludeTargetAuraSpell = 101237;
                break;
            case 100777: // Magma Geyser
            case 100822:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                break;
            case 100941: // Dreadflame dmg
            case 100998:
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE;
                break;
                // ENDOF FIRELANDS
                //
                // BASTION OF TWILIGHT SPELLS
                //
                // Trash
            case 85647: // Mind Sear dmg
                spellInfo->Effects[2].SetRadiusIndex(13);
                break;
            case 85697:
                spellInfo->Effects[2].SetRadiusIndex(29);
                break;
            case 83710: // Furious Roar
            case 86169:
            case 86170:
            case 86171:
                spellInfo->Effects[0].SetRadiusIndex(28);
                spellInfo->Effects[1].SetRadiusIndex(28);
                spellInfo->Effects[2].SetRadiusIndex(28);
                break;
            case 83719: // Fireball Barrage aoe
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 83855: // Scorching Breath dmg
            case 86163:
            case 86164:
            case 86165:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 83734: // Fireball Barrage dmg0
            case 86154:
            case 86155:
            case 86156:
            case 83721: // Fireball Barrage dmg1
            case 86151:
            case 86152:
            case 86153:
                spellInfo->Effects[0].SetRadiusIndex(26);
                break;
				//
				// - Theralion & Valiona
				//
			case 86380: // Dazzling Destruction Summon Missile
				spellInfo->Effects[0].SetRadiusIndex(28);
				break;
			case 86406: // Dazzling Destruction dmg
				spellInfo->Effects[0].SetRadiusIndex(32);
				spellInfo->Effects[1].SetRadiusIndex(32);
				spellInfo->Effects[0].TargetB = 8;
				break;
			case 86505: // Fabolous Flames dmg
				spellInfo->Effects[0].SetRadiusIndex(32);
				break;
			case 86607: // Engulfing Magic
				spellInfo->Effects[0].SetRadiusIndex(28);
				break;
			case 86622: // Engulfing Magic Aur
				spellInfo->Effects[EFFECT_2].Effect = SPELL_EFFECT_APPLY_AURA;
				spellInfo->Effects[EFFECT_2].BasePoints = 100;
				spellInfo->Effects[EFFECT_2].TargetA = TARGET_UNIT_CASTER;
				spellInfo->Effects[EFFECT_2].TargetB = 0;
				spellInfo->Effects[EFFECT_2].ApplyAuraName = SPELL_AURA_DUMMY;
				spellInfo->Effects[EFFECT_2].ValueMultiplier = 100;
				spellInfo->Effects[EFFECT_2].MiscValue = 0;
				spellInfo->Effects[EFFECT_2].MiscValueB = 0;
				spellInfo->Effects[EFFECT_2].Amplitude = 0;
				break;
            case 86631: // Engulfing Magic Dps				
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
                break;
            case 86825: // Blackout dmg
                spellInfo->Effects[0].SetRadiusIndex(14);
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_SHARE_DAMAGE;
                spellInfo->SchoolMask = SPELL_SCHOOL_MASK_SHADOW;
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
                break;
			case 86014: // Twilight Meteorite dmg
				spellInfo->Effects[0].SetRadiusIndex(29);
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_SHARE_DAMAGE;
				break;
			case 88518: // Twilight Meteorite Mark
				spellInfo->Effects[0].SetRadiusIndex(28);
				break;
			case 86199: // Twilight Flame dmg1
				spellInfo->Effects[0].SetRadiusIndex(14);
				spellInfo->Effects[1].Effect = 0;
				spellInfo->Effects[2].SetRadiusIndex(14);
				break;
			case 86228: // Twilight Flame dmg2
				spellInfo->TargetAuraSpell = 0;
				spellInfo->ExcludeTargetAuraSpell = 0;
				spellInfo->Effects[0].SetRadiusIndex(14);
				break;
			case 86305: // Unstable Twilight dmg
				spellInfo->Effects[0].SetRadiusIndex(14);
				break;
			case 86210: // Twilight Zone
				spellInfo->Effects[0].Effect = 0;
				break;
			case 93019: // Rift Blast
			case 93020:
				spellInfo->MaxAffectedTargets = 1;
				spellInfo->Effects[0].SetRadiusIndex(28);
				break;
                // Cho'gall
            case 81571: // Unleashed Shadows dmg
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 81538: // Blaze dmg
                spellInfo->Effects[0].SetRadiusIndex(8);
                break;
            case 81713: // Depravity
                spellInfo->Effects[0].SetRadiusIndex(9);
                break;
            case 81761: // Spilled Blood of the Old God dmg
                spellInfo->Effects[0].SetRadiusIndex(13);
                spellInfo->Effects[1].SetRadiusIndex(13);
                break;
            case 82919: // Sprayed Corruption
                spellInfo->Effects[0].SetRadiusIndex(9);
                spellInfo->Effects[1].SetRadiusIndex(9);
                break;
            case 82299: // Fester Blood
                spellInfo->Effects[0].TargetA = 1;
                spellInfo->Effects[0].TargetB = 0;
                spellInfo->Effects[0].Effect = 3;
                spellInfo->Effects[1].Effect = 0;
                break;
            case 82361: // Corruption of the Old God
                spellInfo->Effects[1].Effect = 0;
                break;
            case 82363: // Corruption of the Old God dmg
                spellInfo->Effects[0].SetRadiusIndex(28);
                spellInfo->Effects[1].SetRadiusIndex(28);
                break;
            case 82151: // Shadow Bolt
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 82630: // Consume Blood of the Old God
                spellInfo->Effects[0].TargetA = 0;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_SRC_AREA_ALLY;
                spellInfo->Effects[1].TargetA = 0;
                spellInfo->Effects[1].TargetB = TARGET_UNIT_SRC_AREA_ALLY;
                break;
            case 91331: // Twisted Devotion
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 93103: // Corrupted Blood aura
                spellInfo->Effects[0].SetRadiusIndex(28);
                spellInfo->ExcludeTargetAuraSpell = 93103;
                break;
                // Sinestra
            case 95822: // Twilight Flame dmg
                spellInfo->Effects[0].SetRadiusIndex(13);
                break;
            case 95855: // Call of Flames
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 89435: // Wrack aoe
            case 92956:
                spellInfo->Effects[0].SetRadiusIndex(28);
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 89421: // Wrack dmg
            case 92955:
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                spellInfo->AttributesEx6 |= SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS;
                break;
            case 90028: // Unleash Essence
            case 92947:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 90045: // Indomitable dmg
            case 92946:
                spellInfo->Effects[0].SetRadiusIndex(14);
                spellInfo->Effects[1].SetRadiusIndex(14);
                break;
            case 87655: // Twilight Infusion dummy
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 87231: // Fiery Barrier aura
                spellInfo->Effects[0].SetRadiusIndex(10);
                break;
            case 88146: // Twilight Essence dmg
                spellInfo->Effects[0].SetRadiusIndex(22);
                spellInfo->Effects[1].SetRadiusIndex(22);
                break;
            case 92950: // Twilight Essence dmg
                spellInfo->Effects[0].SetRadiusIndex(14);
                spellInfo->Effects[1].SetRadiusIndex(14);
                break;
            case 92852: // Twilight Slicer dmg
            case 92954:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 95564: // Twilight Infusion missile
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 89299: // Twilight Spit dmg
            case 92953:
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 92958: // Twilight Pulse dmg
            case 92959:
                spellInfo->Effects[0].SetRadiusIndex(8);
                break;
                // Council of Ascendents
            case 82746: // Glaciate
            case 92506:
            case 92507:
            case 92508:
                spellInfo->Effects[0].SetRadiusIndex(28);
                spellInfo->Effects[1].SetRadiusIndex(28);
                break;
			case 84914:				
				spellInfo->Effects[0].TriggerSpell = 0;
				break;
			case 84915:// Liquid Ice dmg
				spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ENEMY;
				spellInfo->Effects[0].TargetB = 0;
				break;
            case 92548: // Glaciate 2
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 82699: // Water Bomb
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 82762: // Waterlogged
                spellInfo->Effects[0].SetRadiusIndex(29);
                break;
            case 77347: // Water Bomb Summon
                spellInfo->Effects[0].SetRadiusIndex(12);
                spellInfo->Effects[1].SetRadiusIndex(12);
                break;
            case 82666: // Frost Imbued
                spellInfo->Effects[0].SetRadiusIndex(13);
                break;
            case 82859: // Inferno Rush
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 82856: // Inferno Jump
                spellInfo->Effects[1].Effect = 0;
                break;
            case 82663: // Flame Imbued
                spellInfo->Effects[0].SetRadiusIndex(13);
                break;
            case 83067: // Thundershock
            case 92469:
            case 92470:
            case 92471:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 83099: // Lightning Rod
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 83300: // Chain Lightning dummy
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 83282: // Chain Lightning
            case 92448:
            case 92449:
            case 92450:
                spellInfo->Effects[0].SetRadiusIndex(8);
                break;
            case 83070: // Lightning Blast
            case 92454:
            case 92455:
            case 92456:
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[0].TargetB = 0;
                spellInfo->Effects[1].Effect = 0;
                break;
            case 83078: // Disperse 1
                spellInfo->Effects[0].Effect = 0;
                break;
            case 83565: // Quake
            case 92544:
            case 92545:
            case 92546:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;            
            case 92497:
            case 92498:
            case 92499:
                spellInfo->Effects[0].SetRadiusIndex(8);
                spellInfo->Effects[1].SetRadiusIndex(8);
                break;
            case 84948: // Gravity Crush
				spellInfo->MaxAffectedTargets = 1;
				break;
            case 92486:
            case 92487:
            case 92488:
                spellInfo->Effects[0].SetRadiusIndex(28);
                spellInfo->Effects[1].SetRadiusIndex(28);
                spellInfo->Effects[2].SetRadiusIndex(28);
                break;
                // ENDOF BASTION OF TWILIGHT SPELLS
                //
                // THE STONECORE SPELLS
                //
                // Corborus
            case 82415: // Dumpening Wave
            case 92650:
                spellInfo->Effects[0].SetRadiusIndex(28);
                spellInfo->Effects[1].SetRadiusIndex(28);
                break;
            case 92122: // Crystal Shard dmg
                spellInfo->Effects[1].SetRadiusIndex(8);
                break;
                // Slabhide
            case 80800: // Eruption
            case 92657:
                spellInfo->Effects[0].SetRadiusIndex(8);
                break;
            case 80801: // Eruption aura
                spellInfo->Effects[0].SetRadiusIndex(8);
                break;
                // Ozruk
            case 78807: // Shatter
            case 92662:
                spellInfo->Effects[0].SetRadiusIndex(18);
                spellInfo->Effects[1].SetRadiusIndex(18);
                break;
            case 92426: // Paralyze
                spellInfo->Effects[0].SetRadiusIndex(28);
                spellInfo->Effects[1].SetRadiusIndex(28);
                break;
                // High Priestess Azil
            case 79251: // Gratity Well aura
                spellInfo->Effects[0].SetRadiusIndex(13);
                break;
            case 79249: // Gravity Well aura dmg
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                spellInfo->Effects[EFFECT_0].BasePoints = 3000;
                break;
            case 79050: // Arcane Shield dmg
            case 92667:
                spellInfo->Effects[EFFECT_2].SetRadiusIndex(8);
                break;
                // ENDOF STONECORE SPELLS
                //
				// HALLS OF ORIGINATION
				// Trash
			case 77262: // Raging Inferno dmg
			case 91159:
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(13);
				spellInfo->Effects[EFFECT_1].SetRadiusIndex(13);
				break;
			case 77273: // Lava Eruption
			case 91161:
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
				break;
				// Temple Guardian Anhuur
			case 75515: // Searing Flame
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
				break;
			case 75323: // Reverberating Hymn dps
				// Aura is refreshed at 3 seconds, and the tick should happen at the fourth.
				spellInfo->AttributesEx8 |= SPELL_ATTR8_DONT_RESET_PERIODIC_TIMER;
				break;
				// Earthrager Ptah
			case 75540: // Flame Bolt dmg
				spellInfo->MaxAffectedTargets = 2;
				break;
			case 89881: // Flame Bolt dmg H
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
				break;
			case 83650: // Raging Smash
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
				break;
			case 75548: // Quicksand
				spellInfo->Effects[EFFECT_1].Effect = 0;
				break;
			case 89648: // Quicksand
				spellInfo->Effects[EFFECT_0].Effect = 0;
				break;
			case 75546: // Quicksand
				spellInfo->Effects[EFFECT_0].Effect = 0;
				break;
			case 75547: // Quicksand AOE
				spellInfo->Effects[EFFECT_0].Effect = 0;
				break;
			case 89880: // Quicksand AOE H
				spellInfo->Effects[EFFECT_1].Effect = 0;
				break;
			case 75339: // Earthspike dmg
				spellInfo->Effects[EFFECT_2].Effect = 0;
				break;
			case 89882: // Earthspike dmg H
				spellInfo->Effects[EFFECT_1].Effect = 0;
				break;
			case 89398: // Earthspike dmg Kill
				break;
				/*case 89397: // Ride Camel
				spellInfo->Attributes |= 0x29800180;
				spellInfo->AttributesEx |= 0x00000420;
				spellInfo->AttributesEx2 |= 0x10484005;
				spellInfo->AttributesEx3 |= 0x10130200;
				spellInfo->AttributesEx4 |= 0x00800080;
				spellInfo->AttributesEx5 |= 0x00060008;
				spellInfo->AttributesEx6 |= 0x00001200;
				break;*/
				// Anraphet
			case 76904: // Alpha Beams aoe
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(30);
				spellInfo->MaxAffectedTargets = 1;
				break;
			case 75609: // Crumbling Ruin
			case 91206:
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
				break;
				// Isiset
			case 74374: // Arcane Barrage
			case 89886:
				spellInfo->MaxAffectedTargets = 1;
				break;
				// Setesh
			case 76146: // Shadow Bolt Volley
			case 89846:
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
				break;
			case 76674: // Setesh - Chaos Blast
				spellInfo->ProcChance = 100;
				spellInfo->Effects[EFFECT_0].Effect = 0;
				spellInfo->MaxAffectedTargets = 1;
				break;
			case 76675: // Setesh - Chaos Blast_1
				spellInfo->Effects[EFFECT_0].Effect = 0;
				break;
			case 76676: // Setesh - Chaos Blast Misile
				spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
				spellInfo->Effects[EFFECT_0].TargetA = 0;
				break;
			case 76681: // Setesh Chaos Blast Aura Radius
				spellInfo->Effects[EFFECT_0].Effect = 0;
				spellInfo->ProcChance = 100;
				spellInfo->Effects[EFFECT_1].Effect = 0;
				break;
			case 89875:
				spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(32);
				break;
			case 76684: // Chaos Burn Triggger
				spellInfo->Effects[EFFECT_0].Effect = 0;
				break;
			case 77128: // Setesh Chaos Blast Aura Radius
				break;
			case 89874:
				spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(13);
				break;
			case 77023: //Reing of Chaos
				// Ammunae
			case 75657: // Ammunaes Buff should only target himself
				spellInfo->Effects[EFFECT_1].Effect = 0;
				break;
			case 75702: // Noxious Spore
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_SRC_AREA_ENEMY;
				spellInfo->Effects[EFFECT_0].TargetB = 0;
				spellInfo->Effects[EFFECT_1].TargetA = 0;
				spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_SRC_AREA_ALLY;
				break;
			case 89124: // Ammunae Heal
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_NEARBY_ENTRY;
				spellInfo->Effects[EFFECT_0].TargetB = 0;
				break;
			case 89889:
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(13);
				spellInfo->Effects[EFFECT_1].Effect = 0;
				break;
			case 75790: // Rampant Growth
			case 89888:
				spellInfo->Effects[EFFECT_1].SetRadiusIndex(28);
				break;
				// Rajh
			case 73872: // Sun Strike
			case 89887:
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
				break;
			case 73874: // Sun Strike dmg
			case 90009:
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ENEMY;
				spellInfo->Effects[EFFECT_0].TargetA = 0;
				break;
			case 76355: // Blessing of the Sun aura
			case 89879:
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
				spellInfo->Effects[EFFECT_2].SetRadiusIndex(28);
				break;
			case 74108: // Solar Winds dmg
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(26);
				spellInfo->Effects[EFFECT_1].SetRadiusIndex(26);
				break;
			case 89130: // Solar Winds dmg H
				spellInfo->Effects[EFFECT_0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_4_YARDS);
				spellInfo->Effects[EFFECT_1].RadiusEntry = sSpellRadiusStore.LookupEntry(26);
				//spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_KNOCK_BACK_DEST;
				break;
			case 89133: // Solar Fire dmg
			case 89878:
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(26);
				break;
			case 87653: // Inferno Leap
				spellInfo->Effects[0].Effect = SPELL_EFFECT_JUMP;
				spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_NONE;
				break;
				// ENDOF HALLS OF ORIGINATION SPELLS
                // LOST CITY OF THE TOL'VIR
            case 83644: // Mystic Trap N
                spellInfo->MaxAffectedTargets = 3;
                break;
            case 91252: // Mystic Trap H
                spellInfo->MaxAffectedTargets = 5;
                break;
            case 83112: // Land Mine Player Search Effect
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(7);
                spellInfo->AttributesEx3 = SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
            case 91263: // Detonate Traps
                spellInfo->Attributes |= SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY;
                break;
            case 84799: // Paralytic Blow Dart N
            case 89989: // Paralytic Blow Dart H
                spellInfo->Attributes = 0;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[EFFECT_2].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                spellInfo->Effects[EFFECT_2].TargetB = 0;
                spellInfo->Targets = TARGET_FLAG_UNIT;
                break;
            case 83131: // Summon Shockwave Target N
            case 83132: // Summon Shockwave Target S
            case 83133: // Summon Shockwave Target W
            case 83134: // Summon Shockwave Target E
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(15);
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Targets = TARGET_FLAG_DEST_LOCATION;
                break;
            case 83454: // Shockwave N
            case 90029: // Shockwave H
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(15);
                break;
            case 81690: // Scent of Blood N
            case 89998: // Scent of Blood H
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 82263: // Merged Souls
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_2].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                spellInfo->Effects[EFFECT_2].TargetB = 0;
                spellInfo->Targets = TARGET_FLAG_UNIT;
                break;
            case 82430: // Repentance
                spellInfo->Effects[EFFECT_0].MiscValue = 250;
                break;
            case 91196: // Blaze of the Heavens
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(7);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(7);
                break;
            case 88814: // Hallowed Ground N
            case 90010: // Hallowed Ground H
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(32);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(32);
                break;
            case 81942: // Heaven's Fury N
            case 90040: // Heaven's Fury H
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(15);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(15);
                break;
            case 114083:
            case 48721:
                spellInfo->MaxAffectedTargets = 5;
                break;
            case 82622: // Plague of Ages N
            case 89997: // Plague of Ages H
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->AttributesEx = SPELL_ATTR1_CANT_TARGET_SELF;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS | SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 82637: // Plague of Ages N
                spellInfo->ExcludeTargetAuraSpell = 82622;
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->AttributesEx = SPELL_ATTR1_CANT_TARGET_SELF;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS | SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 89996: // Plague of Ages H
                spellInfo->ExcludeTargetAuraSpell = 89997;
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->AttributesEx = SPELL_ATTR1_CANT_TARGET_SELF;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS | SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 82640: // Plague of Ages N
                spellInfo->ExcludeTargetAuraSpell = 82622;
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->AttributesEx = SPELL_ATTR1_CANT_TARGET_SELF;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS | SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 89995: // Plague of Ages H
                spellInfo->ExcludeTargetAuraSpell = 89997;
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->AttributesEx = SPELL_ATTR1_CANT_TARGET_SELF;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS | SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 82139: // Repentance
                spellInfo->AttributesEx3 &= ~SPELL_ATTR3_DEATH_PERSISTENT;
                break;
            case 82425: // Wail of Darkness N
            case 90039: // Wail of Darkness H
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(32);
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 84546: // Static Shock
            case 84555: // Static Shock
            case 84556: // Static Shock
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Targets = TARGET_FLAG_DEST_LOCATION;
                break;
            case 84956: // Call of Sky
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                spellInfo->Targets = TARGET_FLAG_DEST_LOCATION;
                break;
            case 83455: // Chain Lightning N
            case 90027: // Chain Lightning H
                spellInfo->InterruptFlags |= SPELL_INTERRUPT_FLAG_INTERRUPT;
                break;
            case 83790: // Cloud Burst
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_DEST_DEST_RANDOM;
                spellInfo->Targets = TARGET_FLAG_DEST_LOCATION;
                break;
            case 83051: // Cloud Burst
            case 90032: // Cloud Burst
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(7);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(7);
                break;
            case 83566: // Wailing Winds
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_2].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                spellInfo->Effects[EFFECT_2].TargetB = 0;
                break;
            case 83094: // Wailing Winds N
            case 90031: // Wailing Winds H
                spellInfo->AttributesEx4 |= SPELL_ATTR4_TRIGGERED;
                break;
            case 83151: // Absorb Storms
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_NEARBY_ENTRY;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_CASTER;
                break;
            case 84126: // Call Scorpid
            case 82791: // Call Crocolisk
                spellInfo->Attributes = 0;
                break;
            case 84589: //Deflecting Winds
                spellInfo->Effects[0].Effect = 0;
                break;
            case 83089://Wailing Winds
                spellInfo->Effects[1].Effect = 0;
                break;
                // ENDOF LOST CITY OF THE TOL'VIR
                //
                // BARADIN HOLD SPELLS
                //
                // Occu'thar
            case 96913: // Searing Shadow
            case 101007:
                spellInfo->InterruptFlags &= ~SPELL_INTERRUPT_FLAG_INTERRUPT;
                break;
            case 96872: // Focused Fire
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 96883: // Focused Fire dmg
            case 101004:
                spellInfo->Effects[0].SetRadiusIndex(17);
                break;
            case 96920: // Eye of Occu'thar
            case 101006:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 96931: // Eye of Occu'thar script
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[0].TargetB = 0;
                break;
            case 96968: // Occu'thar's Destruction
            case 101008:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 96946: // Eye of Occu'thar dmg
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                break;
                // Alizabal
            case 105065: // Seething Hate Dummy
            case 108090:
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 105069: // Seething Hate dmg
            case 108094:
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_SHARE_DAMAGE;
                spellInfo->Effects[0].SetRadiusIndex(29);
                break;
            case 106248: // Blade Dance Dummy
                spellInfo->Effects[0].SetRadiusIndex(28);
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 104994: // Blade Dance dmg
                spellInfo->Effects[0].SetRadiusIndex(17);
                break;
            case 105726: // Blade Dance charge
                spellInfo->Effects[EFFECT_0].TriggerSpell = 0;
                break;
            case 105784: // Blade Dance aura 1
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
                spellInfo->Effects[EFFECT_0].TriggerSpell = 0;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_DUMMY;
                break;
                // ENDOF BARADIN HOLD SPELLS
                //
                // ZUL'GURUB SPELLS
                //
                // Venoxis
            case 96489: // Toxic Explosion
            case 97093:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(14);
                break;
            case 96560: // Word of Hethiss
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(13);
                break;
            case 96842: // Bloodvenom
                spellInfo->MaxAffectedTargets = 3;
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(41);
                break;
            case 96638: // Bloodvenom dmg
            case 97104:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(26);
                break;
            case 96685: // Venomous Infusion dmg
            case 97338:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(7);
                break;
            case 96475: // Toxic Link
                spellInfo->MaxAffectedTargets = 2;
                break;
            case 96476: // Toxic Link dummy
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->TargetAuraSpell = 96477;
                break;
                // Zanzil
            case 96319:
            case 96316:
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_DUMMY;
                spellInfo->Effects[EFFECT_0].ApplyAuraName = 0;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 96342: // Pursuit
                spellInfo->Effects[EFFECT_2].TargetA = TARGET_UNIT_CASTER;
                break;
            case 96914: // Zanzil Fire
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_MOD_STUN;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
                // Kilnara
            case 96457: // Wall of Agony
                spellInfo->AttributesEx &= ~SPELL_ATTR1_CHANNELED_1;
                break;
            case 96909: // Wail of Sorrow
            case 96948:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(12);
                break;
            case 96422: // Tears of Blood dmg
            case 96957:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(32);
                break;
            case 97355: // Gaping Wound jump
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                break;
            case 97357: // Gaping Wound dmg
            case 97358:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                break;
            case 98238: // Rat Lure
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(10);
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENTRY;
                break;
                // Jindo The Godbreaker
            case 96689: // Spirit World aura
                spellInfo->Effects[EFFECT_1].MiscValue = 2;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 97022: // Hakkar's Chains
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                spellInfo->ChannelInterruptFlags = 0;
                break;
            case 97172: // Shadows of Hakkar
            case 97198: // Body Slam
                spellInfo->InterruptFlags &= ~SPELL_INTERRUPT_FLAG_INTERRUPT;
                break;
            case 97597: // Spirit Warrior's Gaze
            case 97158:
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 97152: // Summon Spirit target
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(33);
                break;
            case 96964: // Sunder Rift
                spellInfo->SetDurationIndex(18);
                break;
            case 96970: // Sunder Rift:
                spellInfo->ProcChance = 100;
                break;
            case 97320: // Sunder Rift
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ALLY;
                spellInfo->InterruptFlags = 0;
                break;
                // ENDOF ZUL'GURUB SPELLS
                //
				// ZUL'AMAN SPELLS
				//
				// Akil'zon
			case 43648: // Electrical Storm
				spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_MOD_STUN;
				break;
				// Daakara
			case 15576: // Whirlwind 
				spellInfo->Effects[EFFECT_0].BasePoints = 100 * 2 + 10;
				break;
			case 43302:  // Lightning Totem
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_25_YARDS);
				break;
			case 43121:  // Tormenta Change target type
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_DEST_AREA_ENEMY;
				break;
				// Correct Triggers for Sweeping Winds Boss Daakara //
			case 97647: // Sweeping Winds Initial cast
				spellInfo->Effects[EFFECT_0].TriggerSpell = 97824;
				break;
			case 97824: // Sweeping Winds Continue Triggering
				spellInfo->Effects[EFFECT_0].TriggerSpell = 97648;
				break;
			case 97648: // Sweeping Winds Final Invoke
				spellInfo->Effects[EFFECT_0].MiscValue = 24136;
				break;
				// Janalai
			case 42471: // Hatch Eggs
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(10);
				spellInfo->MaxAffectedTargets = 3;
				break;
			case 42630: // Fire Bomb dmg
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(15);
				break;
			case 43140: // Flame Breath
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_CONE_LINE;
				spellInfo->InterruptFlags &= ~SPELL_INTERRUPT_FLAG_INTERRUPT;
				spellInfo->ChannelInterruptFlags &= ~SPELL_INTERRUPT_FLAG_INTERRUPT;
				break;
				// Halazzi
			case 97505: // Refreshing Stream heal
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(8);
				break;
				// Hex Lord Malacrass
			case 44132: // Drain Power
				spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
				break;
			case 43095: // Creeping Paralysis
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(12);
				break;
			case 97682: // Pillar of Flame dmg
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(7);
				break;
				// NPC Amani Savage (Spell Rend)
			case 12054: // Rend
				spellInfo->Effects[EFFECT_0].BasePoints = 487;
				break;
				// NPC Zandalari Hierophant
			case 23979:  // Ancient Power
				spellInfo->AttributesEx3 &= ~SPELL_ATTR3_ONLY_TARGET_PLAYERS;
				spellInfo->Effects[EFFECT_0].BasePoints = (1.5 * 15000);
				break;
			case 97978:  // Ancient Guardian
				spellInfo->Effects[EFFECT_0].BasePoints = 2;
				spellInfo->Effects[EFFECT_1].BasePoints = 50000;
				spellInfo->SetDurationIndex(1);
				break;
				// SPELL FIRE WALL FOR JAN'ALAI COMBAT
			case 43113:  //Firewall spell Visual aura
				spellInfo->SetDurationIndex(0);
				break;
				// ENDOF ZUL'AMAN SPELLS
				//				
                // END TIME SPELLS
                //
                // Trash
            case 109952: // Cadaver Toss
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 102066: // Flesh Rip
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
                // Echo of Tyrande
            case 102242: // Tears of Elune script
                spellInfo->MaxAffectedTargets = 2;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 102244: // Tears of Elune dmg
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_2_YARDS);
                break;
            case 102542: // Achievement
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 102002: // Shrink
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 102193: // Moonbolt
                spellInfo->InterruptFlags |= SPELL_INTERRUPT_FLAG_INTERRUPT;
                break;
            case 102149: // Moonlance dmg
                spellInfo->ExcludeTargetAuraSpell = 102149;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 102414: // Dark Moonlight
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_TARGET_ENEMY;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_DEST_AREA_ENEMY;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                spellInfo->Effects[EFFECT_1].TriggerSpell = 0;
                break;
            case 102173: // Stardust
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 102151: // Moonlance summon 1
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_5_YARDS);
                break;
                // Echo of Sylvanas
            case 101397: // Death Grip aoe
                spellInfo->Effects[EFFECT_0].TriggerSpell = 0;
                break;
            case 101200: // Summon Ghoul
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_100_YARDS);
                break;
            case 100865: // Wracking Pain target aura
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_100_YARDS);
                break;
            case 101257: // Wracking Pain dmg
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_100_YARDS);
                break;
                // Echo of Baine
            case 101619: // Magma
                spellInfo->AttributesEx3 &= ~SPELL_ATTR3_NO_DONE_BONUS;
                break;
            case 107837: // Throw Totem aura
                spellInfo->Effects[EFFECT_0].Amplitude = 11000;
                break;
            case 101625: // Pulverize aoe
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
            case 101815: // Pulverize gobject
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_10_YARDS);
                break;
                // Echo of Jaina
            case 101812: // Blink
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_DUMMY;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
                // Murozond
            case 102364: // Blessing of Bronze Dragons
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                break;
                // ENDOF END TIME SPELLS
                //
                // EYE OF ETERNITY SPELLS
                // All spells below work even without these changes. The LOS attribute is due to problem
                // from collision between maps & gos with active destroyed state.
            case 57473: // Arcane Storm bonus explicit visual spell
            case 57431: // Summon Static Field
            case 56091: // Flame Spike (Wyrmrest Skytalon)
            case 56092: // Engulf in Flames (Wyrmrest Skytalon)
            case 57090: // Revivify (Wyrmrest Skytalon)
            case 57143: // Life Burst (Wyrmrest Skytalon)
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS;
                break;
                // This would never crit on retail and it has attribute for SPELL_ATTR3_NO_DONE_BONUS because is handled from player, 
                // until someone figures how to make scions not critting without hack and without making them main casters this should stay here. 
            case 63934: // Arcane Barrage (casted by players and NONMELEEDAMAGELOG with caster Scion of Eternity (original caster)). 
                spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
                break;
                // ENDOF EYE OF ETERNITY SPELLS
                //
                // WELL OF_ ETERNITY SPELLS
                //
               // Legion Demons
            case 108479: // Crushing Leap
				spellInfo->Effects[EFFECT_1].SetRadiusIndex(EFFECT_RADIUS_15_YARDS);
				spellInfo->Effects[EFFECT_2].SetRadiusIndex(EFFECT_RADIUS_15_YARDS);
                break;
                // Perotharn
            case 105496: // Track player
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
           case 108214: //Fel flames visual & DMG aura
				spellInfo->SetDurationIndex(9); // 30 secs
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_15_YARDS);
				break;
			    // Queen Azshara
            case 102478: // Ice Fling
                spellInfo->MaxAffectedTargets = 1;
                break;
			case 102334: // Servant of Queen
				spellInfo->Attributes = SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY;
				spellInfo->AttributesEx = SPELL_ATTR1_CHANNELED_2;
				spellInfo->AttributesEx3 = SPELL_ATTR3_ONLY_TARGET_PLAYERS;
				spellInfo->AttributesEx5 |= SPELL_ATTR5_UNK14;
				spellInfo->SetDurationIndex(1); // 10 segs
				spellInfo->MaxAffectedTargets = 1;
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ENEMY;
				spellInfo->Effects[EFFECT_0].TargetB = 0;
				spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ENEMY;
				spellInfo->Effects[EFFECT_1].TargetB = 0;
				spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_MOD_CONFUSE;
				spellInfo->Effects[EFFECT_1].MaxRadiusEntry = sSpellRadiusStore.LookupEntry(0);
				spellInfo->Effects[EFFECT_2].Effect = 0;
				spellInfo->AuraInterruptFlags = 0x0;
				break;
			case 103241: // Total Obedience
				spellInfo->Attributes = 0;
				spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_MOD_CONFUSE;
				spellInfo->Effects[EFFECT_2].Effect = 0;
				spellInfo->InterruptFlags = SPELL_INTERRUPT_FLAG_INTERRUPT;
				spellInfo->Dispel = SPELL_AURA_DISPEL_IMMUNITY;
				break;
			case 110096: // Total Obedience Mod Stun After Effect
				spellInfo->SetDurationIndex(1); // 10 Segs
				break; 
			case 102310: // Puppet Cross
				spellInfo->SetDurationIndex(1); // 10 Segs
				break;
			case 102312: // Puppet Hover
				spellInfo->SetDurationIndex(1); // 10 Segs
				break;
			case 102315: // Puppet Strings 1
				spellInfo->SetDurationIndex(1); // 10 Segs
				break;
			case 102318: // Puppet Strings 2
				spellInfo->SetDurationIndex(1); // 10 Segs
				break;
			case 102319: // Puppet Strings 3
				spellInfo->SetDurationIndex(1); // 10 Segs
				break;
			// Mannoroth
			case 103891: // fel flames aura
				spellInfo->PreventionType = 0;
				spellInfo->AttributesEx |= SPELL_ATTR1_CANT_BE_REFLECTED;
				break;
            case 104648: // Nether Portal
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_3_YARDS);
                break;
			case 104678: // Debilitating Flay
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 105073: // Wrath of Elune
                spellInfo->MaxAffectedTargets = 3;
                break;
			case 105054: // Summon Felhound
            case 105058: // Summon Felguard
            case 105059: // Summon Devastator
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DB;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
            case 105523: // Magistrike Arc
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
			  // Illidan Tempestira
			case 104387: // Aura of Immolation
				spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
				break;
                // ENDOF WELL OF ETERNITY SPELLS
                //
                // THRONE OF THE TIDES SPELLS
                //
                // Lady Nazjar
            case 90479: // Waterspout dmg
                spellInfo->Effects[0].Effect = 0;
                break;
            case 75690: // Waterspout knock
                spellInfo->Effects[0].SetRadiusIndex(7);
                spellInfo->Effects[1].SetRadiusIndex(7);
                break;
            case 75700: // Geyser Errupt
            case 91469:
                spellInfo->Effects[0].SetRadiusIndex(8);
                spellInfo->Effects[1].SetRadiusIndex(8);
                break;
            case 94046: // Geyser Errupt knock
            case 94047:
                spellInfo->Effects[0].SetRadiusIndex(8);
                spellInfo->Effects[1].SetRadiusIndex(8);
                break;
            case 75813: // Chain Lightning
            case 91450:
                spellInfo->Effects[0].SetRadiusIndex(8);
                break;
            case 75993: // Lightning Surge dmg
            case 91451:
                spellInfo->Effects[0].SetRadiusIndex(8);
                break;
            case 80564: // Fungal Spores dmg
            case 91470:
                spellInfo->Effects[0].SetRadiusIndex(8);
                break;
                // Commander Ulthok
            case 76085: // Dark Fissure dmg
            case 91375:
                spellInfo->Effects[0].SetRadiusIndex(29);
                break;
            case 76047:
                spellInfo->Effects[0].TargetB = 0;
                spellInfo->Effects[0].SetRadiusIndex(29);
                break;
                // Erunak Stonespeaker
            case 84945: // Earth Shards dmg
            case 91491:
                spellInfo->Effects[0].SetRadiusIndex(8);
                break;
                // Mindbender Ghursha
            case 76341: // Unrelenting Agony dmg
            case 91493:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
                // Ozumat
            case 83463: // Entangling Grasp
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 83915: // Brain Spike
            case 91497:
                spellInfo->Effects[0].SetRadiusIndex(23);
                spellInfo->Effects[1].SetRadiusIndex(23);
                break;
            case 83561: // Blight of Ozumat dmg
            case 91495:
                spellInfo->Effects[0].SetRadiusIndex(13);
                break;
                // ENDOF THRONE OF THE TIDES SPELLS
                //
                // HOUR OF TWILIGHT SPELLS
                //
                // Arcurion
             case 102480: // Icy Boulder aoe
                spellInfo->MaxAffectedTargets = 1;
				spellInfo->AttributesEx3 &= ~SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
			case 105433: // Icy Boulder
				spellInfo->AttributesEx = SPELL_ATTR1_CHANNEL_DISPLAY_SPELL_NAME;
				break;
			case 102593: // Hand of Frost
				spellInfo->PreventionType = 0;
				spellInfo->Dispel = DISPEL_NONE;
				spellInfo->AttributesEx = SPELL_ATTR1_CANT_BE_REFLECTED;
				break;
                // Archbishop Benedictus
            case 103600: // Purifying Light targeting
            case 103768: // Corrupting Twilight targeting
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                break;
            case 103648: // Purifying Blast
            case 103776: // Twilight Bolt
                spellInfo->Effects[EFFECT_0].TriggerSpell = 0;
                break;
                // Mobs Various
			case 103021: // Hungering Shadows
				spellInfo->PreventionType = 0;
				spellInfo->ChannelInterruptFlags = 0;
				spellInfo->InterruptFlags = 0;
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_CASTER;
				spellInfo->Effects[EFFECT_0].TargetB = 0;
				break;
			case 103023: // Hungering Shadows Aura Handle
				spellInfo->SetRangeIndex(5);
				spellInfo->AttributesEx3 = 0;
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_CASTER;
				spellInfo->Effects[EFFECT_0].TargetB = 0;
				spellInfo->Effects[EFFECT_1].TargetA = TARGET_DEST_CASTER;
				spellInfo->Effects[EFFECT_1].TargetB = 0;
				break;
			case 108443: // Ice Arrow
				spellInfo->Effects[EFFECT_0].TriggerSpell = 102977;
				break;
			case 102978: //Shoot
				//spellInfo->Attributes = 0;
				spellInfo->AttributesEx6 = 0;
				spellInfo->EquippedItemClass = 0;
				spellInfo->EquippedItemSubClassMask = 0;
				spellInfo->InterruptFlags = 0;
				break;
                // ENDOF HOUR OF TWILIGHT SPELLS
                //
                // DRAGON SOUL SPELLS
                //
			case 105022://Deathwing Meteor Wyrmrest Event
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_TARGET_ANY;
				spellInfo->Effects[EFFECT_1].TargetA = TARGET_DEST_TARGET_ANY;
				break;
			case 108096://Twilight Portal Beam
				spellInfo->SetRangeIndex(6); //100 yards
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
				spellInfo->MaxAffectedTargets = 4;
				break;
            case 109247:
                spellInfo->Effects[EFFECT_0].BasePoints = 0;
                spellInfo->Effects[EFFECT_1].BasePoints = 0;
                break;
                // Flood aoe
            case 107796:
                spellInfo->MaxAffectedTargets = 1;
                break;
                // Flood channel
            case 107791:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
                // Boulder Smash aoe
            case 107596:
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 108220: // Deep Corruption
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE;
                break;
            case 108183: // Twilight Submission
                spellInfo->MaxAffectedTargets = 1;
                break;
                // Morchok
            case 103821: // Earthen Vortex
            case 110047:
            case 110046:
            case 110045:
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_MOD_STUN;
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_60_YARDS);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(EFFECT_RADIUS_60_YARDS);
                spellInfo->Effects[EFFECT_2].SetRadiusIndex(EFFECT_RADIUS_60_YARDS);
                break;
            case 103178: // Earths Vengeance dmg
                spellInfo->SetDurationIndex(618);
                break;
            case 103534: // Danger
            case 103536: // Warning
            case 103541: // Safe
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE;
                break;
            case 103687: // Crush Armor
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 103785: // Black Blood of the Earth dmg
            case 108570:
            case 110288:
            case 110287:
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 109033: // Stomp (heroic)
            case 109034:
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_SHARE_DAMAGE;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
			case 110307: // Siege Missile
				spellInfo->SetRangeIndex(13);
				break;
                // Yor'sahj The Unsleeping
            case 105420: // Color Combine 1
            case 105435: // Color Combine 2
            case 105436: // Color Combine 3
            case 105437: // Color Combine 4
            case 105439: // Color Combine 5
            case 105440: // Color Combine 6
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_DEST_AREA_ENTRY;
                break;
            case 105636: // Corrupted Minions aura
            case 109558:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 105534: // Mana Void dummy
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 109390: // Deep Corruption dmg (trash)
            case 105173: // Deep Corruption dmg
            case 108347:
            case 108348:
            case 108349:
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
                break;
            case 108228: // Mana Diffusion
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
                // Warlord Zon'ozz
            case 103527: // Void Diffusion dmg
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_SHARE_DAMAGE;
                spellInfo->AttributesEx |= SPELL_ATTR1_CANT_BE_REFLECTED;
                break;
			case 103948: // Disrupting Shadow dmg
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
				break;
            case 104031: // Void Diffusion debuff
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
            case 109197: // Tentacle Toss aoe 1
            case 109237: // Tentacle Toss aoe 2
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
            case 109240: // Tentacle Toss jump
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
			case 109258: // Tentacle Toss dmg
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
				break;
            case 104347: // Shadow Gaze
                spellInfo->AttributesEx4 &= ~SPELL_ATTR4_IGNORE_RESISTANCES;
                break;
            case 104378: // Black Blood of Go'rath
            case 110322:
                spellInfo->AttributesEx4 &= ~SPELL_ATTR4_IGNORE_RESISTANCES;
                break;
            case 104377: // Black Blood of Go'rath
            case 110306:
                spellInfo->AttributesEx4 &= ~SPELL_ATTR4_IGNORE_RESISTANCES;
                break;
                // Hagara the Stormbinder
            case 109393:
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ENEMY;
                break;
            case 109368: // Spark
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 109305: // Frozen Grasp
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_100_YARDS);
                break;
            case 109307: // Frozen Grasp grip
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_JUMP_DEST;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_CASTER;
                break;
            case 109315: // Icicle
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 105311: // Crystalline Tether
            case 105482:
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 105316: // Ice Lance dmg
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 105256: // Frozen Tempest
                spellInfo->Mechanic = 0;
                break;
            case 105367: // Lightning Conduit dummy 1
                //spellInfo->SetDurationIndex(39); // 2 secs
                //spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_10_YARDS);
                //spellInfo->Effects[EFFECT_0].ChainTarget = 25;
                spellInfo->SetDurationIndex(39); // 1 secs
                spellInfo->Effects[EFFECT_0].TriggerSpell = 0;
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_PERIODIC_DUMMY;
                spellInfo->Effects[EFFECT_1].Amplitude = 1000;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 105371: // Lightning Conduit dummy 2
                spellInfo->SetDurationIndex(39); // 1 secs
                spellInfo->AttributesEx5 |= SPELL_ATTR5_HIDE_DURATION;
                spellInfo->Effects[EFFECT_0].TriggerSpell = 0;
                break;
            case 105369: // Lightning Conduit dmg
                spellInfo->SetDurationIndex(39); // 1 secs
                spellInfo->AttributesEx5 |= SPELL_ATTR5_HIDE_DURATION;
                spellInfo->Effects[EFFECT_0].TriggerSpell = 0;
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_DONT_RESET_PERIODIC_TIMER;
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
                spellInfo->AttributesEx4 |= SPELL_ATTR4_IGNORE_RESISTANCES;
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
                break;
            case 92203: //Icicles Dps
                spellInfo->Effects[EFFECT_1].BasePoints = 150;
                break;
            case 110316:// Frostflake Snare
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(285); // 1s
                break;
            case 109337:// Frostflake Snare AreaTrigger
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(8); // 15s
                break;
            case 109325: // Frostflake
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 105289: // Shattered Ice
                spellInfo->InterruptFlags = 0;
                break;
            case 105409: // Water Shield
            case 109560:
                spellInfo->Effects[EFFECT_2].Amplitude = 8000;
                break;
            case 109557: // Storm Pillars
                spellInfo->MaxAffectedTargets = 1;
                break;
                // Ultraxion
            case 106374: // Unstable Twilight aoe 1
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 109176: // Unstable Twilight aoe 2
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 109397: // Cosmetic Intro Trigger    
                spellInfo->Effects[EFFECT_0].Amplitude = 2000;
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(21); // Infinite (-1).
                break;
            case 106371: // Hour of Twilight
                spellInfo->Effects[EFFECT_0].TriggerSpell = 103327;
                spellInfo->Effects[EFFECT_1].TriggerSpell = 106174;
                break;
            case 103327: //Hour of Twilight dmg
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 106042: //Gift of Life
            case 106049: //Essence of Dreams
            case 106050: //Source of Magic
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 109345: //Gift of Life for 25p
            case 109344: //Essence of Dreams for 25p
            case 109347: //Source of Magic for 25p
            case 106044: //Summmon Game Object Gift of Life
            case 106047: //Summmon Game Object Essence of Dreams
            case 106048: //Summmon Game Object Source of Magic
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
                //Warmaster_Blackhorn
            case 109204: // Twilight Barrage
            case 109205:
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_SHARE_DAMAGE;
                break;
            case 107588: // Twilight Onslaught
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                break;
            case 108038: // Harpoon
                spellInfo->SetRangeIndex(13); // 5000 yards
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 107518: // Detonate
                spellInfo->Effects[EFFECT_2].Effect = 0;
                break;
            case 108046: // Shockwave Dmg
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                break;
            case 110137: // Shockwave Dmg
                spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_STUNNED;
                break;
            case 108041: // Artillery Barrage
            case 109213:
                spellInfo->Effects[EFFECT_0].BasePoints = 0;
                break;
            case 108050: //Twilight Flame Missile
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                break;
            case 107558: //Degeneration
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_SHARE_DAMAGE;
                break;
            case 107567: //Brutal Strike
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_SHARE_DAMAGE;
                break;
                // Spine of Deathwing
            case 105490: // Fiery Grip
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE;
                break;
            case 105241: // Absorb Blood
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(26); // 4 yards
                break;
            case 105479: // Searing Plasma
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE;
                break;
            case 105777: // Roll Control
                spellInfo->SetDurationIndex(27); // 3 seconds
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                break;
            case 105847: // Seal Armor Breach
            case 105848:
            case 105835: // Nuclear Blast
                spellInfo->InterruptFlags = 0;
                break;
			case 106201:  //Blood of Deathwing
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
				break;
			case 106005: //Degradation
				spellInfo->AttributesEx3 &= ~SPELL_ATTR3_DEATH_PERSISTENT;
				break;
                // Madness of Deathwing
            case 106663: // Carrying Winds
            case 106668:
            case 106670:
            case 106672:
            case 106674:
            case 106676:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_0].TriggerSpell = 0;
                spellInfo->ExcludeCasterAuraSpell = 0;
                break;
            case 106794: // Shrapnel aura
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE;
                spellInfo->AttributesEx &= ~SPELL_ATTR1_CHANNELED_1;
                break;
            case 106776: // Impaling Tentacle summon
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
            case 106766: // Elementium Terror summon
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
            case 108597: // Corrupting Parasite
            case 108601:
            case 108649:
                spellInfo->Attributes |= SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY;
                break;
            case 108787://BackSlash Dps
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
                break;
            case 106728: // Tetanus
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_PROC_TRIGGER_SPELL;
                break;
            case 108813: // Unstable Corruption
                spellInfo->Effects[EFFECT_1].ApplyAuraName = 0;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                spellInfo->Effects[EFFECT_1].TriggerSpell = 0;
                break;
            case 109087: // Congealing Blood script
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 109089: // Congealing Blood missile
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                break;
            case 109102:// Congealing Blood Heal
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                spellInfo->Effects[EFFECT_2].Effect = 0;
                break;
            case 106444: // Impale
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE;
                break;
            case 107029: // Impale Aspect
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
                break;
            case 106548: // Impale Aspect
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
                break;
            case 105937: // Regenerative Blood heal
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
            case 105842: // Degenerative Blood 
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 105841: // Degenerative Blood DPS
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
            case 105445: // Degenerative Blood DPS
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 105799: // Time Zone missile
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                break;
            case 107055: // Time Zone missile
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                break;
            case 105569: // Cauterize dmg
                spellInfo->Effects[EFFECT_0].BasePoints = 55;
                break;
            case 106940: // Trigger Concentration
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_1].TargetB = 0;
                break;
            case 105830: // Time Zone aura (debuff)
                spellInfo->Effects[EFFECT_1].Effect = 0;
                spellInfo->Effects[EFFECT_1].ApplyAuraName = 0;
                break;
            case 106242: // Elementium Meteor target
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(21); // Infinite (-1).
                break;
            case 110628: //Elementium Meteor Aura
                spellInfo->Effects[EFFECT_0].Amplitude = 5200;
                break;
            case 105550: //Vehicle of Deathwing
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 110065: //Fire Dragon Soul Script
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
                // Aspects auras
                // Increase duration up to 10 secs
            case 106028: case 109571: case 109572: case 109573:
            case 106027: case 109622: case 109623: case 109624:
            case 106457: case 109640: case 109641: case 109642: case 106464:
            case 106029: case 109606: case 109607: case 109608:
                spellInfo->SetDurationIndex(1); // 10 secs
                spellInfo->AttributesEx3 |= SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
                // Increase amplitude
            case 105825:
            case 105823:
            case 106456:
            case 106463:
            case 106026:
            case 106039:
                spellInfo->Effects[EFFECT_0].Amplitude = 7000;
                break;
            case 110071: //Beam to the Dragon Soul
            case 110072:
            case 110076:
            case 110077:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 106527: // Cataclysm screen
                spellInfo->SetDurationIndex(1); // 10 secs
                break;
                // Expose Weakness
            case 106588: case 109582: case 109583: case 109584:
            case 106600: case 109619: case 109620: case 109621:
            case 106613: case 109637: case 109638: case 109639:
            case 106624: case 109728: case 109729: case 109730:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
                // ENDOF DRAGON SOUL SPELLS
                //
                // ICECROWN CITADEL SPELLS
                //
                // THESE SPELLS ARE WORKING CORRECTLY EVEN WITHOUT THIS HACK
                // THE ONLY REASON ITS HERE IS THAT CURRENT GRID SYSTEM
                // DOES NOT ALLOW FAR OBJECT SELECTION (dist > 333)
            case 70781: // Light's Hammer Teleport
            case 70856: // Oratory of the Damned Teleport
            case 70857: // Rampart of Skulls Teleport
            case 70858: // Deathbringer's Rise Teleport
            case 70859: // Upper Spire Teleport
            case 70860: // Frozen Throne Teleport
            case 70861: // Sindragosa's Lair Teleport
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DB;
                break;
            case 69055: // Saber Lash (Lord Marrowgar)
            case 70814: // Saber Lash (Lord Marrowgar)
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_5_YARDS); // 5yd
                break;
            case 69075: // Bone Storm (Lord Marrowgar)
            case 70834: // Bone Storm (Lord Marrowgar)
            case 70835: // Bone Storm (Lord Marrowgar)
            case 70836: // Bone Storm (Lord Marrowgar)
            case 72864: // Death Plague (Rotting Frost Giant)
            case 71160: // Plague Stench (Stinky)
            case 71161: // Plague Stench (Stinky)
            case 71123: // Decimate (Stinky & Precious)
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_100_YARDS); // 100yd
                break;
            case 72378: // Blood Nova (Deathbringer Saurfang)
            case 73058: // Blood Nova (Deathbringer Saurfang)
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_200_YARDS);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(EFFECT_RADIUS_200_YARDS);
                break;
            case 72385: // Boiling Blood (Deathbringer Saurfang)
            case 72441: // Boiling Blood (Deathbringer Saurfang)
            case 72442: // Boiling Blood (Deathbringer Saurfang)
            case 72443: // Boiling Blood (Deathbringer Saurfang)
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_200_YARDS);
                break;
            case 72769: // Scent of Blood (Deathbringer Saurfang)
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_200_YARDS);
                // no break
            case 72771: // Scent of Blood (Deathbringer Saurfang)
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(EFFECT_RADIUS_200_YARDS);
                break;
            case 72723: // Resistant Skin (Deathbringer Saurfang adds)
                // this spell initially granted Shadow damage immunity, however it was removed but the data was left in client
                spellInfo->Effects[EFFECT_2].Effect = 0;
                break;
            case 70460: // Coldflame Jets (Traps after Saurfang)
                spellInfo->SetDurationIndex(1); // 10 seconds
                break;
            case 71412: // Green Ooze Summon (Professor Putricide)
            case 71415: // Orange Ooze Summon (Professor Putricide)
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 71159: // Awaken Plagued Zombies
                spellInfo->SetDurationIndex(21);
                break;
            case 71127: // Mortal Wound  
                spellInfo->AttributesEx3 |= SPELL_ATTR3_IGNORE_HIT_RESULT;
                break;
            case 70530: // Volatile Ooze Beam Protection (Professor Putricide)
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_APPLY_AURA; // for an unknown reason this was SPELL_EFFECT_APPLY_AREA_AURA_RAID
                break;
            case 69508: // Slime Spray
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
                // THIS IS HERE BECAUSE COOLDOWN ON CREATURE PROCS IS NOT IMPLEMENTED
            case 71604: // Mutated Strength (Professor Putricide)
            case 72673: // Mutated Strength (Professor Putricide)
            case 72674: // Mutated Strength (Professor Putricide)
            case 72675: // Mutated Strength (Professor Putricide)
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 72454: // Mutated Plague (Professor Putricide)
            case 72464: // Mutated Plague (Professor Putricide)
            case 72506: // Mutated Plague (Professor Putricide)
            case 72507: // Mutated Plague (Professor Putricide)
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_50000_YARDS); // 50000yd
                break;
            case 70911: // Unbound Plague (Professor Putricide) (needs target selection script)
            case 72854: // Unbound Plague (Professor Putricide) (needs target selection script)
            case 72855: // Unbound Plague (Professor Putricide) (needs target selection script)
            case 72856: // Unbound Plague (Professor Putricide) (needs target selection script)
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_TARGET_ENEMY;
                break;
            case 71518: // Unholy Infusion Quest Credit (Professor Putricide)
            case 72934: // Blood Infusion Quest Credit (Blood-Queen Lana'thel)
            case 72289: // Frost Infusion Quest Credit (Sindragosa)
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_50000_YARDS); // another missing radius
                break;
            case 70232: // Empowered Blood
            case 70320: // Empowered Blood
                spellInfo->Effects[EFFECT_0].MiscValue = 127;
                spellInfo->Effects[EFFECT_0].MiscValueB = 127;
                break;
            case 71708: // Empowered Flare (Blood Prince Council)
            case 72785: // Empowered Flare (Blood Prince Council)
            case 72786: // Empowered Flare (Blood Prince Council)
            case 72787: // Empowered Flare (Blood Prince Council)
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                break;
            case 71266: // Swarming Shadows
            case 72890: // Swarming Shadows
                spellInfo->AreaGroupId = 0; // originally, these require area 4522, which is... outside of Icecrown Citadel
                break;
            case 26022: // Pursuit of Justice (Rank 1)
                spellInfo->Effects[1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[1].BasePoints = 7;
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK;
                spellInfo->Effects[1].TargetA = TARGET_UNIT_CASTER;
                break;
            case 26023: // Pursuit of Justice (Rank 1)
                spellInfo->Effects[1].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[1].BasePoints = 14;
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK;
                spellInfo->Effects[1].TargetA = TARGET_UNIT_CASTER;
                break;
            case 70602: // Corruption
            case 48278: // Paralyze
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 70715: // Column of Frost (visual marker)
                spellInfo->SetDurationIndex(32); // 6 seconds (missing)
                break;
            case 71085: // Mana Void (periodic aura)
                spellInfo->SetDurationIndex(9); // 30 seconds (missing)
                break;
            case 72015: // Frostbolt Volley (only heroic)
            case 72016: // Frostbolt Volley (only heroic)
                spellInfo->Effects[EFFECT_2].SetRadiusIndex(EFFECT_RADIUS_40_YARDS);
                break;
            case 70936: // Summon Suppressor (needs target selection script)
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
            case 72706: // Achievement Check (Valithria Dreamwalker)
            case 71357: // Order Whelp
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_200_YARDS);   // 200yd
                break;
            case 70598: // Sindragosa's Fury
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                break;
            case 69846: // Frost Bomb
                spellInfo->Speed = 0.0f;    // This spell's summon happens instantly
                break;
            case 71614: // Ice Lock
                spellInfo->Mechanic = MECHANIC_STUN;
                break;
            case 72762: // Defile
                spellInfo->SetDurationIndex(559); // 53 seconds
                break;
            case 72743: // Defile
                spellInfo->SetDurationIndex(22); // 45 seconds
                break;
            case 72754: // Defile
            case 73708: // Defile
            case 73709: // Defile
            case 73710: // Defile
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_200_YARDS); // 200yd
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(EFFECT_RADIUS_200_YARDS); // 200yd
                break;
            case 69030: // Val'kyr Target Search
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_200_YARDS); // 200yd
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(EFFECT_RADIUS_200_YARDS); // 200yd
                break;
            case 69198: // Raging Spirit Visual
                spellInfo->SetRangeIndex(13); // 50000yd
                break;
            case 73654: // Harvest Souls
            case 74295: // Harvest Souls
            case 74296: // Harvest Souls
            case 74297: // Harvest Souls
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_50000_YARDS); // 50000yd
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(EFFECT_RADIUS_50000_YARDS); // 50000yd
                spellInfo->Effects[EFFECT_2].SetRadiusIndex(EFFECT_RADIUS_50000_YARDS); // 50000yd
                break;
            case 72546: // Harvest Soul
            case 72597: // Harvest Soul
            case 72608: // Harvest Soul
                spellInfo->AttributesEx7 |= SPELL_ATTR7_ZONE_TELEPORT;
                break;
            case 73655: // Harvest Soul
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
                spellInfo->AttributesEx7 |= SPELL_ATTR7_ZONE_TELEPORT;
                break;
            case 73540: // Summon Shadow Trap
                spellInfo->SetDurationIndex(23); // 90 seconds
                break;
            case 73530: // Shadow Trap (visual)
                spellInfo->SetDurationIndex(28); // 5 seconds
                break;
            case 73529: // Shadow Trap
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(EFFECT_RADIUS_10_YARDS); // 10yd
                break;
            case 74282: // Shadow Trap (searcher)
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_3_YARDS); // 3yd
                break;
            case 72595: // Restore Soul
            case 73650: // Restore Soul
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_200_YARDS); // 200yd
                break;
            case 74086: // Destroy Soul
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_200_YARDS); // 200yd
                break;
            case 74302: // Summon Spirit Bomb
            case 74342: // Summon Spirit Bomb
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_200_YARDS); // 200yd
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 74341: // Summon Spirit Bomb
            case 74343: // Summon Spirit Bomb
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_200_YARDS); // 200yd
                spellInfo->MaxAffectedTargets = 3;
                break;
            case 73579: // Summon Spirit Bomb
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_25_YARDS); // 25yd
                break;
            case 72350: // Fury of Frostmourne
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_50000_YARDS); // 50000yd
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(EFFECT_RADIUS_50000_YARDS); // 50000yd
                spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_INSTAKILL;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                spellInfo->Effects[EFFECT_0].Amplitude = 50000;
                break;
            case 75127: // Kill Frostmourne Players
            case 72351: // Fury of Frostmourne
            case 72431: // Jump (removes Fury of Frostmourne debuff)
            case 72429: // Mass Resurrection
            case 73159: // Play Movie
            case 73582: // Trigger Vile Spirit (Inside, Heroic)
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_50000_YARDS); // 50000yd
                break;
            case 72376: // Raise Dead
                spellInfo->MaxAffectedTargets = 3;
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_50000_YARDS); // 50000yd
                break;
            case 71809: // Jump
                spellInfo->SetRangeIndex(3); // 20yd
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_25_YARDS); // 25yd
                break;
            case 72405: // Broken Frostmourne
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(EFFECT_RADIUS_200_YARDS); // 200yd
                break;
            case 68872: // Soulstorm (Bronjahm Encounter)
                spellInfo->InterruptFlags = 0;
                break;
                // ENDOF ICECROWN CITADEL SPELLS
                //
                // SHADOWFANG KEEP SPELLS
                // Baron Ashbury
            case 93720: // Wracking Pain
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(28);
                break;
            case 93810: // Calamity dmg
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(28);
                break;
            case 93423: // Asphyxiate
            case 93710:
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(28);
                spellInfo->Effects[EFFECT_2].SetRadiusIndex(28);
                break;
            case 93468: // Stay of Execution
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                break;
            case 93706: // Stay of Execution t
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(28);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(28);
                break;
                // Baron Silverlaine
            case 93956: // Cursed Veil
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(18);
                spellInfo->Effects[EFFECT_2].SetRadiusIndex(18);
                break;
                // Commander Springvale
            case 93722: // Shield of Perfidious dmg
            case 93737:
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CONE_ENEMY_24;
                break;
                // Lord Valden
            case 93697: // Conjure Poisonous Mixture
            case 93704:
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(18);
                spellInfo->Effects[EFFECT_2].SetRadiusIndex(18);
                break;
            case 93505: // Conjure Frost Mixture
            case 93702:
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(18);
                break;
            case 93617: // Toxic Coagulant dmg
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(12);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(12);
                spellInfo->Effects[EFFECT_2].SetRadiusIndex(12);
                break;
            case 93689: // Toxic Catalyst dmg
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(12);
                spellInfo->Effects[EFFECT_1].SetRadiusIndex(12);
                spellInfo->Effects[EFFECT_2].SetRadiusIndex(12);
                break;
                // Lord Godfrey
            case 96344: // Pistol Barrage sum
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(12);
                break;
                // ENDOF SHADOWFANG SPELLS
                //
                // BLACKROCK CAVERNS SPELLS
                //
                // Rom'ogg Bonecrusher 
            case 75272: // Quake
                spellInfo->Effects[0].SetRadiusIndex(23);
                spellInfo->Effects[1].SetRadiusIndex(23);
                break;
            case 82189: // Chains of Woe
                spellInfo->Effects[0].SetRadiusIndex(28);
                spellInfo->Effects[1].SetRadiusIndex(28);
                break;
                // Ascendant Lord Obsidius
            case 76186: // Thinderclap
                spellInfo->Effects[1].SetRadiusIndex(13);
                break;
            case 76164: // Shadow of Obsidius
                spellInfo->Effects[0].BasePoints = 10000000;
                break;
                // ENDOF BLACKROCK CAVERNS SPELLS
                //
                // THE VORTEX PINNACLE SPELLS
                //
                // Grand Vizier Ertan
            case 86292: // Cyclone Shield dmg
            case 93991:
                spellInfo->Effects[0].SetRadiusIndex(26);
                spellInfo->Effects[1].SetRadiusIndex(26);
                spellInfo->Effects[2].SetRadiusIndex(26);
                break;
                // Asaad
            case 87551: // Supremacy of the Storm dmg
            case 93994:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 87622: // Chain Lightning
            case 93993:
                spellInfo->Effects[0].SetRadiusIndex(12);
                break;
            case 87618: // Static Cling
                spellInfo->Effects[0].SetRadiusIndex(28);
                spellInfo->Effects[1].SetRadiusIndex(28);
                break;
            case 86911: // Unstable Grounding Field
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->InterruptFlags = 0;
                break;
            case 87517: // Grounding Field Visual Beams
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_2].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
                // Altairus
            case 88276: // Call of Wind Dummy
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 88282: // Upwind of Altairus
                spellInfo->Effects[1].SetRadiusIndex(9);
                break;
                // Trash
            case 87854: // Arcane Barrage
            case 92756:
                spellInfo->Effects[0].SetRadiusIndex(12);
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 88073: // Starfall
            case 92783:
                spellInfo->Effects[0].SetRadiusIndex(13);
                break;
            case 87761: // Rally
                spellInfo->Effects[0].SetRadiusIndex(10);
                break;
            case 87933: // Air Nova
            case 92753:
                spellInfo->Effects[1].SetRadiusIndex(13);
                break;
            case 85159: // Howling Gale dmg
                spellInfo->Effects[0].SetRadiusIndex(13);
                break;
            case 85084: // Howling Gale
                spellInfo->Effects[0].TriggerSpell = 85159;
                break;
            case 87765: // Lightning Lash Dummy
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
            case 87768: // Lightning Nova
            case 92780:
                spellInfo->Effects[0].SetRadiusIndex(13);
                spellInfo->Effects[1].SetRadiusIndex(13);
                break;
            case 88170: // Cloudburst
            case 92760:
                spellInfo->Effects[0].SetRadiusIndex(28);
                break;
                // ENDOF THE VORTEX PINNACLE SPELLS
                //
                // DEADMINES SPELLS
                // Admiral Ripsnarl
            case 88736: // Taste for Blood
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_DUMMY;
                spellInfo->Effects[EFFECT_0].TriggerSpell = 0;
                spellInfo->ProcChance = 0;
                spellInfo->ProcFlags = 0;
                break;
            case 95647: // Ripsnarl Achievement Aura
                spellInfo->AttributesEx3 = SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
                // Captain Cookie
            case 89250: // Summon Cauldron
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                break;
            case 89268: // Throw Food Targeting
            case 89740:
            case 90561:
            case 90562:
            case 90582:
            case 90583:
            case 90563:
            case 90584:
            case 90564:
            case 90585:
            case 90565:
            case 90586:
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->AttributesEx3 = SPELL_ATTR3_ONLY_TARGET_PLAYERS;
                break;
                // Vanessa Vancleef
            case 92620: // Backslash targeting
                spellInfo->MaxAffectedTargets = 1;
                break;
                // ENDOF DEADMINES
                //
                // ULDUAR SPELLS
                //
            case 64014: // Expedition Base Camp Teleport
            case 64032: // Formation Grounds Teleport
            case 64028: // Colossal Forge Teleport
            case 64031: // Scrapyard Teleport
            case 64030: // Antechamber Teleport
            case 64029: // Shattered Walkway Teleport
            case 64024: // Conservatory Teleport
            case 64025: // Halls of Invention Teleport
            case 64027: // Descent into Madness Teleport
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DB;
                break;
            case 62374: // Pursued (Flame Leviathan)
                spellInfo->Effects[0].SetRadiusIndex(EFFECT_RADIUS_50000_YARDS);   // 50000yd
                break;
            case 62383: // Shatter (Ignis)
                spellInfo->SpellVisual[0] = 12639;
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
                break;
            case 63342: // Focused Eyebeam Summon Trigger (Kologarn)
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 62716: // Growth of Nature (Freya)
            case 65584: // Growth of Nature (Freya)
            case 64381: // Strength of the Pack (Auriaya)
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 63018: // Searing Light (XT-002)
            case 65121: // Searing Light (25m) (XT-002)
            case 63024: // Gravity Bomb (XT-002)
            case 64234: // Gravity Bomb (25m) (XT-002)
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 62834: // Boom (XT-002)
                // This hack is here because we suspect our implementation of spell effect execution on targets
                // is done in the wrong order. We suspect that EFFECT_0 needs to be applied on all targets,
                // then EFFECT_1, etc - instead of applying each effect on target1, then target2, etc.
                // The above situation causes the visual for this spell to be bugged, so we remove the instakill
                // effect and implement a script hack for that.
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 64386: // Terrifying Screech (Auriaya)
            case 64389: // Sentinel Blast (Auriaya)
            case 64678: // Sentinel Blast (Auriaya)
                spellInfo->SetDurationIndex(28); // 5 seconds, wrong DBC data?
                break;
            case 64321: // Potent Pheromones (Freya)
                // spell should dispel area aura, but doesn't have the attribute
                // may be db data bug, or blizz may keep reapplying area auras every update with checking immunity
                // that will be clear if we get more spells with problem like this
                spellInfo->AttributesEx |= SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY;
                break;
            case 62301: // Cosmic Smash (Algalon the Observer)
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 64598: // Cosmic Smash (Algalon the Observer)
                spellInfo->MaxAffectedTargets = 3;
                break;
            case 62293: // Cosmic Smash (Algalon the Observer)
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_DEST_CASTER;
                break;
            case 62311: // Cosmic Smash (Algalon the Observer)
            case 64596: // Cosmic Smash (Algalon the Observer)
                spellInfo->SetRangeIndex(6);  // 100yd
                break;
                // ENDOF ULDUAR SPELLS
                //

                // -- PVE CONTENT -- PANDARIA //

            case 126389: // Goblin Glider Engineer
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(9);// 30s
                break;
            case 126408: // Goblin Glider Forward Thrust Engineer
                spellInfo->Effects[0].Effect = SPELL_EFFECT_KNOCK_BACK_DEST;
                spellInfo->Effects[0].TargetB = TARGET_DEST_CASTER_BACK;
                spellInfo->Effects[0].BasePoints = 75;
                spellInfo->Effects[0].MiscValue = 250;
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(8);
                break;

                //Mob 
            case 130721: // Torpedo Rocket Dmg
            case 130994: // Powder Round Dmg
                spellInfo->ExplicitTargetMask = TARGET_FLAG_NONE;
                break;

                //Rare Mob ViceJaws
            case 121626: //Between a Saurok and a Hard Place
                spellInfo->Effects[EFFECT_0].MiscValue = 67;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 126382: //Hozen in the Mist
                spellInfo->Effects[EFFECT_0].MiscValue = 176;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 126383: //Fish Tails
                spellInfo->Effects[EFFECT_0].MiscValue = 177;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 126385: //Dark Heart of the Mogu
                spellInfo->Effects[EFFECT_0].MiscValue = 178;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 126386: //What is Worth Fighting For
                spellInfo->Effects[EFFECT_0].MiscValue = 180;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 126388: //Heart of the Mantid Swarm
                spellInfo->Effects[EFFECT_0].MiscValue = 179;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 126387: //Song of the Yaungol
                spellInfo->Effects[EFFECT_0].MiscValue = 181;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 126395: //The Seven Burdens of Shaohao
                spellInfo->Effects[EFFECT_0].MiscValue = 182;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 126396: //Ballad of Liu Lang   
                spellInfo->Effects[EFFECT_0].MiscValue = 174;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;

                //Rare Mob ViceJaws
            case 127620: //Vice Jaws 
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_CASTER;
                break;
            case 126195: //Dirty Mouth
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_CASTER;
                break;
            case 126104: //Chomp Dmg 
                spellInfo->Effects[EFFECT_2].Effect = 0;
                spellInfo->Effects[EFFECT_3].Effect = 0;
                break;
            case 126131: //Chomp Transform
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
				//
				//
				// WORLD BOSSES PANDARIA
			  /////////////
			   /////////////////////
			////
			  //// World Boss Sha of Anger (WORLD BOSS).
			case 119487: //Seethe (Hervir Furia)
				spellInfo->StackAmount = 1;
				break;
			case 119626: // Aggressive Behavior
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
				break;


                // DUNGEONS AND RAIDS - CHECKED 18019 !!!!

                // STORMSTOUT BREWERY (DUNGEON).

                // Doorguard - override Jump Dest DB effect
            case 114809:
                spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_KNOCK_BACK;
                spellInfo->Effects[EFFECT_0].TargetA = 0;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENTRY;
                spellInfo->Effects[EFFECT_0].BasePoints = 100;
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_5_YARDS);
                spellInfo->Effects[EFFECT_0].MiscValue = 500;
                break;
                // Ook Ook
            case 106807: // Ground Pound.
            case 112944: // Carrot Breath.
                spellInfo->ChannelInterruptFlags = 0x0;
                spellInfo->InterruptFlags = 0x0;
                break;
            case 106808: // Ground Pound (Damage).
                spellInfo->AttributesEx &= ~SPELL_ATTR1_CHANNELED_2;
                break;
            case 107351: //Brew Explosion Hozen
            case 106784: //Brew Explosion Ook Ook
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
                // Hoptallus
            case 112945: // Carrot Breath (Damage).
                spellInfo->ExcludeTargetAuraSpell = 0;
                break;
                // Yanzhu the Uncasked
            case 128254:// Brew Finale Wheat Effect
            case 128256:// Brew Finale Medium Effect
            case 128258:// Brew Finale Dark Effect
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_DEST_DEST;
                break;
            case 106546: //Bloat Vehicle
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 144466: // Wall of Suds (Damage).
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 106859: // Ferment.
                spellInfo->ChannelInterruptFlags = 0x0;
                spellInfo->InterruptFlags = 0x0;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 114451: // Ferment (Damage).
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[EFFECT_2].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 116155: // Brew Bolt.
                spellInfo->AttributesEx6 &= ~SPELL_ATTR6_UNK14;
                break;
            case 114468: // Sudsy
                spellInfo->ProcFlags = 0;
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_PERIODIC_TRIGGER_SPELL;
                spellInfo->Effects[EFFECT_0].Amplitude = 200;
                break;

                //END OF STORMSTOUT BREWERY (DUNGEON).

                // TEMPLE OF THE JADE SERPENT (DUNGEON).

                //Wise Mari

            case 115167: // Corrupted Waters Dps                
                spellInfo->Effects[EFFECT_2].Effect = 0;
                break;
            case 106334: // Wash Away Dps         
                spellInfo->Effects[EFFECT_2].Effect = 0;
                break;
            case 106267: // Wise Mari Hydrolance damage Bottom      
                spellInfo->Effects[0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
                spellInfo->AttributesEx4 |= SPELL_ATTR4_IGNORE_RESISTANCES;
                break;
            case 106612: //Bubble Burst
                spellInfo->Effects[EFFECT_0].Effect = 0;
                break;
            case 106329://Wash Away
                spellInfo->Effects[EFFECT_0].Effect = 0;
                break;
            case 115575: // Wash Away Aura Dummy          
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 106105: //Hydrolance Dmg Left and Right            
            case 106319: //Hydrolance Pre Bottom Dmg
            case 106104: //Hydrolance Pre Left and Right Dmg   
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
                spellInfo->AttributesEx4 |= SPELL_ATTR4_IGNORE_RESISTANCES;
                break;

                //Lorewalker Stonestep
            case 111764: //Extract Sha
            case 111806: //Extract Sha
                spellInfo->Effects[0].TargetA = TARGET_DEST_DEST;
                break;

                //Liu Flameheart
            case 106909: //Jade Soul
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;

                //Sha of Doubt
            case 106112: //Release Doubt
                //spellInfo->Effects[1].TargetB = TARGET_UNIT_TARGET_ANY;
                spellInfo->ExplicitTargetMask |= TARGET_FLAG_DEST_LOCATION;
                break;
            case 106228: //Nothingness Dmg
                spellInfo->AttributesEx |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
                break;

                //END OF TEMPLE OF THE JADE SERPENT (DUNGEON).

                // SIEGE OF NIUZAO TEMPLE (DUNGEON).

                //Mob Trash
            case 119354: //Sik'thik Strike
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 119840: //Serrated Blade
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;
            case 124017: //Fire Catapult
                spellInfo->Speed = 28.0f;
                break;

                //Vizier Jinbak
            case 120095: // Cosmetic Detonation 
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 120001: // Detonation Aura 
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 120865: // Grow Sap Puddle
                spellInfo->Effects[1].Effect = 0;
                break;

                //Commander Vojak
            case 120270: //Slowed
                spellInfo->Mechanic = MECHANIC_NONE;
                spellInfo->Effects[1].Mechanic = MECHANIC_NONE;
                break;
            case 120200: //Bombard Dmg
                spellInfo->Effects[1].Effect = 0;
                break;
            case 120202: //Bombard Dummy
                spellInfo->Effects[0].BasePoints = 0;
                break;

                //General Pavalak
            case 124283: //Blade Rush Missile
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 124290: //Blade Rush Sword Dmg
                spellInfo->Effects[0].TargetB = TARGET_UNIT_DEST_AREA_ENEMY;
                break;
            case 124317: //Blade Rush Charge Dmg
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 119703: //Detonate Dmg Player
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                break;

                //Wing Leader Neronok
            case 121448: //Encased in Resin
                spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_MOD_STUN;
                break;
            case 121440: // Quick Dry Resin
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 125318: // Gusting Winds
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(1);
                spellInfo->Effects[0].TargetA = TARGET_DEST_DB;
                break;

                //END OF SIEGE OF NIUZAO TEMPLE (DUNGEON).

                // SCHOLOMANCE (DUNGEON).

                //Trash Mob
            case 111801: // Unholy Blade
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_CAN_STACK_FROM_DIFF_CASTERS;
                break;
                // Instructor Chillheart
            case 111854: // Ice wall
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 122449: //Ice Wall Enter door
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(21); // Infinite (-1).
                break;
            case 122499: //Cosmetic Shadow Channel
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;                
                break;
            case 113854: // Arcane Bomb
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DEST;
                break;
                //Jandice Barov
            case 113775: // Wirl of Illusion
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 114038: // Gravity Flux Dmg
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
                break;
            case 114062: // Wondrous Rapidity
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
                // Rattlegore
            case 113996: // Bone Armor
                spellInfo->ProcFlags = 0;
                spellInfo->ProcChance = 0;
                break;
            case 114077: //Bone Cosmetic
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[0].TargetB = 0;
                break;
                // Lilian Voss
            case 114776: // Shadow Shiv aoe
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_DYNOBJ_NONE;
                spellInfo->Effects[EFFECT_0].TargetB = 0;
                break;
            case 111773: // Shadow Shiv vehicle
                spellInfo->Effects[EFFECT_1].Effect = 0;
                break;
            case 112081: // visual scholomance ray
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ALLY;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_TARGET_ALLY;
                break;
            case 114076:
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ALLY;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_TARGET_ALLY;
                break;
            case 113395: // HARSH WORDS
                spellInfo->MaxAffectedTargets = 1;
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ENEMY;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_TARGET_ENEMY;
                break;
            case 111585: ///< Dark blaze
            case 111609:
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE_EFF0;
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE_EFF1;
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_NEGATIVE_EFF2;
                break;
            case 114874:
                spellInfo->Attributes &= ~SPELL_ATTR0_LEVEL_DAMAGE_CALCULATION;
                break;

                //END OF SCHOLOMANCE (DUNGEON).

                // THRONE OF THUNDER (RAID).

                // Jin'Rokh the Breaker.

            case 137261: // Lightning Storm (Damage).
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 137485: // Lightning Fissure (the same like above).
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_5_YARDS);
                break;
            case 138002: // Fluidity.
                spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(21); // Infinite (-1).
                spellInfo->Effects[EFFECT_2].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_3].Effect = SPELL_EFFECT_APPLY_AURA;
                spellInfo->Effects[EFFECT_0].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENTRY;
                spellInfo->Effects[EFFECT_1].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_SRC_AREA_ENTRY;
                spellInfo->Effects[EFFECT_2].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[EFFECT_2].TargetB = TARGET_UNIT_SRC_AREA_ENTRY;
                spellInfo->Effects[EFFECT_3].TargetA = TARGET_SRC_CASTER;
                spellInfo->Effects[EFFECT_3].TargetB = TARGET_UNIT_SRC_AREA_ENTRY;
                break;

                // Megaera.

            case 139433: // Rampage Fire Cast.
                spellInfo->Effects[0].Effect = SPELL_EFFECT_TRIGGER_MISSILE;
                spellInfo->Effects[0].TargetA = TARGET_DEST_TARGET_ENEMY;
                spellInfo->Effects[0].TriggerSpell = 139548;
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 139440: // Rampage Frost Cast.
                spellInfo->Effects[0].Effect = SPELL_EFFECT_TRIGGER_MISSILE;
                spellInfo->Effects[0].TargetA = TARGET_DEST_TARGET_ENEMY;
                spellInfo->Effects[0].TriggerSpell = 139549;
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 139504: // Rampage Poison Cast.
                spellInfo->Effects[0].Effect = SPELL_EFFECT_TRIGGER_MISSILE;
                spellInfo->Effects[0].TargetA = TARGET_DEST_TARGET_ENEMY;
                spellInfo->Effects[0].TriggerSpell = 139551;
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 139513: // Rampage Arcane Cast.
                spellInfo->Effects[0].Effect = SPELL_EFFECT_TRIGGER_MISSILE;
                spellInfo->Effects[0].TargetA = TARGET_DEST_TARGET_ENEMY;
                spellInfo->Effects[0].TriggerSpell = 139552;
                spellInfo->MaxAffectedTargets = 1;
                break;
            case 139548: // Rampage Fire.
            case 139549: // Rampage Frost.
            case 139551: // Rampage Poison.
            case 139552: // Rampage Arcane.
                spellInfo->Effects[0].TargetB = TARGET_UNIT_SRC_AREA_ENEMY;
                break;
            case 139866: // Torrent of Ice.
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;

                //World Boss
            case 132205: //Sha of Anger Bonus
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 31589: //Slow
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_CAN_STACK_FROM_DIFF_CASTERS;
                break;
            case 119610: //Bitter Thoughts
                spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_CAN_STACK_FROM_DIFF_CASTERS;
                break;
                //Guild
            case 89481: //Guild Battle Standard
                spellInfo->SetDurationIndex(347); // 900000 seconds
                break;
            case 90216: //Guild Battle Standard
                spellInfo->Effects[2].ApplyAuraName = SPELL_AURA_MOD_REPUTATION_GAIN;
                break;

                // SIEGE OF ORGRIMMAR (RAID).

                // Immerseus.

            case 143412: // Swirl.
                spellInfo->Effects[0].Amplitude = 1000;
                break;

                // Fallen Protectors.

            case 144396: // Vengeful Strikes (Rook Stonetoe).
                spellInfo->ChannelInterruptFlags = 0x0;
                spellInfo->InterruptFlags = 0x0;
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_NONE;
                break;

                // Norushen / Amalgam of Corruption.

            case 145212: // Unleashed Anger.
                spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
                break;
            case 145735: // Icy Fear.
            case 147082: // Burst of Anger.
            case 145073: // Residual Corruption.
                spellInfo->TargetAuraSpell = 0;
                break;
            case 144421: // Corruption.
                spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_MOD_DAMAGE_DONE_VERSUS;
                spellInfo->Effects[1].MiscValue = (1 << (CREATURE_TYPE_ABBERATION - 1)); // 8192 - mask.
                spellInfo->Effects[1].RadiusEntry = sSpellRadiusStore.LookupEntry(36);   // No radius.
                spellInfo->Effects[1].MaxRadiusEntry = sSpellRadiusStore.LookupEntry(36);
                break;
            case 144724: // Look Within.
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_NONE;
                break;
            case 145768: // Unleash Corruption.
            case 145769:
            case 144848: // Titanic Corruption.
            case 144980: // Greater Corruption.
                spellInfo->Effects[0].TargetA = TARGET_DEST_TARGET_ANY;
                break;
			case 125557: // Imperial Silk Discovery [Tailoring]
			case 143255: // Balanced Trillium Ingot [Blacksmithing]
			case 140040: // Magnificence of Leather [Leatherworking]
			case 140041: // Magnificence of Scales  [Leatherworking]
				spellInfo->Effects[0].Effect = SPELL_EFFECT_CREATE_ITEM_2;
				break;
            case 144448: // Cleanse Corruption.
            case 144449:
            case 144450:
            case 147657:
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 144849: // Test of Serenity.
            case 144850: // Test of Reliance.
                spellInfo->SetDurationIndex(3); // 1 minute.
                break;
            case 145227: // Blind Hatred.
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[0].TargetB = TARGET_UNIT_TARGET_ANY;
                spellInfo->Effects[1].RadiusEntry = sSpellRadiusStore.LookupEntry(36);   // No radius.
                spellInfo->Effects[1].MaxRadiusEntry = sSpellRadiusStore.LookupEntry(36);
                break;
            case 144479: // Expel Corruption.
            case 145064:
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                spellInfo->Effects[1].Effect = 0;
                break;
               //
                //
			    // Mogu'shan Palace
			    //
			    // Ming the Cunning
			case 120087: // Throw
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ANY;
				spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ANY;
				break;
			case 123654:  // Lightning Bolt Can be INTERRUPTED
				spellInfo->InterruptFlags = SPELL_INTERRUPT_FLAG_INTERRUPT;
				break;
				// Kuai Mushiba
		   	case 119949: // Ravage
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ENEMY;
				break;
			case 119948: // Ravage Trigger
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ENEMY;
				break;
				// Haiyan The Unstoppable
			case 120160:
				spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_PERIODIC_TRIGGER_SPELL;
				spellInfo->Effects[EFFECT_0].Amplitude = 5000;
				spellInfo->Effects[EFFECT_0].TriggerSpell = 120201;
				spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_MOD_CONFUSE;
				spellInfo->Effects[EFFECT_1].Amplitude = 0;
				break;
				// Glintrok's Spells
			case 118940: //Cleansing Flame
				spellInfo->AttributesEx &= ~SPELL_ATTR1_CANT_TARGET_SELF;
				break;
				//
                // Trash Mobs
			case 124352: //AuraCount for Achievement Quilen Statues [Heroic: Mogu'Shan Palace]
				spellInfo->StackAmount = 10;
				spellInfo->ProcChance = 100;
				spellInfo->AuraInterruptFlags = 0;
				spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER_AREA_PARTY;
				break;
				//
               //
				//
				// SHADO-PAN MONASTERY.
			   //
				// Gu Cloudstrike:
			case 106932:  // Static Field Initial Damage
				spellInfo->AttributesEx = SPELL_ATTR1_CANT_BE_REDIRECTED | SPELL_ATTR1_CANT_BE_REFLECTED;
				spellInfo->AttributesEx4 = SPELL_ATTR4_IGNORE_RESISTANCES;
				break;
			case 128889:  // Static Field Ticks
				spellInfo->SchoolMask = SPELL_SCHOOL_MASK_NATURE;
				spellInfo->AttributesEx = SPELL_ATTR1_CANT_BE_REDIRECTED | SPELL_ATTR1_CANT_BE_REFLECTED;
				spellInfo->AttributesEx4 |= SPELL_ATTR4_IGNORE_RESISTANCES;
				break;
			case 111129:  // Overcharged Soul Damage
				spellInfo->AttributesEx4 |= SPELL_ATTR4_IGNORE_RESISTANCES;
				break;
				// Master Snowdrift:
				// case 106853-> "FIST OF FURY" -> With Effect AURA PERIODIC TRIGG was modified 
				// by someone up there on this file, that modification affect the correct fuction
				// of the spell on this Boss->"Boss_Master_Snowdrift".
				// The Modidy was on TARGET_A changed to TARGET_UNIT_TARGET_ENEMY from TARGET_UNIT_CASTER.
				// I comment it and my boss's spell finally work. I let it commented.
				//
			case 106422:  // Quivering Palm
				spellInfo->Effects[EFFECT_2].TargetA = TARGET_UNIT_TARGET_ENEMY;
				spellInfo->AttributesEx4 = SPELL_ATTR4_IGNORE_RESISTANCES;
				spellInfo->AttributesCu = SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE | SPELL_ATTR0_CU_IGNORE_ARMOR;
				spellInfo->InterruptFlags = 0;
				break;
			case 106423:  // Unleash Vibrations
				spellInfo->AttributesEx4 = SPELL_ATTR4_IGNORE_RESISTANCES;
				spellInfo->AttributesCu = SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE | SPELL_ATTR0_CU_IGNORE_ARMOR;
				spellInfo->InterruptFlags = 0;
				break;
			case 118961: // Chase Down
				spellInfo->Effects[EFFECT_0].ApplyAuraName = SPELL_AURA_MOD_ROOT;
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_TARGET_ENEMY;
				spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_APPLY_AURA;
				spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_CONTROL_VEHICLE;
				spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_TARGET_ENEMY;
				spellInfo->Effects[EFFECT_2].Effect = SPELL_EFFECT_APPLY_AURA;
				spellInfo->Effects[EFFECT_2].Effect = SPELL_AURA_DUMMY;
				spellInfo->Effects[EFFECT_2].TargetA = TARGET_UNIT_CASTER;
				break;
			case 106345: // Tornado Slam Jump
				spellInfo->Effects[EFFECT_0].Effect = SPELL_EFFECT_JUMP;
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_CASTER;
				spellInfo->Effects[EFFECT_1].TargetA = TARGET_UNIT_VEHICLE;
				spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_CASTER;;
				spellInfo->Attributes &= ~SPELL_ATTR0_HIDDEN_CLIENTSIDE;
				break;
			case 106352: // Tornado Slam Dps
				spellInfo->Effects[EFFECT_1].ApplyAuraName = SPELL_AURA_NONE;
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_UNIT_VEHICLE;
				spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_CASTER;
				spellInfo->InterruptFlags = 0;
				spellInfo->AttributesEx6 |= SPELL_ATTR6_CASTABLE_WHILE_ON_VEHICLE;
				spellInfo->AttributesEx &= ~SPELL_ATTR1_DONT_DISPLAY_IN_AURA_BAR;
				break;
			case 106439:  // Flying Kick
				spellInfo->AttributesEx4 |= SPELL_ATTR4_IGNORE_RESISTANCES;
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
				spellInfo->InterruptFlags = 0;
				spellInfo->Effects[EFFECT_2].Effect = SPELL_EFFECT_KNOCK_BACK;
				spellInfo->Effects[EFFECT_0].TargetA = TARGET_DEST_CASTER_FRONT;
				spellInfo->DurationEntry = sSpellDurationStore.LookupEntry(27); // 3 seconds
				spellInfo->SchoolMask = 0;
				break;
				// Sha of Violence
			case 106827:  // Smoke Blades
				spellInfo->AttributesEx2 |= SPELL_ATTR2_TRIGGERED_CAN_TRIGGER_PROC;
				spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_MOD_CRIT_PCT;
				spellInfo->Effects[1].BasePoints = -100;
				break;
			case 127576: //Parting Smoke
				spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_CRIT_PCT;
				spellInfo->Effects[1].BasePoints = 200;
				break;
			case 106872: //Disoriented Smash
				spellInfo->Attributes &= ~SPELL_ATTR0_ABILITY;
				break;
			case 106871: // Sha Spike
				spellInfo->CastTimeEntry = sSpellCastTimesStore.LookupEntry(5);
				spellInfo->AttributesEx9 &= ~SPELL_ATTR9_UNK30;
				break;
				// Taran Tzu
			case 107085: // Rising Hate
				spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_SRC_AREA_ENTRY;
				spellInfo->Effects[EFFECT_1].TargetB = TARGET_UNIT_SRC_AREA_ENTRY;
				break;
				// Trash Mobs
			case 106681:  //Call Staff MiniBoss
				spellInfo->Effects[EFFECT_1].Effect = SPELL_EFFECT_SUMMON;
				spellInfo->Effects[EFFECT_1].BasePoints = 0;
				spellInfo->Effects[EFFECT_1].MiscValue = 56678; // Jade Staff Creature Doesn't have appropiate model ID.
				spellInfo->Effects[EFFECT_1].MiscValueB = 64;
				spellInfo->Effects[EFFECT_1].TargetA = TARGET_DEST_CASTER_BACK;
				spellInfo->Effects[EFFECT_2].Effect = SPELL_EFFECT_TRIGGER_MISSILE;
				spellInfo->Effects[EFFECT_2].TriggerSpell = 106620;
				spellInfo->Effects[EFFECT_2].TargetA = TARGET_UNIT_CASTER;
				spellInfo->Effects[EFFECT_2].TargetB = 0;
				break;
			case 106620:  // Jade Staff Protection MiniBoss
				spellInfo->Effects[EFFECT_0].TargetB = TARGET_UNIT_CASTER;
				break;
			case 112911:  // Volley of Hatred
				spellInfo->Attributes &= ~SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY;
				spellInfo->AttributesEx &= ~SPELL_ATTR1_DONT_DISPLAY_IN_AURA_BAR;
				break;
			case 53235:  // Unstable Energy
				spellInfo->SpellVisual[0] = 0;
				spellInfo->Effects[0].Amplitude = 1000;
				spellInfo->AttributesEx &= ~SPELL_ATTR1_CHANNELED_1;
				spellInfo->AttributesEx |= SPELL_ATTR1_DONT_DISPLAY_IN_AURA_BAR;
				break;
			case 128234: // Unstable Energy Cosmetic
				spellInfo->Effects[0].Amplitude = 1000;
				break;
			case 115055: // Volatile Explosive
				spellInfo->AttributesEx4 |= SPELL_ATTR4_IGNORE_RESISTANCES;
				break;
				//
			   //
				// SCARLET MONASTERY (DUNGEON).
			   //
				// Thalnos The Soulrender
			case 115290: // Spirit Gale Trigger
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
				spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
				break;
			case 115291:  // Spirit Gale DMG Aura
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
				spellInfo->AttributesEx2 |= SPELL_ATTR2_CANT_CRIT;
				break;
			case 115157: // Empowering Spirit
				spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_POSSESS;
				break;
			case 40293: // Siphon Soul
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
				break;
				//   
				// Brother Korloff
			case 114465: // Scorched Earth
				spellInfo->Effects[0].SetRadiusIndex(EFFECT_RADIUS_5_YARDS);
				spellInfo->Effects[0].TargetB = TARGET_UNIT_SRC_AREA_ENTRY;
				break;
			case 114808:  // Blazing Fist
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
				break;
			case 114807: // Blazing Fist Trigger Visual Off
				spellInfo->Effects[1].Effect = 0;
				break;
				//
				//
			case 115739: // Dashing Strike
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
				break;
			case 115629: // Flash of Steel
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
				spellInfo->Effects[0].SetRadiusIndex(EFFECT_RADIUS_12_YARDS);
				break;
			case 113134: // Scarlet Judicator Mass Resurrect
				spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_DEAD;
				spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
				spellInfo->Effects[EFFECT_0].SetRadiusIndex(EFFECT_RADIUS_150_YARDS);
				break;
			case 9232: // Scarlet Resurrect
				spellInfo->AttributesEx2 |= SPELL_ATTR2_CAN_TARGET_DEAD;
				spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ANY;
				spellInfo->Effects[1].TargetA = TARGET_UNIT_TARGET_ANY;
				break;
				//
				// Mobs
			case 115519: // Cleave
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
				spellInfo->Effects[0].TargetB = TARGET_UNIT_NEARBY_ENEMY;
				break;
			case 110953: // Hands of Purity Aura DMG
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
				break;
			case 110954: // Hands of Purity Trigger
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
				break;
			case 110956:  //Fanatical Strike
				spellInfo->Effects[1].Effect = 0;
				break;
			case 126843: // Handle Quest Spell " Unto Dust Thou Shalt Return " Heroic
				spellInfo->Effects[2].Effect = SPELL_EFFECT_QUEST_COMPLETE;
				spellInfo->Effects[2].BasePoints = 64838;
				spellInfo->Effects[2].MiscValue = 31516;
				spellInfo->Effects[2].TargetA = TARGET_UNIT_CASTER;
				spellInfo->Effects[2].TargetB = TARGET_SRC_CASTER;
				break;
			case 126787: // Handle Quest Spell " Unto Dust Thou Shalt Return " Normal
				spellInfo->Effects[2].Effect = SPELL_EFFECT_QUEST_COMPLETE;
				spellInfo->Effects[2].BasePoints = 64838;
				spellInfo->Effects[2].MiscValue = 31514;
				spellInfo->Effects[2].TargetA = TARGET_UNIT_CASTER;
				spellInfo->Effects[2].TargetB = TARGET_SRC_CASTER;
				break;
			case 42514: // Squash Soul (Pumpkin Spell of Headless Horseman Event)
				spellInfo->Effects[0].BasePoints = 50;
				spellInfo->Effects[0].ValueMultiplier = 50;
				spellInfo->Effects[0].MiscValue = 1;
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_CAN_STACK_FROM_DIFF_CASTERS;
				break;
			case 42380: // Conflagration
				spellInfo->DmgClass = SPELL_DAMAGE_CLASS_NONE;
				spellInfo->PreventionType = SPELL_PREVENTION_TYPE_NONE;
				spellInfo->AttributesEx4 |= SPELL_ATTR4_IGNORE_RESISTANCES;
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
				spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_PERIODIC_DAMAGE_PERCENT;
				spellInfo->Effects[1].BasePoints = 1;
				spellInfo->Effects[1].ValueMultiplier = 1;
				spellInfo->Effects[1].Mechanic = MECHANIC_NONE;
				spellInfo->Effects[1].ApplyAuraName = SPELL_AURA_MOD_BLIND;
				break;
			case 42381: // Conflagration Aura
				spellInfo->AttributesEx4 |= SPELL_ATTR4_TRIGGERED;
				spellInfo->AttributesEx &= ~SPELL_ATTR1_CANT_TARGET_SELF;
				spellInfo->AttributesEx4 |= SPELL_ATTR4_IGNORE_RESISTANCES;
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_TRIGGERED_IGNORE_RESILENCE;
				spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
				spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
				spellInfo->Effects[0].TargetB = TARGET_UNIT_DEST_AREA_ALLY;
				break;
			case 136538: //Conflagrated Weapon
				spellInfo->Effects[0].TargetA = TARGET_UNIT_TARGET_ENEMY;
				break;
			case 42394: //Pumpkin Missile
				spellInfo->Effects[1].Effect = 0;
				break;
			   //
				//
                // CLASS SPELLS
                //
            case 54934:
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 123040: // Mindbender
                break;
                // Shadow Apparition
            case 87426:
                spellInfo->Effects[0].TargetA = TARGET_UNIT_CASTER;
                break;
            case 73685: // Unleash Life 
                spellInfo->DmgClass = SPELL_DAMAGE_CLASS_MAGIC;
                break;
                // Death Grip
            case 49560:
            case 49576:
                spellInfo->SchoolMask = SPELL_SCHOOL_MASK_SHADOW;
                spellInfo->DmgClass = SPELL_DAMAGE_CLASS_MAGIC;
                spellInfo->Mechanic = MECHANIC_NONE;
                spellInfo->Effects[0].Mechanic = MECHANIC_NONE;
                break;
                // Circle of Healing
            case 34861:
                spellInfo->Effects[0].RadiusEntry = sSpellRadiusStore.LookupEntry(EFFECT_RADIUS_30_YARDS);
                spellInfo->Effects[0].TargetB = TARGET_UNK_119;
                break;
            case 1543: // Flare
                spellInfo->Effects[0].TriggerSpell = 94528;
                break;
			case 63106: // Siphon Life (heal)
				spellInfo->Effects[0].Effect = SPELL_EFFECT_HEAL; ///< Because the Pct used is 0.5 and we're working with integers
				spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
				break;
            default:
                break;
            }

            // Spells which don't reset periodic timers 
            switch (spellInfo->Id)
            {
                // Death Knight
            case 58844: // Blood Plague
            case 55095: // Frost Fever
            case 50536: // Unholy Blight
                // Druid
            case 44203: // Tranquility
            case 8921:  // Moonfire
            case 1822:  // Rake
            case 77758: // Thrash
            case 33745: // Lacerate
            case 8936:  // Regrowth
            case 1079:  // Rip
                // Hunter
            case 64468: // Piercing Shots
            case 1978:  // Serpent Sting
                // Mage
            case 12654: // Ignite
            case 11366: // Pyroblast
            case 92315: // Pyroblast!
            case 44457: // Living Bomb
                // Priest
            case 77489: // Echo of Light
            case 56161: // Glyph of Prayer of Healing
            case 139:   // Renew
                // Paladin
            case 31803: // Censure
                // Rogue
            case 2818:  // Deadly Poison
            case 73651: // Recuperate
            case 1943:  // Rupture
                // Shaman
            case 77661: // Searing Flame
            case 8050:  // Flame Shock
                // Warlock
            case 172:   // Corruption
            case 348:   // Immolate
            case 30108: // Unstable Corruption
            case 980:   // Agony
                // Warrior
            case 115767: // Deep Wounds
                // Items
            case 99002: // Fiery Claws, Item - Druid T12 Feral 2P Bonus
            case 99132: // Divine Fire, Item - Priest T12 Healer 2P Bonus
            case 99173: // Burning Wounds, Item - Rogue T12 2P Bonus
                spellInfo->AttributesCu |= SPELL_ATTR0_CU_DONT_RESET_PERIODIC_TIMER;
                break;
			case 86121:
			case 86213:
				spellInfo->AttributesEx |= SPELL_ATTR1_CANT_BE_REDIRECTED;
				spellInfo->AttributesEx |= SPELL_ATTR1_CANT_BE_REFLECTED;
            }

            // Spells which update last tick (only one tick)
            // switch (spellInfo->Id)
            // {
            // 
            // }

            switch (spellInfo->Id)
            {
            case 19386: // Wyvern Sting
            case 2637: // Hibernate
            case 339: // Entangling Roots
            case 335: // Freezing Trap
            case 118: // Polymorph
            case 20066: // Repentance
            case 9484: // Shackle Undead
            case 2094: // Blind
            case 51514: // Hex
            case 76780: // Shackle Elemental
            case 710: // Banish
            case 6358: // Seduction
                spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
                break;

            default:
                break;
            }

            switch (spellInfo->SpellFamilyName)
            {
            case SPELLFAMILY_SHAMAN:
                if (spellInfo->Id == 60103)
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
                break;
            case SPELLFAMILY_WARRIOR:
                // Shout
                if (spellInfo->SpellFamilyFlags[0] & 0x20000 || spellInfo->SpellFamilyFlags[1] & 0x20)
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_AURA_CC;
                // Dragon Roar
                if (spellInfo->Id == 118000)
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
                break;
            case SPELLFAMILY_DRUID:
                // Roar
                if (spellInfo->SpellFamilyFlags[0] & 0x8)
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_AURA_CC;
                // Rake
                if (spellInfo->Id == 1822)
                    spellInfo->AttributesCu |= SPELL_ATTR0_CU_IGNORE_ARMOR;
                break;
            default:
                break;
            }

            // This must be re-done if targets changed since the spellinfo load
            spellInfo->ExplicitTargetMask = spellInfo->_GetExplicitTargetMask();

            switch (spellInfo->Id)
            {
            case 73680: // Unleash Elements
                spellInfo->ExplicitTargetMask |= TARGET_FLAG_UNIT_ALLY;
                spellInfo->ExplicitTargetMask |= TARGET_FLAG_UNIT_ENEMY;
                break;
            case 107223:
                spellInfo->ExplicitTargetMask = TARGET_FLAG_UNIT_MASK;
                break;
            case 106736:
                spellInfo->ExplicitTargetMask = TARGET_FLAG_UNIT_MASK;
                break;
            case 106113:
                spellInfo->ExplicitTargetMask = TARGET_FLAG_UNIT_MASK;
                break;
            }

            SpellVisualEntry const *spellVisual = sSpellVisualEntryStore.LookupEntry(spellInfo->SpellVisual[0]);
            if (spellVisual)
                spellInfo->SpellVisual[1] = spellVisual->EnemyId;

            spellInfo->UpdateSpellEffectCount(); // Re-cache the maximum number of effects
        }
    }

    CreatureAI::FillAISpellInfo();

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded spell custom attributes in %u ms", GetMSTimeDiffToNow(oldMSTime));
}

void SpellMgr::LoadTalentSpellInfo()
{
    for (uint32 i = 0; i < sTalentStore.GetNumRows(); i++)
    {
        TalentEntry const* talent = sTalentStore.LookupEntry(i);
        if (!talent)
            continue;

        mTalentSpellInfo.insert(talent->spellId);
    }
}


const SpellInfo* SpellMgr::GetSpellInfo(uint32 spellId, Difficulty difficulty) const
{
    if (spellId < GetSpellInfoStoreSize())
    {
        if (mSpellInfoMap[difficulty][spellId])
            return mSpellInfoMap[difficulty][spellId];

        return mSpellInfoMap[REGULAR_DIFFICULTY][spellId];
    }

    return NULL;
}

void SpellMgr::LoadSpellPowerInfo()
{
    mSpellPowerInfo.resize(sSpellStore.GetNumRows());
    for (uint32 i = 0; i < sSpellPowerStore.GetNumRows(); i++)
    {
        SpellPowerEntry const* spellPower = sSpellPowerStore.LookupEntry(i);
        if (!spellPower)
            continue;

        mSpellPowerInfo[spellPower->SpellId].push_back(spellPower->Id);
    }
}

SpellPowerEntry const* SpellMgr::GetSpellPowerEntryByIdAndPower(uint32 id, Powers power) const
{
    for (auto itr : GetSpellPowerList(id))
    {
        SpellPowerEntry const* spellPower = sSpellPowerStore.LookupEntry(itr);
        if (!spellPower)
            continue;

        if (spellPower->powerType == power)
            return spellPower;
    }

    SpellInfo const* spell = sSpellMgr->GetSpellInfo(id);
    return spell->spellPower;
}

void SpellMgr::LoadBonusLootSpells()
{
    uint32 oldMSTime = getMSTime();

    mBonusLootSpells.clear();

    QueryResult result = WorldDatabase.Query("SELECT creatureEntry, currency, spellId, lootSourceType, lootSourceEntry FROM spell_bonus_loot");

    if (!result)
    {
        sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 spell bonus loot records. DB table `spell_bonus_loot` is empty.");

        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 creatureEntry = fields[0].GetUInt32();
        uint32 currencyId = fields[1].GetUInt32();
        uint32 spellId = fields[2].GetUInt32();
        uint32 lootSourceType = fields[3].GetUInt32();
        uint32 lootSourceEntry = fields[4].GetUInt32();

        if (mBonusLootSpells.find(creatureEntry) != mBonusLootSpells.end())
        {
            sLog->outError(LOG_FILTER_SQL, "creatureEntry %u in `spell_bonus_loot` table is already used, skipped", creatureEntry);
            continue;
        }

        if (!sCurrencyTypesStore.LookupEntry(currencyId))
        {
            sLog->outError(LOG_FILTER_SQL, "currencyId %u in `spell_bonus_loot` table is not exists in dbcs, skipped", currencyId);
            continue;
        }

        SpellInfo const* spellInfo = GetSpellInfo(spellId);
        if (!spellInfo)
        {
            sLog->outError(LOG_FILTER_SQL, "spellId %u in `spell_bonus_loot` table is not found in dbcs, skipped", spellId);
            continue;
        }

        if (lootSourceType < 0 || lootSourceType >= BONUS_LOOT_SOURCE_MAX)
        {
            sLog->outError(LOG_FILTER_SQL, "lootSourceType %u in `spell_bonus_loot` table is not supported, skipped", lootSourceType);
            continue;
        }

        BonusLootData& data = mBonusLootSpells[creatureEntry];
        data.creatureEntry = creatureEntry;
        data.requiredCurrency = currencyId;
        data.spellId = spellId;
        data.lootSourceType = (BonusLootSources)lootSourceType;
        data.lootSourceEntry = lootSourceEntry;
    }

    while (result->NextRow());

    sLog->outInfo(LOG_FILTER_SERVER_LOADING, ">> Loaded %u spell bonus loot records in %u ms", mBonusLootSpells.size(), GetMSTimeDiffToNow(oldMSTime));
}