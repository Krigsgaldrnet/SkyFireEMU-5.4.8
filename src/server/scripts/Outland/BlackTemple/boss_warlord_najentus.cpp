/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* ScriptData
SDName: Boss_Warlord_Najentus
SD%Complete: 95
SDComment:
SDCategory: Black Temple
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "black_temple.h"
#include "Player.h"
#include "SpellInfo.h"

enum Yells
{
    SAY_AGGRO                       = 0,
    SAY_NEEDLE                      = 1,
    SAY_SLAY                        = 2,
    SAY_SPECIAL                     = 3,
    SAY_ENRAGE                      = 4,
    SAY_DEATH                       = 5
};

enum Spells
{
    SPELL_NEEDLE_SPINE              = 39992,
    SPELL_TIDAL_BURST               = 39878,
    SPELL_TIDAL_SHIELD              = 39872,
    SPELL_IMPALING_SPINE            = 39837,
    SPELL_CREATE_NAJENTUS_SPINE     = 39956,
    SPELL_HURL_SPINE                = 39948,
    SPELL_BERSERK                   = 26662
};

enum GameObjects
{
    GOBJECT_SPINE                   = 185584
};

enum Events
{
    EVENT_BERSERK                   = 1,
    EVENT_YELL                      = 2,
    EVENT_NEEDLE                    = 3,
    EVENT_SPINE                     = 4,
    EVENT_SHIELD                    = 5
};

enum Misc
{
    GCD_CAST                        = 1,
    GCD_YELL                        = 2
};

class boss_najentus : public CreatureScript
{
public:
    boss_najentus() : CreatureScript("boss_najentus") { }

    struct boss_najentusAI : public ScriptedAI
    {
        boss_najentusAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            SpineTargetGUID = 0;
        }

        void Reset() OVERRIDE
        {
            events.Reset();

            SpineTargetGUID = 0;

            if (instance)
                instance->SetBossState(DATA_HIGH_WARLORD_NAJENTUS, NOT_STARTED);
        }

        void KilledUnit(Unit* /*victim*/) OVERRIDE
        {
            Talk(SAY_SLAY);
            events.DelayEvents(5000, GCD_YELL);
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            if (instance)
                instance->SetBossState(DATA_HIGH_WARLORD_NAJENTUS, DONE);

            Talk(SAY_DEATH);
        }

        void SpellHit(Unit* /*caster*/, const SpellInfo* spell) OVERRIDE
        {
            if (spell->Id == SPELL_HURL_SPINE && me->HasAura(SPELL_TIDAL_SHIELD))
            {
                me->RemoveAurasDueToSpell(SPELL_TIDAL_SHIELD);
                DoCast(me, SPELL_TIDAL_BURST, true);
                ResetTimer();
            }
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            if (instance)
                instance->SetBossState(DATA_HIGH_WARLORD_NAJENTUS, IN_PROGRESS);

            Talk(SAY_AGGRO);
            DoZoneInCombat();
            events.ScheduleEvent(EVENT_BERSERK, 480000, GCD_CAST);
            events.ScheduleEvent(EVENT_YELL, 45000 + (rand()%76)*1000, GCD_YELL);
            ResetTimer();
        }

        bool RemoveImpalingSpine()
        {
            if (!SpineTargetGUID)
                return false;

            Unit* target = Unit::GetUnit(*me, SpineTargetGUID);
            if (target && target->HasAura(SPELL_IMPALING_SPINE))
                target->RemoveAurasDueToSpell(SPELL_IMPALING_SPINE);
            SpineTargetGUID=0;
            return true;
        }

        void ResetTimer(uint32 inc = 0)
        {
            events.RescheduleEvent(EVENT_NEEDLE, 10000 + inc, GCD_CAST);
            events.RescheduleEvent(EVENT_SPINE, 20000 + inc, GCD_CAST);
            events.RescheduleEvent(EVENT_SHIELD, 60000 + inc);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SHIELD:
                        DoCast(me, SPELL_TIDAL_SHIELD, true);
                        ResetTimer(45000);
                        break;
                    case EVENT_BERSERK:
                        Talk(SAY_ENRAGE);
                        DoCast(me, SPELL_BERSERK, true);
                        events.DelayEvents(15000, GCD_YELL);
                        break;
                    case EVENT_SPINE:
                    {
                        Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1);
                        if (!target) target = me->GetVictim();
                        if (target)
                        {
                            DoCast(target, SPELL_IMPALING_SPINE, true);
                            SpineTargetGUID = target->GetGUID();
                            //must let target summon, otherwise you cannot click the spine
                            target->SummonGameObject(GOBJECT_SPINE, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), me->GetOrientation(), 0, 0, 0, 0, 30);
                            Talk(SAY_NEEDLE);
                            events.DelayEvents(1500, GCD_CAST);
                            events.DelayEvents(15000, GCD_YELL);
                        }
                        events.ScheduleEvent(EVENT_SPINE, 21000, GCD_CAST);
                        return;
                    }
                    case EVENT_NEEDLE:
                    {
                        //DoCast(me, SPELL_NEEDLE_SPINE, true);
                        std::list<Unit*> targets;
                        SelectTargetList(targets, 3, SELECT_TARGET_RANDOM, 80, true);
                        for (std::list<Unit*>::const_iterator i = targets.begin(); i != targets.end(); ++i)
                            DoCast(*i, 39835, true);
                        events.ScheduleEvent(EVENT_NEEDLE, std::rand() % 25000 + 15000, GCD_CAST);
                        events.DelayEvents(1500, GCD_CAST);
                        return;
                    }
                    case EVENT_YELL:
                        Talk(SAY_SPECIAL);
                        events.ScheduleEvent(EVENT_YELL, std::rand() % 100000 + 25000, GCD_YELL);
                        events.DelayEvents(15000, GCD_YELL);
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    private:
        InstanceScript* instance;
        EventMap events;

        uint64 SpineTargetGUID;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_najentusAI(creature);
    }
};

class go_najentus_spine : public GameObjectScript
{
public:
    go_najentus_spine() : GameObjectScript("go_najentus_spine") { }

    bool OnGossipHello(Player* player, GameObject* go) OVERRIDE
    {
        if (InstanceScript* instance = go->GetInstanceScript())
            if (Creature* Najentus = ObjectAccessor::GetCreature(*go, instance->GetData64(DATA_HIGH_WARLORD_NAJENTUS)))
                if (CAST_AI(boss_najentus::boss_najentusAI, Najentus->AI())->RemoveImpalingSpine())
                {
                    player->CastSpell(player, SPELL_CREATE_NAJENTUS_SPINE, true);
                    go->Delete();
                }
        return true;
    }
};

void AddSC_boss_najentus()
{
    new boss_najentus();
    new go_najentus_spine();
}
