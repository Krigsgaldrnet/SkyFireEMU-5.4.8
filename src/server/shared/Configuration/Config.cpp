/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "Config.h"
#include "Errors.h"


// Defined here as it must not be exposed to end-users.
bool ConfigMgr::GetValueHelper(const char* name, ACE_TString& result)
{
    GuardType guard(_configLock);

    if (_config.get() == 0)
        return false;

    ACE_TString section_name;
    ACE_Configuration_Section_Key section_key;
    const ACE_Configuration_Section_Key& root_key = _config->root_section();

    int i = 0;
    while (_config->enumerate_sections(root_key, i, section_name) == 0)
    {
        _config->open_section(root_key, section_name.c_str(), 0, section_key);
        if (_config->get_string_value(section_key, name, result) == 0)
            return true;
        ++i;
    }

    return false;
}

bool ConfigMgr::LoadInitial(char const* file)
{
    ASSERT(file);

    GuardType guard(_configLock);

    _filename = file;
    _config.reset(new ACE_Configuration_Heap());
    if (_config->open() == 0)
        if (LoadData(_filename.c_str()))
            return true;

    _config.reset();
    return false;
}

bool ConfigMgr::LoadMore(char const* file)
{
    ASSERT(file);
    ASSERT(_config);

    GuardType guard(_configLock);

    return LoadData(file);
}

bool ConfigMgr::Reload()
{
    return LoadInitial(_filename.c_str());
}

bool ConfigMgr::LoadData(char const* file)
{
    ACE_Ini_ImpExp config_importer(*_config.get());
    if (config_importer.import_config(file) == 0)
        return true;

    return false;
}

std::string ConfigMgr::GetStringDefault(const char* name, const std::string& def)
{
    ACE_TString val;
    return GetValueHelper(name, val) ? val.c_str() : def;
}

bool ConfigMgr::GetBoolDefault(const char* name, bool def)
{
    ACE_TString val;

    if (!GetValueHelper(name, val))
        return def;

    return (val == "true" || val == "TRUE" || val == "yes" || val == "YES" ||
        val == "1");
}

int ConfigMgr::GetIntDefault(const char* name, int def)
{
    ACE_TString val;
    return GetValueHelper(name, val) ? atoi(val.c_str()) : def;
}

float ConfigMgr::GetFloatDefault(const char* name, float def)
{
    ACE_TString val;
    return GetValueHelper(name, val) ? (float)atof(val.c_str()) : def;
}

std::string const& ConfigMgr::GetFilename()
{
    GuardType guard(_configLock);
    return _filename;
}

std::list<std::string> ConfigMgr::GetKeysByString(std::string const& name)
{
    GuardType guard(_configLock);

    std::list<std::string> keys;
    if (_config.get() == 0)
        return keys;

    ACE_TString section_name;
    ACE_Configuration_Section_Key section_key;
    const ACE_Configuration_Section_Key& root_key = _config->root_section();

    int i = 0;
    while (_config->enumerate_sections(root_key, i++, section_name) == 0)
    {
        _config->open_section(root_key, section_name.c_str(), 0, section_key);

        ACE_TString key_name;
        ACE_Configuration::VALUETYPE type;
        int j = 0;
        while (_config->enumerate_values(section_key, j++, key_name, type) == 0)
        {
            std::string temp = key_name.c_str();
            size_t pos = temp.find(name);

            if (pos != std::string::npos)
                keys.push_back(temp);
        }
    }

    return keys;
}
