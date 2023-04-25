//------------------------------------------------------------
// asn1tree.cc - The tree of objects built from the MIBs
//
// Written by Graham Wheeler, August 1995
// (c) 1995, Open Mind Solutions (cc)
// All Rights Reserved
//
// Copying or using this software or derived works without
// the written permission of Open Mind Solutions is strictly
// prohibited.
//------------------------------------------------------------

#ifndef MIBTREE_H
#define MIBTREE_H

//------------------------------------------------------------------
// Forward declarations of classes

class clMIBTreeNode;		// base class
class clInteriorNode;		// interior nodes (have symtab entries)
class clInstanceNode;		// instance leaf nodes base class
class clStringNode;		// string instance leaf nodes
class clIntegerNode;		// integer instance leaf nodes

//------------------------------------------------------------------
// Base class for all tree nodes

class clMIBTreeNode
{
protected:
    int			id;		// object id element
    clMIBTreeNode	*parent;	// parent node ptr
    clMIBTreeNode	*firstchild;	// leftmost child
    clMIBTreeNode	*nextpeer;	// next sibling in list
public:
    clMIBTreeNode(clMIBTreeNode *parent_in, int	id_in);
    clMIBTreeNode *NextPeer();
    clMIBTreeNode *FirstChild();
    clMIBTreeNode *Parent();
    void NextPeer(clMIBTreeNode *nextpeer_in);
    void FirstChild(clMIBTreeNode *firstchild_in);
    clMIBTreeNode *AddChild(clMIBTreeNode *n);
    //clMIBTreeNode *AddChild(int id_in, clMIBTreeNode *n);
    clMIBTreeNode *FindChild(int idval);
    clMIBTreeNode *FindChildNum(int num);
    int NumChildren();
    char *NumericObjectID();
    void Dump(FILE *fp);
    int ID()
    {
	return id;
    }
    virtual int IsTable()
    {
	return 0;
    }
    virtual int HasInstance()
    {
	return 0;
    }

    // pure virtuals

    virtual clMIBTreeNode *FindChild(char *suffix) = 0;
    virtual const char *Name() = 0;
    virtual const char *Description() = 0;
    virtual char *ObjectID() = 0;
    virtual int IsInstance() = 0;

    virtual ~clMIBTreeNode();
};

//---------------------------------------------------------------
// interior nodes

class clInteriorNode : public clMIBTreeNode
{
    char *name;
public:
    clInteriorNode(clMIBTreeNode *parent_in, char *name_in, int id_in);
    virtual const char *Description() = 0;
    virtual char *ObjectID();
    virtual clMIBTreeNode *FindChild(char *suffix);
    virtual int IsInstance();
    virtual const char *Name()
    {
	return name;
    }
    virtual int IsTable()
    {
	return 0;
    }
    virtual int HasInstance()
    {
	return 0;
    }
    virtual ~clInteriorNode()
    {
	delete [] name;
    }
};

//--------------------------------------------------------------------
// Object ID nodes

class clObjectIDNode : public clInteriorNode
{
public:
    clObjectIDNode(clMIBTreeNode *parent_in, char *name_in, int id_in);
    virtual const char *Description()
    {
	return "";
    }
    virtual ~clObjectIDNode()
    { }
};

//------------------------------------------------------------------------
// Object-typ nodes

class clObjectTypNode : public clInteriorNode
{
    char *desc;
    enType typ;
    enAccess access;
    enStatus status;
public:
    clObjectTypNode(clMIBTreeNode *parent_in, char *name_in, int id_in, 
		enType typ_in, char *desc_in, enAccess access_in,
		enStatus status_in);
    virtual const char *Description()
    {
	return desc;
    }
    virtual int IsTable()
    {
	return (typ == TYP_SEQUENCEOF);
    }
    virtual int HasInstance()
    {
	if (typ == TYP_SEQUENCEOF || typ == TYP_SEQUENCE)
	    return 0;
	return 1;
    }
    enAccess Access()
    {
	return access;
    }
    enStatus Status()
    {
	return status;
    }
    enType Type()
    {
	return typ;
    }
    virtual ~clObjectTypNode()
    {
	delete [] desc;
    }
};

//-----------------------------------------------------------------
// Base class of leaf instance nodes.

class clInstanceNode : public clMIBTreeNode
{
public:
    clInstanceNode(clMIBTreeNode *parent_in, int id_in);
    virtual int IsInstance();
    virtual const char *Name();
    virtual const char *Description();
    virtual char *ObjectID();
    virtual clMIBTreeNode *FindChild(char *suffix);
    virtual ~clInstanceNode()
    { }
    virtual int HasInstance()
    {
	return 1;
    }
};

//-----------------------------------------------------------------
// String-valued instance nodes.

class clStringNode : public clInstanceNode
{
    char *val;
    int len;
public:
    clStringNode(clMIBTreeNode *parent_in, int id_in);
    virtual ~clStringNode()
    {
	delete [] val;
    }
};

//-----------------------------------------------------------------
// Integer-valued instance nodes.

class clIntegerNode : public clInstanceNode
{
    long val;
public:
    clIntegerNode(clMIBTreeNode *parent_in, int	id_in);
    virtual ~clIntegerNode()
    { }
};

//-------------------------------------------------------------------
// The MIB tree itself

// We want at least the following methods:

// Methods to compare objid's for ordering
// Methods to get a tree node given object ID string
// Method to get next leaf given an object ID string
// Method to get object ID string given node
// method to get object typ given obj id string

class clMIBTree
{
    clMIBTreeNode	*root;		// root of the tree
public:
    clMIBTree();
    clMIBTreeNode *Root();
    void SetRoot(clMIBTreeNode *root_in);
    clMIBTreeNode *GetNode(char *objid_in); // return 0 if no such node
    void AddNode(char *objid, clMIBTreeNode *node_in);
    void Dump(FILE *fp);
    int LoadMIB(char *fname);
    ~clMIBTree();
    //clMIBTreeNode *GetInstance(char *objid_in); // return 0 if not instance
    //clMIBTreeNode *GetNext(char *objid_in); // only return 0 if at last leaf
    //int CanSet(char (objid, char *berval); ??
};

#ifndef NO_INLINES
#  define INLINE inline
#  include "mibtree.inl"
#  undef INLINE
#endif

#endif

