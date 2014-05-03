////////////////////
//Author: Wireless//
////////////////////

#include <iostream>
#include <string>
#include <vector>

#include "config.h"
#include "grabber.h"

using namespace std;

void printextinfo();

int main(int argc, char* argv[])
{
	Config cfg("grabext.conf");
	Grabber grabber(cfg.gettimeoutsec());
	cout << "[+]Listen(masterserver) ..." << endl;
    grabber.updatemaster(cfg.getmasterhost().c_str(), cfg.getmasterport());

    cout << "[+]Grabbing extinfos (total: " << servers.size() << ")!" << endl;
    for(int i=0;i<servers.size();i++)
    {
        grabber.getextinfo(servers[i]);
    }

    printextinfo();

	return 0;
}


void printextinfo()
{
    for(int i=0;i<servers.size();i++)
    {
        cout << servers[i].serverdesc << endl;
        cout << "##################" << endl;
        cout << "->Version: " << servers[i].protocol_version << endl;
        cout << "->Uptime: " << servers[i].uptime << " s" << endl;
        cout << "->Maxclients: " << servers[i].maxclients << endl;
        cout << "->Gamemode: " << servers[i].gamemode << endl;
        cout << "->Map: " << servers[i].mapname << endl;
        cout << "->Clients: " << servers[i].numplayers << endl;
        for(int a=0;a<servers[i].clients.size();a++)
        {
            cout << " +Name: " << servers[i].clients[a].name << " | Cn: " << servers[i].clients[a].clientnum << endl;
            cout << "  (Team: " << servers[i].clients[a].team << ")" << endl;
            cout << "  (Frags: " << servers[i].clients[a].frags << ")" << endl;
            cout << "  (Deaths: " << servers[i].clients[a].deaths << ")" << endl;
            cout << "  (Teamkills: " << servers[i].clients[a].teamkills << ")" << endl;
            cout << "  (Privilege: " << servers[i].clients[a].privilege << ")" << endl;
        }

        if(i != (servers.size() - 1))
            cout << endl << endl;
    }
}

