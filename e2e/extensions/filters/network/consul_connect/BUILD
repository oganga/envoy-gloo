licenses(["notice"])  # Apache 2

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_binary",
    "envoy_cc_library",
    "envoy_cc_test",
    "envoy_package",
)

envoy_package()

load("@envoy_api//bazel:api_build_system.bzl", "api_cc_py_proto_library")

py_test(
    name = "e2e_test",
    srcs = [
        "authorize_endpoint.py",
        "e2e_test.py",
        "upstream.py",
    ],
    data = [
        "consul",
        "etc/consul.d/redis.json",
        "//:envoy",
    ],
    deps = [
        "//e2e/extensions/filters/common:filtertest_lib",
    ],
)
