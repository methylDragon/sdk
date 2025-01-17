# Copyright 2023 Intrinsic Innovation LLC

"""Make sure versions of gppcio from PyPI and grpc from github are synchronized."""

import argparse
import pathlib


def parse_args():
  parser = argparse.ArgumentParser()
  parser.add_argument("--deps-0", required=True, type=pathlib.Path)
  parser.add_argument("--module-bazel", required=True, type=pathlib.Path)
  parser.add_argument("--requirements-in", required=True, type=pathlib.Path)
  parser.add_argument("--requirements-txt", required=True, type=pathlib.Path)
  args = parser.parse_args()

  assert args.deps_0.exists()
  assert args.requirements_in.exists()
  assert args.requirements_txt.exists()

  return args


def main():
  args = parse_args()
  integrity = "sha256-68Os/ecM+uP08EuNu3IllUDLHcQnvjYlafvCYH2r/jk="
  assert f'integrity = "{integrity}"' in args.deps_0.read_text()
  assert 'name = "grpc", version = "1.65.0"' in args.module_bazel.read_text()
  assert "grpcio==1.65.0" in args.requirements_in.read_text()
  assert "grpcio==1.65.0" in args.requirements_txt.read_text()


if __name__ == "__main__":
  main()
