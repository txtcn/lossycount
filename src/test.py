#!/usr/bin/env python

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

print(lc.capacity())
