////////////////////
//Author: Wireless//
////////////////////

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

using namespace std;

class netbuf
{
private:
    std::vector<unsigned char> buf;
    int pos;
    bool overread;

    unsigned char get()
    {
        unsigned char ret = 0;

        if(pos >= 0 && pos < buf.size() && !overread)
        {
            ret = buf.at(pos);
            pos++;
        }
        else
            overread = true;

        return ret;
    }

    int remaining()
    {
        return (buf.size() - pos + 1);
    }

public:

    netbuf(unsigned char* data, int len)
    {
        pos = 0;
        overread = false;
        for(int i=0;i<len;i++)
        {
            buf.push_back(data[i]);
        }
    }

    ~netbuf()
    {
        buf.clear();
    }

    bool isoverread()
    {
        return overread;
    }

    //orginal function from sauerbraten code, a bit modified (tools.cpp)
    int getint()
    {
        int c = (char)get();
        if(c==-128) { int n = get(); n |= char(get())<<8; return n; }
        else if(c==-127) { int n = get(); n |= get()<<8; n |= get()<<16; return n|(get()<<24); }
        else return c;
    }

    //orginal function from sauerbraten code, a bit modified (tools.cpp)
    void getstring(char *text, int len)
    {
        char *t = text;
        do
        {
            if(t>=&text[len]) { text[len-1] = 0; return; }
            if(!remaining()) { *t = 0; return; }
            *t = getint();
        }
        while(*t++);
    }
};

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

std::vector<server> servers;

void parseextinfo(int type, unsigned char* data, int len, int listindex);
bool updatemaster(const char* host, int port);
bool getextinfo(unsigned long host, int port, int listindex);
void printextinfo();

int main(int argc, char* argv[])
{
    updatemaster("master.sauerbraten.org", 28787);

    cout << "[+]Grabbing extinfo!" << endl;
    for(int i=0;i<servers.size();i++)
    {
        getextinfo(servers.at(i).ip, servers.at(i).port + 1, i);
    }

    printextinfo();

	return 0;
}

bool updatemaster(const char* host, int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
        return false;

    hostent* h;
    if((h = gethostbyname(host)) == NULL)
        return false;

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, h->h_addr_list[0], sizeof(in_addr));

    timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv)) < 0)
    {
        cout << "Can't set socket option!" << endl;
    }
    if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv)) < 0)
    {
        cout << "Can't set socket option!" << endl;
    }

    if(connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        cout << "Can't connect to masterserver!" << endl;
        return false;
    }

    //fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);
    send(sock, "list\n", sizeof("list\n"), 0);

    cout << "[+]Listening(masterserver) ..." << endl;
    char buf[4096]; int rec = 0;
    std::string data;
    while(recv(sock, buf, 4096, 0))
    {
        data.append(buf);
    }

    close(sock);

	std::stringstream ss(data.c_str());
	std::string line;

	while(getline(ss, line, '\n'))
    {
        int pos = line.find(' ');
        if(pos == std::string::npos)
            continue;

        line.erase(0, pos + 1);
        pos = line.find(' ');
        if(pos == std::string::npos)
            continue;

        int prt = atoi(line.substr(pos + 1).c_str());
        unsigned long tip;
        inet_aton(line.substr(0, pos).c_str(), (in_addr*)&tip);

        server serv;
        //memset(&serv, 0, sizeof(server));
        serv.ip = tip;
        serv.port = prt;

        servers.push_back(serv);
    }

    return true;
}

bool getextinfo(unsigned long host, int port, int listindex)
{
    if(listindex < 0 || listindex >= servers.size())
    {
        return false;
    }

    sockaddr_in addr;
    int sock;

    if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        return false;

    timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv)) < 0)
    {
        cout << "Can't set socket option!" << endl;
    }
    if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv)) < 0)
    {
        cout << "Can't set socket option!" << endl;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = host;
    unsigned int slen = sizeof(sockaddr_in);

    cout << "[+]Get ExtInfo from: " << inet_ntoa(addr.sin_addr) << endl;

    char message[3] = {1, 0, 0};
    sendto(sock, message, 3, 0, (sockaddr*)&addr, slen);

    unsigned char buf[1024];
    int rec = recvfrom(sock, buf, 1024, 0, (sockaddr*)&addr, &slen);
    if(rec > 0)
	{
	    parseextinfo(-1, buf, rec, listindex);
	}

	message[0] = 0; //EXT_UPTIME
	sendto(sock, message, 3, 0, (sockaddr*)&addr, slen);
    rec = recvfrom(sock, buf, 1024, 0, (sockaddr*)&addr, &slen);
    if(rec > 0)
	{
	    parseextinfo(0, buf, rec, listindex);
	}

    servers[listindex].clients.clear();
	message[0] = 0; message[1] = 1; message[2] = -1; //EXT_PLAYERSTATS
	sendto(sock, message, 3, 0, (sockaddr*)&addr, slen);
	rec = recvfrom(sock, buf, 1024, 0, (sockaddr*)&addr, &slen); //recieve EXT_PLAYERSTATS_RESP_IDS but don't need it
	for(int i=0;i<servers[listindex].numplayers;i++)
    {
        rec = recvfrom(sock, buf, 1024, 0, (sockaddr*)&addr, &slen);
        if(rec > 0)
        {
            parseextinfo(1, buf, rec, listindex);
        }
    }

    /*message[0] = 0; message[1] = 2; //EXT_TEAMSCORE
	sendto(sock, message, 3, 0, (sockaddr*)&addr, slen);
    rec = recvfrom(sock, buf, 1024, 0, (sockaddr*)&addr, &slen);
    if(rec > 0)
	{
	    parseextinfo(2, buf, rec, listindex);
		std::cout << rec << " bytes recieved (teamscore)!" << std::endl;
	}*/

    close(sock);

    return true;
}

/////////////////////////////////////////////////////////////////////////
void parseextinfo(int type, unsigned char* data, int len, int listindex)
{
    char buf[260];
    netbuf nbuf(data, len);

    if(listindex < 0 || listindex >= servers.size())
    {
        return;
    }

    //remove our data (which the server sends back!)
    nbuf.getint(); nbuf.getint(); nbuf.getint();

    if(type >= 0) //get "ext-header", which normal ext-info packets doesn't contain
    {
        nbuf.getint(); //EXT_ACK = -1
        nbuf.getint(); //EXT_VERSION = 105
    }

    switch(type)
    {
    case -1:
        {
            servers[listindex].numplayers = nbuf.getint();
            int attrfollowing = nbuf.getint();
            servers[listindex].protocol_version = nbuf.getint();
            servers[listindex].gamemode = nbuf.getint();
            nbuf.getint(); //maptime
            servers[listindex].maxclients = nbuf.getint();
            nbuf.getint(); //mode
            if(attrfollowing == 7)
            {
                nbuf.getint(); //gamepaused
                nbuf.getint(); //gamespeed
            }
            nbuf.getstring(buf, 260);
            servers[listindex].mapname = string(buf);
            nbuf.getstring(buf, 260);
            servers[listindex].serverdesc = string(buf);
        }break;
    case 0: //uptime
        {
            servers[listindex].uptime = nbuf.getint();
        }break;
    case 1: //playerstats
        {
            nbuf.getint(); //EXT_NO_ERROR
            nbuf.getint(); //EXT_PLAYERSTATS_RESP_STATS
            client temp;
            temp.clientnum = nbuf.getint();
            temp.ping = nbuf.getint();
            nbuf.getstring(buf, 260);
            temp.name = string(buf);
            nbuf.getstring(buf, 260);
            temp.team = string(buf);
            temp.frags = nbuf.getint();     temp.flags = nbuf.getint();
            temp.deaths = nbuf.getint();    temp.teamkills = nbuf.getint();
            temp.damage = nbuf.getint();    temp.health = nbuf.getint();
            temp.armour = nbuf.getint();    temp.gunselect = nbuf.getint();
            temp.privilege = nbuf.getint(); temp.state = nbuf.getint();

            servers[listindex].clients.push_back(temp);

        }break;
    case 2: //teaminfo
        {
            //teammode
            //gamemode
            //gametime

            //if teammode:

        }break;
    default:
        //error
        break;
    }
}

void printextinfo()
{
    for(int i=0;i<servers.size();i++)
    {
        cout << servers[i].serverdesc << endl;
        cout << "##################" << endl;
        cout << "->Uptime: " << servers[i].uptime << endl;
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

