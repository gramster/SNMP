//-----------------------------------------------------------------------
// asn1err.h - Header file for error handler class of ASN.1 compiler
//
// (c) 1995 Open Mind Solutions(cc)
// All Rights Reserved
//
// Copying or using this software or derived works without
// the written permission of Open Mind Solutions is strictly
// prohibited.
//
// Originally written by G. Wheeler for PLT Course, 1994
// This should be changed to maintain an error log for the
// session which can be queried. Then errors can be logged
// with impunity from anywhere. 
//-----------------------------------------------------------------------

#ifndef ASN1ERR_H
#define ASN1ERR_H

typedef enum
{
    ERR_COMMENT, ERR_NUMSIZE, ERR_BADCHAR, ERR_NOSOURCE,
    ERR_MAXIDENTS, ERR_TOKENLEN, ERR_NOCOMMENT, ERR_EXPECT,
    ERR_IMPORT, ERR_EXPORT, ERR_NOTYPE, ERR_UNDEFINED, ERR_REDEFINE,
    ERR_UNSUPPORTED, ERR_BADELT
} enError;

typedef enum
{
    WARN, ERROR, FATAL
} enErrorClass;

class clErrorHandler
{
private:
    void HandleError(enErrorClass severity, const enError err,
			const char *place, const char *arg);
public:
    void Warning(const enError err, const char *place = 0, const char *arg = 0)
    {
    	HandleError(WARN, err, place, arg);
    }
    void Error(const enError err, const char *place = 0, const char *arg = 0)
    {
    	HandleError(ERROR, err, place, arg);
    }
    void Fatal(const enError err, const char *place = 0, const char *arg = 0)
    {
    	HandleError(FATAL, err, place, arg);
    }
    int Errors();
    int Warnings();
};

extern clErrorHandler *ErrorHandler;

#endif

