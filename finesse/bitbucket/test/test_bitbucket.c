/*
 * Copyright (c) 2017-2020, Tony Mason. All rights reserved.
 */

#include "test_bitbucket.h"
#include <munit.h>


#if !defined(__notused)
#define __notused __attribute__((unused))
#endif // 

MunitResult
test_null(
    const MunitParameter params[] __notused,
    void *prv __notused)
{
    return MUNIT_OK;
}

extern MunitSuite *SetupMunitSuites(void);

int
main(
    int argc,
    char **argv)
{
    MunitSuite suite;

    suite.prefix = (char *)(uintptr_t)"/bitbucket";
    suite.tests = NULL;
    suite.suites = SetupMunitSuites();
    suite.iterations = 1;
    suite.options = MUNIT_SUITE_OPTION_NONE;   

    return munit_suite_main(&suite, NULL, argc, argv);
}


static MunitTest bitbucket_tests[] = {
    TEST((char *)(uintptr_t)"/null", test_null, NULL),
    TEST(NULL, NULL, NULL),
};


const MunitSuite bitbucket_suite = {
    .prefix = (char *)(uintptr_t)"/bitbucket",
    .tests = bitbucket_tests,
    .suites = NULL,
    .iterations = 1,
    .options = MUNIT_SUITE_OPTION_NONE,
};

static MunitSuite testbitbucket_suites[10];

MunitSuite *SetupMunitSuites()
{
    unsigned index = 0;

    memset(testbitbucket_suites, 0, sizeof(testbitbucket_suites));
    testbitbucket_suites[index++] = object_suite;
    testbitbucket_suites[index++] = inode_suite;
    testbitbucket_suites[index++] = trie_suite;
    testbitbucket_suites[index++] = dir_suite;
    return testbitbucket_suites;
}


/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */