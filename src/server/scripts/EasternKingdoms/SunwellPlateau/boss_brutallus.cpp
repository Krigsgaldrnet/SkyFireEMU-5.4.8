/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* ScriptData
SDName: Boss_Brutallus
SD%Complete: 80
SDComment: Find a way to start the intro, best code for the intro
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "sunwell_plateau.h"

enum Quotes
{
    YELL_INTRO                          = 0,
    YELL_INTRO_BREAK_ICE                = 1,
    YELL_INTRO_CHARGE                   = 2,
    YELL_INTRO_KILL_MADRIGOSA           = 3,
    YELL_INTRO_TAUNT                    = 4,

    YELL_AGGRO                          = 5,
    YELL_KILL                           = 6,
    YELL_LOVE                           = 7,
    YELL_BERSERK                        = 8,
    YELL_DEATH                          = 9,

    YELL_MADR_ICE_BARRIER               = 0,
    YELL_MADR_INTRO                     = 1,
    YELL_MADR_ICE_BLOCK                 = 2,
    YELL_MADR_TRAP                      = 3,
    YELL_MADR_DEATH                     = 4,
};

enum Spells
{
    SPELL_METEOR_SLASH                  = 45150,
    SPELL_BURN                          = 46394,
    SPELL_STOMP                         = 45185,
    SPELL_BERSERK                       = 26662,
    SPELL_DUAL_WIELD                    = 42459,

    SPELL_INTRO_FROST_BLAST             = 45203,
    SPELL_INTRO_FROSTBOLT               = 44843,
    SPELL_INTRO_ENCAPSULATE             = 45665,
    SPELL_INTRO_ENCAPSULATE_CHANELLING  = 45661
};

#define FELMYST 25038

class boss_brutallus : public CreatureScript
{
public:
    boss_brutallus() : CreatureScript("boss_brutallus") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_brutallusAI(creature);
    }

    struct boss_brutallusAI : public ScriptedAI
    {
        boss_brutallusAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            Intro = true;
        }

        InstanceScript* instance;

        uint32 SlashTimer;
        uint32 BurnTimer;
        uint32 StompTimer;
        uint32 BerserkTimer;

        uint32 IntroPhase;
        uint32 IntroPhaseTimer;
        uint32 IntroFrostBoltTimer;

        bool Intro;
        bool IsIntro;
        bool Enraged;

        void Reset() OVERRIDE
        {
            SlashTimer = 11000;
            StompTimer = 30000;
            BurnTimer = 60000;
            BerserkTimer = 360000;

            IntroPhase = 0;
            IntroPhaseTimer = 0;
            IntroFrostBoltTimer = 0;

            IsIntro = false;
            Enraged = false;

            DoCast(me, SPELL_DUAL_WIELD, true);

            if (instance)
                instance->SetData(DATA_BRUTALLUS_EVENT, NOT_STARTED);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            Talk(YELL_AGGRO);

            if (instance)
                instance->SetData(DATA_BRUTALLUS_EVENT, IN_PROGRESS);
        }

        void KilledUnit(Unit* /*victim*/) OVERRIDE
        {
            Talk(YELL_KILL);
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            Talk(YELL_DEATH);

            if (instance)
            {
                instance->SetData(DATA_BRUTALLUS_EVENT, DONE);
                float x, y, z;
                me->GetPosition(x, y, z);
                me->SummonCreature(FELMYST, x, y, z+30, me->GetOrientation(), TempSummonType::TEMPSUMMON_MANUAL_DESPAWN, 0);
            }
        }

        void EnterEvadeMode() OVERRIDE
        {
            if (!Intro)
                ScriptedAI::EnterEvadeMode();
        }

        void StartIntro()
        {
            if (!Intro || IsIntro)
                return;
            Creature* Madrigosa = Unit::GetCreature(*me, instance ? instance->GetData64(DATA_MADRIGOSA) : 0);
            if (Madrigosa)
            {
                Madrigosa->Respawn();
                Madrigosa->setActive(true);
                IsIntro = true;
                Madrigosa->SetMaxHealth(me->GetMaxHealth());
                Madrigosa->SetHealth(me->GetMaxHealth());
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->Attack(Madrigosa, true);
                Madrigosa->Attack(me, true);
            }
            else
            {
                //Madrigosa not found, end intro
                SF_LOG_ERROR("scripts", "Madrigosa was not found");
                EndIntro();
            }
        }

        void EndIntro()
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            Intro = false;
            IsIntro = false;
        }

        void AttackStart(Unit* who) OVERRIDE
        {
            if (!who || Intro || IsIntro)
                return;
            ScriptedAI::AttackStart(who);
        }

        void DoIntro()
        {
            Creature* Madrigosa = Unit::GetCreature(*me, instance ? instance->GetData64(DATA_MADRIGOSA) : 0);
            if (!Madrigosa)
                return;

            switch (IntroPhase)
            {
                case 0:
                    Madrigosa->AI()->Talk(YELL_MADR_ICE_BARRIER);
                    IntroPhaseTimer = 7000;
                    ++IntroPhase;
                    break;
                case 1:
                    me->SetInFront(Madrigosa);
                    Madrigosa->SetInFront(me);
                    Madrigosa->AI()->Talk(YELL_MADR_INTRO, me);
                    IntroPhaseTimer = 9000;
                    ++IntroPhase;
                    break;
                case 2:
                    Talk(YELL_INTRO, Madrigosa);
                    IntroPhaseTimer = 13000;
                    ++IntroPhase;
                    break;
                case 3:
                    DoCast(me, SPELL_INTRO_FROST_BLAST);
                    Madrigosa->SetDisableGravity(true);
                    me->AttackStop();
                    Madrigosa->AttackStop();
                    IntroFrostBoltTimer = 3000;
                    IntroPhaseTimer = 28000;
                    ++IntroPhase;
                    break;
                case 4:
                    Talk(YELL_INTRO_BREAK_ICE);
                    IntroPhaseTimer = 6000;
                    ++IntroPhase;
                    break;
                case 5:
                    Madrigosa->CastSpell(me, SPELL_INTRO_ENCAPSULATE_CHANELLING, false);
                    Madrigosa->AI()->Talk(YELL_MADR_TRAP);
                    DoCast(me, SPELL_INTRO_ENCAPSULATE);
                    IntroPhaseTimer = 11000;
                    ++IntroPhase;
                    break;
                case 6:
                    Talk(YELL_INTRO_CHARGE);
                    IntroPhaseTimer = 5000;
                    ++IntroPhase;
                    break;
                case 7:
                    me->Kill(Madrigosa);
                    Madrigosa->AI()->Talk(YELL_MADR_DEATH);
                    me->SetFullHealth();
                    me->AttackStop();
                    IntroPhaseTimer = 4000;
                    ++IntroPhase;
                    break;
                case 8:
                    Talk(YELL_INTRO_KILL_MADRIGOSA);
                    me->SetOrientation(0.14f);
                    me->StopMoving();
                    Madrigosa->setDeathState(DeathState::CORPSE);
                    IntroPhaseTimer = 8000;
                    ++IntroPhase;
                    break;
                case 9:
                    Talk(YELL_INTRO_TAUNT);
                    IntroPhaseTimer = 5000;
                    ++IntroPhase;
                    break;
                case 10:
                    EndIntro();
                    break;
            }
        }

        void MoveInLineOfSight(Unit* who) OVERRIDE

        {
            if (!me->IsValidAttackTarget(who))
                return;
            if (instance && Intro)
                instance->SetData(DATA_BRUTALLUS_EVENT, SPECIAL);

            if (Intro && !IsIntro)
                StartIntro();
            if (!Intro)
                ScriptedAI::MoveInLineOfSight(who);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (IsIntro)
            {
                if (IntroPhaseTimer <= diff)
                    DoIntro();
                else IntroPhaseTimer -= diff;

                if (IntroPhase == 3 + 1)
                {
                    if (IntroFrostBoltTimer <= diff)
                    {
                        if (Creature* Madrigosa = Unit::GetCreature(*me, instance ? instance->GetData64(DATA_MADRIGOSA) : 0))
                        {
                            Madrigosa->CastSpell(me, SPELL_INTRO_FROSTBOLT, true);
                            IntroFrostBoltTimer = 2000;
                        }
                    } else IntroFrostBoltTimer -= diff;
                }
                if (!UpdateVictim())
                    return;
                DoMeleeAttackIfReady();
            }

            if (!UpdateVictim() || IsIntro)
                return;

            if (SlashTimer <= diff)
            {
                DoCastVictim(SPELL_METEOR_SLASH);
                SlashTimer = 11000;
            } else SlashTimer -= diff;

            if (StompTimer <= diff)
            {
                Talk(YELL_LOVE);
                DoCastVictim(SPELL_STOMP);
                StompTimer = 30000;
            } else StompTimer -= diff;

            if (BurnTimer <= diff)
            {
                std::list<Unit*> targets;
                SelectTargetList(targets, 10, SELECT_TARGET_RANDOM, 100, true);
                for (std::list<Unit*>::const_iterator i = targets.begin(); i != targets.end(); ++i)
                    if (!(*i)->HasAura(SPELL_BURN))
                    {
                        (*i)->CastSpell((*i), SPELL_BURN, true);
                        break;
                    }
                BurnTimer = std::rand() % 180000 + 60000;
            } else BurnTimer -= diff;

            if (BerserkTimer < diff && !Enraged)
            {
                Talk(YELL_BERSERK);
                DoCast(me, SPELL_BERSERK);
                Enraged = true;
            } else BerserkTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_brutallus()
{
    new boss_brutallus();
}
