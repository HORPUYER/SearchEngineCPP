#pragma once
#include <string>

class Config 
{
public:
    Config(const std::string& filename);

    std::string GetDbHost() const;
    int GetDbPort() const;
    std::string GetDbName() const;
    std::string GetDbUser() const;
    std::string GetDbPass() const;

    std::string GetStartPage() const;
    int GetRecursionDepth() const;
    int GetServerPort() const;

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
