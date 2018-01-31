/*
 * Copyright (c) 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <inttypes.h>

#define LOGMODULE test
#include "log/log.h"
#include "sapi-util.h"

TSS2_RC
test_invoke (TSS2_SYS_CONTEXT *sapi_context)
{
    TSS2_RC                 rc              = TSS2_RC_SUCCESS;
    TPM2B_SENSITIVE_CREATE  in_sensitive    = { 0 };
    TPM2B_PUBLIC            in_public       = { 0 };
    TPM2B_PRIVATE           out_private     = { 0 };
    TPM2B_PUBLIC            out_public      = { 0 };
    TPM2B_NAME              name            = TPM2B_NAME_INIT;
    TPM2B_NAME              qualified_name  = TPM2B_NAME_INIT;
    TPM2_HANDLE             object_handle   = 0;
    TSS2L_SYS_AUTH_COMMAND  auth_cmd = {
        .auths = {{ .sessionHandle = TPM2_RS_PW }},
        .count = 1
    };
    TSS2L_SYS_AUTH_RESPONSE auth_rsp = {
        .count = 0
    };

    if (sapi_context == NULL)
        return TSS2_RC_LAYER_MASK | TSS2_BASE_RC_BAD_REFERENCE;

    in_public.publicArea.type = TPM2_ALG_RSA;
    in_public.publicArea.nameAlg = TPM2_ALG_SHA256;
    in_public.publicArea.objectAttributes |= TPMA_OBJECT_RESTRICTED;
    in_public.publicArea.objectAttributes |= TPMA_OBJECT_USERWITHAUTH;
    in_public.publicArea.objectAttributes |= TPMA_OBJECT_DECRYPT;
    in_public.publicArea.objectAttributes |= TPMA_OBJECT_FIXEDTPM;
    in_public.publicArea.objectAttributes |= TPMA_OBJECT_FIXEDPARENT;
    in_public.publicArea.objectAttributes |= TPMA_OBJECT_SENSITIVEDATAORIGIN;
    in_public.publicArea.parameters.rsaDetail.symmetric.algorithm = TPM2_ALG_AES;
    in_public.publicArea.parameters.rsaDetail.symmetric.keyBits.aes = 128;
    in_public.publicArea.parameters.rsaDetail.symmetric.mode.aes = TPM2_ALG_CFB;
    in_public.publicArea.parameters.rsaDetail.scheme.scheme = TPM2_ALG_NULL;
    in_public.publicArea.parameters.rsaDetail.keyBits = 2048;

    /* Create an object using CreateLoaded.
     * The result should be that the created object
     * stays in the TPM
     */
    LOG_INFO("Calling CreateLoaded");
    rc = Tss2_Sys_CreateLoaded (sapi_context,
                                TPM2_RH_OWNER,
                                &auth_cmd,
                                &in_sensitive,
                                &in_public,
                                &object_handle,
                                &out_private,
                                &out_public,
                                &name,
                                &auth_rsp);
    if (rc == TPM2_RC_SUCCESS) {
        LOG_INFO("success object handle: 0x%x", object_handle);
    } else {
        LOG_ERROR("CreateLoaded FAILED! Response Code : 0x%x", rc);
        exit(1);
    }

    memset(&out_public, '\0', sizeof(out_public));
    memset(&name, '\0', sizeof(name));

    /* Check if the object is really loaded by accessing its
     * public area */
    LOG_INFO("Calling ReadPublic");
    rc = Tss2_Sys_ReadPublic (sapi_context,
                              object_handle,
                              NULL,
                              &out_public,
                              &name,
                              &qualified_name,
                              NULL);
    if (rc == TPM2_RC_SUCCESS) {
        LOG_INFO("success! Object's qualified name is:");
        LOGBLOB_INFO(qualified_name.name, qualified_name.size, "%s", "name:");
    } else {
        LOG_ERROR("Tss2_Sys_ReadPublic FAILED! Response Code : 0x%x", rc);
        exit(1);
    }

    rc = Tss2_Sys_FlushContext (sapi_context, object_handle);
    if (rc != TSS2_RC_SUCCESS) {
        LOG_ERROR("Tss2_Sys_FlushContext failed: 0x%" PRIx32, rc);
        exit(1);
    }

    return rc;
}