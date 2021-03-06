//
// (C) Copyright 2020
// Tony Mason
// All Rights Reserved

#include <errno.h>
#include <malloc.h>
#include <string.h>
#include "bitbucket.h"

// Extended Attributes support
typedef struct _bitbucket_xattr {
    uint64_t     Magic;
    list_entry_t ListEntry;
    size_t       NameLength;
    size_t       DataLength;
    char         Data[1];
} bitbucket_xattr_t;

#define BITBUCKET_XATTR_MAGIC (0xfe15bce7d848e1c8)
#define CHECK_BITBUCKET_XATTR_MAGIC(x) \
    verify_magic("bitbucket_xattr_t", __FILE__, __func__, __LINE__, BITBUCKET_XATTR_MAGIC, (x)->Magic)

void BitbucketInitializeExtendedAttributes(bitbucket_inode_t *Inode)
{
    assert(NULL != Inode);
    CHECK_BITBUCKET_INODE_MAGIC(Inode);

    initialize_list_entry(&Inode->ExtendedAttributes);
    Inode->ExtendedAttributeTrie = NULL;
}

//
// Given a name, this function will insert this entry into the
// EA list.  Note that if it already exists, this function will
// fail (e.g., we don't "update" EAs).
//
// A copy of the data is made by this routine.
//
// The caller must have an **exclusive** lock on this inode.
//
// returns 0 on success
//
int BitbucketInsertExtendedAttribute(bitbucket_inode_t *Inode, const char *Name, size_t DataLength, const void *Data)
{
    int                status      = EINVAL;
    bitbucket_xattr_t *xattr       = NULL;
    size_t             nameLength  = 0;
    size_t             xattrLength = 0;

    assert(NULL != Inode);
    CHECK_BITBUCKET_INODE_MAGIC(Inode);
    assert(NULL != Name);

    nameLength = strlen(Name);

    while (nameLength > 0) {
        xattrLength = offsetof(bitbucket_xattr_t, Data) + strlen(Name) + 1 + DataLength;
        xattrLength = (xattrLength + 0x3F) & ~0x3F;  // round up
        assert(xattrLength >=
               offsetof(bitbucket_xattr_t, Data) + nameLength + 1 + DataLength);  // in case I screw up rounding up...

        // this is how we verify proper locking - we can't acquire a shared lock
        status = BitbucketTryLockInode(Inode, 0);
        assert(EBUSY == status);
        if (0 == status) {
            // this isn't properly locked
            BitbucketUnlockInode(Inode);
            status = EINVAL;
            break;
        }

        if (NULL == Inode->ExtendedAttributeTrie) {
            Inode->ExtendedAttributeTrie = TrieCreateNode();
            assert(NULL != Inode->ExtendedAttributeTrie);
            if (NULL == Inode->ExtendedAttributeTrie) {
                status = ENOMEM;
                break;
            }
        }

        xattr = TrieSearch(Inode->ExtendedAttributeTrie, Name);
        if (NULL != xattr) {
            status = EEXIST;
            break;
        }

        xattr = malloc(xattrLength);
        if (NULL == xattr) {
            status = ENOMEM;
            break;
        }

        xattr->Magic = BITBUCKET_XATTR_MAGIC;
        initialize_list_entry(&xattr->ListEntry);
        xattr->NameLength = strlen(Name);
        xattr->DataLength = DataLength;
        strcpy((char *)xattr->Data, Name);
        memcpy(((char *)xattr->Data) + nameLength + 1, Data, DataLength);

        TrieInsert(Inode->ExtendedAttributeTrie, Name, xattr);
        insert_list_head(&Inode->ExtendedAttributes, &xattr->ListEntry);
        xattr  = NULL;
        status = 0;
        break;
    }

    if (NULL != xattr) {
        free(xattr);  // something must have gone wrong, so we clean up here
    }

    return status;
}

//
// Given a name, this function returns the data pointer.
// Note: it does not return a copy of the data!
//
// The caller must hold the inode locked during this lookup operation; once the
// lock is released, the data pointer is no longer guaranteed to remain valid.
//
// returns 0 on success
//
int BitbucketLookupExtendedAttribute(bitbucket_inode_t *Inode, const char *Name, size_t *DataLength, const void **Data)
{
    bitbucket_xattr_t *xattr = NULL;

    assert(NULL != Inode);
    CHECK_BITBUCKET_INODE_MAGIC(Inode);
    assert(NULL != Name);

    if (NULL == Inode->ExtendedAttributeTrie) {
        return ENODATA;
    }

    xattr = (bitbucket_xattr_t *)TrieSearch(Inode->ExtendedAttributeTrie, Name);

    if (NULL == xattr) {
        *DataLength = 0;
        *Data       = NULL;
        return ENODATA;
    }

    CHECK_BITBUCKET_XATTR_MAGIC(xattr);

    *DataLength = xattr->DataLength;
    *Data       = ((char *)xattr->Data) + xattr->NameLength + 1;

    return 0;  // success
}

//
// Given a name, this function deletes the extended attribute.
//
// The caller must hold the inode exclusively locked during this operation
//
// Returns 0 on success, error otherwise (e.g., ENOENT if it doesn't exist)
//
int BitbucketRemoveExtendedAttribute(bitbucket_inode_t *Inode, const char *Name)
{
    bitbucket_xattr_t *xattr = NULL;
    assert(NULL != Inode);
    CHECK_BITBUCKET_INODE_MAGIC(Inode);
    assert(NULL != Name);
    int status;

    if (NULL == Inode->ExtendedAttributeTrie) {
        return ENOENT;
    }

    xattr = (bitbucket_xattr_t *)TrieSearch(Inode->ExtendedAttributeTrie, Name);

    if (NULL == xattr) {
        return ENOENT;
    }

    status = TrieDeletion(&Inode->ExtendedAttributeTrie, Name);
    assert(0 == status);  // 0 = success

    remove_list_entry(&xattr->ListEntry);

    free(xattr);
    xattr = NULL;

    return 0;
}

//
// Call this when destroying an inode with extended attributes
//
// Note: this call is NOT assuming any locking as it should only
// be called in object teardown (when there can't be any other
// references!)
void BitbucketDestroyExtendedAttributes(bitbucket_inode_t *Inode)
{
    list_entry_t *     le     = NULL;
    bitbucket_xattr_t *xattr  = NULL;
    int                status = 0;

    // clean up extended attributes
    while (!empty_list(&Inode->ExtendedAttributes)) {
        le    = remove_list_head(&Inode->ExtendedAttributes);
        xattr = container_of(le, bitbucket_xattr_t, ListEntry);

        // The (null terminated) name is the first element in the Data block
        status = TrieDeletion(&Inode->ExtendedAttributeTrie, (const char *)xattr->Data);
        assert(0 == status);  // if not, something is wrong

        free(xattr);
        xattr = NULL;
    }
}
