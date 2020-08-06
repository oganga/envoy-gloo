#pragma once

#include "envoy/api/api.h"
#include "envoy/common/pure.h"
#include "envoy/common/time.h"

#include "common/common/regex.h"

#include "extensions/common/aws/credentials_provider.h"
#include "extensions/filters/http/aws_lambda/sts_fetcher.h"

#include "api/envoy/config/filter/http/aws_lambda/v2/aws_lambda.pb.validate.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace AwsLambda {

class StsCredentialsProvider;
using StsCredentialsProviderPtr = std::shared_ptr<StsCredentialsProvider>;

class StsCredentialsProvider {
public:
  virtual ~StsCredentialsProvider() = default;

  class Callbacks {
  public:
    virtual ~Callbacks() = default;

    /**
     * Called on successful request
     *
     * @param credential the credentials
     */
    virtual void onSuccess(
        std::shared_ptr<const Envoy::Extensions::Common::Aws::Credentials>)
        PURE;

    /**
     * Called on completion of request.
     *
     * @param status the status of the request.
     */
    virtual void onFailure(CredentialsFailureStatus status) PURE;
  };

  class Canceller {
  public:
    virtual ~Canceller() = default;
      /**
     * Called when a given context is cancelled
     */
    virtual void cancel() PURE;
  }

  // Context object to hold data needed for verifier.
  class Context : public Canceller {
  public:

    virtual ~Context() = default;

    /**
     * Returns the request callback wrapped in this context.
     *
     * @returns the request callback.
     */
    virtual Callbacks *callbacks() const PURE;

    /**
     * Cancel any pending requests for this context.
     */
    virtual void cancel() PURE;
  };

  using ContextSharedPtr = std::shared_ptr<Context>;

  // Lookup credentials cache map.
  virtual void find(const absl::optional<std::string> &role_arn,
                    ContextSharedPtr context) PURE;
};

class StsConnectionPool {
  virtual ~StsConnectionPool() = default;
  
  virtual void add(std::unique_ptr<ContextImpl> ctx) PURE;
}

using StsConnectionPoolPtr = std::unique_ptr<StsConnectionPool>;


class StsConnectionPoolImpl : public StsCredentialsProvider::Canceller {
  StsConnectionPoolImpl(Upstream::ClusterManager &cm, Api::Api &api,
              const envoy::config::core::v3::HttpUri &uri,
             const absl::string_view role_arn,
             const absl::string_view web_token)
  
  void cancel() override;

  void add(std::unique_ptr<ContextImpl> ctx) override;

private:
  StsFetcherPtr fetcher_;
  std::list<ContextImpl> connection_list_;
};

class StsCredentialsProviderImpl
    : public StsCredentialsProvider,
      public Logger::Loggable<Logger::Id::aws>,
      public std::enable_shared_from_this<StsCredentialsProviderImpl> {

public:
  void find(const absl::optional<std::string> &role_arn_arg,
            ContextSharedPtr context) override;

  // Factory function to create an instance.
  static StsCredentialsProviderPtr
  create(const envoy::config::filter::http::aws_lambda::v2::
             AWSLambdaConfig_ServiceAccountCredentials &config,
         Api::Api &api, Event::Dispatcher &dispatcher) {

    std::shared_ptr<StsCredentialsProviderImpl> ptr(
        new StsCredentialsProviderImpl(config, api, dispatcher));
    ptr->init();
    return ptr;
  };

private:
  StsCredentialsProviderImpl(
      const envoy::config::filter::http::aws_lambda::v2::
          AWSLambdaConfig_ServiceAccountCredentials &config,
      Api::Api &api, Event::Dispatcher &dispatcher);

  void init();

  Api::Api &api_;
  const envoy::config::filter::http::aws_lambda::v2::
      AWSLambdaConfig_ServiceAccountCredentials config_;

  std::string default_role_arn_;
  std::string token_file_;
  envoy::config::core::v3::HttpUri uri_;

  std::regex regex_access_key_;
  std::regex regex_secret_key_;
  std::regex regex_session_token_;
  std::regex regex_expiration_;

  Envoy::Filesystem::WatcherPtr file_watcher_;

  // web_token set by AWS, will be auto-updated by StsCredentialsProvider
  // TODO: udpate this file, inotify or timer
  std::string web_token_;
  // Credentials storage map, keyed by arn
  std::unordered_map<std::string, StsCredentialsConstSharedPtr>
      credentials_cache_;

  std::unordered_map<std::string, StsConnectionPoolPtr>
      connection_pools_;
};

using ContextSharedPtr = std::shared_ptr<StsCredentialsProvider::Context>;

class ContextFactory {
public:
  ContextFactory(Upstream::ClusterManager &cm, Api::Api &api)
      : cm_(cm), api_(api){};

  virtual ~ContextFactory() = default;

  virtual ContextSharedPtr
  create(StsCredentialsProvider::Callbacks *callbacks) const;

private:
  Upstream::ClusterManager &cm_;
  Api::Api &api_;
};

class StsCredentialsProviderFactory {
public:
  virtual ~StsCredentialsProviderFactory() = default;

  virtual StsCredentialsProviderPtr
  create(const envoy::config::filter::http::aws_lambda::v2::
             AWSLambdaConfig_ServiceAccountCredentials &config) const PURE;
};

class StsCredentialsProviderFactoryImpl : public StsCredentialsProviderFactory {
public:
  StsCredentialsProviderFactoryImpl(Api::Api &api,
                                    Event::Dispatcher &dispatcher)
      : api_(api), dispatcher_(dispatcher){};

  StsCredentialsProviderPtr
  create(const envoy::config::filter::http::aws_lambda::v2::
             AWSLambdaConfig_ServiceAccountCredentials &config) const override;

private:
  Api::Api &api_;
  Event::Dispatcher &dispatcher_;
};

} // namespace AwsLambda
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
