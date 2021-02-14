/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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

/*
 * Scripts for spells with MASTERY.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_mastery_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

enum MasterySpells
{
    MASTERY_SPELL_LIGHTNING_BOLT        = 45284,
    MASTERY_SPELL_CHAIN_LIGHTNING       = 45297,
    MASTERY_SPELL_LAVA_BURST            = 77451,
    MASTERY_SPELL_ELEMENTAL_BLAST       = 120588,
    MASTERY_SPELL_HAND_OF_LIGHT         = 96172,
    MASTERY_SPELL_IGNITE                = 12654,
    MASTERY_SPELL_BLOOD_SHIELD          = 77535,
    MASTERY_SPELL_DISCIPLINE_SHIELD     = 77484,
    SPELL_DK_SCENT_OF_BLOOD             = 50421,
    SPELL_MAGE_MASTERY_ICICLES          = 76613,
    SPELL_MAGE_ICICLE_DAMAGE            = 148022,
    SPELL_MAGE_ICICLE_PERIODIC_TRIGGER  = 148023,
    SPELL_PRIEST_ECHO_OF_LIGHT          = 77489
};

// Called by Power Word : Shield - 17, Power Word : Shield (Divine Insight) - 123258, Spirit Shell - 114908, Angelic Bulwark - 114214 and Divine Aegis - 47753
// Mastery : Shield Discipline - 77484
class spell_mastery_shield_discipline : public SpellScriptLoader
{
    public:
        spell_mastery_shield_discipline() : SpellScriptLoader("spell_mastery_shield_discipline") { }

        class spell_mastery_shield_discipline_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mastery_shield_discipline_AuraScript);

            void CalculateAmount(constAuraEffectPtr , int32 & amount, bool & )
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->HasAura(MASTERY_SPELL_DISCIPLINE_SHIELD) && caster->getLevel() >= 80)
                    {
                        float Mastery = 1 + (caster->GetFloatValue(PLAYER_MASTERY) * 1.6f / 100.0f);
                        amount = int32(amount * Mastery);
                    }
                }
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mastery_shield_discipline_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mastery_shield_discipline_AuraScript();
        }
};

// Called by 45470 - Death Strike (Heal)
// 77513 - Mastery : Blood Shield
class spell_mastery_blood_shield : public SpellScriptLoader
{
    public:
        spell_mastery_blood_shield() : SpellScriptLoader("spell_mastery_blood_shield") { }

        class spell_mastery_blood_shield_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mastery_blood_shield_SpellScript);

            void HandleAfterHit()
            {
                if (Player* _plr = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (_plr->GetTypeId() == TYPEID_PLAYER && _plr->HasAura(77513) && _plr->getLevel() >= 80)
                        {
                            // Check the Mastery aura while in Blood presence
                            if (_plr->HasAura(77513) && _plr->HasAura(48263))
                            {
                                float Mastery = _plr->GetFloatValue(PLAYER_MASTERY) * 6.25f / 100.0f;

                                int32 bp = -int32(GetHitDamage() * Mastery);

                                if (AuraPtr scentOfBlood = _plr->GetAura(SPELL_DK_SCENT_OF_BLOOD))
                                    AddPct(bp, (scentOfBlood->GetStackAmount() * 20));

                                if (AuraEffectPtr bloodShield = target->GetAuraEffect(MASTERY_SPELL_BLOOD_SHIELD, EFFECT_0))
                                    bp += bloodShield->GetAmount();

                                bp = std::min(uint32(bp), target->GetMaxHealth());

                                _plr->CastCustomSpell(target, MASTERY_SPELL_BLOOD_SHIELD, &bp, NULL, NULL, true);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_mastery_blood_shield_SpellScript::HandleAfterHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mastery_blood_shield_SpellScript();
        }
};

// Called by 133 - Fireball, 44614 - Frostfire Bolt, 108853 - Inferno Blast, 2948 - Scorch and 11366 - Pyroblast
// 12846 - Mastery : Ignite
class spell_mastery_ignite : public SpellScriptLoader
{
    public:
        spell_mastery_ignite() : SpellScriptLoader("spell_mastery_ignite") { }

        class spell_mastery_ignite_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mastery_ignite_SpellScript);

            void HandleAfterHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (caster->GetTypeId() == TYPEID_PLAYER && caster->HasAura(12846) && caster->getLevel() >= 80)
                        {
                            uint32 procSpellId = GetSpellInfo()->Id ? GetSpellInfo()->Id : 0;
                            if (procSpellId != MASTERY_SPELL_IGNITE)
                            {
                                float value = caster->GetFloatValue(PLAYER_MASTERY) * 1.5f / 100.0f;

                                int32 bp = int32(GetHitDamage() * value / 2);

                                if (target->HasAura(MASTERY_SPELL_IGNITE, caster->GetGUID()))
                                    bp += target->GetRemainingPeriodicAmount(caster->GetGUID(), MASTERY_SPELL_IGNITE, SPELL_AURA_PERIODIC_DAMAGE);

                                caster->CastCustomSpell(target, MASTERY_SPELL_IGNITE, &bp, NULL, NULL, true);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_mastery_ignite_SpellScript::HandleAfterHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mastery_ignite_SpellScript();
        }
};

// Called by 35395 - Crusader Strike, 53595 - Hammer of the Righteous, 24275 - Hammer of Wrath, 85256 - Templar's Verdict and 53385 - Divine Storm
// 76672 - Mastery : Hand of Light
class spell_mastery_hand_of_light : public SpellScriptLoader
{
    public:
        spell_mastery_hand_of_light() : SpellScriptLoader("spell_mastery_hand_of_light") { }

        class spell_mastery_hand_of_light_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mastery_hand_of_light_SpellScript);

            void HandleAfterHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (caster->GetTypeId() == TYPEID_PLAYER && caster->GetGUID() != target->GetGUID()&& caster->HasAura(76672) && caster->getLevel() >= 80)
                        {
                            uint32 procSpellId = GetSpellInfo()->Id ? GetSpellInfo()->Id : 0;
                            if (procSpellId != MASTERY_SPELL_HAND_OF_LIGHT)
                            {
                                float value = caster->GetFloatValue(PLAYER_MASTERY) * 2.10f / 100.0f;

                                int32 bp = int32(GetHitDamage() * value);

                                // Need to recalculate if damage is absorbed
                                if (bp == 0)
                                {
                                    int32 absorbedDamage = int32(GetAbsorbedDamage() * value);
                                    // If spell didn't hit, absorbedDamage will be random negative value, because of it players receive 1kk+ heal/damage from it.
                                    // 100000 - just in case, to prevent bug with high damage, because it's unreal to deal ~666k damage (666k*0.15%=100k).
                                    if (absorbedDamage > 0 && absorbedDamage < 100000)
                                        caster->CastCustomSpell(target, MASTERY_SPELL_HAND_OF_LIGHT, &absorbedDamage, NULL, NULL, true);
                                }
                                else
                                    caster->CastCustomSpell(target, MASTERY_SPELL_HAND_OF_LIGHT, &bp, NULL, NULL, true);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_mastery_hand_of_light_SpellScript::HandleAfterHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mastery_hand_of_light_SpellScript();
        }
};

const int IcicleAuras[5] = { 148012, 148013, 148014, 148015, 148016 };
const int IcicleHits[5] = { 148017, 148018, 148019, 148020, 148021 };

/// Called by Frostbolt - 116 and Frostfire bolt - 44614
/// Mastery: Icicles - 76613
class spell_mastery_icicles : public SpellScriptLoader
{
public:
	spell_mastery_icicles() : SpellScriptLoader("spell_mastery_icicles") { }

	class spell_mastery_icicles_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_mastery_icicles_SpellScript);

		void HandleOnHit()
		{
			if (Player* l_Player = GetCaster()->ToPlayer())
			{
				if (Unit* l_Target = GetHitUnit())
				{
					if (l_Player->GetTypeId() == TYPEID_PLAYER && l_Player->HasAura(76613) && l_Player->getLevel() >= 80)
					{
						/// Calculate damage
						int32 l_HitDamage = GetHitDamage();
						if (GetSpell()->IsCritForTarget(l_Target))
							l_HitDamage *= 2;

						/// if l_HitDamage == 0 we have a miss, so we need to except this variant
						if (l_Player->HasAura(MasterySpells::SPELL_MAGE_MASTERY_ICICLES) && l_HitDamage != 0)
						{
							l_HitDamage *= (l_Player->GetFloatValue(EPlayerFields::PLAYER_MASTERY) * 2.0f) / 100.0f;

							/// Prevent huge hits on player after hitting low level creatures
							if (l_Player->getLevel() > l_Target->getLevel())
								l_HitDamage = std::min(uint32(l_HitDamage), l_Target->GetMaxHealth());

							/// We need to get the first free icicle slot
							int8 l_IcicleFreeSlot = -1; ///< -1 means no free slot
							for (int8 l_I = 0; l_I < 5; ++l_I)
							{
								if (!l_Player->HasAura(IcicleAuras[l_I]))
								{
									l_IcicleFreeSlot = l_I;
									break;
								}
							}

							switch (l_IcicleFreeSlot)
							{
							case -1:
							{
									   // We need to find the icicle with the smallest duration.
									   int8 l_SmallestIcicle = 0;
									   int32 l_MinDuration = 0xFFFFFF;
									   for (int8 i = 0; i < 5; i++)
									   {
										   if (AuraPtr l_TmpCurrentAura = l_Player->GetAura(IcicleAuras[i]))
										   {
											   if (l_MinDuration > l_TmpCurrentAura->GetDuration())
											   {
												   l_MinDuration = l_TmpCurrentAura->GetDuration();
												   l_SmallestIcicle = i;
											   }
										   }
									   }

									   /// Launch the icicle with the smallest duration
									   if (AuraEffectPtr l_CurrentIcicleAuraEffect = l_Player->GetAuraEffect(IcicleAuras[l_SmallestIcicle], EFFECT_0))
									   {
										   int32 l_BasePoints = l_CurrentIcicleAuraEffect->GetAmount();
										   l_Player->CastSpell(l_Target, IcicleHits[l_SmallestIcicle], true);
										   l_Player->CastCustomSpell(l_Target, SPELL_MAGE_ICICLE_DAMAGE, &l_BasePoints, NULL, NULL, true);
										   l_Player->RemoveAura(IcicleAuras[l_SmallestIcicle]);
									   }

									   l_IcicleFreeSlot = l_SmallestIcicle;
									   /// No break because we'll add the icicle in the next case
							}
							case 0:
							case 1:
							case 2:
							case 3:
							case 4:
							{
									  if (AuraPtr l_CurrentIcicleAura = l_Player->AddAura(IcicleAuras[l_IcicleFreeSlot], l_Player))
									  {
										  if (AuraEffectPtr l_Effect = l_CurrentIcicleAura->GetEffect(EFFECT_0))
											  l_Effect->SetAmount(l_HitDamage);
									  }

									  break;
							}
							}
						}
					}
				}
			}
		}

		void Register()
		{
			OnHit += SpellHitFn(spell_mastery_icicles_SpellScript::HandleOnHit);
		}
	};

	SpellScript* GetSpellScript() const
	{
		return new spell_mastery_icicles_SpellScript();
	}
};

/// Ice Lance - 30455
class spell_mastery_icicles_trigger : public SpellScriptLoader
{
public:
	spell_mastery_icicles_trigger() : SpellScriptLoader("spell_mastery_icicles_trigger") { }

	class spell_mastery_icicles_trigger_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_mastery_icicles_trigger_SpellScript);

		void HandleAfterHit()
		{
			if (Unit* l_Caster = GetCaster())
			{
				if (Unit* l_HitUnit = GetHitUnit())
				{
					if (l_HitUnit->isAlive())
					{
						l_Caster->SetIciclesTarget(l_HitUnit->GetGUID());
						l_Caster->CastSpell(l_Caster, SPELL_MAGE_ICICLE_PERIODIC_TRIGGER, true);
					}
				}
			}
		}

		void Register()
		{
			AfterHit += SpellHitFn(spell_mastery_icicles_trigger_SpellScript::HandleAfterHit);
		}
	};

	SpellScript* GetSpellScript() const
	{
		return new spell_mastery_icicles_trigger_SpellScript();
	}
};

/// Icicles (periodic) - 148023
class spell_mastery_icicles_periodic : public SpellScriptLoader
{
public:
	spell_mastery_icicles_periodic() : SpellScriptLoader("spell_mastery_icicles_periodic") { }

	class spell_mastery_icicles_periodic_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_mastery_icicles_periodic_AuraScript);

		uint32 m_Icicles[5];
		int32 m_IcicleCount;

		void OnApply(constAuraEffectPtr /*l_AurEff*/, AuraEffectHandleModes /*l_Mode*/)
		{
			m_IcicleCount = 0;
			if (Unit* l_Caster = GetCaster())
			{
				for (uint32 l_I = 0; l_I < 5; ++l_I)
				{
					if (AuraPtr l_Icicle = l_Caster->GetAura(IcicleAuras[l_I]))
						m_Icicles[m_IcicleCount++] = IcicleAuras[l_I];
				}
			}
		}

		void OnTick(constAuraEffectPtr aurEff)
		{
			if (Unit* l_Caster = GetCaster())
			{
				if (AuraEffectPtr l_Aura = l_Caster->GetAuraEffect(GetSpellInfo()->Id, EFFECT_0))
				{
					// Maybe not the good target selection ...
					if (Unit* l_Target = ObjectAccessor::GetUnit(*l_Caster, l_Caster->GetIciclesTarget()))
					{
						if (l_Target->isAlive())
						{
							int32 l_Amount = l_Aura->GetAmount();
							if (AuraPtr l_CurrentIcicleAura = l_Caster->GetAura(m_Icicles[l_Amount]))
							{
								int32 l_BasePoints = l_CurrentIcicleAura->GetEffect(0)->GetAmount();

								l_Caster->CastSpell(l_Target, IcicleHits[l_Amount], true);
								l_Caster->CastCustomSpell(l_Target, SPELL_MAGE_ICICLE_DAMAGE, &l_BasePoints, NULL, NULL, true);
								l_Caster->RemoveAura(IcicleAuras[l_Amount]);
							}

							if (++l_Amount >= m_IcicleCount)
								l_Caster->RemoveAura(l_Aura->GetBase());
							else
								l_Aura->SetAmount(l_Amount);
						}
						else
							l_Caster->RemoveAura(l_Aura->GetBase());
					}
				}
			}
		}

		void Register()
		{
			OnEffectApply += AuraEffectApplyFn(spell_mastery_icicles_periodic_AuraScript::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
			OnEffectPeriodic += AuraEffectPeriodicFn(spell_mastery_icicles_periodic_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
		}
	};

	AuraScript* GetAuraScript() const
	{
		return new spell_mastery_icicles_periodic_AuraScript();
	}
};

// Mastery: Echo of Light - 77485
class spell_mastery_echo_of_light : public SpellScriptLoader
{
    public:
        spell_mastery_echo_of_light() : SpellScriptLoader("spell_mastery_echo_of_light") { }

        class spell_mastery_echo_of_light_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mastery_echo_of_light_AuraScript);

            void OnProc(constAuraEffectPtr aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();

                if (!GetCaster())
                    return;

                if (!eventInfo.GetHealInfo() || !eventInfo.GetHealInfo()->GetHeal())
                    return;

                Unit* unitTarget = eventInfo.GetActionTarget();
                Player* plr = GetCaster()->ToPlayer();
                if (!unitTarget || !plr)
                    return;

                float Mastery = plr->GetFloatValue(PLAYER_MASTERY) * 1.30f / 100.0f;
                int32 bp = (Mastery * eventInfo.GetHealInfo()->GetHeal()) / 6;

                bp += unitTarget->GetRemainingPeriodicAmount(plr->GetGUID(), SPELL_PRIEST_ECHO_OF_LIGHT, SPELL_AURA_PERIODIC_HEAL);
                plr->CastCustomSpell(unitTarget, SPELL_PRIEST_ECHO_OF_LIGHT, &bp, NULL, NULL, true);
            }

            void Register()
            {
                OnEffectProc += AuraEffectProcFn(spell_mastery_echo_of_light_AuraScript::OnProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mastery_echo_of_light_AuraScript();
        }
};

void AddSC_mastery_spell_scripts()
{
    new spell_mastery_shield_discipline();
    new spell_mastery_blood_shield();
    new spell_mastery_ignite();
    new spell_mastery_hand_of_light();
    new spell_mastery_icicles();
    new spell_mastery_icicles_trigger();
    new spell_mastery_icicles_periodic();
  //  new spell_mastery_icicles_hit();
    new spell_mastery_echo_of_light();
}
