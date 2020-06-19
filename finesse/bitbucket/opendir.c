//
// (C) Copyright 2020
// Tony Mason
// All Rights Reserved

#include "bitbucket.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void bitbucket_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	void *userdata = fuse_req_userdata(req);
	bitbucket_userdata_t *BBud = (bitbucket_userdata_t *)userdata;
	bitbucket_inode_t *inode = NULL;
	int status = 0;

	// TODO: should we be doing anything with the flags in fi->flags?

	CHECK_BITBUCKET_USER_DATA_MAGIC(BBud);

	if (FUSE_ROOT_ID == ino) {
		inode = BBud->RootDirectory;
		BitbucketReferenceInode(inode, INODE_LOOKUP_REFERENCE); // must have a lookup ref.
	}
	else {
		inode = BitbucketLookupInodeInTable(BBud->InodeTable, ino); // returns a lookup ref.	
	}

	if (NULL == inode) {
		status = ENOENT;
	}

	while (NULL != inode){
		if (!S_ISDIR(inode->Attributes.st_mode)) {
			// This inode is not a directory inode
			status = ENOTDIR;
		}

		break;
	}

	if (NULL != inode) {
		BitbucketReferenceInode(inode, INODE_FUSE_OPENDIR_REFERENCE);   // matches release call
		BitbucketDereferenceInode(inode, INODE_LOOKUP_REFERENCE);
		inode = NULL;
	}

	if (0 != status) {
		fuse_reply_err(req, status);
	}
	else {
		fi->fh = (uint64_t)inode;
		fi->cache_readdir = 1; // permit caching
		fuse_reply_open(req, fi);
	}

}
