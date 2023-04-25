//------------------------------------------------------------
// snmpman.h - SNMP manager functionality wrapper
//
// Written by Graham Wheeler, August 1995
// (c) 1995, Open Mind Solutions (cc)
// All Rights Reserved
//
// Copying or using this software or derived works without
// the written permission of Open Mind Solutions is strictly
// prohibited.
//------------------------------------------------------------

#ifndef SNMPMAN_H
#define SNMPMAN_H

class clSNMPManager
{
    clMIBTree *mib;
    int retrycnt;
    int tmout;
    char *host;
    char *community;
    char **objids; // used for table traversal via GetNexts
    int   numobjids;
    int   tblcol;
    clObjectTypNode *tbltyp;
public:
    clSNMPManager();
    clMIBTreeNode *MIBRoot()
    {
	return mib->Root();
    }
    char *Host()
    {
	return host;
    }
    char *Community()
    {
	return community;
    }
    void SetRetryCount(int retrycnt_in)
    {
	retrycnt = retrycnt_in;
    }
    void SetTimeout(int tmout_in)
    {
	tmout = tmout_in;
    }
    void SetHost(char *host_in);
    void SetCommunity(char *community_in);
    int LoadMIBs(char *fname);

    // scalar access

    char *GetValue(char *objid_in);
    char *Set(clMIBTreeNode *node, char *val);
    // also have set's that take an objid and a long or a string

    // table access. Use depends on whether row traversal or column traversal
    // is desired. Specify col_in = 0 (or omit parameter) for row traversal;
    // otherwise the specified column is traversed. Each call to GetNext
    // then returns an array of char values and the size of the array. When
    // a size of zero is returned, traversal of the table is complete.

    void SelectTable(char *objid_in, int col_in = 0);	// choose table
    int  GetNext(char **&values_out); // traverse by row

    ~clSNMPManager();
};

#endif

