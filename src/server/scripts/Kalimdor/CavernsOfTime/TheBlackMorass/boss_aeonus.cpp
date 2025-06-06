/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/*
Name: Boss_Aeonus
%Complete: 80
Comment: Some spells not implemented
Category: Caverns of Time, The Dark Portal
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "the_black_morass.h"

enum Enums
{
    SAY_ENTER           = 0,
    SAY_AGGRO           = 1,
    SAY_BANISH          = 2,
    SAY_SLAY            = 3,
    SAY_DEATH           = 4,
    EMOTE_FRENZY        = 5,

    SPELL_CLEAVE        = 40504,
    SPELL_TIME_STOP     = 31422,
    SPELL_ENRAGE        = 37605,
    SPELL_SAND_BREATH   = 31473,
    H_SPELL_SAND_BREATH = 39049
};

enum Events
{
    EVENT_SANDBREATH    = 1,
    EVENT_TIMESTOP      = 2,
    EVENT_FRENZY        = 3
};

class boss_aeonus : public CreatureScript
{
public:
    boss_aeonus() : CreatureScript("boss_aeonus") { }

    struct boss_aeonusAI : public BossAI
    {
        boss_aeonusAI(Creature* creature) : BossAI(creature, TYPE_AEONUS) { }

        void Reset() OVERRIDE { }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            events.ScheduleEvent(EVENT_SANDBREATH, std::rand() % 30000 + 15000);
            events.ScheduleEvent(EVENT_TIMESTOP, std::rand() % 15000 + 10000);
            events.ScheduleEvent(EVENT_FRENZY, std::rand() % 45000 + 30000);

            Talk(SAY_AGGRO);
        }

        void MoveInLineOfSight(Unit* who) OVERRIDE

        {
            //Despawn Time Keeper
            if (who->GetTypeId() == TypeID::TYPEID_UNIT && who->GetEntry() == NPC_TIME_KEEPER)
            {
                if (me->IsWithinDistInMap(who, 20.0f))
                {
                    Talk(SAY_BANISH);
                    me->DealDamage(who, who->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                }
            }

            ScriptedAI::MoveInLineOfSight(who);
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            Talk(SAY_DEATH);

             if (instance)
             {
                 instance->SetData(TYPE_RIFT, DONE);
                 instance->SetData(TYPE_MEDIVH, DONE); // FIXME: later should be removed
             }
        }

        void KilledUnit(Unit* /*victim*/) OVERRIDE
        {
            Talk(SAY_SLAY);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SANDBREATH:
                        DoCastVictim(SPELL_SAND_BREATH);
                        events.ScheduleEvent(EVENT_SANDBREATH, std::rand() % 25000 + 15000);
                        break;
                    case EVENT_TIMESTOP:
                        DoCastVictim(SPELL_TIME_STOP);
                        events.ScheduleEvent(EVENT_TIMESTOP, std::rand() % 35000 + 20000);
                        break;
                    case EVENT_FRENZY:
                        Talk(EMOTE_FRENZY);
                        DoCast(me, SPELL_ENRAGE);
                        events.ScheduleEvent(EVENT_FRENZY, std::rand() % 35000 + 25000);
                        break;
                    default:
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_aeonusAI(creature);
    }
};

void AddSC_boss_aeonus()
{
    new boss_aeonus();
}
