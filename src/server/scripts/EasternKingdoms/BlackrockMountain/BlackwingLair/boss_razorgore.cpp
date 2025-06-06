/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "blackwing_lair.h"
#include "Player.h"

enum Say
{
    SAY_EGGS_BROKEN1        = 0,
    SAY_EGGS_BROKEN2        = 1,
    SAY_EGGS_BROKEN3        = 2,
    SAY_DEATH               = 3,
};

enum Spells
{
    SPELL_MINDCONTROL       = 42013,
    SPELL_CHANNEL           = 45537,
    SPELL_EGG_DESTROY       = 19873,

    SPELL_CLEAVE            = 22540,
    SPELL_WARSTOMP          = 24375,
    SPELL_FIREBALLVOLLEY    = 22425,
    SPELL_CONFLAGRATION     = 23023
};

enum Summons
{
    NPC_ELITE_DRACHKIN      = 12422,
    NPC_ELITE_WARRIOR       = 12458,
    NPC_WARRIOR             = 12416,
    NPC_MAGE                = 12420,
    NPC_WARLOCK             = 12459,

    GO_EGG                  = 177807
};

enum EVENTS
{
    EVENT_CLEAVE            = 1,
    EVENT_STOMP             = 2,
    EVENT_FIREBALL          = 3,
    EVENT_CONFLAGRATION     = 4
};

class boss_razorgore : public CreatureScript
{
public:
    boss_razorgore() : CreatureScript("boss_razorgore") { }

    struct boss_razorgoreAI : public BossAI
    {
        boss_razorgoreAI(Creature* creature) : BossAI(creature, BOSS_RAZORGORE) { }

        void Reset() OVERRIDE
        {
            _Reset();

            secondPhase = false;
            if (instance)
                instance->SetData(DATA_EGG_EVENT, NOT_STARTED);
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            _JustDied();
            Talk(SAY_DEATH);

            if (instance)
                instance->SetData(DATA_EGG_EVENT, NOT_STARTED);
        }

        void DoChangePhase()
        {
            events.ScheduleEvent(EVENT_CLEAVE, 15000);
            events.ScheduleEvent(EVENT_STOMP, 35000);
            events.ScheduleEvent(EVENT_FIREBALL, 7000);
            events.ScheduleEvent(EVENT_CONFLAGRATION, 12000);

            secondPhase = true;
            me->RemoveAllAuras();
            me->SetHealth(me->GetMaxHealth());
        }

        void DoAction(int32 action) OVERRIDE
        {
            if (action == ACTION_PHASE_TWO)
                DoChangePhase();
        }

        void DamageTaken(Unit* /*who*/, uint32& damage) OVERRIDE
        {
            if (!secondPhase)
                damage = 0;
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
                    case EVENT_CLEAVE:
                        DoCastVictim(SPELL_CLEAVE);
                        events.ScheduleEvent(EVENT_CLEAVE, std::rand() % 10000 + 7000);
                        break;
                    case EVENT_STOMP:
                        DoCastVictim(SPELL_WARSTOMP);
                        events.ScheduleEvent(EVENT_STOMP, std::rand() % 25000 + 15000);
                        break;
                    case EVENT_FIREBALL:
                        DoCastVictim(SPELL_FIREBALLVOLLEY);
                        events.ScheduleEvent(EVENT_FIREBALL, std::rand() % 15000 + 12000);
                        break;
                    case EVENT_CONFLAGRATION:
                        DoCastVictim(SPELL_CONFLAGRATION);
                        if (me->GetVictim() && me->GetVictim()->HasAura(SPELL_CONFLAGRATION))
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 100, true))
                                me->TauntApply(target);
                        events.ScheduleEvent(EVENT_CONFLAGRATION, 30000);
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    private:
        bool secondPhase;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_razorgoreAI(creature);
    }
};

class go_orb_of_domination : public GameObjectScript
{
public:
    go_orb_of_domination() : GameObjectScript("go_orb_of_domination") { }

    bool OnGossipHello(Player* player, GameObject* go) OVERRIDE
    {
        if (InstanceScript* instance = go->GetInstanceScript())
            if (instance->GetData(DATA_EGG_EVENT) != DONE)
                if (Creature* razor = Unit::GetCreature(*go, instance->GetData64(DATA_RAZORGORE_THE_UNTAMED)))
                {
                    razor->Attack(player, true);
                    player->CastSpell(razor, SPELL_MINDCONTROL);
                }
        return true;
    }
};

class spell_egg_event : public SpellScriptLoader
{
    public:
        spell_egg_event() : SpellScriptLoader("spell_egg_event") { }

        class spell_egg_eventSpellScript : public SpellScript
        {
            PrepareSpellScript(spell_egg_eventSpellScript);

            void HandleOnHit()
            {
                if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                    instance->SetData(DATA_EGG_EVENT, SPECIAL);
            }

            void Register() OVERRIDE
            {
                OnHit += SpellHitFn(spell_egg_eventSpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_egg_eventSpellScript();
        }
};

void AddSC_boss_razorgore()
{
    new boss_razorgore();
    new go_orb_of_domination();
    new spell_egg_event();
}
