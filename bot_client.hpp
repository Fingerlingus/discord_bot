#ifndef _BOT_CLIENT_H_
#define _BOT_CLIENT_H_

#include <array>
#include <map>
#include <vector>

#include "sleepy_discord/sleepy_discord.h"

#include "command.hpp"
#include "server_bot_settings.hpp"

class MyClientClass : public SleepyDiscord::DiscordClient {
public:
	using SleepyDiscord::DiscordClient::DiscordClient;

	Command changePrefix 	= Command("prefix",				this, Command::COMMAND_TYPE::ADMIN,		&MyClientClass::fn_changePrefix);
	Command hello 			= Command("hello",				this, Command::COMMAND_TYPE::NON_ADMIN, &MyClientClass::fn_hello);
	Command echo  			= Command("echo",				this, Command::COMMAND_TYPE::NON_ADMIN, &MyClientClass::fn_echo );
	Command muteVoice		= Command("mute voice", 		this, Command::COMMAND_TYPE::ADMIN,		&MyClientClass::fn_muteVoice);
	Command unmuteVoice		= Command("unmute voice", 		this, Command::COMMAND_TYPE::ADMIN,		&MyClientClass::fn_unmuteVoice);
	Command muteText		= Command("mute text", 			this, Command::COMMAND_TYPE::ADMIN,		&MyClientClass::fn_muteText);
	Command unmuteText		= Command("unmute text", 		this, Command::COMMAND_TYPE::ADMIN,		&MyClientClass::fn_unmuteText);
	Command kick			= Command("kick", 				this, Command::COMMAND_TYPE::ADMIN,		&MyClientClass::fn_kick);
	Command ban				= Command("ban", 				this, Command::COMMAND_TYPE::ADMIN,		&MyClientClass::fn_ban);
	Command setBotAdminRole = Command("bot_admin_role set", this, Command::COMMAND_TYPE::ADMIN,		&MyClientClass::fn_setBotAdminRole);
	Command logsDisable		= Command("logs disable", 		this, Command::COMMAND_TYPE::ADMIN,		&MyClientClass::fn_logsDisable);
	Command logsEnable		= Command("logs enable", 		this, Command::COMMAND_TYPE::ADMIN,		&MyClientClass::fn_logsDisable);
	Command setLogsChannel	= Command("logs", 				this, Command::COMMAND_TYPE::ADMIN,		&MyClientClass::fn_setLogsChannel);
	Command setSilent		= Command("silent",				this, Command::COMMAND_TYPE::ADMIN,		&MyClientClass::fn_setSilent);
	Command noSilent 		= Command("nosilent", 			this, Command::COMMAND_TYPE::ADMIN,		&MyClientClass::fn_logsDisable);
	Command deleteMsg		= Command("message delete",		this, Command::COMMAND_TYPE::ADMIN,	  	&MyClientClass::fn_deleteMsg);
	Command logAction		= Command("log_action",			this, Command::COMMAND_TYPE::ADMIN,	  	&MyClientClass::fn_logAction);
	Command setPermissions	= Command("permissions set",	this, Command::COMMAND_TYPE::ADMIN,	  	&MyClientClass::fn_setPermissions);
	Command die				= Command("die",				this, Command::COMMAND_TYPE::ADMIN,	  	&MyClientClass::fn_die);
	Command bannedOps		= Command("banned_ops",			this, Command::COMMAND_TYPE::NON_ADMIN,	&MyClientClass::fn_bannedOps);

	void onMessage(SleepyDiscord::Message aMessage) override;
	void onServer(SleepyDiscord::Server aServer) override;

    enum COMMAND_PERMISSION {
		OWNER_ONLY,
		BOT_ADMIN,
		CMD_ALL
	};

	enum COMMAND_TYPE {
		ADMIN,
		NON_ADMIN,
		ROLE_ALL
	};

	std::unordered_map<std::string, SleepyDiscord::Server> m_servers;
	std::unordered_map<std::string, ServerBotSettings> m_serverBotSettings;

protected:
	void fn_changePrefix	(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID, const std::string& acrNewPrefix);
	void fn_hello			(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID);
	void fn_echo			(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID, const std::string& acrMessage);
	void fn_muteVoice		(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser);
	void fn_unmuteVoice		(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser);
	void fn_muteText		(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser);
	void fn_unmuteText		(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser);
	void fn_kick			(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrKickedUserID, const std::string& acrReason = "");
	void fn_ban				(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrBannedUserID, const std::string& acrReason = "", int acDeleteMessageDays = 36500);
	void fn_setBotAdminRole	(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Role>& acrRoleID);
	void fn_setLogsChannel	(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID);
	void fn_logsDisable		(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser, bool b = true);
	void fn_setSilent		(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser, bool b = true);
	void fn_deleteMsg		(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser, SleepyDiscord::Message& arMessage);
	void fn_logAction		(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser, const std::string& acrString);
	void fn_setPermissions	(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser, COMMAND_PERMISSION aCommandPerm, COMMAND_TYPE aCommandType);
	void fn_die				(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID);
	void fn_bannedOps		(SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID);

private:
	static std::vector<std::string> split	(const std::string& acrString);
	static std::string getSnowflake			(const std::string& acrString);
	static COMMAND_TYPE toCommandType		(const std::string& acrString);
	static COMMAND_PERMISSION toCommandPerm	(const std::string& acrString);
		   bool isMuted						(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& arServerID, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrUserID);
};

#endif