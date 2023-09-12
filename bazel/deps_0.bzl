# Copyright 2023 Intrinsic Innovation LLC
# Intrinsic Proprietary and Confidential
# Provided subject to written agreement between the parties.

"""Workspace dependencies needed for the Intrinsic SDKs as a 3rd-party consumer (part 0)."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def intrinsic_sdks_deps_0():
    """Loads workspace dependencies needed for the Intrinsic SDKs.

    This is only the first part. Projects which want to use the Intrinsic SDKs with Bazel and which
    don't define the necessary dependencies in another way need to call *all* macros. E.g., do the
    following in your WORKSPACE:

    git_repository(name = "com_googlesource_intrinsic_intrinsic_sdks", remote = "...", ...)
    load("@com_googlesource_intrinsic_intrinsic_sdks//:intrinsic_sdks_deps_0.bzl", "intrinsic_sdks_deps_0")
    intrinsic_sdks_deps_0()
    load("@com_googlesource_intrinsic_intrinsic_sdks//:intrinsic_sdks_deps_1.bzl", "intrinsic_sdks_deps_1")
    intrinsic_sdks_deps_1()
    load("@com_googlesource_intrinsic_intrinsic_sdks//:intrinsic_sdks_deps_2.bzl", "intrinsic_sdks_deps_2")
    intrinsic_sdks_deps_2()
    load("@com_googlesource_intrinsic_intrinsic_sdks//:intrinsic_sdks_deps_3.bzl", "intrinsic_sdks_deps_3")
    intrinsic_sdks_deps_3()

    The reason why this is split into multiple files and macros is that .bzl-files can only contain
    loads at the very top. Loads and macro calls that depend on a previous macro call in this file
    are located in intrinsic_sdks_deps_1.bzl/intrinsic_sdks_deps_1() and so on.
    """

    # Go rules and toolchain
    maybe(
        http_archive,
        name = "io_bazel_rules_go",
        sha256 = "56d8c5a5c91e1af73eca71a6fab2ced959b67c86d12ba37feedb0a2dfea441a6",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/rules_go/releases/download/v0.37.0/rules_go-v0.37.0.zip",
            "https://github.com/bazelbuild/rules_go/releases/download/v0.37.0/rules_go-v0.37.0.zip",
        ],
    )

    maybe(
        http_archive,
        name = "bazel_gazelle",
        sha256 = "29d5dafc2a5582995488c6735115d1d366fcd6a0fc2e2a153f02988706349825",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-gazelle/releases/download/v0.31.0/bazel-gazelle-v0.31.0.tar.gz",
            "https://github.com/bazelbuild/bazel-gazelle/releases/download/v0.31.0/bazel-gazelle-v0.31.0.tar.gz",
        ],
    )

    # CC toolchain
    BAZEL_TOOLCHAIN_TAG = "0.8.1"
    BAZEL_TOOLCHAIN_SHA = "751bbe30bcaa462aef792b18bbd16c401af42fc937c42ad0ae463f099dc04ea2"
    maybe(
        http_archive,
        name = "com_grail_bazel_toolchain",
        sha256 = BAZEL_TOOLCHAIN_SHA,
        strip_prefix = "bazel-toolchain-{tag}".format(tag = BAZEL_TOOLCHAIN_TAG),
        canonical_id = BAZEL_TOOLCHAIN_TAG,
        url = "https://github.com/grailbio/bazel-toolchain/archive/{tag}.tar.gz".format(tag = BAZEL_TOOLCHAIN_TAG),
    )

    # Sysroot and libc
    # How to upgrade:
    # - Find image in https://storage.googleapis.com/chrome-linux-sysroot/ for amd64 for
    #   a stable Linux (here: Debian stretch), of this pick a current build.
    # - Verify the image contains expected /lib/x86_64-linux-gnu/libc* and defines correct
    #   __GLIBC_MINOR__ in /usr/include/features.h
    # - If system files are not found, add them in ../BUILD.sysroot
    maybe(
        http_archive,
        name = "com_googleapis_storage_chrome_linux_amd64_sysroot",
        build_file = Label("//intrinsic/production/external:BUILD.sysroot"),
        sha256 = "66bed6fb2617a50227a824b1f9cfaf0a031ce33e6635edaa2148f71a5d50be48",
        urls = [
            # features.h defines GLIBC 2.24. Contains /lib/x86_64-linux-gnu/libc-2.24.so,
            # last modified by Chrome 2018-02-22.
            "https://storage.googleapis.com/chrome-linux-sysroot/toolchain/15b7efb900d75f7316c6e713e80f87b9904791b1/debian_stretch_amd64_sysroot.tar.xz",
        ],
    )

    # Python rules, toolchain and pip dependencies
    maybe(
        http_archive,
        name = "rules_python",
        strip_prefix = "rules_python-0.14.0",
        url = "https://github.com/bazelbuild/rules_python/archive/refs/tags/0.14.0.zip",
        sha256 = "329a6a0f9a9a266b185e94ddf0f6ae6732ff5e1ce759b7d703b96041af36b7f9",
    )

    # Docker
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
            "https://mirror.bazel.build/github.com/bazelbuild/rules_pkg/releases/download/0.8.1/rules_pkg-0.8.1.tar.gz",
            "https://github.com/bazelbuild/rules_pkg/releases/download/0.8.1/rules_pkg-0.8.1.tar.gz",
        ],
        sha256 = "8c20f74bca25d2d442b327ae26768c02cf3c99e93fad0381f32be9aab1967675",
    )

    maybe(
        http_archive,
        name = "io_bazel_rules_docker",
        sha256 = "f6d71a193ff6df39900417b50e67a5cf0baad2e90f83c8aefe66902acce4c34d",
        strip_prefix = "rules_docker-ca2f3086ead9f751975d77db0255ffe9ee07a781",
        urls = ["https://github.com/bazelbuild/rules_docker/archive/ca2f3086ead9f751975d77db0255ffe9ee07a781.tar.gz"],
    )

    # Overrides gRPC's dependency on re2. Solves a build error in opt mode
    # http://b/277071259
    # http://go/upsalite-prod/8e107935-5aad-422c-bda4-747ffd58f4b9/targets
    RE2_COMMIT = "055e1241fcfde4f19f072b4dd909a824de54ceb8"  # tag: 2023-03-01
    maybe(
        git_repository,
        name = "com_googlesource_code_re2",
        commit = RE2_COMMIT,
        remote = "https://github.com/google/re2.git",
        shallow_since = "1645717357 +0000",
    )

    # Another alias for RE2
    maybe(
        git_repository,
        name = "com_github_google_re2",
        commit = RE2_COMMIT,
        remote = "https://github.com/google/re2.git",
        shallow_since = "1645717357 +0000",
    )

    maybe(
        http_archive,
        name = "com_google_googleapis",
        patch_args = ["-p1"],
        patches = [Label("//intrinsic/production/external/patches:0004-Simplify-longrunning_py_proto.patch")],
        sha256 = "6c005ec1356d00821c00529b7d1283669633caf7e1e30d06909dda85a351b42f",
        strip_prefix = "googleapis-f3d6f41ed50ae9271fbf9ce2355d1f1e8afb4d94",
        urls = ["https://github.com/googleapis/googleapis/archive/f3d6f41ed50ae9271fbf9ce2355d1f1e8afb4d94.zip"],
    )

    # Protobuf
    maybe(
        http_archive,
        name = "com_google_protobuf",
        patch_args = ["-p1"],
        patches = [
            Label("//intrinsic/production/external/patches:0006-Ignore-unused-function-warnings.patch"),
            # Use the implicit/native style for the "google" namespace package by removing
            # python/google/__init__.py. The presence of this file (=legacy namespace package style)
            # breaks import resolution in VS Code/Pylance in that contents of different paths that
            # contain content for the same package won't be correctly merged and some imports from
            # the "google" namespace won't be found.
            # See:
            #   - https://github.com/protocolbuffers/protobuf/issues/9876
            #   - https://github.com/microsoft/pylance-release/issues/2562
            Label("//intrinsic/production/external/patches:0008-Remove-python-google-module-file.patch"),
            Label("//intrinsic/production/external/patches:0009-Also-generate-pyi-files-protobuf.patch"),
            Label("//intrinsic/production/external/patches:0010-Remove-unknown-warning-option.patch"),
        ],
        sha256 = "2dc7254fc975bb40efcab799273c9330d7ed11f4b3263dcbf7328f5c6b067d3e",  # v3.23.1
        strip_prefix = "protobuf-2dca62f7296e5b49d729f7384f975cecb38382a0",  # v3.23.1
        urls = ["https://github.com/protocolbuffers/protobuf/archive/2dca62f7296e5b49d729f7384f975cecb38382a0.zip"],  # v3.23.1
    )

    # gRPC
    maybe(
        http_archive,
        name = "com_github_grpc_grpc",
        patch_args = ["-p1"],
        patches = [
            Label("//intrinsic/production/external/patches:0001-Add-absl-Status-conversions-for-grpc-Status.patch"),
            Label("//intrinsic/production/external/patches:0002-Add-warning-suppressions-to-cython_library.patch"),
            Label("//intrinsic/production/external/patches:0003-Remove-competing-local_config_python-definition.patch"),
            Label("//intrinsic/production/external/patches:0005-Remove-competing-go-deps.patch"),
            Label("//intrinsic/production/external/patches:0007-Also-generate-pyi-files-grpc.patch"),
        ],
        sha256 = "315dbe5aa9235ccc5fcb4e40e6ada4c1135d4377238bf9ea65534fa38f7c695a",  # v1.55.0
        strip_prefix = "grpc-0bf4a618b17a3f0ed61c22364913c7f66fc1c61a",  # v1.55.0
        urls = ["https://github.com/grpc/grpc/archive/0bf4a618b17a3f0ed61c22364913c7f66fc1c61a.zip"],  # v1.55.0
    )

    # Dependency of Protobuf and gRPC, explicitly pinned here so that we don't get the definition
    # from protobuf_deps() which applies a patch that does not build in our WORKSPACE. See
    # https://github.com/protocolbuffers/protobuf/blob/2dca62f7296e5b49d729f7384f975cecb38382a0/protobuf_deps.bzl#L156
    # Here we use a copy of gRPC's definition, see
    # https://github.com/grpc/grpc/blob/0bf4a618b17a3f0ed61c22364913c7f66fc1c61a/bazel/grpc_deps.bzl#L393-L402
    maybe(
        http_archive,
        name = "upb",
        sha256 = "7d19f2ac9c1e508a86a272913d9aa67c8147827f949035828910bb05d9f2cf03",  # Commit from May 16, 2023
        strip_prefix = "upb-61a97efa24a5ce01fb8cc73c9d1e6e7060f8ea98",  # Commit from May 16, 2023
        urls = [
            # https://github.com/protocolbuffers/upb/commits/23.x
            "https://storage.googleapis.com/grpc-bazel-mirror/github.com/protocolbuffers/upb/archive/61a97efa24a5ce01fb8cc73c9d1e6e7060f8ea98.tar.gz",  # Commit from May 16, 2023
            "https://github.com/protocolbuffers/upb/archive/61a97efa24a5ce01fb8cc73c9d1e6e7060f8ea98.tar.gz",  # Commit from May 16, 2023
        ],
    )

    # GoogleTest/GoogleMock framework. Used by most unit-tests.
    maybe(
        http_archive,
        name = "com_google_googletest",
        sha256 = "ad7fdba11ea011c1d925b3289cf4af2c66a352e18d4c7264392fead75e919363",
        strip_prefix = "googletest-1.13.0",
        urls = ["https://github.com/google/googletest/archive/refs/tags/v1.13.0.tar.gz"],
    )

    # Google benchmark.
    maybe(
        http_archive,
        name = "com_github_google_benchmark",
        sha256 = "59f918c8ccd4d74b6ac43484467b500f1d64b40cc1010daa055375b322a43ba3",
        strip_prefix = "benchmark-16703ff83c1ae6d53e5155df3bb3ab0bc96083be",
        urls = ["https://github.com/google/benchmark/archive/16703ff83c1ae6d53e5155df3bb3ab0bc96083be.zip"],
    )

    # C++ rules for Bazel.
    maybe(
        http_archive,
        name = "rules_cc",
        sha256 = "9a446e9dd9c1bb180c86977a8dc1e9e659550ae732ae58bd2e8fd51e15b2c91d",
        strip_prefix = "rules_cc-262ebec3c2296296526740db4aefce68c80de7fa",
        urls = [
            "https://github.com/bazelbuild/rules_cc/archive/262ebec3c2296296526740db4aefce68c80de7fa.zip",
        ],
    )

    # Eigen math library.
    maybe(
        http_archive,
        name = "eigen",
        build_file = Label("//intrinsic/production/external:BUILD.eigen"),
        sha256 = "be47d7280bdb186b8c4109c7323ca3f216e3d911dbae883383c2e970c189ed5a",
        strip_prefix = "eigen-f0f1d7938b7083800ff75fe88e15092f08a4e67e",
        urls = [
            "https://storage.googleapis.com/mirror.tensorflow.org/gitlab.com/libeigen/eigen/-/archive/f0f1d7938b7083800ff75fe88e15092f08a4e67e/eigen-f0f1d7938b7083800ff75fe88e15092f08a4e67e.tar.gz",
            "https://gitlab.com/libeigen/eigen/-/archive/f0f1d7938b7083800ff75fe88e15092f08a4e67e/eigen-f0f1d7938b7083800ff75fe88e15092f08a4e67e.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_google_absl_py",
        sha256 = "47059bfaef938dfe3818c8efdc14fafc8dfcf057aed93b0e8362eec2de9f4497",
        strip_prefix = "abseil-py-1.1.0",
        urls = [
            "https://github.com/abseil/abseil-py/archive/refs/tags/v1.1.0.zip",
        ],
    )

    maybe(
        http_archive,
        name = "com_google_absl",
        sha256 = "affadb4979b75e541551c1382996c135f7d7c4841a619fc35c6170a54e8dbcb0",
        # HEAD as of 2023-08-16
        strip_prefix = "abseil-cpp-9a6d9c6eae90f4a5ddb162b8ef49af1e321c9769",
        urls = [
            "https://github.com/abseil/abseil-cpp/archive/9a6d9c6eae90f4a5ddb162b8ef49af1e321c9769.zip",
        ],
    )

    # C++ rules for pybind11
    maybe(
        git_repository,
        name = "pybind11_abseil",
        commit = "2bf606ceddb0b7d874022defa8ea6d2d3e1605ad",  # May 24, 2023
        remote = "https://github.com/pybind/pybind11_abseil.git",
        repo_mapping = {"@local_config_python": "@local_config_python"},
        shallow_since = "1647991761 -0700",
    )

    maybe(
        git_repository,
        name = "pybind11_bazel",
        commit = "b162c7c88a253e3f6b673df0c621aca27596ce6b",  # May 3, 2023
        remote = "https://github.com/pybind/pybind11_bazel.git",
        repo_mapping = {"@local_config_python": "@local_config_python"},
        shallow_since = "1638580149 -0800",
    )

    maybe(
        git_repository,
        name = "pybind11_protobuf",
        commit = "5baa2dc9d93e3b608cde86dfa4b8c63aeab4ac78",  #  Jun 19, 2023
        remote = "https://github.com/pybind/pybind11_protobuf.git",
        repo_mapping = {"@local_config_python": "@local_config_python"},
        shallow_since = "1642616122 -0800",
    )

    maybe(
        http_archive,
        name = "pybind11",
        build_file = "@pybind11_bazel//:pybind11.BUILD",
        repo_mapping = {"@local_config_python": "@local_config_python"},
        sha256 = "b011a730c8845bfc265f0f81ee4e5e9e1d354df390836d2a25880e123d021f89",
        strip_prefix = "pybind11-2.11.1",
        urls = ["https://github.com/pybind/pybind11/archive/v2.11.1.zip"],  #  Jul 17, 2023
    )

    # Flatbuffers
    maybe(
        git_repository,
        name = "com_github_google_flatbuffers",
        commit = "615616cb5549a34bdf288c04bc1b94bd7a65c396",  # v2.0.6
        remote = "https://github.com/google/flatbuffers.git",
        shallow_since = "1644943722 -0500",
    )

    # Bazel skylib
    maybe(
        http_archive,
        name = "bazel_skylib",
        sha256 = "f7be3474d42aae265405a592bb7da8e171919d74c16f082a5457840f06054728",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.2.1/bazel-skylib-1.2.1.tar.gz",
            "https://github.com/bazelbuild/bazel-skylib/releases/download/1.2.1/bazel-skylib-1.2.1.tar.gz",
        ],
    )