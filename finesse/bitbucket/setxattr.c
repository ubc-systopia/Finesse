//
// (C) Copyright 2020
// Tony Mason
// All Rights Reserved

#include "bitbucket.h"

void bitbucket_setxattr(fuse_req_t req, fuse_ino_t ino, const char *name, const char *value, size_t size, int flags)
{
	(void) req;
	(void) ino;
	(void) name;
	(void) value;
	(void) size;
	(void) flags;

	assert(0); // Not implemented
}
