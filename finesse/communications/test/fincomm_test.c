/*
 * Copyright (c) 2017-2020, Tony Mason. All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <uuid/uuid.h>
#include <pthread.h>
#include "munit.h"
#include <errno.h>
#include <finesse.h>
#include "finesse_test.h"


#if !defined(__notused)
#define __notused __attribute__((unused))
#endif // 

static MunitResult
test_server_connect(
    const MunitParameter params[] __notused,
    void *prv __notused)
{
    int status;
    finesse_server_handle_t fsh;

    status = FinesseStartServerConnection(&fsh);
    munit_assert(0 == status);

    status = FinesseStopServerConnection(fsh);
    munit_assert(0 == status);

    return MUNIT_OK;
}

static MunitResult
test_client_connect(
    const MunitParameter params[] __notused,
    void *prv __notused)
{
    int status;
    finesse_server_handle_t fsh;
    finesse_client_handle_t fch;

    status = FinesseStartServerConnection(&fsh);
    munit_assert(0 == status);

    status = FinesseStartClientConnection(&fch);
    munit_assert(0 == status);

    status = FinesseStopClientConnection(fch);
    munit_assert(0 == status);

    status = FinesseStopServerConnection(fsh);
    munit_assert(0 == status);

    return MUNIT_OK;
}

static MunitResult
test_client_connect_without_server(
    const MunitParameter params[] __notused,
    void *prv __notused)
{
    int status;
    finesse_client_handle_t fch;

    status = FinesseStartClientConnection(&fch);
    munit_assert(0 != status);

    return MUNIT_OK;
}

static MunitTest fincomm_tests[] = {
    TEST((char *)(uintptr_t)"/null", test_null, NULL),
    TEST(NULL, NULL, NULL),
};


const MunitSuite fincomm_suite = {
    .prefix = (char *)(uintptr_t)"/fincomm",
    .tests = fincomm_tests,
    .suites = NULL,
    .iterations = 1,
    .options = MUNIT_SUITE_OPTION_NONE,
};

#if 0
int
main(
    int argc,
    char **argv)
{
    static MunitTest fincomm_tests[] = {
        TEST((char *)(uintptr_t)"/null", test_null, NULL),
    	TEST(NULL, NULL, NULL),
    };

    static const MunitSuite suite = {
        .prefix = (char *)(uintptr_t)"/finesse",
        .tests = finesse_tests,
        .suites = NULL,
        .iterations = 1,
        .options = MUNIT_SUITE_OPTION_NONE,
    };

    static const MunitSuite fincomm_suite = {
        .prefix = (char *)(uintptr_t)"/fincomm",
        .tests = fincomm_tests,
        .suites = NULL,
        .iterations = 1,
        .options = MUNIT_SUITE_OPTION_NONE,
    };

    int status;

    status = munit_suite_main(&suite, NULL, argc, argv);

    if (0 != status) {
        exit(0);
    }

    status = munit_suite_main(&fincomm_suite, NULL, argc, argv);

    return status;
}
#endif // 0

/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
