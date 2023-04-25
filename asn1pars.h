//------------------------------------------------------------
// asn1pars.h - A compiler for a subset of ASN.1 for SNMP
//
// Written by Graham Wheeler, August 1995
// (c) 1995, Open Mind Solutions (cc)
// All Rights Reserved
//
// Copying or using this software or derived works without
// the written permission of Open Mind Solutions is strictly
// prohibited.
//------------------------------------------------------------

#ifndef ASN1PARS_H
#define ASN1PARS_H

const MAX_IMPORTS = 128;
const MAX_EXPORTS = 128;

class clParser
{
    clTokeniser *lex;		// lexical analyser
    clSymtab *stab;		// symbol table
    int  expidx[MAX_EXPORTS];	// indices of exported names
    int  numexp;		// number of exports
    
    void	InitPredefs();
    void	Import();
    void	Imports();
    void	Exports();
    void	MarkExports();
    void	ModuleBody();
    void	Definition();
    clSymtabEntry *Type();
    clSymtabEntry *TypeReference();
    clSymtabEntry *ElementType();
    clSymtabEntry *ElementTypeList();
    clSymtabEntry *Value(clSymtabEntry *typ);
    enClass	Tag(long &val);
    void	ModuleDefinition();
    clSymtabEntry *ObjectType();
    clSymtabEntry *ElementValueList(clSymtabEntry *typ);
    clSymtabEntry *ValueList(clSymtabEntry *typ);
    clObjIDEntry *ObjIDComponentList();
    clSymtabEntry *IndexType(clSymtabEntry *table);
    clSymtabEntry *IndexTypes(clSymtabEntry *table);
    // hand-offs to lexer
    void Warning(const enError err, const char *arg = 0);
    void Error(const enError err, const char *arg = 0);
    void Fatal(const enError err, const char *arg = 0);
    enToken	NextToken();
    void	Expect(enToken tok);
    enToken	Token();
    long	TokenVal();
    const char *TokenName();
    const char *TokenID(enToken t);
  public:
    clParser(clTokeniser *lex_in, clSymtab *stab_in);
    void Parse();
    ~clParser();
};
    
inline void clParser::Warning(const enError err, const char *arg)
{
    lex->Warning(err, arg);
}

inline void clParser::Error(const enError err, const char *arg)
{
    lex->Error(err, arg);
}

inline void clParser::Fatal(const enError err, const char *arg)
{
    lex->Fatal(err, arg);
}

inline enToken clParser::NextToken()
{
    return lex->NextToken();
}

inline void clParser::Expect(enToken tok)
{
    lex->Expect(tok);
}

inline enToken clParser::Token()
{
    return lex->Token();
}

inline long clParser::TokenVal()
{
    return lex->TokenVal();
}

inline const char *clParser::TokenName()
{
    return lex->TokenName();
}

inline const char *clParser::TokenID(enToken t)
{
    return lex->TokenID(t);
}

inline clParser::clParser(clTokeniser *lex_in, clSymtab *stab_in)
    : lex(lex_in), stab(stab_in), numexp(0)
{
}

inline clParser::~clParser()
{ 
    delete lex;
    delete stab;
}

#endif

