#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/in.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#include "connect.h"

#ifdef NO_INLINE
#include "connect.inl"
#endif

static int timedout = 0;

static void
sigalarm(int signo)
{
    timedout = 1;
    (void)signo;
}

Connection::~Connection()
{
    Unbind();
    Close();
    delete [] addr;
}

void Connection::Bind()
{
    if (fd>=0 && local && bind(fd, local, addrsz) < 0)
    {
	perror("Socket bind");
	Close();
    }
#ifdef DEBUG
    else fputs("Bound socket\n", stderr);
#endif
}

void Connection::Unbind() 
{
}

void Connection::Listen()
{
    if (fd >= 0 && listen(fd, 5) < 0)
    {
	perror("listen");
	Close();
    }
#ifdef DEBUG
    else fputs("Done listen\n", stderr);
#endif
}

void Connection::Connect()
{
    if (fd>=0 && remote && connect(fd, remote, addrsz) < 0)
    {
	perror("Socket connect");
	Close();
    }
#ifdef DEBUG
    else fputs("Done connect\n", stderr);
#endif
}

int Connection::Accept()
{
    struct sockaddr from;
    int addrlen;
    int sd = accept(fd, &from, &addrlen);
    if (sd  < 0)
	perror("accept");
#ifdef DEBUG
    else fputs("Done accept\n", stderr);
#endif
    return sd;
}

int Connection::Receive(char *buf, int tmout)
{
    buf[0] = 0;
    if (fd>=0)
    {
	char buff[1024];
	if (tmout)
	{
	    (void)signal(SIGALRM, sigalarm);
	    alarm(tmout);
	    timedout = 0;
	}
	int rtn = recvfrom(fd, buff, 1024, 0, remote, &addrsz);
	if (rtn >= 0)
	{
	    memcpy(buf, buff, rtn);
	    buf[rtn] = 0;
	}
	else if (timedout)
	    return TIMEDOUT;
	else
	    perror("Socket recvfrom");
	return rtn;
    }
    else return -1;
}

void Connection::Send(char *buf, int len)
{
    if (fd >= 0)
	if (send(fd, buf, len, 0)<0)
	    perror("Socket send");
}	

void LocalConnection::Bind()
{
    local = MakeAddress(&locaddr);
    addrsz = sizeof(locaddr);
    Connection::Bind();
}

void LocalConnection::Connect()
{	
    remote = NULL;
    if (fd>=0)
    {
	remote = MakeAddress(&remaddr);
	addrsz = sizeof(remaddr);
	Connection::Connect();
    }
}

void LocalConnection::Unbind()
{
    if (addr) unlink(addr);
}

void INetConnection::Bind()
{
    local = MakeAddress(&locaddr);
    addrsz = sizeof(locaddr);
    Connection::Bind();
}

void INetConnection::Connect()
{	
    if (fd>=0)
    {
	remote = MakeAddress(&remaddr);
	addrsz = sizeof(remaddr);
	Connection::Connect();
    }
    else remote = NULL;
}

void INetConnection::Listen()
{
    Connection::Listen();
}

void INetConnection::Send(char *buf, int len)
{
    Connection::Send(buf, len);
}

int INetConnection::Receive(char *buf, int tmout)
{
    return Connection::Receive(buf, tmout);
}

int INetConnection::Accept()
{
    return Connection::Accept();
}

void UDPConnection::Listen()
{
#ifdef DEBUG
    fputs("Done listen\n", stderr);
#endif
}

void UDPConnection::Send(char *buf, int len)
{
    if (fd >= 0)
	if (sendto(fd, buf, len, 0, remote, addrsz)<0)
	    perror("Socket sendto");
}	

int UDPConnection::Receive(char *buf, int tmout)
{
    buf[0] = 0;
    if (fd>=0)
    {
	char buff[1024];
	if (tmout)
	{
	    (void)signal(SIGALRM, sigalarm);
	    alarm(tmout);
	    timedout = 0;
	}
	int rtn = recvfrom(fd, buff, 1024, 0, remote, &addrsz);
	if (rtn >= 0)
	{
	    memcpy(buf, buff, rtn);
	    buf[rtn] = 0;
	}
	else if (timedout)
	    return TIMEDOUT;
	else
	    perror("Socket recvfrom");
	return rtn;
    }
    else return -1;
}

int UDPConnection::Accept()
{
#ifdef DEBUG
    fputs("Done accept\n", stderr);
#endif
    return dup(fd);
}




