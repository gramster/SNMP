//-----------------------------------------------------------------------
// asn1lex.h - Header file for ASN.1 lexical analyser
//
// Written by Graham Wheeler, August 1995.
// Based on a Pascal - lexer written for UCT PLT Course, 1994
//
// (c) 1995, Open Mind Solutions (cc)
// All Rights Reserved
//
// Copying or using this software or derived works without
// the written permission of Open Mind Solutions is strictly
// prohibited.
//
//-----------------------------------------------------------------------

#ifndef ASN1LEX_H
#define ASN1LEX_H

#include <stdio.h>

#define MAX_TOKEN_LENGTH 	2048

// Token types returned by lexical analyser

typedef enum
{
    // reserved words and `predefined' objects

    SYM_ACCESS,
//    SYM_ANY,
    SYM_CAPABILITIES,
    SYM_APPLICATION,
    SYM_AUGMENTS,
    SYM_BEGIN,
//    SYM_BIT,
//    SYM_BITSTRING,
    SYM_CHOICE,
    SYM_COMPONENTS,
    SYM_CONTACTINFO,
//    SYM_Counter,
    SYM_Counter32,
    SYM_Counter64,
    SYM_DEFAULT,
    SYM_DEFINITIONS,
    SYM_DEFVAL,
    SYM_DESCRIPTION,
    SYM_DISPLAYHINT,
    SYM_END,
//    SYM_ENUMERATED,
    SYM_EXPLICIT,
    SYM_EXPORTS,
    SYM_FROM,
//    SYM_Gauge,
    SYM_Gauge32,
    SYM_IDENTIFIER,
    SYM_IMPLICIT,
    SYM_IMPORTS,
    SYM_INDEX,
    SYM_INTEGER,
    SYM_Integer32,
//    SYM_IpAddress,
    SYM_LASTUPDATED,
    SYM_MACRO,
    SYM_MAXACCESS,
    SYM_MODCOMPLY,
    SYM_MODIDENT,
    SYM_NOACCESS,
    SYM_NOTATION,
    SYM_NOTIFTYP,
    SYM_NULL,
    SYM_NUMENTRIES,
//    SYM_NetAddress,
//    SYM_NsapAddress,
    SYM_OBJECT,
    SYM_OBJECTID,
    SYM_OBJECTS,
    SYM_OBJECTGRP,
    SYM_OBJECTIDENT,
    SYM_OBJECTTYP,
    SYM_OCTET,
    SYM_OCTETSTRING,
    SYM_OF,
//    SYM_Opaque,
    SYM_ORGANIZATION,
    SYM_PRIVATE,
//    SYM_REAL,
    SYM_REFERENCE,
    SYM_SEQUENCE,
//    SYM_SET,
    SYM_SIZE,
    SYM_STATUS,
    SYM_STRING,
    SYM_SYNTAX,
    SYM_TEXTCONV,
    SYM_TRAPTYP,
    SYM_TYPE,
//    SYM_TimeTicks,
    SYM_UInteger32,
    SYM_UNITS,
    SYM_UNIVERSAL,
    SYM_VALUE,
    SYM_CURRENT,
    SYM_DEPRECATED,
    SYM_MANDATORY,
    SYM_UNACCESS,
    SYM_OBSOLETE,
    SYM_OPTIONAL,
    SYM_READCREATE,
    SYM_READONLY,
    SYM_READWRITE,
    SYM_RECOMMENDED,
    SYM_WRITEONLY,

    // Identifiers and literals

    SYM_IDENT,
    SYM_INTLITERAL,
    SYM_STRLITERAL,

    // Punctuation

    SYM_ASSIGN,
    SYM_LEFTBRACE,
    SYM_RIGHTBRACE,
    SYM_LEFTPAREN,
    SYM_RIGHTPAREN,
    SYM_LEFTBRACKET,
    SYM_RIGHTBRACKET,
    SYM_PIPE,
    SYM_COMMA,
    SYM_SEMICOLON,
    SYM_DOTDOT,
    SYM_MINUS,

    // EOF and undefined

    SYM_EOF,
    SYM_UNDEFINED
} enToken;

class clTokeniser
{
private:
    FILE*	source_fp;
    int		ch;
    enToken	token_type;
    char	token_name[MAX_TOKEN_LENGTH+1];
    long	token_value;
    int		line_number;
    enToken	LookupName();
    void	NextChar();
    void	StripComment();
    void	ScanInteger(int isminus);
    void	ScanString();
    void	ScanName();
    char *	Place();
public:
    enToken	NextToken();
    void	Expect(enToken tok);
    enToken	Token()
    {
	return token_type;
    }
    long	TokenVal()
    {
	return token_value;
    }
    clTokeniser(char *source_file_name);
    int Line()
    {
	return line_number;
    }
    const char *TokenName()
    {
	return token_name;
    }
    const char *TokenID(enToken t);

    void Warning(const enError err, const char *arg = 0)
    {
    	ErrorHandler->Warning(err, Place(), arg);
    }
    void Error(const enError err, const char *arg = 0)
    {
    	ErrorHandler->Error(err, Place(), arg);
    }
    void Fatal(const enError err, const char *arg = 0)
    {
    	ErrorHandler->Fatal(err, Place(), arg);
    }
};

#endif

