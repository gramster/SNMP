//------------------------------------------------------------
// asn1tree.cc - MIB tree with hooks to MIB compiler
//
// Written by Graham Wheeler, August 1995
// (c) 1995, Open Mind Solutions (cc)
// All Rights Reserved
//
// Copying or using this software or derived works without
// the written permission of Open Mind Solutions is strictly
// prohibited.
//------------------------------------------------------------

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "smi.h"
/*
#include "asn1err.h"
#include "asn1name.h"
#include "asn1lex.h"
#include "asn1stab.h"
#include "asn1pars.h"
*/
#include "mibtree.h"

#ifdef NO_INLINES
#  define INLINE
#  include "mibtree.inl"
#  undef INLINE
#endif

//---------------------------------------------------------------------
// A useful utility routine to read a file of records of colon-separated
// fields. The caller is responsible for setting all the pointers in
// the field array to zero before the first call, and for deleting the
// returned strings after the last call (usually not necessary if 
// readfields returns zero).

int readfields(FILE *fp, char *flds[], int maxflds)
{
    for (int i = 0; i < maxflds; i++)
    {
	delete [] flds[i];
	flds[i] = 0;
    }
    char buff[1024], fld[1024];
    int fi = 0, last = 0, tchar = ':';
    fld[0] = 0;
    while (!feof(fp) && !last)
    {
	if (fgets(buff, 1024, fp)==0) break;
	if (buff[0]=='#') continue;
	int l = strlen(buff);
	buff[l-1] = 0; // strip newline

	// ugly replacement of terminal backslash with newline. The
	// replacement is done if we are building a multi line field
	// and we are not on the first line, or, if we are on the first
	// line, the backslash is not the first char in the field (this
	// latter check is to prevent a newline being put in field B
	// when we have something like:
	//
	//	A:\
	//	B
	
	if (l>1 && buff[l-2]=='\\')
	    buff[l-2] = ((l>3 && buff[l-3]!=':') || fld[0]) ? '\n' : 0;
	else last = 1; // line don't end with \ => end of record
	char *s = buff;
	for (;;)
	{
	    // check for start of string
	    if (fld[0]==0 && tchar==':' && *s == '"')
	    {
		tchar = '"';
		s++;
	    }
	    // look for end of field
	    char *t = strchr(s, tchar);
	    if (t)
	    {
		*t++ = 0;
		if (tchar == '"')
		{
		    tchar = ':'; // switch back to other type
		    if (*t == ':') // is next field on this line?
			t++; // set t to point to next field
		    else t = 0; // skip to next line
		}
	    }
	    strcat(fld, s);
	    s = t;
	    if (s)
	    {
		flds[fi] = new char [strlen(fld)+1];
		strcpy(flds[fi++], fld);
		fld[0] = 0;
	    }
	    else break;
	}
    }
    if (fld[0])
    {
        flds[fi] = new char [strlen(fld)+1];
        strcpy(flds[fi++], fld);
    }
    return fi;
}

//---------------------------------------------------------------------

void clMIBTreeNode::Dump(FILE *fp)
{
#if 0
    if (Description())
    {
	char *nid = NumericObjectID();
	char *sid = ObjectID();
        //fprintf(fp, "%s\n%s\n\t%s\n", nid, sid, Description());
        fprintf(fp, "%s\n\t%s\n", nid, sid);
	delete [] nid;
	delete [] sid;
	if (FirstChild() == 0) // leaf? Then print type info
	{
	    // this is presumptious if there are instances in the tree
	    clSymtabEntry *s = ((clObjectTypNode*)this)->Info();
	    if (s->Type())
		if (s->Type()->Index())
	            fprintf(fp, "\t\t%s\n", NameStore->Name(s->Type()->Index()));
		else if (s->Type()->TypeClass() == TYP_OCTETSTRING)
	            fprintf(fp, "\t\tOCTET STRING\n");
		else if (s->Type()->TypeClass() == TYP_INTEGER)
	            fprintf(fp, "\t\tINTEGER\n");
		else if (s->Type()->TypeClass() == TYP_ENUM)
	            fprintf(fp, "\t\tINTEGER\n");
	}
    }
    for (clMIBTreeNode *n = FirstChild(); n; n = n->NextPeer())
	n->Dump(fp);
#endif
}

clMIBTreeNode *clMIBTreeNode::FindChild(int idval)
{
    clMIBTreeNode *n = FirstChild();
    while (n && n->id != idval)
	n = n->NextPeer();
    return n;
}

clMIBTreeNode *clMIBTreeNode::FindChildNum(int num)
{
    for (clMIBTreeNode *e = FirstChild();
	 e != 0 && --num>=0 ;
	 e = e->NextPeer());
    return e;
}

int clMIBTreeNode::NumChildren()
{
    int rtn = 0;
    for (clMIBTreeNode *c = FirstChild(); c; c = c->NextPeer())
        rtn++;
    return rtn;
}

// Next routine assumes children are added in lexicographic order

clMIBTreeNode *clMIBTreeNode::AddChild(clMIBTreeNode *node_in)
{
    if (FirstChild())
    {
	clMIBTreeNode *n = FirstChild();
	while (n->NextPeer()) n = n->NextPeer();
    	n->nextpeer = node_in;
    }
    else firstchild = node_in;
    return node_in;
}

char *clMIBTreeNode::NumericObjectID()
{
    char *pid = parent ? parent->NumericObjectID() : 0, *rtn, buf[10];
    int len = pid ? (strlen(pid)+1) : 0;
    sprintf(buf, "%d", id);
    rtn = new char [len + strlen(buf) + 1];
    if (rtn)
    {
	if (pid)
	{
	    strcpy(rtn, pid);
	    strcat(rtn, ".");
	}
	else rtn[0] = 0;
	strcat(rtn, buf);
    }
    delete [] pid;
    return rtn;
}

clMIBTreeNode::~clMIBTreeNode()
{
    clMIBTreeNode *n = firstchild;
    while (n)
    {
        clMIBTreeNode *tmp = n;
	n = n->nextpeer;
	delete tmp;
    }
}

//---------------------------------------------------------------------------

char *clInteriorNode::ObjectID()
{
    char *pid = parent ? parent->ObjectID() : 0, *rtn;
    int len = pid ? (strlen(pid)+1) : 0;
    rtn = new char [len + strlen(Name()) + 1];
    if (rtn)
    {
	if (pid)
	{
	    strcpy(rtn, pid);
	    strcat(rtn, ".");
	}
	else rtn[0] = 0;
	strcat(rtn, Name());
    }
    delete [] pid;
    return rtn;
}

clMIBTreeNode *clInteriorNode::FindChild(char *suffix)
{
    if (suffix == 0) return 0;
    char *s = strchr(suffix, '.');
    if (s) s++;
    clMIBTreeNode *chld = 0;
    if (isdigit(suffix[0]))
        chld = clMIBTreeNode::FindChild(atoi(suffix));
    else
    {
	int len = s ? (s - suffix - 1) : strlen(suffix);
	chld = FirstChild();
	while (chld && strncmp(chld->Name(), suffix, len) != 0)
	    chld = chld->NextPeer();
    }
    if (chld && s)
	return chld->FindChild(s);
    else
	return chld;
}

int clInteriorNode::IsInstance()
{
    return 0;
}

//---------------------------------------------------------------

int clInstanceNode::IsInstance()
{
    return 1;
}

const char *clInstanceNode::Name()
{
    assert(parent);
    return parent->Name();
}

const char *clInstanceNode::Description()
{
    assert(parent);
    return parent->Description();
}

char *clInstanceNode::ObjectID()
{
    char *pid = parent ? parent->ObjectID() : 0, *rtn, buf[10];
    int len = pid ? (strlen(pid)+1) : 0;
    sprintf(buf, "%d", id);
    rtn = new char [len + strlen(buf) + 1];
    if (rtn)
    {
	if (pid)
	{
	    strcpy(rtn, pid);
	    strcat(rtn, ".");
	}
	else rtn[0] = 0;
	strcat(rtn, buf);
    }
    delete [] pid;
    return rtn;
}

clMIBTreeNode *clInstanceNode::FindChild(char *suffix)
{
    if (suffix == 0 || !isdigit(suffix[0])) return 0;
    char *s = strchr(suffix, '.');
    if (s) s++;
    clMIBTreeNode *chld = clMIBTreeNode::FindChild(atoi(suffix));
    if (chld && s)
	return chld->FindChild(s);
    else
	return chld;
}

//---------------------------------------------------------------------------

#if 0
void clMIBTree::AddNode(char *objid_in, clMIBTreeNode *node_in)
{
    clMIBTreeNode *n = root, *last = n;
    char *buf = new char[strlen(objid_in)+1], *p = buf;
    strcpy(buf, objid_in);
    while (p && n)
    {
	char *s = strchr(p, '.');
	if (s) *s++ = 0;
	last = n;
	n = n->FindChild(p);
	if (n==0) break;
	p = s;
    }
    if (n==0)
    {
	n = last->AddChild(p);
	p = s;
        while (p)
        {
	    char *s = strchr(p, '.');
	    if (s) *s++ = 0;
	    n = n->AddChild(p);
	    p = s;
        }
    }
    // else error, already defined!!
    delete [] buf;
}
#endif

clMIBTreeNode *clMIBTree::GetNode(char *objid_in)
{
    assert(objid_in[0] == '1');
    if (objid_in[1] == 0) return root;
#if 1
/*fprintf(stderr, "In GetNode(%s)\n", objid_in);*/
    clMIBTreeNode *n = root;
    char *buf = new char[strlen(objid_in)+1];
    strcpy(buf, objid_in);
    for (char *s, *p = buf+2; p && n; p = s)
    {
	s = strchr(p, '.');
	if (s) *s++ = 0;
	n = n->FindChild(atoi(p));
    }
    delete [] buf;
    return n;
#else
    return root->FindChild(objid_in+2);
#endif
}

void clMIBTree::Dump(FILE *fp)
{
    root->Dump(fp);
}

clMIBTree::~clMIBTree()
{
    delete root;
}

int clMIBTree::LoadMIB(char *fname)
{
    FILE *fp = fopen(fname, "r");
    if (fp == 0) return -1;
    char *flds[20];
    for (int i = 0; i < 20; i++) flds[i] = 0;
    for (;;)
    {
	int rtn = readfields(fp, flds, 20);
	if (rtn == 0) break;
	else if (rtn != 2 && rtn != 6) continue; // warn!!
	// split object id into parent node id and new single digit id
	int l = strlen(flds[1]);
	clMIBTreeNode *tn = 0;
	while (--l > 0 && flds[1][l] != '.');
	if (l>0)
	{
	    flds[1][l++] = 0;
	    tn = GetNode(flds[1]);
	}
	int id = atoi(flds[1]+l);
	if (rtn == 2) // object ID 
	{
	    if (tn)
		tn->AddChild(new clObjectIDNode(tn, flds[0], id));
	    else if (Root() == 0)
		SetRoot(new clObjectIDNode(tn, flds[0], id));
	    // else warn!!
	}
	else  // object typ
	{
	    enType typ;
	    switch(flds[2][0])
	    {
	    case 'T':
		typ = TYP_SEQUENCEOF;
		break;
	    case 't':
		if (flds[2][1] == 't') typ = TYP_TIMETICKS;
		else typ = TYP_SEQUENCE;
		break;
	    case 'e':
		typ = TYP_ENUM;
		break;
	    case 'o':
		if (flds[2][1] == 'i') typ = TYP_OBJECTID;
		else if (flds[2][1] == 'q') typ = TYP_OPAQUE;
		else typ = TYP_OCTETSTRING;
		break;
	    case 'i':
		if (flds[2][1] == 'p') typ = TYP_IPADDRESS;
		else typ = TYP_INTEGER;
		break;
	    case 'n':
		if (flds[2][1] == 'a') typ = TYP_NETADDRESS;
		else typ = TYP_NULL;
		break;
	    case 'c':
		typ = TYP_COUNTER;
		break;
	    case 'g':
		typ = TYP_GAUGE;
		break;
	    default:
		; // error!
	    }
	    enAccess access;
	    if (flds[3][0] == 'r')
	        access = ACC_READONLY;
	    else if (flds[3][0] == 'a')
	        access = ACC_READWRITE;
	    else if (flds[3][0] == 'w')
	        access = ACC_WRITEONLY;
	    else if (flds[3][0] == 'n')
	        access = ACC_UNACCESS;
	    enStatus status;
	    if (flds[4][0] == 'm')
	        status = STS_MANDATORY;
	    else if (flds[4][0] == 'o')
	        status = STS_OPTIONAL;
	    else if (flds[4][0] == 'd')
	        status = STS_DEPRECATED;
	    else if (flds[4][0] == 'x')
	        status = STS_OBSOLETE;
	    if (tn)
	        tn->AddChild(new clObjectTypNode(tn, flds[0], id, typ, flds[5], access, status));
	}
    }
    fclose(fp);
    return 0;
}


