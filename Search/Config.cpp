#include "Config.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

Config::Config(const std::string& filename)
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

std::string Config::GetDbHost() const { return m_dbHost; }
int Config::GetDbPort() const { return m_dbPort; }
std::string Config::GetDbName() const { return m_dbName; }
std::string Config::GetDbUser() const { return m_dbUser; }
std::string Config::GetDbPass() const { return m_dbPass; }

std::string Config::GetStartPage() const { return m_startPage; }
int Config::GetRecursionDepth() const { return m_recursionDepth; }
int Config::GetServerPort() const { return m_serverPort; }
