#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "smi.h"
#include "asn1err.h"
#include "asn1name.h"
#include "asn1stab.h"

typedef struct
{
    char *name;
    char *id;
    int idx;
} typlookup_t;

static typlookup_t lookuptbl[32];

char *
clSymtabEntry::TypeID()
{
    if (TypeClass() == TYP_SEQUENCE)
	return "t";
    else if (TypeClass() == TYP_SEQUENCEOF)
	return "T";
    else if (TypeClass() == TYP_ENUM)
	return "en";
    for (int i = 0; lookuptbl[i].id; i++)
	if (Index() == lookuptbl[i].idx)
	    return lookuptbl[i].id;
    if (TypeClass() == TYP_OCTETSTRING)
	return "o";
    else if (TypeClass() == TYP_INTEGER)
	return "i";
    // else a subtype (e.g. tagged), in which case we recurse
    assert(Type());
    return Type()->TypeID();
}

#ifdef DEBUG

static char *classnames[] = 
{
    "UNDEFINED",
    "INTEGER",
    "NULL",
    "SEQUENCE",
    "SEQUENCEOF",
    "OBJECTID",
    "OCTETSTRING",
    "TAGGED",
    "IMPLICITTAG",
    "NAMEDTYPE",
    "ELEMENTTYPE",
    "ENUMELT",
    "ENUM",
    "OBJECTTYP",
    "INDEXELT",
    "MODULE"
};

char *
clSymtabEntry::TypeClassName()
{
    return classnames[(int)typeclass];
}

void 
clSymtabEntry::Dump(FILE *fp)
{
    if (Index() > 0)
        fprintf(fp, "%-20s", NameStore->Name(Index()));
    else
        fprintf(fp, "(Unnamed)");
}

void 
clUndefinedEntry::Dump(FILE *fp)
{
    fprintf(fp, "%-20s : UNDEFINED", NameStore->Name(Index()));
}

void 
clNamedTypeEntry::Dump(FILE *fp)
{
    fprintf(fp, "%-20s", NameStore->Name(Index()));
}

void 
clElementTypeEntry::Dump(FILE *fp)
{
    fprintf(fp, "%-20s ", NameStore->Name(Index()));
    if (type)
	if (type->Index())
	    fprintf(fp, "%s ", NameStore->Name(type->Index()));
	else type->Dump(fp);
}

void 
clObjIDEntry::Dump(FILE *fp)
{
    if (parent)
	fprintf(fp, "%-20s OBJECT IDENTIFIER ::= {%s %d}",
		NameStore->Name(Index()),
		NameStore->Name(parent->Index()),
		val);
    else
	fprintf(fp, "%-20s OBJECT IDENTIFIER ::= {%d}",
		NameStore->Name(Index()), val);
}

void 
clTaggedTypeEntry::Dump(FILE *fp)
{
    fprintf(fp, "%-20s %sTAGGED [%s %d]",
		  NameStore->Name(Index()),
		  typeclass==TYP_TAGGED ? "" : "IMPLICIT ",
		  ClassName(), val);
    if (type) type->Dump(fp);
}

void 
clStringEntry::Dump(FILE *fp)
{
    fprintf(fp, "%-20s OCTET STRING", Index() ? NameStore->Name(Index()): "");
    if (minlen == maxlen)
	fprintf(fp, "(SIZE (%d))", maxlen);
    else if (minlen != 0 || maxlen != -1)
	fprintf(fp, "(SIZE (%d..%d))", minlen, maxlen);
}

void 
clStringValueEntry::Dump(FILE *fp)
{
    fprintf(fp, "%-20s OCTET STRING ::= %s",
		NameStore->Name(Index()), value);
}

void 
clNullValueEntry::Dump(FILE *fp)
{
    fprintf(fp, "%-20s NULL ::= NULL", NameStore->Name(Index()));
}

void 
clIntegerEntry::Dump(FILE *fp)
{
    fprintf(fp, "%-20s INTEGER", Index() ? NameStore->Name(Index()) : "");
    if (maxval < minval)
	fprintf(fp, " (%lu..%lu)", minval, maxval);
    else
	fprintf(fp, " (%ld..%ld)", minval, maxval);
}

void 
clIntegerValueEntry::Dump(FILE *fp)
{
    fprintf(fp, "%-20s INTEGER ::= %ld", NameStore->Name(Index()), val);
}

void 
clEnumEltEntry::Dump(FILE *fp)
{
    fprintf(fp, "%s (%d)", NameStore->Name(Index()), val);
}

void 
clEnumEntry::Dump(FILE *fp)
{
    fprintf(fp, "%s {", NameStore->Name(type->Index()));
    clSymtabEntry *l = list;
    while (l)
    {
        fprintf(fp, "\n\t");
	l->Dump(fp);
	l = l->Next();
	if (l) fprintf(fp, ",");
    }
    fprintf(fp, "\t}");
}

void
 clSequenceEntry::Dump(FILE *fp)
{
    if (Index()) fprintf(fp, "%-20s ", NameStore->Name(Index()));
    fprintf(fp, "SEQUENCE {");
    clSymtabEntry *n = list;
    while (n)
    {
        fprintf(fp, "\n\t");
	n->Dump(fp);
	n = n->Next();
        if (n) fprintf(fp, ",");
    }
    fprintf(fp, "\n\t}");
}

void
 clSequenceOfEntry::Dump(FILE *fp)
{
    if (Index()) fprintf(fp, "%-20s ", NameStore->Name(Index()));
    fprintf(fp, "SEQUENCE OF %s", NameStore->Name(type->Index()));
}

void
 clIndexEltEntry::Dump(FILE *fp)
{
    fprintf(fp, "%s", NameStore->Name(Index()));
}

static char *accessnames[] = 
{
    "read-only",
    "read-write",
    "write-only",
    "not-accessible"
};

static char *statusnames[] = 
{
    "mandatory",
    "optional",
    "deprecated",
    "obsolete"
};

void 
clObjectTypEntry::Dump(FILE *fp)
{
    fprintf(fp, "OBJECT-TYP %s\n", NameStore->Name(Index()));
    fprintf(fp, "\tSYNTAX ");
    if (type->Index())
	fprintf(fp, NameStore->Name(type->Index()));
    else type->Dump(fp);
    fprintf(fp, "\n\tACCESS %s\n", accessnames[access]);
    fprintf(fp, "\tSTATUS %s\n", statusnames[status]);
    if (desc)
        fprintf(fp, "\tDESCRIPTION \"%s\"\n", desc);
    if (ref)
        fprintf(fp, "\tREFERENCE \"%s\"\n", ref);
    if (index)
    {
        fprintf(fp, "\tINDEX {");
	clSymtabEntry *n = index;
	while (n)
	{
	    fprintf(fp, "\n\t\t%s", NameStore->Name(n->Index()));
	    n = n->Next();
	    if (n) fprintf(fp, ",");
	}
        fprintf(fp, "\n\t}\n");
    }
    clObjIDEntry *oi = (clObjIDEntry *)objid;
    fprintf(fp, "\t::= {%s %d}", NameStore->Name(oi->Parent()->Index()),
		oi->Value());
}

void 
clSymtab::Dump(FILE *fp)
{
    for (clSymtabEntry *n = first; n; n = n->next)
    {
	n->Dump(fp);
	fprintf(fp, "\n");
    }
}

#endif

//--------------------------------------------------------------------------

// Recursive routine to write a full object ID

void
clObjIDEntry::WriteObjID(FILE *fp)
{
    if (Parent())
    {
	Parent()->WriteObjID(fp);
	fprintf(fp, ".");
    }
    fprintf(fp, "%d", Value());
}

char *
clTaggedTypeEntry::ClassName()
{
    switch(clas)
    {
    case CLS_PRIVATE:
	return "PRIVATE";
    case CLS_UNIVERSAL:
	return "UNIVERSAL";
    case CLS_EXPERIMENTAL:
	return "EXPERIMENTAL";
    case CLS_APPLICATION:
	return "APPLICATION";
    default:
	return "UNKNOWN";
    }
}

void 
clObjectTypEntry::SetDesc(char *desc_in) 
{
    delete [] desc;
    if (desc_in)
    {
	desc = new char[strlen(desc_in)+1];
	if (desc) strcpy(desc, desc_in);
    }
    else desc = 0;
}

void 
clObjectTypEntry::SetRef(char *ref_in) 
{
    delete [] ref;
    if (ref_in)
    {
	ref = new char[strlen(ref_in)+1];
	if (ref) strcpy(ref, ref_in);
    }
    else ref = 0;
}

void
clObjectTypEntry::WriteObjID(FILE *fp)
{
    ObjID()->WriteObjID(fp);
}

void
clObjectTypEntry::DBWrite(FILE *fp)
{
    // write name field
    fprintf(fp, "%s:", NameStore->Name(Index()));
    // write object id field
    WriteObjID(fp);
    // write type field
    // write accessibility and status fields
    char *acc = 0, *sta = 0;
    switch (Access())
    {
    case ACC_READONLY:	acc = "r";	break;
    case ACC_READWRITE:	acc = "a";	break;
    case ACC_WRITEONLY:	acc = "w";	break;
    case ACC_UNACCESS:	acc = "n";	break;
    }
    switch(Status())
    {
    case STS_MANDATORY:	sta = "m";	break;
    case STS_OPTIONAL:	sta = "o";	break;
    case STS_DEPRECATED:sta = "d";	break;
    case STS_OBSOLETE:	sta = "x";	break;
    }
    fprintf(fp, ":%s:%s:%s:\\\n\"\t%s\"\n", TypeID(), acc, sta, desc);
}

//----------------------------------------------------------------------

clSymtabEntry*
 clSymtab::IsDefined(int index_in)
{
    for (clSymtabEntry *e = first ; e ; e = e->next)
	{
	    if (e->index == index_in)
		return e;
	}
	return 0;
}

clSymtabEntry *
clSymtab::Define(clSymtabEntry *e)
{
    assert(e);
    if (first == 0) first = e;
    else last->next = e;
    last = e;
    e->next = 0;
    return e;
}

clSymtabEntry *
clSymtab::Define(enType typeclass_in, int index_in,
				clSymtabEntry *type_in)
{
    if (index_in==0 || !IsDefined(index_in))
    {
	clSymtabEntry *e =
	    new clSymtabEntry(typeclass_in, index_in, type_in);
	if (first == 0) first = e;
	else last->next = e;
	last = e;
	e->next = 0;
	return e;
    }
    ErrorHandler->Error(ERR_REDEFINE, NameStore->Name(index_in));
    return 0;
}

clSymtabEntry *
clSymtab::Redefine(clSymtabEntry *olde, clSymtabEntry *newe)
{
    clPatchRef *ref = 0;
    if (olde != 0)
    {
    	if (olde == first)
    	{
	    first = first->Next();
	    if (last == olde) last = 0;
    	}
    	else
    	{
            clSymtabEntry *n = first;
            while (n && n->Next() != olde)
	        n = n->Next();
            assert(n && n->Next());
	    if (last == olde) last = n;
	    n->SetNext(olde->Next());
	}
	ref = ((clUndefinedEntry*)olde)->GetRef();
	delete olde;
    }
    clSymtabEntry *rtn = Define(newe);
    while (ref)
    {
	ref->Ref()->SetType(rtn);
	ref = ref->Next();
    }
    return rtn;
}

clSymtabEntry *
clSymtab::Find(int index_in)
{
    clSymtabEntry *rtn = IsDefined(index_in);
    if (rtn == 0)
    {
//        ErrorHandler->Error(ERR_UNDEFINED, 0, NameStore->Name(index_in));
        rtn = Define(new clUndefinedEntry(index_in)); // so we don't report it undefined again
    }
    return rtn;
}

void 
clSymtab::MarkImport(int idx)
{
    clSymtabEntry *e = Find(idx);
    if (e == 0 || !e->IsExported())
        ErrorHandler->Error(ERR_IMPORT, NameStore->Name(idx));
    else
    {
        while (e)
	{
	    e->MarkImported();
	    e = e->type;
	}
    }
}

void 
clSymtab::MergeImports(clSymtab *stab)
{
    if (first && first->IsImported())
    {
        stab->last->next = first;
        while (first && first->IsImported())
	{
	    stab->last = first;
	    first = first->next;
	}
        stab->last->next = 0;
    }
    clSymtabEntry *e = first;
    while (e)
    {
        clSymtabEntry *tmp = e;
	e = e->next;
	while (e && e->IsImported())
	{
	    tmp->next = e->next;
	    stab->last->next = e;
	    stab->last = e;
	    e->next = 0;
	    e = tmp->next;
	}
    }
}

clSymtabEntry *
clSymtab::FindStringType(clSymtabEntry *typ, int minlen, int maxlen)
{
    clSymtabEntry *n = first;
    while (n)
    {
	if (n->Index() == 0 && n->TypeClass()==TYP_OCTETSTRING &&
		typ == n->Type())
	{
	    clStringEntry *e = (clStringEntry *)n;
	    if (e->MinLen() == minlen && e->MaxLen() == maxlen)
		return n;
	}
	n = n->Next();
    }
    return n;
}

clSymtabEntry *
clSymtab::FindIntegerType(clSymtabEntry *typ, long minval, long maxval)
{
    clSymtabEntry *n = first;
    while (n)
    {
	if (n->Index() == 0 && n->TypeClass()==TYP_INTEGER &&
		typ == n->Type())
	{
	    clIntegerEntry *e = (clIntegerEntry *)n;
	    if (e->MinVal() == minval && e->MaxVal() == maxval)
		return n;
	}
	n = n->Next();
    }
    return n;
}

void
clSymtab::BuildIDTable() // this should really be file driven...
{
    static int first = 1;
    if (first == 0) return;
    first = 0;
    lookuptbl[0].id = "n";	lookuptbl[0].idx = NameStore->Lookup("NULL");
    lookuptbl[1].id = "o";	lookuptbl[1].idx = NameStore->Lookup("OCTETSTRING");
    lookuptbl[2].id = "i";	lookuptbl[2].idx = NameStore->Lookup("INTEGER");
    lookuptbl[3].id = "oi";	lookuptbl[3].idx = NameStore->Lookup("OBJECTID");
    lookuptbl[4].id = "na";	lookuptbl[4].idx = NameStore->Lookup("NetworkAddress");
    lookuptbl[5].id = "ip";	lookuptbl[5].idx = NameStore->Lookup("IpAddress");
    lookuptbl[6].id = "tt";	lookuptbl[6].idx = NameStore->Lookup("TimeTicks");
    lookuptbl[7].id = "c32";	lookuptbl[7].idx = NameStore->Lookup("Counter");
    lookuptbl[8].id = "g32";	lookuptbl[8].idx = NameStore->Lookup("Gauge");
    lookuptbl[9].id = "oq";	lookuptbl[9].idx = NameStore->Lookup("Opaque");
    lookuptbl[10].id = 0;
}

void 
clSymtab::WriteMIB(FILE *fp)
{
    clSymtabEntry *n = first;
    BuildIDTable();
    while (n)
    {
	n->DBWrite(fp);
	n = n->Next();
    }
}

clSymtab::~clSymtab()
{
    clSymtabEntry *e = first;
    while (e)
    {
        clSymtabEntry *tmp = e;
	e = e->next;
	if (!tmp->IsMarked()) delete tmp;
    }
}

