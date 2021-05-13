#include "bot_client.hpp"
#include "mention.hpp"

void MyClientClass::init() {
	s_botID = getID();
	try {
		readServerInfo();
	}
	
	catch (const std::runtime_error& e) {
		std::fprintf(stderr, "init(): %s\n", e.what());
		return;
	}
}

void MyClientClass::readConfig() {
	std::mutex mutex;
	std::lock_guard lock(mutex);

	if(!std::ifstream("../config.json")) {
		std::fprintf(stderr, "readConfig(): config.json not found or failed to open, exiting.\n");
		std::exit(2);
	}
	std::ifstream lConfigFile("../config.json");
	std::stringstream lSS;
	lSS << lConfigFile.rdbuf();
	if(lSS.str().empty()) {
		std::fprintf(stderr, "readConfig(): config.json empty, exiting.\n");
		std::exit(5);
	}
	m_configJSON = lSS.str();
}

void MyClientClass::readServerInfo() {
	std::mutex mutex;
	std::lock_guard lock(mutex);

	// if server_info.json doesn't exist, create and return early since it will have no data to be parsed
	if(!std::ifstream("../server_info.json")) {
		rapidjson::Document lDoc;
		rapidjson::Value lStringVal;
		rapidjson::Value lServerInfoVal;
		lStringVal.SetString("bot");
		lServerInfoVal.SetObject();
		lDoc.AddMember("project", lStringVal, lDoc.GetAllocator());
		lDoc.AddMember("serverInfo", lServerInfoVal, lDoc.GetAllocator());
		
		// stringify document
		rapidjson::StringBuffer lStringBuffer;
		rapidjson::Writer<rapidjson::StringBuffer> lWriter(lStringBuffer);
		lDoc.Accept(lWriter);

		// update m_serverInfoJSON
		m_serverInfoJSON = lStringBuffer.GetString();

		// and write to disk
		std::ofstream lServerInfoFile("../server_info.json", std::ofstream::trunc);

		return;
	}
	std::ifstream lServerInfoFile("../server_info.json");
	std::stringstream lSS;
	lSS << lServerInfoFile.rdbuf();
	m_serverInfoJSON = lSS.str();
	try {
		parseServers();
	}
	catch (const std::exception& e) {
		throw std::runtime_error(std::string("readConfig(): ") + e.what());
	}
}

void MyClientClass::parseServers() {
	std::mutex mutex;
	std::lock_guard lock(mutex);

	if (m_serverInfoJSON.empty()) {
		throw std::runtime_error("parseServers(): m_serverInfoJSON empty.");
	}

	rapidjson::Document lDoc;
	lDoc.Parse(m_serverInfoJSON.c_str());
	if (lDoc.HasMember("serverInfo") == false) {
		throw std::runtime_error("parseServers(): serverInfo not found.");
	}
	rapidjson::Value& lServerInfoVal = lDoc["serverInfo"]; 
	if (lServerInfoVal.MemberCount() == 0) {
		return;
	}

	for (rapidjson::Value::ConstMemberIterator itr = lServerInfoVal.MemberBegin(); itr != lServerInfoVal.MemberEnd(); ++itr) {
		const std::string lcSnowflake = itr->name.GetString();
		m_serverBotSettings[lcSnowflake] = ServerBotSettings();
		std::array<const char*, 7> lKeys = {
			"silent", "noLogs", "prefix", "logsChannelID", "botAdminRoleID", "mutedUserIDs", "permissions"
		};

		for (const auto& lKey : lKeys) {
			if (itr->value.HasMember(lKey) == false) {
				throw std::runtime_error(std::string("parseServers(): serverInfo val has no member '") + lKey + "'.");
			}
		}
		/*std::fprintf(stderr, "%s:\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%u\n\t%u\n\n",
			itr->name.GetString(),						 itr->value["silent"].GetString(),		  itr->value["noLogs"].GetString(),
			itr->value["prefix"].GetString(),			 itr->value["logsChannelID"].GetString(), itr->value["botAdminRoleID"].GetString(),
			itr->value["mutedUserIDs"][0].GetString(),	 itr->value["permissions"][0].GetInt(),	  itr->value["permissions"][1].GetInt()
		);
		*/
		m_serverBotSettings.at(lcSnowflake).silent			= itr->value["silent"].GetBool();
		m_serverBotSettings.at(lcSnowflake).noLogs			= itr->value["noLogs"].GetBool();
		m_serverBotSettings.at(lcSnowflake).prefix			= itr->value["prefix"].GetString();
		m_serverBotSettings.at(lcSnowflake).permissions[0]	= itr->value["permissions"].GetArray()[0].GetInt();
		m_serverBotSettings.at(lcSnowflake).permissions[1]	= itr->value["permissions"].GetArray()[1].GetInt();

		if (!itr->value["logsChannelID"].IsNull()) {
			m_serverBotSettings.at(lcSnowflake).logsChannelID = itr->value["logsChannelID"].GetString();
		}
		if (!itr->value["botAdminRoleID"].IsNull()) {
			m_serverBotSettings.at(lcSnowflake).botAdminRoleID = itr->value["botAdminRoleID"].GetString();
		}

		m_serverBotSettings.at(lcSnowflake).mutedUserIDs.resize(itr->value["mutedUserIDs"].Size());
		for (int i = 0; i < m_serverBotSettings.at(lcSnowflake).mutedUserIDs.size(); i++) {
			m_serverBotSettings.at(lcSnowflake).mutedUserIDs[i] = itr->value["mutedUserIDs"].GetArray()[i].GetString();
		}
	}
}

void MyClientClass::addServerInfo(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID) {
	std::mutex mutex;
	std::lock_guard lock(mutex);

	// read in m_serverInfoJSON
	rapidjson::Document lDoc;
	lDoc.Parse(m_serverInfoJSON.c_str());

	// create empty array for mutedUserIDs field of serverInfo
	rapidjson::Value lMutedUserIDsArrayVal;
	lMutedUserIDsArrayVal.SetArray();
	lMutedUserIDsArrayVal.Clear();

	// create array for permissions field of server info and initialize both elements to 2
	rapidjson::Value lPermissionsArrayVal;
	lPermissionsArrayVal.SetArray();
	lPermissionsArrayVal.PushBack(2, lDoc.GetAllocator());
	lPermissionsArrayVal.PushBack(2, lDoc.GetAllocator());			

	// create new member for serverInfo, and intialize to defaults of serverBotSettings
	rapidjson::Value lServerInfoVal;
	lServerInfoVal.SetObject();
	lServerInfoVal.AddMember("silent", 		   false,				  lDoc.GetAllocator());
	lServerInfoVal.AddMember("noLogs", 		   false,				  lDoc.GetAllocator());
	lServerInfoVal.AddMember("prefix", 		   _C_DEFAULT_BOT_PREFIX, lDoc.GetAllocator());
	lServerInfoVal.AddMember("logsChannelID",  rapidjson::kNullType,  lDoc.GetAllocator());
	lServerInfoVal.AddMember("botAdminRoleID", rapidjson::kNullType,  lDoc.GetAllocator());
	lServerInfoVal.AddMember("mutedUserIDs",   lMutedUserIDsArrayVal, lDoc.GetAllocator());
	lServerInfoVal.AddMember("permissions",	   lPermissionsArrayVal,  lDoc.GetAllocator());
	
	// add new member to serverInfo
	rapidjson::Value lStringVal;	// make AddMember play nice with std::string
	lStringVal.SetString(acrServerID.string().c_str(), acrServerID.string().size(), lDoc.GetAllocator());
	lDoc["serverInfo"].AddMember(lStringVal, lServerInfoVal, lDoc.GetAllocator());

	// stringify document
	rapidjson::StringBuffer lStringBuffer;
	rapidjson::Writer<rapidjson::StringBuffer> lWriter(lStringBuffer);
	lDoc.Accept(lWriter);
	// update m_serverInfoJSON
	m_serverInfoJSON = lStringBuffer.GetString();

	// and write update config to disk
	std::ofstream lServerInfoFile("../server_info.json", std::ofstream::trunc);
	if(!lServerInfoFile) {
		throw std::ios_base::failure("addServerInfo(): failed to open server_info.json");
	}
	lServerInfoFile << lStringBuffer.GetString();
}

void MyClientClass::updateServerInfo(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID) {
	std::mutex mutex;
	std::lock_guard lock(mutex);

	// read in m_serverInfoJSON
	rapidjson::Document lDoc;
	lDoc.Parse(m_serverInfoJSON.c_str());

	rapidjson::Value& lServerInfoVal = lDoc["serverInfo"];
	rapidjson::Value lStringVal;
	lStringVal.SetString(acrServerID.string().c_str(), acrServerID.string().size(), lDoc.GetAllocator());
	rapidjson::Value::MemberIterator itr = lServerInfoVal.FindMember(lStringVal);

	itr->value["silent"].SetBool(m_serverBotSettings.at(acrServerID).silent);
	itr->value["noLogs"].SetBool(m_serverBotSettings.at(acrServerID).noLogs);
	itr->value["prefix"].SetString(rapidjson::StringRef(m_serverBotSettings.at(acrServerID).prefix.c_str()));
	itr->value["logsChannelID"].SetString(rapidjson::StringRef(m_serverBotSettings.at(acrServerID).logsChannelID.string().c_str()));
	itr->value["botAdminRoleID"].SetString(rapidjson::StringRef(m_serverBotSettings.at(acrServerID).botAdminRoleID.string().c_str()));
	itr->value["permissions"].GetArray()[0].SetInt(m_serverBotSettings.at(acrServerID).permissions[0]);
	itr->value["permissions"].GetArray()[1].SetInt(m_serverBotSettings.at(acrServerID).permissions[1]);

	itr->value["mutedUserIDs"].Clear();
	for (int i = 0; i < m_serverBotSettings.at(acrServerID).mutedUserIDs.size(); i++) {
		itr->value["mutedUserIDs"].PushBack(rapidjson::StringRef(m_serverBotSettings.at(acrServerID).mutedUserIDs[i].string().c_str()), lDoc.GetAllocator());
	}

	// stringify document
	rapidjson::StringBuffer lStringBuffer;
	rapidjson::Writer<rapidjson::StringBuffer> lWriter(lStringBuffer);
	lDoc.Accept(lWriter);
	// update m_serverInfoJSON
	m_serverInfoJSON = lStringBuffer.GetString();

	// and write update config to disk
	std::ofstream lServerInfoFile("../server_info.json", std::ofstream::trunc);
	if(!lServerInfoFile) {
		throw std::ios_base::failure("updateServerInfo(): failed to open server_info.json");
	}
	lServerInfoFile << lStringBuffer.GetString();
}

// ignore this clusterfuck, I haven't the time nor energy to sort it out
void MyClientClass::onMessage(SleepyDiscord::Message aMessage) {
    try {
		std::mutex mutex;
		mutex.lock();
	    const std::string& lcrPrefix = m_serverBotSettings[aMessage.serverID].prefix;
		mutex.unlock();
	    const auto lcWords = split(aMessage.content);
	    if (aMessage.startsWith(lcrPrefix + "prefix ")) {
	    	changePrefix(aMessage.serverID, aMessage.author, aMessage.channelID, aMessage.content.substr(lcrPrefix.size() + changePrefix.name().size() + 1));
	    } else if (aMessage.startsWith(lcrPrefix + "hello")) {
	    	hello(aMessage.serverID, aMessage.author, aMessage.channelID);
	    } else if (aMessage.startsWith(lcrPrefix + "echo ")) {
	    	echo(aMessage.serverID, aMessage.author, aMessage.channelID, aMessage.content.substr(lcrPrefix.size() + echo.name().size() + 1));
	    } else if (aMessage.startsWith(lcrPrefix + "dm ")) {
	    	std::string lSnowflake;
	    	try {
	    		lSnowflake = getSnowflake(lcWords[1]);
		    } catch(std::exception& e) {
                const std::string lcError = std::string("dm: ") + e.what();
			    throw std::runtime_error(lcError);
		    }
		    dmUser(aMessage.serverID, aMessage.author, lSnowflake, aMessage.content.substr(lcrPrefix.size() + dmUser.name().size() + 1 + lcWords[1].size() + 1));
	    } else if (aMessage.startsWith(lcrPrefix + "mute voice ")) {
	    	muteVoice(aMessage.serverID, aMessage.author, aMessage.content.substr(lcrPrefix.size() + muteVoice.name().size() + 1));
	    } else if (aMessage.startsWith(lcrPrefix + "unmute voice ")) {
	    	unmuteVoice(aMessage.serverID, aMessage.author, aMessage.content.substr(lcrPrefix.size() + unmuteVoice.name().size() + 1));
	    } else if (aMessage.startsWith(lcrPrefix + "mute text ")) {
	    	std::string lSnowflake;
	    	try {
		    	lSnowflake = getSnowflake(lcWords[2]);
		    } catch(std::exception& e) {
                const std::string lcError = std::string("mute text: ") + e.what();
			    throw std::runtime_error(lcError);
		    }
		    muteText(aMessage.serverID, aMessage.author, lSnowflake);
	    } else if (aMessage.startsWith(lcrPrefix + "unmute text ")) {
		    std::string lSnowflake;
		    try {
		    	lSnowflake = getSnowflake(lcWords[2]);
		    } catch(std::exception& e) {
                const std::string lcError = std::string("unmute text: ") + e.what();
		    	throw std::runtime_error(lcError);
		    }
		    unmuteText(aMessage.serverID, aMessage.author, lSnowflake);
    	} else if (aMessage.startsWith(lcrPrefix + "kick ")) {
    		std::string lSnowflake;
	    	try {
		    	lSnowflake = getSnowflake(lcWords[1]);
    		} catch(std::exception& e) {
                const std::string lcError = std::string("kick: ") + e.what();
	    		throw std::runtime_error(lcError);
		    }  
    		if(lcWords.size() == 2) {
	    		kick(aMessage.serverID, aMessage.author, lSnowflake, "");
    		} else {
	    		kick(aMessage.serverID, aMessage.author, lSnowflake, aMessage.content.substr(lcrPrefix.size() + kick.name().size() + 1 + lcWords[1].size() + 1));
		    }
    	} else if (aMessage.startsWith(lcrPrefix + "ban ")) {
	    	std::string lSnowflake;
		    try {
    			lSnowflake = getSnowflake(lcWords[1]);
	    	} catch(std::exception& e) {
                const std::string lcError = std::string("ban: ") + e.what();
		    	throw std::runtime_error(lcError);
		    }
            int lDeleteMessageDays;
		    if(lcWords.size() == 2) {
			    ban(aMessage.serverID, aMessage.author, lSnowflake, "", 36500);
		    } else {
                try {
                    lDeleteMessageDays = std::stoi(lcWords[2]);
                } catch(std::out_of_range& e) {
                    const std::string lcError = std::string("ban: acDeleteMessageDays provided with out of range value.");
                    throw(lcError);
                }
                
                if(lcWords.size() == 3) { 
			        ban(aMessage.serverID, aMessage.author, lSnowflake, "", lDeleteMessageDays);
		        } else {
			        ban(aMessage.serverID, aMessage.author, lSnowflake, aMessage.content.substr(lcrPrefix.size() + ban.name().size() + 1 + lcWords[1].size() + 1 + lcWords[2].size() + 1), lDeleteMessageDays);
                }
            }
	    } else if (aMessage.startsWith(lcrPrefix + "unban ")) {
		    if(lcWords.size() == 2) {
			    unban(aMessage.serverID, aMessage.author, aMessage.channelID, lcWords[1], "");
    		} else {
	    		unban(aMessage.serverID, aMessage.author, aMessage.channelID, lcWords[1], aMessage.content.substr(lcrPrefix.size() + unban.name().size() + 1 + lcWords[1].size() + 1));
		    }
    	} else if (aMessage.startsWith(lcrPrefix + "invite send ")) {
	    	std::string lSnowflake;
		    try {
			    lSnowflake = getSnowflake(lcWords[2]);
    		} catch(std::exception& e) {
                const std::string lcError = std::string("invite send: ") + e.what();
	    		throw std::runtime_error(lcError);
    		}
	    	invite(aMessage.serverID, aMessage.author, aMessage.channelID, lSnowflake);
    	} else if (aMessage.startsWith(lcrPrefix + "bot_admin_role set ")) {
	    	std::string lSnowflake;
		    try {
    			lSnowflake = getSnowflake(lcWords[2]);
	    	} catch(std::exception& e) {
                const std::string lcError = std::string("bot_admin_role set: ") + e.what();
		    	throw std::runtime_error(lcError);
    		}
	    	setBotAdminRole(aMessage.serverID, aMessage.author, lSnowflake);
    	} else if (aMessage.startsWith(lcrPrefix + "nologs")) {
	    	logsDisable(aMessage.serverID, aMessage.author);
    	} else if (aMessage.startsWith(lcrPrefix + "logs disable")) {
	    	logsDisable(aMessage.serverID, aMessage.author);
    	} else if (aMessage.startsWith(lcrPrefix + "logs enable")) {
	    	logsDisable(aMessage.serverID, aMessage.author, false);
    	} else if (aMessage.startsWith(lcrPrefix + "logs ")) {
	    	std::string lSnowflake;
		    try {
			    lSnowflake = getSnowflake(lcWords[1]);
    		} catch(std::exception& e) {
                const std::string lcError = std::string("logs: ") + e.what();
	    		throw std::runtime_error(lcError);
		    }
		    setLogsChannel(aMessage.serverID, aMessage.author, SleepyDiscord::Snowflake<SleepyDiscord::Channel>(lSnowflake));
    	} else if (aMessage.startsWith(lcrPrefix + "silent")) {
	    	setSilent(aMessage.serverID, aMessage.author);
    	} else if (aMessage.startsWith(lcrPrefix + "nosilent")) {
	    	setSilent(aMessage.serverID, aMessage.author, false);
    	} else if (aMessage.startsWith(lcrPrefix + "delete ")) {
	    	deleteMsg(aMessage.serverID, aMessage.author, aMessage);
    	} else if (aMessage.startsWith(lcrPrefix + "permissions set ")) {
			COMMAND_PERMISSION lCommandPerm;
			COMMAND_TYPE lCommandType;
			try {
				lCommandPerm = toCommandPerm(lcWords[3]);
				lCommandType = toCommandType(lcWords[2]);
			} catch(const std::exception& e) {
				throw std::runtime_error(std::string("onMessage(): permissions_set: ") + e.what());
			}
	    	setPermissions(aMessage.serverID, aMessage.author, lCommandPerm, lCommandType);
	    } else if (aMessage.startsWith(lcrPrefix + "channel rename ")) {
		    std::string lSnowflake;
		    try {
			    lSnowflake = getSnowflake(lcWords[2]);
		    } catch(std::exception& e) {
                const std::string lcError = std::string("channel rename: ") + e.what();
			    throw std::runtime_error(lcError);
		    }
		    renameChannel(aMessage.serverID, aMessage.author, lSnowflake, lcWords[3]);
	    } else if (aMessage.startsWith(lcrPrefix + "channel topic set ")) {
		    std::string lSnowflake;
		    try {
			    lSnowflake = getSnowflake(lcWords[3]);
    		} catch(std::exception& e) {
                const std::string lcError = std::string("channel topic set: ") + e.what();
	    		throw std::runtime_error(lcError);
		    }
    		setChannelTopic(aMessage.serverID, aMessage.author, lSnowflake, aMessage.content.substr(lcrPrefix.size() + setChannelTopic.name().size() + 1 + lcWords[3].size() + 1));
    	} else if (aMessage.startsWith(lcrPrefix + "channel delete ")) {
    		std::string lSnowflake;
	    	try {
		    	lSnowflake = getSnowflake(lcWords[2]);
    		} catch(std::exception& e) {
                const std::string lcError = std::string("channel delete: ") + e.what();
	    		throw std::runtime_error(lcError);
    		}
	    	removeChannel(aMessage.serverID, aMessage.author, lSnowflake);
    	} else if (aMessage.startsWith(lcrPrefix + "pin ")) { // http error
	    	//pin(aMessage.serverID, aMessage.author, aMessage.channelID, split(aMessage.content)[1]);
	    } else if (aMessage.startsWith(lcrPrefix + "unpin ")) { // http error
		    //unpin(aMessage.serverID, aMessage.author, aMessage.channelID, split(aMessage.content)[1]);
    	} else if (aMessage.startsWith(lcrPrefix + "nickname ")) { // discord permissions error on changing other users' nicknames
	    	std::string lSnowflake;
		    try {
			    lSnowflake = getSnowflake(lcWords[1]);
    		} catch(std::exception& e) {
                const std::string lcError = std::string("nickname: ") + e.what();
	    		throw std::runtime_error(lcError);
		    	return;
    		}
    		changeNickname(aMessage.serverID, aMessage.author, lSnowflake, aMessage.content.substr(lcrPrefix.size() + changeNickname.name().size() + 1 + lcWords[1].size() + 1));
    	} else if (aMessage.startsWith(lcrPrefix + "role revoke ")) {
	    	std::string lSnowflake1;
		    std::string lSnowflake2;
    		try {
	    		lSnowflake1 = getSnowflake(lcWords[2]);
		    	lSnowflake2 = getSnowflake(lcWords[3]);
	    	} catch(std::exception& e) {
                const std::string lcError = std::string("role revoke: ") + e.what();
    			throw std::runtime_error(lcError);
		    }
    		revokeRole(aMessage.serverID, aMessage.author, lSnowflake1, lSnowflake2);
    	} else if (aMessage.startsWith(lcrPrefix + "role delete ")) {
	    	std::string lSnowflake;
		    try {
			    lSnowflake = getSnowflake(lcWords[2]);
    		} catch(std::exception& e) {
                const std::string lcError = std::string("mute text: ") + e.what();
	    		throw std::runtime_error(lcError);
		    }
    		rmRole(aMessage.serverID, aMessage.author, lSnowflake);
	    } else if (aMessage.startsWith(lcrPrefix + "prune ")) {
		    int lNumDays;
    		try {
	    		lNumDays = std::stoi(lcWords[1]);
    		} catch(std::out_of_range& e) {
	    		fprintf(stderr, "onMessage(): prune provided with out of range value.\n");
		    	lNumDays = 0;
    		}
	    	revokeRole(aMessage.serverID, aMessage.author, lNumDays);
	    } else if (aMessage.startsWith(lcrPrefix + "invite delete ")) {
		    deleteInviteCode(aMessage.serverID, aMessage.author, split(aMessage.content)[2]);
    	} else if (aMessage.startsWith(lcrPrefix + "channel invites delete ")) {
	    	std::string lSnowflake;
		    try {
			    lSnowflake = getSnowflake(lcWords[3]);
    		} catch(std::exception& e) {
                const std::string lcError = std::string("channel invites delete: ") + e.what();
	    		throw std::runtime_error(lcError);
    		}
	    	deleteChannelInvites(aMessage.serverID, aMessage.author, lSnowflake);
	    } else if (aMessage.startsWith(lcrPrefix + "invites delete all")) {
    		deleteServerInvites(aMessage.serverID, aMessage.author);
        } else if (aMessage.startsWith(lcrPrefix + "leave")) { // not verified working
    	    leave(aMessage.serverID, aMessage.author);
        } else if (aMessage.startsWith(lcrPrefix + "status ")) { // not verified working
	        if(lcWords.size() == 1) {
                const std::string lcError = std::string("status: provided with no arguments.");
		        throw std::runtime_error(lcError);
            }
	    	const std::string& lcrActivity = lcWords[1];
		    SleepyDiscord::Status lStatus = SleepyDiscord::Status::online;
		    bool lbAFK = false;
    		int lIdleTime = 0;
			lStatus = toStatus(lcWords[2]);

			if(lcWords.size() > 3) {
				if(lcWords[3] == "1" || lcWords[3] == "true") {
					lbAFK = true;
				} else if (lcWords[3] == "0" || lcWords[3] == "false") {
					lbAFK = false;
				} else {
                    const std::string lcError = "status: provided with invalid value for bool value.";
					throw std::runtime_error(lcError);
				}
			}
            if(lcWords.size() > 4) {
			    try {
		    		lIdleTime = std::stoi(lcWords[4]);
	    		} catch(std::out_of_range& e) {
                    const std::string lcError = "status: provided with out of range value for lIdleTime.";
    				throw std::out_of_range(lcError);
			    }
			}
		    status(aMessage.serverID, aMessage.author, lcrActivity, lStatus, lbAFK, lIdleTime);
	    } else if (aMessage.startsWith(lcrPrefix + "bot activity set")) {
		    setBotActivity(aMessage.serverID, aMessage.author, aMessage.content.substr(setBotActivity.name().size() + 1));
	    } else if (aMessage.startsWith(lcrPrefix + "bot idle set")) {
		    uint64_t lIdleTime;
    		try {
	    		lIdleTime = std::stoi(lcWords[3]);
    		} catch(const std::exception& e) {
	    		const std::string lcError = "bot idle set: provided with out of range value for lIdleTime.";
    			throw std::out_of_range(lcError);
	    	}
		    setBotIdle(aMessage.serverID, aMessage.author, lIdleTime);
    	} else if (aMessage.startsWith(lcrPrefix + "bot status set")) {
	    	setBotStatus(aMessage.serverID, aMessage.author, toStatus(lcWords[3]));
    	} else if (aMessage.startsWith(lcrPrefix + "bot afk set")) {
	    	bool b;
    		if(lcWords[3] == "1" || lcWords[3] == "true") {
	    		b = true;
		    } else if(lcWords[3] == "0" || lcWords[3] == "false") {
    			b = false;
	    	}
    		else {
	    		const std::string lcError = "bot afk set: provided with out of range value for lIdleTime.";
    			throw std::out_of_range(lcError);
    		}
	    	setBotAFK(aMessage.serverID, aMessage.author, b);
    	}
	
	    else if (aMessage.startsWith(lcrPrefix + "die")) {
		    die(aMessage.serverID, aMessage.author, aMessage.channelID);
    	} else if (aMessage.startsWith(lcrPrefix + "banned_ops")) {
	    	bannedOps(aMessage.serverID, aMessage.author, aMessage.channelID);
    	} else if (aMessage.startsWith(lcrPrefix + "sonar_ping")) {
	    	int lNumPings;
		    try {
			    lNumPings = stoi(lcWords[2]);
    		} catch(const std::out_of_range& e) {
		    	echo(aMessage.serverID, aMessage.author, aMessage.channelID, "Value is too large, must be less than 2^32.");
                const std::string lcError = "sonar_ping: provided with out of range value for lNumPings.";
    			throw std::out_of_range(lcError);
    		}
	    	std::string lSnowflake;
		    try {
    			lSnowflake = getSnowflake(lcWords[1]);
	    	} catch(std::exception& e) {
                const std::string lcError = std::string("sonar_ping: ") + e.what();
    			throw std::runtime_error(lcError);
	    	}
		    sonarPing(aMessage.serverID, aMessage.author, lSnowflake, aMessage.channelID, lNumPings);
    	} else if (aMessage.startsWith(lcrPrefix + "fuckoff")) { // not verified working
	    	fuckoff(aMessage.serverID, aMessage.author);
	    }

	    else if(aMessage.startsWith(lcrPrefix)) { 
		    const std::string lcMessage = "Unknown command.";
		    echo(aMessage.serverID, aMessage.author, aMessage.channelID, lcMessage);
	    }

	    if(isMuted(aMessage.serverID, aMessage.author.ID)) {
	    	deleteMsg(aMessage.serverID, aMessage.author, aMessage.channelID, aMessage);
	    }
    } catch(const std::exception& e) {
        std::fprintf(stderr, "onMessage(): %s\n", e.what());
        return;
    }
}

void MyClientClass::onServer(SleepyDiscord::Server aServer) {
	std::mutex mutex;
	mutex.lock();

	m_servers[aServer.ID] = aServer;
	try {
		// if std::map::at throws std::out_of_range, entry doesn't exist
		m_serverBotSettings.at(aServer.ID);
	} catch(const std::out_of_range& e) {
		// if entry doesn't exist, create it and add it to server info
		m_serverBotSettings[aServer.ID] = ServerBotSettings();
		try {
			mutex.unlock();
			addServerInfo(aServer.ID);
		} catch(const std::ios_base::failure& e) {
			std::fprintf(stderr, "onServer(): %s", e.what());
			mutex.unlock();
			return;
		}
		mutex.unlock();
		return;
	}
	mutex.unlock();
}

void MyClientClass::onBan(SleepyDiscord::Snowflake<SleepyDiscord::Server> aServerID, SleepyDiscord::User aBannedUser) {
	std::mutex mutex;
	std::lock_guard lock(mutex);

	// if user wasn't already added to m_bannedUsers by ban()
	if(m_bannedUsers.find(aBannedUser.ID) == m_bannedUsers.end()) {
		// if user wasn't already added, then they weren't manually banned (set bool value to false)
		m_bannedUsers[aBannedUser.ID] = std::make_pair(aBannedUser, false);
	}
	// client automatically calls onRemoveUser(), which handles the logging
}

void MyClientClass::onRemoveMember(SleepyDiscord::Snowflake<SleepyDiscord::Server> aServerID, SleepyDiscord::User aRemovedUser) {
	std::mutex mutex;

	std::time_t lTime = std::time(nullptr);
	std::put_time(std::gmtime(&lTime), "%c"); // get date/time info for log
	std::stringstream lTimeSS;
	lTimeSS << lTime;
	const std::string lcTimeStr = lTimeSS.str(); // hacky way to get std::string from time_t
	std::string lLog;
	
	mutex.lock();
	m_servers.at(aServerID).members.erase(m_servers.at(aServerID).findMember(aRemovedUser.ID)); // remove user from cached server
	// if user was banned, not kicked
	if((m_bannedUsers.find(aRemovedUser.ID) != m_bannedUsers.end()) && (m_kickedUsers.find(aRemovedUser.ID) == m_kickedUsers.end())) {
		// if user was not banned by bot, log ban
		// if user was banned by bot, ban will have already been logged by ban()
		if(m_bannedUsers.at(aRemovedUser.ID).second == false) {
			lLog = "**BANNED USER**\n```User: " + aRemovedUser.username + "#" + aRemovedUser.discriminator + "\nBanned by: Unknown\nReason given:\nOn: " + lcTimeStr + "```";
			logAction(aServerID, aRemovedUser, lLog);
		}
		// erase user if user was previously kicked, and now banned
		m_kickedUsers.erase(m_kickedUsers.find(aRemovedUser.ID));
	}
	// else if user was manually kicked, but not banned
	else if((m_bannedUsers.find(aRemovedUser.ID) == m_bannedUsers.end()) && (m_kickedUsers.find(aRemovedUser.ID) != m_kickedUsers.end())) {
		// same as above
		if(m_kickedUsers.at(aRemovedUser.ID).second == false) {
			lLog = "**KICKED USER**\n```User: " + aRemovedUser.username + "#" + aRemovedUser.discriminator + "\nBanned by: Unknown\nReason given:\nOn: " + lcTimeStr + "```";
			logAction(aServerID, aRemovedUser, lLog);
		}
	}
	// else user was not manually kicked/left on their own 
	// (can't detect difference between non-manual kick and normal leave)
	else {
		lLog = "**KICKED USER/USER LEFT**\n```User: " + aRemovedUser.username + "#" + aRemovedUser.discriminator + "\nOn: " + lcTimeStr + "```";
		logAction(aServerID, aRemovedUser, lLog);
	}
	mutex.unlock();
}



void MyClientClass::fn_changePrefix(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID, const std::string& acrNewPrefix) {
	std::mutex mutex;
	
	if(acrNewPrefix.find(" ") != std::string::npos) {
		const std::string lcMessage = "Prefix may not contain whitespace.";
		echo(acrServerID, acrUser, acrChannelID, lcMessage);
	}
	mutex.lock();
	m_serverBotSettings.at(acrServerID).prefix = acrNewPrefix;
	mutex.unlock();
	updateServerInfo(acrServerID);
}

void MyClientClass::fn_hello(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID) {
	echo(acrServerID, acrUser, acrChannelID, "Hello, " + Mention<SleepyDiscord::User>(acrUser));
}

void MyClientClass::fn_echo(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID, const std::string& acrMessage) {
	std::mutex mutex;
	std::lock_guard lock(mutex);

	if (m_serverBotSettings.at(acrServerID).silent == false) {
		sendMessage(acrChannelID, acrMessage);
	}
}

void MyClientClass::fn_dmUser(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrDMedUserID, const std::string& acrMessage) {
	std::mutex mutex;
	mutex.lock();

	if(m_userDMchannelIDs.find(acrDMedUserID) == m_userDMchannelIDs.end()) { // if user DM channel was not already created
		m_userDMchannelIDs[acrDMedUserID] = SleepyDiscord::Channel(createDirectMessageChannel(acrDMedUserID)).ID;
	}
	const SleepyDiscord::Snowflake<SleepyDiscord::Channel> lcChannelID = m_userDMchannelIDs[acrDMedUserID];

	mutex.unlock();
	sendMessage(lcChannelID, acrMessage);
}

void MyClientClass::fn_muteVoice(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrMutedUserID) {
	muteServerMember(acrServerID, acrMutedUserID, true);
}

void MyClientClass::fn_unmuteVoice(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrMutedUserID) {
	muteServerMember(acrServerID, acrMutedUserID, false);
}

void MyClientClass::fn_muteText(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrMutedUserID) {
	std::mutex mutex;
	std::lock_guard lock(mutex);

	if(std::count(m_serverBotSettings.at(acrServerID).mutedUserIDs.begin(), m_serverBotSettings.at(acrServerID).mutedUserIDs.end(), acrMutedUserID) == 0) { // if user is not already text-muted
		m_serverBotSettings.at(acrServerID).mutedUserIDs.push_back(acrMutedUserID);
	}
}

void MyClientClass::fn_unmuteText(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrMutedUserID) {
	std::mutex mutex;
	std::lock_guard lock(mutex);

	if(std::count(m_serverBotSettings.at(acrServerID).mutedUserIDs.begin(), m_serverBotSettings.at(acrServerID).mutedUserIDs.end(), acrMutedUserID) > 0) { // if user is text-muted, remove from mutedUserIDs
		m_serverBotSettings.at(acrServerID).mutedUserIDs.erase(std::find(m_serverBotSettings.at(acrServerID).mutedUserIDs.begin(), m_serverBotSettings.at(acrServerID).mutedUserIDs.end(), acrMutedUserID)); 
	}
}

void MyClientClass::fn_kick(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrKickedUserID, const std::string& acrReason) {
	std::mutex mutex;

	SleepyDiscord::User lKickedUser = getUser(acrKickedUserID);
	mutex.lock();
	m_kickedUsers[acrKickedUserID] = std::make_pair(lKickedUser, true); // bool value set to true to indicate kick via bot
	mutex.unlock();
	kickMember(acrServerID, lKickedUser);
	std::time_t lTime = std::time(nullptr);
	std::put_time(std::gmtime(&lTime), "%c"); // get date/time info for log
	std::stringstream lTimeSS;
	lTimeSS << lTime;
	const std::string lcTimeStr = lTimeSS.str(); // hacky time_t thing again
	std::string lLog;
	if(acrReason == "") {
		lLog = std::string("**KICKED USER**\n```User: " + lKickedUser.username + "#" + lKickedUser.discriminator + "\nBanned by: " + acrUser.username + "#" + acrUser.discriminator + "\nReason given:\nOn: " + lcTimeStr + "```");
	} else {
		lLog = std::string("**KICKED USER**\n```User: " + lKickedUser.username + "#" + lKickedUser.discriminator + "\nBanned by: " + acrUser.username + "#" + acrUser.discriminator + "\nReason given: " + acrReason + "\nOn: " + lcTimeStr + "```");
	}
	logAction(acrServerID, acrUser, lLog);
}

void MyClientClass::fn_ban(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrBannedUserID, const std::string& acrReason, int acDeleteMessageDays) {
	std::mutex mutex;
	std::lock_guard<std::mutex> lock(mutex);

	SleepyDiscord::User lBannedUser = getUser(acrBannedUserID);
	m_bannedUsers[acrBannedUserID] = std::make_pair(lBannedUser, true); // bool value set to true to indicate ban via bot
	banMember(acrServerID, lBannedUser, acDeleteMessageDays, acrReason);
	std::time_t lTime = std::time(nullptr);
	std::put_time(std::gmtime(&lTime), "%c"); // get time info for log
	std::stringstream lTimeSS;
	lTimeSS << lTime;
	const std::string lcTimeStr = lTimeSS.str(); // hacky time thing yet again
	std::string lLog;
	if(acrReason == "") {
		lLog = std::string("**BANNED USER**\n```User: " + lBannedUser.username + "#" + lBannedUser.discriminator + "\nBanned by: " + acrUser.username + "#" + acrUser.discriminator + "\nReason given:\nOn: " + lcTimeStr + "```");
	} else {
		lLog = std::string("**BANNED USER**\n```User: " + lBannedUser.username + "#" + lBannedUser.discriminator + "\nBanned by: " + acrUser.username + "#" + acrUser.discriminator + "\nReason given: " + acrReason + "\nOn: " + lcTimeStr + "```");
	}
	logAction(acrServerID, acrUser, lLog);
}

void MyClientClass::fn_unban(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrBannedUserID, const std::string& acrReason) {
	auto lBannedUsers = getBans(acrServerID).vector();
	SleepyDiscord::User lBannedUser;

	for(const auto& lUser : lBannedUsers) {
		if(lBannedUser.ID == acrBannedUserID) {
			lBannedUser = lUser;
		}
	}

	if(lBannedUser.ID == "") {
		const std::string lcLog = "User not found.";
		echo(acrServerID, acrUser, acrChannelID, lcLog);
		return;
	} else {
		std::mutex mutex;

		mutex.lock();
		m_bannedUsers.erase(acrBannedUserID); // remove user from ban list
		mutex.unlock();
		unbanMember(acrServerID, acrBannedUserID);
		std::time_t lTime = std::time(nullptr);
		std::put_time(std::gmtime(&lTime), "%c");
		std::stringstream lTimeSS;
		lTimeSS << lTime;
		const std::string lcTimeStr = lTimeSS.str(); // once more...
		std::string lLog;
		if(acrReason == "") {
			lLog = std::string("**UNBANNED USER**\n```User: " + lBannedUser.username + "#" + lBannedUser.discriminator + "\nUnbanned by: " + acrUser.username + "#" + acrUser.discriminator + "\nReason given:\nOn: " + lcTimeStr + "```");
		} else {
			lLog = std::string("**UNBANNED USER**\n```User: " + lBannedUser.username + "#" + lBannedUser.discriminator + "\nUnbanned by: " + acrUser.username + "#" + acrUser.discriminator + "\nReason given: " + acrReason + "\nOn: " + lcTimeStr + "```");
		}
		logAction(acrServerID, acrUser, lLog);
	}
}

void MyClientClass::fn_invite(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrInvitedUserID) {
	SleepyDiscord::Invite invite = createChannelInvite(acrChannelID, 0, 1, false, false);
	const std::string lcInviteCode = invite.code;
	dmUser(acrServerID, acrUser, acrInvitedUserID, "https://discord.gg/" + lcInviteCode);
}

void MyClientClass::fn_setBotAdminRole(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Role>& acrRoleID) {
	std::mutex mutex;

	mutex.lock();
	m_serverBotSettings.at(acrServerID).botAdminRoleID = acrRoleID;
	mutex.unlock();

	logAction(acrServerID, acrUser, "**BOT ADMIN ROLE SET**\n```Set to: " + acrRoleID.string() + "\nSet by: " + acrUser.username + "#" + acrUser.discriminator + "\n```");
}

void MyClientClass::fn_setLogsChannel(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID) {
	std::mutex mutex;

	mutex.lock();
	m_serverBotSettings.at(acrServerID).logsChannelID = acrChannelID;
	mutex.unlock();

	logAction(acrServerID, acrUser, "Logs enabled in " + Mention<SleepyDiscord::Snowflake<SleepyDiscord::Channel>>(acrChannelID));
}

void MyClientClass::fn_logsDisable(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, bool b) {
	std::mutex mutex;
	std::lock_guard lock(mutex);
	m_serverBotSettings.at(acrServerID).noLogs = b;
}

void MyClientClass::fn_setSilent(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, bool b) {
	std::mutex mutex;
	std::lock_guard lock(mutex);
	m_serverBotSettings.at(acrServerID).silent = b;
}

void MyClientClass::fn_deleteMsg(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID, SleepyDiscord::Message& arMessage) {
	deleteMessage(acrChannelID, arMessage.ID);
	std::string lLog = "**MESSAGE DELETE:**\n```Message ID: " + arMessage.ID.string() + "\nDeleted by: " + acrUser.username + "#" + acrUser.discriminator + "\nChannel: " + arMessage.channelID.string() + "\nMessage content: " + arMessage.content + "\n```";
	logAction(acrServerID, acrUser, lLog);
}

void MyClientClass::fn_logAction(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const std::string& acrString) {
	std::mutex mutex;
	mutex.lock();
	if (m_serverBotSettings.at(acrServerID).noLogs == false) {
		if (m_serverBotSettings.at(acrServerID).logsChannelID != SleepyDiscord::Snowflake<SleepyDiscord::Channel>()) {
			mutex.unlock();
			echo(acrServerID, acrUser, m_serverBotSettings.at(acrServerID).logsChannelID, acrString);
		}
		else {
			mutex.unlock();
		}
	}
	else {
		mutex.unlock();
	}
}

void MyClientClass::fn_setPermissions(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, COMMAND_PERMISSION aCommandPerm, COMMAND_TYPE aCommandType) {
	std::mutex mutex;

	mutex.lock();
	if(aCommandType == ROLE_ALL) {
		m_serverBotSettings.at(acrServerID).permissions[COMMAND_TYPE::ADMIN] = aCommandPerm;
		m_serverBotSettings.at(acrServerID).permissions[COMMAND_TYPE::NON_ADMIN] = aCommandPerm;
	} else {
		m_serverBotSettings.at(acrServerID).permissions[aCommandType] = aCommandPerm;
	}
	mutex.unlock();

	// used for log
	std::map<COMMAND_PERMISSION, std::string> lPerms = {
		{ COMMAND_PERMISSION::OWNER_ONLY, std::string("owner_only") },
		{ COMMAND_PERMISSION::BOT_ADMIN,  std::string("bot_admin")  },
		{ COMMAND_PERMISSION::CMD_ALL,    std::string("all") 		}
	};
	std::map<COMMAND_TYPE, std::string> lType = {
		{ COMMAND_TYPE::ADMIN, 	   std::string("admin") 	},
		{ COMMAND_TYPE::NON_ADMIN, std::string("non_admin") },
		{ COMMAND_TYPE::ROLE_ALL,  std::string("all") 		}
	}; 

	logAction(acrServerID, acrUser, "**PERMISSIONS CHANGE:**\n```Type: " + lType[aCommandType] + "\nSet to: " + lPerms[aCommandPerm] + "\nSet by: " + acrUser.username + "#" + acrUser.discriminator + "```\n");
}

void MyClientClass::fn_setChannelName(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID, const std::string& acrName) {
	editChannelName(acrChannelID, acrName);
}

void MyClientClass::fn_setChannelTopic	(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID, const std::string& acrTopic) {
	editChannelTopic(acrChannelID, acrTopic);
}

void MyClientClass::fn_deleteChannel(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID) {
	deleteChannel(acrChannelID);
}

void MyClientClass::fn_pinMessage(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID, const SleepyDiscord::Snowflake<SleepyDiscord::Message>& acrMessageID) {
	if(std::find(getPinnedMessages(acrChannelID).vector().begin(), getPinnedMessages(acrChannelID).vector().end(), acrMessageID) == getPinnedMessages(acrChannelID).vector().end()) {
		pinMessage(acrChannelID, acrMessageID);
	} else {
		std::fprintf(stderr, "pinMessage(): message was already pinned.");
	}
}

void MyClientClass::fn_unpinMessage(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID, const SleepyDiscord::Snowflake<SleepyDiscord::Message>& acrMessageID) {
	if(std::find(getPinnedMessages(acrChannelID).vector().begin(), getPinnedMessages(acrChannelID).vector().end(), acrMessageID) != getPinnedMessages(acrChannelID).vector().end()) {
		unpinMessage(acrChannelID, acrMessageID);
	} else {
		std::fprintf(stderr, "unpinMessage(): message was not pinned.");
	}
}

void MyClientClass::fn_changeNickname(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrNicknamedUserID, const std::string& acrNickname) {
	if(isBot(acrNicknamedUserID)) {
		editNickname(acrServerID, acrNickname);
	} else {
		// throws discord permissions error
		//editMember(acrServerID, acrNicknamedUserID, acrNickname);
	}
}

void MyClientClass::fn_removeRole(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrRemovedUserID, const SleepyDiscord::Snowflake<SleepyDiscord::Role>& acrRoleID) {
	removeRole(acrServerID, acrRemovedUserID, acrRoleID);
}

void MyClientClass::fn_deleteRole(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Role>& acrRoleID) {
	deleteRole(acrServerID, acrRoleID);
}

void MyClientClass::fn_pruneUsers(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, int aNumDays) {
	pruneMembers(acrServerID, aNumDays);
}

void MyClientClass::fn_deleteInvite(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const std::string& acrInvite) {
	std::string lInviteCode;
	if(acrInvite.find("https://discord.gg/") != std::string::npos) {
		lInviteCode = acrInvite.substr(strlen("https://discord.gg/"));
	} else if(acrInvite.find("discord.gg/") != std::string::npos) {
		lInviteCode = acrInvite.substr(strlen("discord.gg/"));
	} else {
		lInviteCode = acrInvite;
	}
	deleteInvite(lInviteCode);
}

void MyClientClass::fn_deleteChannelInvites(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID) {
	auto lVec = getChannelInvites(acrChannelID).vector();
	for(auto lInvite : lVec) {
		deleteInvite(lInvite.code);
	}
}

void MyClientClass::fn_deleteAllInvites(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser) {
	auto lVec = getServerInvites(acrServerID).vector();
	for(auto lInvite : lVec) {
		deleteInvite(lInvite.code);
	}
}

void MyClientClass::fn_leaveServer(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser) {
	leaveServer(acrServerID);
}

void MyClientClass::fn_setStatus(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const std::string& acrActivityName, const SleepyDiscord::Status acStatus, bool abAFK, int aIdleTime) {
	updateStatus(acrActivityName, aIdleTime, acStatus, abAFK);
}

void MyClientClass::fn_setBotActivity(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const std::string& acrActivityName) {
	botStatus.activity = acrActivityName;
	updateStatus(botStatus.activity, botStatus.idleSince, botStatus.status, botStatus.AFK);
}

void MyClientClass::fn_setBotIdle(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, int aIdleTime) {
	botStatus.idleSince = aIdleTime;
	updateStatus(botStatus.activity, botStatus.idleSince, botStatus.status, botStatus.AFK);
}

void MyClientClass::fn_setBotStatus(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Status acStatus) {
	botStatus.status = acStatus;
	updateStatus(botStatus.activity, botStatus.idleSince, botStatus.status, botStatus.AFK);
}
	
void MyClientClass::fn_setBotAFK(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, bool abAFK) {
	botStatus.AFK = abAFK;
	updateStatus(botStatus.activity, botStatus.idleSince, botStatus.status, botStatus.AFK);
}

void MyClientClass::fn_die(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID) {
	const std::string lcMessage = "Okay.";
	echo(acrServerID, acrUser, acrChannelID, lcMessage);
	quit();
}

void MyClientClass::fn_bannedOps(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID) {
	const std::string lcMessage = "Aruni, Clash, Oryx.";
	echo(acrServerID, acrUser, acrChannelID, lcMessage);
}

void MyClientClass::fn_sonarPing(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::User& acrUser, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrPingedUserID, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID, unsigned int aCount) {
	if(aCount == 0) {
		return;
	} else {
		std::string lMention = Mention<SleepyDiscord::User>(acrPingedUserID);
		for(int i = 0; i < aCount; i++) {
			echo(acrServerID, acrUser, acrChannelID, lMention + " Sonar ping.");
			sleep(_C_SONAR_PING_DELAY);
			echo(acrServerID, acrUser, acrChannelID, lMention + " Sonar pong.");
			sleep(_C_SONAR_PONG_DELAY);
		}
	}
}

std::vector<std::string> MyClientClass::split(const std::string& acrString) { // split std::string into vector of words delimited by whitespace
	std::vector<std::string> lVec;
	std::string lTemp;
    std::stringstream lSS(acrString);

    while (lSS >> lTemp) {
        lVec.push_back(lTemp);
	}
	return lVec;
}

std::string MyClientClass::getSnowflake(const std::string& acrString) {
	if(acrString.find("<@!") != std::string::npos) { // user mention
		return acrString.substr(3, acrString.size() - 3 - 1);
	}
	else if(acrString.find("<#") != std::string::npos) { // channel mention
		return acrString.substr(2, acrString.size() - 2 - 1);
	}
	else if (acrString.find("<@&") != std::string::npos) { // role mention
		return acrString.substr(3, acrString.size() - 3 - 1);
	}
	else throw std::runtime_error("getSnowflake(): string is not a member, role, or channel mention.");
}

MyClientClass::COMMAND_PERMISSION MyClientClass::toCommandPerm(const std::string& acrString) {
    std::map<std::string, COMMAND_PERMISSION> lPerms = {
		{ std::string("owner_only"), COMMAND_PERMISSION::OWNER_ONLY },
		{ std::string("bot_admin"), COMMAND_PERMISSION::BOT_ADMIN },
		{ std::string("all"), COMMAND_PERMISSION::CMD_ALL }
	};
    MyClientClass::COMMAND_PERMISSION lPerm;
    try {
        lPerm = lPerms.at(acrString);
    }
    catch(std::out_of_range& e) {
		std::string lError = "toCommandPerm(): invalid string provided (" + acrString + ")";
		throw std::runtime_error(lError);
		return COMMAND_PERMISSION::PERM_ERR;
	}
	return lPerm;
}

MyClientClass::COMMAND_TYPE MyClientClass::toCommandType(const std::string& acrString) {
	std::map<std::string, COMMAND_TYPE> lTypes = {
		{ std::string("admin"), COMMAND_TYPE::ADMIN },
		{ std::string("non_admin"), COMMAND_TYPE::NON_ADMIN },
		{ std::string("all"), COMMAND_TYPE::ROLE_ALL }
	};
    MyClientClass::COMMAND_TYPE lType;
    try {
        lType = lTypes.at(acrString);
    } catch(std::out_of_range& e) {
        std::string lError = "toCommandType(): invalid string provided (" + acrString + ")";
		throw std::runtime_error(lError);
		return COMMAND_TYPE::TYPE_ERR;
	}
	return lType;
}

SleepyDiscord::Status MyClientClass::toStatus(const std::string& acrString) {
	std::map<std::string,	SleepyDiscord::Status> lStatuses = {
		{ "online", 		SleepyDiscord::Status::online },
		{ "dnd", 			SleepyDiscord::Status::doNotDisturb },
		{ "do_not_disturb", SleepyDiscord::Status::doNotDisturb },
		{ "afk",			SleepyDiscord::Status::idle },
		{ "idle",			SleepyDiscord::Status::idle },
		{ "invisible",		SleepyDiscord::Status::invisible },
		{ "offline",		SleepyDiscord::Status::offline }
	};
	SleepyDiscord::Status lStatus;
	try {
		lStatus = lStatuses.at(acrString);
	} catch (const std::out_of_range& e) {
		std::string lError = "toStatus(): invalid string provided (" + acrString + ")";
		throw std::runtime_error(lError);
		return SleepyDiscord::Status();
	}
	return lStatus;
}

bool MyClientClass::hasRole(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrUserID, const SleepyDiscord::Snowflake<SleepyDiscord::Role>& acrRoleID) {
	SleepyDiscord::Server lServer = getServer(acrServerID);
	const SleepyDiscord::ServerMember& lcrMember = *lServer.findMember(acrUserID);
	for (const auto& lMemberRoleID : lcrMember.roles) {
	    if (lMemberRoleID == acrRoleID) {
            return true;
		}
	}
	return false;
}

bool MyClientClass::hasPermission(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrUserID, const SleepyDiscord::Permission acPermission) {
	SleepyDiscord::Server lServer = getServer(acrServerID);
	const SleepyDiscord::ServerMember lcMember = getMember(acrServerID, acrUserID);
	std::vector<SleepyDiscord::Role> lcRoles = getRoles(acrServerID);
	for(const SleepyDiscord::Role& lcRole : lcRoles) {
		if(hasRole(acrServerID, acrUserID, lcRole.ID)) {
			if(lcRole.permissions & acPermission) {
				return true;
			}
		}
	}
	return false;
}

bool MyClientClass::isMuted(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrUserID) const {
	std::mutex mutex;
	std::lock_guard lock(mutex);

	return std::count(m_serverBotSettings.at(acrServerID).mutedUserIDs.begin(), m_serverBotSettings.at(acrServerID).mutedUserIDs.end(), acrUserID) >= 1;
}