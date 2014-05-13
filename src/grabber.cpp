////////////////////
//Author: Wireless//
////////////////////

#include "grabber.h"

#include <sstream>

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

std::vector<server> servers;

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

    //orginal function from sauerbraten code, a bit modified (tools.cpp)
    void filtertext(char *dst, const char *src, bool whitespace, int len)
    {
        for(int c = (unsigned char)*src; c; c = (unsigned char)*++src)
        {
            if(c == '\f')
            {
                if(!*++src) break;
                continue;
            }
            if(whitespace)
            {
                *dst++ = c;
                if(!--len) break;
            }
        }
        *dst = '\0';
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
Grabber::Grabber(int timeo, std::vector<unsigned long> blist)
{
    timeout = timeo;
    blacklist = blist;
}

Grabber::~Grabber()
{
}


bool Grabber::updatemaster(const char* host, int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
        return false;

    hostent* h;
    if((h = gethostbyname(host)) == NULL)
    {
        std::cout << "Can't resolve hostname!" << std::endl;
        return false;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, h->h_addr_list[0], sizeof(in_addr));

    timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv)) < 0)
    {
        std::cout << "Can't set socket option!" << std::endl;
    }
    if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv)) < 0)
    {
        std::cout << "Can't set socket option!" << std::endl;
    }

    if(connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        std::cout << "Can't connect to masterserver!" << std::endl;
        return false;
    }

    //fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);
    send(sock, "list\n", sizeof("list\n"), 0);

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

        bool blacklisted = false;
        for(int i=0;i<blacklist.size();i++)
        {
            if(blacklist[i] == tip)
            {
                blacklisted = true;
                break;
            }
        }

        if(blacklisted)
            continue;

        server serv;
        serv.ip = tip;
        serv.port = prt;

        servers.push_back(serv);
    }

    return true;
}


void Grabber::getextinfo(server &srv)
{
    sockaddr_in addr;
    int sock;

    if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        return;

    timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv)) < 0)
    {
        std::cout << "Can't set socket option!" << std::endl;
    }
    if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv)) < 0)
    {
        std::cout << "Can't set socket option!" << std::endl;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(srv.port + 1);
    addr.sin_addr.s_addr = srv.ip;
    unsigned int slen = sizeof(sockaddr_in);

    char message[3] = {1, 0, 0};
    sendto(sock, message, 3, 0, (sockaddr*)&addr, slen);

    unsigned char buf[1024];
    int rec = recvfrom(sock, buf, 1024, 0, (sockaddr*)&addr, &slen);
    if(rec > 0)
    {
        parseextinfo(-1, buf, rec, srv);
    }
    else
    {
        std::cout << "Timeout (IP: " << inet_ntoa(addr.sin_addr) << ")!" << std::endl;
        return;
    }

    message[0] = 0; //EXT_UPTIME
    sendto(sock, message, 3, 0, (sockaddr*)&addr, slen);
    rec = recvfrom(sock, buf, 1024, 0, (sockaddr*)&addr, &slen);
    if(rec > 0)
    {
        parseextinfo(0, buf, rec, srv);
    }

    srv.clients.clear();
    message[0] = 0; message[1] = 1; message[2] = -1; //EXT_PLAYERSTATS
    sendto(sock, message, 3, 0, (sockaddr*)&addr, slen);
    rec = recvfrom(sock, buf, 1024, 0, (sockaddr*)&addr, &slen); //recieve EXT_PLAYERSTATS_RESP_IDS but don't need it
    for(int i=0;i<srv.numplayers;i++)
    {
        rec = recvfrom(sock, buf, 1024, 0, (sockaddr*)&addr, &slen);
        if(rec > 0)
        {
            parseextinfo(1, buf, rec, srv);
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
}


void Grabber::parseextinfo(int type, unsigned char* data, int len, server& srv)
{
    char buf[260];
    netbuf nbuf(data, len);

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
            srv.numplayers = nbuf.getint();
            int attrfollowing = nbuf.getint();
            srv.protocol_version = nbuf.getint();
            srv.gamemode = nbuf.getint();
            nbuf.getint(); //maptime
            srv.maxclients = nbuf.getint();
            nbuf.getint(); //mode
            if(attrfollowing == 7)
            {
                nbuf.getint(); //gamepaused
                nbuf.getint(); //gamespeed
            }
            nbuf.getstring(buf, 260);
            nbuf.filtertext(buf, buf, true, 259);
            srv.mapname = std::string(buf);
            nbuf.getstring(buf, 260);
            nbuf.filtertext(buf, buf, true, 259);
            srv.serverdesc = std::string(buf);
        }break;
    case 0: //uptime
        {
            srv.uptime = nbuf.getint();
        }break;
    case 1: //playerstats
        {
            nbuf.getint(); //EXT_NO_ERROR
            nbuf.getint(); //EXT_PLAYERSTATS_RESP_STATS
            client temp;
            temp.clientnum = nbuf.getint();
            temp.ping = nbuf.getint();
            nbuf.getstring(buf, 260);
            temp.name = std::string(buf);
            nbuf.getstring(buf, 260);
            temp.team = std::string(buf);
            temp.frags = nbuf.getint();     temp.flags = nbuf.getint();
            temp.deaths = nbuf.getint();    temp.teamkills = nbuf.getint();
            temp.damage = nbuf.getint();    temp.health = nbuf.getint();
            temp.armour = nbuf.getint();    temp.gunselect = nbuf.getint();
            temp.privilege = nbuf.getint(); temp.state = nbuf.getint();
            //get ip
            temp.ip_range[0] = (unsigned char)nbuf.getint();
            temp.ip_range[1] = (unsigned char)nbuf.getint();
            temp.ip_range[2] = (unsigned char)nbuf.getint();

            srv.clients.push_back(temp);

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

std::string iptostr(unsigned long ip)
{
    in_addr addr; addr.s_addr = ip;
    std::string ret(inet_ntoa(addr));
    return ret;
}
