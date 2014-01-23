# - Try to find the RDMA libraries
# Once done this will define
#
# RDMA_FOUND - system has RDMA libraries
# RDMA_INCLUDE_DIRS - the RDMA include directory
# RDMA_LIBRARIES - RDMA libraries

MESSAGE(STATUS "Looking for RDMA...")

FIND_PATH(RDMA_CM_INCLUDE_DIR rdma_cma.h PATHS
        /usr/include/
        /usr/local/include/
        PATH_SUFFIXES "rdma"
        )

FIND_PATH(RDMA_VERBS_INCLUDE_DIR ib_user_verbs.h PATHS
        /usr/include/
        /usr/local/include/
        PATH_SUFFIXES "infiniband" "rdma"
        )

FIND_LIBRARY(RDMA_LIBRARIES NAMES rdmacm ibverbs)

IF(RDMA_CM_INCLUDE_DIR AND RDMA_VERBS_INCLUDE_DIR AND RDMA_LIBRARIES)
    SET(RDMA_FOUND 1)
  
    STRING(REGEX REPLACE "/rdma$" "" RDMA_CM_INCLUDE_DIR_SUP ${RDMA_CM_INCLUDE_DIR})
    SET(RDMA_CM_INCLUDE_DIR ${RDMA_CM_INCLUDE_DIR_SUP} CACHE PATH "RDMA CM header directory" FORCE)
    
    STRING(REGEX REPLACE "/infiniband$" "" RDMA_VERBS_INCLUDE_DIR_SUP ${RDMA_VERBS_INCLUDE_DIR})
    SET(RDMA_VERBS_INCLUDE_DIR ${RDMA_VERBS_INCLUDE_DIR_SUP} CACHE PATH "RDMA Verbs header directory" FORCE)
  
    IF(NOT RDMA_FIND_QUIETLY)
        MESSAGE(STATUS "Found RDMA: ${RDMA_LIBRARIES}")
    ENDIF(NOT RDMA_FIND_QUIETLY)
ELSE()
    SET(RDMA_FOUND 0 CACHE BOOL "RDMA library not found")
    IF(RDMA_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could NOT find RDMA, error")
    ELSE()
        MESSAGE(STATUS "Could NOT find RDMA, disabled")
    ENDIF()
ENDIF()

SET(RDMA_INCLUDE_DIRS ${RDMA_CM_INCLUDE_DIR} ${RDMA_VERBS_INCLUDE_DIR})

MARK_AS_ADVANCED(RDMA_CM_INCLUDE_DIR RDMA_VERBS_INCLUDE_DIR RDMA_LIBRARIES)
