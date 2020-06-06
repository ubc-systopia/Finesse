//
// (C) Copyright 2020 Tony Mason (fsgeek@cs.ubc.ca)
// All Rights Reserved
//

#include "bitbucket.h"
#include "bitbucketdata.h"
#include "trie.h"
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void DirectoryInitialize(void *Inode, size_t Length)
{
    bitbucket_inode_t *DirInode = (bitbucket_inode_t *)Inode;

    assert(BITBUCKET_INODE_MAGIC == DirInode->Magic);
    assert(BITBUCKET_UNKNOWN_TYPE == DirInode->InodeType);
    assert(Length == DirInode->InodeLength);

    DirInode->InodeType = BITBUCKET_DIR_TYPE; // Mark this as being a directory
    DirInode->Instance.Directory.Magic = BITBUCKET_DIR_MAGIC;
    initialize_list_entry(&DirInode->Instance.Directory.Entries);
    initialize_list_entry(&DirInode->Instance.Directory.EAs);
    DirInode->Instance.Directory.Children = NULL; // allocated when needed
    DirInode->Instance.Directory.ExtendedAttributes = NULL; // allocated when needed
    DirInode->Attributes.st_mode |= S_IFDIR; // mark as a directory
    DirInode->Attributes.st_nlink = 1; // .
    DirInode->Instance.Directory.Epoch = rand(); // detect changes

}

static void DirectoryDeallocate(void *Inode, size_t Length)
{
    bitbucket_inode_t *bbi = (bitbucket_inode_t *)Inode;

    assert(NULL != bbi);
    assert(0 == Length); // we don't have any extra data
    CHECK_BITBUCKET_INODE_MAGIC(bbi);
    CHECK_BITBUCKET_DIR_MAGIC(&bbi->Instance.Directory); // layers of sanity checking

    assert(empty_list(&bbi->Instance.Directory.Entries)); // directory should be empty
    assert(empty_list(&bbi->Instance.Directory.EAs)); // Don't handle cleanup yet

    bbi->Instance.Directory.Magic = ~BITBUCKET_DIR_MAGIC; // make it easy to recognize use after free

    // Our state is torn down at this point. 

}

static void DirectoryLock(void *Inode, int Exclusive)
{
    bitbucket_inode_t *bbi = (bitbucket_inode_t *)Inode;

    assert(NULL != bbi);
    CHECK_BITBUCKET_INODE_MAGIC(bbi);

    if (Exclusive) {
        pthread_rwlock_wrlock(&bbi->InodeLock);
        do {
            bbi->Instance.Directory.Epoch++; // just assume it changed
        } while (0 == bbi->Instance.Directory.Epoch); // Reserve 0 so it is never valid
    }
    else {
        pthread_rwlock_rdlock(&bbi->InodeLock);
    }

}

static void DirectoryUnlock(void *Inode)
{
    bitbucket_inode_t *bbi = (bitbucket_inode_t *)Inode;

    assert(NULL != bbi);
    CHECK_BITBUCKET_INODE_MAGIC(bbi);

    pthread_rwlock_unlock(&bbi->InodeLock);
}

static bitbucket_object_attributes_t DirectoryObjectAttributes = {
    .Magic = BITBUCKET_DIR_MAGIC,
    .Initialize = DirectoryInitialize,
    .Deallocate = DirectoryDeallocate,
    .Lock = DirectoryLock,
    .Unlock = DirectoryUnlock,
};

// Insert a new Inode into an existing directory
// Note: this function does not deal with name collisions very well at the moment (it should assert)
// A root directory entry has identical Inodes and a NULL pointer to the name
int BitbucketInsertDirectoryEntry(bitbucket_inode_t *DirInode, bitbucket_inode_t *Inode, const char *Name)
{
    bitbucket_dir_entry_t *newentry = NULL;
    size_t entry_length = sizeof(bitbucket_dir_entry_t);
    size_t name_length = 0;
    void *tobj = NULL;

    assert(NULL != DirInode);
    assert(NULL != Inode);
    assert(NULL != Name);

    assert(BITBUCKET_DIR_TYPE == DirInode->InodeType);
    CHECK_BITBUCKET_DIR_MAGIC(&DirInode->Instance.Directory);

    if (NULL != Name) {
        name_length = strlen(Name) + 1;
        assert(name_length > 1);
    }

    assert(name_length < MAX_FILE_NAME_SIZE);
    entry_length += name_length;
    newentry = (bitbucket_dir_entry_t *)malloc(entry_length);
    assert(NULL != newentry);

    newentry->Magic = BITBUCKET_DIR_ENTRY_MAGIC;
    newentry->Inode = Inode;
    BitbucketReferenceInode(Inode);
    initialize_list_entry(&newentry->ListEntry);
    if (NULL != Name) {
        strncpy(newentry->Name, Name, name_length);
        assert(strlen(newentry->Name) == strlen(Name)); // must be the same
    }

    if (BITBUCKET_DIR_TYPE == Inode->InodeType) {
        // Here we need to store the reference
        Inode->Instance.Directory.Parent = DirInode;
        BitbucketReferenceInode(DirInode);
    }


    DirectoryLock(DirInode, 1);
    if (NULL == DirInode->Instance.Directory.Children) {
        DirInode->Instance.Directory.Children = TrieCreateNode();
    }
    assert(NULL != DirInode->Instance.Directory.Children);

    tobj = TrieSearch(DirInode->Instance.Directory.Children, Name);
    assert(NULL == tobj); // TODO: deal with name collision
    TrieInsert(DirInode->Instance.Directory.Children, Name, newentry);

    insert_list_head(&DirInode->Instance.Directory.Entries, &newentry->ListEntry);

    DirInode->Attributes.st_nlink++;

    DirectoryUnlock(DirInode);


    return 0;
}

void BitbucketLookupObjectInDirectory(bitbucket_inode_t *Inode, const char *Name, bitbucket_inode_t **Object) 
{
    bitbucket_dir_t *Directory;
    bitbucket_dir_entry_t *dirent = NULL;

    assert(NULL != Inode);
    assert(NULL != Object);

    assert(BITBUCKET_DIR_TYPE == Inode->InodeType);
    Directory = &Inode->Instance.Directory;

    CHECK_BITBUCKET_DIR_MAGIC(Directory);


    if (NULL != Directory->Children) {

        DirectoryLock(Inode, 0);
        dirent = (bitbucket_dir_entry_t *) TrieSearch(Directory->Children, Name);
        if (NULL != dirent) {
            *Object = dirent->Inode;
        }
        DirectoryUnlock(Inode);
    }

    if (NULL == dirent) {
        // Entry not found
        *Object = NULL;
    }

    return;
}

// Bitbucket 

//
// Create a new directory with the given name, insert it into the parent directory, and
// return a pointer to that directory.
// Note: if the Parent is NULL, a root directory is created (so it contains itself)
//
bitbucket_inode_t *BitbucketCreateDirectory(bitbucket_inode_t *Parent, const char *DirName)
{
    bitbucket_inode_t *newdir = NULL;
    int status = 0;
    bitbucket_inode_table_t *table = NULL;

    // Either we have BOTH (normal subdir) or we have NEITHER (root directory case)
    assert(((NULL == Parent) && (NULL == DirName)) || ((NULL != Parent) && (NULL != DirName)));

    if (NULL != Parent) {
        CHECK_BITBUCKET_INODE_MAGIC(Parent);
        assert(BITBUCKET_DIR_TYPE == Parent->InodeType); // don't support anything else with "contents" (for now)
        CHECK_BITBUCKET_DIR_MAGIC(&Parent->Instance.Directory);

        table = Parent->Table; // it lives in the same table as the parent.
    }

    newdir = BitbucketCreateInode(table, &DirectoryObjectAttributes, 0);
    assert(NULL != newdir);
    CHECK_BITBUCKET_INODE_MAGIC(newdir);
    assert(BITBUCKET_DIR_TYPE == newdir->InodeType); // don't support anything else with "contents"
    CHECK_BITBUCKET_DIR_MAGIC(&newdir->Instance.Directory);

    if (NULL == Parent) {
        Parent = newdir; // Root is it's own Parent directory.
    }

    // All new directories point to themselves.
    status = BitbucketInsertDirectoryEntry(newdir, newdir, ".");
    assert(0 == status);

    // Child points to its parent (no name required)
    status = BitbucketInsertDirectoryEntry(newdir, Parent, "..");

    // Parent points to the child (if there's a name)
    if (NULL != DirName) {
        status = BitbucketInsertDirectoryEntry(Parent, newdir, DirName);
        assert(0 == status);
    }

    if (Parent != newdir) {
        // We don't need to keep the parent reference
        BitbucketDereferenceInode(Parent);
        Parent = NULL;
    }

    return newdir;

}

int BitbucketDeleteDirectoryEntry(bitbucket_inode_t *Directory, const char *Name)
{
    bitbucket_dir_t *directory = NULL;
    bitbucket_inode_t *inode = NULL;
    bitbucket_dir_entry_t *dirent = NULL;
    int status = ENOSYS;

    assert(NULL != Directory);
    assert(NULL != Name);
    assert(strlen(Name) > 0);
    CHECK_BITBUCKET_INODE_MAGIC(Directory);
    assert(BITBUCKET_DIR_TYPE == Directory->InodeType);
    CHECK_BITBUCKET_DIR_MAGIC(&Directory->Instance.Directory);
    directory = &Directory->Instance.Directory;

    // Now let's go find the entry

    BitbucketLookupObjectInDirectory(Directory, Name, &inode);
    if (NULL == inode) {
        return ENOENT;
    }

    DirectoryLock(Directory, 1);
    while (NULL == dirent) {
        dirent = (bitbucket_dir_entry_t *) TrieSearch(directory->Children, Name);
        if (NULL == dirent) {
            status = ENOENT;
            break;
        }

        // Remove the entries
        remove_list_entry(&dirent->ListEntry);
        TrieDeletion(&directory->Children, Name);
        status = 0;
        break;
    }
    DirectoryUnlock(Directory);

    BitbucketDereferenceInode(dirent->Inode);
    dirent->Inode = NULL;
    memset(dirent, 0, sizeof(bitbucket_dir_entry_t));
    free(dirent);
    dirent = NULL;

    return status;
}

void BitbucketLockDirectory(bitbucket_inode_t *Directory, int Exclusive)
{
    DirectoryLock(Directory, Exclusive);
}

void BitbucketUnlockDirectory(bitbucket_inode_t *Directory)
{
    DirectoryUnlock(Directory);
}


//
// This function is used to initialize an enumeration context structure
//
void BitbucketInitalizeDirectoryEnumerationContext(bitbucket_dir_enum_context_t *EnumerationContext)
{
    EnumerationContext->Magic = BITBUCKET_DIR_ENUM_CONTEXT_MAGIC;
    EnumerationContext->Directory = NULL;
    EnumerationContext->NextEntry = NULL;
    EnumerationContext->Offset = 0;
    EnumerationContext->Epoch = 0;
    EnumerationContext->LastError = 0;
}

// This is an iterative enumeration call/request 
// Note: this MUST be called with the directory locked (at least for read).
// If NULL is returned check the LastError field in the context: ENOENT means the enumeration is done, ERESTARTSYS means the
// directory contents changed during enumeration.  The enumeration cannot be continued in either case.
const bitbucket_dir_entry_t *BitbucketEnumerateDirectory(bitbucket_inode_t *Inode, bitbucket_dir_enum_context_t *EnumerationContext)
{
    list_entry_t *le = NULL;
    bitbucket_dir_entry_t *returnEntry = NULL;

    CHECK_BITBUCKET_INODE_MAGIC(Inode);
    assert(BITBUCKET_DIR_TYPE == Inode->InodeType);
    CHECK_BITBUCKET_DIR_MAGIC(&Inode->Instance.Directory);

    CHECK_BITBUCKET_DIR_ENUM_CONTEXT_MAGIC(EnumerationContext);

    while (1) {

        if (NULL == EnumerationContext->Directory) {
            // This is the first time it has been used.
            EnumerationContext->Directory = Inode;
            if (empty_list(&Inode->Instance.Directory.Entries)) {
                EnumerationContext->NextEntry = NULL;
            }
            else {
                EnumerationContext->NextEntry = container_of(le, bitbucket_dir_entry_t, ListEntry);
                le = list_head(&Inode->Instance.Directory.Entries);
            }
            EnumerationContext->Offset = EnumerationContext->NextEntry->Inode->Attributes.st_ino;
            EnumerationContext->Epoch = Inode->Instance.Directory.Epoch;
        }

        if (Inode != EnumerationContext->Directory) {
            // We're being handed an enumeration for the wrong directory
            EnumerationContext->LastError = EBADF;
            EnumerationContext->Offset = 0; // invalid
            break;
        }

        if (0 != EnumerationContext->LastError) {
            // We can't continue this enumeration; the caller needs to "clean things up".
            returnEntry = NULL;
            EnumerationContext->Offset = 0; // invalid
            break;
        }

        if (NULL == EnumerationContext->NextEntry) {
            // There are no more entries to return
            EnumerationContext->LastError = ENOENT;
            EnumerationContext->Offset = 0; // invalid
            break;
        }

        if (Inode->Instance.Directory.Epoch != EnumerationContext->Epoch) {
            // The directory shape has changed, so we need to re-verify.
            // Note: we _could_ be more forgiving here, such as trying to find the correct
            // location in the directory enumeration.
            EnumerationContext->LastError = ERESTART;
            EnumerationContext->Offset = 0; // invalid
            returnEntry = NULL;
            break;
        }

        returnEntry = EnumerationContext->NextEntry;
        EnumerationContext->Offset = returnEntry->Inode->Attributes.st_ino; // This is the "position"

        if (returnEntry->ListEntry.next == &Inode->Instance.Directory.Entries) {
            EnumerationContext->NextEntry = NULL; // we've hit the end
            break;
        }

        // set up for the next entry
        EnumerationContext->NextEntry = container_of(returnEntry->ListEntry.next, bitbucket_dir_entry_t, ListEntry);
        break;

    }

    return returnEntry;
}

//
// This will set the enumeration context to match the information in the Offset.  If the offset is not valid, this will return
// an error (not zero) otherwise it returns zero.  Note that the offset is an opaque value - do not rely upon the current
// implementation not changing.
// Note: the caller is responsible for locking the directory (for read) prior to invoking this call.
int BitbucketSeekDirectory(bitbucket_inode_t *Inode, bitbucket_dir_enum_context_t *EnumerationContext, uint64_t Offset)
{
    list_entry_t *le = NULL;
    bitbucket_dir_entry_t *dirEntry = NULL;
    int status = ENOENT;

    CHECK_BITBUCKET_INODE_MAGIC(Inode);
    assert(BITBUCKET_DIR_TYPE == Inode->InodeType);
    CHECK_BITBUCKET_DIR_MAGIC(&Inode->Instance.Directory);

    // Set up for the default return (which is an error condition)
    EnumerationContext->Magic = BITBUCKET_DIR_ENUM_CONTEXT_MAGIC;
    EnumerationContext->Directory = Inode;
    EnumerationContext->NextEntry = NULL;
    EnumerationContext->Offset = 0;
    EnumerationContext->Epoch = Inode->Instance.Directory.Epoch;
    EnumerationContext->LastError = ENOENT;

    list_for_each(&Inode->Instance.Directory.Entries, le) {
        dirEntry = container_of(le, bitbucket_dir_entry_t, ListEntry);
        if (dirEntry->Inode->Attributes.st_ino) {

            EnumerationContext->NextEntry = dirEntry;
            EnumerationContext->Offset = Offset;
            EnumerationContext->LastError = 0;
            status = 0;
            break;
        }
    }

    return status;

}