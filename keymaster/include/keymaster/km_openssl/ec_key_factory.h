/*
 * Copyright 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <openssl/ec.h>
#include <openssl/evp.h>

#include <keymaster/asymmetric_key_factory.h>
#include <keymaster/soft_key_factory.h>

namespace keymaster {

class EcKeyFactory : public AsymmetricKeyFactory, public SoftKeyFactoryMixin {
  public:
    explicit EcKeyFactory(const SoftwareKeyBlobMaker& blob_maker, const KeymasterContext& context)
        : AsymmetricKeyFactory(context), SoftKeyFactoryMixin(blob_maker) {}

    keymaster_algorithm_t keymaster_key_type() const override { return KM_ALGORITHM_EC; }
    int evp_key_type() const override { return EVP_PKEY_EC; }

    keymaster_error_t GenerateKey(const AuthorizationSet& key_description,
                                  UniquePtr<Key> attest_key,            //
                                  const KeymasterBlob& issuer_subject,  //
                                  KeymasterKeyBlob* key_blob,
                                  AuthorizationSet* hw_enforced,  //
                                  AuthorizationSet* sw_enforced,
                                  CertificateChain* cert_chain) const override;
    keymaster_error_t ImportKey(const AuthorizationSet& key_description,
                                keymaster_key_format_t input_key_material_format,
                                const KeymasterKeyBlob& input_key_material,
                                UniquePtr<Key> attest_key,  //
                                const KeymasterBlob& issuer_subject,
                                KeymasterKeyBlob* output_key_blob,  //
                                AuthorizationSet* hw_enforced,      //
                                AuthorizationSet* sw_enforced,
                                CertificateChain* cert_chain) const override;

    keymaster_error_t CreateEmptyKey(AuthorizationSet&& hw_enforced, AuthorizationSet&& sw_enforced,
                                     UniquePtr<AsymmetricKey>* key) const override;

    keymaster_error_t UpdateImportKeyDescription(const AuthorizationSet& key_description,
                                                 keymaster_key_format_t key_format,
                                                 const KeymasterKeyBlob& key_material,
                                                 AuthorizationSet* updated_description,
                                                 uint32_t* key_size) const;

    OperationFactory* GetOperationFactory(keymaster_purpose_t purpose) const override;

  protected:
    static EC_GROUP* ChooseGroup(size_t key_size_bits);
    static EC_GROUP* ChooseGroup(keymaster_ec_curve_t ec_curve);

    static keymaster_error_t GetCurveAndSize(const AuthorizationSet& key_description,
                                             keymaster_ec_curve_t* curve, uint32_t* key_size_bits);
};

}  // namespace keymaster
