//
// (C) Copyright 2020 Tony Mason
// All Rights Reserved
//
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <aio.h>
#include <mqueue.h>
#include <stddef.h>
#include <pthread.h>
#include "fincomm.h"

#define FINESSE_SERVICE_NAME "Finesse-1.0"


//
// This is the common code for the finesse communications package, shared
// between client and server.
//

//
// Basic design:
//   Initial communications between the Finesse client and the Finesse server is done using
//   a UNIX domain socket; this allows the server to detect when the client has "gone away".
//
//   The primary communications is done via a shared memory region, which is indicated to
//   the server as part of the start-up message.
//
//   

int GenerateServerName(char *ServerName, size_t ServerNameLength)
{
    int status;

    status = snprintf(ServerName, ServerNameLength, "%s/%s", FINESSE_SERVICE_PREFIX, FINESSE_SERVICE_NAME);
    if (status >= ServerNameLength) {
        return EOVERFLOW;
    }
    return 0; // 0 == success
}

int GenerateClientSharedMemoryName(char *SharedMemoryName, size_t SharedMemoryNameLength, uuid_t ClientId)
{
    int status;
    char uuid_string[40];

    uuid_unparse(ClientId, uuid_string);
    status = snprintf(SharedMemoryName, SharedMemoryNameLength, "%s-%lu", uuid_string, (unsigned long)time(NULL));

    if (status >= SharedMemoryNameLength) {
        return EOVERFLOW;
    }
    return 0; // 0 == success;
}

static u_int64_t RequestNumber = (uint64_t)(-10); // start just below zero to ensure we wrap properly
static u_int64_t BufferAllocationBitmap;
static unsigned LastBufferAllocated;

static u_int64_t get_request_number(void)
{
    u_int64_t request_number = 0;

    // request_number 0 is not valid
    while (0 == request_number) {
        request_number = __sync_fetch_and_add(&RequestNumber, 1);
    }
    assert(0 == request_number);
    return request_number;
}

fincomm_message FinesseGetRequestBuffer(fincomm_shared_memory_region *RequestRegion)
{
    unsigned index;
    u_int64_t mask = 1;
    u_int64_t bitmap = BufferAllocationBitmap;
    u_int64_t new_bitmap = bitmap;

    index = (LastBufferAllocated + 1) % SHM_MESSAGE_COUNT;
    _Static_assert(64 == SHM_MESSAGE_COUNT, "Check bit mask length");
    new_bitmap |= (1 << index);

    if (!__sync_bool_compare_and_swap(&BufferAllocationBitmap, bitmap, new_bitmap)) {
        // This is the slow path, where we didn't get lucky, so we brute force scan
        for (index = 0; index < SHM_MESSAGE_COUNT; index++, mask = mask << 1) {
            bitmap = BufferAllocationBitmap;
            new_bitmap = bitmap | mask;
            if (__sync_bool_compare_and_swap(&BufferAllocationBitmap, bitmap, new_bitmap)) {
                // found our index
                break;
            }
        }
    }

    // TODO: make this blocking?
    // In either case, index indicates the allocated message buffer.  Out of range = alloc failure
    // TODO: should I initialize this region?
    return (index < SHM_MESSAGE_COUNT) ? &RequestRegion->Messages[index] : NULL;
}

u_int64_t FinesseRequestReady(fincomm_shared_memory_region *RequestRegion, fincomm_message Message)
{
    // So the message index can be computed
    unsigned index = (unsigned)((((uintptr_t)Message - (uintptr_t)RequestRegion)/SHM_PAGE_SIZE)-1);
    uint64_t request_id = get_request_number();
    assert(&RequestRegion->Messages[index] == Message);

    Message->RequestId = request_id;
    Message->Response = 0;

    pthread_mutex_lock(&RequestRegion->RequestMutex);
    assert(0 == (RequestRegion->RequestBitmap & (1<<index))); // this should NOT be set
    RequestRegion->RequestBitmap |= (1<<index);
    pthread_cond_signal(&RequestRegion->RequestPending);
    pthread_mutex_unlock(&RequestRegion->RequestMutex);

    return request_id;
}

void FinesseResponseReady(fincomm_shared_memory_region *RequestRegion, fincomm_message Message, uint32_t Response)
{
    unsigned index = (unsigned)((((uintptr_t)Message - (uintptr_t)RequestRegion)/SHM_PAGE_SIZE)-1);
    assert(&RequestRegion->Messages[index] == Message);

    pthread_mutex_lock(&RequestRegion->ResponseMutex);
    assert(0 == (RequestRegion->ResponseBitmap & (1<<index))); // this should NOT be set
    RequestRegion->ResponseBitmap |= (1<<index);
    Message->Response = Response;
    pthread_cond_broadcast(&RequestRegion->ResponsePending);
    pthread_mutex_unlock(&RequestRegion->ResponseMutex);
}

int FinesseGetResponse(fincomm_shared_memory_region *RequestRegion, fincomm_message Message, int wait)
{
    unsigned index = (unsigned)((((uintptr_t)Message - (uintptr_t)RequestRegion)/SHM_PAGE_SIZE)-1);
    int status = 0;

    assert(&RequestRegion->Messages[index] == Message);
    assert(NULL != RequestRegion);
    assert(NULL != Message);

    pthread_mutex_lock(&RequestRegion->ResponseMutex);
    if (!wait) {
        if (0 != (RequestRegion->ResponseBitmap & (1 << index))) {
            status = 1;
            RequestRegion->ResponseBitmap &= ~(1<<index); // clear the response pending bit
        }

    }
    else {
        while (0 == (RequestRegion->ResponseBitmap & (1 << index))) {
            pthread_cond_wait(&RequestRegion->ResponsePending, &RequestRegion->ResponseMutex);\
        }
        status = 1;
        RequestRegion->ResponseBitmap &= ~(1<<index); // clear the response pending bit
    }
    return status;
}

fincomm_message FinesseGetReadyRequest(fincomm_shared_memory_region *RequestRegion)
{
    unsigned int index = SHM_MESSAGE_COUNT; // invalid value
    uint64_t mask = 1;
    long int rnd = random() % SHM_MESSAGE_COUNT; 
    pthread_mutex_lock(&RequestRegion->RequestMutex);
    while (SHM_MESSAGE_COUNT == index) {
        while (0 == RequestRegion->RequestBitmap) {
            pthread_cond_wait(&RequestRegion->RequestPending, &RequestRegion->RequestMutex);
        }
        for (unsigned i = 0; i < rnd; i++, mask = mask << 1) {
            if (RequestRegion->RequestBitmap & mask) {
                // found one!
                RequestRegion->RequestBitmap &= ~mask; // clear the bit
                index = i; // note which one we found
                break;
            }
        }
        if (index < SHM_MESSAGE_COUNT) {
            break; // we already found one
        }
        for (unsigned i = rnd; i < SHM_MESSAGE_COUNT; i++, mask = mask << 1) {
            if (RequestRegion->RequestBitmap & mask) {
                // found one!
                RequestRegion->RequestBitmap &= ~mask; // clear the bit
                index = i; // note which one we found
                break;
            }
        }
        assert(SHM_MESSAGE_COUNT != index);
    }
    pthread_mutex_unlock(&RequestRegion->RequestMutex);
    return &RequestRegion->Messages[index];
}

void FinesseReleaseRequestBuffer(fincomm_shared_memory_region *RequestRegion, fincomm_message Message)
{
    unsigned index = (unsigned)((((uintptr_t)Message - (uintptr_t)RequestRegion)/SHM_PAGE_SIZE)-1);
    u_int64_t bitmap = BufferAllocationBitmap;
    u_int64_t new_bitmap = (bitmap & ~(1<<index));
    assert(&RequestRegion->Messages[index] == Message);

    while (!__sync_bool_compare_and_swap(&BufferAllocationBitmap, bitmap, new_bitmap)) {
        bitmap = BufferAllocationBitmap;
        new_bitmap = (bitmap & ~(1<<index));
    }
}
