from setuptools import setup, Extension
from os.path import join, abspath, dirname
pwd = abspath(dirname(__file__))
with open(
  join(pwd,
       'README.md'),
  encoding='utf-8'
) as f:
  long_description = f.read()

with open(join(pwd, 'requirements.txt')) as f:
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
  version='1.3',
  long_description_content_type='text/markdown',
  long_description=long_description,
  author_email='zsp042@gmail.com',
  url="https://github.com/txtcn/lossycount",
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
      zip_safe=False,
      libraries=['boost_python3'],
      language='c++',
    ),
  ],
)
