/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/net/certificate_utility.h"

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/sys_string_conversions.h"

#include "net/base/net_errors.h"
#include "net/cert/cert_verify_proc_ios.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/crl_set.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util_apple.h"
#include "net/log/net_log_with_source.h"

@implementation BraveCertificateUtility
+ (int)verifyTrust:(SecTrustRef)trust host:(NSString*)host {
  auto create_cert_from_trust =
      [](SecTrustRef trust) -> scoped_refptr<net::X509Certificate> {
    if (!trust) {
      return nullptr;
    }

    CFIndex cert_count = SecTrustGetCertificateCount(trust);
    if (cert_count == 0) {
      return nullptr;
    }

    std::vector<base::ScopedCFTypeRef<SecCertificateRef>> intermediates;
    for (CFIndex i = 1; i < cert_count; ++i) {
      intermediates.emplace_back(SecTrustGetCertificateAtIndex(trust, i),
                                 base::scoped_policy::RETAIN);
    }

    return net::x509_util::CreateX509CertificateFromSecCertificate(
        /*root_cert=*/base::ScopedCFTypeRef<SecCertificateRef>(
            SecTrustGetCertificateAtIndex(trust, 0),
            base::scoped_policy::RETAIN),
        intermediates);
  };

  auto cert = create_cert_from_trust(trust);
  if (!cert) {
    return net::ERR_FAILED;
  }

  net::CertVerifyResult verify_result;
  scoped_refptr<net::CertVerifyProc> verifier =
      base::MakeRefCounted<net::CertVerifyProcIOS>();
  verifier->Verify(cert.get(), base::SysNSStringToUTF8(host),
                   /*ocsp_response=*/std::string(),
                   /*sct_list=*/std::string(),
                   /*flags=*/0,
                   /*crl_set=*/nullptr,
                   /*additional_trust_anchors=*/net::CertificateList(),
                   &verify_result, net::NetLogWithSource());

  return verify_result.cert_status;
}
@end
