//------------------------------------------------------------
// snmpman.cc - SNMP manager functionality wrapper
//
// Written by Graham Wheeler, August 1995
// (c) 1995, Open Mind Solutions (cc)
// All Rights Reserved
//
// Copying or using this software or derived works without
// the written permission of Open Mind Solutions is strictly
// prohibited.
//------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>

#include "connect.h"
#include "clsrv.h"
/*
#include "asn1err.h"
#include "asn1name.h"
#include "asn1stab.h"
*/
#include "smi.h"
#include "mibtree.h"
#include "berobj.h"
#include "snmpman.h"

// SNMP client class for issuing Get Requests

class SNMPGetClient : public Client
{
    clSNMPMessage *m;
    char *rtn;
    clObjectTypNode *typ;
    int retrycnt;
    int tmout;
    void SetReturn(char *val)
    {
	rtn = new char[strlen(val)+1];
	strcpy(rtn, val);
    }
public:
    SNMPGetClient(Connection *conn_in, char *comm_in, char *objid_in,
		clObjectTypNode *typ_in, int retrycnt_in = 1, int tmout_in = 0)
	: Client(conn_in), rtn(0), typ(typ_in), retrycnt(retrycnt_in), tmout(tmout_in)
    {
	clGetRequestPDU *pdu = new clGetRequestPDU();
        m = new clSNMPMessage(pdu, comm_in);
	pdu->AddBinding(objid_in);
    }
    virtual void Request();
    char *Result()
    {
	return rtn;
    }
    ~SNMPGetClient()
    {
	delete m;
    }
};

void SNMPGetClient::Request()
{
    unsigned char sbuff[1024], buff[1024];
    int len = m->Encode(sbuff);
    for (int tr = 0; tr < retrycnt; tr++)
    {
        Send(sbuff, len);
        int rtn = Receive(buff, tmout);
        if (rtn > 0 && rtn != TIMEDOUT)
        {
    	    m->Decode(buff);
	    char *err = ((clSNMPPDU *)m->Data())->ErrorMessage();
	    if (err) SetReturn(err);
	    else
	    {
    	        clVarBindList *bv = ((clSNMPPDU *)m->Data())->Bindings();
    	        bv->Rewind();
	        bv->Next()->Value()->Print(buff);
#if 0 // fix this!!!!!!!!!!!!
	        if (typ->Type() == TYP_ENUM)
	        {
		    clEnumEntry *e = (clEnumEntry*)typ->Info()->Type();
		    int idx = atoi(buff);
		    clEnumEltEntry *ee = (clEnumEltEntry*)e->List();
		    while (ee)
		    {
		        if (ee->Value() == idx)
		        {
			    strcpy(buff, NameStore->Name(ee->Index()));
			    break;
		        }
		        ee = (clEnumEltEntry *)ee->Next();
		    }
	        }
#endif
	        SetReturn(buff);
	    }
	    return;
        }
    }
    SetReturn("(No response from host)");
}

//-----------------------------------------------------------------------

class SNMPSetClient : public Client
{
    clSNMPMessage *m;
    char *rtn;
    int retrycnt;
    int tmout;
    void SetReturn(char *val)
    {
	rtn = new char[strlen(val)+1];
	strcpy(rtn, val);
    }
public:
    SNMPSetClient(Connection *conn_in, char *comm_in, char *objid_in,
		  clObjectTypNode *node_in, char *val_in, int retrycnt_in = 1,
		  int tmout_in = 0);
    virtual void Request();
    char *Result()
    {
	return rtn;
    }
    ~SNMPSetClient()
    {
	delete m;
    }
};

SNMPSetClient::SNMPSetClient(Connection *conn_in, char *comm_in, char *objid_in,
			     clObjectTypNode *node_in, char *val_in,
			     int retrycnt_in, int tmout_in)
	: Client(conn_in), rtn(0), retrycnt(retrycnt_in), tmout(tmout_in)
{ 
    clSetRequestPDU *pdu = new clSetRequestPDU();
    m = new clSNMPMessage(pdu, comm_in);
    clASN1Object *v = 0;
#if 0
    clSymtabEntry *e = node_in->Info();
    enType typ;
    // find base type, with a yukky kludge for IP addresses
    int ipidx = NameStore->Lookup("IpAddress");
    while (e->Type())
    {
        typ = e->Type()->TypeClass();
	if (e->Type()->Index() == ipidx)
	{
	    v = new clIPAddress(val_in);
fprintf(stderr, "Created an IPAddress for value\n");
	    break;
	}
// MUST ALSO CHECK FOR ENUMS AND TRANSLATE SYMBOLIC VALS TO INTS HERE!!
	e = e->Type();
    }
    if (v==0) switch (typ)
#else
    switch (node_in->Type())
#endif
    {
    case TYP_IPADDRESS:
	v = new clIPAddress(val_in);
fprintf(stderr, "Created an IPAddress for value\n");
	break;
    case TYP_OCTETSTRING:
	v = new clASN1OctetString(val_in);
fprintf(stderr, "Created an OctetString for value\n");
	break;
    case TYP_OBJECTID:
	v = new clASN1ObjectIdentifier(val_in);
fprintf(stderr, "Created an ObjectIdentifier for value\n");
	break;
    case TYP_INTEGER:
    case TYP_ENUM:
	v = new clASN1Integer(atol(val_in));
fprintf(stderr, "Created an Integer for value\n");
	break;
    default:
	delete m;
	m = 0;
	v = 0;
    }
fprintf(stderr, "Adding binding %s, %s\n", objid_in, val_in);
    if (v) pdu->AddBinding(objid_in, v);
}

void SNMPSetClient::Request()
{
    unsigned char sbuff[1024], buff[1024];
    if (m == 0) return;
    int len = m->Encode(sbuff);
    for (int tr = 0; tr < retrycnt; tr++)
    {
        Send(sbuff, len);
        int rtn = Receive(buff, tmout);
        if (rtn > 0 && rtn != TIMEDOUT)
        {
    	    m->Decode(buff);
	    char *err = ((clSNMPPDU *)m->Data())->ErrorMessage();
	    if (err) SetReturn(err);
	    return;
        }
    }
    SetReturn("(No response from host)");
}

//--------------------------------------------------------
// SNMP client class for issuing Get Requests

class SNMPGetNextClient : public Client
{
    clSNMPMessage *m;
    clGetNextRequestPDU *pdu;
    clObjectTypNode *tbltyp;
    int retrycnt;
    int tmout;
public:
    SNMPGetNextClient(Connection *conn_in, char *comm_in, 
		clObjectTypNode *tbltyp_in, int retrycnt_in = 1, int tmout_in = 0)
	: Client(conn_in), tbltyp(tbltyp_in), retrycnt(retrycnt_in), tmout(tmout_in)
    {
	pdu = new clGetNextRequestPDU();
        m = new clSNMPMessage(pdu, comm_in);
    }
    void AddObject(char *objid)
    {
	if (objid) pdu->AddBinding(objid);
    }
    virtual void Request();
    int Result(char **&ids, char **&vals); // returns 0 on failure
    char *Error()
    {
	return ((clSNMPPDU*)m->Data())->ErrorMessage();
    }
    ~SNMPGetNextClient()
    {
	delete m;
    }
};

void SNMPGetNextClient::Request()
{
    unsigned char buff[1024], sbuff[1024];
    int len = m->Encode(sbuff);
    for (int tr = 0; tr < retrycnt; tr++)
    {
        Send(sbuff, len);
        int rtn = Receive(buff);
        if (rtn > 0 && rtn != TIMEDOUT)
        {
    	    m->Decode(buff);
#if 0
fprintf(stderr, "Community %s\n", m->Community());
fprintf(stderr, "Version %d\n", m->Version());
clGetResponsePDU *pdu = (clGetResponsePDU*)(m->Data());
fprintf(stderr, "Request id %d\n", pdu->RequestID());
fprintf(stderr, "Error status %d\n", pdu->ErrorStatus());
fprintf(stderr, "Error index %d\n", pdu->ErrorIndex());
fprintf(stderr, "Error message %s\n", pdu->ErrorMessage());
clVarBindList *vb = pdu->Bindings();
vb->Rewind();
clVarBinding *b;
while ((b = vb->Next()) != 0)
{
    char buf[1024];
    fprintf(stderr, "%s = %s\n", b->ObjectID(), b->Value()->Print(buf));
}
#endif
	    return;
        }
    }
    tmout = -1;
}

int SNMPGetNextClient::Result(char **&ids, char **&vals)
{
    int vi = 0;
    ids = vals = 0;
    if (tmout < 0) return -1;
    if (Error() == 0)
    {
	// we assume a limit of 64 entries!
	char *tids[64], *tvals[64], buff[1024];
	clGetResponsePDU *pdu = (clGetResponsePDU*)m->Data();
    	clVarBindList *bv = pdu->Bindings();
    	bv->Rewind();
	clObjectTypNode *me = (clObjectTypNode*)tbltyp->FirstChild();
	for (clVarBinding *b = bv->Next();
		b && vi<64;
		b = bv->Next(), vi++, me = (clObjectTypNode*)me->NextPeer())
	{
	    tids[vi] = new char[strlen(b->ObjectID())+1];
	    strcpy(tids[vi], b->ObjectID());
	    (void)b->Value()->Print(buff);
	    // Handle enumerations
#if 0 // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! fix
	    if (me->Info()->Type() && me->Info()->Type()->TypeClass() == TYP_ENUM)
	    {
		clEnumEntry *e = (clEnumEntry*)me->Info()->Type();
		int idx = atoi(buff);
		clEnumEltEntry *ee = (clEnumEltEntry*)e->List();
		while (ee)
		{
		    if (ee->Value() == idx)
		    {
			strcpy(buff, NameStore->Name(ee->Index()));
			break;
		    }
		    ee = (clEnumEltEntry *)ee->Next();
		}
	    }
#endif
	    tvals[vi] = new char[strlen(buff)+1];
	    strcpy(tvals[vi], buff);
	}
	// if (b != 0) warn that some entries are skipped!!
	if (vi > 0)
	{
	    ids = new char*[vi];
	    vals = new char*[vi];
	    assert(ids != 0 && vals != 0);
	    for (int i = 0; i < vi; i++)
	    {
		ids[i] = tids[i];
		vals[i] = tvals[i];
	    }
	}
    }
/*else fprintf(stderr, "Skipping processing, error is %s\n", Error());*/
    return vi;
}

//-----------------------------------------------------------------------

void clSNMPManager::SetHost(char *host_in)
{
    assert(host_in);
    host = new char[strlen(host_in)+1];
    assert(host);
    strcpy(host, host_in);
}

void clSNMPManager::SetCommunity(char *community_in)
{
    assert(community_in);
    community = new char[strlen(community_in)+1];
    assert(community);
    strcpy(community, community_in);
}

int clSNMPManager::LoadMIBs(char *fname)
{
    return mib->LoadMIB(fname);
}

char *clSNMPManager::GetValue(char *objid)
{
    char bigbuf[1024];
    strcpy(bigbuf, objid);
    strcat(bigbuf, ".0");
    SNMPGetClient c(new UDPConnection(Host(), "snmp"), Community(), bigbuf,
	(clObjectTypNode*)mib->GetNode(objid), retrycnt, tmout);
    c.Run();
    return c.Result();
}

// Set scalar given object typ node and value (as a string)

char *clSNMPManager::Set(clMIBTreeNode *node, char *val)
{
    char bigbuf[1024], *t = node->NumericObjectID();
    strcpy(bigbuf, t);
    strcat(bigbuf, ".0");
    delete [] t;
    SNMPSetClient c(new UDPConnection(Host(), "snmp"), Community(), bigbuf,
			(clObjectTypNode*)node, val, retrycnt, tmout);
    c.Run();
    return c.Result();
}

// Table traversal
//
// To understand the mechanism, note that objects within tables are
// identified by a three part ID, of the form:
//
//  [Table object ID].[Column ID].[Row index info]
//
// To traverse a column, the table object ID and column number should
// be catenated and used in the first call to get-next to get the first
// column entry. The returned object ID should be used for subsequent
// calls to get next, terminating when the [Table objid].[col id] prefix
// is no longer matched by the returned object ID.
//
// Traversing a row is done in an identical fashion, except that a
// sequence of object ids are used, representing one column each.

void clSNMPManager::SelectTable(char *objid_in, int col_in)
{
    if (objids) // delete old table info, if any
    {
	for (int i = 0; i < numobjids; i++)
	    delete [] objids[i];
	delete [] objids;
	objids = 0;
    }
    clMIBTreeNode *n = mib->GetNode(objid_in);
    assert(n && n->IsTable());
    tblcol = col_in;
    tbltyp = (clObjectTypNode*)n->FirstChild();
    // initialise objids to point to row or table objects
    if (col_in > 0) // one column
    {
	objids = new char*[1];
	objids[0] = new char[strlen(objid_in)+10];
	sprintf(objids[0], "%s.%d", objid_in, col_in);
	numobjids = 1;
    }
    else // all columns
    {
        clMIBTreeNode *seq = n->FirstChild();
	assert(seq->NextPeer() == 0);
	numobjids = 0;
	for (clMIBTreeNode *c = seq->FirstChild(); c; c = c->NextPeer())
	    numobjids++;
#if 0
	char *nid = n->NumericObjectID();
	int len = strlen(nid)+10;
#endif
	if (numobjids > 0)
	{
	    objids = new char*[numobjids];
	    numobjids = 0;
	    for (clMIBTreeNode *c = seq->FirstChild(); c; c = c->NextPeer())
	    {
#if 0
		objids[numobjids] = new char[len];
		assert(objids[numobjids]);
		sprintf(objids[numobjids], "%s.%d", nid, c->ID());
		numobjids++;
#else
		objids[numobjids++] = c->NumericObjectID();
#endif
	    }
	}
#if 0
	delete [] nid;
#endif
    }
/*fprintf(stderr, "After Select, num %d, ptr %X\n", numobjids, objids);*/
}

int  clSNMPManager::GetNext(char **&values_out)
{
    char **ids, **vals;
    values_out = 0;
    if (objids == 0) return 0;
    SNMPGetNextClient c(new UDPConnection(Host(), "snmp"), Community(), tbltyp,
				retrycnt, tmout);
    for (int i = 0; i < numobjids; i++)
	c.AddObject(objids[i]);
    int l = strlen(objids[0]);
    char *save = new char[l+1];
    strcpy(save, objids[0]);
    while (save[--l] != '.');
    save[l] = 0;
    c.Run();
    int rtn = c.Result(ids, vals);
    for (i = 0; i < numobjids; i++)
	delete [] objids[i];

    if (rtn > 0)
    {
	assert(rtn == numobjids);
	if (strncmp(save, ids[0], l) != 0) // out of table
	{
	    // free everything up
	    for (int i = 0; i < numobjids; i++)
	    {
		delete [] ids[i];
		delete [] vals[i];
	    }
	    delete [] objids;
	    delete [] vals;
	    objids = 0;
	    numobjids = 0;
	    rtn = 0;
	}
	else
	{
	    for (int i = 0; i < numobjids; i++)
		objids[i] = ids[i];
	    values_out = vals;
	}
	delete [] ids;
    }
    else
    {
	assert(ids == 0 && vals == 0);
	delete [] objids;
	objids = 0;
	numobjids = 0;
    }
    delete [] save;
    return rtn;
}

//----------------------------------------------------------------------

clSNMPManager::clSNMPManager()
    : host(0), community(0), objids(0), numobjids(0), retrycnt(1), tmout(0)
{
    // kludge 8-(
/*
    ErrorHandler = new clErrorHandler;
    assert(ErrorHandler);
    NameStore = new clNameStore;
    assert(NameStore);
*/
    mib = new clMIBTree;
}

clSNMPManager::~clSNMPManager()
{
    delete mib;
    delete [] community;
    delete [] host;
/*
    delete ErrorHandler;
    delete NameStore;
    ErrorHandler = 0;
    NameStore = 0;
*/
}


