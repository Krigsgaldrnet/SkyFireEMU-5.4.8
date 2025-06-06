/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* ScriptData
SDName: Boss_High_King_Maulgar
SD%Complete: 90
SDComment: Correct timers, after whirlwind melee attack bug, prayer of healing
SDCategory: Gruul's Lair
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "gruuls_lair.h"

enum HighKingMaulgar
{
    SAY_AGGRO                   = 0,
    SAY_ENRAGE                  = 1,
    SAY_OGRE_DEATH              = 2,
    SAY_SLAY                    = 3,
    SAY_DEATH                   = 4,

    // High King Maulgar
    SPELL_ARCING_SMASH          = 39144,
    SPELL_MIGHTY_BLOW           = 33230,
    SPELL_WHIRLWIND             = 33238,
    SPELL_BERSERKER_C           = 26561,
    SPELL_ROAR                  = 16508,
    SPELL_FLURRY                = 33232,
    SPELL_DUAL_WIELD            = 29651,

    // Olm the Summoner
    SPELL_DARK_DECAY            = 33129,
    SPELL_DEATH_COIL            = 33130,
    SPELL_SUMMON_WFH            = 33131,

    // Kiggler the Craed
    SPELL_GREATER_POLYMORPH     = 33173,
    SPELL_LIGHTNING_BOLT        = 36152,
    SPELL_ARCANE_SHOCK          = 33175,
    SPELL_ARCANE_EXPLOSION      = 33237,

    // Blindeye the Seer
    SPELL_GREATER_PW_SHIELD     = 33147,
    SPELL_HEAL                  = 33144,
    SPELL_PRAYER_OH             = 33152,

    // Krosh Firehand
    SPELL_GREATER_FIREBALL      = 33051,
    SPELL_SPELLSHIELD           = 33054,
    SPELL_BLAST_WAVE            = 33061,

    ACTION_ADD_DEATH            = 1
};

class boss_high_king_maulgar : public CreatureScript
{
public:
    boss_high_king_maulgar() : CreatureScript("boss_high_king_maulgar") { }

    struct boss_high_king_maulgarAI : public ScriptedAI
    {
        boss_high_king_maulgarAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            ArcingSmash_Timer = 0;
            MightyBlow_Timer = 0;
            Whirlwind_Timer = 0;
            Charging_Timer = 0;
            Roar_Timer = 0;
            Phase2 = false;
        }

        void Reset() OVERRIDE
        {
            ArcingSmash_Timer = 10000;
            MightyBlow_Timer = 40000;
            Whirlwind_Timer = 30000;
            Charging_Timer = 0;
            Roar_Timer = 0;

            DoCast(me, SPELL_DUAL_WIELD, false);

            Phase2 = false;

            instance->SetBossState(DATA_MAULGAR, NOT_STARTED);
        }

        void KilledUnit(Unit* /*victim*/) OVERRIDE
        {
            Talk(SAY_SLAY);
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            Talk(SAY_DEATH);

            instance->SetBossState(DATA_MAULGAR, DONE);
        }

        void DoAction(int32 actionId) OVERRIDE
        {
            if (actionId == ACTION_ADD_DEATH)
                Talk(SAY_OGRE_DEATH);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            DoZoneInCombat();
            instance->SetBossState(DATA_MAULGAR, IN_PROGRESS);
            Talk(SAY_AGGRO);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            //ArcingSmash_Timer
            if (ArcingSmash_Timer <= diff)
            {
                DoCastVictim(SPELL_ARCING_SMASH);
                ArcingSmash_Timer = 10000;
            } else ArcingSmash_Timer -= diff;

            //Whirlwind_Timer
                   if (Whirlwind_Timer <= diff)
                   {
                        DoCastVictim(SPELL_WHIRLWIND);
                        Whirlwind_Timer = 55000;
                   } else Whirlwind_Timer -= diff;

            //MightyBlow_Timer
            if (MightyBlow_Timer <= diff)
            {
                DoCastVictim(SPELL_MIGHTY_BLOW);
                MightyBlow_Timer = 30000+rand()%10000;
            } else MightyBlow_Timer -= diff;

            //Entering Phase 2
            if (!Phase2 && HealthBelowPct(50))
            {
                Phase2 = true;
                Talk(SAY_ENRAGE);

                DoCast(me, SPELL_DUAL_WIELD, true);
                me->SetUInt32Value(UNIT_FIELD_VIRTUAL_ITEM_ID, 0);
                me->SetUInt32Value(UNIT_FIELD_VIRTUAL_ITEM_ID+1, 0);
            }

            if (Phase2)
            {
                //Charging_Timer
                if (Charging_Timer <= diff)
                {
                    Unit* target = NULL;
                    target = SelectTarget(SELECT_TARGET_RANDOM, 0);
                    if (target)
                    {
                        AttackStart(target);
                        DoCast(target, SPELL_BERSERKER_C);
                    }
                    Charging_Timer = 20000;
                } else Charging_Timer -= diff;

                //Intimidating Roar
                if (Roar_Timer <= diff)
                {
                    DoCast(me, SPELL_ROAR);
                    Roar_Timer = 40000+(rand()%10000);
                } else Roar_Timer -= diff;
            }

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        uint32 ArcingSmash_Timer;
        uint32 MightyBlow_Timer;
        uint32 Whirlwind_Timer;
        uint32 Charging_Timer;
        uint32 Roar_Timer;
        bool Phase2;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return GetGruulsLairAI<boss_high_king_maulgarAI>(creature);
    }
};

class boss_olm_the_summoner : public CreatureScript
{
public:
    boss_olm_the_summoner() : CreatureScript("boss_olm_the_summoner") { }

    struct boss_olm_the_summonerAI : public ScriptedAI
    {
        boss_olm_the_summonerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            DarkDecay_Timer = 0;
            Summon_Timer = 0;
            DeathCoil_Timer = 0;
        }

        void Reset() OVERRIDE
        {
            DarkDecay_Timer = 10000;
            Summon_Timer = 15000;
            DeathCoil_Timer = 20000;

            instance->SetBossState(DATA_MAULGAR, NOT_STARTED);
        }

        void AttackStart(Unit* who) OVERRIDE
        {
            if (!who)
                return;

            if (me->Attack(who, true))
            {
                me->AddThreat(who, 0.0f);
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);

                me->GetMotionMaster()->MoveChase(who, 30.0f);
            }
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            DoZoneInCombat();
            instance->SetBossState(DATA_MAULGAR, IN_PROGRESS);
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            if (Creature* maulgar = ObjectAccessor::GetCreature(*me, instance->GetData64(DATA_MAULGAR)))
                maulgar->AI()->DoAction(ACTION_ADD_DEATH);

            instance->SetBossState(DATA_MAULGAR, DONE);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            //DarkDecay_Timer
            if (DarkDecay_Timer <= diff)
            {
                DoCastVictim(SPELL_DARK_DECAY);
                DarkDecay_Timer = 20000;
            } else DarkDecay_Timer -= diff;

            //Summon_Timer
            if (Summon_Timer <= diff)
            {
                DoCast(me, SPELL_SUMMON_WFH);
                Summon_Timer = 30000;
            } else Summon_Timer -= diff;

            //DeathCoil Timer /need correct timer
            if (DeathCoil_Timer <= diff)
            {
                Unit* target = NULL;
                target = SelectTarget(SELECT_TARGET_RANDOM, 0);
                if (target)
                    DoCast(target, SPELL_DEATH_COIL);
                DeathCoil_Timer = 20000;
            } else DeathCoil_Timer -= diff;

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        uint32 DarkDecay_Timer;
        uint32 Summon_Timer;
        uint32 DeathCoil_Timer;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return GetGruulsLairAI<boss_olm_the_summonerAI>(creature);
    }
};

//Kiggler The Crazed AI
class boss_kiggler_the_crazed : public CreatureScript
{
public:
    boss_kiggler_the_crazed() : CreatureScript("boss_kiggler_the_crazed") { }

    struct boss_kiggler_the_crazedAI : public ScriptedAI
    {
        boss_kiggler_the_crazedAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            GreaterPolymorph_Timer = 0;
            LightningBolt_Timer = 0;
            ArcaneShock_Timer = 0;
            ArcaneExplosion_Timer = 0;
        }

        void Reset() OVERRIDE
        {
            GreaterPolymorph_Timer = 5000;
            LightningBolt_Timer = 10000;
            ArcaneShock_Timer = 20000;
            ArcaneExplosion_Timer = 30000;

            instance->SetBossState(DATA_MAULGAR, NOT_STARTED);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            DoZoneInCombat();
            instance->SetBossState(DATA_MAULGAR, IN_PROGRESS);
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            if (Creature* maulgar = ObjectAccessor::GetCreature(*me, instance->GetData64(DATA_MAULGAR)))
                maulgar->AI()->DoAction(ACTION_ADD_DEATH);

            instance->SetBossState(DATA_MAULGAR, DONE);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            //GreaterPolymorph_Timer
            if (GreaterPolymorph_Timer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    DoCast(target, SPELL_GREATER_POLYMORPH);

                GreaterPolymorph_Timer = std::rand() % 20000 + 15000;
            } else GreaterPolymorph_Timer -= diff;

            //LightningBolt_Timer
            if (LightningBolt_Timer <= diff)
            {
                DoCastVictim(SPELL_LIGHTNING_BOLT);
                LightningBolt_Timer = 15000;
            } else LightningBolt_Timer -= diff;

            //ArcaneShock_Timer
            if (ArcaneShock_Timer <= diff)
            {
                DoCastVictim(SPELL_ARCANE_SHOCK);
                ArcaneShock_Timer = 20000;
            } else ArcaneShock_Timer -= diff;

            //ArcaneExplosion_Timer
            if (ArcaneExplosion_Timer <= diff)
            {
                DoCastVictim(SPELL_ARCANE_EXPLOSION);
                ArcaneExplosion_Timer = 30000;
            } else ArcaneExplosion_Timer -= diff;

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        uint32 GreaterPolymorph_Timer;
        uint32 LightningBolt_Timer;
        uint32 ArcaneShock_Timer;
        uint32 ArcaneExplosion_Timer;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return GetGruulsLairAI<boss_kiggler_the_crazedAI>(creature);
    }
};

class boss_blindeye_the_seer : public CreatureScript
{
public:
    boss_blindeye_the_seer() : CreatureScript("boss_blindeye_the_seer") { }

    struct boss_blindeye_the_seerAI : public ScriptedAI
    {
        boss_blindeye_the_seerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            GreaterPowerWordShield_Timer = 0;
            Heal_Timer = 0;
            PrayerofHealing_Timer = 0;
        }

        void Reset() OVERRIDE
        {
            GreaterPowerWordShield_Timer = 5000;
            Heal_Timer = std::rand() % 40000 + 25000;
            PrayerofHealing_Timer = std::rand() % 55000 + 45000;

            instance->SetBossState(DATA_MAULGAR, NOT_STARTED);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            DoZoneInCombat();
            instance->SetBossState(DATA_MAULGAR, IN_PROGRESS);
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            if (Creature* maulgar = ObjectAccessor::GetCreature(*me, instance->GetData64(DATA_MAULGAR)))
                maulgar->AI()->DoAction(ACTION_ADD_DEATH);

            instance->SetBossState(DATA_MAULGAR, DONE);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            //GreaterPowerWordShield_Timer
            if (GreaterPowerWordShield_Timer <= diff)
            {
                DoCast(me, SPELL_GREATER_PW_SHIELD);
                GreaterPowerWordShield_Timer = 40000;
            } else GreaterPowerWordShield_Timer -= diff;

            //Heal_Timer
            if (Heal_Timer <= diff)
            {
                DoCast(me, SPELL_HEAL);
                Heal_Timer = std::rand() % 40000 + 15000;
            } else Heal_Timer -= diff;

            //PrayerofHealing_Timer
            if (PrayerofHealing_Timer <= diff)
            {
                DoCast(me, SPELL_PRAYER_OH);
                PrayerofHealing_Timer = std::rand() % 50000 + 35000;
            } else PrayerofHealing_Timer -= diff;

            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        uint32 GreaterPowerWordShield_Timer;
        uint32 Heal_Timer;
        uint32 PrayerofHealing_Timer;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return GetGruulsLairAI<boss_blindeye_the_seerAI>(creature);
    }
};

class boss_krosh_firehand : public CreatureScript
{
public:
    boss_krosh_firehand() : CreatureScript("boss_krosh_firehand") { }

    struct boss_krosh_firehandAI : public ScriptedAI
    {
        boss_krosh_firehandAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            GreaterFireball_Timer = 0;
            SpellShield_Timer = 0;
            BlastWave_Timer = 0;
        }

        void Reset() OVERRIDE
        {
            GreaterFireball_Timer = 1000;
            SpellShield_Timer = 5000;
            BlastWave_Timer = 20000;

            instance->SetBossState(DATA_MAULGAR, NOT_STARTED);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            DoZoneInCombat();
            instance->SetBossState(DATA_MAULGAR, IN_PROGRESS);
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            if (Creature* maulgar = ObjectAccessor::GetCreature(*me, instance->GetData64(DATA_MAULGAR)))
                maulgar->AI()->DoAction(ACTION_ADD_DEATH);

            instance->SetBossState(DATA_MAULGAR, DONE);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            //GreaterFireball_Timer
            if (GreaterFireball_Timer < diff || me->IsWithinDist(me->GetVictim(), 30))
            {
                DoCastVictim(SPELL_GREATER_FIREBALL);
                GreaterFireball_Timer = 2000;
            } else GreaterFireball_Timer -= diff;

            //SpellShield_Timer
            if (SpellShield_Timer <= diff)
            {
                me->InterruptNonMeleeSpells(false);
                DoCastVictim(SPELL_SPELLSHIELD);
                SpellShield_Timer = 30000;
            } else SpellShield_Timer -= diff;

            //BlastWave_Timer
            if (BlastWave_Timer <= diff)
            {
                Unit* target = NULL;
                std::list<HostileReference*> t_list = me->getThreatManager().getThreatList();
                std::vector<Unit*> target_list;
                for (std::list<HostileReference*>::const_iterator itr = t_list.begin(); itr!= t_list.end(); ++itr)
                {
                    target = Unit::GetUnit(*me, (*itr)->getUnitGuid());
                                                                //15 yard radius minimum
                    if (target && target->IsWithinDist(me, 15, false))
                        target_list.push_back(target);
                    target = NULL;
                }
                if (!target_list.empty())
                    target = *(target_list.begin()+rand()%target_list.size());

                me->InterruptNonMeleeSpells(false);
                DoCast(target, SPELL_BLAST_WAVE);
                BlastWave_Timer = 60000;
            } else BlastWave_Timer -= diff;
        }

    private:
        InstanceScript* instance;
        uint32 GreaterFireball_Timer;
        uint32 SpellShield_Timer;
        uint32 BlastWave_Timer;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return GetGruulsLairAI<boss_krosh_firehandAI>(creature);
    }
};

void AddSC_boss_high_king_maulgar()
{
    new boss_high_king_maulgar();
    new boss_kiggler_the_crazed();
    new boss_blindeye_the_seer();
    new boss_olm_the_summoner();
    new boss_krosh_firehand();
}
