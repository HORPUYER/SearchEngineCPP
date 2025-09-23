#include "Config.h"
#include "DBase.h"
#include "Spider.h"

#include <iostream>
#include <sstream>

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
        Spider spider(cfg, db, 8); // 8 потоков
        spider.run();
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
