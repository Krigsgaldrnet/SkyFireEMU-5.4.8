/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ruins_of_ahnqiraj.h"
#include "CreatureTextMgr.h"

enum Spells
{
    SPELL_MORTALWOUND       = 25646,
    SPELL_SANDTRAP          = 25648,
    SPELL_ENRAGE            = 26527,
    SPELL_SUMMON_PLAYER     = 26446,
    SPELL_TRASH             =  3391, // Should perhaps be triggered by an aura? Couldn't find any though
    SPELL_WIDE_SLASH        = 25814
};

enum Events
{
    EVENT_MORTAL_WOUND      = 1,
    EVENT_SANDTRAP          = 2,
    EVENT_TRASH             = 3,
    EVENT_WIDE_SLASH        = 4
};

enum Texts
{
    SAY_KURINAXX_DEATH      = 5, // Yelled by Ossirian the Unscarred
};

class boss_kurinnaxx : public CreatureScript
{
    public:
        boss_kurinnaxx() : CreatureScript("boss_kurinnaxx") { }

        struct boss_kurinnaxxAI : public BossAI
        {
            boss_kurinnaxxAI(Creature* creature) : BossAI(creature, DATA_KURINNAXX)
            {
            }

            void Reset() OVERRIDE
            {
                _Reset();
                _enraged = false;
                events.ScheduleEvent(EVENT_MORTAL_WOUND, 8000);
                events.ScheduleEvent(EVENT_SANDTRAP, std::rand() % 15000 + 5000);
                events.ScheduleEvent(EVENT_TRASH, 1000);
                events.ScheduleEvent(EVENT_WIDE_SLASH, 11000);
            }

            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/) OVERRIDE
            {
                if (!_enraged && HealthBelowPct(30))
                {
                    DoCast(me, SPELL_ENRAGE);
                    _enraged = true;
                }
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                _JustDied();
                if (Creature* Ossirian = me->GetMap()->GetCreature(instance->GetData64(DATA_OSSIRIAN)))
                    sCreatureTextMgr->SendChat(Ossirian, SAY_KURINAXX_DEATH, 0, ChatMsg::CHAT_MSG_ADDON, Language::LANG_ADDON, TEXT_RANGE_ZONE);
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
                        case EVENT_MORTAL_WOUND:
                            DoCastVictim(SPELL_MORTALWOUND);
                            events.ScheduleEvent(EVENT_MORTAL_WOUND, 8000);
                            break;
                        case EVENT_SANDTRAP:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                                target->CastSpell(target, SPELL_SANDTRAP, true);
                            else if (Unit* victim = me->GetVictim())
                                victim->CastSpell(victim, SPELL_SANDTRAP, true);
                            events.ScheduleEvent(EVENT_SANDTRAP, std::rand() % 15000 + 5000);
                            break;
                        case EVENT_WIDE_SLASH:
                            DoCast(me, SPELL_WIDE_SLASH);
                            events.ScheduleEvent(EVENT_WIDE_SLASH, 11000);
                            break;
                        case EVENT_TRASH:
                            DoCast(me, SPELL_TRASH);
                            events.ScheduleEvent(EVENT_WIDE_SLASH, 16000);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
            private:
                bool _enraged;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new boss_kurinnaxxAI(creature);
        }
};

void AddSC_boss_kurinnaxx()
{
    new boss_kurinnaxx();
}
