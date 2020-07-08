/**
 * CUnit test of the CIC filter.
 *
 * This test creates a square wave for the tests. The fist test interpolates
 * the it. The resulting square wave is checked on the edge. The second test
 * decimates it back to the original sample rate. The result is checked again.
 *
 * Author: Sven Krauß
 */

#include "cic_filter.h"


#include <CUnit/CUnit.h>
#include <CUnit/Automated.h>
#include <CUnit/Basic.h>

#define R           (5)
#define N           (4)
#define M           (2)

#define SA          (1000000)
#define SB          (SA * R)

//We use an odd number to test the correct behavior on multiple calls.
#define BLOCK_LEN   (89)

#define TEST_VALUE  (6431)

static cic_t *input;
static cic_t *output;
static cic_t *buffer;

static int cic_test_init(void){

    input = malloc(sizeof(cic_t) * SA);
    output = malloc(sizeof(cic_t) * SB);
    buffer = malloc(sizeof(cic_t) * BLOCK_LEN * R);

    if (input == NULL)
        return -1;
    if (output == NULL)
        return -1;
    if (buffer == NULL)
        return -1;

    // Here we generate a square wave signal with a period of 128
    for (size_t i = 0; i < SA; i++){
        if ((i & 64) == 0)
            input[i] = TEST_VALUE;
        else
            input[i] = -TEST_VALUE;
    }

    return 0;
}

static int cic_test_cleanup(void){

    free(input);
    free(output);
    free(buffer);

    return 0;
}

static void cic_test_interpolator(void) {

    struct cic_filter filter;

    CU_ASSERT_EQUAL_FATAL(cic_filter_init(&filter, N, M, R, NULL, true), 0);

    size_t samples = SA;
    size_t ipos = 0;
    size_t opos = 0;
    while(samples > 0){
        size_t to_process = samples;
        if (to_process > BLOCK_LEN)
            to_process = BLOCK_LEN;

        memcpy(buffer, &input[ipos], to_process * sizeof(cic_t));
        size_t rlen = cic_filter_interpolate(&filter, buffer, BLOCK_LEN * R, to_process);
        CU_ASSERT_EQUAL_FATAL(rlen, to_process * R);
        memcpy(&output[opos], buffer, rlen * sizeof(cic_t));

        samples -= to_process;
        ipos += to_process;
        opos += rlen;
    }

    // Check the validity of the up-sampled data
    for (size_t i = 1; i < SB; i++){
        const cic_t v = output[i];
        const cic_t pv = output[i - 1];
        CU_ASSERT_TRUE(v <= TEST_VALUE);
        CU_ASSERT_TRUE(v >= -TEST_VALUE);
        // Detect the square wave edges
        if ((i % (64 * R)) != 0)
            continue;
        size_t id = i / R;
        if (((id & 64) == 0)){
            CU_ASSERT_TRUE(v > -TEST_VALUE);
            CU_ASSERT_EQUAL(pv, -TEST_VALUE);
        }else{
            CU_ASSERT_TRUE(v < TEST_VALUE);
            CU_ASSERT_EQUAL(pv, TEST_VALUE);
        }
    }

    cic_filter_free(&filter);
}

static void cic_test_decimator(void) {

    struct cic_filter filter;

    CU_ASSERT_EQUAL_FATAL(cic_filter_init(&filter, N, M, R, NULL, true), 0);

    size_t samples = SB;
    size_t ipos = 0;
    size_t opos = 0;
    while(samples > 0){
        size_t to_process = samples;
        if (to_process > BLOCK_LEN)
            to_process = BLOCK_LEN;

        memcpy(buffer, &output[ipos], to_process * sizeof(cic_t));
        size_t rlen = cic_filter_decimate(&filter, buffer, to_process);
        CU_ASSERT_TRUE_FATAL(rlen <= ((to_process / R) + 1));
        memcpy(&output[opos], buffer, rlen * sizeof(cic_t));

        samples -= to_process;
        ipos += to_process;
        opos += rlen;
    }

    // Check the validity of the up-sampled data
    for (size_t i = 1; i < SA; i++){
        const cic_t v = output[i];
        const cic_t pv = output[i - 1];
        CU_ASSERT_TRUE(v <= TEST_VALUE);
        CU_ASSERT_TRUE(v >= -TEST_VALUE);
        // Detect the square wave edges
        if ((i % 64) != 0)
            continue;
        if (((i & 64) == 0)){
            CU_ASSERT_TRUE(v > -TEST_VALUE);
            CU_ASSERT_EQUAL(pv, -TEST_VALUE);
        }else{
            CU_ASSERT_TRUE(v < TEST_VALUE);
            CU_ASSERT_EQUAL(pv, TEST_VALUE);
        }
    }

    cic_filter_free(&filter);
}



int main(int argc, char *argv[]) {
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* add a suite to the registry */
    pSuite = CU_add_suite("CIC Test", cic_test_init, cic_test_cleanup);
    if (NULL == pSuite)
        goto out;

    if (CU_add_test(pSuite, "Interpolator", cic_test_interpolator) == NULL)
        goto out;

    if (CU_add_test(pSuite, "Decimator", cic_test_decimator) == NULL)
        goto out;

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

out:
    CU_cleanup_registry();

    return CU_get_error();
}
