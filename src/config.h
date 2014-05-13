////////////////////
//Author: Wireless//
////////////////////

#ifndef CONFIG_H
#define CONFIG_H
#include <iostream>
#include <string>
#include <vector>

class Config
{
private:
    std::string masterhost;
    int masterport;
    int timeoutsec;

    void parseline(std::string line);
public:
    std::vector<unsigned long> blacklist;

    Config(const char* cfile);
    ~Config();
    std::string getmasterhost();
    int getmasterport();
    int gettimeoutsec();
};

#endif
