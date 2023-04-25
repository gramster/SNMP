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
#include "clsrv.h"

#ifdef NO_INLINE
#include "clsrv.inl"
#endif

RolePlayer::~RolePlayer()
{
    delete conn;
}

Client::~Client()
{
}

Server::~Server()
{
    conn->Unbind();
}



