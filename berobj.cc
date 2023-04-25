//------------------------------------------------------------------
// berobj.h - CCITT Basic Encoding Representation for ASN.1
//
// (c) Copyright 1995 by Open Mind Solutions, All Rights Reserved.
//
// Copying or using this software or derived works without
// the written permission of Open Mind Solutions is strictly
// prohibited.
//
// v1.0 by Gram, April '94
// v2.0 by Gram, August '95
//------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <stdio.h>  // for sscanf
#include <iostream.h>
#include <assert.h>

#include "berobj.h"

//----------------------------------------------------------------

#ifdef DEBUG

static char *tagclasses[] = 
{
    "UNIVERSAL", "APPLICATION", "", "PRIVATE"
};

// Utility used for indentation by Dump methods

static int indentation = 0;

static char *
indent()
{
    int i = indentation*3;
    static char buf[80];
    if (i>79) i = 79;
    buf[i] = 0;
    while (i--) buf[i] = ' ';
    return buf;
}
#endif

//-------------------------------------------------------------------
// Reverse the order of a memory buffer

void
ReverseBuffer(const char *buf, unsigned long len)
{
    char *tmp = new char[len], *p = tmp+len, *b = (char *)buf;
    for (int i = len; --i>=0; ) *--p = *b++;
    b = (char *)buf;
    p = tmp;
    for (i = len; --i >= 0; ) *b++ = *p++; // should use memcpy
    delete [] tmp;
}

//-------------------------------------------------------------------
// BER Type Tags

unsigned long
clBERTagNum::RvEncode(char* &buf)
{
    assert(buf);
    if (tagnum > 30)
    {
	for (unsigned long t = tagnum, l = 1; t != 0l ; t>>=7, l++)
	    *buf++ = (t & 0x7F) | ((l>1) ? 0x80 : 0);
	*buf = 0x1F;
	return l+1;
    }
    else
    {
	*buf = (char)tagnum;
	return 1;
    }
}

unsigned char *
clBERTagNum::Decode(unsigned char *buf)
{
    assert(buf);
    int t = 0x1F & buf[0], l = 1;
    if (t==0x1F)
    {
        tagnum = 0;
	for (;;)
	{
	    t = buf[l++];
	    tagnum = (tagnum << 7) + (t & 0x7F);
	    if ((t & 0x80)==0) break;
	}
    }
    else tagnum = t;
#ifdef DEBUG
    cout << "Tagnum " << tagnum << " length " << l << endl;
#endif
    return buf+l;
}

//-----------------------------------------------------------------------
// BER Types

unsigned long 
clBERType::RvEncode(char* &buf)
{
    assert(buf);
    int len = tag.RvEncode(buf);
    *buf++ |= ( (char) ( cl | en) );
    return len;
}
    
unsigned char *
clBERType::Decode(unsigned char *buf)
{
    assert(buf);
    cl = (enBERClass)(0xC0 & buf[0]);
#ifdef DEBUG
    cout << "TagClass " << tagclasses[cl>>6] << endl;
#endif
    en = (enBEREncoding)(0x20 & buf[0]);
    return tag.Decode(buf);
}

//-----------------------------------------------------------------------
// BER Lengths

unsigned long
clBERLength::RvEncode(char* &buf)
{
    assert(buf);
    if (length <=127ul)
    {
	*buf++ = (char)length;
	return 1ul;
    }
    else 
    {
	for (unsigned long l = length, cnt = 0; l != 0l; l>>=8, cnt++)
	    *buf++ = (char)((l & 0xFF));
	*buf++ = (char)(0x80 | (unsigned char)cnt);
	return cnt+1ul;
    }
}

unsigned char *
clBERLength::Decode(unsigned char *buf)
{
    assert(buf);
    int t = buf[0], l = 1;
    if ((t & 0x80) == 0) // short definite form?
        length = (unsigned long)t;
    else if (t == 0x80) // indefinite form
    {
    	assert(0); // not supported yet
    }
    else // long definite form
    {
        int k = t & 0x7F;
	length = 0ul;
	while (--k >= 0)
	    length = (length << 8) + (unsigned long)(buf[l++]);
    }
#ifdef DEBUG
    cout << "Length " << length << " len " << l << endl;
#endif
    return buf+l;
}

//-----------------------------------------------------------------
// BER Tag-Length-Values

unsigned long
clBERTLV::RvEncode(char* &buf)
{
    assert(buf);
    unsigned long rtn = len.RvEncode(buf);
    rtn += typ.RvEncode(buf);
    return rtn;
}

unsigned char *
clBERTLV::Decode(unsigned char *buf)
{
    unsigned char *b = len.Decode(typ.Decode(buf));
#ifdef DEBUG
    cout << "TLV len " << b - buf << endl;
#endif
    return b;
}

//=================================================================
// Now come the actual ASN.1 object types
//=================================================================

// NULL object

unsigned long
clASN1NULL::RvEncode(char* &buf)
{
    assert(buf);
    clBERTLV tlv(typ, 0);
    return tlv.RvEncode(buf);
}

unsigned char *
clASN1NULL::Decode(unsigned char *buf)
{
    clBERTLV tlv(typ, 0);
    return tlv.Decode(buf);
}

#ifdef DEBUG

void
clASN1NULL::Dump(ostream &os)
{
    os << " NULL ";
}
#endif

//-----------------------------------------------------------------
// Integers

unsigned long
clASN1Integer::RvEncode(char* &buf)
{
    assert(buf);
    int len = 0;
    if (val == 0l)
    {
        *buf++ = 0;
	len = 1;
    }
    else for (unsigned long v = val; v != 0l; v>>=8, len++)
	*buf++ = (char)((v & 0xFF));
    clBERTLV tlv(typ, len);
    return len + tlv.RvEncode(buf);
}

unsigned char *
clASN1Integer::Decode(unsigned char *buf)
{
    clBERTLV tlv(typ, 0);
    unsigned char *b = tlv.Decode(buf);
    assert(b);
    int l = tlv.Length();
    val = 0l;
    while (--l >= 0)
        val = (val << 8) + (*b++);
#ifdef DEBUG
    cout << "INT " << val << " len " << (b - buf) << endl;
#endif
    return b;
}

#ifdef DEBUG

void
clASN1Integer::Dump(ostream &os)
{
    os << indent() << "INTEGER " << hex << val << dec;
}
#endif

//-----------------------------------------------------------------
// Octet Strings (using primitive encoding for SNMP)

void 
clASN1OctetString::Set(char *val_in, int len_in)
{
    delete [] val;
    if (val_in)
    {
	val = new char[len = len_in];
	memcpy(val, val_in, len);
    }
    else
    { 
	val = 0;
	len = 0;
    }
}

unsigned long
clASN1OctetString::RvEncode(char* &buf)
{
    assert(buf);
    char *s = val + len;
    for (int i = len; --i >= 0; )
        *buf++ = *--s;
    clBERTLV tlv(typ, len);
    return len + tlv.RvEncode(buf);
}

unsigned char *
clASN1OctetString::Decode(unsigned char *buf)
{
    clBERTLV tlv(typ, 0);
    unsigned char *b = tlv.Decode(buf);
    len = tlv.Length();
    Set(b, len);
#ifdef DEBUG
    cout << "STR len " << (b - buf) + len << endl;
#endif
    return b+len;
}

#ifdef DEBUG

void
clASN1OctetString::Dump(ostream &os)
{
    char *buf = new char[len+1];
    assert(buf);
    memcpy(buf, val, len);
    buf[len] = 0;
    os << indent() << "STRING \"" << buf << "\""; // kludge!
    delete [] buf;
}
#endif

//-----------------------------------------------------------------
// Object Identifiers

unsigned long 
clASN1ObjectIdentifier::RvEncodeElement(char* &buf, 
	unsigned long v)
{
    unsigned long len = 0; 
    do
    {
        *buf++ = ( (len>0 ? 0x80 : 0) | (v & 0x7F));
	v >>= 7;
	len++;
    }
    while (v != 0l);
    return len;
}

unsigned long 
clASN1ObjectIdentifier::RvEncode(char* &buf)
{
    char *s;
    unsigned len = 0;
    unsigned long uv;
    char *v = new char[strlen(val)+1];
    strcpy(v, val);
    while ((s = strrchr(v, '.')) != 0)
    {
    	*s++ = 0;
	if (strrchr(v, '.') != 0)
	{
	    int rtn = sscanf(s, "%ul%*s", &uv);
	    assert(rtn == 1); // else error in objid
	    len += RvEncodeElement(buf, uv);
	}
	else
	{ 
	    // encode last two held in v and s1
	    unsigned long uv1;
	    int rtn = sscanf(s, "%ul%*s", &uv);
	    assert(rtn == 1); // else error in objid
	    rtn = sscanf(v, "%ul%*s", &uv1);
	    assert(rtn == 1); // else error in objid
	    len += RvEncodeElement(buf, uv1*40l+uv);
	    break;
	}
    }
    if (len == 0)
    {
        // only one element
	int rtn = sscanf(v, "%ul%*s", &uv);
	assert(rtn == 1); // else error in objid
	len += RvEncodeElement(buf, uv);
    }
    clBERTLV tlv(typ, len);
    return len + tlv.RvEncode(buf);
}

unsigned char * 
clASN1ObjectIdentifier::DecodeElement(unsigned char* buf, unsigned long &v)
{
    v = 0ul;
    int l = 0;
    for (;;)
    {
        unsigned t = buf[l++];
	v = (v<<7) + (t & 0x7F);
	if ((t & 0x80)==0) break;
    }
    return buf+l;
}

unsigned char *
clASN1ObjectIdentifier::Decode(unsigned char *buf)
{
    clBERTLV tlv(typ, 0);
    unsigned char *b = tlv.Decode(buf);
    unsigned long len = tlv.Length();
    unsigned char *e = b + len;
    unsigned char *v = new char[4*len+10]; // should be space enuf
    delete [] val;
    val = 0;
    if (v)
    {
        v[0] = 0;
	while (b < e)
    	{
            unsigned long iv;
	    unsigned char num[16];
	    b = DecodeElement(b, iv);
	    if (v[0] == 0) // first?
	    {
	        sprintf(num, "%lu.%lu", iv/40l, iv%40l);
		strcat(v, num);
	    }
	    else
	    {
	        sprintf(num, ".%lu", iv);
		strcat(v, num);
	    }
	}
	val = new char[strlen(v)+1];
	if (val) strcpy(val, v);
	delete [] v;
    }
#ifdef DEBUG
    cout << "OBJID len " << (e - buf) << "  " << val << endl;
#endif
    return e;
}

#ifdef DEBUG

void
clASN1ObjectIdentifier::Dump(ostream &os)
{
    os << indent() << "OBJID \"" << val << "\"";
}
#endif

//-----------------------------------------------------------------
// Constructed objects (sets, sequences, etc)

void 
clASN1Constructed::Append(clASN1Object *obj)
{
    clASN1ObjectListNode *n = new clASN1ObjectListNode(obj);
    if (tail == 0) 
	head = tail = n;
    else
    {
	tail->next = n;
	n->prev = tail;
	tail = n;
    }
}

void 
clASN1Constructed::Prepend(clASN1Object *obj)
{
    clASN1ObjectListNode *n = new clASN1ObjectListNode(obj);
    if (head == 0) 
	head = tail = n;
    else
    {
	head->prev = n;
	n->next = head;
	head = n;
    }
}

unsigned long
clASN1Constructed::RvEncode(char* &buf)
{
    unsigned long len = 0ul;
    Forward();
    clASN1Object *obj;
    while ((obj = Prev()) != 0)
	len += obj->RvEncode(buf);
    clBERTLV tlv(typ, len);
    return len + tlv.RvEncode(buf); // encode type and length
}

void
clASN1Constructed::Free()
{
    Rewind();
    while (now)
    {
        clASN1ObjectListNode *tmp = now;
	now = now->next;
	delete tmp;
    }
    head = tail = now = 0;
}

unsigned char *
clASN1Constructed::Decode(unsigned char *buf)
{
    clBERTLV tlv(typ, 0);
    unsigned char *b = tlv.Decode(buf), *e;
    unsigned long len = tlv.Length();
    e = b + len;
    Free();
    // Decode the components.
    // This can't be done in general without extra info - we need
    // to know whether the underlying type is int, string, or objid,
    // given a BER type (cls, enc, tag).
    while (b < e)
    {
        clBERType typ;
        clBERTLV subtlv(typ, 0);
	clASN1Object *obj = 0;
	(void)subtlv.Decode(b);
	// map subtlv to underlying type, allocate an instance of the
	// appropriate class for that underlying type, and decode it.
	// We fudge it for now using domain-specific knowledge from 
	// SMI, and the SNMP PDU's.
	if (subtlv.Class() == C_UNIVERSAL)
	{
	    switch (subtlv.Tag())
	    {
	    case T_INTEGER:
	        obj = new clASN1Integer();
	    	break;
	    case T_OCTETSTRING:
	        obj = new clASN1OctetString();
	    	break;
	    case T_OBJECTID:
	        obj = new clASN1ObjectIdentifier();
	    	break;
	    case T_SEQUENCE:
	        obj = new clASN1Sequence();
	        break;
	    case T_NULLTYPE:
	        obj = new clASN1NULL();
	        break;
	    }
	}
	else if (subtlv.Class() == C_APPLICATION)
	{
	    switch (subtlv.Tag())
	    {
	    case 0:
	        obj = new clIPAddress();
	    	break;
	    case 1:
	        obj = new clCounter();
	    	break;
	    case 2:
	        obj = new clGauge();
	    	break;
	    case 3:
	        obj = new clTimeTicks();
	    	break;
	    case 4:
	        obj = new clOpaque();
	    	break;
	    }
	}
	else if (subtlv.Class() == C_CONTEXT)
	{
	    if (subtlv.Encoding() == E_CONSTRUCTED)
	        obj = new clASN1Sequence(); // a bit presumptious!
	}
	if (obj)
	{
	    b = obj->Decode(b);
	    Append(obj);
	}
	else
	{
#ifdef DEBUG
	    cerr << "Bad component: cls " << tagclasses[subtlv.Class()>>6] <<
		"  enc " << subtlv.Encoding() << "  tag " << subtlv.Tag()
		<< endl;
#endif
	    assert(0); // for now
	}
    }
    return e;
}

clASN1Constructed::~clASN1Constructed()
{
    Free();
}

#ifdef DEBUG

void
clASN1Constructed::Dump(ostream &os)
{
    Rewind();
    indentation++;
    clASN1Object *t = Next();
    for(;;)
    {
        t->Dump(os);
	if ((t = Next()) != 0)
	    os << "," << endl;
	else break;
    }
    indentation--;
}
#endif // DEBUG

//-----------------------------------------------------------------
// Sequences

#ifdef DEBUG

void
clASN1Sequence::Dump(ostream &os)
{
    if (typ.Class() != C_UNIVERSAL || typ.Tag() != T_SEQUENCE)
        os << indent() << "[" << tagclasses[typ.Class()>>6] << " " <<
		typ.Tag() << "] ";
    os << indent() << "SEQUENCE {" << endl;
    clASN1Constructed::Dump(os);
    os << indent() << "}" << endl;
}
#endif // DEBUG

//-----------------------------------------------------------------
// Explicitly tagged objects

#ifdef DEBUG

void
clASN1ExplicitTaggedObject::Dump(ostream &os)
{
    os << indent() << "[" << tagclasses[typ.Class()>>6] << " " << typ.Tag() << "] ";
    clASN1Constructed::Dump(os);
}
#endif // DEBUG

//----------------------------------------------------------------------
// SMI Types

unsigned long clIPAddress::RvEncode(char* &buf)
{
    assert(buf);
    char buf2[4];
    sscanf(val, "%d.%d.%d.%d", &buf2[0], &buf2[1], &buf2[2], &buf2[3]);
    char *s = buf2 + 4;
    for (int i = len; --i >= 0; )
        *buf++ = *--s;
    clBERTLV tlv(typ, 4);
    return 4 + tlv.RvEncode(buf);
}

unsigned char *clIPAddress::Decode(unsigned char *buf)
{
    clBERTLV tlv(typ, 0);
    unsigned char *b = tlv.Decode(buf);
    len = tlv.Length();
    char buf2[80];
    buf2[0] = 0;
    for (int i = 0; i < len; i++)
    {
	char num[10];
	sprintf(num, "%d", b[i]);
	if (i) strcat(buf2, ".");
	strcat(buf2, num);
    }
    Set(buf2);
#ifdef DEBUG
    cout << "IPaddr len " << (b - buf) + len << endl;
#endif
    return b+len;
}

#ifdef DEBUG

void
clIPAddress::Dump(ostream &os)
{
    clASN1OctetString::Dump(os);
}

void
clCounter::Dump(ostream &os)
{
    clASN1Integer::Dump(os);
}

void
clGauge::Dump(ostream &os)
{
    clASN1Integer::Dump(os);
}

void
clTimeTicks::Dump(ostream &os)
{
    clASN1Integer::Dump(os);
}

void
clOpaque::Dump(ostream &os)
{
    clASN1OctetString::Dump(os);
}

#endif

//----------------------------------------------------------------------

unsigned char *
clVarBinding::Decode(unsigned char *buf)
{
    return clASN1Sequence::Decode(buf); // prob not general enuf...
}

#ifdef DEBUG

void
clVarBinding::Dump(ostream &os)
{
    clASN1Sequence::Dump(os);
}

#endif // DEBUG

//----------------------------------------------------------------------

unsigned char *
clVarBindList::Decode(unsigned char *buf)
{
    return clASN1Sequence::Decode(buf);
}

#ifdef DEBUG

void
clVarBindList::Dump(ostream &os)
{
    clASN1Sequence::Dump(os);
}
#endif // DEBUG

//----------------------------------------------------------------------

unsigned char *
clSNMPPDU::Decode(unsigned char *berval)
{
    Free();
    clBERTLV tlv(typ, 0);
    berval = tlv.Decode(berval);
    // should check the tag for validity
    clASN1Object *o;
    Append(o = new clASN1Integer());	berval = o->Decode(berval);
    Append(o = new clASN1Integer());	berval = o->Decode(berval);
    Append(o = new clASN1Integer());	berval = o->Decode(berval);
    Append(vb = new clVarBindList());	berval = vb->Decode(berval);
    return berval;
}

int clSNMPPDU::RequestID()
{
    Rewind();
    return ((clASN1Integer*)Next())->Value();
}

enSNMPError clSNMPPDU::ErrorStatus()
{
    Rewind(); (void)Next();
    return (enSNMPError)((clASN1Integer*)Next())->Value();
}

char *clSNMPPDU::ErrorMessage()
{
    switch(ErrorStatus())
    {
    case E_noError:
        return 0;
    case E_tooBig:
        return "Too much data requested";
    case E_noSuchName:
        return "No such name";
    case E_badValue:
        return "Bad value";
    case E_readOnly:
        return "Cannot set read-only object";
    case E_genErr:
        return "Request failed";
    default:
    	return "Unknown error";
    }
}

int clSNMPPDU::ErrorIndex()
{
    Rewind(); (void)Next(); (void)Next();
    return ((clASN1Integer*)Next())->Value();
}

#ifdef DEBUG

void
clSNMPPDU::Dump(ostream &os)
{
    clASN1Sequence::Dump(os);
}

void
clSNMPGetPDU::Dump(ostream &os)
{
    clSNMPPDU::Dump(os);
}

void
clGetRequestPDU::Dump(ostream &os)
{
    clSNMPGetPDU::Dump(os);
}

void
clGetNextRequestPDU::Dump(ostream &os)
{
    clSNMPGetPDU::Dump(os);
}

void
clGetResponsePDU::Dump(ostream &os)
{
    clSNMPPDU::Dump(os);
}

void
clSetRequestPDU::Dump(ostream &os)
{
    clSNMPPDU::Dump(os);
}

#endif // DEBUG

//----------------------------------------------------------------------

clTrapPDU::clTrapPDU(char *objid, char *netaddr, 
		     enSNMPGenTrap generic, int specific, long ticks)
    : clASN1Sequence(C_CONTEXT, 4)
{
    Append(new clASN1ObjectIdentifier(objid));
    Append(new clNetworkAddress(netaddr));
    Append(new clASN1Integer(generic));
    Append(new clASN1Integer(specific));
    Append(new clTimeTicks(ticks));
    Append(vb = new clVarBindList);
}

unsigned char *
clTrapPDU::Decode(unsigned char *buf)
{
    Free();
    clBERTLV tlv(typ, 0);
    unsigned char *berval = tlv.Decode(berval = buf);
    // should check the tag for validity
    clASN1Object *o;
    Append(o = new clASN1ObjectIdentifier());	berval = o->Decode(berval);
    Append(o = new clNetworkAddress());		berval = o->Decode(berval);
    Append(o = new clASN1Integer());		berval = o->Decode(berval);
    Append(o = new clASN1Integer());		berval = o->Decode(berval);
    Append(o = new clTimeTicks());		berval = o->Decode(berval);
    Append(vb = new clVarBindList());		berval = vb->Decode(berval);
    return berval;
}

char *clTrapPDU::ObjectID()
{
    Rewind();
    clASN1ObjectIdentifier *s = (clASN1ObjectIdentifier*)Next();
    return s->Value();
}

char *clTrapPDU::NetAddress()
{
    Rewind(); (void)Next();
    clNetworkAddress *s = (clNetworkAddress*)Next();
    return s->Value();
}

enSNMPGenTrap clTrapPDU::Generic()
{
    Rewind(); (void)Next(); (void)Next();
    return (enSNMPGenTrap)((clASN1Integer*)Next())->Value();
}

int clTrapPDU::Specific()
{
    Rewind(); (void)Next(); (void)Next(); (void)Next();
    return ((clASN1Integer*)Next())->Value();
}

long clTrapPDU::Ticks()
{
    Rewind(); (void)Next(); (void)Next(); (void)Next(); (void)Next();
    return ((clASN1Integer*)Next())->Value();
}
    
#ifdef DEBUG

void
clTrapPDU::Dump(ostream &os)
{
    clASN1Sequence::Dump(os);
}
#endif // DEBUG

//----------------------------------------------------------------------

unsigned char *
clSNMPMessage::Decode(unsigned char *berval)
{
    Free();
    clBERTLV tlv(typ, 0);
    berval = tlv.Decode(berval);
    // should check the tag for validity
    clASN1Object *o;
    Append(o = new clASN1Integer());	berval = o->Decode(berval);
    Append(o = new clASN1OctetString());berval = o->Decode(berval);
    
    // have to decode a tlv for lookahead to resolve PDU type...
    clBERType typ2;
    clBERTLV tlv2(typ2, 0);
    (void)tlv2.Decode(berval);
    if (tlv2.Class() == C_CONTEXT)
    {
        switch(tlv2.Tag())
	{
	case 0:
	    o = new clGetRequestPDU();
	    break;
	case 1:
	    o = new clGetNextRequestPDU();
	    break;
	case 2:
	    o = new clGetResponsePDU();
	    break;
	case 3:
	    o = new clSetRequestPDU();
	    break;
	case 4:
	    o = new clTrapPDU();
	    break;
	default:
	    ; // error!
	}
	Append(o);
	berval = o->Decode(berval);
    }
    // else error!
    return berval;
}

enSNMPVersion clSNMPMessage::Version()
{
    Rewind();
    clASN1Integer *i = (clASN1Integer*)Next();
    return (enSNMPVersion)(i->Value());
}

char *clSNMPMessage::Community()
{
    Rewind(); (void)Next();
    clASN1OctetString *s = (clASN1OctetString*)Next();
    return s->Value();
}

clASN1Object *clSNMPMessage::Data()
{
    Rewind(); (void)Next(); (void)Next();
    return Next();
}

#ifdef DEBUG

void
clSNMPMessage::Dump(ostream &os)
{
    clASN1Sequence::Dump(os);
}
#endif // DEBUG

//-----------------------------------------------------------------

#ifdef TEST

#include <stdio.h>

void 
dump(const char *buf, char *expect, int len)
{
    char *p = (char *)buf;
    while (len--) printf("%02X ", (int)(*p++)&0xFF);
    printf("   (Expect %s)\n\n", expect);
}

int 
main()
{
    char buf[100];
    int len;

    // test 0 - NULL
    
    clASN1NULL *nul = new clASN1NULL;
    len = nul->Encode(buf);
    cout << "NULL is ";
    dump(buf, "05 00", len);
    nul->Dump(cout);
    delete nul;
    
    // test 1 - simple integer

    clASN1Integer *bip = new clASN1Integer(-129);
    len = bip->Encode(buf);
    cout << endl << "-129 as integer is ";
    dump(buf, "02 02 FF 7F", len);
    bip->Dump(cout);

    // test 2 - sequence

    clASN1Sequence *bsp = new clASN1Sequence();
    bsp->Append(new clASN1Integer(3));
    bsp->Append(new clASN1Integer(8));
    len = bsp->Encode(buf);
    cout << endl << "Sequence of (integer 3, integer 8) is ";
    dump(buf, "30 06 02 01 03 02 01 08", len);
    bsp->Dump(cout);

    // test 3 - sequence of sequences

    clASN1Sequence *bsp2 = new clASN1Sequence();
    bsp2->Append(bip);
    clASN1Sequence *bspmain = new clASN1Sequence();
    bspmain->Append(bsp);
    bspmain->Append(bsp2);
    len = bspmain->Encode(buf);
    cout << endl << "Sequence of sequences is ";
    dump(buf, "??", len);
    bspmain->Dump(cout);
    delete bspmain;
    
    // test 4 - octet string
    
    clASN1OctetString *os = new clASN1OctetString("Jones", 6);
    len = os->Encode(buf);
    cout << endl << "Octet String is ";
    dump(buf, "04 06 4A 6F 6E 65 73 00", len);
    os->Dump(cout);
    delete os;
    
    // test 5 - object identifier
    
    clASN1ObjectIdentifier *oi = new clASN1ObjectIdentifier("2.100.3");
    len = oi->Encode(buf);
    cout << endl << "Object ID is ";
    dump(buf, "06 03 81 34 03", len);
    oi->Dump(cout);
    delete oi;
    
    // test 6 - object identifier
    
    oi = new clASN1ObjectIdentifier("1.0.8571.5.1");
    len = oi->Encode(buf);
    cout << endl << "Object ID is ";
    dump(buf, "06 05 28 C2 7B 05 01", len);
    oi->Dump(cout);
    delete oi;
    
    // test 7 - SNMP GetResponse PDU
    
    clGetResponsePDU *p = new clGetResponsePDU(17);
    p->AddBinding("1.3.6.1.2.1.1.1.1.0", new clASN1OctetString("unix"));
    clSNMPMessage *m = new clSNMPMessage(p);
    
    // test encode
    
    len = m->Encode(buf);
    cout << endl << "Get Request is ";
    dump(buf, "\n\t30 2B 02 01 00 04 06 70 75 62 6C 69 63 A2 1E"
	      "\n\t02 01 11 02 01 00 02 01 00 30 13 30 11 06 09"
	      "\n\t2B 06 01 02 01 01 01 01 00 04 04 75 6E 69 78", len);
    m->Dump(cout);
    delete m;
    
    // test decode
    
    m = new clSNMPMessage(buf);
    
    len = m->Encode(buf);
    cout << endl << "Get Request is ";
    dump(buf, "\n\t30 2B 02 01 00 04 06 70 75 62 6C 69 63 A2 1E"
	      "\n\t02 01 11 02 01 00 02 01 00 30 13 30 11 06 09"
	      "\n\t2B 06 01 02 01 01 01 01 00 04 04 75 6E 69 78", len);
    m->Dump(cout);
    cout << "Community: " << m->Community() << endl;
    cout << "Version: " << m->Version() << endl;
    clASN1Object *pdu = m->Data();
    if (pdu->Tag() == 4) // trap?
        assert(0);
    else if (pdu->Tag()<=3) 
    {
        clSNMPPDU *pp = (clSNMPPDU*)pdu;
        cout << "Request ID: " << pp->RequestID() << endl;
        cout << "Error Status: " << pp->ErrorStatus() << endl;
        cout << "Error Index: " << pp->ErrorIndex() << endl;
	clVarBindList *vb = pp->Bindings();
	vb->Rewind();
	for (clVarBinding *b = vb->Next(); b; b = vb->Next())
	{
	    cout << "Object ID: " << b->ObjectID() << endl;
	    //cout << "Value: " << b->ObjectName() << endl;
	}
    }
    delete m;
    return 0;
}

#endif // TEST
	

