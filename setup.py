from setuptools import setup, Extension
from os.path import join, abspath, dirname
pwd = abspath(dirname(__file__))
with open(
  join(pwd,
       'README.md'),
  encoding='utf-8'
) as f:
  long_description = f.read()
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
  version='1.6',
  long_description_content_type='text/markdown',
  long_description=long_description,
  author_email='zsp042@gmail.com',
  url="https://github.com/txtcn/lossycount",
  zip_safe=False,
  ext_modules=[
    Extension(
      'lossycount',
      [
        'src/wrap.cc',
        'src/rand48.cc',
        'src/qdigest.cc',
        'src/prng.cc',
        'src/lossycount.cc'
      ],
      extra_compile_args=[
        '-O3',
        '-pipe',
        '-DNDEBUG',
        '-fomit-frame-pointer',
      ],
      libraries=['boost_python3'],
      language='c++',
    ),
  ],
)
