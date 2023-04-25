// Very basic SNMP test program. Takes one or more object-IDs on the command
// line, does a GetRequest, and prints out the returned value(s).

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
#include <string.h>
#include <iostream.h>
#include <assert.h>

#include "berobj.h"
#include "clsrv.h"

class SNMPGetClient : public Client
{
    clGetRequestPDU *pdu;
public:
    SNMPGetClient(Connection *conn_in)
	: Client(conn_in)
    {
	pdu = new clGetRequestPDU();
    }
    void AddVar(char *objid)
    {
	pdu->AddBinding(objid);
    }
    virtual void Request();
};

void SNMPGetClient::Request()
{
    unsigned char buff[1024];
    clSNMPMessage m(pdu);
    int l = m.Encode(buff);
    Send(buff, l);
    if ((l=Receive(buff)) > 0)
    {
#ifdef DEBUG
        for (int i = 0; i < l; i++)
	    printf("%02X ", buff[i]);
	printf("\n\n");
        for (i = 0; i < l; i++)
	    printf("%c ", isprint(buff[i]) ? buff[i] : '?');
	printf("\n");
#endif
    	m.Decode(buff);
#ifdef DEBUG
    	m.Dump(cout);
#endif
	char *err = ((clSNMPPDU *)m.Data())->ErrorMessage();
	if (err)
	    puts(err);
	else
	{
    	    clVarBindList *bv = ((clSNMPPDU *)m.Data())->Bindings();
    	    bv->Rewind();
    	    for (clVarBinding *b = bv->Next(); b; b = bv->Next())
                printf("%s = %s\n", b->ObjectID(), b->Value()->Print(buff));
	}
    }
}
		 
void useage()
{	     
    fprintf(stderr, "Useage: snmpget <objectid> [ <objectid> ... ]\n");
    exit(-1);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
        useage();
    Connection *conn = new UDPConnection("gramppp", "snmp");
    SNMPGetClient cl(conn);
    for (int i = 1; i < argc; i++)
	cl.AddVar(argv[i]);
    cl.Run();
    return 0;
}


