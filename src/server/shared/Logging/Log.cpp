/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "AppenderConsole.h"
#include "AppenderDB.h"
#include "AppenderFile.h"
#include "Common.h"
#include "Config.h"
#include "Log.h"
#include "LogOperation.h"
#include "Util.h"

#include <cstdarg>
#include <cstdio>
#include <sstream>

Log::Log() : worker(NULL)
{
    m_logsTimestamp = "_" + GetTimestampStr();
    LoadFromConfig();
}

Log::~Log()
{
    Close();
}

uint8 Log::NextAppenderId()
{
    return AppenderId++;
}

int32 GetConfigIntDefault(std::string base, const char* name, int32 value)
{
    base.append(name);
    return sConfigMgr->GetIntDefault(base.c_str(), value);
}

std::string GetConfigStringDefault(std::string base, const char* name, const char* value)
{
    base.append(name);
    return sConfigMgr->GetStringDefault(base.c_str(), value);
}

Appender* Log::GetAppenderByName(std::string const& name)
{
    AppenderMap::iterator it = appenders.begin();
    while (it != appenders.end() && it->second && it->second->getName() != name)
        ++it;

    return it == appenders.end() ? NULL : it->second;
}

void Log::CreateAppenderFromConfig(std::string const& appenderName)
{
    if (appenderName.empty())
        return;

    // Format=type, level, flags, optional1, optional2
    // if type = File. optional1 = file and option2 = mode
    // if type = Console. optional1 = Color
    std::string options = sConfigMgr->GetStringDefault(appenderName.c_str(), "");

    Tokenizer tokens(options, ',');
    Tokenizer::const_iterator iter = tokens.begin();

    size_t size = tokens.size();
    std::string name = appenderName.substr(9);

    if (size < 2)
    {
        fprintf(stderr, "Log::CreateAppenderFromConfig: Wrong configuration for appender %s. Config line: %s\n", name.c_str(), options.c_str());
        return;
    }

    AppenderFlags flags = APPENDER_FLAGS_NONE;
    AppenderType type = AppenderType(atoi(*iter++));
    LogLevel level = LogLevel(atoi(*iter++));

    if (level > LogLevel::LOG_LEVEL_FATAL)
    {
        fprintf(stderr, "Log::CreateAppenderFromConfig: Wrong Log Level %d for appender %s\n", static_cast<int>(level), name.c_str());
        return;
    }

    if (size > 2)
        flags = AppenderFlags(atoi(*iter++));

    switch (type)
    {
    case AppenderType::APPENDER_CONSOLE:
    {
        AppenderConsole* appender = new AppenderConsole(NextAppenderId(), name, level, flags);
        appenders[appender->getId()] = appender;
        if (size > 3)
            appender->InitColors(*iter++);
        //fprintf(stdout, "Log::CreateAppenderFromConfig: Created Appender %s (%u), Type CONSOLE, Mask %u\n", appender->getName().c_str(), appender->getId(), appender->getLogLevel());
        break;
    }
    case AppenderType::APPENDER_FILE:
    {
        std::string filename;
        std::string mode = "a";

        if (size < 4)
        {
            fprintf(stderr, "Log::CreateAppenderFromConfig: Missing file name for appender %s\n", name.c_str());
            return;
        }

        filename = *iter++;

        if (size > 4)
            mode = *iter++;

        if (flags & APPENDER_FLAGS_USE_TIMESTAMP)
        {
            size_t dot_pos = filename.find_last_of(".");
            if (dot_pos != filename.npos)
                filename.insert(dot_pos, m_logsTimestamp);
            else
                filename += m_logsTimestamp;
        }

        uint64 maxFileSize = 0;
        if (size > 5)
            maxFileSize = atoi(*iter++);

        uint8 id = NextAppenderId();
        appenders[id] = new AppenderFile(id, name, level, filename.c_str(), m_logsDir.c_str(), mode.c_str(), flags, maxFileSize);
        //fprintf(stdout, "Log::CreateAppenderFromConfig: Created Appender %s (%u), Type FILE, Mask %u, File %s, Mode %s\n", name.c_str(), id, level, filename.c_str(), mode.c_str());
        break;
    }
    case AppenderType::APPENDER_DB:
    {
        uint8 id = NextAppenderId();
        appenders[id] = new AppenderDB(id, name, level);
        break;
    }
    default:
        fprintf(stderr, "Log::CreateAppenderFromConfig: Unknown type %d for appender %s\n", static_cast<int>(type), name.c_str());
        break;
    }
}

void Log::CreateLoggerFromConfig(std::string const& appenderName)
{
    if (appenderName.empty())
        return;

    LogLevel level = LogLevel::LOG_LEVEL_DISABLED;
    uint8 type = uint8(-1);

    std::string options = sConfigMgr->GetStringDefault(appenderName.c_str(), "");
    std::string name = appenderName.substr(7);

    if (options.empty())
    {
        fprintf(stderr, "Log::CreateLoggerFromConfig: Missing config option Logger.%s\n", name.c_str());
        return;
    }

    Tokenizer tokens(options, ',');
    Tokenizer::const_iterator iter = tokens.begin();

    if (tokens.size() != 2)
    {
        fprintf(stderr, "Log::CreateLoggerFromConfig: Wrong config option Logger.%s=%s\n", name.c_str(), options.c_str());
        return;
    }

    Logger& logger = loggers[name];
    if (!logger.getName().empty())
    {
        fprintf(stderr, "Error while configuring Logger %s. Already defined\n", name.c_str());
        return;
    }

    level = LogLevel(atoi(*iter++));
    if (level > LogLevel::LOG_LEVEL_FATAL)
    {
        fprintf(stderr, "Log::CreateLoggerFromConfig: Wrong Log Level %u for logger %s\n", type, name.c_str());
        return;
    }

    logger.Create(name, level);
    //fprintf(stdout, "Log::CreateLoggerFromConfig: Created Logger %s, Level %u\n", name.c_str(), level);

    std::istringstream ss(*iter);
    std::string str;

    ss >> str;
    while (ss)
    {
        if (Appender* appender = GetAppenderByName(str))
        {
            logger.addAppender(appender->getId(), appender);
            //fprintf(stdout, "Log::CreateLoggerFromConfig: Added Appender %s to Logger %s\n", appender->getName().c_str(), name.c_str());
        }
        else
            fprintf(stderr, "Error while configuring Appender %s in Logger %s. Appender does not exist", str.c_str(), name.c_str());
        ss >> str;
    }
}

void Log::ReadAppendersFromConfig()
{
    std::list<std::string> keys = sConfigMgr->GetKeysByString("Appender.");

    while (!keys.empty())
    {
        CreateAppenderFromConfig(keys.front());
        keys.pop_front();
    }
}

void Log::ReadLoggersFromConfig()
{
    std::list<std::string> keys = sConfigMgr->GetKeysByString("Logger.");

    while (!keys.empty())
    {
        CreateLoggerFromConfig(keys.front());
        keys.pop_front();
    }

    // Bad config configuration, creating default config
    if (loggers.find(LOGGER_ROOT) == loggers.end())
    {
        fprintf(stderr, "Wrong Loggers configuration. Review your Logger config section.\n"
            "Creating default loggers [root (Error), server (Info)] to console\n");

        Close(); // Clean any Logger or Appender created

        AppenderConsole* appender = new AppenderConsole(NextAppenderId(), "Console", LogLevel::LOG_LEVEL_DEBUG, APPENDER_FLAGS_NONE);
        appenders[appender->getId()] = appender;

        Logger& logger = loggers[LOGGER_ROOT];
        logger.Create(LOGGER_ROOT, LogLevel::LOG_LEVEL_ERROR);
        logger.addAppender(appender->getId(), appender);

        logger = loggers["server"];
        logger.Create("server", LogLevel::LOG_LEVEL_ERROR);
        logger.addAppender(appender->getId(), appender);
    }
}

void Log::vlog(std::string const& filter, LogLevel level, char const* str, va_list argptr)
{
    char text[MAX_QUERY_LEN];
    vsnprintf(text, MAX_QUERY_LEN, str, argptr);
    write(new LogMessage(level, filter, text));
}

void Log::write(LogMessage* msg) const
{
    Logger const* logger = GetLoggerByType(msg->type);
    msg->text.append("\n");

    if (worker)
        worker->enqueue(new LogOperation(logger, msg));
    else
    {
        logger->write(*msg);
        delete msg;
    }
}

std::string Log::GetTimestampStr()
{
    time_t t = time(NULL);
    tm aTm;
    ACE_OS::localtime_r(&t, &aTm);
    char buf[20];
    snprintf(buf, sizeof(buf), "%u-%u-%u_%u-%u-%u", uint32(aTm.tm_year + 1900), uint32(aTm.tm_mon + 1), uint32(aTm.tm_mday), uint32(aTm.tm_hour), uint32(aTm.tm_min), uint32(aTm.tm_sec));
    return std::string(buf);
}

bool Log::SetLogLevel(std::string const& name, const char* newLevelc, bool isLogger /* = true */)
{
    LogLevel newLevel = LogLevel(atoi(newLevelc));
    if (newLevel < LogLevel::LOG_LEVEL_DISABLED)
        return false;

    if (isLogger)
    {
        LoggerMap::iterator it = loggers.begin();
        while (it != loggers.end() && it->second.getName() != name)
            ++it;

        if (it == loggers.end())
            return false;

        it->second.setLogLevel(newLevel);
    }
    else
    {
        Appender* appender = GetAppenderByName(name);
        if (!appender)
            return false;

        appender->setLogLevel(newLevel);
    }

    return true;
}

void Log::outCharDump(char const* str, uint32 accountId, uint32 guid, char const* name)
{
    if (!str || !ShouldLog("entities.player.dump", LogLevel::LOG_LEVEL_INFO))
        return;

    std::ostringstream ss;
    ss << "== START DUMP == (account: " << accountId << " guid: " << guid << " name: " << name
        << ")\n" << str << "\n== END DUMP ==\n";

    LogMessage* msg = new LogMessage(LogLevel::LOG_LEVEL_INFO, "entities.player.dump", ss.str());
    std::ostringstream param;
    param << guid << '_' << name;

    msg->param1 = param.str();

    write(msg);
}

void Log::outCommand(uint32 account, const char* str, ...)
{
    if (!str || !ShouldLog("commands.gm", LogLevel::LOG_LEVEL_INFO))
        return;

    va_list ap;
    va_start(ap, str);
    char text[MAX_QUERY_LEN];
    vsnprintf(text, MAX_QUERY_LEN, str, ap);
    va_end(ap);

    LogMessage* msg = new LogMessage(LogLevel::LOG_LEVEL_INFO, "commands.gm", text);

    std::ostringstream ss;
    ss << account;
    msg->param1 = ss.str();

    write(msg);
}

void Log::SetRealmId(uint32 id)
{
    for (AppenderMap::iterator it = appenders.begin(); it != appenders.end(); ++it)
        if (it->second && it->second->getType() == AppenderType::APPENDER_DB)
            ((AppenderDB*)it->second)->setRealmId(id);
}

void Log::Close()
{
    delete worker;
    worker = NULL;
    loggers.clear();
    for (AppenderMap::iterator it = appenders.begin(); it != appenders.end(); ++it)
    {
        delete it->second;
        it->second = NULL;
    }
    appenders.clear();
}

void Log::LoadFromConfig()
{
    Close();

    if (sConfigMgr->GetBoolDefault("Log.Async.Enable", false))
        worker = new LogWorker();

    AppenderId = 0;
    m_logsDir = sConfigMgr->GetStringDefault("LogsDir", "");
    if (!m_logsDir.empty())
        if ((m_logsDir.at(m_logsDir.length() - 1) != '/') && (m_logsDir.at(m_logsDir.length() - 1) != '\\'))
            m_logsDir.push_back('/');

    ReadAppendersFromConfig();
    ReadLoggersFromConfig();
}
