#ifndef SMI_H
#define SMI_H

typedef enum		// ASN.1 classes
{
    CLS_PRIVATE,
    CLS_UNIVERSAL,
    CLS_EXPERIMENTAL,
    CLS_APPLICATION
}
enClass;

typedef enum		// SMI Object Type Access
{
    ACC_READONLY,
    ACC_READWRITE,
    ACC_WRITEONLY,
    ACC_UNACCESS
}
enAccess;

typedef enum		// SMI Object Type Status
{
    STS_MANDATORY,
    STS_OPTIONAL,
    STS_DEPRECATED,
    STS_OBSOLETE
}
enStatus;

// object types. The compiler only uses a subset of these (for ASN.1
// types) as it defines the SMI types as derived. However, to keep 
// the MIB tree simple, the fundamental SMI subtypes are also defined
// and used by the manager and agents.

typedef enum
{
    TYP_UNDEFINED,
    TYP_INTEGER,
    TYP_NULL,
    TYP_SEQUENCE,
    TYP_SEQUENCEOF,
    TYP_OBJECTID,
    TYP_OCTETSTRING,
    TYP_TAGGED,
    TYP_IMPLICITTAG,
    TYP_NAMEDTYPE,
    TYP_ELEMENTTYPE,	// sequence elt
    TYP_ENUMELT,	// enum elt
    TYP_ENUM,		// enum
    TYP_OBJECTTYP,
    TYP_INDEXELT,
    TYP_MODULE,

    // These aren't used by the compiler

    TYP_NETADDRESS,
    TYP_IPADDRESS,
    TYP_COUNTER,
    TYP_GAUGE,
    TYP_OPAQUE,
    TYP_TIMETICKS
} enType;

#endif

