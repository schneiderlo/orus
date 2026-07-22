"""Bzlmod repository rule exposing Nix-fetched sources without another fetcher."""

def _require_source(repository_ctx):
    source = repository_ctx.os.environ.get(repository_ctx.attr.source_env)
    if not source:
        fail("BUILD_ACQUISITION_DENIED: %s is not set; enter `nix develop`" % repository_ctx.attr.source_env)
    if not source.startswith("/nix/store/"):
        fail("BUILD_ACQUISITION_DENIED: %s is not a Nix-store path" % repository_ctx.attr.source_env)
    return source

def _glaze(repository_ctx, source):
    repository_ctx.symlink(source + "/include", "include")
    repository_ctx.symlink(source + "/LICENSE", "LICENSE")
    repository_ctx.file("BUILD.bazel", """
load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])
licenses(["notice"])

cc_library(
    name = "glaze",
    hdrs = glob(["include/glaze/**/*.h", "include/glaze/**/*.hpp"]),
    includes = ["include"],
)
""")

def _googletest(repository_ctx, source):
    repository_ctx.symlink(source + "/googletest", "googletest")
    repository_ctx.symlink(source + "/googlemock", "googlemock")
    repository_ctx.symlink(source + "/LICENSE", "LICENSE")
    repository_ctx.file("BUILD.bazel", """
load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])
licenses(["notice"])

cc_library(
    name = "gtest",
    srcs = glob(
        [
            "googletest/src/*.cc",
            "googletest/src/*.h",
            "googletest/include/gtest/**/*.h",
            "googlemock/src/*.cc",
            "googlemock/include/gmock/**/*.h",
        ],
        exclude = [
            "googletest/src/gtest-all.cc",
            "googletest/src/gtest_main.cc",
            "googlemock/src/gmock-all.cc",
            "googlemock/src/gmock_main.cc",
        ],
    ),
    hdrs = glob([
        "googletest/include/gtest/*.h",
        "googlemock/include/gmock/*.h",
    ]),
    copts = ["-pthread"],
    includes = [
        "googlemock",
        "googlemock/include",
        "googletest",
        "googletest/include",
    ],
    linkopts = ["-pthread"],
)

cc_library(
    name = "gtest_main",
    srcs = ["googlemock/src/gmock_main.cc"],
    deps = [":gtest"],
)
""")

def _google_benchmark(repository_ctx, source):
    repository_ctx.symlink(source + "/include", "include")
    repository_ctx.symlink(source + "/src", "src")
    repository_ctx.symlink(source + "/LICENSE", "LICENSE")
    repository_ctx.file("BUILD.bazel", """
load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])
licenses(["notice"])

cc_library(
    name = "benchmark",
    srcs = glob(["src/*.cc", "src/*.h"], exclude = ["src/benchmark_main.cc"]),
    hdrs = ["include/benchmark/benchmark.h", "include/benchmark/export.h"],
    copts = [
        "-pedantic",
        "-std=c++17",
        "-Wall",
        "-Wextra",
        "-Wno-unused-variable",
    ],
    defines = ["BENCHMARK_STATIC_DEFINE", "BENCHMARK_VERSION=\\\\\\\"1.9.5\\\\\\\""],
    includes = ["include"],
    linkopts = ["-pthread"],
    linkstatic = True,
    local_defines = ["_FILE_OFFSET_BITS=64", "_LARGEFILE64_SOURCE", "_LARGEFILE_SOURCE"],
)

cc_library(
    name = "benchmark_main",
    srcs = ["src/benchmark_main.cc"],
    hdrs = ["include/benchmark/benchmark.h", "include/benchmark/export.h"],
    includes = ["include"],
    deps = [":benchmark"],
)
""")

def _nix_source_repository_impl(repository_ctx):
    source = _require_source(repository_ctx)
    if repository_ctx.attr.kind == "glaze":
        _glaze(repository_ctx, source)
    elif repository_ctx.attr.kind == "googletest":
        _googletest(repository_ctx, source)
    elif repository_ctx.attr.kind == "google_benchmark":
        _google_benchmark(repository_ctx, source)
    else:
        fail("BUILD_ACQUISITION_DENIED: unknown Nix source kind %s" % repository_ctx.attr.kind)

nix_source_repository = repository_rule(
    implementation = _nix_source_repository_impl,
    attrs = {
        "kind": attr.string(mandatory = True),
        "source_env": attr.string(mandatory = True),
    },
    environ = [
        "ORUS_BENCHMARK_SRC",
        "ORUS_GLAZE_SRC",
        "ORUS_GOOGLETEST_SRC",
    ],
    local = True,
)

def _nix_python_repository_impl(repository_ctx):
    interpreter = repository_ctx.os.environ.get(repository_ctx.attr.interpreter_env)
    if not interpreter or not interpreter.startswith("/nix/store/") or not interpreter.endswith("/bin/python3"):
        fail("BUILD_UNDECLARED_INPUT: ORUS_PYTHON must name the pinned Nix Python executable")
    repository_ctx.file(
        "coverage_main.py",
        "from coverage.cmdline import main\nraise SystemExit(main())\n",
    )
    repository_ctx.file("BUILD.bazel", """
load("@rules_python//python:py_runtime.bzl", "py_runtime")
load("@rules_python//python:py_runtime_pair.bzl", "py_runtime_pair")
load("@rules_python//python/private:py_exec_tools_toolchain.bzl", "py_exec_tools_toolchain")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "coverage_tool",
    srcs = ["coverage_main.py"],
)

py_runtime(
    name = "py3_runtime",
    coverage_tool = ":coverage_tool",
    implementation_name = "cpython",
    interpreter_path = "%s",
    interpreter_version_info = {
        "major": "3",
        "micro": "6",
        "minor": "14",
    },
    python_version = "PY3",
    stub_shebang = "#!%s",
)

py_runtime_pair(
    name = "runtime_pair",
    py3_runtime = ":py3_runtime",
)

py_exec_tools_toolchain(
    name = "exec_tools",
    precompiler = "@rules_python//tools/precompiler:precompiler",
)

toolchain(
    name = "exec_tools_toolchain",
    toolchain = ":exec_tools",
    toolchain_type = "@rules_python//python:exec_tools_toolchain_type",
)

toolchain(
    name = "toolchain",
    toolchain = ":runtime_pair",
    toolchain_type = "@rules_python//python:toolchain_type",
)
""" % (interpreter, interpreter))

nix_python_repository = repository_rule(
    implementation = _nix_python_repository_impl,
    attrs = {
        "interpreter_env": attr.string(mandatory = True),
    },
    environ = ["ORUS_PYTHON"],
    local = True,
)
