/*
 * Copyright (C) 2013 Argonne National Laboratory, Department of Energy,
 *                    UChicago Argonne, LLC and The HDF Group.
 * All rights reserved.
 *
 * The full copyright notice, including terms governing use, modification,
 * and redistribution, is contained in the COPYING file that can be
 * found at the root of the source code distribution tree.
 */

#include "mercury_test.h"
#include "na_test.h"

#include "mercury_time.h"

#include <stdio.h>
#include <stdlib.h>

#define RPC_SKIP 20
#define BULK_SKIP 20

extern int na_test_comm_rank_g;
extern int na_test_comm_size_g;

extern hg_id_t hg_test_scale_open_id_g;
extern hg_id_t hg_test_scale_write_id_g;

/**
 *
 */
static int
measure_rpc(na_addr_t addr)
{
    rpc_open_in_t rpc_open_in_struct;
    rpc_open_out_t rpc_open_out_struct;
    hg_request_t rpc_open_request;
    hg_status_t rpc_open_status;

    hg_const_string_t rpc_open_path = MERCURY_TESTING_TEMP_DIRECTORY "/test.h5";
    rpc_handle_t rpc_open_handle;

    int avg_iter;
    double time_read = 0, min_time_read = 0, max_time_read = 0;
    double calls_per_sec, min_calls_per_sec, max_calls_per_sec;

    int hg_ret;
    size_t i;

    if (na_test_comm_rank_g == 0) {
        printf("# Executing RPC with %d client(s) -- loop %d time(s)\n",
                na_test_comm_size_g, MERCURY_TESTING_MAX_LOOP);
    }

    /* Fill input structure */
    rpc_open_handle.cookie = 12345;
    rpc_open_in_struct.path = rpc_open_path;
    rpc_open_in_struct.handle = rpc_open_handle;

    if (na_test_comm_rank_g == 0) printf("# Warming up...\n");

    /* Warm up for RPC */
    for (i = 0; i < RPC_SKIP; i++) {
        hg_ret = HG_Forward(addr, hg_test_scale_open_id_g,
                &rpc_open_in_struct, &rpc_open_out_struct, &rpc_open_request);
        if (hg_ret != HG_SUCCESS) {
            fprintf(stderr, "Could not forward call\n");
            return HG_FAIL;
        }

        hg_ret = HG_Wait(rpc_open_request, HG_MAX_IDLE_TIME, &rpc_open_status);
        if (hg_ret != HG_SUCCESS) {
            fprintf(stderr, "Error during wait\n");
            return HG_FAIL;
        }

        hg_ret = HG_Request_free(rpc_open_request);
        if (hg_ret != HG_SUCCESS) {
            fprintf(stderr, "Could not free request\n");
            return HG_FAIL;
        }
    }

    NA_Test_barrier();

    if (na_test_comm_rank_g == 0) printf("%*s%*s%*s%*s%*s%*s",
            10, "# Time (s)", 10, "Min (s)", 10, "Max (s)",
            12, "Calls (c/s)", 12, "Min (c/s)", 12, "Max (c/s)");
    if (na_test_comm_rank_g == 0) printf("\n");

    /* RPC benchmark */
    for (avg_iter = 0; avg_iter < MERCURY_TESTING_MAX_LOOP; avg_iter++) {
        hg_time_t t1, t2;
        double td;

        hg_time_get_current(&t1);

        /* Forward call to remote addr and get a new request */
        hg_ret = HG_Forward(addr, hg_test_scale_open_id_g,
                &rpc_open_in_struct, &rpc_open_out_struct, &rpc_open_request);
        if (hg_ret != HG_SUCCESS) {
            fprintf(stderr, "Could not forward call\n");
            return HG_FAIL;
        }

        /* Wait for call to be executed and return value to be sent back
         * (Request is freed when the call completes)
         */
        hg_ret = HG_Wait(rpc_open_request, HG_MAX_IDLE_TIME, &rpc_open_status);
        if (hg_ret != HG_SUCCESS) {
            fprintf(stderr, "Error during wait\n");
            return HG_FAIL;
        }
        if (!rpc_open_status) {
            fprintf(stderr, "Operation did not complete\n");
            return HG_FAIL;
        } else {
            /* printf("Call completed\n"); */
        }

        /* Free request */
        hg_ret = HG_Request_free(rpc_open_request);
        if (hg_ret != HG_SUCCESS) {
            fprintf(stderr, "Could not free request\n");
            return HG_FAIL;
        }

        NA_Test_barrier();

        hg_time_get_current(&t2);
        td = hg_time_to_double(hg_time_subtract(t2, t1));

        time_read += td;
        if (!min_time_read) min_time_read = time_read;
        min_time_read = (td < min_time_read) ? td : min_time_read;
        max_time_read = (td > max_time_read) ? td : max_time_read;
    }

    time_read = time_read / MERCURY_TESTING_MAX_LOOP;
    calls_per_sec = na_test_comm_size_g / time_read;
    min_calls_per_sec = na_test_comm_size_g / max_time_read;
    max_calls_per_sec = na_test_comm_size_g / min_time_read;

    /* At this point we have received everything so work out the bandwidth */
    printf("%*f%*f%*f%*.*f%*.*f%*.*f\n",
            10, time_read, 10, min_time_read, 10, max_time_read,
            12, 2, calls_per_sec, 12, 2, min_calls_per_sec, 12, 2,
            max_calls_per_sec);

    return HG_SUCCESS;
}

/**
 *
 */
static int
measure_bulk_transfer(na_addr_t addr)
{
    bulk_write_in_t bulk_write_in_struct;
    bulk_write_out_t bulk_write_out_struct;
    hg_request_t bulk_write_request;
    hg_status_t bulk_write_status;

    int fildes = 12345;
    int *bulk_buf;
    void *buf_ptr[1];
    size_t bulk_size = 1024 * 1024 * MERCURY_TESTING_BUFFER_SIZE / sizeof(int);
    hg_bulk_t bulk_handle = HG_BULK_NULL;
    size_t bulk_write_ret = 0;
    size_t nbytes;
    double nmbytes;

    int avg_iter;
    double time_read = 0, min_time_read = 0, max_time_read = 0;
    double read_bandwidth, min_read_bandwidth, max_read_bandwidth;

    int hg_ret;
    size_t i;

    /* Prepare bulk_buf */
    nbytes = bulk_size * sizeof(int);
    nmbytes = (double) nbytes / (1024 * 1024);
    if (na_test_comm_rank_g == 0) {
        printf("# Reading Bulk Data (%f MB) with %d client(s) -- loop %d time(s)\n",
                nmbytes, na_test_comm_size_g, MERCURY_TESTING_MAX_LOOP);
    }

    bulk_buf = (int*) malloc(nbytes);
    for (i = 0; i < bulk_size; i++) {
        bulk_buf[i] = (int) i;
    }
    *buf_ptr = bulk_buf,

    /* Register memory */
    hg_ret = HG_Bulk_handle_create(1, buf_ptr, &nbytes, HG_BULK_READ_ONLY,
            &bulk_handle);
    if (hg_ret != HG_SUCCESS) {
        fprintf(stderr, "Could not create bulk data handle\n");
        return HG_FAIL;
    }

    /* Fill input structure */
    bulk_write_in_struct.fildes = fildes;
    bulk_write_in_struct.bulk_handle = bulk_handle;

    if (na_test_comm_rank_g == 0) printf("# Warming up...\n");

    /* Warm up for bulk data */
    for (i = 0; i < BULK_SKIP; i++) {
        hg_ret = HG_Forward(addr, hg_test_scale_write_id_g,
                &bulk_write_in_struct, &bulk_write_out_struct, &bulk_write_request);
        if (hg_ret != HG_SUCCESS) {
            fprintf(stderr, "Could not forward call\n");
            return HG_FAIL;
        }

        hg_ret = HG_Wait(bulk_write_request, HG_MAX_IDLE_TIME, &bulk_write_status);
        if (hg_ret != HG_SUCCESS) {
            fprintf(stderr, "Error during wait\n");
            return HG_FAIL;
        }


        /* Free request */
        hg_ret = HG_Request_free(bulk_write_request);
        if (hg_ret != HG_SUCCESS) {
            fprintf(stderr, "Could not free request\n");
            return HG_FAIL;
        }
    }

    NA_Test_barrier();

    if (na_test_comm_rank_g == 0) printf("%*s%*s%*s%*s%*s%*s",
            10, "# Time (s)", 10, "Min (s)", 10, "Max (s)",
            12, "BW (MB/s)", 12, "Min (MB/s)", 12, "Max (MB/s)");
    if (na_test_comm_rank_g == 0) printf("\n");

    /* Bulk data benchmark */
    for (avg_iter = 0; avg_iter < MERCURY_TESTING_MAX_LOOP; avg_iter++) {
        hg_time_t t1, t2;
        double td;

        hg_time_get_current(&t1);

        /* Forward call to remote addr and get a new request */
        hg_ret = HG_Forward(addr, hg_test_scale_write_id_g,
                &bulk_write_in_struct, &bulk_write_out_struct, &bulk_write_request);
        if (hg_ret != HG_SUCCESS) {
            fprintf(stderr, "Could not forward call\n");
            return HG_FAIL;
        }

        /* Wait for call to be executed and return value to be sent back
         * (Request is freed when the call completes)
         */
        hg_ret = HG_Wait(bulk_write_request, HG_MAX_IDLE_TIME, &bulk_write_status);
        if (hg_ret != HG_SUCCESS) {
            fprintf(stderr, "Error during wait\n");
            return HG_FAIL;
        }
        if (!bulk_write_status) {
            fprintf(stderr, "Operation did not complete\n");
            return HG_FAIL;
        } else {
            /* printf("Call completed\n"); */
        }

        /* Get output parameters */
        bulk_write_ret = bulk_write_out_struct.ret;
        if (bulk_write_ret != (bulk_size * sizeof(int))) {
            fprintf(stderr, "Data not correctly processed\n");
        }

        /* Free request */
        hg_ret = HG_Request_free(bulk_write_request);
        if (hg_ret != HG_SUCCESS) {
            fprintf(stderr, "Could not free request\n");
            return HG_FAIL;
        }

        NA_Test_barrier();

        hg_time_get_current(&t2);
        td = hg_time_to_double(hg_time_subtract(t2, t1));

        time_read += td;
        if (!min_time_read) min_time_read = time_read;
        min_time_read = (td < min_time_read) ? td : min_time_read;
        max_time_read = (td > max_time_read) ? td : max_time_read;
    }

    time_read = time_read / MERCURY_TESTING_MAX_LOOP;
    read_bandwidth = nmbytes * na_test_comm_size_g / time_read;
    min_read_bandwidth = nmbytes * na_test_comm_size_g / max_time_read;
    max_read_bandwidth = nmbytes * na_test_comm_size_g / min_time_read;

    /* At this point we have received everything so work out the bandwidth */
    printf("%*f%*f%*f%*.*f%*.*f%*.*f\n",
            10, time_read, 10, min_time_read, 10, max_time_read,
            12, 2, read_bandwidth, 12, 2, min_read_bandwidth, 12, 2,
            max_read_bandwidth);

    /* Free memory handle */
    hg_ret = HG_Bulk_handle_free(bulk_handle);
    if (hg_ret != HG_SUCCESS) {
        fprintf(stderr, "Could not free bulk data handle\n");
        return HG_FAIL;
    }
    free(bulk_buf);

    return HG_SUCCESS;
}

/*****************************************************************************/
int
main(int argc, char *argv[])
{
    na_addr_t addr;

    HG_Test_client_init(argc, argv, &addr, &na_test_comm_rank_g);

    /* Run RPC test */
    measure_rpc(addr);

    NA_Test_barrier();

    /* Run Bulk test */
    measure_bulk_transfer(addr);

    HG_Test_finalize();

    return EXIT_SUCCESS;
}
