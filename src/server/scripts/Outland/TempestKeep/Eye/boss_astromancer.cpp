/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* ScriptData
SDName: Boss_Astromancer
SD%Complete: 80
SDComment:
SDCategory: Tempest Keep, The Eye
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

#include "the_eye.h"

enum Yells
{
    SAY_AGGRO                           = 0,
    SAY_SUMMON1                         = 1,
    SAY_SUMMON2                         = 2,
    SAY_KILL                            = 3,
    SAY_DEATH                           = 4,
    SAY_VOIDA                           = 5,
    SAY_VOIDB                           = 6
};

enum Spells
{
    SPELL_ARCANE_MISSILES               = 33031,
    SPELL_WRATH_OF_THE_ASTROMANCER      = 42783,
    SPELL_WRATH_OF_THE_ASTROMANCER_DOT  = 42784,
    SPELL_BLINDING_LIGHT                = 33009,
    SPELL_FEAR                          = 34322,
    SPELL_VOID_BOLT                     = 39329,

    SPELL_SPOTLIGHT                     = 25824,

    SPELL_SOLARIUM_GREAT_HEAL           = 33387,
    SPELL_SOLARIUM_HOLY_SMITE           = 25054,
    SPELL_SOLARIUM_ARCANE_TORRENT       = 33390
};

enum Creatures
{
    NPC_ASTROMANCER_SOLARIAN_SPOTLIGHT  = 18928,

    NPC_SOLARIUM_AGENT                  = 18925,
    NPC_SOLARIUM_PRIEST                 = 18806
};

enum Models
{
    MODEL_HUMAN                         = 18239,
    MODEL_VOIDWALKER                    = 18988
};

enum Misc
{
    WV_ARMOR                            = 31000
};

const float CENTER_X                    = 432.909f;
const float CENTER_Y                    = -373.424f;
const float CENTER_Z                    = 17.9608f;
const float CENTER_O                    = 1.06421f;
const float SMALL_PORTAL_RADIUS         = 12.6f;
const float LARGE_PORTAL_RADIUS         = 26.0f;
const float PORTAL_Z                    = 17.005f;

/* not used                        // x,          y,      z,         o
static float SolarianPos[4] = {432.909f, -373.424f, 17.9608f, 1.06421f};
*/

class boss_high_astromancer_solarian : public CreatureScript
{
    public:
        boss_high_astromancer_solarian() : CreatureScript("boss_high_astromancer_solarian") { }

        struct boss_high_astromancer_solarianAI : public ScriptedAI
        {
            boss_high_astromancer_solarianAI(Creature* creature) : ScriptedAI(creature), Summons(me)
            {
                instance = creature->GetInstanceScript();

                defaultarmor = creature->GetArmor();
                defaultsize = creature->GetObjectScale();
            }

            InstanceScript* instance;
            SummonList Summons;

            uint8 Phase;

            uint32 ArcaneMissiles_Timer;
            uint32 m_uiWrathOfTheAstromancer_Timer;
            uint32 BlindingLight_Timer;
            uint32 Fear_Timer;
            uint32 VoidBolt_Timer;
            uint32 Phase1_Timer;
            uint32 Phase2_Timer;
            uint32 Phase3_Timer;
            uint32 AppearDelay_Timer;
            uint32 defaultarmor;
            uint32 Wrath_Timer;

            float defaultsize;
            float Portals[3][3];

            bool AppearDelay;
            bool BlindingLight;

            void Reset() OVERRIDE
            {
                ArcaneMissiles_Timer = 2000;
                m_uiWrathOfTheAstromancer_Timer = 15000;
                BlindingLight_Timer = 41000;
                Fear_Timer = 20000;
                VoidBolt_Timer = 10000;
                Phase1_Timer = 50000;
                Phase2_Timer = 10000;
                Phase3_Timer = 15000;
                AppearDelay_Timer = 2000;
                BlindingLight = false;
                AppearDelay = false;
                Wrath_Timer = 20000+rand()%5000;//twice in phase one
                Phase = 1;

                if (instance)
                    instance->SetData(DATA_HIGHASTROMANCERSOLARIANEVENT, NOT_STARTED);

                me->SetArmor(defaultarmor);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetVisible(true);
                me->SetObjectScale(defaultsize);
                me->SetDisplayId(MODEL_HUMAN);

                Summons.DespawnAll();
            }

            void KilledUnit(Unit* /*victim*/) OVERRIDE
            {
                Talk(SAY_KILL);
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                me->SetObjectScale(defaultsize);
                me->SetDisplayId(MODEL_HUMAN);
                Talk(SAY_DEATH);
                if (instance)
                    instance->SetData(DATA_HIGHASTROMANCERSOLARIANEVENT, DONE);
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
                Talk(SAY_AGGRO);
                DoZoneInCombat();

                if (instance)
                    instance->SetData(DATA_HIGHASTROMANCERSOLARIANEVENT, IN_PROGRESS);
            }

            void SummonMinion(uint32 entry, float x, float y, float z)
            {
                Creature* Summoned = me->SummonCreature(entry, x, y, z, 0, TempSummonType::TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
                if (Summoned)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        Summoned->AI()->AttackStart(target);

                    Summons.Summon(Summoned);
                }
            }

            float Portal_X(float radius)
            {
                if (std::rand() % 1)
                    radius = -radius;

                return radius * (float)(rand()%100)/100.0f + CENTER_X;
            }

            float Portal_Y(float x, float radius)
            {
                float z = RAND(1.0f, -1.0f);

                return (z*sqrt(radius*radius - (x - CENTER_X)*(x - CENTER_X)) + CENTER_Y);
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;
                if (AppearDelay)
                {
                    me->StopMoving();
                    me->AttackStop();
                    if (AppearDelay_Timer <= diff)
                    {
                        AppearDelay = false;
                        if (Phase == 2)
                        {
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            me->SetVisible(false);
                        }
                        AppearDelay_Timer = 2000;
                    }
                    else
                        AppearDelay_Timer -= diff;
                }
                if (Phase == 1)
                {
                    if (BlindingLight_Timer <= diff)
                    {
                        BlindingLight = true;
                        BlindingLight_Timer = 45000;
                    }
                    else
                        BlindingLight_Timer -= diff;

                    if (Wrath_Timer <= diff)
                    {
                        me->InterruptNonMeleeSpells(false);
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 100, true))
                            DoCast(target, SPELL_WRATH_OF_THE_ASTROMANCER, true);
                        Wrath_Timer = 20000+rand()%5000;
                    }
                    else
                        Wrath_Timer -= diff;

                    if (ArcaneMissiles_Timer <= diff)
                    {
                        if (BlindingLight)
                        {
                            DoCastVictim(SPELL_BLINDING_LIGHT);
                            BlindingLight = false;
                        }
                        else
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            {
                                if (!me->HasInArc(2.5f, target))
                                    target = me->GetVictim();

                                DoCast(target, SPELL_ARCANE_MISSILES);
                            }
                        }
                        ArcaneMissiles_Timer = 3000;
                    }
                    else
                        ArcaneMissiles_Timer -= diff;

                    if (m_uiWrathOfTheAstromancer_Timer <= diff)
                    {
                        me->InterruptNonMeleeSpells(false);
                        //Target the tank ?
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                        {
                            if (target->GetTypeId() == TypeID::TYPEID_PLAYER)
                            {
                                DoCast(target, SPELL_WRATH_OF_THE_ASTROMANCER);
                                m_uiWrathOfTheAstromancer_Timer = 25000;
                            }
                            else
                                m_uiWrathOfTheAstromancer_Timer = 1000;
                        }
                    }
                    else
                        m_uiWrathOfTheAstromancer_Timer -= diff;

                    //Phase1_Timer
                    if (Phase1_Timer <= diff)
                    {
                        Phase = 2;
                        Phase1_Timer = 50000;
                        //After these 50 seconds she portals to the middle of the room and disappears, leaving 3 light portals behind.
                        me->GetMotionMaster()->Clear();
                        me->SetPosition(CENTER_X, CENTER_Y, CENTER_Z, CENTER_O);
                        for (uint8 i=0; i <= 2; ++i)
                        {
                            if (!i)
                            {
                                Portals[i][0] = Portal_X(SMALL_PORTAL_RADIUS);
                                Portals[i][1] = Portal_Y(Portals[i][0], SMALL_PORTAL_RADIUS);
                                Portals[i][2] = CENTER_Z;
                            }
                            else
                            {
                                Portals[i][0] = Portal_X(LARGE_PORTAL_RADIUS);
                                Portals[i][1] = Portal_Y(Portals[i][0], LARGE_PORTAL_RADIUS);
                                Portals[i][2] = PORTAL_Z;
                            }
                        }
                        if ((std::abs(Portals[2][0] - Portals[1][0]) < 7) && (std::abs(Portals[2][1] - Portals[1][1]) < 7))
                        {
                            int i=1;
                            if (std::abs(CENTER_X + 26.0f - Portals[2][0]) < 7)
                                i = -1;
                            Portals[2][0] = Portals[2][0]+7*i;
                            Portals[2][1] = Portal_Y(Portals[2][0], LARGE_PORTAL_RADIUS);
                        }
                        for (int i=0; i <= 2; ++i)
                        {
                            if (Creature* Summoned = me->SummonCreature(NPC_ASTROMANCER_SOLARIAN_SPOTLIGHT, Portals[i][0], Portals[i][1], Portals[i][2], CENTER_O, TempSummonType::TEMPSUMMON_TIMED_DESPAWN, Phase2_Timer+Phase3_Timer+AppearDelay_Timer+1700))
                            {
                                Summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                Summoned->CastSpell(Summoned, SPELL_SPOTLIGHT, false);
                            }
                        }
                        AppearDelay = true;
                    }
                    else
                        Phase1_Timer-=diff;
                }
                else
                    if (Phase == 2)
                    {
                        //10 seconds after Solarian disappears, 12 mobs spawn out of the three portals.
                        me->AttackStop();
                        me->StopMoving();
                        if (Phase2_Timer <= diff)
                        {
                            Phase = 3;
                            for (int i=0; i <= 2; ++i)
                                for (int j=1; j <= 4; j++)
                                    SummonMinion(NPC_SOLARIUM_AGENT, Portals[i][0], Portals[i][1], Portals[i][2]);

                            Talk(SAY_SUMMON1);
                            Phase2_Timer = 10000;
                        }
                        else
                            Phase2_Timer -= diff;
                    }
                    else
                        if (Phase == 3)
                        {
                            me->AttackStop();
                            me->StopMoving();
                            //Check Phase3_Timer
                            if (Phase3_Timer <= diff)
                            {
                                Phase = 1;
                                //15 seconds later Solarian reappears out of one of the 3 portals. Simultaneously, 2 healers appear in the two other portals.
                                int i = rand()%3;
                                me->GetMotionMaster()->Clear();
                                me->SetPosition(Portals[i][0], Portals[i][1], Portals[i][2], CENTER_O);

                                for (int j=0; j <= 2; j++)
                                    if (j != i)
                                        SummonMinion(NPC_SOLARIUM_PRIEST, Portals[j][0], Portals[j][1], Portals[j][2]);

                                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                me->SetVisible(true);

                                Talk(SAY_SUMMON2);
                                AppearDelay = true;
                                Phase3_Timer = 15000;
                            }
                            else
                                Phase3_Timer -= diff;
                        }
                        else
                            if (Phase == 4)
                            {
                                //Fear_Timer
                                if (Fear_Timer <= diff)
                                {
                                    DoCast(me, SPELL_FEAR);
                                    Fear_Timer = 20000;
                                }
                                else
                                    Fear_Timer -= diff;
                                //VoidBolt_Timer
                                if (VoidBolt_Timer <= diff)
                                {
                                    DoCastVictim(SPELL_VOID_BOLT);
                                    VoidBolt_Timer = 10000;
                                }
                                else
                                    VoidBolt_Timer -= diff;
                            }
                            //When Solarian reaches 20% she will transform into a huge void walker.
                            if (Phase != 4 && me->HealthBelowPct(20))
                            {
                                Phase = 4;
                                //To make sure she wont be invisible or not selecatble
                                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                me->SetVisible(true);
                                Talk(SAY_VOIDA);
                                Talk(SAY_VOIDB);
                                me->SetArmor(WV_ARMOR);
                                me->SetDisplayId(MODEL_VOIDWALKER);
                                me->SetObjectScale(defaultsize*2.5f);
                            }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new boss_high_astromancer_solarianAI(creature);
        }
};

class npc_solarium_priest : public CreatureScript
{
    public:
        npc_solarium_priest() : CreatureScript("npc_solarium_priest") { }

        struct npc_solarium_priestAI : public ScriptedAI
        {
            npc_solarium_priestAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            uint32 healTimer;
            uint32 holysmiteTimer;
            uint32 aoesilenceTimer;

            void Reset() OVERRIDE
            {
                healTimer = 9000;
                holysmiteTimer = 1;
                aoesilenceTimer = 15000;
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                if (healTimer <= diff)
                {
                    Unit* target = NULL;
                    switch (std::rand() % 1)
                    {
                        case 0:
                            if (instance)
                                target = Unit::GetUnit(*me, instance->GetData64(DATA_ASTROMANCER));
                            break;
                        case 1:
                            target = me;
                            break;
                    }

                    if (target)
                    {
                        DoCast(target, SPELL_SOLARIUM_GREAT_HEAL);
                        healTimer = 9000;
                    }
                }
                else
                    healTimer -= diff;

                if (holysmiteTimer <= diff)
                {
                    DoCastVictim(SPELL_SOLARIUM_HOLY_SMITE);
                    holysmiteTimer = 4000;
                }
                else
                    holysmiteTimer -= diff;

                if (aoesilenceTimer <= diff)
                {
                    DoCastVictim(SPELL_SOLARIUM_ARCANE_TORRENT);
                    aoesilenceTimer = 13000;
                }
                else
                    aoesilenceTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_solarium_priestAI(creature);
        }
};

class spell_astromancer_wrath_of_the_astromancer : public SpellScriptLoader
{
    public:
        spell_astromancer_wrath_of_the_astromancer() : SpellScriptLoader("spell_astromancer_wrath_of_the_astromancer") { }

        class spell_astromancer_wrath_of_the_astromancer_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_astromancer_wrath_of_the_astromancer_AuraScript);

            bool Validate(SpellInfo const* /*spellInfo*/) OVERRIDE
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_WRATH_OF_THE_ASTROMANCER_DOT))
                    return false;
                return true;
            }

            void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                // Final heal only on duration end
                if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
                    return;

                Unit* target = GetUnitOwner();
                target->CastSpell(target, GetSpellInfo()->Effects[EFFECT_1].CalcValue(), false);
            }

            void Register() OVERRIDE
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_astromancer_wrath_of_the_astromancer_AuraScript::AfterRemove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const OVERRIDE
        {
            return new spell_astromancer_wrath_of_the_astromancer_AuraScript();
        }
};

void AddSC_boss_high_astromancer_solarian()
{
    new boss_high_astromancer_solarian();
    new npc_solarium_priest();
    new spell_astromancer_wrath_of_the_astromancer();
}

