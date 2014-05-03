////////////////////
//Author: Wireless//
////////////////////

#ifndef GRABBER_H
#define GRABBER_H

#include <iostream>
#include <string>
#include <vector>

struct client
{
    int clientnum;
    int ping;
    std::string name;
    std::string team;
    int frags;
    int flags;
    int deaths;
    int teamkills;
    int damage;
    int health;
    int armour;
    int gunselect;
    int privilege;
    int state;
};

struct server
{
    unsigned long ip;
    int port;
    int protocol_version;
    int maxclients;
    std::string serverdesc;
    std::string mapname;
    int uptime;

    int gamemode;
    int numplayers;
    std::vector<client> clients;
};

extern std::vector<server> servers;

class Grabber
{
private:
	int timeout;
public:
	Grabber(int timeo);
	~Grabber();
	bool updatemaster(const char* host, int port);
	void getextinfo(server &srv);
	void parseextinfo(int type, unsigned char* data, int len, server &srv);
};

#endif
