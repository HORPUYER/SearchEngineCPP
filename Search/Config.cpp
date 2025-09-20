#include "Config.h"

Config::Config(std::string& filename)
{
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(filename, pt);

    m_dbHost = pt.get<std::string>("DataBase.bd_host");
    m_dbPort = pt.get<int>("DataBase.bd_port");
    m_dbName = pt.get<std::string>("DataBase.bd_name");
    m_dbUser = pt.get<std::string>("DataBase.bd_user");
    m_dbPass = pt.get<std::string>("DataBase.bd_pass");

    m_startPage = pt.get<std::string>("Client.start_page");
    m_recursionDepth = pt.get<int>("Client.recursion_depth");

    m_serverPort = pt.get<int>("Server.server_port");
}

std::string Config::GetDbHost()
{
    return m_dbHost;
}

int Config::GetDbPort()
{
    return m_dbPort;
}

std::string Config::GetDbName()
{
    return m_dbName;
}

std::string Config::GetDbUser()
{
    return m_dbUser;
}

std::string Config::GetDbPass()
{
    return m_dbPass;
}

std::string Config::GetStartPage()
{
    return m_startPage;
}

int Config::GetRecursionDepth()
{
    return m_recursionDepth;
}

int Config::GetServerPort()
{
    return m_serverPort;
}
