//-----------------------------------------------------------------------
// asn1name.h - Header file for the identifier name store class
//	of the ASN.1 compiler
//
// Written by Graham Wheeler, August 1995
// Based on code wriiten for UCT PLT Course, 1994
//
// (c) 1995, Open Mind Solutions (cc)
// All Rights Reserved
//
// Copying or using this software or derived works without
// the written permission of Open Mind Solutions is strictly
// prohibited.
//
//-----------------------------------------------------------------------

#ifndef ASN1NAME_H
#define ASN1NAME_H

#define NUM_RESERVED_WORDS	80	// First 80 name indices are reserved
#define MAX_KEY			631	// Number of hash table entries
#define MAX_IDENTIFIERS		1000
#define MAX_STORESPACE		(MAX_IDENTIFIERS * 15)

// Hash table linked list entry

class clNameStore; // forward declaration

class clHashEntry
{
private:
    int		index;
    clHashEntry	*next;
    friend	clNameStore;
public:
    clHashEntry(int index_in, clHashEntry *next_in)
        : next(next_in), index(index_in)
    { }
};

#define MAPSIZE		(MAX_IDENTIFIERS-NUM_RESERVED_WORDS)

class clNameStore
{
private:
    clHashEntry	*hash[MAX_KEY];		// hash table
    int		map[MAPSIZE];		// indices => store offsets
    char	store[MAX_STORESPACE];	// holds the actual names
    int		top; 			// top of store
    int		next;			// next index to use
    const int Key(const char *name);
public:
    clNameStore();
    const int	Index(char *name);
    int		Lookup(char *name);
    const char*	Name(const int index) const;
    ~clNameStore();
};

extern clNameStore *NameStore;

#endif

