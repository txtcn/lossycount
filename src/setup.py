from setuptools import setup, Extension

import sys
with open("../readme.md") as f:
  long_description = f.read()

with open('../requirements.txt') as f:
  install_requires = f.read().splitlines()
"""
CXXFLAGS=-O2 -DNDEBUG -fPIC
CXX=g+
OBJECTS=rand48.o qdigest.o prng.o lossycount.o gk.o frequent.o countmin.o cgt.o ccfc.o
all: $(OBJECTS)
    $(CXX) $(CXXFLAGS) -shared wrap.cc $(OBJECTS) -o Release/lossycount.so -lboost_python
    rm -rf *.o
$(OBJECTS): rand48.h qdigest.h prng.h lossycount.h gk4.h frequent.h countmin.h cgt.h ccfc.h
    $(CXX) $(CXXFLAGS) -c $*.cc
"""
setup(
  name='lossycount',
  version='1.1',
  long_description_content_type='text/markdown',
  long_description=long_description,
  author_email='zsp042@gmail.com',
  url="https://github.com/txtcn/lossycount",
  ext_modules=[
    Extension(
      'lossycount',
      [
        'wrap.cc',
        'rand48.cc',
        'qdigest.cc',
        'prng.cc',
        'lossycount.cc'
      ],
      extra_compile_args=[
        '-O3',
        '-pipe',
        '-DNDEBUG',
        '-fomit-frame-pointer',
      ],
      libraries=[
        'boost_python%s%s' % sys.version_info[:2],
      ],
      language='c++',
    ),
  ],
)