# Copyright 2023 Intrinsic Innovation LLC

"""Workspace dependencies needed for the Intrinsic SDKs as a 3rd-party consumer (part 0)."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("//bazel:non_module_deps.bzl", "non_module_deps")

def intrinsic_sdks_deps_0():
    """Loads workspace dependencies needed for the Intrinsic SDKs.

    This is only the first part. Projects which want to use the Intrinsic SDKs with Bazel and which
    don't define the necessary dependencies in another way need to call *all* macros. E.g., do the
    following in your WORKSPACE:

    git_repository(name = "com_googlesource_intrinsic_intrinsic_sdks", remote = "...", ...)
    load("@com_googlesource_intrinsic_intrinsic_sdks//bazel:deps_0.bzl", "intrinsic_sdks_deps_0")
    intrinsic_sdks_deps_0()
    load("@com_googlesource_intrinsic_intrinsic_sdks//bazel:deps_1.bzl", "intrinsic_sdks_deps_1")
    intrinsic_sdks_deps_1()
    load("@com_googlesource_intrinsic_intrinsic_sdks//bazel:deps_2.bzl", "intrinsic_sdks_deps_2")
    intrinsic_sdks_deps_2()
    load("@com_googlesource_intrinsic_intrinsic_sdks//bazel:deps_3.bzl", "intrinsic_sdks_deps_3")
    intrinsic_sdks_deps_3()

    The reason why this is split into multiple files and macros is that .bzl-files can only contain
    loads at the very top. Loads and macro calls that depend on a previous macro call in this file
    are located in intrinsic_sdks_deps_1.bzl/intrinsic_sdks_deps_1() and so on.
    """

    # Include non-bzlmod dependencies
    non_module_deps()

    # Go rules and toolchain
    maybe(
        http_archive,
        name = "io_bazel_rules_go",
        sha256 = "d93ef02f1e72c82d8bb3d5169519b36167b33cf68c252525e3b9d3d5dd143de7",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/rules_go/releases/download/v0.49.0/rules_go-v0.49.0.zip",
            "https://github.com/bazelbuild/rules_go/releases/download/v0.49.0/rules_go-v0.49.0.zip",
        ],
    )

    maybe(
        http_archive,
        name = "bazel_gazelle",
        sha256 = "75df288c4b31c81eb50f51e2e14f4763cb7548daae126817247064637fd9ea62",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-gazelle/releases/download/v0.36.0/bazel-gazelle-v0.36.0.tar.gz",
            "https://github.com/bazelbuild/bazel-gazelle/releases/download/v0.36.0/bazel-gazelle-v0.36.0.tar.gz",
        ],
    )

    # CC toolchain
    maybe(
        http_archive,
        name = "toolchains_llvm",
        sha256 = "b7cd301ef7b0ece28d20d3e778697a5e3b81828393150bed04838c0c52963a01",
        strip_prefix = "toolchains_llvm-0.10.3",
        canonical_id = "0.10.3",
        url = "https://github.com/bazel-contrib/toolchains_llvm/releases/download/0.10.3/toolchains_llvm-0.10.3.tar.gz",
    )

    # Python rules, toolchain and pip dependencies
    maybe(
        http_archive,
        name = "rules_python",
        strip_prefix = "rules_python-0.31.0",
        url = "https://github.com/bazelbuild/rules_python/archive/refs/tags/0.31.0.zip",
        sha256 = "9110e83a233c9edce177241f3ae95eae4e4cc3b602d845878d76ad4e3bab7c60",
    )

    maybe(
        http_archive,
        name = "rules_license",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/rules_license/releases/download/0.0.7/rules_license-0.0.7.tar.gz",
            "https://github.com/bazelbuild/rules_license/releases/download/0.0.7/rules_license-0.0.7.tar.gz",
        ],
        sha256 = "4531deccb913639c30e5c7512a054d5d875698daeb75d8cf90f284375fe7c360",
    )

    maybe(
        http_archive,
        name = "rules_pkg",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/rules_pkg/releases/download/0.9.1/rules_pkg-0.9.1.tar.gz",
            "https://github.com/bazelbuild/rules_pkg/releases/download/0.9.1/rules_pkg-0.9.1.tar.gz",
        ],
        sha256 = "8f9ee2dc10c1ae514ee599a8b42ed99fa262b757058f65ad3c384289ff70c4b8",
    )

    maybe(
        http_archive,
        name = "aspect_bazel_lib",
        sha256 = "c780120ab99a4ca9daac69911eb06434b297214743ee7e0a1f1298353ef686db",
        strip_prefix = "bazel-lib-2.7.9",
        url = "https://github.com/aspect-build/bazel-lib/releases/download/v2.7.9/bazel-lib-v2.7.9.tar.gz",
    )

    maybe(
        http_archive,
        name = "com_google_googleapis",
        urls = [
            "https://github.com/googleapis/googleapis/archive/1c8d509c574aeab7478be1bfd4f2e8f0931cfead.tar.gz",
        ],
        sha256 = "b854ae17ddb933c249530f743db8d78df80905dfb42681255564a1d1921dfc3c",
        strip_prefix = "googleapis-1c8d509c574aeab7478be1bfd4f2e8f0931cfead",
    )

    maybe(
        http_archive,
        name = "rules_oci",
        sha256 = "686f871f9697e08877b85ea6c16c8d48f911bf466c3aeaf108ca0ab2603c7306",
        strip_prefix = "rules_oci-1.5.1",
        url = "https://github.com/bazel-contrib/rules_oci/releases/download/v1.5.1/rules_oci-v1.5.1.tar.gz",
    )

    maybe(
        http_archive,
        name = "rules_proto",
        sha256 = "303e86e722a520f6f326a50b41cfc16b98fe6d1955ce46642a5b7a67c11c0f5d",
        strip_prefix = "rules_proto-6.0.0",
        url = "https://github.com/bazelbuild/rules_proto/releases/download/6.0.0/rules_proto-6.0.0.tar.gz",
    )

    # Overrides gRPC's dependency on re2. Solves a build error in opt mode
    # http://b/277071259
    # http://go/upsalite-prod/8e107935-5aad-422c-bda4-747ffd58f4b9/targets
    RE2_COMMIT = "055e1241fcfde4f19f072b4dd909a824de54ceb8"  # tag: 2023-03-01
    maybe(
        http_archive,
        name = "com_googlesource_code_re2",
        sha256 = "47c5b6f31745cb0507dd6a4e8ed1d531ba871d0d8fc9f359c16386615018c2c0",
        strip_prefix = "re2-" + RE2_COMMIT,
        urls = ["https://github.com/google/re2/archive/" + RE2_COMMIT + ".tar.gz"],
    )

    # Another alias for RE2
    maybe(
        http_archive,
        name = "com_github_google_re2",
        sha256 = "47c5b6f31745cb0507dd6a4e8ed1d531ba871d0d8fc9f359c16386615018c2c0",
        strip_prefix = "re2-" + RE2_COMMIT,
        urls = ["https://github.com/google/re2/archive/" + RE2_COMMIT + ".tar.gz"],
    )

    # Protobuf
    maybe(
        http_archive,
        name = "com_google_protobuf",
        patch_args = ["-p1"],
        patches = [
            Label("//intrinsic/production/external/patches:0015-Upgrade-protobuf-rules-python.patch"),
        ],
        integrity = "sha256-4yEAqAE4cNJP/Dfa1ngaYeXQyZUBvLBNOcNAocRKjmM=",
        strip_prefix = "protobuf-26.0",
        urls = ["https://github.com/protocolbuffers/protobuf/releases/download/v26.0/protobuf-26.0.tar.gz"],
    )

    # gRPC
    maybe(
        http_archive,
        name = "com_github_grpc_grpc",
        patch_args = ["-p1"],
        patches = [
            Label("//intrinsic/production/external/patches:0003-Remove-competing-local_config_python-definition.patch"),
            Label("//intrinsic/production/external/patches:0005-Remove-competing-go-deps.patch"),
            Label("//intrinsic/production/external/patches:0017-Fix-py-grpc-library.patch"),
        ],
        integrity = "sha256-68Os/ecM+uP08EuNu3IllUDLHcQnvjYlafvCYH2r/jk=",
        strip_prefix = "grpc-1.65.0",
        urls = ["https://github.com/grpc/grpc/archive/refs/tags/v1.65.0.tar.gz"],
    )

    # GoogleTest/GoogleMock framework. Used by most unit-tests.
    maybe(
        http_archive,
        name = "com_google_googletest",
        sha256 = "8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7",
        strip_prefix = "googletest-1.14.0",
        urls = ["https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz"],
    )

    # Google benchmark.
    maybe(
        http_archive,
        name = "com_github_google_benchmark",
        sha256 = "6bc180a57d23d4d9515519f92b0c83d61b05b5bab188961f36ac7b06b0d9e9ce",
        strip_prefix = "benchmark-1.8.3",
        urls = ["https://github.com/google/benchmark/archive/refs/tags/v1.8.3.tar.gz"],
    )

    # Google Commandline Flags.
    maybe(
        http_archive,
        name = "com_github_gflags_gflags",
        sha256 = "19713a36c9f32b33df59d1c79b4958434cb005b5b47dc5400a7a4b078111d9b5",
        strip_prefix = "gflags-2.2.2",
        urls = [
            "https://github.com/gflags/gflags/archive/refs/tags/v2.2.2.zip",
        ],
    )

    # Google Logging Library.
    maybe(
        http_archive,
        name = "com_github_google_glog",
        sha256 = "122fb6b712808ef43fbf80f75c52a21c9760683dae470154f02bddfc61135022",
        strip_prefix = "glog-0.6.0",
        urls = [
            "https://github.com/google/glog/archive/refs/tags/v0.6.0.zip",
        ],
    )

    # C++ rules for Bazel.
    maybe(
        http_archive,
        name = "rules_cc",
        sha256 = "2037875b9a4456dce4a79d112a8ae885bbc4aad968e6587dca6e64f3a0900cdf",
        strip_prefix = "rules_cc-0.0.9",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/rules_cc/releases/download/0.0.9/rules_cc-0.0.9.tar.gz",
            "https://github.com/bazelbuild/rules_cc/releases/download/0.0.9/rules_cc-0.0.9.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_google_absl_py",
        sha256 = "8a3d0830e4eb4f66c4fa907c06edf6ce1c719ced811a12e26d9d3162f8471758",
        strip_prefix = "abseil-py-2.1.0",
        urls = [
            "https://github.com/abseil/abseil-py/archive/refs/tags/v2.1.0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_google_absl",
        integrity = "sha256-czcmuMOm05pBINfkXqi0GkNM2s3kAculAPFCNsSbOdw=",
        strip_prefix = "abseil-cpp-20240116.2",
        urls = [
            "https://github.com/abseil/abseil-cpp/releases/download/20240116.2/abseil-cpp-20240116.2.tar.gz",
        ],
    )

    # C++ rules for pybind11
    maybe(
        http_archive,
        name = "pybind11_bazel",
        sha256 = "dc14a960672babf6da2f283079a5b5c13e404a940ea7cdb8297b71f8f31643a5",
        strip_prefix = "pybind11_bazel-2.12.0",
        urls = ["https://github.com/pybind/pybind11_bazel/archive/refs/tags/v2.12.0.tar.gz"],
    )

    maybe(
        http_archive,
        name = "pybind11",
        build_file = "@pybind11_bazel//:pybind11-BUILD.bazel",
        sha256 = "411f77380c43798506b39ec594fc7f2b532a13c4db674fcf2b1ca344efaefb68",
        strip_prefix = "pybind11-2.12.0",
        urls = ["https://github.com/pybind/pybind11/archive/v2.12.0.zip"],
    )

    maybe(
        http_archive,
        name = "pybind11_abseil",
        sha256 = "1496b112e86416e2dcf288569a3e7b64f3537f0b18132224f492266e9ff76c44",
        strip_prefix = "pybind11_abseil-202402.0",
        urls = ["https://github.com/pybind/pybind11_abseil/archive/refs/tags/v202402.0.tar.gz"],
    )

    maybe(
        http_archive,
        name = "pybind11_protobuf",
        sha256 = "21189abd098528fd3986dcec349ef61ae5506d29d0c51c7a13d6f9dfcc17c676",
        strip_prefix = "pybind11_protobuf-1d7a7296604537db5f6ace2ddafd1d08c967ec63",
        urls = ["https://github.com/pybind/pybind11_protobuf/archive/1d7a7296604537db5f6ace2ddafd1d08c967ec63.tar.gz"],  #  Jun 19, 2023
    )

    # Bazel skylib
    maybe(
        http_archive,
        name = "bazel_skylib",
        sha256 = "cd55a062e763b9349921f0f5db8c3933288dc8ba4f76dd9416aac68acee3cb94",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.5.0/bazel-skylib-1.5.0.tar.gz",
            "https://github.com/bazelbuild/bazel-skylib/releases/download/1.5.0/bazel-skylib-1.5.0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "bazel_features",
        sha256 = "2cd9e57d4c38675d321731d65c15258f3a66438ad531ae09cb8bb14217dc8572",
        strip_prefix = "bazel_features-1.11.0",
        url = "https://github.com/bazel-contrib/bazel_features/releases/download/v1.11.0/bazel_features-v1.11.0.tar.gz",
    )

    # Rules for building C/C++ projects using foreign build systems inside Bazel projects
    maybe(
        http_archive,
        name = "rules_foreign_cc",
        sha256 = "476303bd0f1b04cc311fc258f1708a5f6ef82d3091e53fd1977fa20383425a6a",
        strip_prefix = "rules_foreign_cc-0.10.1",
        url = "https://github.com/bazelbuild/rules_foreign_cc/releases/download/0.10.1/rules_foreign_cc-0.10.1.tar.gz",
    )

    # Used by com_github_google_flatbuffers
    http_archive(
        name = "aspect_rules_js",
        sha256 = "76a04ef2120ee00231d85d1ff012ede23963733339ad8db81f590791a031f643",
        strip_prefix = "rules_js-1.34.1",
        url = "https://github.com/aspect-build/rules_js/releases/download/v1.34.1/rules_js-v1.34.1.tar.gz",
    )
    http_archive(
        name = "aspect_rules_ts",
        sha256 = "4c3f34fff9f96ffc9c26635d8235a32a23a6797324486c7d23c1dfa477e8b451",
        strip_prefix = "rules_ts-1.4.5",
        url = "https://github.com/aspect-build/rules_ts/releases/download/v1.4.5/rules_ts-v1.4.5.tar.gz",
    )

    http_archive(
        name = "com_github_google_flatbuffers",
        urls = ["https://github.com/google/flatbuffers/archive/refs/tags/v24.3.7.tar.gz"],
        integrity = "sha256-v/+dIVD8/4j4ROjGCLArKg6UySrqObBMBiR4NGQwR4Q=",
        strip_prefix = "flatbuffers-24.3.7",
    )

    # GMock matchers for protocol buffers
    http_archive(
        name = "com_github_inazarenko_protobuf_matchers",
        sha256 = "a3bf4c3effbe0a8ef1021c3a55dbcc0449f838c363c9d1cb4b3dd3530d139fe2",
        strip_prefix = "protobuf-matchers-57d98500ef05ed21b953a8b10eef877a5664feb4",
        url = "https://github.com/inazarenko/protobuf-matchers/archive/57d98500ef05ed21b953a8b10eef877a5664feb4.tar.gz",
    )
