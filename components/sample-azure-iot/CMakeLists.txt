# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(ROOT_PATH
    ${CMAKE_CURRENT_LIST_DIR}/../..
)

file(GLOB_RECURSE COMPONENT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/sample_azure_iot_pnp/*.c
)

idf_component_get_property(MBEDTLS_DIR mbedtls COMPONENT_DIR)

list(APPEND COMPONENT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/backoff_algorithm.c
    ${CMAKE_CURRENT_LIST_DIR}/transport_tls_esp32.c
    ${CMAKE_CURRENT_LIST_DIR}/crypto_esp32.c
)

set(COMPONENT_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/../../config
    ${CMAKE_CURRENT_LIST_DIR}
    ${MBEDTLS_DIR}/mbedtls/include
    ${CMAKE_CURRENT_LIST_DIR}/common/transport
    ${CMAKE_CURRENT_LIST_DIR}/common/utilities
    ${CMAKE_CURRENT_LIST_DIR}/common/connection
)

list(APPEND COMPONENT_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/sample_azure_iot_pnp
)

idf_component_register(
    SRCS ${COMPONENT_SOURCES}
    INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
    REQUIRES 
        mbedtls
        tcp_transport
        coreMQTT
        azure-sdk-for-c
        azure-iot-middleware-freertos
        lwip
        main)
