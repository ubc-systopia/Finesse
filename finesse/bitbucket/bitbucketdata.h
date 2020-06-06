//
// (C) Copyright 2020 Tony Mason (fsgeek@cs.ubc.ca)
// All Rights Reserved
//

#if !defined(_BITBUCKET_DATA_H_)
#define _BITBUCKET_DATA_H_ (1)

#if !defined(FUSE_USE_VERSION)
#define FUSE_USE_VERSION 39
#endif // FUSE_USE_VERSION

#include <fuse_lowlevel.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <uuid/uuid.h>
#include "trie.h"
#include "finesse-list.h"

// Arbitrary limits of what we will support.
#define MAX_FILE_NAME_SIZE (1024)
#define MAX_PATH_NAME_SIZE (65536)

// Common data structures used within the bitbucket file system.
static inline void verify_magic(const char *StructureName, const char *File, const char *Function, unsigned Line, uint64_t Magic, uint64_t CheckMagic)
{
    if (Magic != CheckMagic) {
        fprintf(stderr, "%s (%s:%d) Magic number mismatch (%lx != %lx) for %s\n", Function, File, Line, Magic, CheckMagic, StructureName);
        assert(Magic == CheckMagic);
    }
}

typedef struct _bitbucket_inode bitbucket_inode_t;


typedef struct _bitbucket_file {
    uint64_t            Magic; // magic number
    pthread_rwlock_t    Lock;
    uuid_t              FileId;
    char                FileIdName[40];
    struct stat         Attributes;
} bitbucket_file_t;

#define BITBUCKET_FILE_MAGIC (0x901bb9acacca7b19)

#define CHECK_BITBUCKET_FILE_MAGIC(bbf) verify_magic("bitbucket_file_t", __FILE__, __PRETTY_FUNCTION__, __LINE__, BITBUCKET_FILE_MAGIC, (bbf)->Magic)

typedef struct _bitbucket_dir {
    uint64_t              Magic; // magic number
    uuid_t                DirId;
    char                  DirIdName[40];
    bitbucket_inode_t    *Parent;
    list_entry_t          Entries;
    struct Trie          *Children;
    list_entry_t          EAs;
    struct Trie          *ExtendedAttributes;
} bitbucket_dir_t;

#define BITBUCKET_DIR_MAGIC (0x895fe26d657f24bd)
#define CHECK_BITBUCKET_DIR_MAGIC(bbd) verify_magic("bitbucket_dir_t", __FILE__, __PRETTY_FUNCTION__, __LINE__, BITBUCKET_DIR_MAGIC, (bbd)->Magic)

typedef struct _bitbucket_inode_table bitbucket_inode_table_t;
#define BITBUCKET_INODE_TABLE_BUCKETS (1024)


typedef struct _bitbucket_userdata {
    uint64_t            Magic;
    bitbucket_inode_t  *RootDirectory;
    void               *InodeTable;
} bitbucket_user_data_t;

#define BITBUCKET_USER_DATA_MAGIC 0xb2035aef09927b87  
#define CHECK_BITBUCKET_USER_DATA_MAGIC(bbud) verify_magic("bitbucket_userdata_t", __FILE__, __PRETTY_FUNCTION__, __LINE__, BITBUCKET_USER_DATA_MAGIC, (bbud)->Magic)

int BitbucketInsertInodeInTable(void *Table, bitbucket_inode_t *Inode);
void BitbucketRemoveInodeFromTable(bitbucket_inode_t *Inode);
bitbucket_inode_t *BitbucketLookupInodeInTable(void *Table, ino_t Inode);
void *BitbucketCreateInodeTable(uint16_t BucketCount);
void BitbucketDestroyInodeTable(void *Table);

#define BITBUCKET_FILE_TYPE (0x10)
#define BITBUCKET_DIR_TYPE  (0x11)
#define BITBUCKET_SYMLINK_TYPE (0x12)
#define BITBUCKET_DEVNODE_TYPE (0x13)
#define BITBUCKET_UNKNOWN_TYPE (0xFF)


//
// This allows coordination with the specialization component
//   * Initialize - this is called with the specialized portion of the object for initialization
//   * Deallocate - this is called when the reference count drops to zero.  Note that it is called with the lock held.
//                  The lock is assumed to be released (and likely destroyed) upon return.
//   * Lock - this is called whenever the reference count is changing; shared or exclusive.
//   * Unlock - this is called whenever the reference count change is complete
// The lock/unlock operation(s) are optional.  If they are not provided, the object package will
// use an internal default lock
//
typedef struct _bitbucket_object_attributes {
    uint64_t            Magic;
    void              (*Initialize)(void *Object, size_t Length); //
    void              (*Deallocate)(void *Object, size_t Length); // Call this when the reference count drops to zero
    void              (*Lock)(void *Object, int Exclusive);
    void              (*Unlock)(void *Object);
} bitbucket_object_attributes_t;

#define BITBUCKET_OBJECT_ATTRIBUTES_MAGIC (0x1b126261e6db52cd)
#define CHECK_BITBUCKET_OBJECT_ATTRIBUTES_MAGIC(bboa) verify_magic("bitbucket_object_attributes_t", __FILE__, __PRETTY_FUNCTION__, __LINE__, BITBUCKET_OBJECT_ATTRIBUTES_MAGIC, (bboa)->Magic)


typedef struct _bitbucket_symlink {
    uint64_t    Magic;
    char        LinkContents[0];
} bitbucket_symlink_t;

#define BITBUCKET_SYMLINK_MAGIC (0x6fba60a633f4e259)
#define CHECK_BITBUCKET_SYMLINK_MAGIC(bbsl) verify_magic("bitbucket_symlink_t", __FILE__, __PRETTY_FUNCTION__, __LINE__, BITBUCKET_SYMLINK_MAGIC, (bbsl)->Magic)

typedef struct _bitbucket_devnode {
    uint64_t    Magic;
    dev_t       Device;
    char        Data[0];
} bitbucket_devnode_t;

#define BITBUCKET_DEVNODE_MAGIC (0x08cb9f938b09ddfd)
#define CHECK_BITBUCKET_DEVNODE_MAGIC(bbdn) verify_magic("bitbucket_devnode_t", __FILE__, __PRETTY_FUNCTION__, __LINE__, BITBUCKET_DEVNODE_MAGIC, (bbdn)->Magic)

typedef struct _bitbucket_inode {
    uint64_t                        Magic;
    size_t                          InodeLength;
    uint8_t                         InodeType;
    pthread_rwlock_t                InodeLock;
    bitbucket_inode_table_t         *Table; // if not null, inode is inserted in an inode table
    uuid_t                          Uuid;
    char                            UuidString[40];
    struct stat                     Attributes;
    struct timeval                  AccessTime; // last time anyone accessed this file
    struct timeval                  ModifiedTime; // last time anyone changed the _contents_ of this file
    struct timeval                  CreationTime; // when this file was (first) created
    struct timeval                  ChangeTime; // last time _attributes_ of this file changed (including other timestamps)
    union {
        bitbucket_dir_t             Directory;
        bitbucket_file_t            File;
        bitbucket_symlink_t         SymbolicLink;
        bitbucket_devnode_t         DeviceNode;
    } Instance;
} bitbucket_inode_t;

#define BITBUCKET_INODE_MAGIC (0x3eb0674fe159eab4)
#define CHECK_BITBUCKET_INODE_MAGIC(bbi) verify_magic("bitbucket_inode_t", __FILE__, __PRETTY_FUNCTION__, __LINE__, BITBUCKET_INODE_MAGIC, (bbi)->Magic)

// Create a reference counted object;  On return the region returned will be at least
// ObjectSize bytes long and may be used as the caller sees fit.
//
// Note: the callbacks registered are invoked as needed: 
//   * initialize (set up this object)
//   * deallocate (prepare this object for deletion) - called with the lock held
//   * lock (synchronize access to this object)
//   * unlock (release this object)
//
// Default values are provided if NULL is passed
//
// Upon return, this object has a reference count of 1.
void *BitbucketObjectCreate(bitbucket_object_attributes_t *ObjectAttributes, size_t ObjectSize);

// Increment the reference count on this object
void BitbucketObjectReference(void *Object);

// Decrement the reference count on this object; if it drops to zero,
// call the deallocate callback and then delete the object.
// Note: this is done in a thread-safe fashion, provided that the
// object owner is locking it prior to lookup and reference counting it.
void BitbucketObjectDereference(void *Object);

// Return the number of objects outstanding in the system
uint64_t BitbucketObjectCount(void);

// Return the reference count of the given object
uint64_t BitbucketGetObjectReferenceCount(void *Object);


bitbucket_inode_t *BitbucketCreateInode(bitbucket_inode_table_t *Table, bitbucket_object_attributes_t *ObjectAttributes, size_t DataLength);


// Given an inode, insert it into the directory with the specified name.
int BitbucketInsertDirectoryEntry(bitbucket_inode_t *DirInode, bitbucket_inode_t *Inode, const char *Name);

// Given a directory, search for the name; if found, return a (referenced) pointer to the inode
void BitbucketLookupObjectInDirectory(bitbucket_inode_t *Inode, const char *Name, bitbucket_inode_t **Object);

// Given an inode, remove it from the specified entry.
// Return ENOENT if the entry isn't found.
int BitbucketDeleteDirectoryEntry(bitbucket_inode_t *Directory, const char *Name);


void BitbucketReferenceInode(bitbucket_inode_t *Inode);
void BitbucketDereferenceInode(bitbucket_inode_t *Inode);
uint64_t BitbucketGetInodeReferenceCount(bitbucket_inode_t *Inode);


// More random numbers
// 
// 
// 
//  
// b0 58 a9 26 86 f1 e0 2d
// fe 15 bc e7 d8 48 e1 c8 
// d5 a7 31 20 77 dc 4c 89
// a9 e1 65 46 9a 58 d6 0c  
//
// Source: https://www.random.org/cgi-bin/randbyte?nbytes=16&format=h

#endif // _BITBUCKET_DATA_H_
