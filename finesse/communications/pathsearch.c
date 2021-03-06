
/*
 * (C) Copyright 2020 Tony Mason
 * All Rights Reserved
*/

#include <fcinternal.h>

int FinesseSendPathSearchRequest(finesse_client_handle_t FinesseClientHandle, char **Files, char **Paths, fincomm_message *Message)
{
    int status = ENOTSUP;
    (void) FinesseClientHandle;
    (void) Files;
    (void) Paths;
    *Message = NULL;
#if 0
    client_connection_state_t *ccs = FinesseClientHandle;
    fincomm_shared_memory_region *fsmr;
    fincomm_message message = NULL;
    finesse_msg *fmsg = NULL;
    size_t nameLength, bufSize;

    assert(NULL != ccs);
    fsmr = (fincomm_shared_memory_region *)ccs->server_shm;
    assert(NULL != fsmr);
    message = FinesseGetRequestBuffer(fsmr);
    assert(NULL != message);
    message->MessageType = FINESSE_REQUEST;
    fmsg = (finesse_msg *)message->Data;
    fmsg->Version = FINESSE_MESSAGE_VERSION;
    fmsg->MessageClass = FINESSE_FUSE_MESSAGE;
    fmsg->Message.Fuse.Request.Type = FINESSE_FUSE_REQ_UNLINK;
    memcpy(&fmsg->Message.Fuse.Request.Parameters.Unlink.Parent, Parent, sizeof(uuid_t)); // at least for now, we only support 
    assert(NULL != NameToUnlink);
    nameLength = strlen(NameToUnlink);
    bufSize = SHM_PAGE_SIZE - offsetof(finesse_msg, Message.Fuse.Request.Parameters.Unlink.Name);
    assert(nameLength < bufSize);
    memcpy(fmsg->Message.Fuse.Request.Parameters.Unlink.Name, NameToUnlink, nameLength+1);

    status = FinesseRequestReady(fsmr, message);
    assert(0 != status); // invalid request ID
    *Message = message;
    status = 0;
#endif // 0

    return status;
}

int FinesseSendPathSearchResponse(finesse_server_handle_t FinesseServerHandle, void *Client, fincomm_message Message, char *Path, int Result)
{
    int status = ENOTSUP;
    (void) FinesseServerHandle;
    (void) Client;
    (void) Message;
    (void) Path;
    (void) Result;
#if 0
    fincomm_shared_memory_region *fsmr = NULL;
    finesse_msg *ffm;
    unsigned index = (unsigned)(uintptr_t)Client;

    fsmr = FcGetSharedMemoryRegion(FinesseServerHandle, index);
    assert(NULL != fsmr);
    assert (index < SHM_MESSAGE_COUNT);
    assert(0 != Message);
    assert(FINESSE_REQUEST == Message->MessageType);
    
    Message->Result = Result;
    Message->MessageType = FINESSE_RESPONSE;
    ffm = (finesse_msg *)Message->Data;
    memset(ffm, 0, sizeof(finesse_msg)); // not necessary for production
    ffm->Version = FINESSE_MESSAGE_VERSION;
    ffm->MessageClass = FINESSE_FUSE_MESSAGE;
    ffm->Message.Fuse.Response.Type = FINESSE_FUSE_RSP_ERR;
    ffm->Message.Fuse.Response.Parameters.ReplyErr.Err = Result;

    FinesseResponseReady(fsmr, Message, 0);
#endif
    return status;
}

int FinesseGetPathSearchResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Message, char **Path)
{
    int status = ENOTSUP;
    (void) FinesseClientHandle;
    (void) Message;
    (void) Path;
#if 0
    client_connection_state_t *ccs = FinesseClientHandle;
    fincomm_shared_memory_region *fsmr = NULL;
    finesse_msg *fmsg = NULL;

    assert(NULL != ccs);
    fsmr = (fincomm_shared_memory_region *)ccs->server_shm;
    assert(NULL != fsmr);
    assert(0 != Message);

    // This is a blocking get
    status = FinesseGetResponse(fsmr, Message, 1);
    assert(0 != status);
    status = 0; // FinesseGetResponse is a boolean return function

    assert(FINESSE_RESPONSE == Message->MessageType);
    fmsg = (finesse_msg *)Message->Data;
    assert(FINESSE_MESSAGE_VERSION == fmsg->Version);
    assert(FINESSE_FUSE_MESSAGE == fmsg->MessageClass);
    assert(FINESSE_FUSE_RSP_ERR == fmsg->Message.Fuse.Response.Type);
#endif
    return status;

}

void FinesseFreePathSearchResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Response)
{
    // TODO: this may need to do secondary shm cleanup
    FinesseFreeClientResponse(FinesseClientHandle, Response);
}

/*
int FinesseSendPathSearchRequest(finesse_client_handle_t FinesseClientHandle, char **Files, char **Paths, fincomm_message *Message);
int FinesseSendPathSearchResponse(finesse_server_handle_t FinesseServerHandle, void *Client, fincomm_message Message, char *Path, int Result);
int FinesseGetPathSearchResponse(finesse_client_handle_t FinesseClientHandle, fincomm_message Message, char **Path);
void FinesseFreePathSearchResponse(finesse_client_handle_t FinesseClientHandle, char *PathToFree);
*/















