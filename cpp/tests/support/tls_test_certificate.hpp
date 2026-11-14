#pragma once

#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace umbra::test::tls {

class TemporaryDirectory final {
 public:
  TemporaryDirectory() {
    static std::atomic_uint64_t next{0U};
    auto const nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    for (;;) {
      path_ = std::filesystem::temp_directory_path() /
          ("umbra-tls-test-" + std::to_string(nonce) + "-" +
           std::to_string(++next));
      std::error_code error;
      if (std::filesystem::create_directory(path_, error)) {
        return;
      }
      if (error) {
        throw std::filesystem::filesystem_error(
            "Creating the TLS test directory failed", path_, error);
      }
    }
  }

  TemporaryDirectory(TemporaryDirectory const&) = delete;
  TemporaryDirectory& operator=(TemporaryDirectory const&) = delete;

  ~TemporaryDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path_, ignored);
  }

  [[nodiscard]] std::filesystem::path const& path() const noexcept {
    return path_;
  }

 private:
  std::filesystem::path path_;
};

inline void writeTestCertificate(
    std::filesystem::path const& certificatePath,
    std::filesystem::path const& privateKeyPath,
    char const* subjectAlternativeName = "DNS:localhost,IP:127.0.0.1") {
  auto keyContext = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>(
      EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr), EVP_PKEY_CTX_free);
  if (!keyContext || EVP_PKEY_keygen_init(keyContext.get()) <= 0 ||
      EVP_PKEY_CTX_set_ec_paramgen_curve_nid(
          keyContext.get(), NID_X9_62_prime256v1) <= 0) {
    throw std::runtime_error("Creating the TLS test key generator failed.");
  }

  EVP_PKEY* rawKey = nullptr;
  if (EVP_PKEY_keygen(keyContext.get(), &rawKey) <= 0 || rawKey == nullptr) {
    throw std::runtime_error("Generating the TLS test key failed.");
  }
  auto key = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>(
      rawKey, EVP_PKEY_free);

  auto certificate = std::unique_ptr<X509, decltype(&X509_free)>(
      X509_new(), X509_free);
  if (!certificate || X509_set_version(certificate.get(), 2L) != 1 ||
      ASN1_INTEGER_set(X509_get_serialNumber(certificate.get()), 1L) != 1 ||
      X509_gmtime_adj(X509_getm_notBefore(certificate.get()), -60L) == nullptr ||
      X509_gmtime_adj(X509_getm_notAfter(certificate.get()), 3600L) == nullptr ||
      X509_set_pubkey(certificate.get(), key.get()) != 1) {
    throw std::runtime_error("Creating the TLS test certificate failed.");
  }

  auto subject = std::unique_ptr<X509_NAME, decltype(&X509_NAME_free)>(
      X509_NAME_new(), X509_NAME_free);
  unsigned char const commonName[] = "localhost";
  if (!subject ||
      X509_NAME_add_entry_by_txt(
          subject.get(),
          "CN",
          MBSTRING_ASC,
          commonName,
          -1,
          -1,
          0) != 1 ||
      X509_set_subject_name(certificate.get(), subject.get()) != 1 ||
      X509_set_issuer_name(certificate.get(), subject.get()) != 1) {
    throw std::runtime_error("Setting the TLS test certificate identity failed.");
  }

  X509V3_CTX extensionContext{};
  X509V3_set_ctx(
      &extensionContext,
      certificate.get(),
      certificate.get(),
      nullptr,
      nullptr,
      0);
  auto addExtension = [&](int nid, char const* value) {
    auto extension = std::unique_ptr<
        X509_EXTENSION,
        decltype(&X509_EXTENSION_free)>(
        X509V3_EXT_conf_nid(
            nullptr,
            &extensionContext,
            nid,
            const_cast<char*>(value)),
        X509_EXTENSION_free);
    if (!extension || X509_add_ext(certificate.get(), extension.get(), -1) != 1) {
      throw std::runtime_error("Adding a TLS test certificate extension failed.");
    }
  };
  addExtension(NID_basic_constraints, "critical,CA:TRUE");
  addExtension(NID_key_usage, "critical,digitalSignature,keyCertSign");
  addExtension(NID_subject_alt_name, subjectAlternativeName);

  if (X509_sign(certificate.get(), key.get(), EVP_sha256()) <= 0) {
    throw std::runtime_error("Signing the TLS test certificate failed.");
  }

  auto certificateBio = std::unique_ptr<BIO, decltype(&BIO_free)>(
      BIO_new_file(certificatePath.string().c_str(), "wb"), BIO_free);
  auto privateKeyBio = std::unique_ptr<BIO, decltype(&BIO_free)>(
      BIO_new_file(privateKeyPath.string().c_str(), "wb"), BIO_free);
  if (!certificateBio || !privateKeyBio ||
      PEM_write_bio_X509(certificateBio.get(), certificate.get()) != 1 ||
      PEM_write_bio_PrivateKey(
          privateKeyBio.get(),
          key.get(),
          nullptr,
          nullptr,
          0,
          nullptr,
          nullptr) != 1) {
    throw std::runtime_error("Writing the TLS test credentials failed.");
  }
}

}  // namespace umbra::test::tls
