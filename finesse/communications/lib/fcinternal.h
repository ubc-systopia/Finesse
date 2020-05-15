//
// (C) Copyright 2020 Tony Mason
// All Rights Reserved
//
#define _GNU_SOURCE

#if !defined(__FCINTERNAL_H__)
#define __FCINTERNAL_H__ (1)

#include <finesse.h>

fincomm_shared_memory_region *FcGetSharedMemoryRegion(finesse_server_handle_t ServerHandle, unsigned Index);
int FinesseSendRequest(finesse_client_handle_t FinesseClientHandle, fincomm_message Request, size_t RequestLen);
int FinesseGetClientResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message *Response, size_t *ResponseLen);
void FinesseFreeClientResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Response);

#endif // __FCINTERNAL_H__