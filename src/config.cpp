////////////////////
//Author: Wireless//
////////////////////

#include "config.h"
#include <fstream>

#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netdb.h>


void Config::parseline(std::string line)
{
	if(line[0] == '#') //comment
		return;
	
	int pos = line.find(' ');
	if(pos == std::string::npos)
	{
		hostent *h;
		unsigned long ip = 0;
		if(inet_aton(line.c_str(), (in_addr*)&ip) == 0)
			h = gethostbyname(line.c_str());
        
        if(ip != 0)
        {
        	blacklist.push_back(ip);
        }
        else if(h != NULL && ip == 0)
        {
        	memcpy(&ip, h->h_addr_list[0], sizeof(unsigned long));
        	blacklist.push_back(ip);
        }    
        
		return;
	}
		
	std::string var(line.substr(0, pos));
	pos = line.rfind(' ');
	
	if(pos == std::string::npos)
		return;
	
	std::string val(line.substr(pos+1));
	if(var == (const char*)"masterhost")
	{
		masterhost = val;
	}
	else if(var == (const char*)"masterport")
	{
		masterport = atoi(val.c_str());
	}
	else if(var == (const char*)"timeoutsec")
	{
		timeoutsec = atoi(val.c_str());
	}
}


Config::Config(const char* file)
{
	//initalize default values
	masterhost = std::string("master.sauerbraten.org");
	masterport = 28787;

	std::fstream cfg(file, std::ios::in);
	if(cfg.is_open())
	{
		std::string line;
		while(std::getline(cfg, line))
		{
			if(line.length() > 0)
			{
				parseline(line);
			}
		}
	}
}

Config::~Config()
{
}

std::string Config::getmasterhost()
{
	return masterhost;
}

int Config::getmasterport()
{
	return masterport;
}

int Config::gettimeoutsec()
{
	return timeoutsec;
}
