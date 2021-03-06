//
// (C) Copyright 2020
// Tony Mason
// All Rights Reserved

#include <errno.h>
#include <malloc.h>
#include "bitbucket.h"
#include "bitbucketcalls.h"

#define PAGE_SIZE (4096)

static int bitbucket_internal_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi);

void bitbucket_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
{
    struct timespec start, stop, elapsed;
    int             status, tstatus;

    tstatus = clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    assert(0 == tstatus);
    status  = bitbucket_internal_read(req, ino, size, off, fi);
    tstatus = clock_gettime(CLOCK_MONOTONIC_RAW, &stop);
    assert(0 == tstatus);
    timespec_diff(&start, &stop, &elapsed);
    BitbucketCountCall(BITBUCKET_CALL_READ, status ? 0 : 1, &elapsed);
}

static int bitbucket_internal_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
{
    void *                userdata = fuse_req_userdata(req);
    bitbucket_userdata_t *BBud     = (bitbucket_userdata_t *)userdata;
    bitbucket_inode_t *   inode    = NULL;
    int                   status   = 0;
    size_t                outsize  = 0;

    (void)fi;

    if (0 == size) {
        // zero byte reads always succeed
        fuse_reply_buf(req, NULL, 0);
        return 0;
    }

    // TODO: should we be doing anything with the flags in fi->flags?

    CHECK_BITBUCKET_USER_DATA_MAGIC(BBud);

    if (FUSE_ROOT_ID == ino) {
        inode = BBud->RootDirectory;
        BitbucketReferenceInode(inode, INODE_LOOKUP_REFERENCE);  // must have a lookup ref.
    }
    else {
        inode = BitbucketLookupInodeInTable(BBud->InodeTable, ino);  // returns a lookup ref.
    }

    //
    // This is what makes the file system a bit bucket: writes are discarded (with success, of course)
    //
    status = EBADF;

    BitbucketLockInode(inode, 0);
    while (NULL != inode) {
        if (BITBUCKET_FILE_TYPE != inode->InodeType) {
            status = EINVAL;
            break;
        }

        status = 0;  // only success cases after this point
        if (off > inode->Attributes.st_size) {
            // no data to read
            outsize = 0;
            break;
        }

        if (size + off > inode->Attributes.st_size) {
            outsize = inode->Attributes.st_size - off;
        }
        else {
            outsize = size;
        }

        if (outsize > 0) {
            // We need to ensure the file doesn't shrink while the data is being returned
            fuse_reply_buf(req, (void *)(((uintptr_t)inode->Instance.File.Map) + off), outsize);
        }
        break;
    }
    BitbucketUnlockInode(inode);  // don't need stable size any longer

    if (0 == status) {
        if (0 == outsize) {
            fuse_reply_buf(req, NULL, 0);
        }
        // else: we sent it with the lock held
    }
    else {
        fuse_reply_err(req, status);
    }

    if (NULL != inode) {
        BitbucketDereferenceInode(inode, INODE_LOOKUP_REFERENCE, 1);
        inode = NULL;
    }

    return status;
}
