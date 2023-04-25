//------------------------------------------------------------
// asn1pars.cc - A compiler for a subset of ASN.1 for SNMP
//
// Written by Graham Wheeler, August 1995
// (c) 1995, Open Mind Solutions (cc)
// All Rights Reserved
//
// Copying or using this software or derived works without
// the written permission of Open Mind Solutions is strictly
// prohibited.
//
// This parser supports a subset of ASN.1 suitable for SNMP MIBs.
// Thus, it supports the following primitive types:
//
//	NULL
//	INTEGER
//	OCTET STRING
//	OBJECT IDENTIFIER
//
// and the following constructed types:
//
//	SEQUENCE
//	SEQUENCE OF
//
// Macros, CHOICEs, etc, are not supported - the OBJECT TYPE macro
// and the types defined in the SMI are hard-coded. This is is
// accordance with the SMI, which states that the OBJECT-TYP macro
// is an implementation-time issue.
//
// Value definitions cannot implicitly define new types but must
// refer to an existing type name.
//
// The following subtyping rules can be used:
//
//	IMPLICIT INTEGER (min..max)
//	IMPLICIT OCTET STRING (SIZE (len))
//	IMPLICIT OCTET STRING (SIZE (min..max))
//
// Before compiling a MIB, remove any IMPORT statement that
// references the SMI definitions, as the parser will barf
// if it attempts to parse the SMI itself - it only works 
// with MIBs.
//------------------------------------------------------------

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "smi.h"
#include "asn1err.h"
#include "asn1name.h"
#include "asn1lex.h"
#include "asn1stab.h"
#include "asn1pars.h"

void 
clParser::InitPredefs()
{
    clSymtabEntry *integer, *string, *noname, *objid, *ipaddr;
    clObjIDEntry *oi;
    // predefined ASN.1 types

    integer = stab->Define(new clSymtabEntry(TYP_INTEGER, "INTEGER"));
    stab->Define(new clSymtabEntry(TYP_NULL, "NULL"));
    // the names of the next two are not used, as they are
    // really multi-word and thus handled by parser explicitly.
    // Should really use garbage for names in case MIB defines them.
    string = stab->Define(new clSymtabEntry(TYP_OCTETSTRING, "OCTETSTRING"));
    objid = stab->Define(new clSymtabEntry(TYP_OBJECTID, "OBJECTID"));

    // SMI types from RFC1155. These could all be compiled rather
    // than hard-coded; this may be a good idea later.
   
	// is ObjectName needed?
    //stab->Define(new clNamedTypeEntry("ObjectName", objid));

    noname = stab->Define(new clStringEntry(0, string, 4, 4));
    ipaddr = stab->Define(new clImplicitTagEntry("IpAddress",  CLS_APPLICATION, 0, noname));
    // What about NetworkAddress??
    stab->Define(new clNamedTypeEntry("NetworkAddress", ipaddr));
    (void)stab->Define(new clImplicitTagEntry("Opaque", CLS_APPLICATION, 4, string));
    noname = stab->Define(new clIntegerEntry(0, integer, 0l, 0xFFFFFFFFl));
    stab->Define(new clImplicitTagEntry("Counter", CLS_APPLICATION, 1, noname));
    stab->Define(new clImplicitTagEntry("Gauge", CLS_APPLICATION, 2, noname));
    stab->Define(new clImplicitTagEntry("TimeTicks", CLS_APPLICATION, 3, noname));

    // predefined object IDs

    oi = new clObjIDEntry(0, "iso", 1);		stab->Define(oi);
    oi = new clObjIDEntry(oi, "org", 3);	stab->Define(oi);
    oi = new clObjIDEntry(oi, "dod", 6);	stab->Define(oi);
    oi = new clObjIDEntry(oi, "internet", 1);	stab->Define(oi);
    stab->Define(new clObjIDEntry(oi, "directory", 1));
    stab->Define(new clObjIDEntry(oi, "mgmt", 2));
    stab->Define(new clObjIDEntry(oi, "experimental", 3));
    oi = new clObjIDEntry(oi, "private", 4);	stab->Define(oi);
    stab->Define(new clObjIDEntry(oi, "enterprises", 1));
}

void 
clParser::Parse()
{
    InitPredefs();
    NextToken();
    ModuleDefinition();
    // check for trailing garbage
    if (Token() != SYM_EOF)
        Error(ERR_EXPECT, "EOF");
    // mark exportable objects for merging if imported from another module
    MarkExports();
//    stab->Dump(stdout);
}

void
clParser::ModuleDefinition()
{
    stab->Define(TYP_MODULE, TokenVal());
    Expect(SYM_IDENT);
    Expect(SYM_DEFINITIONS);
    Expect(SYM_ASSIGN);
    Expect(SYM_BEGIN);
    for (;;)
    {
        if (Token() == SYM_IMPORTS)
	{
	    NextToken();
	    Imports();
	}
        else if (Token() == SYM_EXPORTS)
	    Exports();
    	else break;
    }
    if (Token() != SYM_END && Token() != SYM_EOF)
	ModuleBody();
    Expect(SYM_END);
}

void 
clParser::MarkExports()
{
    for (int i = 0; i < numexp; i++)
    {
	clSymtabEntry *e = stab->Find(expidx[i]);
	if (e == 0)
	    Error(ERR_EXPORT, NameStore->Name(expidx[i]));
	else e->Export();
    }
}

void 
clParser::Import()
{
    int impidx[MAX_IMPORTS], numimp = 0;
    for (;;)
    {
	if (Token() == SYM_IDENT)
	{
	    impidx[numimp++] = TokenVal();
	    assert(numimp<=MAX_IMPORTS);
            NextToken();
	}
	// because we hard code the OBJECT-TYPE macro
	// we check it as a special case of identifier
	else if (Token() == SYM_OBJECTTYP)
            NextToken();
	else break;
	if (Token() == SYM_COMMA)
	     NextToken();
	else break;
    }
    Expect(SYM_FROM);
    if (Token() == SYM_IDENT)
    {
        // In the current implementation, only one global
	// namestore is used. Ideally each parser/lexer should
	// have their own namestore, but this makes merging
	// imports much more complex. The disadvantage of this
	// approach is that non-imported names get stored in
	// the namestore as well.
	if (strncmp(TokenName(), "RFC1155", 7)!=0 &&
	    strncmp(TokenName(), "RFC1212", 7)!=0) // not a SMI RFC?
	{
	    clSymtab *substab = new clSymtab();
	    clParser *subprs = 
	        new clParser(new clTokeniser((char*)TokenName()), substab);
	    subprs->Parse();
	    for (int i = 0; i < numimp; i++)
	        substab->MarkImport(impidx[i]);
	    stab->MergeImports(substab);
	    delete subprs;
	}
	numimp = 0;
    }
    Expect(SYM_IDENT);
}

void
clParser::Imports()
{
    for (;;)
    {
        Import();
	if (Token() == SYM_SEMICOLON || Token() == SYM_EOF)
	    break;
    }
    Expect(SYM_SEMICOLON);
}

void
clParser::Exports()
{
    Expect(SYM_EXPORTS);
    for (;;)
    {
	if (Token() == SYM_IDENT)
	{
	    expidx[numexp++] = TokenVal();
	    assert(numexp<=MAX_EXPORTS);
            NextToken();
	}
	// because we hard code the OBJECT-TYPE macro
	// we check it as a special case of identifier
	else if (Token() == SYM_OBJECTTYP)
            NextToken();
	else break;
	if (Token() == SYM_COMMA)
	    NextToken();
	else break;
    }
    Expect(SYM_SEMICOLON);
}

void
clParser::ModuleBody()
{
    while (Token() != SYM_END && Token() != SYM_EOF)
        Definition();
}

void 
clParser::Definition()
{
    int idx = TokenVal();
    clSymtabEntry *e = stab->IsDefined(idx);
    if (e && e->TypeClass() != TYP_UNDEFINED)
	Error(ERR_REDEFINE, TokenName());
    Expect(SYM_IDENT);
    if (Token() == SYM_ASSIGN) // type definition
    {
        NextToken();
	clSymtabEntry *t = Type();
	if (t->Index() == 0)	// named; create an alias
	{
	    t->SetIndex(idx);
	    if (e) stab->Redefine(e, t);
	}
	else stab->Redefine(e, new clNamedTypeEntry(idx, t));
    }
    else if (Token() == SYM_OBJECTTYP) // object type macro instance
        (stab->Redefine(e, ObjectType()))->SetIndex(idx);
    else // value definition
    {
        clSymtabEntry *t = Type();
        Expect(SYM_ASSIGN);
        clSymtabEntry *v = Value(t);
	v->SetIndex(idx);
	stab->Redefine(e, v);
    }
}

clSymtabEntry *
clParser::TypeReference()
{
    clSymtabEntry *e = stab->Find(TokenVal());
    if (e == 0)
    {
	Error(ERR_NOTYPE, NameStore->Name(TokenVal()));
	e = stab->Define(new clUndefinedEntry(TokenVal()));
    }
    Expect(SYM_IDENT);
    if (Token() == SYM_LEFTPAREN) // subtype
    {
	NextToken();
        if (Token() == SYM_SIZE) // string subtype
	{
	    NextToken();
	    Expect(SYM_LEFTPAREN);
	    long minlen, maxlen;
	    minlen = maxlen = TokenVal();
	    Expect(SYM_INTLITERAL);
	    if (Token() == SYM_DOTDOT)
	    {
		NextToken();
		maxlen = TokenVal();
	        Expect(SYM_INTLITERAL);
	    }
	    Expect(SYM_RIGHTPAREN);
	    clSymtabEntry *e2 = stab->FindStringType(e, minlen, maxlen);
	    if (e2) e = e2;
	    else e = stab->Define(new clStringEntry(0, e, minlen, maxlen));
	}
	else
	{
	    long minval = TokenVal();
	    Expect(SYM_INTLITERAL);
	    Expect(SYM_DOTDOT);
	    long maxval = TokenVal();
	    Expect(SYM_INTLITERAL);
	    clSymtabEntry *e2 = stab->FindIntegerType(e, minval, maxval);
	    if (e2) e = e2;
	    else e = stab->Define(new clIntegerEntry(0, e, minval, maxval));
	}
	Expect(SYM_RIGHTPAREN);
    }
    return e;
}

clSymtabEntry *
clParser::Type()
{
    clSymtabEntry *e = 0;
    switch (Token())
    {
    case SYM_INTEGER:
        NextToken();
	e = stab->Find(NameStore->Lookup("INTEGER"));
	if (Token() == SYM_LEFTPAREN)
	{
	    NextToken();
	    long minval = TokenVal();
	    Expect(SYM_INTLITERAL);
	    Expect(SYM_DOTDOT);
	    long maxval = TokenVal();
	    Expect(SYM_INTLITERAL);
	    Expect(SYM_RIGHTPAREN);
	    e = stab->Define(new clIntegerEntry(0, e, minval, maxval));
	}
	else if (Token() == SYM_LEFTBRACE) // enum
	{
	    clSymtabEntry *vlist = 0, *n = 0;
	    NextToken();
	    for (;;)
	    {
		int idx = TokenVal();
		Expect(SYM_IDENT);
		Expect(SYM_LEFTPAREN);
		if (n)
		{
		    n->SetNext(new clEnumEltEntry(idx, e, TokenVal()));
		    n = n->Next();
		}
		else
		    vlist = n = new clEnumEltEntry(idx, e, TokenVal());
		Expect(SYM_INTLITERAL);
		Expect(SYM_RIGHTPAREN);
		if (Token() == SYM_COMMA) NextToken();
		else break;
	    }
	    Expect(SYM_RIGHTBRACE);
	    e = stab->Define(new clEnumEntry(0, e, vlist));
	}
	break;
    case SYM_OCTET:
        NextToken();
	Expect(SYM_STRING);
	e = stab->Find(NameStore->Lookup("OCTETSTRING"));
	if (Token() == SYM_LEFTPAREN)
	{
	    NextToken();
	    Expect(SYM_SIZE);
	    Expect(SYM_LEFTPAREN);
	    long minlen, maxlen;
	    minlen = maxlen = TokenVal();
	    Expect(SYM_INTLITERAL);
	    if (Token() == SYM_DOTDOT)
	    {
		NextToken();
		maxlen = TokenVal();
	        Expect(SYM_INTLITERAL);
	    }
	    Expect(SYM_RIGHTPAREN);
	    e = stab->Define(new clStringEntry(0, e, minlen, maxlen));
	    Expect(SYM_RIGHTPAREN);
	}
	break;
    case SYM_NULL:
        NextToken();
	e = stab->Find(NameStore->Lookup("NULL"));
	break;
    case SYM_OBJECT: // object ID
        NextToken();
        Expect(SYM_IDENTIFIER);
	e = stab->Find(NameStore->Lookup("OBJECTID"));
	break;
    case SYM_IDENT: // type reference
	e = TypeReference();
	break;
    case SYM_LEFTBRACKET: // tagged type
    {
        long val;
	enClass cls;
	cls = Tag(val);
        if (Token() == SYM_IMPLICIT)
	{
            NextToken();
	    e = stab->Define(new clImplicitTagEntry(0, cls, val, Type()));
	}
	else e = stab->Define(new clTaggedEntry(0, cls, val, Type()));
	break;
    }
    case SYM_SEQUENCE:
        NextToken();
        if (Token() == SYM_LEFTBRACE)
	{
            NextToken();
	    clSymtabEntry *el = (Token() != SYM_RIGHTBRACE) ? 
				        ElementTypeList() : 0;
	    Expect(SYM_RIGHTBRACE);
	    e = stab->Define(new clSequenceEntry(0, el));
	}
	else // sequence of
	{
            Expect(SYM_OF);
	    clSymtabEntry *typ = Type();
	    e = stab->Define(new clSequenceOfEntry(0, typ));
	    if (typ->TypeClass() == TYP_UNDEFINED) // forward ref?
		// for patching after real definition
		((clUndefinedEntry*)typ)->AddRef(e);
	}
	break;
    default:
        ErrorHandler->Error(ERR_EXPECT, "Type");
	break;
    }
    assert(e);
    return e;
}

clSymtabEntry *
clParser::Value(clSymtabEntry *typ) // value or tagged value
{
    clSymtabEntry *v = 0;
    while (typ->TypeClass() == TYP_NAMEDTYPE) // get actual type
	typ = typ->Type();
    if (Token() == SYM_IDENT) // value reference
    {
	v = stab->Find(TokenVal());
        NextToken();
    }
    else switch(typ->TypeClass())
    {
    case TYP_INTEGER:
	v = new clIntegerValueEntry(0, typ, TokenVal());
	Expect(SYM_INTLITERAL);
        break;
    case TYP_OCTETSTRING:
	v = new clStringValueEntry(0, typ, (char *)TokenName());
	Expect(SYM_STRLITERAL);
        break;
    case TYP_NULL: // null value
	v = new clNullValueEntry(0, typ);
	Expect(SYM_NULL);
        break;
    case TYP_SEQUENCE:
	v = ElementValueList(typ);
	break;
    case TYP_SEQUENCEOF:
	v = ValueList(typ);
	break;
    case TYP_OBJECTID:
	v = ObjIDComponentList();
	break;
    default:
	Error(ERR_EXPECT, "Value");
        break;
    }
    return v;
}

clSymtabEntry *
clParser::ElementType()
{
    int idx = TokenVal();
    Expect(SYM_IDENT);
    clSymtabEntry *rtn = new clElementTypeEntry(idx, Type());
    if (Token() == SYM_OPTIONAL || Token() == SYM_DEFAULT)
    {
	Error(ERR_UNSUPPORTED, TokenName());
	NextToken();
    }
    return rtn;
}

clSymtabEntry *
clParser::ElementTypeList()
{
    clSymtabEntry *head = ElementType(), *now = head;
    while (Token() == SYM_COMMA)
    {
        NextToken();
        now->SetNext(ElementType());
	now = now->Next();
    }
    return head;
}

enClass clParser::Tag(long &val)
{
    enClass rtn = CLS_EXPERIMENTAL;
    Expect(SYM_LEFTBRACKET);
    if (Token() == SYM_UNIVERSAL)
    {
        NextToken();
	rtn = CLS_UNIVERSAL;
    }
    else if (Token() == SYM_APPLICATION)
    {
        NextToken();
	rtn = CLS_APPLICATION;
    }
    else if (Token() == SYM_PRIVATE)
    {
        NextToken();
	rtn = CLS_PRIVATE;
    }
    else Error(ERR_EXPECT, "Tag class");
    val = TokenVal();
    Expect(SYM_INTLITERAL);
    Expect(SYM_RIGHTBRACKET);
    return rtn;
}

clSymtabEntry *
clParser::ElementValueList(clSymtabEntry *typ)
{
    clSymtabEntry *head = 0, *now, *tmp;
    clSymtabEntry *typnow = ((clSequenceEntry*)typ)->List();
    Expect(SYM_LEFTBRACE);
    while (Token() != SYM_EOF && Token() != SYM_RIGHTBRACE)
    {
	if (TokenVal() != typnow->Index())
	{
	    Error(ERR_BADELT);
	    break;
	}
	Expect(SYM_IDENT); // element being assigned to
	tmp = Value(typnow->Type());
	if (head == 0) head = tmp;
	else now->SetNext(tmp);
	now = tmp;
	typnow = typnow->Next();
    }
    Expect(SYM_RIGHTBRACE);
    return head;
}

clSymtabEntry *
clParser::ValueList(clSymtabEntry *typ)
{
    clSymtabEntry *head = 0, *now, *tmp;
    Expect(SYM_LEFTBRACE);
    while (Token() != SYM_EOF && Token() != SYM_RIGHTBRACE)
    {
	tmp = Value(typ);
	if (head == 0) head = tmp;
	else now->SetNext(tmp);
	now = tmp;
    }
    Expect(SYM_RIGHTBRACE);
    return head;
}

clObjIDEntry *
clParser::ObjIDComponentList()
{
    // we assume this consists of an identifier identifying an existing
    // objid, followed by an integer defining the new child. This 
    // may have to be extended.

    clSymtabEntry *parent = 0;
    clObjIDEntry *rtn = 0;
    Expect(SYM_LEFTBRACE);
    if (Token() == SYM_IDENT)
    {
	parent = stab->Find(TokenVal());
        Expect(SYM_IDENT);
    }
    else Error(ERR_EXPECT, "Object identifier");
    if (parent && parent->TypeClass() != TYP_OBJECTID &&
    	parent->TypeClass() != TYP_OBJECTTYP)
        Error(ERR_EXPECT, "Object identifier");
    else
        rtn = new clObjIDEntry((clObjIDEntry*)parent, 0, TokenVal());
    Expect(SYM_INTLITERAL);
    Expect(SYM_RIGHTBRACE);
    return rtn;
}

clSymtabEntry *
clParser::IndexType(clSymtabEntry *syntax)
{
    clSymtabEntry *rtn = new clIndexEltEntry(TokenVal(), syntax);
    Expect(SYM_IDENT);
    return rtn;
}

clSymtabEntry *
clParser::IndexTypes(clSymtabEntry *syntax)
{
    clSymtabEntry *rtn = IndexType(syntax), *n = rtn;
    while (Token() == SYM_COMMA)
    {
	NextToken();
    	n->SetNext(IndexType(syntax));
	n = n->Next();
    }
    return rtn;
}

// The OBJECT-TYP macro

clSymtabEntry *
clParser::ObjectType()
{
    char *desc = 0, *ref = 0;
    enAccess access = ACC_UNACCESS;
    clObjIDEntry *objid;
    clSymtabEntry *syntax, *val = 0, *rtn = 0, *index = 0;
    enStatus status = STS_OPTIONAL;
    Expect(SYM_OBJECTTYP);
    Expect(SYM_SYNTAX);
    syntax = Type();
    Expect(SYM_ACCESS);
    switch(Token())
    {
    case SYM_READONLY:
        access = ACC_READONLY;
        break;
    case SYM_READWRITE:
        access = ACC_READWRITE;
        break;
    case SYM_WRITEONLY:
        access = ACC_WRITEONLY;
        break;
    case SYM_UNACCESS:
        access = ACC_UNACCESS;
        break;
    default:
	Error(ERR_EXPECT, "Access class");
    }
    NextToken();
    Expect(SYM_STATUS);
    switch(Token())
    {
    case SYM_MANDATORY:
	status = STS_MANDATORY;
        break;
    case SYM_OPTIONAL:
	status = STS_OPTIONAL;
        break;
    case SYM_DEPRECATED:
	status = STS_DEPRECATED;
        break;
    case SYM_OBSOLETE:
	status = STS_OBSOLETE;
        break;
    default:
	Error(ERR_EXPECT, "Object type status");
    }
    NextToken();
    if (Token() == SYM_DESCRIPTION)
    {
        NextToken();
	desc = new char[strlen(TokenName())+1];
	if (desc) strcpy(desc, TokenName());
        Expect(SYM_STRLITERAL);
    }
    if (Token() == SYM_REFERENCE)
    {
        NextToken();
	ref = new char[strlen(TokenName())+1];
	if (ref) strcpy(desc, TokenName());
        Expect(SYM_STRLITERAL);
    }
    if (Token() == SYM_INDEX)
    {
        NextToken();
        Expect(SYM_LEFTBRACE);
	index = IndexTypes(syntax); // see rfc1212
        Expect(SYM_RIGHTBRACE);
    }
    if (Token() == SYM_DEFVAL)
    {
        NextToken();
        Expect(SYM_LEFTBRACE);
	val = Value(syntax);
        Expect(SYM_RIGHTBRACE);
    }
    Expect(SYM_ASSIGN);
    objid = ObjIDComponentList();
    rtn = new clObjectTypEntry(0, syntax, val, access, status, ref, desc, index, objid);
    if (syntax->TypeClass() == TYP_UNDEFINED)
	((clUndefinedEntry *)syntax)->AddRef(rtn);
    delete [] desc;
    delete [] ref;
    return rtn;
}

