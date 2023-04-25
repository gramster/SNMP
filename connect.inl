INLINE Connection::Connection(char *addr_in)
    : local(0), remote(0), fd(-1)
{
    addr = new char[strlen(addr_in)+1];
    if (addr) strcpy(addr, addr_in);
}

INLINE void Connection::Close()
{
    if (fd >= 0) close(fd);
    fd = -1;
}

INLINE int Connection::HasEvent() // true if socket has event, including connect indication
{
    fd_set read_map;
    FD_ZERO(&read_map);
    FD_SET(fd, &read_map);
    int num_fds = select(fd+1, &read_map, NULL, NULL, NULL);
    if (num_fds<0)
	perror("select");
    else if (num_fds>0 && FD_ISSET(fd, &read_map))
	return 1;
    return 0;
}

INLINE int Connection::HasData() // True if socket has data to be read
{
    int rtn;
    return (ioctl(fd, FIONREAD, (char*)&rtn) >= 0 && rtn >= 0);
}

INLINE int Connection::ReceiveStr(char *buf, int tmout)
{
    int rtn = Receive(buf, tmout);
    if (rtn >=0) buf[rtn] = 0;
    return rtn;
}

INLINE void Connection::Send(char *buf)
{
    Send(buf, strlen(buf));
}

INLINE int Connection::NewFD(int newfd)
{
    int oldfd = fd;
    if (newfd < 0) close(fd);
    fd = newfd;
    return oldfd;
}

INLINE LocalConnection::LocalConnection(char *sockname)
    : Connection(sockname)
{
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd<0)
	perror("Domain socket");
}

INLINE struct sockaddr *LocalConnection::MakeAddress(struct sockaddr_un *sa)
{
    sa->sun_family = AF_UNIX;
    strcpy(sa->sun_path, addr);
    return (struct sockaddr *)sa;
}

INLINE unsigned long INetConnection::GetRemote(char *host)
{
    if (!isdigit(host[0]))
    {
	struct hostent *he = gethostbyname(host);
	if (he)
	    return (unsigned long)(*((long*)(he->h_addr)));
	else perror("Unknown host");
    }
    else return inet_addr(host);
    return 0;
}

INLINE struct sockaddr *INetConnection::MakeAddress(struct sockaddr_in *sa)
{
    sa->sin_family = AF_INET;
    sa->sin_port = port;
    sa->sin_addr.s_addr =  GetRemote(addr);
    for (int i = 0; i < sizeof(sa->sin_zero); i++)
	sa->sin_zero[i] = 0;
    return (struct sockaddr *)sa;
}

INLINE INetConnection::INetConnection(char *addr_in, int port_in)
    : Connection(addr_in), port(port_in)
{
}

INLINE INetConnection::INetConnection(char *addr_in, char *srv_in, char *prot_in)
    : Connection(addr_in)
{
    struct servent *se = getservbyname(srv_in, prot_in);
    if (se) 
    {
	port = se->s_port;
#ifdef DEBUG
	fprintf(stderr, "Attaching to port %d\n", ntohs(port));
#endif
    }
    else perror("getservbyname");
}

INLINE void UDPConnection::Init()
{
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd<0)
	perror("UDP socket");
    else
    {
	local = (struct sockaddr *)&locaddr;
	remote = (struct sockaddr *)&remaddr;
    }
}

INLINE UDPConnection::UDPConnection(char *addr_in, int port_in)
    : INetConnection(addr_in, port_in)
{
    Init();
}

INLINE UDPConnection::UDPConnection(char *addr_in, char *serv_in)
    : INetConnection(addr_in, serv_in, "udp")
{
    Init();
}

INLINE void TCPConnection::Init()
{
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd<0)
	perror("TCP socket");
}

INLINE TCPConnection::TCPConnection(char *addr_in, int port_in)
    : INetConnection(addr_in, port_in)
{
    Init();
}

INLINE TCPConnection::TCPConnection(char *addr_in, char *serv_in)
    : INetConnection(addr_in, serv_in, "tcp")
{
    Init();
}



