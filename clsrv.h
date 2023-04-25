// Client-server base classes
//
// Note that whoever instantiates a client or server must allocate
// a Connection for the class; the client or server will then destroy
// this connection itself.

#ifndef OMS_CLSRV_H
#define OMS_CLSRV_H

#ifndef INLINE
#  ifdef NO_INLINE
#    define INLINE
#  else
#    define INLINE inline
#  endif
#endif

#include "connect.h"

// Local pipe isn't removed if server is killed by a signal. Modify
// the code so that signals are caught and termination is graceful

class RolePlayer
{
protected:
    Connection *conn;
public:
    INLINE RolePlayer(Connection *conn_in);
    virtual ~RolePlayer();
    INLINE int Receive(char *buf, int tmout = 0);
    INLINE int ReceiveStr(char *buf, int tmout = 0);
    INLINE void Send(char *buf, int len);
    INLINE void Send(char *buf);
    INLINE int HasData();
};
    
class Client : public RolePlayer
{
public:
    INLINE Client(Connection *conn_in);
    virtual void Request() = 0;
    INLINE void Run(); // this is for consistency with servers
    virtual ~Client();
};

class Server : public RolePlayer
{
    int nofork;
public:
    INLINE Server(Connection *conn_in, int nofork_in = 0);
    virtual void Service() = 0;
    INLINE void Daemonize();
    INLINE void Run();
    virtual ~Server();
};

#ifndef NO_INLINE
#include "clsrv.inl"
#endif

#endif


