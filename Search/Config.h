#pragma once
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>
#include <string>

class Config
{
public:
    Config(std::string& filename);

    std::string GetDbHost();
    int GetDbPort();
    std::string GetDbName();
    std::string GetDbUser();
    std::string GetDbPass();
    std::string GetStartPage();
    int GetRecursionDepth();
    int GetServerPort();

private:
    std::string m_dbHost;
    int m_dbPort;
    std::string m_dbName;
    std::string m_dbUser;
    std::string m_dbPass;

    std::string m_startPage;
    int m_recursionDepth;

    int m_serverPort;
};
