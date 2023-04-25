#ifndef ASN1STAB_H
#define ASN1STAB_H

class clSymtabEntry;
class clMIBTreeNode;
class clMIBTree;

#define F_EXPORTED	1
#define F_IMPORTED	2
#define F_MARKED	4

class clSymtabEntry
{
protected:
    int		 	index;
    unsigned		flags;
    enType		typeclass;
    clSymtabEntry	*type;
    clSymtabEntry	*next;
    friend class clSymtab;
public:
    clSymtabEntry(enType typeclass_in, int idx_in,
		clSymtabEntry *type_in = 0)
	: typeclass(typeclass_in),
	  index(idx_in), next(0), flags(0),
	  type(type_in)
    { }
    clSymtabEntry(enType typeclass_in, char *name_in,
		clSymtabEntry *type_in = 0)
	: typeclass(typeclass_in),
	  index(NameStore->Lookup(name_in)), next(0), flags(0),
	  type(type_in)
    { }
    enType TypeClass()
    {
	return typeclass;	
    }
    int Index()
    {
	return index;
    }
    const char *Name()
    {
	return index ? NameStore->Name(index) : "NONAME";
    }
    clSymtabEntry *Next()
    {
	return next;
    }
    clSymtabEntry *Type()
    {
	return type;
    }
    void Type(clSymtabEntry *t)
    {
	type = t;
    }
    void Export()
    {
        flags |= F_EXPORTED;
    }
    void MarkImported()
    {
        flags |= F_IMPORTED;
    }
    int IsExported()
    {
        return ((flags & F_EXPORTED) != 0);
    }
    int IsImported()
    {
        return ((flags & F_IMPORTED) != 0);
    }
    void Mark()
    {
    	flags |= F_MARKED;
    	if (type) type->Mark();
    }
    int IsMarked()
    {
        return ((flags & F_MARKED) != 0);
    }
    void SetIndex(int idx_in)
    {
	index = idx_in;
    }
    void SetNext(clSymtabEntry *next_in)
    {
	next = next_in;
    }
    void SetType(clSymtabEntry *type_in)
    {
	type = type_in;
    }
    // Override the next two for any classes that must be dumped to the
    // MIB DB file
    virtual void WriteObjID(FILE *fp)
    { }
    virtual void DBWrite(FILE *fp)
    { }
    char *TypeID();	// for MIB database
    virtual ~clSymtabEntry()
    { }
#ifdef DEBUG
    char *TypeClassName();
    virtual void Dump(FILE *fp);
    //void Dump(FILE *fp);
#endif
};

//-------------------------------------------------------------------
// Specialisations

class clPatchRef
{
    clPatchRef *next;
    clSymtabEntry *ref;
public:
    clPatchRef(clSymtabEntry *ref_in, clPatchRef *next_in = 0)
	: next(next_in), ref(ref_in)
    { }
    clPatchRef *Next()
    {
	return next;
    }
    clSymtabEntry *Ref()
    {
	return ref;
    }
    ~clPatchRef();
};

class clUndefinedEntry: public clSymtabEntry
{
    clPatchRef *ref;
public:
    clUndefinedEntry(int idx_in)
	: clSymtabEntry(TYP_UNDEFINED, idx_in), ref(0)
    { }
    clUndefinedEntry(char *name_in)
	: clSymtabEntry(TYP_UNDEFINED, name_in)
    { }
    void AddRef(clSymtabEntry *ref_in)
    {
	ref = new clPatchRef(ref_in, ref);
    }
    clPatchRef *GetRef()
    {
	return ref;
    }
    virtual ~clUndefinedEntry()
    { }
#ifdef DEBUG
    virtual void Dump(FILE *fp);
#endif
};

class clNamedTypeEntry: public clSymtabEntry
{
public:
    clNamedTypeEntry(int idx_in, clSymtabEntry *type_in)
	: clSymtabEntry(TYP_NAMEDTYPE, idx_in, type_in)
    { }
    clNamedTypeEntry(char *name_in, clSymtabEntry *type_in)
	: clSymtabEntry(TYP_NAMEDTYPE, name_in, type_in)
    { }
    virtual ~clNamedTypeEntry()
    { }
#ifdef DEBUG
    virtual void Dump(FILE *fp);
#endif
};

class clElementTypeEntry: public clSymtabEntry // sequence elt
{
public:
    clElementTypeEntry(int idx_in, clSymtabEntry *type_in)
	: clSymtabEntry(TYP_ELEMENTTYPE, idx_in, type_in)
    { }
    clElementTypeEntry(char *name_in, clSymtabEntry *type_in)
	: clSymtabEntry(TYP_ELEMENTTYPE, name_in, type_in)
    { }
    virtual ~clElementTypeEntry()
    { }
#ifdef DEBUG
    virtual void Dump(FILE *fp);
#endif
};

class clObjIDEntry: public clSymtabEntry
{
    int val;
    clSymtabEntry *parent;
    clMIBTreeNode *node;
public:
    clObjIDEntry(clObjIDEntry *parent_in, int idx_in, long val_in)
	: clSymtabEntry(TYP_OBJECTID, idx_in),
	  parent(parent_in), val(val_in), node(0)
    { }
    clObjIDEntry(clObjIDEntry *parent_in, char *name_in, long val_in)
	: clSymtabEntry(TYP_OBJECTID, name_in),
	  parent(parent_in), val(val_in), node(0)
    { }
    int Value()
    {
	return val;
    }
    clMIBTreeNode *Node()
    {
	return node;
    }
    void SetNode(clMIBTreeNode *node_in)
    {
	node = node_in;
    }
    clSymtabEntry *Parent()
    {
	return parent;
    }
    virtual void WriteObjID(FILE *fp);
    virtual void DBWrite(FILE *fp)
    {
	fprintf(fp, "%s:", NameStore->Name(Index()));
	WriteObjID(fp);
	fprintf(fp, "\n");
    }
    virtual ~clObjIDEntry()
    { }
#ifdef DEBUG
    virtual void Dump(FILE *fp);
#endif
};

class clTaggedTypeEntry: public clSymtabEntry
{
    long val;
    enClass clas;
public:
    clTaggedTypeEntry(enType typeclass_in, int idx_in, enClass class_in,
			long val_in, clSymtabEntry *type_in)
	: clSymtabEntry(typeclass_in, idx_in, type_in),
		clas(class_in), val(val_in)
    { }
    clTaggedTypeEntry(enType typeclass_in, char *name_in, enClass class_in,
			long val_in, clSymtabEntry *type_in)
	: clSymtabEntry(typeclass_in, name_in, type_in),
		clas(class_in), val(val_in)
    { }
    char *ClassName();
    virtual ~clTaggedTypeEntry()
    { }
#ifdef DEBUG
    virtual void Dump(FILE *fp);
#endif
};

class clImplicitTagEntry: public clTaggedTypeEntry
{
public:
    clImplicitTagEntry(int idx_in, enClass class_in, long val_in, clSymtabEntry *type_in)
	: clTaggedTypeEntry(TYP_IMPLICITTAG, idx_in, class_in, val_in, type_in)
    { }
    clImplicitTagEntry(char *name_in, enClass class_in, long val_in, clSymtabEntry *type_in)
	: clTaggedTypeEntry(TYP_IMPLICITTAG, name_in, class_in, val_in, type_in)
    { }
    virtual ~clImplicitTagEntry()
    { }
};

class clTaggedEntry: public clTaggedTypeEntry
{
public:
    clTaggedEntry(int idx_in, enClass class_in, long val_in, clSymtabEntry *type_in)
	: clTaggedTypeEntry(TYP_TAGGED, idx_in, class_in, val_in, type_in)
    { }
    clTaggedEntry(char *name_in, enClass class_in, long val_in, clSymtabEntry *type_in)
	: clTaggedTypeEntry(TYP_TAGGED, name_in, class_in, val_in, type_in)
    { }
    virtual ~clTaggedEntry()
    { }
};

class clStringEntry: public clSymtabEntry
{
    int minlen, maxlen;
public:
    clStringEntry(int idx_in, clSymtabEntry *type_in, int minlen_in = 0,
			int maxlen_in = -1)
	: clSymtabEntry(TYP_OCTETSTRING, idx_in, type_in),
		minlen(minlen_in), maxlen(maxlen_in)
    { }
    clStringEntry(char *name_in, clSymtabEntry *type_in, int minlen_in = 0,
			int maxlen_in = -1)
	: clSymtabEntry(TYP_OCTETSTRING, name_in, type_in),
		minlen(minlen_in), maxlen(maxlen_in)
    { }
    long MinLen()
    {
	return minlen;
    }	
    long MaxLen()
    {
	return maxlen;
    }	
    virtual ~clStringEntry()
    { }
#ifdef DEBUG
    virtual void Dump(FILE *fp);
#endif
};

class clStringValueEntry: public clSymtabEntry
{
    char *value;
public:
    void SetValue(char *value_in)
    {
	delete [] value;
	value = new char[strlen(value_in)+1];
	strcpy(value, value_in);
    }
    clStringValueEntry(int idx_in, clSymtabEntry *type_in, char *value_in)
	: clSymtabEntry(TYP_OCTETSTRING, idx_in, type_in), value(0)
    {
	SetValue(value_in);
    }
    clStringValueEntry(char *name_in, clSymtabEntry *type_in, char *value_in)
	: clSymtabEntry(TYP_OCTETSTRING, name_in, type_in), value(0)
    {
	SetValue(value_in);
    }
    virtual ~clStringValueEntry()
    {
	delete [] value;
    }
#ifdef DEBUG
    virtual void Dump(FILE *fp);
#endif
};

class clNullValueEntry: public clSymtabEntry
{
public:
    clNullValueEntry(int idx_in, clSymtabEntry *type_in)
	: clSymtabEntry(TYP_NULL, idx_in, type_in)
    { }
    clNullValueEntry(char *name_in, clSymtabEntry *type_in)
	: clSymtabEntry(TYP_NULL, name_in, type_in)
    { }
    virtual ~clNullValueEntry()
    { }
#ifdef DEBUG
    virtual void Dump(FILE *fp);
#endif
};

class clIntegerEntry: public clSymtabEntry
{
    long minval, maxval;
public:
    clIntegerEntry(int idx_in, clSymtabEntry *type_in,
		   long minval_in, long maxval_in)
	: clSymtabEntry(TYP_INTEGER, idx_in, type_in),
		minval(minval_in), maxval(maxval_in)
    { }
    clIntegerEntry(char *name_in, clSymtabEntry *type_in,
		   long minval_in, long maxval_in)
	: clSymtabEntry(TYP_INTEGER, name_in, type_in),
		minval(minval_in), maxval(maxval_in)
    { }
    long MinVal()
    {
	return minval;
    }	
    long MaxVal()
    {
	return maxval;
    }	
    virtual ~clIntegerEntry()
    { }
#ifdef DEBUG
    virtual void Dump(FILE *fp);
#endif
};

class clIntegerValueEntry: public clSymtabEntry
{
    long val;
public:
    clIntegerValueEntry(int idx_in, clSymtabEntry *type_in, long val_in)
	: clSymtabEntry(TYP_INTEGER, idx_in, type_in), val(val_in)
    { }
    clIntegerValueEntry(char *name_in, clSymtabEntry *type_in, long val_in)
	: clSymtabEntry(TYP_INTEGER, name_in, type_in), val(val_in)
    { }
    virtual ~clIntegerValueEntry()
    { }
#ifdef DEBUG
    virtual void Dump(FILE *fp);
#endif
};

class clEnumEltEntry: public clSymtabEntry
{
    long val;
public:
    clEnumEltEntry(int idx_in, clSymtabEntry *type_in, long val_in)
	: clSymtabEntry(TYP_ENUMELT, idx_in, type_in), val(val_in)
    { }
    clEnumEltEntry(char *name_in, clSymtabEntry *type_in, long val_in)
	: clSymtabEntry(TYP_ENUMELT, name_in, type_in), val(val_in)
    { }
    long Value()
    {
	return val;
    }
    virtual ~clEnumEltEntry()
    { }
#ifdef DEBUG
    virtual void Dump(FILE *fp);
#endif
};

class clEnumEntry: public clSymtabEntry
{
    clSymtabEntry *list;
public:
    clEnumEntry(int idx_in, clSymtabEntry *type_in, clSymtabEntry *list_in)
	: clSymtabEntry(TYP_ENUM, idx_in, type_in), list(list_in)
    { }
    clEnumEntry(char *name_in, clSymtabEntry *type_in, clSymtabEntry *list_in)
	: clSymtabEntry(TYP_ENUM, name_in, type_in), list(list_in)
    { }
    clSymtabEntry *List()
    {
	return list;
    }
    virtual ~clEnumEntry() // should delete list
    { }
#ifdef DEBUG
    virtual void Dump(FILE *fp);
#endif
};

class clIndexEltEntry: public clSymtabEntry // incomplete
{
    clSymtabEntry *table; // table type (SEQUENCE) containing this index elt
public:
    clIndexEltEntry(int idx_in, clSymtabEntry *table_in)
	: clSymtabEntry(TYP_INDEXELT, idx_in), table(table_in)
    { }
    clIndexEltEntry(char *name_in, clSymtabEntry *table_in)
	: clSymtabEntry(TYP_INDEXELT, name_in), table(table_in)
    { }
    clSymtabEntry *Table()
    {
	return table;
    }
    virtual ~clIndexEltEntry() // should delete table
    { }
#ifdef DEBUG
    virtual void Dump(FILE *fp);
#endif
};

class clSequenceEntry: public clSymtabEntry // incomplete
{
    clSymtabEntry *list;
public:
    clSequenceEntry(int idx_in, clSymtabEntry *list_in)
	: clSymtabEntry(TYP_SEQUENCE, idx_in, 0), list(list_in)
    { }
    clSequenceEntry(char *name_in, clSymtabEntry *list_in)
	: clSymtabEntry(TYP_SEQUENCE, name_in, 0), list(list_in)
    { }
    clSymtabEntry *List()
    {
	return list;
    }
    virtual ~clSequenceEntry() // should delete list
    { }
#ifdef DEBUG
    virtual void Dump(FILE *fp);
#endif
};

class clSequenceOfEntry: public clSymtabEntry // incomplete
{
public:
    clSequenceOfEntry(int idx_in, clSymtabEntry *type_in)
	: clSymtabEntry(TYP_SEQUENCEOF, idx_in, type_in)
    { }
    clSequenceOfEntry(char *name_in, clSymtabEntry *type_in)
	: clSymtabEntry(TYP_SEQUENCEOF, name_in, type_in)
    { }
    virtual ~clSequenceOfEntry()
    { }
#ifdef DEBUG
    virtual void Dump(FILE *fp);
#endif
};

class clObjectTypEntry: public clSymtabEntry // incomplete
{
    clSymtabEntry *val, *index;
    clObjIDEntry *objid;
    enAccess access;
    enStatus status;
    char *ref, *desc;
public:
    clObjectTypEntry(int idx_in, clSymtabEntry *type_in, clSymtabEntry *val_in,
		     enAccess access_in, enStatus status_in, char *ref_in,
		     char *desc_in, clSymtabEntry *index_in,
		     clObjIDEntry *objid_in)
	: clSymtabEntry(TYP_OBJECTTYP, idx_in, type_in),
	  val(val_in), access(access_in), status(status_in), ref(0),
	  desc(0), index(index_in), objid(objid_in)
    {
	SetDesc(desc_in);
	SetRef(ref_in);
    }
    clObjectTypEntry(char *name_in, clSymtabEntry *type_in, clSymtabEntry *val_in,
		     enAccess access_in, enStatus status_in, char *ref_in,
		     char *desc_in, clSymtabEntry *index_in,
		     clObjIDEntry *objid_in)
	: clSymtabEntry(TYP_OBJECTTYP, name_in, type_in),
	  val(val_in), access(access_in), status(status_in), ref(0),
	  desc(0), index(index_in), objid(objid_in)
    {
	SetDesc(desc_in);
	SetRef(ref_in);
    }
    void SetDesc(char *desc_in);
    void SetRef(char *ref_in);
    char *Desc()
    {
	return desc;
    }
    clObjIDEntry *ObjID()
    {
	return objid;
    }
    clMIBTreeNode *Node()
    {
	return ObjID()->Node();
    }
    enAccess Access()
    {
	return access;
    }
    enStatus Status()
    {
	return status;
    }
    void SetNode(clMIBTreeNode *node_in)
    {
	ObjID()->SetNode(node_in);
    }
    clSymtabEntry *Parent()
    {
	return objid->Parent();
    }
    char *Ref()
    {
	return ref;
    }
    virtual void WriteObjID(FILE *fp);
    virtual void DBWrite(FILE *fp);
    virtual ~clObjectTypEntry()
    {
	delete [] desc;
	delete [] ref;
    }
#ifdef DEBUG
    virtual void Dump(FILE *fp);
#endif
};

//-------------------------------------------------------------------
// The Symbol Table

class clSymtab
{
    clSymtabEntry *first, *last;
public:
    clSymtab()
        : first(0), last(0)
    { }
    ~clSymtab();
    clSymtabEntry *IsDefined(int index_in);
    clSymtabEntry *Redefine(clSymtabEntry *olde, clSymtabEntry *newe);
    clSymtabEntry *Define(clSymtabEntry *e);
    clSymtabEntry *Define(enType typeclass_in, int index_in,
				clSymtabEntry *type_in = 0);
    clSymtabEntry *Find(int index_in);
    clSymtabEntry *Find(char *name_in);
    clSymtabEntry *FindStringType(clSymtabEntry *typ, int minlen, int maxlen);
    clSymtabEntry *FindIntegerType(clSymtabEntry *typ, long minval, long maxval);
    void MergeImports(clSymtab *stab);
    void MarkImport(int idx);
    void Dump(FILE *fp);
    void WriteObjID(FILE *fp, clObjIDEntry *n);
    void BuildIDTable(); // this should really be file driven...
    void WriteMIB(FILE *fp);
};

inline clSymtabEntry *clSymtab::Find(char *name_in)
{
    return Find(NameStore->Lookup(name_in));
}

#endif

