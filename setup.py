import os
from setuptools import Extension, setup

setup(ext_modules=[Extension(
    "_cfgy",
    sources=["src/python/native.cpp", "src/c/cfgy_c.cpp", "src/cpp/cfgy.cpp", "src/cpp/evaluation.cpp"],
    include_dirs=["src/c", "src/cpp"],
    depends=["src/c/cfgy.h", "src/cpp/cfgy", "src/cpp/evaluation.hpp"],
    language="c++",
    extra_compile_args=["/std:c++17" if os.name == "nt" else "-std=c++17"],
)])
