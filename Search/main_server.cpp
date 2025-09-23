#include "Config.h"
#include "DBase.h"

#include <iostream>
#include <sstream>

// объявление функции из SearchServer.cpp
int run_server(const Config& cfg, Database& db);

int main(int argc, char* argv[])
{
    std::string cfgFile = "C:\\NetoC++\\SearchEngine\\Configs\\config.ini";
    if (argc > 1) cfgFile = argv[1];

    try 
    {
        Config cfg(cfgFile);

        std::ostringstream conn;
        conn << "host=" << cfg.GetDbHost()
            << " port=" << cfg.GetDbPort()
            << " dbname=" << cfg.GetDbName()
            << " user=" << cfg.GetDbUser()
            << " password=" << cfg.GetDbPass();

        Database db(conn.str());
        return run_server(cfg, db);
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }
}
