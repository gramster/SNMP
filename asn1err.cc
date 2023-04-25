//-----------------------------------------------------------------------
// error.cpp - error handler class of Pascal- compiler
//
// All Rights Reserved
//
// Copying or using this software or derived works without
// the written permission of Open Mind Solutions is strictly
// prohibited.
//
// Written by G. Wheeler for PLT Course, 1994
//-----------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "asn1err.h"
#include "asn1lex.h"

clErrorHandler *ErrorHandler = 0;

static int error_counts[3] =
{
    0, 0, 0
};

static char *error_class_names[3] =
{
    "Warning",
    "Error",
    "Fatal Error"
};

static char *error_message[] =
{
    "End of file reached while processing comment",
    "Integer is out of allowed range",
    "Illegal character %s",
    "Cannot open %s for input",
    "Maximum allowed number of unique identifiers exceeded",
    "Token is too long; truncated to %s",
    "Comment expected",
    "%s expected",
    "%s cannot be imported",
    "Exported name `%s' was not defined",
    "Undefined type: %s",
    "Undefined identifier: %s",
    "Redefinition of identifier: %s",
    "`%s' is not supported",
    "Bad sequence element specifier"
};  

int 
clErrorHandler::Warnings()
{
    return error_counts[WARN];
}

int 
clErrorHandler::Errors()
{
    return error_counts[ERROR];
}

void 
clErrorHandler::HandleError(enErrorClass severity, const enError err,
				const char *place, const char *arg)
{
    char msg[256];
    sprintf(msg, error_message[(int)err], arg);
    fprintf(stderr,"[%s %d] %s %s\n",
    	error_class_names[(int)severity],
    	++error_counts[(int)severity],
    	place?place:"", msg);
    if (severity == FATAL) exit(-1);
}

