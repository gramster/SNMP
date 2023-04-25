//-----------------------------------------------------------------------
// namestore.cpp - identifier name store class 
//
// (c) 1995, Open Mind Solutions (cc)
// All Rights Reserved
//
// Copying or using this software or derived works without
// the written permission of Open Mind Solutions is strictly
// prohibited.
//
// Written by G. Wheeler for PLT Course, 1994
//-----------------------------------------------------------------------

#include <string.h>
#include <assert.h>

#include "asn1err.h"
#include "asn1name.h"

clNameStore *NameStore = 0;

// Compute hash key for identifier

const int 
clNameStore::Key(const char *name)
{
    int sum=0;
    while (*name)
	sum += *name++;
    return sum % MAX_KEY;
}

// Look up a name in the hash table; return index or 0 if not present

const int 
clNameStore::Index(char *name)
{
    for (clHashEntry *e = hash[Key(name)] ; e ; e = e->next)
	if (strcmp(name, Name(e->index)) == 0)
	    return e->index;
    return 0;
}

// Look up a name in the name store, adding it if not yet present

int 
clNameStore::Lookup(char *name)
{
    int rtn = Index(name); // is it already present?
    if (rtn == 0) // no, this is a new name
    {
	if (next >= MAX_IDENTIFIERS)
	    ErrorHandler->Fatal(ERR_MAXIDENTS);
	int key = Key(name);
	// prepend a new hash entry to list for this key
	hash[key] = new clHashEntry(next, hash[key]);
	map[next - NUM_RESERVED_WORDS] = top;
	while (*name) store[top++] = *name++;
	store[top++] = '\0';
	rtn = next++;
    }
    return rtn;
}

// Get a name given its index

const char *
clNameStore::Name(const int index) const
{
    assert(index >= NUM_RESERVED_WORDS && index < next);
    return (const char *)(store+map[index - NUM_RESERVED_WORDS]);
}

// create a name store

clNameStore::clNameStore()
{
    for (int i = 0; i < MAX_KEY; i++)
	hash[i] = 0;
    next = NUM_RESERVED_WORDS;
    top = 0;
}

// destroy a name store

clNameStore::~clNameStore()
{
    for (int i = 0; i < MAX_KEY; i++)
    	while (hash[i])
    	{
    	    clHashEntry *e = hash[i];
    	    hash[i] = hash[i]->next;
    	    delete e;
    	}
}


