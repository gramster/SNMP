//--------------------------------------------------------------------
// Inlines for MIB tree classes
//
// Written by Graham Wheeler, August 1995
// (c) 1995, Open Mind Solutions (cc)
// All Rights Reserved
//
// Copying or using this software or derived works without
// the written permission of Open Mind Solutions is strictly
// prohibited.
//------------------------------------------------------------

INLINE clMIBTreeNode::clMIBTreeNode(clMIBTreeNode *parent_in, int id_in)
    : parent(parent_in), firstchild(0), id(id_in), nextpeer(0)
{
}

INLINE clMIBTreeNode *clMIBTreeNode::NextPeer()
{
    return nextpeer;
}

INLINE clMIBTreeNode *clMIBTreeNode::FirstChild()
{
    return firstchild;
}

INLINE clMIBTreeNode *clMIBTreeNode::Parent()
{
    return parent;
}

INLINE void clMIBTreeNode::NextPeer(clMIBTreeNode *nextpeer_in)
{
    nextpeer = nextpeer_in;
}

INLINE void clMIBTreeNode::FirstChild(clMIBTreeNode *firstchild_in)
{
    firstchild = firstchild_in;
}

//--------------------------------

INLINE clInteriorNode::clInteriorNode(clMIBTreeNode *parent_in, char *name_in,
		int id_in)
    : clMIBTreeNode(parent_in, id_in)
{
    name = new char[strlen(name_in)+1];
    strcpy(name, name_in);
}

//--------------------------------

INLINE clObjectIDNode::clObjectIDNode(clMIBTreeNode *parent_in, char *name_in,
		int id_in)
    : clInteriorNode(parent_in, name_in, id_in)
{
}

//--------------------------------

INLINE clObjectTypNode::clObjectTypNode(clMIBTreeNode *parent_in, char *name_in,
		int id_in, enType typ_in, char *desc_in, enAccess access_in,
		enStatus status_in)
    : clInteriorNode(parent_in, name_in, id_in),
	access(access_in), status(status_in), typ(typ_in)
{
    desc = new char[strlen(desc_in)+1];
    strcpy(desc, desc_in);
}

//--------------------------------

INLINE clInstanceNode::clInstanceNode(clMIBTreeNode *parent_in, int id_in)
    : clMIBTreeNode(parent_in, id_in)
{
}

//--------------------------------

INLINE clStringNode::clStringNode(clMIBTreeNode *parent_in, int id_in)
    : clInstanceNode(parent_in, id_in), val(0), len(0)
{
}

//--------------------------------

INLINE clIntegerNode::clIntegerNode(clMIBTreeNode *parent_in, int id_in)
    : clInstanceNode(parent_in, id_in)
{
}

//--------------------------------

INLINE clMIBTree::clMIBTree()
    : root(0)
{
}

INLINE clMIBTreeNode *clMIBTree::Root()
{
    return root;
}

INLINE void clMIBTree::SetRoot(clMIBTreeNode *root_in)
{
    delete root;
    root = root_in;
}

//--------------------------------


