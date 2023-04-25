INLINE RolePlayer::RolePlayer(Connection *conn_in)
    : conn(conn_in)
{
}

INLINE int RolePlayer::Receive(char *buf, int tmout)
{
    return conn->Receive(buf, tmout);
}

INLINE int RolePlayer::ReceiveStr(char *buf, int tmout)
{
    return conn->ReceiveStr(buf, tmout);
}

INLINE void RolePlayer::Send(char *buf, int len)
{
    conn->Send(buf, len);
}

INLINE void RolePlayer::Send(char *buf)
{
    conn->Send(buf);
}
 
INLINE int RolePlayer::HasData()
{
    return conn->HasData();
}
   
INLINE Client::Client(Connection *conn_in)
    : RolePlayer(conn_in)
{
}

INLINE void Client::Run() // this is for consistency with servers
{
    conn->Connect();
    Request();
}

INLINE Server::Server(Connection *conn_in, int nofork_in)
    : RolePlayer(conn_in), nofork(nofork_in)
{
    if (conn)
    {
	conn->Bind();
	conn->Listen();
    }
}

INLINE void Server::Daemonize()
{
    int pid = fork();
    if (pid < 0)
	perror("fork");
    else if (pid > 0)
    {
	conn->Close();
	exit(0);
    }
    else
    {
	setsid();
	chdir("/");
	umask(0);
    }
}

INLINE void Server::Run()
{
    for (;;)
    {
	int sd = conn->Accept();
	if (sd < 0) continue;
	if (nofork)
	{
	    int tmpd = conn->NewFD(sd);
	    Service(); // should pass remote address
            close(sd);
	    (void)conn->NewFD(tmpd);
	}
	else if (fork() > 0)
	    close(sd);
	else
	{
	    (void)conn->NewFD(-1);
	    (void)conn->NewFD(sd);
	    Service(); // should pass remote address
	    exit(0);
	}
    }
}












