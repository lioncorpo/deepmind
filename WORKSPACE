workspace(name = "launchpad")

# To change to a version of protoc compatible with tensorflow:
#  1. Convert the required header version to a version string, e.g.:
#     3011004 => "3.11.4"
#  2. Calculate the sha256 of the binary:
#     PROTOC_VERSION="3.11.4"
#     curl -L "https://github.com/protocolbuffers/protobuf/releases/download/v${PROTOC_VERSION}/protoc-${PROTOC_VERSION}-linux-x86_64.zip" | sha256
#  3. Update the two variables below.
#
# Alternatively, run bazel with the environment var "LP_PROTOC_VERSION"
# set to override PROTOC_VERSION.
#
# *WARNING* If using the LP_PROTOC_VERSION environment variable, sha256
# checking is disabled.  Use at your own risk.
PROTOC_VERSION = "3.9.0"
PROTOC_SHA256 = "15e395b648a1a6dda8fd66868824a396e9d3e89bc2c8648e3b9ab9801bea5d55"

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "com_google_googletest",
    sha256 = "ff7a82736e158c077e76188232eac77913a15dac0b22508c390ab3f88e6d6d86",
    strip_prefix = "googletest-b6cd405286ed8635ece71c72f118e659f4ade3fb",
    urls = [
        "https://storage.googleapis.com/mirror.tensorflow.org/github.com/google/googletest/archive/b6cd405286ed8635ece71c72f118e659f4ade3fb.zip",
        "https://github.com/google/googletest/archive/b6cd405286ed8635ece71c72f118e659f4ade3fb.zip",
    ],
)

http_archive(
  name = "com_google_absl",
  strip_prefix = "abseil-cpp-20200923.3",
  sha256 = "6622893ab117501fc23268a2936e0d46ee6cb0319dcf2275e33a708cd9634ea6",
  urls = ["https://github.com/abseil/abseil-cpp/archive/20200923.3.zip"],
)

http_archive(
  name = "pybind11_abseil",
  strip_prefix = "pybind11_abseil-dc0479f3973b062783958da333a235bc528bfd22",
  sha256 = "046fd1c297ba4eff0a59a336a4aee23774509e497bebced550cf8c9633cf3f54",
  urls = ["https://github.com/pybind/pybind11_abseil/archive/dc0479f3973b062783958da333a235bc528bfd22.zip"],
)

http_archive(
  name = "pybind11_bazel",
  strip_prefix = "pybind11_bazel-26973c0ff320cb4b39e45bc3e4297b82bc3a6c09",
  sha256 = "8f546c03bdd55d0e88cb491ddfbabe5aeb087f87de2fbf441391d70483affe39",
  urls = ["https://github.com/pybind/pybind11_bazel/archive/26973c0ff320cb4b39e45bc3e4297b82bc3a6c09.tar.gz"],
)

## Begin GRPC related deps
http_archive(
    name = "com_github_grpc_grpc",
    patch_cmds = [
        """sed -i.bak 's/"python",/"python3",/g' third_party/py/python_configure.bzl""",
        """sed -i.bak 's/PYTHONHASHSEED=0/PYTHONHASHSEED=0 python3/g' bazel/cython_library.bzl""",
    ],
    sha256 = "b956598d8cbe168b5ee717b5dafa56563eb5201a947856a6688bbeac9cac4e1f",
    strip_prefix = "grpc-b54a5b338637f92bfcf4b0bc05e0f57a5fd8fadd",
    urls = [
        "https://storage.googleapis.com/mirror.tensorflow.org/github.com/grpc/grpc/archive/b54a5b338637f92bfcf4b0bc05e0f57a5fd8fadd.tar.gz",
        "https://github.com/grpc/grpc/archive/b54a5b338637f92bfcf4b0bc05e0f57a5fd8fadd.tar.gz",
    ],
)

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")

grpc_deps()

load("@upb//bazel:repository_defs.bzl", "bazel_version_repository")

bazel_version_repository(
    name = "bazel_version",
)

load("@build_bazel_rules_apple//apple:repositories.bzl", "apple_rules_dependencies")

apple_rules_dependencies()

load("@build_bazel_apple_support//lib:repositories.bzl", "apple_support_dependencies")

apple_support_dependencies()
## End GRPC related deps

load(
    "//launchpad:repo.bzl",
    "cc_tf_configure",
    "lp_protoc_deps",
    "lp_python_deps",
)

cc_tf_configure()

lp_python_deps()

lp_protoc_deps(version = PROTOC_VERSION, sha256 = PROTOC_SHA256)
