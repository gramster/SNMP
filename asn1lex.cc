//-----------------------------------------------------------------------
// asn1lex.cc - lexical analyser for ASN.1
//
// Written by Graham Wheeler, August 1995.
// Based on a Pascal- lexer written for UCT PLT Course, 1994.
//
// (c) 1995, Open Mind Solutions (cc)
// All Rights Reserved
//
// Copying or using this software or derived works without
// the written permission of Open Mind Solutions is strictly
// prohibited.
//
//-----------------------------------------------------------------------

#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "asn1err.h"
#include "asn1name.h"
#include "asn1lex.h"

#define MAXINT		(0x7FFFFFFFl)

/*
 * Reserved word lookup
 */

typedef struct
{
    char		*name;
    enToken		token;
} stKeywordEntry;

#define TOKEN(t)	t

static stKeywordEntry keyword_table[] =
{
    { "ACCESS",			SYM_ACCESS	},
    { "AGENT-CAPABILITIES",	SYM_CAPABILITIES},
//    { "ANY",			SYM_ANY		},
    { "APPLICATION",		SYM_APPLICATION	},
    { "AUGMENTS",		SYM_AUGMENTS	},
    { "BEGIN",			SYM_BEGIN	},
//    { "BIT",			SYM_BIT		},
//    { "BITSTRING",		SYM_BITSTRING	},
    { "CHOICE",			SYM_CHOICE	},
    { "COMPONENTS",		SYM_COMPONENTS	},
    { "CONTACT-INFO",		SYM_CONTACTINFO	},
//    { "Counter", 		SYM_Counter	},
    { "Counter32",		SYM_Counter32	},
    { "Counter64",		SYM_Counter64	},
    { "DEFAULT",		SYM_DEFAULT	},
    { "DEFINITIONS",		SYM_DEFINITIONS	},
    { "DEFVAL",			SYM_DEFVAL	},
    { "DESCRIPTION",		SYM_DESCRIPTION	},
    { "DISPLAY-HINT",		SYM_DISPLAYHINT	},
    { "END",			SYM_END		},
//    { "ENUMERATED",			SYM_ENNUMERATED	},
    { "EXPLICIT",		SYM_EXPLICIT	},
    { "EXPORTS",		SYM_EXPORTS	},
    { "FROM", 			SYM_FROM	},
//    { "Gauge",			SYM_Gauge	},
    { "Gauge32",		SYM_Gauge32	},
    { "IDENTIFIER",		SYM_IDENTIFIER	},
    { "IMPLICIT",		SYM_IMPLICIT	},
    { "IMPORTS",		SYM_IMPORTS	},
    { "INDEX",			SYM_INDEX	},
    { "INTEGER",		SYM_INTEGER	},
    { "Integer32",		SYM_Integer32	},
//    { "IpAddress",		SYM_IpAddress	},
    { "LAST-UPDATED",		SYM_LASTUPDATED	},
    { "MACRO",			SYM_MACRO	},
    { "MAX-ACCESS",		SYM_MAXACCESS	},
    { "MODULE-COMPLIANCE",	SYM_MODCOMPLY	},
    { "MODULE-IDENTITY",	SYM_MODIDENT	},
    { "NOACCESS",		SYM_NOACCESS	},
    { "NOTATION",		SYM_NOTATION	},
    { "NOTIFICATION-TYPE",	SYM_NOTIFTYP	},
    { "NULL",			SYM_NULL	},
    { "NUM-ENTRIES",		SYM_NUMENTRIES	},
//    { "NetworkAddress",		SYM_NetAddress	},
//    { "NsapAddress",		SYM_NsapAddress	},
    { "OBJECT",			SYM_OBJECT	},
    { "OBJECTIDENTIFIER",	SYM_OBJECTID	},
    { "OBJECTS",		SYM_OBJECTS	},
    { "OBJECT-GROUP",		SYM_OBJECTGRP	},
    { "OBJECT-IDENTITY",	SYM_OBJECTIDENT	},
    { "OBJECT-TYPE",		SYM_OBJECTTYP	},
    { "OCTET",			SYM_OCTET	},
    { "OCTETSTRING",		SYM_OCTETSTRING	},
    { "OF",			SYM_OF		},
//    { "Opaque",			SYM_Opaque	},
    { "ORGANIZATION",		SYM_ORGANIZATION},
    { "PRIVATE",		SYM_PRIVATE	},
//    { "REAL",			SYM_REAL	},
    { "REFERENCE",		SYM_REFERENCE	},
    { "SEQUENCE",		SYM_SEQUENCE	},
//    { "SET",			SYM_SET		},
    { "SIZE",			SYM_SIZE	},
    { "STATUS",			SYM_STATUS	},
    { "STRING",			SYM_STRING	},
    { "SYNTAX",			SYM_SYNTAX	},
    { "TEXTUAL-CONVENTION",	SYM_TEXTCONV	},
    { "TRAP-TYPE",		SYM_TRAPTYP	},
    { "TYPE",			SYM_TYPE	},
//    { "TimeTicks",		SYM_TimeTicks	},
    { "UInteger32",		SYM_UInteger32	},
    { "UNITS",			SYM_UNITS	},
    { "UNIVERSAL",		SYM_UNIVERSAL	},
    { "VALUE",			SYM_VALUE	},
    { "current",		SYM_CURRENT	},
    { "deprecated",		SYM_DEPRECATED	},
    { "mandatory",		SYM_MANDATORY	},
    { "not-accessible",		SYM_UNACCESS	},
    { "obsolete",		SYM_OBSOLETE	},
    { "optional",		SYM_OPTIONAL	},
    { "read-create",		SYM_READCREATE	},
    { "read-only",		SYM_READONLY	},
    { "read-write",		SYM_READWRITE	},
    { "recommended",		SYM_RECOMMENDED	},
    { "write-only",		SYM_WRITEONLY	}
};
     
//-------------------------------------------------------------------
// Search the reserved word table with a binary search

#define END(v)	(v-1+sizeof(v) / sizeof(v[0]))

enToken 
clTokeniser::LookupName()
{
    stKeywordEntry *low = keyword_table,
		    *high = END(keyword_table),
		    *mid;
    int c;
    while (low <= high)
    {
	mid = low + (high-low)/2;
	if ((c = strcmp(mid->name, token_name)) == 0)
	    return mid->token;
	else if (c < 0)
	    low = mid + 1;
	else
	    high = mid - 1;
    }
    // It isn't a reserved word, so we enter it into name store

    token_value = (int)(NameStore->Lookup(token_name));
    return (TOKEN(SYM_IDENT));
}

//-------------------------------------------------------------------
// Get current location for error messages

char *
clTokeniser::Place()
{
    static char place[128];
    if (token_name[0]>=' ' && token_name[0]<='z')
    	sprintf(place, "line %d near <%s>", line_number, token_name);
    else
    	sprintf(place, "line %d", line_number);
    return place;
}

//-------------------------------------------------------------------

#define EOF	(-1)

void 
clTokeniser::NextChar()
{
    if (!feof(source_fp))
	ch = fgetc(source_fp);
    else
	ch = EOF;
    if (ch == '\n') line_number++;
}

void 
clTokeniser::StripComment()
{
    NextChar();
  skipmore:
    while (ch != '\n' && ch != '-' && ch != EOF)
	NextChar();
    if (ch == '-')
    {
        NextChar();
	if (ch != '\n' && ch != '-' && ch != EOF)
	    goto skipmore;
    }
    if (ch == EOF)
	Fatal(ERR_COMMENT);
    else NextChar();
}

void 
clTokeniser::ScanInteger(int isminus)
{
    unsigned long v = 0l;
    while (isdigit(ch))
    {
	int dval = ch - '0';
	if (v > (MAXINT - (unsigned long)dval)/10l)
	    Warning(ERR_NUMSIZE);
	v *= 10l;
	v += (unsigned long)dval;
	NextChar();
    }
    if (isminus) token_value = -v;
    else token_value = v;
}

void 
clTokeniser::ScanString()
{
    char *cptr = token_name;
    int len = MAX_TOKEN_LENGTH;
    NextChar();
    while (ch != '"')
    {
	// We insert backslashes before newlines and strip leading whitespace
	// on each line, replacing it with a tab. This assumes that the only
	// such multi-line strings will be object typ descriptions
	if (len>0)
	if (ch == '\n' && len>3)
	{
	    *cptr++ = '\\';
	    *cptr++ = '\n';
	    *cptr++ = '\t';
	    len--;
	    NextChar();
	    while (ch == ' ' || ch == '\t') NextChar();
	}
	else
	{
	    *cptr++ = ch;
	    NextChar();
	}
	if (len-- == 0)
	{
	    *cptr = '\0';
	    Error(ERR_TOKENLEN, token_name);
	}
    }
    *cptr = '\0';
}

void 
clTokeniser::ScanName()
{
    char *cptr = token_name;
    int len = MAX_TOKEN_LENGTH;
    while (isalnum(ch) || ch=='_' || ch=='-')
    {
	if (len>0) *cptr++ = ch;
	NextChar();
	if (len-- == 0)
	{
	    *cptr = '\0';
	    Error(ERR_TOKENLEN, token_name);
	}
    }
    *cptr = '\0';
}

enToken
 clTokeniser::NextToken()
{
    token_type = SYM_UNDEFINED;
    do
    {
	int isminus = 0;
	switch (ch)
	{
	case '-':
	    NextChar();
	    if (ch == '-')
	        StripComment();
	    else if (isdigit(ch))
	    {
		isminus = 1;
		goto getnumber;
	    }
	    else Error(ERR_NOCOMMENT);
	    continue;
	case EOF:
	    return token_type = SYM_EOF;
	case ' ':
	case '\n':
	case '\t':
	    break;
	case '.':
	    NextChar();
	    if (ch == '.')
	        token_type = SYM_DOTDOT;
	    else
	        Error(ERR_EXPECT, "..");
	    break;
	case '(':
	    token_type = SYM_LEFTPAREN;
	    break;
	case ')':
	    token_type = SYM_RIGHTPAREN;
	    break;
	case '{':
	    token_type = SYM_LEFTBRACE;
	    break;
	case '}':
	    token_type = SYM_RIGHTBRACE;
	    break;
	case '[':
	    token_type = SYM_LEFTBRACKET;
	    break;
	case ']':
	    token_type = SYM_RIGHTBRACKET;
	    break;
	case ',':
	    token_type = SYM_COMMA;
	    break;
	case ';':
	    token_type = SYM_SEMICOLON;
	    break;
	case '|':
	    token_type = SYM_PIPE;
	    break;
	case ':':
	    NextChar();
	    if (ch == ':')
	    {
	        NextChar();
	        if (ch == '=')
	    	    token_type = SYM_ASSIGN;
	    }
	    if (token_type == SYM_UNDEFINED)
	        Error(ERR_EXPECT, "::=");
	    break;
	case '"':
	    ScanString();
	    token_type = SYM_STRLITERAL;
	    break;
	default:
	getnumber:
	    if (isdigit(ch))
	    {
	    	ScanInteger(isminus);
	    	return token_type = SYM_INTLITERAL;
	    }
	    else if (isalpha(ch) || ch=='_')
	    {
	    	ScanName();
	    	return token_type = LookupName();
	    }
	    else
	    {
	    	token_name[0] = ch;
	    	token_name[1] = '\0';
	    	Error(ERR_BADCHAR, token_name);
	    }
	    break;
	}
	NextChar();
    }
    while (token_type == SYM_UNDEFINED);
    return token_type;
}

void
 clTokeniser::Expect(enToken tok)
{
    if (token_type != tok)
    {
        Error(ERR_EXPECT, TokenID(tok));
        while (token_type != tok && token_type != SYM_EOF)
	    NextToken();
    }
    NextToken();
}

clTokeniser::clTokeniser(char *source_file_name)
{
    source_fp = fopen(source_file_name, "r");
    token_name[0] = 0;
    line_number = 1;
    if (source_fp == NULL)
    	ErrorHandler->Fatal(ERR_NOSOURCE, source_file_name);
    NextChar();
}

static char *tokenIDs[] =
{
    "ACCESS",
    "AGENT-CAPABILITIES",
    "APPLICATION",
    "AUGMENTS",
    "BEGIN",
    "CHOICE",
    "COMPONENTS",
    "CONTACT-INFO",
//    "Counter",
    "Counter32",
    "Counter64",
    "DEFAULT",
    "DEFINITIONS",
    "DEFVAL",
    "DESCRIPTION",
    "DISPLAY-HINT",
    "END",
//    "ENUMERATED",
    "EXPORTS",
    "EXPLICIT",
    "FROM",
//    "Gauge",
    "Gauge32",
    "IDENTIFIER",
    "IMPLICIT",
    "IMPORTS",
    "INDEX",
    "INTEGER",
    "Integer32",
//    "IpAddress",
    "LAST-UPDATED",
    "MACRO",
    "MAX-ACCESS",
    "MODULE-COMPLIANCE",
    "MODULE-IDENTITY",
    "NOACCESS",
    "NOTATION",
    "NOTIFICATION-TYPE",
    "NULL",
    "NUM-ENTRIES",
//    "NetworkAddress",
//    "NsapAddress",
    "OBJECT",
    "OBJECTIDENTIFIER",
    "OBJECTS",
    "OBJECT-GROUP",
    "OBJECT-IDENTITY",
    "OBJECT-TYPE",
    "OCTET",
    "OCTETSTRING",
    "OF",
//    "Opaque",
    "ORGANIZATION",
    "PRIVATE",
    "REFERENCE",
//    "REAL",
    "SEQUENCE",
//    "SET",
    "SIZE",
    "STATUS",
    "STRING",
    "SYNTAX",
    "TEXTUAL-CONVENTION",
    "TRAP-TYPE",
    "TYPE",
//    "TimeTicks",
    "UInteger32",
    "UNITS",
    "UNIVERSAL",
    "VALUE",
    "current",
    "deprecated",
    "mandatory",
    "not-accessible",
    "obsolete",
    "optional",
    "read-create",
    "read-only",
    "read-write",
    "recommended",
    "write-only",
    
    "Identifier",
    "Integer",
    "String",

    "::=",
    "{",
    "}",
    "(",
    ")",
    "[",
    "]",
    "|",
    ",",
    ";",
    "..",
    "-",

    "EOF",
    "UNDEFINED"
};  

const char *
clTokeniser::TokenID(enToken t)
{
    assert(t<=SYM_UNDEFINED);
    return tokenIDs[(int)t];
}

