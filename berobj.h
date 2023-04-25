//-------------------------------------------------------------------
// berobj.h - ASN.1 classes with CCITT Basic Encoding Representation
//
// (c) Copyright 1995 by Open Mind Solutions, All Rights Reserved.
//
// Copying or using this software or derived works without
// the written permission of Open Mind Solutions is strictly
// prohibited.
//
// v1.0 by Gram, April '94
// v2.0 by Gram, August '95
//
// No indefinite-length encoding supported as yet
//
// Any value that needs to be BER encoded should be held in an instance
// of one of these classes. Generally only simple string, objid and integer
// values are of interest as far as the MIB goes; support for sequences
// is necessary only for encoding PDUs.
//
// The following ASN.1 types and constructs are supported:
//
//	NULL
//	INTEGER
//	SEQUENCE
//	SEQUENCE OF
//      OCTET STRING
//      OBJECT IDENTIFIER
//
// The following SNMP SMI types are supported
//
//	IpAddress
//	NetworkAddress (just aliased to IpAddress)
//	Counter
//	Gauge
//	TimeTicks
//	DisplayString	(just aliased to OCTET STRING)
//
// SNMP PDU's are also supported.
//-------------------------------------------------------------------

#ifndef BEROBJ_H
#define BEROBJ_H

//------------------------------------------------------------------------
// BER Classes

typedef enum
{
    C_UNIVERSAL		= (0<<6),
    C_APPLICATION	= (1<<6),
    C_CONTEXT		= (2<<6),
    C_PRIVATE		= (3<<6)
} enBERClass;

//------------------------------------------------------------------------
// BER Encoding Classes

typedef enum
{
    E_PRIMITIVE		= (0<<5),
    E_CONSTRUCTED	= (1<<5)
} enBEREncoding;

//------------------------------------------------------------------------
// Tag classes

typedef enum 
{
    T_IMPLICIT		= 0,
    T_EXPLICIT		= 1
} enBERTagType;

//------------------------------------------------------------------------
// BER Basic Tags

typedef enum
{
    T_UNINITIALISED	= 0,
    T_BOOLEAN		= 1,
    T_INTEGER   	= 2,
    T_BITSTRING 	= 3,
    T_OCTETSTRING 	= 4,
    T_NULLTYPE  	= 5,
    T_OBJECTID  	= 6,
    T_REAL      	= 9,
    T_ENUMERATED	= 10,
    T_SEQUENCE  	= 16,
    T_SET       	= 17
} enBERTagNum;

//-------------------------------------------------------------------
// SNMP Versions

typedef enum
{
    SNMP_V1		= 0,
    SNMP_V2		= 1
} enSNMPVersion;


//-------------------------------------------------------------------
// SNMP Errors

typedef enum
{
    E_noError		= 0,
    E_tooBig		= 1,
    E_noSuchName	= 2,
    E_badValue		= 3,
    E_readOnly		= 4,
    E_genErr		= 5
} enSNMPError;

//-------------------------------------------------------------------
// SNMP generic traps

typedef enum
{
    T_coldStart		= 0,
    T_warmStart		= 1,
    T_linkDown		= 2,
    T_linkUp		= 3,
    T_authFailure	= 4,
    T_egpNeighbourLoss	= 5,
    T_enterpriseSpecific= 6
} enSNMPGenTrap;

//-------------------------------------------------------------------

extern void
ReverseBuffer(const char *buf, unsigned long len);

//-------------------------------------------------------------------
// Tag numbers. We restrict to max 32-bits, but make it a type for
// later extensibility if necessary.

class clBERTagNum
{
    unsigned long tagnum;
  public:
    clBERTagNum(unsigned long tagnum_in = T_UNINITIALISED)
	: tagnum(tagnum_in)
    { }
    unsigned long Tag()
    {
        return tagnum;
    }
    unsigned long RvEncode(char* &buf);
    unsigned char *Decode(unsigned char *buf);
    ~clBERTagNum()
    { }
};

//------------------------------------------------------------------------
// Type class

class clBERType
{
    clBERTagNum	  tag;
    enBEREncoding en;
    enBERClass	  cl;
  public:
    // constructor
    clBERType()
	: tag(T_UNINITIALISED)
    { } 
    clBERType(enBERClass cl_in, enBEREncoding en_in, int tagnum_in)
	: tag(tagnum_in), en(en_in), cl(cl_in)
    { }
    unsigned long Tag()
    {
        return tag.Tag();
    }
    enBEREncoding Encoding()
    {
        return en;
    }
    enBERClass Class()
    {
        return cl;
    }
    unsigned long RvEncode(char* &buf);		// encoder
    unsigned char *Decode(unsigned char *buf);
    ~clBERType()
    { }
};

//------------------------------------------------------------------------
// Length class

class clBERLength
{
    unsigned long length;
  public:
    // constructor
    clBERLength(unsigned long length_in = 0ul)
        : length(length_in)
    { }
    unsigned long RvEncode(char* &buf);		// encoder
    unsigned char *Decode(unsigned char *buf);
    unsigned long Length()
    {
        return length;
    }
    ~clBERLength()
    { }
};

//------------------------------------------------------------------------
// TLV Class. Essentially, each ASN1Object class is responsible
// for handling its value (encoding, decoding, etc), while this
// class encodes a whole TLV.

class clBERTLV
{
    clBERType	&typ;
    clBERLength	len;
  public:
    clBERTLV(clBERType &typ_in, unsigned long len_in)
	: typ(typ_in), len(len_in)
    { }
    unsigned long RvEncode(char* &buf);
    unsigned char *Decode(unsigned char *buf);
    enBERClass Class()
    {
        return typ.Class();
    }
    enBEREncoding Encoding()
    {
        return typ.Encoding();
    }
    unsigned long Tag()
    {
        return typ.Tag();
    }
    unsigned long Length()
    {
        return len.Length();
    }
#ifdef DEBUG
    void Dump(ostream &os);
#endif
};

//=================================================================
// Abstract base class for ASN1 Objects

class clASN1Object
{
  protected:
    clBERType	typ;
  public:
    clASN1Object(enBERClass cls_in, enBEREncoding enc_in, int tag_in)
        : typ(cls_in, enc_in, tag_in)
    { }
    virtual unsigned long RvEncode(char *&buf) = 0;
    unsigned long Encode(const char *buf)
    {
        char *b = (char *)buf;
        unsigned long len = RvEncode(b);
        ReverseBuffer(buf,len);
        return len;
    }
    virtual unsigned char *Decode(unsigned char *buf) = 0;
    virtual char *Print(char *buf) = 0;
    unsigned long Tag()
    {
        return typ.Tag();
    }
    enBEREncoding Encoding()
    {
        return typ.Encoding();
    }
    enBERClass Class()
    {
        return typ.Class();
    }
    virtual ~clASN1Object()
    { }    
#ifdef DEBUG
    virtual void Dump(ostream &os) = 0;
#endif
};

//------------------------------------------------------------------------
// Integer

class clASN1Integer: public clASN1Object
{
  protected:
    long val;
  public:
    clASN1Integer(long val_in = 0l, 
		  enBERClass cls_in = C_UNIVERSAL,
		  enBEREncoding enc_in = E_PRIMITIVE,
		  int tag_in = (int)T_INTEGER)
	: clASN1Object(cls_in, enc_in, tag_in), val(val_in)
    { }
    virtual unsigned long RvEncode(char* &buf);
    virtual unsigned char *Decode(unsigned char *buf);
    virtual char *Print(char *buf)
    {
        sprintf(buf, "%ld", val); // what about unsigned ones??
	return buf;
    }
    long Value()
    {
        return val;
    }
    virtual ~clASN1Integer()
    { }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
};

//------------------------------------------------------------------------
// Octetstring
		     
class clASN1OctetString : public clASN1Object
{
  protected:
    char *val;
    int len;
  public:
    clASN1OctetString(char *val_in = 0,
		      int len_in = 0,	    		  
		      enBERClass cls_in = C_UNIVERSAL,
		      enBEREncoding enc_in = E_PRIMITIVE,
		      int tag_in = (int)T_OCTETSTRING)
	: clASN1Object(cls_in, enc_in, tag_in), val(0)
    { 
        if (val_in && val_in[0] && len_in == 0)
	    Set(val_in);
	else
	    Set(val_in, len_in);
    }
    void Set(char *val_in, int len_in);
    void Set(char *val_in)
    {
        assert(val_in);
        Set(val_in, strlen(val_in));
    }
    char *Value(int *len_out = 0)
    {
        if (len_out) *len_out = len;
        return val;
    }
    virtual char *Print(char *buf)
    {
        strncpy(buf, val, len);
	buf[len] = 0;
	return buf;
    }
    virtual unsigned long RvEncode (char* &buf);
    virtual unsigned char *Decode(unsigned char *buf);
    virtual ~clASN1OctetString()
    { 
        delete [] val;
    }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
};

//------------------------------------------------------------------------
// NULL type

class clASN1NULL : public clASN1Object
{
  public:
    clASN1NULL(enBERClass cls_in = C_UNIVERSAL,
	       enBEREncoding enc_in = E_PRIMITIVE,
	       int tag_in = (int)T_NULLTYPE)
        : clASN1Object(cls_in, enc_in, tag_in)
    { }
    virtual unsigned long RvEncode (char* &buf);
    virtual unsigned char *Decode(unsigned char *buf);
    virtual ~clASN1NULL()
    { }
    virtual char *Print(char *buf)
    {
        strcpy(buf, "NULL");
	return buf;
    }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
};

//------------------------------------------------------------------------
// Object Identifier

class clASN1ObjectIdentifier : public clASN1Object
{
    char *val;
  public:
    clASN1ObjectIdentifier(char *val_in = 0,
			   enBERClass cls_in = C_UNIVERSAL,
			   enBEREncoding enc_in = E_PRIMITIVE,
			   int tag_in = (int)T_OBJECTID)
        : clASN1Object(cls_in, enc_in, tag_in), val(0)
    {
        Set(val_in);
    }
    void Set(char *val_in)
    {
	delete [] val;
	if (val_in)
	{
	    val = new char[strlen(val_in)+1];
	    strcpy(val, val_in);
	}
	else val = 0;
    }
    char *Value()
    {
        return val;
    }
    virtual char *Print(char *buf)
    {
        strcpy(buf, val);
	return buf;
    }
    unsigned long RvEncodeElement(char* &buf, unsigned long v);
    virtual unsigned long RvEncode(char* &buf);
    unsigned char *DecodeElement(unsigned char* buf, unsigned long &v);
    virtual unsigned char *Decode(unsigned char *buf);
    virtual ~clASN1ObjectIdentifier()
    { 
        delete [] val;
    }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
};

//------------------------------------------------------------------------
// Doubly-linked list node

class clASN1ObjectListNode
{
  private:
    clASN1Object            *val;
    clASN1ObjectListNode    *next;
    clASN1ObjectListNode    *prev;
    friend class clASN1Constructed;
  public:
    clASN1ObjectListNode(clASN1Object *val_in = 0)
	: next(0), prev(0), val(val_in)
    { }
    ~clASN1ObjectListNode()
    {
        delete val;
    }
};

//--------------------------------------------------------------------
// Base class for constructed (composite) objects (sequences and sets)

class clASN1Constructed : public clASN1Object
{
  private:
    clASN1ObjectListNode *head;
    clASN1ObjectListNode *tail;
    clASN1ObjectListNode *now;
  public:
    clASN1Constructed(enBERClass cls_in, int tag_in)
	: clASN1Object(cls_in, E_CONSTRUCTED, tag_in),
	  head(0), tail(0), now(0)
    { }
    void Rewind()
    {
	now = head;
    }
    void Forward()
    {
	now = tail;
    }
    clASN1Object *Next()
    {
	if (now==0) return 0;
	clASN1Object *rtn = now->val;
	now = now->next;
	return rtn;
    }
    clASN1Object *Prev()
    {
	if (now==0) return 0;
	clASN1Object *rtn = now->val;
	now = now->prev;
	return rtn;
    }
    void Free();
    void Append(clASN1Object *tlv);
    void Prepend(clASN1Object *tlv);
    virtual unsigned long RvEncode(char* &buf);
    virtual unsigned char *Decode(unsigned char *buf);
    virtual char *Print(char *buf)
    {
        assert(0); // for now
	return buf;
    }
    virtual ~clASN1Constructed();
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
};

//------------------------------------------------------------------------
// Sequences

class clASN1Sequence: public clASN1Constructed
{
  public:
    clASN1Sequence(enBERClass cls_in = C_UNIVERSAL, int tag_in = T_SEQUENCE)
	: clASN1Constructed(cls_in, tag_in)
    { }
    virtual ~clASN1Sequence()
    { }    
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
};

//--------------------------------------------------------------------
// Explictly tagged types; essentially the same as a sequence, but
// with only one element (but this is not enforced!)

class clASN1ExplicitTaggedObject : public clASN1Constructed
{
  public:
    clASN1ExplicitTaggedObject(enBERClass cls_in = C_CONTEXT, int tag_in = 0)
	: clASN1Constructed(cls_in, tag_in)
    { }
    virtual ~clASN1ExplicitTaggedObject()
    { }    
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
};

//===================================================================
// SNMP-specific types

class clIPAddress : public clASN1OctetString
{
public:
    clIPAddress(char *addr = 0,
		enBERClass cls_in = C_APPLICATION,
		enBEREncoding enc_in = E_PRIMITIVE,
		int tag_in = 0)
        : clASN1OctetString(addr, strlen(addr), cls_in, enc_in, tag_in)
    { }
    virtual unsigned long RvEncode(char* &buf);
    virtual unsigned char *Decode(unsigned char *buf);
    virtual ~clIPAddress()
    { }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
};

typedef clIPAddress clNetworkAddress; // for now

//------------------------------------------------------------------

class clCounter : public clASN1Integer
{
public:
    clCounter(long val_in = 0l,
	      enBERClass cls_in = C_APPLICATION,
	      enBEREncoding enc_in = E_PRIMITIVE,
	      int tag_in = 1)
        : clASN1Integer(val_in, cls_in, enc_in, tag_in)
    { }
    virtual ~clCounter()
    { }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
};

//------------------------------------------------------------------
// may need to check for latch when setting

class clGauge : public clASN1Integer
{
public:
    clGauge(long val_in = 0l,
	    enBERClass cls_in = C_APPLICATION,
	    enBEREncoding enc_in = E_PRIMITIVE,
	    int tag_in = 2)
        : clASN1Integer(val_in, cls_in, enc_in, tag_in)
    { }
    virtual ~clGauge()
    { }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
};

//------------------------------------------------------------------

class clTimeTicks : public clASN1Integer
{
public:
    clTimeTicks(long val_in = 0l,
	    enBERClass cls_in = C_APPLICATION,
	    enBEREncoding enc_in = E_PRIMITIVE,
	    int tag_in = 3)
        : clASN1Integer(val_in, cls_in, enc_in, tag_in)
    { }
    virtual char *Print(char *buf)
    {
	sprintf(buf, "%d days %d hrs %d mins %d.%02d secs", val/8640000l,
			(val %8640000l)/360000l, (val%360000l)/6000l,
			(val%6000l)/100l, val%100l);
	return buf;
    }
    virtual ~clTimeTicks()
    { }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
};

//------------------------------------------------------------------

class clOpaque : public clASN1OctetString
{
public:
    clOpaque(char *val_in = 0,
	     int len_in = 0,
	     enBERClass cls_in = C_APPLICATION,
	     enBEREncoding enc_in = E_PRIMITIVE,
	     int tag_in = 4)
        : clASN1OctetString(val_in, len_in, cls_in, enc_in, tag_in)
    { }
    virtual ~clOpaque()
    { }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
};

//------------------------------------------------------------------

typedef clASN1OctetString	clDisplayString;

//--------------------------------------------------------------------
// Classes for SNMP PDU's

class clVarBinding : public clASN1Sequence
{
public:
    clVarBinding(char *objname, clASN1Object *val = 0)
        : clASN1Sequence()
    {
        Append(new clASN1ObjectIdentifier(objname)); // check!! ObjectName...
	if (val) Append(val);
	else Append(new clASN1NULL());
    }
    virtual unsigned char *Decode(unsigned char *buf);
    char *ObjectID()
    {
        Rewind();
	return ((clASN1ObjectIdentifier*)Next())->Value();
    }
    clASN1Object *Value()
    {
        Rewind(); (void)Next();
	return Next();
    }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
    virtual ~clVarBinding()
    { }
};

class clVarBindList : public clASN1Sequence
{
public:
    clVarBindList()
        : clASN1Sequence()
    { }
    void AddBinding(clVarBinding *vb)
    {
        Append(vb);
    }
    void AddBinding(char *objname, clASN1Object *val = 0)
    {
        Append(new clVarBinding(objname, val));
    }
    virtual unsigned char *Decode(unsigned char *buf);
    clVarBinding *Next()
    {
        return (clVarBinding*)clASN1Sequence::Next();
    }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
    virtual ~clVarBindList()
    { }
};

//--------------------------------------------------------------------

class clSNMPPDU : public clASN1Sequence
{
    clVarBindList *vb; // variable bindings
    void AddBinding(clVarBinding *b)
    {
        assert(vb);
	vb->Append(b);
    }
public:
    // constructor for sender
    clSNMPPDU(int tag, int reqid, 
	      enSNMPError errstatus = E_noError, int errindex = 0)
	: clASN1Sequence(C_CONTEXT, tag)
    {
    	Append(new clASN1Integer(reqid));
    	Append(new clASN1Integer(errstatus));
    	Append(new clASN1Integer(errindex));
	Append(vb = new clVarBindList());
    }
    // constructor for receiver
    clSNMPPDU(unsigned char *berval = 0)
        : clASN1Sequence(), vb(0)
    {
        if (berval) Decode(berval);
    }
    virtual unsigned char *Decode(unsigned char *berval);
    void AddBinding(char *objname) // use this for gets
    {
        AddBinding(new clVarBinding(objname));
    }
    void AddBinding(char *objname, clASN1Object *value) // use for sets
    {
        AddBinding(new clVarBinding(objname, value));
    }
    int RequestID();
    enSNMPError ErrorStatus();
    char *ErrorMessage();
    int ErrorIndex();
    clVarBindList *Bindings()
    {
        return vb;
    }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
    virtual ~clSNMPPDU()
    { }
};
		 
//---------------------------------------------------------------------

class clSNMPGetPDU : public clSNMPPDU
{
public:
    clSNMPGetPDU(int tag, int reqid = 0)
        : clSNMPPDU(tag, reqid)
    { }
    clSNMPGetPDU(char *berval)
        : clSNMPPDU(berval)
    { }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
    virtual ~clSNMPGetPDU()
    { }
};

//---------------------------------------------------------------------

class clGetRequestPDU : public clSNMPGetPDU
{
public:
    clGetRequestPDU(int reqid = 0)
        : clSNMPGetPDU(0, reqid)
    { }
    clGetRequestPDU(char *berval)
        : clSNMPGetPDU(berval)
    { }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
    virtual ~clGetRequestPDU()
    { }
};

//---------------------------------------------------------------------

class clGetNextRequestPDU : public clSNMPGetPDU
{
public:
    clGetNextRequestPDU(int reqid = 0)
        : clSNMPGetPDU(1, reqid)
    { }
    clGetNextRequestPDU(char *berval)
        : clSNMPGetPDU(berval)
    { }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
    virtual ~clGetNextRequestPDU()
    { }
};

//---------------------------------------------------------------------

class clGetResponsePDU : public clSNMPPDU
{
public:
    clGetResponsePDU(int reqid = 0,
		     enSNMPError errstatus = E_noError,
		     int errindex = 0)
        : clSNMPPDU(2, reqid, errstatus, errindex)
    { }
    clGetResponsePDU(char *berval)
        : clSNMPPDU(berval)
    { }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
    virtual ~clGetResponsePDU()
    { }
};

//---------------------------------------------------------------------

class clSetRequestPDU : public clSNMPPDU
{
public:
    clSetRequestPDU(int reqid = 0)
        : clSNMPPDU(3, reqid)
    { }
    clSetRequestPDU(char *berval)
        : clSNMPPDU(berval)
    { }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
    virtual ~clSetRequestPDU()
    { }
};

//---------------------------------------------------------------------

class clTrapPDU : public clASN1Sequence
{
    clVarBindList *vb; // variable bindings
public:
    clTrapPDU(char *objid, char *netaddr, enSNMPGenTrap generic, int specific, 
		long ticks);
    clTrapPDU(unsigned char *berval = 0)
        : clASN1Sequence()
    {
	if (berval) Decode(berval);
    }
    virtual unsigned char *Decode(unsigned char *buf);
    void AddBinding(clASN1Sequence *b)
    {
        assert(vb);
	vb->Append(b);
    }
    char	*ObjectID();
    char	*NetAddress();
    enSNMPGenTrap Generic();
    int		Specific();
    long	Ticks();
    clVarBindList *Bindings()
    {
        return vb;
    }
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
    virtual ~clTrapPDU()
    { }
};

//--------------------------------------------------------------------

class clSNMPMessage : public clASN1Sequence
{
public:
    clSNMPMessage(unsigned char *berval)
        : clASN1Sequence()
    { 
        Decode(berval);
    }
    clSNMPMessage(clASN1Object *data_in = 0,
		  char *community_in = "public",
		  enSNMPVersion version_in = SNMP_V1)
        : clASN1Sequence()
    { 
        Append(new clASN1Integer(version_in));
        Append(new clASN1OctetString(community_in));
        if (data_in) Append(data_in);
    }
    virtual unsigned char *Decode(unsigned char *berval);
    char *Community();
    enSNMPVersion Version();
    clASN1Object *Data();
#ifdef DEBUG
    virtual void Dump(ostream &os);
#endif
    virtual ~clSNMPMessage()
    { }
};

#endif



