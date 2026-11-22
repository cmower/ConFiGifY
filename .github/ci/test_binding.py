"""Run one binding's conformance tests from a GitHub-hosted runner or local checkout."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
FIXTURE = "tests/conformance.cfgy"
BINDINGS = ("c", "cpp", "csharp", "go", "java", "julia", "lua", "octave", "python", "rust")


def run(*args, cwd=None):
    print("+", *args, flush=True)
    subprocess.run(args, cwd=cwd, check=True)


def build_native(binding, build):
    windows_gnu = os.name == "nt" and binding in ("go", "lua", "octave")
    visual_studio = os.name == "nt" and not windows_gnu
    native_test = binding in ("c", "cpp")
    options = ["-DBUILD_SHARED_LIBS=ON", f"-DBUILD_TESTING={'ON' if native_test else 'OFF'}"]
    if visual_studio:
        options.extend(["-G", "Visual Studio 17 2022", "-A", "x64"])
    else:
        options.append("-DCMAKE_BUILD_TYPE=Release")
        if windows_gnu:
            options.extend(["-G", "Ninja", "-DCMAKE_C_COMPILER=gcc", "-DCMAKE_CXX_COMPILER=g++"])
    if binding in ("java", "lua"):
        options.append(f"-DCFGY_{binding.upper()}=ON")

    targets = {
        "c": ["cfgy-c-test", "example-c"],
        "cpp": ["cfgy-test", "example"],
        "java": ["cfgy_java", "cfgy_jni"],
        "lua": ["cfgy_lua"],
    }
    run("cmake", "-S", ".", "-B", build, *options)
    run("cmake", "--build", build, "--config", "Release", "--parallel", "2",
        "--target", *targets.get(binding, ["cfgy"]))
    return build / "Release" if visual_studio else build


def test(binding):
    os.chdir(ROOT)
    if binding == "python":
        run(sys.executable, "-m", "pip", "install", ".")
        run(sys.executable, "tests/python/test_configify.py")
        return

    build = ROOT / "build" / "ci" / binding
    lib = build_native(binding, build)
    loader = {"darwin": "DYLD_LIBRARY_PATH", "win32": "PATH"}.get(sys.platform, "LD_LIBRARY_PATH")
    previous = os.environ.get(loader)
    os.environ[loader] = str(lib) + (os.pathsep + previous if previous else "")

    if binding in ("c", "cpp"):
        pattern = "^cfgy-(c-api|example-c-cli)$" if binding == "c" else "^cfgy-(parser|example-cli)$"
        run("ctest", "--test-dir", build, "-C", "Release", "--output-on-failure", "--no-tests=error", "-R", pattern)
    elif binding == "csharp":
        run("dotnet", "run", "--project", "tests/csharp", "--configuration", "Release", "--", FIXTURE)
    elif binding == "go":
        os.environ["CGO_ENABLED"] = "1"
        os.environ["CGO_LDFLAGS"] = f'"-L{lib.as_posix()}"'
        run("go", "test", "-count=1", "./...", cwd=ROOT / "src/go")
    elif binding == "java":
        run("java", f"-Djava.library.path={lib}", "-cp", build / "cfgy.jar", "tests/java/Check.java", FIXTURE)
    elif binding == "julia":
        run("julia", "--project=src/julia", "tests/julia/check.jl", FIXTURE)
    elif binding == "lua":
        suffix = "dll" if os.name == "nt" else "so"
        os.environ["LUA_CPATH"] = f"{build.as_posix()}/lua/?.{suffix};;"
        run(shutil.which("lua5.4") or "lua", "tests/lua/check.lua", FIXTURE)
    elif binding == "octave":
        mex = build / "cfgy_native.mex"
        rpath = [] if os.name == "nt" else [f"-Wl,-rpath,{lib}"]
        run("mkoctfile", "--mex", "-Isrc/c", "src/matlab/native.c", f"-L{lib}", "-lcfgy", *rpath, "-o", mex)
        run("octave-cli", "--quiet", "--no-init-file", "--eval",
            f"addpath('src/matlab','{build.relative_to(ROOT).as_posix()}','tests/matlab'); check_cfgy('{FIXTURE}');")
    elif binding == "rust":
        os.environ["CFGY_LIB_DIR"] = str(lib)
        run("cargo", "test", "--manifest-path", "src/rust/Cargo.toml")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binding", choices=BINDINGS)
    test(parser.parse_args().binding)
