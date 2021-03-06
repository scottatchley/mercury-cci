/*
 * Copyright (C) 2013 Argonne National Laboratory, Department of Energy,
 *                    UChicago Argonne, LLC and The HDF Group.
 * All rights reserved.
 *
 * The full copyright notice, including terms governing use, modification,
 * and redistribution, is contained in the COPYING file that can be
 * found at the root of the source code distribution tree.
 */

#ifndef MERCURY_H
#define MERCURY_H

#include "mercury_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get Mercury version number.
 *
 * \param major [OUT]           pointer to unsigned integer
 * \param minor [OUT]           pointer to unsigned integer
 * \param patch [OUT]           pointer to unsigned integer
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
HG_Version_get(unsigned int *major, unsigned int *minor, unsigned int *patch);

/**
 * Initialize the Mercury layer.
 * Calling HG_Init also calls HG_Bulk_init with the same NA class if
 * HG_Bulk_init has not been called before, this allows users to
 * eventually initialize the bulk interface with a different NA class.
 *
 * \param na_class [IN]    pointer to network class
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
HG_Init(na_class_t *na_class);

/**
 * Finalize the Mercury layer.
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
HG_Finalize(void);

/**
 * Indicate whether HG_Init has been called and return associated network class.
 *
 * \param flag [OUT]            pointer to boolean
 * \param na_class [OUT]        pointer to returned network class pointer
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
HG_Initialized(hg_bool_t *flag, na_class_t **na_class);

/**
 * Register a function name that can be sent using the RPC layer.
 *
 * \param func_name [IN]        unique name associated to function
 * \param in_proc_cb [IN]       pointer to input proc routine
 * \param out_proc_cb [IN]      pointer to output proc routine
 * \param rpc_cb [IN]           RPC callback (may only be defined in server code)
 *
 *
 * \return unique ID associated to the registered function
 */
HG_EXPORT hg_id_t
HG_Register(const char *func_name, hg_proc_cb_t in_proc_cb,
        hg_proc_cb_t out_proc_cb, hg_rpc_cb_t rpc_cb);

/**
 * Indicate whether HG_Register has been called and return associated ID.
 *
 * \param func_name [IN]        name associated to function
 * \param flag [OUT]            pointer to boolean
 * \param id [OUT]              pointer to ID
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
HG_Registered(const char *func_name, hg_bool_t *flag, hg_id_t *id);

/**
 * Forward a call to a remote server.
 * Request must be freed using HG_Request_free.
 *
 * \param addr [IN]             abstract network address of destination
 * \param id [IN]               registered function ID
 * \param in_struct [IN]        pointer to input structure
 * \param out_struct [OUT]      pointer to output structure
 * \param request [OUT]         pointer to RPC request
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
HG_Forward(na_addr_t addr, hg_id_t id,
        void *in_struct, void *out_struct, hg_request_t *request);

/**
 * Wait for an operation request to complete.
 * Once the request has completed, request must be freed using HG_Request_free.
 *
 * \param request [IN]          RPC request
 * \param timeout [IN]          timeout (in milliseconds)
 * \param status [OUT]          pointer to returned status
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
HG_Wait(hg_request_t request, unsigned int timeout, hg_status_t *status);

/**
 * Wait for all operations in array_of_requests to complete.
 *
 * \param count [IN]              number of RPC requests
 * \param array_of_requests [IN]  arrays of RPC requests
 * \param timeout [IN]            timeout (in milliseconds)
 * \param array_of_statuses [OUT] array of statuses
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
HG_Wait_all(int count, hg_request_t array_of_requests[],
        unsigned int timeout, hg_status_t array_of_statuses[]);

/**
 * Free request and resources allocated when decoding the output.
 * User must get output parameters contained in the output structure
 * before calling HG_Request_free.
 *
 * \param request [IN]          RPC request
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
HG_Request_free(hg_request_t request);

/**
 * Convert error return code to string (null terminated).
 *
 * \param errnum [IN]           error return code
 *
 * \return String
 */
HG_EXPORT const char *
HG_Error_to_string(hg_return_t errnum);

#ifdef __cplusplus
}
#endif

#endif /* MERCURY_H */
