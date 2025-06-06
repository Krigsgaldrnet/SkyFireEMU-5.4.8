/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/*
 * Spells used in holidays/game events that do not fit any other category.
 * Ordered alphabetically using scriptname.
 * Scriptnames in this file should be prefixed with "spell_#holidayname_".
 */

#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

// 45102 Romantic Picnic
enum SpellsPicnic
{
    SPELL_BASKET_CHECK              = 45119, // Holiday - Valentine - Romantic Picnic Near Basket Check
    SPELL_MEAL_PERIODIC             = 45103, // Holiday - Valentine - Romantic Picnic Meal Periodic - effect dummy
    SPELL_MEAL_EAT_VISUAL           = 45120, // Holiday - Valentine - Romantic Picnic Meal Eat Visual
    //SPELL_MEAL_PARTICLE             = 45114, // Holiday - Valentine - Romantic Picnic Meal Particle - unused
    SPELL_DRINK_VISUAL              = 45121, // Holiday - Valentine - Romantic Picnic Drink Visual
    SPELL_ROMANTIC_PICNIC_ACHIEV    = 45123, // Romantic Picnic periodic = 5000
};

class spell_love_is_in_the_air_romantic_picnic : public SpellScriptLoader
{
public:
    spell_love_is_in_the_air_romantic_picnic() : SpellScriptLoader("spell_love_is_in_the_air_romantic_picnic") { }

    class spell_love_is_in_the_air_romantic_picnic_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_love_is_in_the_air_romantic_picnic_AuraScript);

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            target->SetStandState(UNIT_STAND_STATE_SIT);
            target->CastSpell(target, SPELL_MEAL_PERIODIC, false);
        }

        void OnPeriodic(AuraEffect const* /*aurEff*/)
        {
            // Every 5 seconds
            Unit* target = GetTarget();
            Unit* caster = GetCaster();

            // If our player is no longer sit, remove all auras
            if (target->getStandState() != UNIT_STAND_STATE_SIT)
            {
                target->RemoveAura(SPELL_ROMANTIC_PICNIC_ACHIEV);
                target->RemoveAura(GetAura());
                return;
            }

            target->CastSpell(target, SPELL_BASKET_CHECK, false); // unknown use, it targets Romantic Basket
            target->CastSpell(target, RAND(SPELL_MEAL_EAT_VISUAL, SPELL_DRINK_VISUAL), false);

            bool foundSomeone = false;
            // For nearby players, check if they have the same aura. If so, cast Romantic Picnic (45123)
            // required by achievement and "hearts" visual
            std::list<Player*> playerList;
            Skyfire::AnyPlayerInObjectRangeCheck checker(target, INTERACTION_DISTANCE * 2);
            Skyfire::PlayerListSearcher<Skyfire::AnyPlayerInObjectRangeCheck> searcher(target, playerList, checker);
            target->VisitNearbyWorldObject(INTERACTION_DISTANCE * 2, searcher);
            for (std::list<Player*>::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
            {
                if ((*itr) != target && (*itr)->HasAura(GetId())) // && (*itr)->getStandState() == UNIT_STAND_STATE_SIT)
                {
                    if (caster)
                    {
                        caster->CastSpell(*itr, SPELL_ROMANTIC_PICNIC_ACHIEV, true);
                        caster->CastSpell(target, SPELL_ROMANTIC_PICNIC_ACHIEV, true);
                    }
                    foundSomeone = true;
                    // break;
                }
            }

            if (!foundSomeone && target->HasAura(SPELL_ROMANTIC_PICNIC_ACHIEV))
                target->RemoveAura(SPELL_ROMANTIC_PICNIC_ACHIEV);
        }

        void Register() OVERRIDE
        {
            AfterEffectApply += AuraEffectApplyFn(spell_love_is_in_the_air_romantic_picnic_AuraScript::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_love_is_in_the_air_romantic_picnic_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const OVERRIDE
    {
        return new spell_love_is_in_the_air_romantic_picnic_AuraScript();
    }
};

// 24750 Trick
enum TrickSpells
{
    SPELL_PIRATE_COSTUME_MALE = 24708,
    SPELL_PIRATE_COSTUME_FEMALE = 24709,
    SPELL_NINJA_COSTUME_MALE = 24710,
    SPELL_NINJA_COSTUME_FEMALE = 24711,
    SPELL_LEPER_GNOME_COSTUME_MALE = 24712,
    SPELL_LEPER_GNOME_COSTUME_FEMALE = 24713,
    SPELL_SKELETON_COSTUME = 24723,
    SPELL_GHOST_COSTUME_MALE = 24735,
    SPELL_GHOST_COSTUME_FEMALE = 24736,
    SPELL_TRICK_BUFF = 24753,
};

class spell_hallow_end_trick : public SpellScriptLoader
{
public:
    spell_hallow_end_trick() : SpellScriptLoader("spell_hallow_end_trick") { }

    class spell_hallow_end_trick_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_hallow_end_trick_SpellScript);

        bool Validate(SpellInfo const* /*spell*/) OVERRIDE
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_PIRATE_COSTUME_MALE) || !sSpellMgr->GetSpellInfo(SPELL_PIRATE_COSTUME_FEMALE) || !sSpellMgr->GetSpellInfo(SPELL_NINJA_COSTUME_MALE)
                || !sSpellMgr->GetSpellInfo(SPELL_NINJA_COSTUME_FEMALE) || !sSpellMgr->GetSpellInfo(SPELL_LEPER_GNOME_COSTUME_MALE) || !sSpellMgr->GetSpellInfo(SPELL_LEPER_GNOME_COSTUME_FEMALE)
                || !sSpellMgr->GetSpellInfo(SPELL_SKELETON_COSTUME) || !sSpellMgr->GetSpellInfo(SPELL_GHOST_COSTUME_MALE) || !sSpellMgr->GetSpellInfo(SPELL_GHOST_COSTUME_FEMALE) || !sSpellMgr->GetSpellInfo(SPELL_TRICK_BUFF))
                return false;
            return true;
        }

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (Player* target = GetHitPlayer())
            {
                uint8 gender = target->getGender();
                uint32 spellId = SPELL_TRICK_BUFF;
                switch (std::rand() % 5)
                {
                case 1:
                    spellId = gender ? SPELL_LEPER_GNOME_COSTUME_FEMALE : SPELL_LEPER_GNOME_COSTUME_MALE;
                    break;
                case 2:
                    spellId = gender ? SPELL_PIRATE_COSTUME_FEMALE : SPELL_PIRATE_COSTUME_MALE;
                    break;
                case 3:
                    spellId = gender ? SPELL_GHOST_COSTUME_FEMALE : SPELL_GHOST_COSTUME_MALE;
                    break;
                case 4:
                    spellId = gender ? SPELL_NINJA_COSTUME_FEMALE : SPELL_NINJA_COSTUME_MALE;
                    break;
                case 5:
                    spellId = SPELL_SKELETON_COSTUME;
                    break;
                default:
                    break;
                }

                caster->CastSpell(target, spellId, true);
            }
        }

        void Register() OVERRIDE
        {
            OnEffectHitTarget += SpellEffectFn(spell_hallow_end_trick_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const OVERRIDE
    {
        return new spell_hallow_end_trick_SpellScript();
    }
};

// 24751 Trick or Treat
enum TrickOrTreatSpells
{
    SPELL_TRICK = 24714,
    SPELL_TREAT = 24715,
    SPELL_TRICKED_OR_TREATED = 24755,
    SPELL_TRICKY_TREAT_SPEED = 42919,
    SPELL_TRICKY_TREAT_TRIGGER = 42965,
    SPELL_UPSET_TUMMY = 42966
};

class spell_hallow_end_trick_or_treat : public SpellScriptLoader
{
public:
    spell_hallow_end_trick_or_treat() : SpellScriptLoader("spell_hallow_end_trick_or_treat") { }

    class spell_hallow_end_trick_or_treat_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_hallow_end_trick_or_treat_SpellScript);

        bool Validate(SpellInfo const* /*spell*/) OVERRIDE
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_TRICK) || !sSpellMgr->GetSpellInfo(SPELL_TREAT) || !sSpellMgr->GetSpellInfo(SPELL_TRICKED_OR_TREATED))
                return false;
            return true;
        }

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (Player* target = GetHitPlayer())
            {
                caster->CastSpell(target, roll_chance_i(50) ? SPELL_TRICK : SPELL_TREAT, true);
                caster->CastSpell(target, SPELL_TRICKED_OR_TREATED, true);
            }
        }

        void Register() OVERRIDE
        {
            OnEffectHitTarget += SpellEffectFn(spell_hallow_end_trick_or_treat_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const OVERRIDE
    {
        return new spell_hallow_end_trick_or_treat_SpellScript();
    }
};

class spell_hallow_end_tricky_treat : public SpellScriptLoader
{
public:
    spell_hallow_end_tricky_treat() : SpellScriptLoader("spell_hallow_end_tricky_treat") { }

    class spell_hallow_end_tricky_treat_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_hallow_end_tricky_treat_SpellScript);

        bool Validate(SpellInfo const* /*spell*/) OVERRIDE
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_TRICKY_TREAT_SPEED))
                return false;
            if (!sSpellMgr->GetSpellInfo(SPELL_TRICKY_TREAT_TRIGGER))
                return false;
            if (!sSpellMgr->GetSpellInfo(SPELL_UPSET_TUMMY))
                return false;
            return true;
        }

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (caster->HasAura(SPELL_TRICKY_TREAT_TRIGGER) && caster->GetAuraCount(SPELL_TRICKY_TREAT_SPEED) > 3 && roll_chance_i(33))
                caster->CastSpell(caster, SPELL_UPSET_TUMMY, true);
        }

        void Register() OVERRIDE
        {
            OnEffectHitTarget += SpellEffectFn(spell_hallow_end_tricky_treat_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const OVERRIDE
    {
        return new spell_hallow_end_tricky_treat_SpellScript();
    }
};

enum Mistletoe
{
    SPELL_CREATE_MISTLETOE = 26206,
    SPELL_CREATE_HOLLY = 26207,
    SPELL_CREATE_SNOWFLAKES = 45036
};

class spell_winter_veil_mistletoe : public SpellScriptLoader
{
public:
    spell_winter_veil_mistletoe() : SpellScriptLoader("spell_winter_veil_mistletoe") { }

    class spell_winter_veil_mistletoe_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_winter_veil_mistletoe_SpellScript);

        bool Validate(SpellInfo const* /*spell*/) OVERRIDE
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_CREATE_MISTLETOE) ||
                !sSpellMgr->GetSpellInfo(SPELL_CREATE_HOLLY) ||
                !sSpellMgr->GetSpellInfo(SPELL_CREATE_SNOWFLAKES))
                return false;
            return true;
        }

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            if (Player* target = GetHitPlayer())
            {
                uint32 spellId = RAND(SPELL_CREATE_HOLLY, SPELL_CREATE_MISTLETOE, SPELL_CREATE_SNOWFLAKES);
                GetCaster()->CastSpell(target, spellId, true);
            }
        }

        void Register() OVERRIDE
        {
            OnEffectHitTarget += SpellEffectFn(spell_winter_veil_mistletoe_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const OVERRIDE
    {
        return new spell_winter_veil_mistletoe_SpellScript();
    }
};

// 26275 - PX-238 Winter Wondervolt TRAP
enum PX238WinterWondervolt
{
    SPELL_PX_238_WINTER_WONDERVOLT_TRANSFORM_1 = 26157,
    SPELL_PX_238_WINTER_WONDERVOLT_TRANSFORM_2 = 26272,
    SPELL_PX_238_WINTER_WONDERVOLT_TRANSFORM_3 = 26273,
    SPELL_PX_238_WINTER_WONDERVOLT_TRANSFORM_4 = 26274
};

class spell_winter_veil_px_238_winter_wondervolt : public SpellScriptLoader
{
public:
    spell_winter_veil_px_238_winter_wondervolt() : SpellScriptLoader("spell_winter_veil_px_238_winter_wondervolt") { }

    class spell_winter_veil_px_238_winter_wondervolt_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_winter_veil_px_238_winter_wondervolt_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) OVERRIDE
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_PX_238_WINTER_WONDERVOLT_TRANSFORM_1) ||
                !sSpellMgr->GetSpellInfo(SPELL_PX_238_WINTER_WONDERVOLT_TRANSFORM_2) ||
                !sSpellMgr->GetSpellInfo(SPELL_PX_238_WINTER_WONDERVOLT_TRANSFORM_3) ||
                !sSpellMgr->GetSpellInfo(SPELL_PX_238_WINTER_WONDERVOLT_TRANSFORM_4))
                return false;
            return true;
        }

        void HandleScript(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);

            uint32 const spells[4] =
            {
                SPELL_PX_238_WINTER_WONDERVOLT_TRANSFORM_1,
                SPELL_PX_238_WINTER_WONDERVOLT_TRANSFORM_2,
                SPELL_PX_238_WINTER_WONDERVOLT_TRANSFORM_3,
                SPELL_PX_238_WINTER_WONDERVOLT_TRANSFORM_4
            };

            if (Unit* target = GetHitUnit())
            {
                for (uint8 i = 0; i < 4; ++i)
                    if (target->HasAura(spells[i]))
                        return;

                target->CastSpell(target, spells[std::rand() % 3], true);
            }
        }

        void Register() OVERRIDE
        {
            OnEffectHitTarget += SpellEffectFn(spell_winter_veil_px_238_winter_wondervolt_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const OVERRIDE
    {
        return new spell_winter_veil_px_238_winter_wondervolt_SpellScript();
    }
};

void AddSC_holiday_spell_scripts()
{
    // Love is in the Air
    new spell_love_is_in_the_air_romantic_picnic();
    // Hallow's End
    new spell_hallow_end_trick();
    new spell_hallow_end_trick_or_treat();
    new spell_hallow_end_tricky_treat();
    // Winter Veil
    new spell_winter_veil_mistletoe();
    new spell_winter_veil_px_238_winter_wondervolt();
}
