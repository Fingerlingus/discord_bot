#include "server_cache.hpp"

void ServerCache::addServer(const SleepyDiscord::Server& acrServer) {
    m_servers[acrServer.ID] = acrServer;
}
    

/*
SleepyDiscord::Invite ServerCache::getInvite(const std::string& acrInviteCode) {

}
*/

SleepyDiscord::Server& ServerCache::getServer(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID) {
    SleepyDiscord::Server* lpReturnVal = nullptr;
    try {
        *lpReturnVal = m_servers.at(acrServerID);
    } catch(const std::out_of_range& e) {
        throw std::out_of_range("getServer(): server not found");
        return m_servers.end()->second;
    }
    return (*lpReturnVal);
}

/*
SleepyDiscord::Invite ServerCache::getInvite(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const std::string& acrInviteCode) {
    SleepyDiscord::Invite lReturnVal;
    try {
        SleepyDiscord::Server lServer = m_servers.at(acrServerID);
        try {
            lServer.findChannel(acrInviteCode);
        } catch(const std::exception& e) {
            throw std::out_of_range("getInvite(): invite not found in server");
        }
    } catch(const std::out_of_range& e) {
        throw std::out_of_range(std::string("getInvite(): ") + e.what());
        return SleepyDiscord::Invite();
    }
    return lReturnVal;
}
*/

SleepyDiscord::Channel ServerCache::getChannel(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID) {
    SleepyDiscord::Channel lReturnVal;
    try {
        SleepyDiscord::Server lServer = m_servers.at(acrServerID);
        try {
            lServer.findChannel(acrChannelID);
        } catch(const std::exception& e) {
            throw std::out_of_range("getChannel(): channel not found in server");
        }
    } catch(const std::out_of_range& e) {
        throw std::out_of_range(std::string("getChannel(): ") + e.what());
        return SleepyDiscord::Channel();
    }
    return lReturnVal;
}

/*
SleepyDiscord::Message ServerCache::getMessage(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::Snowflake<SleepyDiscord::Channel>& acrChannelID, const SleepyDiscord::Snowflake<SleepyDiscord::Message>& acrMessageID) {
    SleepyDiscord::Message lReturnVal;
    try {
        SleepyDiscord::Server lServer = m_servers.at(acrServerID);
        try {
            lServer.findChannel(acrChannelID).findMessage(acrMessageID);
        } catch(const std::exception& e) {
            throw std::out_of_range("getMessage(): message not found in server");
        }
    } catch(const std::out_of_range& e) {
        throw std::out_of_range(std::string("getMessage(): ") + e.what());
        return SleepyDiscord::Message();
    }
    return lReturnVal;
}
*/

/*
SleepyDiscord::User ServerCache::getRole(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::Snowflake<SleepyDiscord::Role>& acrRoleID) {
    SleepyDiscord::ServerMember lReturnVal;
    try {
        SleepyDiscord::Server lServer = m_servers.at(acrServerID);
        try {
            lReturnVal = std::find(lServer.roles.begin(), lServer.roles.end(), acrRoleID);
        } catch(const std::exception& e) {
            throw std::out_of_range("getMember(): user not found in server");
        }
    } catch(const std::out_of_range& e) {
        throw std::out_of_range(std::string("getMember(): ") + e.what());
        return SleepyDiscord::ServerMember();
    }
    return lReturnVal;
}
*/

SleepyDiscord::ServerMember ServerCache::getMember(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrUserID) {
    SleepyDiscord::ServerMember lReturnVal;
    try {
        SleepyDiscord::Server lServer = m_servers.at(acrServerID);
        try {
            lReturnVal = *lServer.findMember(acrUserID);
        } catch(const std::exception& e) {
            throw std::out_of_range("getMember(): user not found in server");
        }
    } catch(const std::out_of_range& e) {
        throw std::out_of_range(std::string("getMember(): ") + e.what());
        return SleepyDiscord::ServerMember();
    }
    return lReturnVal;
}

SleepyDiscord::User ServerCache::getUser(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID, const SleepyDiscord::Snowflake<SleepyDiscord::User>& acrUserID) {
    SleepyDiscord::User lReturnVal;
    try {
        SleepyDiscord::Server lServer = m_servers.at(acrServerID);
        try {
            lReturnVal = (*lServer.findMember(acrUserID)).user;
        } catch(const std::exception& e) {
            throw std::out_of_range("getUser(): user not found in server");
        }
    } catch(const std::out_of_range& e) {
        throw std::out_of_range(std::string("getUser(): ") + e.what());
        return SleepyDiscord::ServerMember();
    }
    return lReturnVal;
}

std::vector<SleepyDiscord::Server> ServerCache::getServers() {
    std::vector<SleepyDiscord::Server> lServers;
    for(const auto& lVal : m_servers) {
        lServers.push_back(lVal.second);
    }
    return lServers;
}

std::vector<SleepyDiscord::Channel> ServerCache::getServerChannels(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID) {
    std::vector<SleepyDiscord::Channel> lReturnVal;
    try {
        SleepyDiscord::Server lServer = m_servers.at(acrServerID);
        for(const auto& lVal : lServer.channels) {
            lReturnVal.push_back(lVal);
        }
    } catch(const std::out_of_range& e) {
        throw std::out_of_range(std::string("getServerChannels(): ") + e.what());
    }
    return lReturnVal;
}

std::vector<SleepyDiscord::Role> ServerCache::getRoles(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID) {
    std::vector<SleepyDiscord::Role> lReturnVal;
    try {
        SleepyDiscord::Server lServer = m_servers.at(acrServerID);
        for(const auto& lVal : lServer.roles) {
            lReturnVal.push_back(lVal);
        }
    } catch(const std::out_of_range& e) {
        throw std::out_of_range(std::string("getRoles(): ") + e.what());
    }
    return lReturnVal;
}

std::vector<SleepyDiscord::ServerMember> ServerCache::getServerMembers(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID) {
    std::vector<SleepyDiscord::ServerMember> lReturnVal;
    try {
        SleepyDiscord::Server lServer = m_servers.at(acrServerID);
        for(const auto& lVal : lServer.members) {
            lReturnVal.push_back(lVal);
        }
    } catch(const std::out_of_range& e) {
        throw std::out_of_range(std::string("getServerMembers(): ") + e.what());
    }
    return lReturnVal;
}

std::vector<SleepyDiscord::User> ServerCache::getUsers(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID) {
    std::vector<SleepyDiscord::User> lReturnVal;
    try {
        SleepyDiscord::Server lServer = m_servers.at(acrServerID);
        for(const auto& lVal : lServer.members) {
            lReturnVal.push_back(lVal.user);
        }
    } catch(const std::out_of_range& e) {
        throw std::out_of_range(std::string("getServerRoles(): ") + e.what());
    }
    return lReturnVal;
}

std::vector<SleepyDiscord::Snowflake<SleepyDiscord::Server>> ServerCache::getServerIDs() {
    std::vector<SleepyDiscord::Snowflake<SleepyDiscord::Server>> lServerIDs;
    for(const auto lVal : m_servers) {
        lServerIDs.push_back(lVal.second.ID);
    }
    return lServerIDs;
}

std::vector<SleepyDiscord::Snowflake<SleepyDiscord::Channel>> ServerCache::getServerChannelIDs(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID) {
    std::vector<SleepyDiscord::Snowflake<SleepyDiscord::Channel>> lReturnVal;
    try {
        SleepyDiscord::Server lServer = m_servers.at(acrServerID);
        for(const auto lVal : lServer.channels) {
            lReturnVal.push_back(lVal.ID);
        }
    } catch(const std::out_of_range& e) {
        throw std::out_of_range(std::string("getServerChannelIDs(): ") + e.what());
    }
    return lReturnVal;
}

std::vector<SleepyDiscord::Snowflake<SleepyDiscord::Role>> ServerCache::getRoleIDs(const SleepyDiscord::Snowflake<SleepyDiscord::Server>& acrServerID) {
    std::vector<SleepyDiscord::Snowflake<SleepyDiscord::Role>> lReturnVal;
    try {
        SleepyDiscord::Server lServer = m_servers.at(acrServerID);
        for(const auto lVal : lServer.roles) {
            lReturnVal.push_back(lVal.ID);
        }
    } catch(const std::out_of_range& e) {
        throw std::out_of_range(std::string("getRoleIDs(): ") + e.what());
    }
    return lReturnVal;
}