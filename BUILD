# Copyright 2023 Intrinsic Innovation LLC
# Intrinsic Proprietary and Confidential
# Provided subject to written agreement between the parties.

load("@bazel_gazelle//:def.bzl", "gazelle")

gazelle(name = "gazelle")

exports_files(
    srcs = [".bazelrc"],
    visibility = ["//intrinsic/tools/inctl/cmd/bazel/templates:__subpackages__"],
)