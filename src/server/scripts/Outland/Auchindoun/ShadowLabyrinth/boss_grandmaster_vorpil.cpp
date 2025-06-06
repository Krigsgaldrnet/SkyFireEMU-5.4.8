/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/*
Name: Boss_Grandmaster_Vorpil
%Complete: 100
Category: Auchindoun, Shadow Labyrinth
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "shadow_labyrinth.h"
#include "Player.h"

enum GrandmasterVorpil
{
    SAY_INTRO                   = 0,
    SAY_AGGRO                   = 1,
    SAY_HELP                    = 2,
    SAY_SLAY                    = 3,
    SAY_DEATH                   = 4,

    SPELL_RAIN_OF_FIRE          = 33617,
    H_SPELL_RAIN_OF_FIRE        = 39363,

    SPELL_DRAW_SHADOWS          = 33563,
    SPELL_SHADOWBOLT_VOLLEY     = 33841,
    SPELL_BANISH                = 38791,

    NPC_VOID_TRAVELER           = 19226,
    SPELL_SUMMON_VOID_TRAVELER_A = 33582,
    SPELL_SUMMON_VOID_TRAVELER_B = 33583,
    SPELL_SUMMON_VOID_TRAVELER_C = 33584,
    SPELL_SUMMON_VOID_TRAVELER_D = 33585,
    SPELL_SUMMON_VOID_TRAVELER_E = 33586,

    SPELL_SACRIFICE             = 33587,
    SPELL_SHADOW_NOVA           = 33846,
    SPELL_EMPOWERING_SHADOWS    = 33783,
    H_SPELL_EMPOWERING_SHADOWS  = 39364,

    NPC_VOID_PORTAL             = 19224,
    SPELL_SUMMON_PORTAL         = 33566,
    SPELL_VOID_PORTAL_VISUAL    = 33569
};

Position const VorpilPosition = { -252.8820f, -264.3030f, 17.1f, 0.0f };

float VoidPortalCoords[5][3] =
{
    {-283.5894f, -239.5718f, 12.7f},
    {-306.5853f, -258.4539f, 12.7f},
    {-295.8789f, -269.0899f, 12.7f},
    {-209.3401f, -262.7564f, 17.1f},
    {-261.4533f, -297.3298f, 17.1f}
};

enum Events
{
    EVENT_SHADOWBOLT_VOLLEY     = 1,
    EVENT_BANISH                = 2,
    EVENT_DRAW_SHADOWS          = 3,
    EVENT_SUMMON_TRAVELER       = 4
};

class boss_grandmaster_vorpil : public CreatureScript
{
    public:
        boss_grandmaster_vorpil() : CreatureScript("boss_grandmaster_vorpil") { }

        struct boss_grandmaster_vorpilAI : public BossAI
        {
            boss_grandmaster_vorpilAI(Creature* creature) : BossAI(creature, DATA_GRANDMASTER_VORPIL)
            {
                _intro = false;
            }

            void Reset() OVERRIDE
            {
                _Reset();
                _helpYell = false;
            }

            void SummonPortals()
            {
                for (uint8 i = 0; i < 5; ++i)
                    if (Creature* portal = me->SummonCreature(NPC_VOID_PORTAL, VoidPortalCoords[i][0], VoidPortalCoords[i][1], VoidPortalCoords[i][2], 0, TempSummonType::TEMPSUMMON_CORPSE_DESPAWN, 3000000))
                        portal->CastSpell(portal, SPELL_VOID_PORTAL_VISUAL, true);

                events.ScheduleEvent(EVENT_SUMMON_TRAVELER, 5000);
            }

            void spawnVoidTraveler()
            {
                uint8 pos = std::rand() % 4;
                me->SummonCreature(NPC_VOID_TRAVELER, VoidPortalCoords[pos][0], VoidPortalCoords[pos][1], VoidPortalCoords[pos][2], 0, TempSummonType::TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5000);
                if (!_helpYell)
                {
                    Talk(SAY_HELP);
                    _helpYell = true;
                }
            }

            void KilledUnit(Unit* who) OVERRIDE
            {
                if (who->GetTypeId() == TypeID::TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                _JustDied();
                Talk(SAY_DEATH);
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
                _EnterCombat();
                events.ScheduleEvent(EVENT_SHADOWBOLT_VOLLEY, std::rand() % 14000 + 7000);
                if (IsHeroic())
                    events.ScheduleEvent(EVENT_BANISH, 17000);
                events.ScheduleEvent(EVENT_DRAW_SHADOWS, 45000);
                events.ScheduleEvent(EVENT_SUMMON_TRAVELER, 90000);

                Talk(SAY_AGGRO);
                SummonPortals();
            }

            void MoveInLineOfSight(Unit* who) OVERRIDE
            {
                BossAI::MoveInLineOfSight(who);

                if (!_intro && me->IsWithinLOSInMap(who) && me->IsWithinDistInMap(who, 100) && me->IsValidAttackTarget(who))
                {
                    Talk(SAY_INTRO);
                    _intro = true;
                }
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SHADOWBOLT_VOLLEY:
                            DoCast(me, SPELL_SHADOWBOLT_VOLLEY);
                            events.ScheduleEvent(EVENT_SHADOWBOLT_VOLLEY, std::rand() % 30000 + 15000);
                            break;
                        case EVENT_BANISH:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30.0f, false))
                                 DoCast(target, SPELL_BANISH);
                            events.ScheduleEvent(EVENT_BANISH, 16000);
                            break;
                        case EVENT_DRAW_SHADOWS:
                            {
                                Map* map = me->GetMap();
                                Map::PlayerList const &PlayerList = map->GetPlayers();
                                for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                                    if (Player* i_pl = i->GetSource())
                                        if (i_pl->IsAlive() && !i_pl->HasAura(SPELL_BANISH))
                                            i_pl->TeleportTo(me->GetMapId(), VorpilPosition.GetPositionX(), VorpilPosition.GetPositionY(), VorpilPosition.GetPositionZ(), VorpilPosition.GetOrientation(), TELE_TO_NOT_LEAVE_COMBAT);

                                me->SetPosition(VorpilPosition);
                                DoCast(me, SPELL_DRAW_SHADOWS, true);
                                DoCast(me, SPELL_RAIN_OF_FIRE);
                                events.ScheduleEvent(EVENT_SHADOWBOLT_VOLLEY, 6000);
                                events.ScheduleEvent(EVENT_DRAW_SHADOWS, 30000);
                                break;
                            }
                        case EVENT_SUMMON_TRAVELER:
                            spawnVoidTraveler();
                            events.ScheduleEvent(EVENT_SUMMON_TRAVELER, 10000);
                            // enrage at 20%
                            if (HealthBelowPct(20))
                                events.ScheduleEvent(EVENT_SUMMON_TRAVELER, 5000);
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            private:
                bool _intro;
                bool _helpYell;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return GetShadowLabyrinthAI<boss_grandmaster_vorpilAI>(creature);
        }
};

class npc_voidtraveler : public CreatureScript
{
    public:
        npc_voidtraveler() : CreatureScript("npc_voidtraveler") { }

        struct npc_voidtravelerAI : public ScriptedAI
        {
            npc_voidtravelerAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
            }

            void Reset() OVERRIDE
            {
                _moveTimer = 0;
                _sacrificed = false;
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE { }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (_moveTimer <= diff)
                {
                    Creature* Vorpil = ObjectAccessor::GetCreature(*me, _instance->GetData64(DATA_GRANDMASTER_VORPIL));
                    if (!Vorpil)
                        return;

                    if (_sacrificed)
                    {
                        DoCastAOE(DUNGEON_MODE(SPELL_EMPOWERING_SHADOWS, H_SPELL_EMPOWERING_SHADOWS), true);
                        DoCast(me, SPELL_SHADOW_NOVA, true);
                        me->Kill(me);
                        return;
                    }
                    me->GetMotionMaster()->MoveFollow(Vorpil, 0, 0);
                    if (me->IsWithinDist(Vorpil, 3))
                    {
                        DoCast(me, SPELL_SACRIFICE, false);
                        _sacrificed = true;
                        _moveTimer = 500;
                        return;
                    }
                    _moveTimer = 1000;
                }
                else
                    _moveTimer -= diff;
            }

        private:
            InstanceScript* _instance;
            uint32 _moveTimer;
            bool _sacrificed;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return GetShadowLabyrinthAI<npc_voidtravelerAI>(creature);
        }
};

void AddSC_boss_grandmaster_vorpil()
{
    new boss_grandmaster_vorpil();
    new npc_voidtraveler();
}
