/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* Script Data Start
SDName: Boss palehoof
SDAuthor: LordVanMartin
SD%Complete:
SDComment:
SDCategory:
Script Data End */

#include <random>
#include <algorithm>
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "utgarde_pinnacle.h"

enum Spells
{
    SPELL_ARCING_SMASH                          = 48260,
    SPELL_IMPALE                                = 48261,
    H_SPELL_IMPALE                              = 59268,
    SPELL_WITHERING_ROAR                        = 48256,
    H_SPELL_WITHERING_ROAR                      = 59267,
    SPELL_FREEZE                                = 16245
};

//Orb spells
enum OrbSpells
{
    SPELL_ORB_VISUAL                            = 48044,
    SPELL_ORB_CHANNEL                           = 48048
};

//not in db
enum Yells
{
    SAY_AGGRO                                = 0,
    SAY_SLAY                                 = 1
  //SAY_DEATH                                = 2 Missing in database
};

enum Creatures
{
    NPC_STASIS_CONTROLLER                       = 26688
};

struct Locations
{
    float x, y, z;
};

struct Locations moveLocs[]=
{
    {261.6f, -449.3f, 109.5f},
    {263.3f, -454.0f, 109.5f},
    {291.5f, -450.4f, 109.5f},
    {291.5f, -454.0f, 109.5f},
    {310.0f, -453.4f, 109.5f},
    {238.6f, -460.7f, 109.5f}
};

enum Phase
{
    PHASE_FRENZIED_WORGEN,
    PHASE_RAVENOUS_FURLBORG,
    PHASE_MASSIVE_JORMUNGAR,
    PHASE_FEROCIOUS_RHINO,
    PHASE_GORTOK_PALEHOOF,
    PHASE_NONE
};

class boss_palehoof : public CreatureScript
{
public:
    boss_palehoof() : CreatureScript("boss_palehoof") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_palehoofAI(creature);
    }

    struct boss_palehoofAI : public ScriptedAI
    {
        boss_palehoofAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        uint32 uiArcingSmashTimer;
        uint32 uiImpaleTimer;
        uint32 uiWhiteringRoarTimer;
        uint32 uiWaitingTimer;
        Phase currentPhase;
        uint8 AddCount;
        Phase Sequence[5];

        InstanceScript* instance;

        void Reset() OVERRIDE
        {
            /// There is a good reason to store them like this, we are going to shuffle the order.
            for (uint32 i = PHASE_FRENZIED_WORGEN; i < PHASE_NONE; ++i)
                Sequence[i] = Phase(i);

            std::vector<int> v = { Sequence[0], Sequence[1], Sequence[2], Sequence[3], Sequence[4] };
            std::random_device rd;
            std::mt19937 g(rd());

            /// This ensures a random order and only executes each phase once.
            std::shuffle(v.begin(), v.end(), g);

            uiArcingSmashTimer = 15000;
            uiImpaleTimer = 12000;
            uiWhiteringRoarTimer = 10000;

            me->GetMotionMaster()->MoveTargetedHome();

            AddCount = 0;

            currentPhase = PHASE_NONE;

            if (instance)
            {
                instance->SetData(DATA_GORTOK_PALEHOOF_EVENT, NOT_STARTED);

                Creature* temp = Unit::GetCreature((*me), instance->GetData64(DATA_NPC_FRENZIED_WORGEN));
                if (temp && !temp->IsAlive())
                    temp->Respawn();

                temp = Unit::GetCreature((*me), instance->GetData64(DATA_NPC_FEROCIOUS_RHINO));
                if (temp && !temp->IsAlive())
                    temp->Respawn();

                temp = Unit::GetCreature((*me), instance->GetData64(DATA_NPC_MASSIVE_JORMUNGAR));
                if (temp && !temp->IsAlive())
                    temp->Respawn();

                temp = Unit::GetCreature((*me), instance->GetData64(DATA_NPC_RAVENOUS_FURBOLG));
                if (temp && !temp->IsAlive())
                    temp->Respawn();

                GameObject* go = instance->instance->GetGameObject(instance->GetData64(DATA_GORTOK_PALEHOOF_SPHERE));
                if (go)
                {
                    go->SetGoState(GOState::GO_STATE_READY);
                    go->RemoveFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
                }
            }
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            Talk(SAY_AGGRO);
        }

        void AttackStart(Unit* who) OVERRIDE
        {
            if (!who)
                return;

            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                return;

            if (me->Attack(who, true))
            {
                me->AddThreat(who, 0.0f);
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);
                DoStartMovement(who);
            }
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (currentPhase != PHASE_GORTOK_PALEHOOF)
                return;

            //Return since we have no target
            if (!UpdateVictim())
                return;

            Creature* temp = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_NPC_ORB) : 0);
            if (temp && temp->IsAlive())
                temp->DisappearAndDie();

            if (uiArcingSmashTimer <= diff)
            {
                DoCast(me, SPELL_ARCING_SMASH);
                uiArcingSmashTimer = std::rand() % 17000 + 13000;
            } else uiArcingSmashTimer -= diff;

            if (uiImpaleTimer <= diff)
            {
              if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                  DoCast(target, SPELL_IMPALE);
              uiImpaleTimer = std::rand() % 12000 + 8000;
            } else uiImpaleTimer -= diff;

            if (uiWhiteringRoarTimer <= diff)
            {
                DoCast(me, SPELL_WITHERING_ROAR);
                uiWhiteringRoarTimer = std::rand() % 12000 + 8000;
            } else uiWhiteringRoarTimer -= diff;

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
          //Talk(SAY_DEATH);
            if (instance)
                instance->SetData(DATA_GORTOK_PALEHOOF_EVENT, DONE);
            Creature* temp = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_NPC_ORB) : 0);
            if (temp && temp->IsAlive())
                temp->DisappearAndDie();
        }

        void KilledUnit(Unit* /*victim*/) OVERRIDE
        {
            Talk(SAY_SLAY);
        }

        void NextPhase()
        {
            if (currentPhase == PHASE_NONE)
            {
                if (instance)
                    instance->SetData(DATA_GORTOK_PALEHOOF_EVENT, IN_PROGRESS);

                me->SummonCreature(NPC_STASIS_CONTROLLER, moveLocs[5].x, moveLocs[5].y, moveLocs[5].z, 0, TempSummonType::TEMPSUMMON_CORPSE_DESPAWN);
            }
            Phase move = PHASE_NONE;
            if (AddCount >= DUNGEON_MODE(2, 4))
                move = PHASE_GORTOK_PALEHOOF;
            else
                move = Sequence[AddCount++];
            //send orb to summon spot
            Creature* pOrb = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_NPC_ORB) : 0);
            if (pOrb && pOrb->IsAlive())
            {
                if (currentPhase == PHASE_NONE)
                    pOrb->CastSpell(me, SPELL_ORB_VISUAL, true);
                pOrb->GetMotionMaster()->MovePoint(move, moveLocs[move].x, moveLocs[move].y, moveLocs[move].z);
            }
            currentPhase = move;
        }

        void JustReachedHome() OVERRIDE
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NOT_ATTACKABLE_1|UNIT_FLAG_IMMUNE_TO_PC);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            DoCast(me, SPELL_FREEZE);
        }
    };
};

//ravenous furbolg's spells
enum RavenousSpells
{
    SPELL_CHAIN_LIGHTING                        = 48140,
    H_SPELL_CHAIN_LIGHTING                      = 59273,
    SPELL_CRAZED                                = 48139,
    SPELL_TERRIFYING_ROAR                       = 48144
};

class npc_ravenous_furbolg : public CreatureScript
{
public:
    npc_ravenous_furbolg() : CreatureScript("npc_ravenous_furbolg") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_ravenous_furbolgAI(creature);
    }

    struct npc_ravenous_furbolgAI : public ScriptedAI
    {
        npc_ravenous_furbolgAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        uint32 uiChainLightingTimer;
        uint32 uiCrazedTimer;
        uint32 uiTerrifyingRoarTimer;

        InstanceScript* instance;

        void Reset() OVERRIDE
        {
            uiChainLightingTimer = 5000;
            uiCrazedTimer = 10000;
            uiTerrifyingRoarTimer = 15000;

            me->GetMotionMaster()->MoveTargetedHome();

            if (instance)
                if (instance->GetData(DATA_GORTOK_PALEHOOF_EVENT) == IN_PROGRESS)
                {
                    Creature* pPalehoof = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_GORTOK_PALEHOOF) : 0);
                    if (pPalehoof && pPalehoof->IsAlive())
                        CAST_AI(boss_palehoof::boss_palehoofAI, pPalehoof->AI())->Reset();
                }
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            if (uiChainLightingTimer <= diff)
            {
                DoCastVictim(SPELL_CHAIN_LIGHTING);
                uiChainLightingTimer = 5000 + rand() % 5000;
            } else uiChainLightingTimer -=  diff;

            if (uiCrazedTimer <= diff)
            {
                DoCast(me, SPELL_CRAZED);
                uiCrazedTimer = 8000 + rand() % 4000;
            } else uiCrazedTimer -=  diff;

            if (uiTerrifyingRoarTimer <= diff)
            {
                DoCast(me, SPELL_TERRIFYING_ROAR);
                uiTerrifyingRoarTimer = 10000 + rand() % 10000;
            } else uiTerrifyingRoarTimer -=  diff;

            DoMeleeAttackIfReady();
        }

        void AttackStart(Unit* who) OVERRIDE
        {
            if (!who)
                return;

            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                return;

            if (me->Attack(who, true))
            {
                me->AddThreat(who, 0.0f);
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);
                DoStartMovement(who);
            }
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            if (instance)
            {
                Creature* pPalehoof = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_GORTOK_PALEHOOF) : 0);
                if (pPalehoof)
                    CAST_AI(boss_palehoof::boss_palehoofAI, pPalehoof->AI())->NextPhase();
            }
        }

        void JustReachedHome() OVERRIDE
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            DoCast(me, SPELL_FREEZE);
        }
    };
};

//frenzied worgen's spells
enum FrenziedSpells
{
    SPELL_MORTAL_WOUND                          = 48137,
    H_SPELL_MORTAL_WOUND                        = 59265,
    SPELL_ENRAGE_1                              = 48138,
    SPELL_ENRAGE_2                              = 48142
};

class npc_frenzied_worgen : public CreatureScript
{
public:
    npc_frenzied_worgen() : CreatureScript("npc_frenzied_worgen") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_frenzied_worgenAI(creature);
    }

    struct npc_frenzied_worgenAI : public ScriptedAI
    {
        npc_frenzied_worgenAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        uint32 uiMortalWoundTimer;
        uint32 uiEnrage1Timer;
        uint32 uiEnrage2Timer;

        InstanceScript* instance;

        void Reset() OVERRIDE
        {
            uiMortalWoundTimer = 5000;
            uiEnrage1Timer = 15000;
            uiEnrage2Timer = 10000;

            me->GetMotionMaster()->MoveTargetedHome();

            if (instance)
                if (instance->GetData(DATA_GORTOK_PALEHOOF_EVENT) == IN_PROGRESS)
                {
                    Creature* pPalehoof = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_GORTOK_PALEHOOF) : 0);
                    if (pPalehoof && pPalehoof->IsAlive())
                        CAST_AI(boss_palehoof::boss_palehoofAI, pPalehoof->AI())->Reset();
                }
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            if (uiMortalWoundTimer <= diff)
            {
                DoCastVictim(SPELL_MORTAL_WOUND);
                uiMortalWoundTimer = 3000 + rand() % 4000;
            } else uiMortalWoundTimer -= diff;

            if (uiEnrage1Timer <= diff)
            {
                DoCast(me, SPELL_ENRAGE_1);
                uiEnrage1Timer = 15000;
            } else uiEnrage1Timer -= diff;

            if (uiEnrage2Timer <= diff)
            {
                DoCast(me, SPELL_ENRAGE_2);
                uiEnrage2Timer = 10000;
            } else uiEnrage2Timer -= diff;

            DoMeleeAttackIfReady();
        }

        void AttackStart(Unit* who) OVERRIDE
        {
            if (!who)
                return;

            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                return;

            if (me->Attack(who, true))
            {
                me->AddThreat(who, 0.0f);
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);
                DoStartMovement(who);
            }
            if (instance)
                instance->SetData(DATA_GORTOK_PALEHOOF_EVENT, IN_PROGRESS);
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            if (instance)
            {
                Creature* pPalehoof = Unit::GetCreature((*me), instance->GetData64(DATA_GORTOK_PALEHOOF));
                if (pPalehoof)
                    CAST_AI(boss_palehoof::boss_palehoofAI, pPalehoof->AI())->NextPhase();
            }
        }

        void JustReachedHome() OVERRIDE
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            DoCast(me, SPELL_FREEZE);
        }
    };
};

//ferocious rhino's spells
enum FerociousSpells
{
    SPELL_GORE                                  = 48130,
    H_SPELL_GORE                                = 59264,
    SPELL_GRIEVOUS_WOUND                        = 48105,
    H_SPELL_GRIEVOUS_WOUND                      = 59263,
    SPELL_STOMP                                 = 48131
};

class npc_ferocious_rhino : public CreatureScript
{
public:
    npc_ferocious_rhino() : CreatureScript("npc_ferocious_rhino") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_ferocious_rhinoAI(creature);
    }

    struct npc_ferocious_rhinoAI : public ScriptedAI
    {
        npc_ferocious_rhinoAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        uint32 uiStompTimer;
        uint32 uiGoreTimer;
        uint32 uiGrievousWoundTimer;

        InstanceScript* instance;

        void Reset() OVERRIDE
        {
            uiStompTimer = 10000;
            uiGoreTimer = 15000;
            uiGrievousWoundTimer = 20000;

            me->GetMotionMaster()->MoveTargetedHome();

            if (instance)
                if (instance->GetData(DATA_GORTOK_PALEHOOF_EVENT) == IN_PROGRESS)
                {
                    Creature* pPalehoof = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_GORTOK_PALEHOOF) : 0);
                    if (pPalehoof && pPalehoof->IsAlive())
                        CAST_AI(boss_palehoof::boss_palehoofAI, pPalehoof->AI())->Reset();
                }
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            if (uiStompTimer <= diff)
            {
                DoCastVictim(SPELL_STOMP);
                uiStompTimer = 8000 + rand() % 4000;
            } else uiStompTimer -= diff;

            if (uiGoreTimer <= diff)
            {
                DoCastVictim(SPELL_GORE);
                uiGoreTimer = 13000 + rand() % 4000;
            } else uiGoreTimer -= diff;

            if (uiGrievousWoundTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                    DoCast(target, SPELL_GRIEVOUS_WOUND);
                uiGrievousWoundTimer = 18000 + rand() % 4000;
            } else uiGrievousWoundTimer -= diff;

            DoMeleeAttackIfReady();
        }

        void AttackStart(Unit* who) OVERRIDE
        {
            if (!who)
                return;

            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                return;

            if (me->Attack(who, true))
            {
                me->AddThreat(who, 0.0f);
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);
                DoStartMovement(who);
            }
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            if (instance)
            {
                Creature* pPalehoof = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_GORTOK_PALEHOOF) : 0);
                if (pPalehoof)
                    CAST_AI(boss_palehoof::boss_palehoofAI, pPalehoof->AI())->NextPhase();
            }
        }

        void JustReachedHome() OVERRIDE
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            DoCast(me, SPELL_FREEZE);
        }
    };
};

//massive jormungar's spells
enum MassiveSpells
{
    SPELL_ACID_SPIT                             = 48132,
    SPELL_ACID_SPLATTER                         = 48136,
    H_SPELL_ACID_SPLATTER                       = 59272,
    SPELL_POISON_BREATH                         = 48133,
    H_SPELL_POISON_BREATH                       = 59271
};

enum MassiveAdds
{
  CREATURE_JORMUNGAR_WORM                     = 27228
};

class npc_massive_jormungar : public CreatureScript
{
public:
    npc_massive_jormungar() : CreatureScript("npc_massive_jormungar") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_massive_jormungarAI(creature);
    }

    struct npc_massive_jormungarAI : public ScriptedAI
    {
        npc_massive_jormungarAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        uint32 uiAcidSpitTimer;
        uint32 uiAcidSplatterTimer;
        uint32 uiPoisonBreathTimer;

        InstanceScript* instance;

        void Reset() OVERRIDE
        {
            uiAcidSpitTimer = 3000;
            uiAcidSplatterTimer = 12000;
            uiPoisonBreathTimer = 10000;

            me->GetMotionMaster()->MoveTargetedHome();

            if (instance)
                if (instance->GetData(DATA_GORTOK_PALEHOOF_EVENT) == IN_PROGRESS)
                {
                    Creature* pPalehoof = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_GORTOK_PALEHOOF) : 0);
                    if (pPalehoof && pPalehoof->IsAlive())
                        CAST_AI(boss_palehoof::boss_palehoofAI, pPalehoof->AI())->Reset();
                }
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            if (uiAcidSpitTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                    DoCast(target, SPELL_ACID_SPIT);
                uiAcidSpitTimer = 2000 + rand() % 2000;
            } else uiAcidSpitTimer -= diff;

            if (uiAcidSplatterTimer <= diff)
            {
                DoCast(me, SPELL_POISON_BREATH);
                uiAcidSplatterTimer = 10000 + rand() % 4000;
            } else uiAcidSplatterTimer -= diff;

            if (uiPoisonBreathTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                    DoCast(target, SPELL_POISON_BREATH);
                uiPoisonBreathTimer = 8000 + rand() % 4000;
            } else uiPoisonBreathTimer -= diff;

            DoMeleeAttackIfReady();
        }

        void AttackStart(Unit* who) OVERRIDE
        {
            if (!who)
                return;

            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                return;

            if (me->Attack(who, true))
            {
                me->AddThreat(who, 0.0f);
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);
                DoStartMovement(who);
            }
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            if (instance)
            {
                Creature* pPalehoof = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_GORTOK_PALEHOOF) : 0);
                if (pPalehoof)
                    CAST_AI(boss_palehoof::boss_palehoofAI, pPalehoof->AI())->NextPhase();
            }
        }

        void JustReachedHome() OVERRIDE
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            DoCast(me, SPELL_FREEZE);
        }
    };
};

class npc_palehoof_orb : public CreatureScript
{
public:
    npc_palehoof_orb() : CreatureScript("npc_palehoof_orb") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_palehoof_orbAI(creature);
    }

    struct npc_palehoof_orbAI : public ScriptedAI
    {
        npc_palehoof_orbAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 SummonTimer;
        Phase currentPhase;

        void Reset() OVERRIDE
        {
            currentPhase = PHASE_NONE;
            SummonTimer = 5000;
            //! HACK: Creature's can't have MOVEMENTFLAG_FLYING
            me->AddUnitMovementFlag(MOVEMENTFLAG_FLYING);
            me->RemoveAurasDueToSpell(SPELL_ORB_VISUAL);
            me->SetSpeed(MOVE_FLIGHT, 0.5f);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (currentPhase == PHASE_NONE)
                return;

            if (SummonTimer <= diff)
            {
                if (currentPhase<5&&currentPhase >= 0)
                {
                    Creature* pNext = NULL;
                    switch (currentPhase)
                    {
                        case PHASE_FRENZIED_WORGEN: pNext = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_NPC_FRENZIED_WORGEN) : 0); break;
                        case PHASE_RAVENOUS_FURLBORG: pNext = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_NPC_RAVENOUS_FURBOLG) : 0); break;
                        case PHASE_MASSIVE_JORMUNGAR: pNext = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_NPC_MASSIVE_JORMUNGAR) : 0); break;
                        case PHASE_FEROCIOUS_RHINO: pNext = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_NPC_FEROCIOUS_RHINO) : 0); break;
                        case PHASE_GORTOK_PALEHOOF: pNext = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_GORTOK_PALEHOOF) : 0); break;
                        default: break;
                    }

                    if (pNext)
                    {
                        pNext->RemoveAurasDueToSpell(SPELL_FREEZE);
                        pNext->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC);
                        pNext->SetStandState(UNIT_STAND_STATE_STAND);
                        pNext->SetInCombatWithZone();
                        pNext->Attack(pNext->SelectNearestTarget(100), true);
                    }
                    currentPhase = PHASE_NONE;
                }
            } else SummonTimer -= diff;
        }

        void MovementInform(uint32 type, uint32 id) OVERRIDE
        {
            if (type != POINT_MOTION_TYPE)
                return;
            if (id > 4)
                return;
            Creature* pNext = NULL;
            switch (id)
            {
                case PHASE_FRENZIED_WORGEN: pNext = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_NPC_FRENZIED_WORGEN) : 0); break;
                case PHASE_RAVENOUS_FURLBORG: pNext = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_NPC_RAVENOUS_FURBOLG) : 0); break;
                case PHASE_MASSIVE_JORMUNGAR: pNext = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_NPC_MASSIVE_JORMUNGAR) : 0); break;
                case PHASE_FEROCIOUS_RHINO: pNext = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_NPC_FEROCIOUS_RHINO) : 0); break;
                case PHASE_GORTOK_PALEHOOF: pNext = Unit::GetCreature((*me), instance ? instance->GetData64(DATA_GORTOK_PALEHOOF) : 0); break;
                default: break;
            }
            if (pNext)
                DoCast(pNext, SPELL_ORB_CHANNEL, false);
            currentPhase = (Phase)id;
            SummonTimer = 5000;
        }
    };
};

class go_palehoof_sphere : public GameObjectScript
{
public:
    go_palehoof_sphere() : GameObjectScript("go_palehoof_sphere") { }

    bool OnGossipHello(Player* /*player*/, GameObject* go) OVERRIDE
    {
        InstanceScript* instance = go->GetInstanceScript();

        Creature* pPalehoof = Unit::GetCreature(*go, instance ? instance->GetData64(DATA_GORTOK_PALEHOOF) : 0);
        if (pPalehoof && pPalehoof->IsAlive())
        {
            go->SetFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE);
            go->SetGoState(GOState::GO_STATE_ACTIVE);

            CAST_AI(boss_palehoof::boss_palehoofAI, pPalehoof->AI())->NextPhase();
        }
        return true;
    }
};

void AddSC_boss_palehoof()
{
    new boss_palehoof();
    new npc_ravenous_furbolg();
    new npc_frenzied_worgen();
    new npc_ferocious_rhino();
    new npc_massive_jormungar();
    new npc_palehoof_orb();
    new go_palehoof_sphere();
}
