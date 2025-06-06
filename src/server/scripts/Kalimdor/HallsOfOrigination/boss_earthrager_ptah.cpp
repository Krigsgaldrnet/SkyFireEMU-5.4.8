/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Player.h"
#include "Weather.h"
#include "WorldSession.h"
#include "halls_of_origination.h"

enum Texts
{
    SAY_AGGRO                       = 0,
    SAY_DEATH                       = 1,
};

enum Events
{
    EVENT_RAGING_SMASH              = 1,
    EVENT_FLAME_BOLT                = 2,
    EVENT_EARTH_SPIKE               = 3,
    EVENT_PTAH_EXPLODE              = 4,
    EVENT_QUICKSAND                 = 5,
};

enum Spells
{
    SPELL_RAGING_SMASH              = 83650,
    SPELL_FLAME_BOLT                = 77370,
    SPELL_EARTH_SPIKE_WARN          = 94974,

    SPELL_PTAH_EXPLOSION            = 75519,
    SPELL_SANDSTORM                 = 75491,

    SPELL_SUMMON_QUICKSAND          = 75550, // Spell not in DBC, no SMSG_SPELL_START/GO for it

    SPELL_BEETLE_BURROW             = 75463,

    SPELL_SUMMON_JEWELED_SCARAB     = 75462,
    SPELL_SUMMON_DUSTBONE_HORROR    = 75521,
};

enum Phases
{
    PHASE_NORMAL                    = 1,
    PHASE_DISPERSE                  = 2,

    PHASE_MASK_DISPERSE             = (1 << PHASE_DISPERSE),
    PHASE_MASK_NORMAL               = (1 << PHASE_NORMAL),
};

enum PtahData
{
    DATA_SUMMON_DEATHS              = 0
};

class SummonScarab : public BasicEvent
{
public:
    SummonScarab(Unit* owner, InstanceScript* instance) : _owner(owner), _instance(instance) { }

    bool Execute(uint64 /*execTime*/, uint32 /*diff*/)
    {
        if (!_instance || _instance->GetBossState(DATA_EARTHRAGER_PTAH) != IN_PROGRESS)
            return true;    // delete event

        _owner->CastSpell(_owner, SPELL_SUMMON_JEWELED_SCARAB);
        _owner->RemoveAurasDueToSpell(SPELL_BEETLE_BURROW);
        return true;
    }
protected:
    Unit* _owner;
    InstanceScript* _instance;
};

class boss_earthrager_ptah : public CreatureScript
{
public:
    boss_earthrager_ptah() : CreatureScript("boss_earthrager_ptah") { }

    struct boss_earthrager_ptahAI : public BossAI
    {
        boss_earthrager_ptahAI(Creature* creature) : BossAI(creature, DATA_EARTHRAGER_PTAH), _summonDeaths(0), _hasDispersed(false) { }

        void Cleanup()
        {
            std::list<Creature*> units;

            GetCreatureListWithEntryInGrid(units, me, NPC_DUSTBONE_HORROR, 100.0f);
            for (std::list<Creature*>::iterator itr = units.begin(); itr != units.end(); ++itr)
                (*itr)->DespawnOrUnsummon();

            GetCreatureListWithEntryInGrid(units, me, NPC_JEWELED_SCARAB, 100.0f);
            for (std::list<Creature*>::iterator itr = units.begin(); itr != units.end(); ++itr)
                (*itr)->DespawnOrUnsummon();
        }

        void SendWeather(WeatherState weather, float grade) const
        {
            WorldPacket data(SMSG_WEATHER, 4 + 4 + 1);
            data << uint32(weather); // WeatherID
            data << float(grade);    // Intensity
            data.WriteBit(false);    // Abrupt
            data.FlushBits();
            SendPacketToPlayers(&data);
        }

        // Send packet to all players in Tomb of the Earthrager
        void SendPacketToPlayers(WorldPacket const* data) const
        {
            Map::PlayerList const& players = me->GetMap()->GetPlayers();
            if (!players.isEmpty())
                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    if (Player* player = itr->GetSource())
                        if (player->GetAreaId() == AREA_TOMB_OF_THE_EARTHRAGER)
                            player->GetSession()->SendPacket(data);
        }

        void Reset() OVERRIDE
        {
            _summonDeaths = 0;
            _hasDispersed = false;
            Cleanup();
            _Reset();
            events.SetPhase(PHASE_NORMAL);
            events.ScheduleEvent(EVENT_RAGING_SMASH, std::rand() % 12000 + 7000, 0, PHASE_NORMAL);
            events.ScheduleEvent(EVENT_FLAME_BOLT, 15000, 0, PHASE_NORMAL);
            events.ScheduleEvent(EVENT_EARTH_SPIKE, std::rand() % 21000 + 16000, 0, PHASE_NORMAL);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage) OVERRIDE
        {
            if (me->HealthBelowPctDamaged(50, damage) && (events.GetPhaseMask() & PHASE_MASK_NORMAL) && !_hasDispersed)
            {
                events.SetPhase(PHASE_DISPERSE);
                _hasDispersed = true;

                me->AttackStop();
                DoCast(me, SPELL_SANDSTORM);
                SendWeather(WEATHER_STATE_LIGHT_SANDSTORM, 1.0f);
                events.ScheduleEvent(EVENT_PTAH_EXPLODE, 6000, 0, PHASE_DISPERSE);
                events.ScheduleEvent(EVENT_QUICKSAND, 10000, 0, PHASE_DISPERSE);

                std::list<Creature*> stalkers;
                GetCreatureListWithEntryInGrid(stalkers, me, NPC_BEETLE_STALKER, 100.0f);
                std::list<Creature*> beetlers = stalkers;

                Skyfire::Containers::RandomResizeList(beetlers, 9); // Holds the summoners of Jeweled Scarab

                for (std::list<Creature*>::iterator itr = beetlers.begin(); itr != beetlers.end(); ++itr)
                {
                    stalkers.remove((*itr)); // Remove it to prevent a single trigger from spawning multiple npcs.
                    (*itr)->CastSpell((*itr), SPELL_BEETLE_BURROW); // Cast visual
                    // Summon after 5 seconds.
                    (*itr)->m_Events.AddEvent(new SummonScarab((*itr), instance), (*itr)->m_Events.CalculateTime(5000));
                }

                Skyfire::Containers::RandomResizeList(stalkers, 2); // Holds the summoners of Dustbone Horror

                for (std::list<Creature*>::iterator itr = stalkers.begin(); itr != stalkers.end(); ++itr)
                    (*itr)->CastSpell((*itr), SPELL_SUMMON_DUSTBONE_HORROR);
            }
        }

        void SetData(uint32 index, uint32 /*value*/) OVERRIDE
        {
            if (index == DATA_SUMMON_DEATHS)
            {
                ++_summonDeaths;
                if (_summonDeaths == 11) // All summons died
                {
                    SendWeather(WEATHER_STATE_FOG, 0.0f);
                    me->RemoveAurasDueToSpell(SPELL_PTAH_EXPLOSION);
                    events.SetPhase(PHASE_NORMAL);
                    events.ScheduleEvent(EVENT_RAGING_SMASH, std::rand() % 12000 + 7000, 0, PHASE_NORMAL);
                    events.ScheduleEvent(EVENT_FLAME_BOLT, 15000, 0, PHASE_NORMAL);
                    events.ScheduleEvent(EVENT_EARTH_SPIKE, std::rand() % 21000 + 16000, 0, PHASE_NORMAL);
                }
            }
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me, 1);
            Talk(SAY_AGGRO);
            _EnterCombat();
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            Talk(SAY_DEATH);
            _JustDied();
            Cleanup();
        }

        void JustReachedHome() OVERRIDE
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            _JustReachedHome();
            instance->SetBossState(DATA_EARTHRAGER_PTAH, FAIL);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim() || !CheckInRoom())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_RAGING_SMASH:
                        DoCastVictim(SPELL_RAGING_SMASH);
                        events.ScheduleEvent(EVENT_RAGING_SMASH, std::rand() % 12000 + 7000, 0, PHASE_NORMAL);
                        break;
                    case EVENT_FLAME_BOLT:
                        DoCast(me, SPELL_FLAME_BOLT);
                        events.ScheduleEvent(EVENT_FLAME_BOLT, 15000, 0, PHASE_NORMAL);
                        break;
                    case EVENT_EARTH_SPIKE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                            DoCast(target, SPELL_EARTH_SPIKE_WARN);
                        events.ScheduleEvent(EVENT_EARTH_SPIKE, std::rand() % 21000 + 16000, 0, PHASE_NORMAL);
                        break;
                    case EVENT_PTAH_EXPLODE:
                        DoCast(me, SPELL_PTAH_EXPLOSION);
                        break;
                    case EVENT_QUICKSAND:
                        // Spell not in DBC, it is not cast either, according to sniffs
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                            if (Creature* quicksand = me->SummonCreature(NPC_QUICKSAND, *target))
                                quicksand->SetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL, SPELL_SUMMON_QUICKSAND);
                        events.ScheduleEvent(EVENT_QUICKSAND, 10000, 0, PHASE_DISPERSE);
                        break;
                }
            }

            if (events.GetPhaseMask() & PHASE_MASK_NORMAL) // Do not melee in the disperse phase
                DoMeleeAttackIfReady();
        }

    protected:
        uint8 _summonDeaths;
        bool _hasDispersed;
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return GetHallsOfOriginationAI<boss_earthrager_ptahAI>(creature);
    }
};

class spell_earthrager_ptah_flame_bolt : public SpellScriptLoader
{
    public:
        spell_earthrager_ptah_flame_bolt() : SpellScriptLoader("spell_earthrager_ptah_flame_bolt") { }

        class spell_earthrager_ptah_flame_bolt_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_earthrager_ptah_flame_bolt_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                Skyfire::Containers::RandomResizeList(targets, GetCaster()->GetMap()->IsHeroic() ? 3 : 2);
            }

            void Register() OVERRIDE
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_earthrager_ptah_flame_bolt_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const OVERRIDE
        {
            return new spell_earthrager_ptah_flame_bolt_SpellScript();
        }
};

class spell_earthrager_ptah_explosion : public SpellScriptLoader
{
public:
    spell_earthrager_ptah_explosion() : SpellScriptLoader("spell_earthrager_ptah_explosion") { }

    class spell_earthrager_ptah_explosion_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_earthrager_ptah_explosion_AuraScript);

        void SetFlags(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (Unit* ptah = GetCaster())
            {
                ptah->SetFlag(UNIT_FIELD_FLAGS, uint32(UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_POWERS | UNIT_FLAG_UNK_31));
                ptah->SetFlag(UNIT_FIELD_FLAGS2, UNIT_FLAG2_FEIGN_DEATH);
            }
        }

        void RemoveFlags(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (Unit* ptah = GetCaster())
            {
                ptah->RemoveFlag(UNIT_FIELD_FLAGS, uint32(UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_POWERS | UNIT_FLAG_UNK_31));
                ptah->RemoveFlag(UNIT_FIELD_FLAGS2, UNIT_FLAG2_FEIGN_DEATH);
            }
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_earthrager_ptah_explosion_AuraScript::SetFlags, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(spell_earthrager_ptah_explosion_AuraScript::RemoveFlags, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_earthrager_ptah_explosion_AuraScript();
    }
};

void AddSC_boss_earthrager_ptah()
{
    new boss_earthrager_ptah();
    new spell_earthrager_ptah_flame_bolt();
    new spell_earthrager_ptah_explosion();
}
