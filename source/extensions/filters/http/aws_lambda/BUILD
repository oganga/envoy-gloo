licenses(["notice"])  # Apache 2

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_binary",
    "envoy_cc_library",
    "envoy_cc_test",
    "envoy_package",
)

envoy_package()

load("@envoy_api//bazel:api_build_system.bzl", "api_proto_library")

envoy_cc_library(
    name = "aws_authenticator_lib",
    srcs = ["aws_authenticator.cc"],
    hdrs = ["aws_authenticator.h"],
    external_deps = ["ssl"],
    repository = "@envoy",
    deps = [
        "@envoy//include/envoy/buffer:buffer_interface",
        "@envoy//include/envoy/http:header_map_interface",
        "@envoy//source/common/common:assert_lib",
        "@envoy//source/common/common:empty_string",
        "@envoy//source/common/common:hex_lib",
        "@envoy//source/common/config:json_utility_lib",
        "@envoy//source/common/http:headers_lib",
        "@envoy//source/common/http:utility_lib",
    ],
)

envoy_cc_library(
    name = "aws_lambda_filter_lib",
    srcs = [
        "aws_lambda_filter.cc",
    ],
    hdrs = [
        "aws_lambda_filter.h",
    ],
    repository = "@envoy",
    deps = [
        ":aws_authenticator_lib",
        "//api/envoy/config/filter/http/aws_lambda/v2:aws_lambda_proto_cc",
        "//include/envoy/http:metadata_accessor_interface",
        "@envoy//source/common/http:filter_utility_lib",
        "//source/common/http:solo_filter_utility_lib",
        # TODO(yuval-k): merge this lib to the common known names
        "//source/extensions/filters/http:aws_lambda_well_known_names_lib",
    ],
)

envoy_cc_library(
    name = "aws_lambda_filter_config_lib",
    srcs = ["aws_lambda_filter_config_factory.cc"],
    hdrs = ["aws_lambda_filter_config_factory.h"],
    repository = "@envoy",
    deps = [
        ":aws_lambda_filter_lib",
        "@envoy//include/envoy/registry",
        "@envoy//source/extensions/filters/http/common:empty_http_filter_config_lib",
    ],
)