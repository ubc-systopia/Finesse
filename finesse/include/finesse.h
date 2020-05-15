/*
 * (C) Copyright 2017-2020 Tony Mason
 * All Rights Reserved
*/

#include <fincomm.h>
#include <uuid/uuid.h>
#include <stdint.h>
#include <sys/statvfs.h>

typedef void *finesse_server_handle_t;
typedef void *finesse_client_handle_t;
typedef uint64_t fuse_ino_t;

int FinesseStartServerConnection(finesse_server_handle_t *FinesseServerHandle);
int FinesseStopServerConnection(finesse_server_handle_t FinesseServerHandle);
int FinesseGetRequest(finesse_server_handle_t FinesseServerHandle, void **Client,  fincomm_message *Request);
int FinesseSendResponse(finesse_server_handle_t FinesseServerHandle, void *Client, void *Response);

int FinesseStartClientConnection(finesse_client_handle_t *FinesseClientHandle);
int FinesseStopClientConnection(finesse_client_handle_t FinesseClientHandle);

int FinesseSendTestRequest(finesse_client_handle_t FinesseClientHandle, fincomm_message *Message);
int FinesseSendTestResponse(finesse_server_handle_t FinesseServerHandle, void *Client, fincomm_message Message, int Result);
int FinesseGetTestResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Message);
void FinesseFreeTestResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Response);

int FinesseSendNameMapRequest(finesse_client_handle_t FinesseClientHandle, char *NameToMap, fincomm_message *Message);
int FinesseSendNameMapResponse(finesse_server_handle_t FinesseServerHandle, void *Client, fincomm_message Message, uuid_t *MapKey, int Result);
int FinesseGetNameMapResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Message, uuid_t *MapKey);
void FinesseFreeNameMapResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Response);

int FinesseSendNameMapReleaseRequest(finesse_client_handle_t FinesseClientHandle, uuid_t *MapKey, fincomm_message *Message);
int FinesseSendNameMapReleaseResponse(finesse_server_handle_t FinesseServerHandle, void *Client, fincomm_message Message, int Result);
int FinesseGetNameMapReleaseResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Message);
void FinesseFreeNameMapReleaseResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Response);

int FinesseSendPathSearchRequest(finesse_client_handle_t FinesseClientHandle, char **Files, char **Paths, fincomm_message *Message);
int FinesseSendPathSearchResponse(finesse_server_handle_t FinesseServerHandle, void *Client, fincomm_message Message, char *Path, int Result);
int FinesseGetPathSearchResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Message, char **Path);
void FinesseFreePathSearchResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Response);

int FinesseSendDirMapRequest(finesse_client_handle_t FinesseClientHandle, uuid_t *Key, char *Path, fincomm_message *Message);
int FinesseSendDirMapResponse(finesse_server_handle_t FinesseServerHandle, void *Client, fincomm_message Message, char *Path, int Result);
int FinesseGetDirMapResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Message);
void FinesseFreeDirMapResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Response);

int FinesseSendUnlinkRequest(finesse_client_handle_t FinesseClientHandle, uuid_t Parent, const char *NameToUnlink, fincomm_message *Message);
int FinesseSendUnlinkResponse(finesse_server_handle_t FinesseServerHandle, void *Client, fincomm_message Message, int64_t Result);
int FinesseGetUnlinkResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Message);
void FinesseFreeUnlinkMapResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Response);

int FinesseSendStatfsRequest(finesse_client_handle_t FinesseClientHandle, const char *path, fincomm_message *Message);
int FinesseSendStatfsResponse(finesse_server_handle_t FinesseServerHandle, void *Client, fincomm_message Message, struct statvfs *buf, int Result);
int FinesseGetStatfsResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Message, struct statvfs *buf);
void FinesseFreeStatfsResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Response);

int FinesseSendFstatfsRequest(finesse_client_handle_t FinesseClientHandle, uuid_t *Inode, fincomm_message *Message);
int FinesseSendFstatfsResponse(finesse_server_handle_t FinesseServerHandle, void *Client, fincomm_message Message, struct statvfs *buf, int Result);
int FinesseGetFstatfsResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Message, struct statvfs *buf);
void FinesseFreeFstatfsResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Response);


void (*finesse_init)(void);
int finesse_check_prefix(const char *pathname);
int finesse_open(const char *pathname, int flags, ...);
int finesse_creat(const char *pathname, mode_t mode);
int finesse_openat(int dirfd, const char *pathname, int flags, ...);
int finesse_close(int fd);
int finesse_unlink(const char *pathname);
int finesse_unlinkat(int dirfd, const char *pathname, int flags);
int finesse_statfs(const char *path, struct statvfs *buf);
int finesse_fstatfs(fuse_ino_t nodeid, struct statvfs *buf);
//int finesse_mkdir(const char *path, mode_t mode);
//int finesse_mkdirat(int fd, const char *path, mode_t mode);
