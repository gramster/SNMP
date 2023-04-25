//--------------------------------------------------------------
// asn1cc.cc - driver program for ASN.1 compiler for SNMP MIBs
// 
//
// Written by Graham Wheeler, August 1995.
// Based on the Pascal - driver written for UCT PLT course.
//
// (c) 1995, Open Mind Solutions (cc)
// All Rights Reserved
//
// Copying or using this software or derived works without
// the written permission of Open Mind Solutions is strictly
// prohibited.
//
// NOTE: Macros are not supported. Support for the SNMP
//	 OBJECT-IDENTIFIER macro is hard-coded in the parser.
//       ASN.1 values are not supported, and only a small
//	 number of primitive and constructed types are supported.
// 
//--------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "smi.h"
#include "asn1err.h"
#include "asn1name.h"
#include "asn1lex.h"
#include "asn1stab.h"
#include "asn1pars.h"

static void useage()
{
    fprintf(stderr,"Useage: asncc <filename>\n");
    exit(-1);
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < (argc-1); i++)
    {
    	if (argv[i][0]=='-' && argv[i][2]==0)
    	    switch(argv[i][1])
    	    {
    	    default:
    		useage();
    	    }
    	else useage();
    }
    char *ifname;
    if (i == (argc-1)) ifname = argv[i];
    else useage();
    ErrorHandler = new clErrorHandler;
    assert(ErrorHandler);
    NameStore = new clNameStore;
    assert(NameStore);

    clSymtab *stab = new clSymtab;
    clParser *parser = new clParser(new clTokeniser(ifname), stab);
    assert(parser);
    // parse the program
    parser->Parse();
    // should hit the end-of-file at end of parse
    // did we succeed?
    if (ErrorHandler->Errors())
    {
    	fprintf(stderr,"Compilation failed: %d errors and %d warnings\n",
    		ErrorHandler->Errors(), ErrorHandler->Warnings());
    }
    else
    {
    	fprintf(stderr,"Compilation succeeded: %d warnings\n",
    		ErrorHandler->Warnings());
	FILE *fp = fopen("mib.db", "w");
	stab->WriteMIB(fp);
	fclose(fp);
    }
    delete parser;
    int rtn = ErrorHandler->Errors();
    // delete stuff
    delete NameStore;
    delete ErrorHandler;
    return rtn;
}

