#ifndef OMS_CONNECT_H
#define OMS_CONNECT_H

#ifndef INLINE
#  ifdef NO_INLINE
#    define INLINE
#  else
#    define INLINE inline
#  endif
#endif

const int TIMEDOUT = 9999;

// Local pipe isn't removed if server is killed by a signal. Modify
// the code so that signals are caught and termination is graceful

// Connection classes - these wrap up sockets

class Connection
{
protected:
    int fd;
    struct sockaddr *local;
    struct sockaddr *remote;
    char *addr; // address for bind or connect
    int addrsz;
public:
    INLINE Connection(char *addr_in);
    INLINE void Close();
    virtual ~Connection();
    virtual void Bind();
    virtual void Unbind();
    virtual void Listen();
    virtual void Connect();
    virtual int Accept();
    INLINE int HasEvent();
    INLINE int HasData();
    virtual int Receive(char *buf, int tmout = 0);
    INLINE int ReceiveStr(char *buf, int tmout = 0);
    virtual void Send(char *buf, int len);
    INLINE void Send(char *buf);
    INLINE int NewFD(int newfd);
};

class LocalConnection : public Connection
{
protected:
    struct sockaddr_un locaddr;
    struct sockaddr_un remaddr;
public:
    INLINE LocalConnection(char *sockname);
    INLINE struct sockaddr *MakeAddress(struct sockaddr_un *sa);
    virtual void Bind();
    virtual void Connect();
    virtual void Unbind();
};

class INetConnection : public Connection
{
protected:
    struct sockaddr_in locaddr;
    struct sockaddr_in remaddr;
    int port;
    INLINE unsigned long GetRemote(char *host);
public:
    INLINE struct sockaddr *MakeAddress(struct sockaddr_in *sa);
    INLINE INetConnection(char *addr_in, int port_in);
    INLINE INetConnection(char *addr_in, char *srv_in, char *prot_in);
    virtual void Bind();
    virtual void Connect();
    virtual void Listen();
    virtual void Send(char *buf, int len);
    virtual int Receive(char *buf, int tmout = 0);
    virtual int Accept();
};

class UDPConnection : public INetConnection
{
    INLINE void Init();
public:
    INLINE UDPConnection(char *addr_in, int port_in);
    INLINE UDPConnection(char *addr_in, char *serv_in);
    virtual void Listen();
    virtual void Send(char *buf, int len);
    virtual int Receive(char *buf, int tmout = 0);
    virtual int Accept();
};

class TCPConnection : public INetConnection
{
    INLINE void Init();
public:
    INLINE TCPConnection(char *addr_in, int port_in);
    INLINE TCPConnection(char *addr_in, char *serv_in);
};

#ifndef NO_INLINE
#include "connect.inl"
#endif

#endif


