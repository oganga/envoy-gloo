#pragma once

#include <chrono>
#include <set>
#include <string>

#include "envoy/buffer/buffer.h"
#include "envoy/http/header_map.h"

#include "openssl/digest.h"
#include "openssl/hmac.h"
#include "openssl/sha.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace AwsLambda {

typedef bool (*LowerCaseStringCompareFunc)(const Http::LowerCaseString &,
                                           const Http::LowerCaseString &);

typedef std::set<Http::LowerCaseString, LowerCaseStringCompareFunc> HeaderList;

class AwsAuthenticator {
public:
  AwsAuthenticator();

  ~AwsAuthenticator();

  void init(const std::string *access_key, const std::string *secret_key);

  void updatePayloadHash(const Buffer::Instance &data);

  void sign(Http::HeaderMap *request_headers, const HeaderList &headers_to_sign,
            const std::string &region);

  /**
   * This creates a a list of headers to sign to be used by sign.
   */
  static HeaderList
  createHeaderToSign(std::initializer_list<Http::LowerCaseString> headers);

private:
  // TODO(yuval-k) can I refactor our the friendliness?
  friend class AwsAuthenticatorTest;

  std::string
  signWithTime(Http::HeaderMap *request_headers,
               const HeaderList &headers_to_sign, const std::string &region,
               std::chrono::time_point<std::chrono::system_clock> now);

  std::string addDate(std::chrono::time_point<std::chrono::system_clock> now);

  std::pair<std::string, std::string>
  prepareHeaders(const HeaderList &headers_to_sign);

  std::string getBodyHexSha();
  void fetchUrl();
  std::string computeCanonicalRequestHash(const std::string &request_method,
                                          const std::string &canonical_Headers,
                                          const std::string &signed_headers,
                                          const std::string &hexpayload);
  std::string
  getCredntialScopeDate(std::chrono::time_point<std::chrono::system_clock> now);
  std::string getCredntialScope(const std::string &region,
                                const std::string &datenow);

  std::string computeSignature(const std::string &region,
                               const std::string &credential_scope_date,
                               const std::string &credential_scope,
                               const std::string &request_date_time,
                               const std::string &hashed_canonical_request);

  static bool lowercasecompare(const Http::LowerCaseString &i,
                               const Http::LowerCaseString &j);

  class Sha256 {
  public:
    static const int LENGTH = SHA256_DIGEST_LENGTH;
    Sha256();
    void update(const Buffer::Instance &data);
    void update(const std::string &data);
    void update(char c);
    void update(const uint8_t *bytes, size_t size);
    void update(const char *chars, size_t size);
    void finalize(uint8_t *out);

  private:
    SHA256_CTX context_;
  };

  class HMACSha256 {
  public:
    HMACSha256();
    ~HMACSha256();
    size_t length() const;
    void init(const std::string &data);
    void init(const uint8_t *bytes, size_t size);
    void update(const std::string &data);
    void update(std::initializer_list<const std::string *> strings);
    void update(const uint8_t *bytes, size_t size);
    void finalize(uint8_t *out, unsigned int *out_len);

  private:
    HMAC_CTX context_;
    const EVP_MD *evp_;
    bool firstinit{true};
  };

  template <typename T>
  static void recusiveHmacHelper(HMACSha256 &hmac, uint8_t *out,
                                 unsigned int &out_len, const T &what) {
    hmac.init(out, out_len);
    hmac.update(what);
    hmac.finalize(out, &out_len);
  }

  Sha256 body_sha_;

  const std::string *access_key_{};
  std::string first_key_;
  const std::string *service_{};
  const std::string *method_{};
  const char *query_string_start_{};
  size_t query_string_len_{};
  const char *url_start_{};
  size_t url_len_{};

  Http::HeaderMap *request_headers_{};
};

} // namespace AwsLambda
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
