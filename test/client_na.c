/*
 * client_na.c
 */

#include "iofsl_compat.h"
#include "network_mpi.h"
#include "network_bmi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    char *ion_name;
    na_addr_t ion_target = 0;

    na_tag_t send_tag = 100;
    na_tag_t recv_tag = 101;

    na_request_t send_request = NULL;
    na_request_t recv_request = NULL;

    char *send_buf = NULL;
    char *recv_buf = NULL;

    int *bulk_buf = NULL;
    int bulk_size = 1024*1024;
    na_mem_handle_t local_mem_handle = NULL;

    na_tag_t bulk_tag = 102;
    na_tag_t ack_tag = 103;

    na_request_t bulk_request = NULL;
    na_request_t ack_request = NULL;

    int i;

    /* Initialize the interface */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <BMI|MPI>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp("MPI", argv[1]) == 0) {
        FILE *config;
        na_mpi_init(NULL, 0);
        if ((config = fopen("port.cfg", "r")) != NULL) {
            char mpi_port_name[MPI_MAX_PORT_NAME];
            fread(mpi_port_name, sizeof(char), MPI_MAX_PORT_NAME, config);
            printf("Using MPI port name: %s.\n", mpi_port_name);
            fclose(config);
            setenv(ION_ENV, mpi_port_name, 1);
        }
    } else {
        na_bmi_init(NULL, NULL, 0);
    }
    ion_name = getenv(ION_ENV);
    if (!ion_name) {
        fprintf(stderr, "getenv(\"%s\") failed.\n", ION_ENV);
    }

    /* Perform an address lookup on the ION */
    na_addr_lookup(ion_name, &ion_target);

    /* Allocate send and recv bufs */
    send_buf = malloc(na_get_unexpected_size());
    recv_buf = malloc(na_get_unexpected_size());

    /* Send a message to addr */
    sprintf(send_buf, "Hello ION!\n");
    na_send_unexpected(send_buf, na_get_unexpected_size(), ion_target, send_tag, &send_request, NULL);
    na_recv(recv_buf, na_get_unexpected_size(), ion_target, recv_tag, &recv_request, NULL);

    na_wait(send_request, NA_MAX_IDLE_TIME, NA_STATUS_IGNORE);
    na_wait(recv_request, NA_MAX_IDLE_TIME, NA_STATUS_IGNORE);
    printf("Received from ION: %s\n", recv_buf);

    /* Prepare bulk_buf */
    bulk_buf = malloc(sizeof(int) * bulk_size);
    for (i = 0; i < bulk_size; i++) {
        bulk_buf[i] = i;
    }

    /* Register memory */
    printf("Registering local memory...\n");
    na_mem_register(bulk_buf, sizeof(int) * bulk_size, NA_MEM_TARGET_GET, &local_mem_handle);

    /* Serialize mem handle */
    printf("Serializing local memory handle...\n");
    na_mem_handle_serialize(send_buf, na_get_unexpected_size(), local_mem_handle);

    /* Send mem handle */
    printf("Sending local memory handle...\n");
    na_send(send_buf, na_get_unexpected_size(), ion_target, bulk_tag, &bulk_request, NULL);

    /* Recv completion ack */
    printf("Receiving end of transfer ack...\n");
    na_recv(recv_buf, na_get_unexpected_size(), ion_target, ack_tag, &ack_request, NULL);

    na_wait(bulk_request, NA_MAX_IDLE_TIME, NA_STATUS_IGNORE);
    na_wait(ack_request, NA_MAX_IDLE_TIME, NA_STATUS_IGNORE);

    printf("Finalizing...\n");

    /* Free memory and addresses */
    na_mem_deregister(local_mem_handle);
    free(bulk_buf);
    bulk_buf = NULL;

    na_addr_free(ion_target);
    ion_target = NULL;

    free(recv_buf);
    recv_buf = NULL;

    free(send_buf);
    send_buf = NULL;

    na_finalize();
    return EXIT_SUCCESS;
}
