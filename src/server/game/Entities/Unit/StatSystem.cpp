/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "Creature.h"
#include "Pet.h"
#include "Player.h"
#include "SharedDefines.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "Unit.h"
#include "World.h"
#include <numeric>

inline bool _ModifyUInt32(bool apply, uint32& baseValue, int32& amount)
{
    // If amount is negative, change sign and value of apply.
    if (amount < 0)
    {
        apply = !apply;
        amount = -amount;
    }
    if (apply)
        baseValue += amount;
    else
    {
        // Make sure we do not get uint32 overflow.
        if (amount > int32(baseValue))
            amount = baseValue;
        baseValue -= amount;
    }
    return apply;
}

/*#######################################
########                         ########
########   PLAYERS STAT SYSTEM   ########
########                         ########
#######################################*/

bool Player::UpdateStats(Stats stat)
{
    if (stat > STAT_SPIRIT)
        return false;

    // value = ((base_value * base_pct) + total_value) * total_pct
    float value = GetTotalStatValue(stat);

    SetStat(stat, int32(value));

    if (stat == STAT_STAMINA || stat == STAT_INTELLECT || stat == STAT_STRENGTH)
    {
        Pet* pet = GetPet();
        if (pet)
            pet->UpdateStats(stat);
    }

    switch (stat)
    {
        case STAT_AGILITY:
            UpdateArmor();
            UpdateAllCritPercentages();
            UpdateDodgePercentage();
            break;
        case STAT_STAMINA:
            UpdateMaxHealth();
            break;
        case STAT_INTELLECT:
            UpdateAllSpellCritChances();
            UpdateArmor();                                  //SPELL_AURA_MOD_RESISTANCE_OF_INTELLECT_PERCENT, only armor currently
            break;
        case STAT_SPIRIT:
            break;
        default:
            break;
    }

    if (stat == STAT_STRENGTH)
        UpdateAttackPowerAndDamage(false);
    else if (stat == STAT_AGILITY)
    {
        UpdateAttackPowerAndDamage(false);
        UpdateAttackPowerAndDamage(true);
    }

    UpdateSpellDamageAndHealingBonus();
    UpdateManaRegen();

    // Update ratings in exist SPELL_AURA_MOD_RATING_FROM_STAT and only depends from stat
    uint32 mask = 0;
    AuraEffectList const& modRatingFromStat = GetAuraEffectsByType(SPELL_AURA_MOD_RATING_FROM_STAT);
    for (AuraEffectList::const_iterator i = modRatingFromStat.begin(); i != modRatingFromStat.end(); ++i)
        if (Stats((*i)->GetMiscValueB()) == stat)
            mask |= (*i)->GetMiscValue();
    if (mask)
    {
        for (uint32 rating = 0; rating < MAX_COMBAT_RATING; ++rating)
            if (mask & (1 << rating))
                ApplyRatingMod(CombatRating(rating), 0, true);
    }
    return true;
}

void Player::UpdatePvpPower()
{
    float precision = 100.0f;
    float value = GetRatingBonusValue(CombatRating::CR_PVP_POWER);
    value += GetTotalAuraModifier(SPELL_AURA_MOD_RATING);

    float pvpHealing = value / precision;
    float pvpDamage = value / precision;

    SetFloatValue(PLAYER_FIELD_PVP_POWER_HEALING, pvpHealing);
    SetFloatValue(PLAYER_FIELD_PVP_POWER_DAMAGE, pvpDamage);
}

void Player::ApplySpellPowerBonus(int32 amount, bool apply)
{
    if (HasAuraType(SPELL_AURA_OVERRIDE_SPELL_POWER_BY_AP_PCT))
        return;

    apply = _ModifyUInt32(apply, m_baseSpellPower, amount);

    // For speed just update for client
    ApplyModUInt32Value(PLAYER_FIELD_MOD_HEALING_DONE_POS, amount, apply);
    for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
        ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + i, amount, apply);

    if (HasAuraType(SPELL_AURA_OVERRIDE_AP_BY_SPELL_POWER_PCT))
    {
        UpdateAttackPowerAndDamage();
        UpdateAttackPowerAndDamage(true);
    }
}

void Player::UpdateSpellDamageAndHealingBonus()
{
    // Magic damage modifiers implemented in Unit::SpellDamageBonusDone
    // This information for client side use only
    // Get healing bonus for all schools
    SetStatInt32Value(PLAYER_FIELD_MOD_HEALING_DONE_POS, SpellBaseHealingBonusDone(SPELL_SCHOOL_MASK_ALL));
    // Get damage bonus for all schools
    Unit::AuraEffectList const& modDamageAuras = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE);
    for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
    {
        SetStatInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + i, std::accumulate(modDamageAuras.begin(), modDamageAuras.end(), 0, [i](int32 negativeMod, AuraEffect const* aurEff)
            {
                if (aurEff->GetAmount() < 0 && aurEff->GetMiscValue() & (1 << i))
                    negativeMod += aurEff->GetAmount();
                return negativeMod;
            }));
        SetStatInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + i, SpellBaseDamageBonusDone(SpellSchoolMask(1 << i)) - GetInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + i));
    }
    if (HasAuraType(SPELL_AURA_OVERRIDE_AP_BY_SPELL_POWER_PCT))
    {
        UpdateAttackPowerAndDamage();
        UpdateAttackPowerAndDamage(true);
    }
}

bool Player::UpdateAllStats()
{
    for (int8 i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        float value = GetTotalStatValue(Stats(i));
        SetStat(Stats(i), int32(value));
    }

    UpdateArmor();
    // calls UpdateAttackPowerAndDamage() in UpdateArmor for SPELL_AURA_MOD_ATTACK_POWER_OF_ARMOR
    UpdateAttackPowerAndDamage(true);
    UpdateMaxHealth();

    for (uint8 i = POWER_MANA; i < MAX_POWERS; ++i)
        UpdateMaxPower(Powers(i));

    UpdateAllRatings();
    UpdateAllCritPercentages();
    UpdateAllSpellCritChances();
    UpdateBlockPercentage();
    UpdateParryPercentage();
    UpdateDodgePercentage();
    UpdateSpellDamageAndHealingBonus();
    UpdateManaRegen();
    UpdateExpertise(WeaponAttackType::BASE_ATTACK);
    UpdateExpertise(WeaponAttackType::OFF_ATTACK);
    RecalculateRating(CombatRating::CR_ARMOR_PENETRATION);
    for (int i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        UpdateResistances(i);

    UpdateMastery();
    UpdatePvpPower();

    return true;
}

void Player::ApplySpellPenetrationBonus(int32 amount, bool apply)
{
    ApplyModInt32Value(PLAYER_FIELD_MOD_TARGET_RESISTANCE, -amount, apply);
    m_spellPenetrationItemMod += apply ? amount : -amount;
}

void Player::UpdateResistances(uint32 school)
{
    if (school > SPELL_SCHOOL_NORMAL)
    {
        float value = GetTotalAuraModValue(UnitMods(UNIT_MOD_RESISTANCE_START + school));
        SetResistance(SpellSchools(school), int32(value));

        Pet* pet = GetPet();
        if (pet)
            pet->UpdateResistances(school);
    }
    else
        UpdateArmor();
}

void Player::UpdateArmor()
{
    UnitMods unitMod = UNIT_MOD_ARMOR;

    float value = GetModifierValue(unitMod, BASE_VALUE);    // base armor (from items)
    value *= GetModifierValue(unitMod, BASE_PCT);           // armor percent from items
    value += GetModifierValue(unitMod, TOTAL_VALUE);
    value *= GetModifierValue(unitMod, TOTAL_PCT);

    SetArmor(int32(value));

    Pet* pet = GetPet();
    if (pet)
        pet->UpdateArmor();

    UpdateAttackPowerAndDamage();                           // armor dependent auras update for SPELL_AURA_MOD_ATTACK_POWER_OF_ARMOR
}

float Player::GetHealthBonusFromStamina()
{
    // Taken from PaperDollFrame.lua - 4.3.4.15595
    float ratio = 10.0f;
    if (gtOCTHpPerStaminaEntry const* hpBase = sGtOCTHpPerStaminaStore.LookupEntry(getLevel()))
        ratio = hpBase->ratio;

    float stamina = GetStat(STAT_STAMINA);
    float baseStam = std::min(20.0f, stamina);
    float moreStam = stamina - baseStam;

    return baseStam + moreStam * ratio;
}
void Player::UpdateTalentSpecializationManaBonus()
{
    if (getPowerType() == POWER_MANA)
    {
        UpdateMaxPower(POWER_MANA);                         // Update max Mana (to add/reset bonus from specialization)
    }
}

float Player::GetManaSpecializationMultiplier()
{
    switch (GetTalentSpecialization(GetActiveSpec()))
    {
        case SPEC_PALADIN_HOLY:
        case SPEC_SHAMAN_ELEMENTAL:
        case SPEC_SHAMAN_RESTORATION:
        case SPEC_DRUID_BALANCE:
        case SPEC_DRUID_RESTORATION:
            return 5.0f;
            break;
    }
    return 0.0f;
}

void Player::UpdateMaxHealth()
{
    UnitMods unitMod = UNIT_MOD_HEALTH;

    float value = GetModifierValue(unitMod, BASE_VALUE) + GetCreateHealth();
    value *= GetModifierValue(unitMod, BASE_PCT);
    value += GetModifierValue(unitMod, TOTAL_VALUE) + GetHealthBonusFromStamina();
    value *= GetModifierValue(unitMod, TOTAL_PCT);

    SetMaxHealth((uint32)value);
}

void Player::UpdateMaxPower(Powers power)
{
    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + UnitMods(power));

    float bonusPower = (power == POWER_MANA && GetCreatePowers(power) > 0) ? GetManaSpecializationMultiplier() : 0;

    float value = GetModifierValue(unitMod, BASE_VALUE) + (GetCreatePowers(power) * (bonusPower > 0 ? bonusPower : 1));
    value *= GetModifierValue(unitMod, BASE_PCT);
    value += GetModifierValue(unitMod, TOTAL_VALUE);
    value *= GetModifierValue(unitMod, TOTAL_PCT);

    SetMaxPower(power, uint32(value));
}

void Player::UpdateAttackPowerAndDamage(bool ranged)
{
    float val2 = 0.0f;
    float level = float(getLevel());

    ChrClassesEntry const* entry = sChrClassesStore.LookupEntry(getClass());
    UnitMods unitMod = ranged ? UNIT_MOD_ATTACK_POWER_RANGED : UNIT_MOD_ATTACK_POWER;

    uint16 index = UNIT_FIELD_ATTACK_POWER;
    uint16 index_mod = UNIT_FIELD_ATTACK_POWER_MOD_POS;
    uint16 index_mult = UNIT_FIELD_ATTACK_POWER_MULTIPLIER;

    if (ranged)
    {
        index = UNIT_FIELD_RANGED_ATTACK_POWER;
        index_mod = UNIT_FIELD_RANGED_ATTACK_POWER_MOD_POS;
        index_mult = UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER;
    }
    if (!HasAuraType(SPELL_AURA_OVERRIDE_AP_BY_SPELL_POWER_PCT))
    {
        if (!ranged)
        {
            float strengthValue = std::max((GetStat(STAT_STRENGTH) - 10.0f) * entry->APPerStrenth, 0.0f);
            float agilityValue = std::max((GetStat(STAT_AGILITY) - 10.0f) * entry->APPerAgility, 0.0f);

            SpellShapeshiftFormEntry const* form = sSpellShapeshiftFormStore.LookupEntry(GetShapeshiftForm());
            // Directly taken from client, SHAPESHIFT_FLAG_AP_FROM_STRENGTH ?
            if (form && form->flags1 & 0x20)
                agilityValue += std::max((GetStat(STAT_AGILITY) - 10.0f) * entry->APPerStrenth, 0.0f);

            val2 = strengthValue + agilityValue;
        }
        else
            val2 = (level + std::max(GetStat(STAT_AGILITY) - 10.0f, 0.0f)) * entry->RAPPerAgility;
    }
    else
    {
        int32 minSpellPower = GetInt32Value(PLAYER_FIELD_MOD_HEALING_DONE_POS);
        for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            minSpellPower = std::min(minSpellPower, GetInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + i));

        val2 = CalculatePct(float(minSpellPower), GetFloatValue(PLAYER_FIELD_OVERRIDE_APBY_SPELL_POWER_PERCENT));
    }

    SetModifierValue(unitMod, BASE_VALUE, val2);

    float base_attPower = GetModifierValue(unitMod, BASE_VALUE) * GetModifierValue(unitMod, BASE_PCT);
    float attPowerMod = GetModifierValue(unitMod, TOTAL_VALUE);
    float attPowerMultiplier = GetModifierValue(unitMod, TOTAL_PCT) - 1.0f;

    SetInt32Value(index, (uint32)base_attPower);            //UNIT_FIELD_(RANGED)_ATTACK_POWER field
    SetInt32Value(index_mod, (uint32)attPowerMod);          //UNIT_FIELD_(RANGED)_ATTACK_POWER_MOD_POS field
    SetFloatValue(index_mult, attPowerMultiplier);          //UNIT_FIELD_(RANGED)_ATTACK_POWER_MULTIPLIER field

    Pet* pet = GetPet();                                //update pet's AP
    Guardian* guardian = GetGuardianPet();
    //automatically update weapon damage after attack power modification
    if (ranged)
    {
        UpdateDamagePhysical(WeaponAttackType::RANGED_ATTACK);
        if (pet && pet->IsHunterPet()) // At ranged attack change for hunter pet
            pet->UpdateAttackPowerAndDamage();
    }
    else
    {
        UpdateDamagePhysical(WeaponAttackType::BASE_ATTACK);
        if (CanDualWield() && haveOffhandWeapon())           //allow update offhand damage only if player knows DualWield Spec and has equipped offhand weapon
            UpdateDamagePhysical(WeaponAttackType::OFF_ATTACK);

        if (HasAuraType(SPELL_AURA_OVERRIDE_SPELL_POWER_BY_AP_PCT))
            UpdateSpellDamageAndHealingBonus();

        if (pet && pet->IsPetGhoul()) // At melee attack power change for DK pet
            pet->UpdateAttackPowerAndDamage();

        if (guardian && guardian->IsSpiritWolf()) // At melee attack power change for Shaman feral spirit
            guardian->UpdateAttackPowerAndDamage();
    }
}

void Player::CalculateMinMaxDamage(WeaponAttackType attType, bool normalized, bool addTotalPct, float& min_damage, float& max_damage)
{
    UnitMods unitMod;

    switch (attType)
    {
        case WeaponAttackType::BASE_ATTACK:
        default:
            unitMod = UNIT_MOD_DAMAGE_MAINHAND;
            break;
        case WeaponAttackType::OFF_ATTACK:
            unitMod = UNIT_MOD_DAMAGE_OFFHAND;
            break;
        case WeaponAttackType::RANGED_ATTACK:
            unitMod = UNIT_MOD_DAMAGE_RANGED;
            break;
    }

    float att_speed = GetAPMultiplier(attType, normalized);

    float base_value = GetModifierValue(unitMod, BASE_VALUE) + GetTotalAttackPowerValue(attType) / 14.0f * att_speed;
    float base_pct = GetModifierValue(unitMod, BASE_PCT);
    float total_value = GetModifierValue(unitMod, TOTAL_VALUE);
    float total_pct = addTotalPct ? GetModifierValue(unitMod, TOTAL_PCT) : 1.0f;

    float weapon_mindamage = GetWeaponDamageRange(attType, WeaponDamageRange::MINDAMAGE);
    float weapon_maxdamage = GetWeaponDamageRange(attType, WeaponDamageRange::MAXDAMAGE);

    if (IsInFeralForm())                                    //check if player is druid and in cat or bear forms
    {
        float weaponSpeed = BASE_ATTACK_TIME / 1000.f;
        if (Item* weapon = GetWeaponForAttack(WeaponAttackType::BASE_ATTACK, true))
            weaponSpeed = weapon->GetTemplate()->Delay / 1000;

        if (GetShapeshiftForm() == FORM_CAT)
        {
            weapon_mindamage = weapon_mindamage / weaponSpeed;
            weapon_maxdamage = weapon_maxdamage / weaponSpeed;
        }
        else if (GetShapeshiftForm() == FORM_BEAR)
        {
            weapon_mindamage = weapon_mindamage / weaponSpeed + weapon_mindamage / 2.5;
            weapon_maxdamage = weapon_mindamage / weaponSpeed + weapon_maxdamage / 2.5;
        }
    }
    else if (!CanUseAttackType(attType))      //check if player not in form but still can't use (disarm case)
    {
        //cannot use ranged/off attack, set values to 0
        if (attType != WeaponAttackType::BASE_ATTACK)
        {
            min_damage = 0;
            max_damage = 0;
            return;
        }
        weapon_mindamage = BASE_MINDAMAGE;
        weapon_maxdamage = BASE_MAXDAMAGE;
    }
    /*
    TODO: Is this still needed after ammo has been removed?
    else if (attType == RANGED_ATTACK)                       //add ammo DPS to ranged damage
    {
        weapon_mindamage += ammo * att_speed;
        weapon_maxdamage += ammo * att_speed;
    }*/

    min_damage = ((base_value + weapon_mindamage) * base_pct + total_value) * total_pct;
    max_damage = ((base_value + weapon_maxdamage) * base_pct + total_value) * total_pct;
}

void Player::UpdateDamagePhysical(WeaponAttackType attType)
{
    float mindamage;
    float maxdamage;

    CalculateMinMaxDamage(attType, false, true, mindamage, maxdamage);

    switch (attType)
    {
        case WeaponAttackType::BASE_ATTACK:
        default:
            SetStatFloatValue(UNIT_FIELD_MIN_DAMAGE, mindamage);
            SetStatFloatValue(UNIT_FIELD_MAX_DAMAGE, maxdamage);
            break;
        case WeaponAttackType::OFF_ATTACK:
            SetStatFloatValue(UNIT_FIELD_MIN_OFF_HAND_DAMAGE, mindamage);
            SetStatFloatValue(UNIT_FIELD_MAX_OFF_HAND_DAMAGE, maxdamage);
            break;
        case WeaponAttackType::RANGED_ATTACK:
            SetStatFloatValue(UNIT_FIELD_MIN_RANGED_DAMAGE, mindamage);
            SetStatFloatValue(UNIT_FIELD_MAX_RANGED_DAMAGE, maxdamage);
            break;
    }
}

void Player::UpdateBlockPercentage()
{
    // No block
    float value = 0.0f;
    if (CanBlock())
    {
        // Base value
        value = 5.0f;
        // Increase from SPELL_AURA_MOD_BLOCK_PERCENT aura
        value += GetTotalAuraModifier(SPELL_AURA_MOD_BLOCK_PERCENT);
        // Increase from rating
        value += GetRatingBonusValue(CombatRating::CR_BLOCK);

        if (sWorld->GetBoolConfig(WorldBoolConfigs::CONFIG_STATS_LIMITS_ENABLE))
            value = value > sWorld->GetFloatConfig(WorldFloatConfigs::CONFIG_STATS_LIMITS_BLOCK) ? sWorld->GetFloatConfig(WorldFloatConfigs::CONFIG_STATS_LIMITS_BLOCK) : value;

        value = value < 0.0f ? 0.0f : value;
    }
    SetStatFloatValue(PLAYER_FIELD_BLOCK_PERCENTAGE, value);
}

void Player::UpdateCritPercentage(WeaponAttackType attType)
{
    BaseModGroup modGroup;
    uint16 index;
    CombatRating cr;

    switch (attType)
    {
        case WeaponAttackType::OFF_ATTACK:
            modGroup = OFFHAND_CRIT_PERCENTAGE;
            index = PLAYER_FIELD_OFFHAND_CRIT_PERCENTAGE;
            cr = CombatRating::CR_CRIT_MELEE;
            break;
        case WeaponAttackType::RANGED_ATTACK:
            modGroup = RANGED_CRIT_PERCENTAGE;
            index = PLAYER_FIELD_RANGED_CRIT_PERCENTAGE;
            cr = CombatRating::CR_CRIT_RANGED;
            break;
        case WeaponAttackType::BASE_ATTACK:
        default:
            modGroup = CRIT_PERCENTAGE;
            index = PLAYER_FIELD_CRIT_PERCENTAGE;
            cr = CombatRating::CR_CRIT_MELEE;
            break;
    }

    float value = GetTotalPercentageModValue(modGroup) + GetRatingBonusValue(cr);
    // Modify crit from weapon skill and maximized defense skill of same level victim difference
    value += (int32(GetMaxSkillValueForLevel()) - int32(GetMaxSkillValueForLevel())) * 0.04f;

    if (sWorld->GetBoolConfig(WorldBoolConfigs::CONFIG_STATS_LIMITS_ENABLE))
        value = value > sWorld->GetFloatConfig(WorldFloatConfigs::CONFIG_STATS_LIMITS_CRIT) ? sWorld->GetFloatConfig(WorldFloatConfigs::CONFIG_STATS_LIMITS_CRIT) : value;

    value = value < 0.0f ? 0.0f : value;
    SetStatFloatValue(index, value);
}

void Player::UpdateAllCritPercentages()
{
    float value = GetMeleeCritFromAgility();

    SetBaseModValue(CRIT_PERCENTAGE, PCT_MOD, value);
    SetBaseModValue(OFFHAND_CRIT_PERCENTAGE, PCT_MOD, value);
    SetBaseModValue(RANGED_CRIT_PERCENTAGE, PCT_MOD, value);

    UpdateCritPercentage(WeaponAttackType::BASE_ATTACK);
    UpdateCritPercentage(WeaponAttackType::OFF_ATTACK);
    UpdateCritPercentage(WeaponAttackType::RANGED_ATTACK);
}

void Player::UpdateMastery()
{
    float value = 8.0f;
    value += GetTotalAuraModifier(SPELL_AURA_MASTERY);
    value += GetRatingBonusValue(CombatRating::CR_MASTERY);
    SetFloatValue(PLAYER_FIELD_MASTERY, value);
}


const float m_diminishing_k[MAX_CLASSES] =
{
    0.9560f,  // Warrior
    0.9560f,  // Paladin
    0.9880f,  // Hunter
    0.9880f,  // Rogue
    0.9830f,  // Priest
    0.9560f,  // DK
    0.9880f,  // Shaman
    0.9830f,  // Mage
    0.9830f,  // Warlock
    0.9880f,  // Monk
    0.9720f   // Druid
};

void Player::UpdateParryPercentage()
{
    const float parry_cap[MAX_CLASSES] =
    {
        65.631440f,     // Warrior
        65.631440f,     // Paladin
        145.560408f,    // Hunter
        145.560408f,    // Rogue
        0.0f,           // Priest
        65.631440f,     // DK
        145.560408f,    // Shaman
        0.0f,           // Mage
        0.0f,           // Warlock
        0.0f,           // Monk
        0.0f            // Druid
    };

    // No parry
    float value = 0.0f;
    uint32 pclass = getClass() - 1;
    if (CanParry() && parry_cap[pclass] > 0.0f)
    {
        float nondiminishing = 5.0f;
        // Parry from rating
        float diminishing = GetRatingBonusValue(CombatRating::CR_PARRY);
        // Parry from SPELL_AURA_MOD_PARRY_PERCENT aura
        nondiminishing += GetTotalAuraModifier(SPELL_AURA_MOD_PARRY_PERCENT);
        // apply diminishing formula to diminishing parry chance
        value = nondiminishing + diminishing * parry_cap[pclass] / (diminishing + parry_cap[pclass] * m_diminishing_k[pclass]);

        if (sWorld->GetBoolConfig(WorldBoolConfigs::CONFIG_STATS_LIMITS_ENABLE))
            value = value > sWorld->GetFloatConfig(WorldFloatConfigs::CONFIG_STATS_LIMITS_PARRY) ? sWorld->GetFloatConfig(WorldFloatConfigs::CONFIG_STATS_LIMITS_PARRY) : value;

        value = value < 0.0f ? 0.0f : value;
    }
    SetStatFloatValue(PLAYER_FIELD_PARRY_PERCENTAGE, value);
}

void Player::UpdateDodgePercentage()
{
    const float dodge_cap[MAX_CLASSES] =
    {
        65.631440f,     // Warrior
        65.631440f,     // Paladin
        145.560408f,    // Hunter
        145.560408f,    // Rogue
        150.375940f,    // Priest
        65.631440f,     // DK
        145.560408f,    // Shaman
        150.375940f,    // Mage
        150.375940f,    // Warlock
        145.560408f,    // Monk
        116.890707f     // Druid
    };

    float diminishing = 0.0f, nondiminishing = 0.0f;
    GetDodgeFromAgility(diminishing, nondiminishing);
    // Dodge from SPELL_AURA_MOD_DODGE_PERCENT aura
    nondiminishing += GetTotalAuraModifier(SPELL_AURA_MOD_DODGE_PERCENT);
    // Dodge from rating
    diminishing += GetRatingBonusValue(CombatRating::CR_DODGE);
    // apply diminishing formula to diminishing dodge chance
    uint32 pclass = getClass() - 1;
    float value = nondiminishing + (diminishing * dodge_cap[pclass] / (diminishing + dodge_cap[pclass] * m_diminishing_k[pclass]));

    if (sWorld->GetBoolConfig(WorldBoolConfigs::CONFIG_STATS_LIMITS_ENABLE))
        value = value > sWorld->GetFloatConfig(WorldFloatConfigs::CONFIG_STATS_LIMITS_DODGE) ? sWorld->GetFloatConfig(WorldFloatConfigs::CONFIG_STATS_LIMITS_DODGE) : value;

    value = value < 0.0f ? 0.0f : value;
    SetStatFloatValue(PLAYER_FIELD_DODGE_PERCENTAGE, value);
}

void Player::UpdateSpellCritChance(uint32 school)
{
    // For normal school set zero crit chance
    if (school == SPELL_SCHOOL_NORMAL)
    {
        SetFloatValue(PLAYER_FIELD_SPELL_CRIT_PERCENTAGE, 0.0f);
        return;
    }
    // For others recalculate it from:
    float crit = 0.0f;
    // Crit from Intellect
    crit += GetSpellCritFromIntellect();
    // Increase crit from SPELL_AURA_MOD_SPELL_CRIT_CHANCE
    crit += GetTotalAuraModifier(SPELL_AURA_MOD_SPELL_CRIT_CHANCE);
    // Increase crit from SPELL_AURA_MOD_CRIT_PCT
    crit += GetTotalAuraModifier(SPELL_AURA_MOD_CRIT_PCT);
    // Increase crit from spell crit ratings
    crit += GetRatingBonusValue(CombatRating::CR_CRIT_SPELL);

    // Store crit value
    SetFloatValue(PLAYER_FIELD_SPELL_CRIT_PERCENTAGE + school, crit);
}

void Player::UpdateArmorPenetration(int32 amount)
{
    // Store Rating Value
    SetUInt32Value(PLAYER_FIELD_COMBAT_RATINGS + uint8(CombatRating::CR_ARMOR_PENETRATION), amount);
}

void Player::UpdateMeleeHitChances()
{
    m_modMeleeHitChance = (float)GetTotalAuraModifier(SPELL_AURA_MOD_HIT_CHANCE);
    m_modMeleeHitChance += GetRatingBonusValue(CombatRating::CR_HIT_MELEE);
}

void Player::UpdateRangedHitChances()
{
    m_modRangedHitChance = (float)GetTotalAuraModifier(SPELL_AURA_MOD_HIT_CHANCE);
    m_modRangedHitChance += GetRatingBonusValue(CombatRating::CR_HIT_RANGED);
}

void Player::UpdateSpellHitChances()
{
    m_modSpellHitChance = (float)GetTotalAuraModifier(SPELL_AURA_MOD_SPELL_HIT_CHANCE);
    m_modSpellHitChance += GetRatingBonusValue(CombatRating::CR_HIT_SPELL);
}

void Player::UpdateAllSpellCritChances()
{
    for (int i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        UpdateSpellCritChance(i);
}

void Player::UpdateExpertise(WeaponAttackType attack)
{
    int32 expertise = int32(GetRatingBonusValue(CombatRating::CR_EXPERTISE));

    Item* weapon = GetWeaponForAttack(attack, true);

    AuraEffectList const& expAuras = GetAuraEffectsByType(SPELL_AURA_MOD_EXPERTISE);
    for (AuraEffectList::const_iterator itr = expAuras.begin(); itr != expAuras.end(); ++itr)
    {
        // item neutral spell
        if ((*itr)->GetSpellInfo()->EquippedItemClass == -1)
            expertise += (*itr)->GetAmount();
        // item dependent spell
        else if (weapon && weapon->IsFitToSpellRequirements((*itr)->GetSpellInfo()))
            expertise += (*itr)->GetAmount();
    }

    if (expertise < 0)
        expertise = 0;

    switch (attack)
    {
        case WeaponAttackType::BASE_ATTACK: SetUInt32Value(PLAYER_FIELD_MAINHAND_EXPERTISE, expertise); break;
        case WeaponAttackType::OFF_ATTACK:  SetUInt32Value(PLAYER_FIELD_OFFHAND_EXPERTISE, expertise); break;
        case WeaponAttackType::RANGED_ATTACK: SetUInt32Value(PLAYER_FIELD_RANGED_EXPERTISE, expertise); break;
        default: break;
    }
}

void Player::ApplyManaRegenBonus(int32 amount, bool apply)
{
    _ModifyUInt32(apply, m_baseManaRegen, amount);
    UpdateManaRegen();
}

void Player::ApplyHealthRegenBonus(int32 amount, bool apply)
{
    _ModifyUInt32(apply, m_baseHealthRegen, amount);
}

void Player::UpdateManaRegen()
{
    /* -- 5.4.8 PAPERDOLL
    local base, casting = GetManaRegen();
    --All mana regen stats are displayed as mana / 5 sec.
        base = floor(base * 5.0);
    casting = BreakUpLargeNumbers(floor(casting * 5.0));
    text:SetText(casting);
    statFrame.tooltip = HIGHLIGHT_FONT_COLOR_CODE ..MANA_REGEN_COMBAT ..FONT_COLOR_CODE_CLOSE;
    statFrame.tooltip2 = format(MANA_COMBAT_REGEN_TOOLTIP, casting);
    statFrame:Show();
    */
    // base regen since pandaria is 2% aka 6000 MP @90.
    float base_regen = GetMaxPower(POWER_MANA) * 0.02f;
    // Mana regen from spirit
    float spirit_regen = OCTRegenMPPerSpirit();
    // Apply PCT bonus from SPELL_AURA_MOD_POWER_REGEN_PERCENT aura on spirit base regen
    spirit_regen *= GetTotalAuraMultiplierByMiscValue(SPELL_AURA_MOD_POWER_REGEN_PERCENT, POWER_MANA);

    //Apply FLAT bonus from SPELL_AURA_MOD_POWER_REGEN 
    spirit_regen += GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_POWER_REGEN, POWER_MANA);

    // mp5
    spirit_regen /= 5.0f;
    base_regen /= 5.0f;

    // Set regen rate in cast state apply only on spirit based regen
    int32 modManaRegenInterrupt = GetTotalAuraModifier(SPELL_AURA_MOD_MANA_REGEN_INTERRUPT);


    SetStatFloatValue(UNIT_FIELD_POWER_REGEN_INTERRUPTED_FLAT_MODIFIER, base_regen + CalculatePct(spirit_regen, modManaRegenInterrupt));
    SetStatFloatValue(UNIT_FIELD_POWER_REGEN_FLAT_MODIFIER, base_regen + spirit_regen);
}

void Player::UpdateRuneRegen(RuneType rune)
{
    if (rune >= RuneType::NUM_RUNE_TYPES)
        return;

    uint32 cooldown = 0;

    for (uint32 i = 0; i < MAX_RUNES; ++i)
        if (GetBaseRune(i) == rune)
        {
            cooldown = GetRuneBaseCooldown(i);
            break;
        }

    if (cooldown <= 0)
        return;

    float regen = float(1 * IN_MILLISECONDS) / float(cooldown);
    SetFloatValue(PLAYER_FIELD_RUNE_REGEN + uint8(rune), regen);
}

void Player::UpdateAllRunesRegen()
{
    for (uint8 i = 0; i < uint8(RuneType::NUM_RUNE_TYPES); ++i)
        if (uint32 cooldown = GetRuneTypeBaseCooldown(RuneType(i)))
            SetFloatValue(PLAYER_FIELD_RUNE_REGEN + i, float(1 * IN_MILLISECONDS) / float(cooldown));
}

void Player::_ApplyAllStatBonuses()
{
    SetCanModifyStats(false);

    _ApplyAllAuraStatMods();
    _ApplyAllItemMods();

    SetCanModifyStats(true);

    UpdateAllStats();
}

void Player::_RemoveAllStatBonuses()
{
    SetCanModifyStats(false);

    _RemoveAllItemMods();
    _RemoveAllAuraStatMods();

    SetCanModifyStats(true);

    UpdateAllStats();
}

/*#######################################
########                         ########
########    MOBS STAT SYSTEM     ########
########                         ########
#######################################*/

bool Creature::UpdateStats(Stats /*stat*/)
{
    return true;
}

bool Creature::UpdateAllStats()
{
    UpdateMaxHealth();
    UpdateAttackPowerAndDamage();
    UpdateAttackPowerAndDamage(true);

    for (uint8 i = POWER_MANA; i < MAX_POWERS; ++i)
        UpdateMaxPower(Powers(i));

    for (int8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        UpdateResistances(i);

    return true;
}

void Creature::UpdateResistances(uint32 school)
{
    if (school > SPELL_SCHOOL_NORMAL)
    {
        float value = GetTotalAuraModValue(UnitMods(UNIT_MOD_RESISTANCE_START + school));
        SetResistance(SpellSchools(school), int32(value));
    }
    else
        UpdateArmor();
}

void Creature::UpdateArmor()
{
    float value = GetTotalAuraModValue(UNIT_MOD_ARMOR);
    SetArmor(int32(value));
}

void Creature::UpdateMaxHealth()
{
    float value = GetTotalAuraModValue(UNIT_MOD_HEALTH);
    SetMaxHealth((uint32)value);
}

void Creature::UpdateMaxPower(Powers power)
{
    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + UnitMods(power));

    float value = GetTotalAuraModValue(unitMod);
    SetMaxPower(power, uint32(value));
}

void Creature::UpdateAttackPowerAndDamage(bool ranged)
{
    UnitMods unitMod = ranged ? UNIT_MOD_ATTACK_POWER_RANGED : UNIT_MOD_ATTACK_POWER;

    uint16 index = UNIT_FIELD_ATTACK_POWER;
    uint16 index_mult = UNIT_FIELD_ATTACK_POWER_MULTIPLIER;

    if (ranged)
    {
        index = UNIT_FIELD_RANGED_ATTACK_POWER;
        index_mult = UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER;
    }

    float base_attPower = GetModifierValue(unitMod, BASE_VALUE) * GetModifierValue(unitMod, BASE_PCT);
    float attPowerMultiplier = GetModifierValue(unitMod, TOTAL_PCT) - 1.0f;

    SetInt32Value(index, (uint32)base_attPower);            //UNIT_FIELD_(RANGED)_ATTACK_POWER field
    SetFloatValue(index_mult, attPowerMultiplier);          //UNIT_FIELD_(RANGED)_ATTACK_POWER_MULTIPLIER field

    //automatically update weapon damage after attack power modification
    if (ranged)
        UpdateDamagePhysical(WeaponAttackType::RANGED_ATTACK);
    else
    {
        UpdateDamagePhysical(WeaponAttackType::BASE_ATTACK);
        UpdateDamagePhysical(WeaponAttackType::OFF_ATTACK);
    }
}

void Creature::UpdateDamagePhysical(WeaponAttackType attType)
{
    UnitMods unitMod;
    switch (attType)
    {
        case WeaponAttackType::BASE_ATTACK:
        default:
            unitMod = UNIT_MOD_DAMAGE_MAINHAND;
            break;
        case WeaponAttackType::OFF_ATTACK:
            unitMod = UNIT_MOD_DAMAGE_OFFHAND;
            break;
        case WeaponAttackType::RANGED_ATTACK:
            unitMod = UNIT_MOD_DAMAGE_RANGED;
            break;
    }

    //float att_speed = float(GetAttackTime(attType))/1000.0f;

    float weapon_mindamage = GetWeaponDamageRange(attType, WeaponDamageRange::MINDAMAGE);
    float weapon_maxdamage = GetWeaponDamageRange(attType, WeaponDamageRange::MAXDAMAGE);

    /* difference in AP between current attack power and base value from DB */
    float att_pwr_change = GetTotalAttackPowerValue(attType) - GetCreatureTemplate()->attackpower;
    float base_value = GetModifierValue(unitMod, BASE_VALUE) + (att_pwr_change * GetAPMultiplier(attType, false) / 14.0f);
    float base_pct = GetModifierValue(unitMod, BASE_PCT);
    float total_value = GetModifierValue(unitMod, TOTAL_VALUE);
    float total_pct = GetModifierValue(unitMod, TOTAL_PCT);
    float dmg_multiplier = GetCreatureTemplate()->dmg_multiplier;

    if (!CanUseAttackType(attType))
    {
        weapon_mindamage = 0;
        weapon_maxdamage = 0;
    }

    float mindamage = ((base_value + weapon_mindamage) * dmg_multiplier * base_pct + total_value) * total_pct;
    float maxdamage = ((base_value + weapon_maxdamage) * dmg_multiplier * base_pct + total_value) * total_pct;

    switch (attType)
    {
        case WeaponAttackType::BASE_ATTACK:
        default:
            SetStatFloatValue(UNIT_FIELD_MIN_DAMAGE, mindamage);
            SetStatFloatValue(UNIT_FIELD_MAX_DAMAGE, maxdamage);
            break;
        case WeaponAttackType::OFF_ATTACK:
            SetStatFloatValue(UNIT_FIELD_MIN_OFF_HAND_DAMAGE, mindamage);
            SetStatFloatValue(UNIT_FIELD_MAX_OFF_HAND_DAMAGE, maxdamage);
            break;
        case WeaponAttackType::RANGED_ATTACK:
            SetStatFloatValue(UNIT_FIELD_MIN_RANGED_DAMAGE, mindamage);
            SetStatFloatValue(UNIT_FIELD_MAX_RANGED_DAMAGE, maxdamage);
            break;
    }
}

/*#######################################
########                         ########
########    PETS STAT SYSTEM     ########
########                         ########
#######################################*/

#define ENTRY_IMP               416
#define ENTRY_VOIDWALKER        1860
#define ENTRY_SUCCUBUS          1863
#define ENTRY_FELHUNTER         417
#define ENTRY_FELGUARD          17252
#define ENTRY_WATER_ELEMENTAL   510
#define ENTRY_TREANT            1964
#define ENTRY_FIRE_ELEMENTAL    15438
#define ENTRY_GHOUL             26125
#define ENTRY_BLOODWORM         28017

bool Guardian::UpdateStats(Stats stat)
{
    if (stat >= MAX_STATS)
        return false;

    // value = ((base_value * base_pct) + total_value) * total_pct
    float value = GetTotalStatValue(stat);
    ApplyStatBuffMod(stat, m_statFromOwner[stat], false);
    float ownersBonus = 0.0f;

    Unit* owner = GetOwner();

    // Handle Death Knight Ghoul
    if (IsPetGhoul() && (stat == STAT_STAMINA || stat == STAT_STRENGTH))
    {
        float mod = 0.75f;
        if (stat == STAT_STAMINA)
            mod = 0.3f; // Default Owner's Stamina scale
        else
            mod = 0.7f; // Default Owner's Strength scale

        ownersBonus = float(owner->GetStat(stat)) * mod;
        value += ownersBonus;
    }
    else if (stat == STAT_STAMINA)
    {
        ownersBonus = CalculatePct(owner->GetStat(STAT_STAMINA), 30);
        value += ownersBonus;
    }
    else if (stat == STAT_INTELLECT) // warlock's and mage's pets gain 30% of owner's intellect
    {
        if (owner->getClass() == CLASS_WARLOCK || owner->getClass() == CLASS_MAGE)
        {
            ownersBonus = CalculatePct(owner->GetStat(stat), 30);
            value += ownersBonus;
        }
    }

    /*
        else if (stat == STAT_STRENGTH)
        {
            if (IsPetGhoul())
                value += float(owner->GetStat(stat)) * 0.3f;
        }
    */

    SetStat(stat, int32(value));
    m_statFromOwner[stat] = ownersBonus;
    ApplyStatBuffMod(stat, m_statFromOwner[stat], true);

    switch (stat)
    {
        case STAT_STRENGTH:         UpdateAttackPowerAndDamage();        break;
        case STAT_AGILITY:          UpdateArmor();                       break;
        case STAT_STAMINA:          UpdateMaxHealth();                   break;
        case STAT_INTELLECT:        UpdateMaxPower(POWER_MANA);          break;
        case STAT_SPIRIT:
        default:
            break;
    }

    return true;
}

bool Guardian::UpdateAllStats()
{
    for (uint8 i = STAT_STRENGTH; i < MAX_STATS; ++i)
        UpdateStats(Stats(i));

    for (uint8 i = POWER_MANA; i < MAX_POWERS; ++i)
        UpdateMaxPower(Powers(i));

    for (uint8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        UpdateResistances(i);

    return true;
}

void Guardian::UpdateResistances(uint32 school)
{
    if (school > SPELL_SCHOOL_NORMAL)
    {
        float value = GetTotalAuraModValue(UnitMods(UNIT_MOD_RESISTANCE_START + school));

        // hunter and warlock pets gain 40% of owner's resistance
        if (IsPet())
            value += float(CalculatePct(m_owner->GetResistance(SpellSchools(school)), 40));

        SetResistance(SpellSchools(school), int32(value));
    }
    else
        UpdateArmor();
}

void Guardian::UpdateArmor()
{
    float value = 0.0f;
    float bonus_armor = 0.0f;
    UnitMods unitMod = UNIT_MOD_ARMOR;

    // hunter pets gain 35% of owner's armor value, warlock pets gain 100% of owner's armor
    if (IsHunterPet())
        bonus_armor = float(CalculatePct(m_owner->GetArmor(), 70));
    else if (IsPet())
        bonus_armor = m_owner->GetArmor();

    value = GetModifierValue(unitMod, BASE_VALUE);
    value *= GetModifierValue(unitMod, BASE_PCT);
    value += GetModifierValue(unitMod, TOTAL_VALUE) + bonus_armor;
    value *= GetModifierValue(unitMod, TOTAL_PCT);

    SetArmor(int32(value));
}

void Guardian::UpdateMaxHealth()
{
    UnitMods unitMod = UNIT_MOD_HEALTH;
    float stamina = GetStat(STAT_STAMINA) - GetCreateStat(STAT_STAMINA);

    float multiplicator;
    switch (GetEntry())
    {
        case ENTRY_IMP:         multiplicator = 8.4f;   break;
        case ENTRY_VOIDWALKER:  multiplicator = 11.0f;  break;
        case ENTRY_SUCCUBUS:    multiplicator = 9.1f;   break;
        case ENTRY_FELHUNTER:   multiplicator = 9.5f;   break;
        case ENTRY_FELGUARD:    multiplicator = 11.0f;  break;
        case ENTRY_BLOODWORM:   multiplicator = 1.0f;   break;
        default:                multiplicator = 10.0f;  break;
    }

    float value = GetModifierValue(unitMod, BASE_VALUE) + GetCreateHealth();
    value *= GetModifierValue(unitMod, BASE_PCT);
    value += GetModifierValue(unitMod, TOTAL_VALUE) + stamina * multiplicator;
    value *= GetModifierValue(unitMod, TOTAL_PCT);

    SetMaxHealth((uint32)value);
}

void Guardian::UpdateMaxPower(Powers power)
{
    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + UnitMods(power));

    float addValue = (power == POWER_MANA) ? GetStat(STAT_INTELLECT) - GetCreateStat(STAT_INTELLECT) : 0.0f;
    float multiplicator = 15.0f;

    switch (GetEntry())
    {
        case ENTRY_IMP:         multiplicator = 4.95f;  break;
        case ENTRY_VOIDWALKER:
        case ENTRY_SUCCUBUS:
        case ENTRY_FELHUNTER:
        case ENTRY_FELGUARD:    multiplicator = 11.5f;  break;
        default:                multiplicator = 15.0f;  break;
    }

    float value = GetModifierValue(unitMod, BASE_VALUE) + GetCreatePowers(power);
    value *= GetModifierValue(unitMod, BASE_PCT);
    value += GetModifierValue(unitMod, TOTAL_VALUE) + addValue * multiplicator;
    value *= GetModifierValue(unitMod, TOTAL_PCT);

    SetMaxPower(power, uint32(value));
}

void Guardian::UpdateAttackPowerAndDamage(bool ranged)
{
    if (ranged)
        return;

    float val = 0.0f;
    float bonusAP = 0.0f;
    UnitMods unitMod = UNIT_MOD_ATTACK_POWER;

    if (GetEntry() == ENTRY_IMP)                                   // imp's attack power
        val = GetStat(STAT_STRENGTH) - 10.0f;
    else
        val = 2 * GetStat(STAT_STRENGTH) - 20.0f;

    Unit* owner = GetOwner();
    if (owner && owner->GetTypeId() == TypeID::TYPEID_PLAYER)
    {
        if (IsHunterPet())                      //hunter pets benefit from owner's attack power
        {
            float mod = 1.0f;                                                 //Hunter contribution modifier
            bonusAP = owner->GetTotalAttackPowerValue(WeaponAttackType::RANGED_ATTACK) * 0.22f * mod;
            SetBonusDamage(int32(owner->GetTotalAttackPowerValue(WeaponAttackType::RANGED_ATTACK) * 0.1287f * mod));
        }
        else if (IsPetGhoul()) //ghouls benefit from deathknight's attack power (may be summon pet or not)
        {
            bonusAP = owner->GetTotalAttackPowerValue(WeaponAttackType::BASE_ATTACK) * 0.22f;
            SetBonusDamage(int32(owner->GetTotalAttackPowerValue(WeaponAttackType::BASE_ATTACK) * 0.1287f));
        }
        else if (IsSpiritWolf()) //wolf benefit from shaman's attack power
        {
            float dmg_multiplier = 0.31f;
            if (m_owner->GetAuraEffect(63271, 0)) // Glyph of Feral Spirit
                dmg_multiplier = 0.61f;
            bonusAP = owner->GetTotalAttackPowerValue(WeaponAttackType::BASE_ATTACK) * dmg_multiplier;
            SetBonusDamage(int32(owner->GetTotalAttackPowerValue(WeaponAttackType::BASE_ATTACK) * dmg_multiplier));
        }
        else if (IsWhiteTiger())
        {
            bonusAP = owner->GetTotalAttackPowerValue(WeaponAttackType::BASE_ATTACK) * 0.22f;
            SetBonusDamage(int32(owner->GetTotalAttackPowerValue(WeaponAttackType::BASE_ATTACK) * 0.1287f));
        }
        //demons benefit from warlocks shadow or fire damage
        else if (IsPet())
        {
            int32 fire = int32(owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + EPlayerFields(SPELL_SCHOOL_FIRE))) + owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + EPlayerFields(SPELL_SCHOOL_FIRE));
            int32 shadow = int32(owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + EPlayerFields(SPELL_SCHOOL_SHADOW))) + owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + EPlayerFields(SPELL_SCHOOL_SHADOW));
            int32 maximum = (fire > shadow) ? fire : shadow;
            if (maximum < 0)
                maximum = 0;
            SetBonusDamage(int32(maximum * 0.15f));
            bonusAP = maximum * 0.57f;
        }
        //water elementals benefit from mage's frost damage
        else if (GetEntry() == ENTRY_WATER_ELEMENTAL)
        {
            int32 frost = int32(owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + EPlayerFields(SPELL_SCHOOL_FROST))) + owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + EPlayerFields(SPELL_SCHOOL_FROST));
            if (frost < 0)
                frost = 0;
            SetBonusDamage(int32(frost * 0.4f));
        }
    }

    SetModifierValue(UNIT_MOD_ATTACK_POWER, BASE_VALUE, val + bonusAP);

    //in BASE_VALUE of UNIT_MOD_ATTACK_POWER for creatures we store data of meleeattackpower field in DB
    float base_attPower = GetModifierValue(unitMod, BASE_VALUE) * GetModifierValue(unitMod, BASE_PCT);
    float attPowerMultiplier = GetModifierValue(unitMod, TOTAL_PCT) - 1.0f;

    //UNIT_FIELD_(RANGED)_ATTACK_POWER field
    SetInt32Value(UNIT_FIELD_ATTACK_POWER, (int32)base_attPower);
    //UNIT_FIELD_(RANGED)_ATTACK_POWER_MULTIPLIER field
    SetFloatValue(UNIT_FIELD_ATTACK_POWER_MULTIPLIER, attPowerMultiplier);

    //automatically update weapon damage after attack power modification
    UpdateDamagePhysical(WeaponAttackType::BASE_ATTACK);
}

void Guardian::UpdateDamagePhysical(WeaponAttackType attType)
{
    if (attType > WeaponAttackType::BASE_ATTACK)
        return;

    float bonusDamage = 0.0f;
    if (m_owner->GetTypeId() == TypeID::TYPEID_PLAYER)
    {
        //force of nature
        if (GetEntry() == ENTRY_TREANT)
        {
            int32 spellDmg = int32(m_owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + EPlayerFields(SPELL_SCHOOL_NATURE))) + m_owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + EPlayerFields(SPELL_SCHOOL_NATURE));
            if (spellDmg > 0)
                bonusDamage = spellDmg * 0.09f;
        }
        //greater fire elemental
        else if (GetEntry() == ENTRY_FIRE_ELEMENTAL)
        {
            int32 spellDmg = int32(m_owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + EPlayerFields(SPELL_SCHOOL_FIRE))) + m_owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + EPlayerFields(SPELL_SCHOOL_FIRE));
            if (spellDmg > 0)
                bonusDamage = spellDmg * 0.4f;
        }
    }

    UnitMods unitMod = UNIT_MOD_DAMAGE_MAINHAND;

    float att_speed = float(GetAttackTime(WeaponAttackType::BASE_ATTACK)) / 1000.0f;

    float base_value = GetModifierValue(unitMod, BASE_VALUE) + GetTotalAttackPowerValue(attType) / 14.0f * att_speed + bonusDamage;
    float base_pct = GetModifierValue(unitMod, BASE_PCT);
    float total_value = GetModifierValue(unitMod, TOTAL_VALUE);
    float total_pct = GetModifierValue(unitMod, TOTAL_PCT);

    float weapon_mindamage = GetWeaponDamageRange(WeaponAttackType::BASE_ATTACK, WeaponDamageRange::MINDAMAGE);
    float weapon_maxdamage = GetWeaponDamageRange(WeaponAttackType::BASE_ATTACK, WeaponDamageRange::MAXDAMAGE);

    float mindamage = ((base_value + weapon_mindamage) * base_pct + total_value) * total_pct;
    float maxdamage = ((base_value + weapon_maxdamage) * base_pct + total_value) * total_pct;

    Unit::AuraEffectList const& mDummy = GetAuraEffectsByType(SPELL_AURA_MOD_ATTACKSPEED);
    for (Unit::AuraEffectList::const_iterator itr = mDummy.begin(); itr != mDummy.end(); ++itr)
    {
        switch ((*itr)->GetSpellInfo()->Id)
        {
            case 61682:
            case 61683:
                AddPct(mindamage, -(*itr)->GetAmount());
                AddPct(maxdamage, -(*itr)->GetAmount());
                break;
            default:
                break;
        }
    }

    SetStatFloatValue(UNIT_FIELD_MIN_DAMAGE, mindamage);
    SetStatFloatValue(UNIT_FIELD_MAX_DAMAGE, maxdamage);
}

void Guardian::SetBonusDamage(int32 damage)
{
    m_bonusSpellDamage = damage;
    if (GetOwner()->GetTypeId() == TypeID::TYPEID_PLAYER)
        GetOwner()->SetUInt32Value(PLAYER_FIELD_PET_SPELL_POWER, damage);
}
