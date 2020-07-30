# Finding Frequent Items 频繁集挖掘

## Install

```
apt-get install -y libboost-python-dev
pip install lossycount
```

if cannot find -lboost_python3

occurred.

Then I went to

/usr/lib/x86_64-linux-gnu

search and found that the library file is in different name as

libboost_python-py35.so

so I made a link by following command

sudo ln -s libboost_python-py35.so libboost_python3.so 
which solved my problem.

## Use
```
from lossycount import LossyCount

# 0.001 是要统计的频率下限
lc = LossyCount(0.001)

for i in range(200):
  for j in range(100):
    for k in range(j):
      lc.update(j, 1)

for i in range(1, 100, 30):
  print(i)
  print("出现的次数(估计值)", lc.est(i))
  print(
    "estimate the worst case error in the estimate of a) particular item :",
    lc.err(i)
  )
  print("---" * 20)

result = lc.output(1000)
result.sort(key=lambda x: -x[1])
print(result)
```

## Thanks

[hadjieleftheriou.com/frequent-items](http://hadjieleftheriou.com/frequent-items/index.html)

version 1.0

## Description

This package provides implementations of various one pass algorithms for finding frequent items in data streams. In particular it contains the following:

* Frequent Algorithm
* Lossy Counting, and variations
* Space Saving
* Greewald & Khanna
* Quantile Digest
* Count Sketch
* Hierarchical Count-Min Sketch
* Combinatorial Group Testing


The code is an extension of the MassDAL library. Implementations are by Graham Cormode.


## Citations

* [Finding Frequent Items in Data Streams](./Finding_Frequent_Items_in_Data_Streams.pdf)

  G. Cormode, M. Hadjieleftheriou

  Proc. of the International Conference on Very Large Data Bases (VLDB)

  Auckland, New Zealand, August 2008.
