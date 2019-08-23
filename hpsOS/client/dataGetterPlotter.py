import numpy as np
import time
import subprocess
import os
import matplotlib.pyplot as plt

# adc generate clocks = bad

# 32ns = bad
# 33ns = good
# 34ns = good
# 35ns = good
# 36ns = bad

d = np.loadtxt("data_file.dat",delimiter=',')
t = np.linspace(0,len(d)/25.0,len(d))
print 'max :',np.max(d[0:-10,0]).astype(int)
print 'min :',np.min(d[0:-10,0]).astype(int)
print 'mean:',np.mean(d[0:-10,0]).astype(int)
for n in range(0,1):
	plt.plot(t[0:-10],d[0:-10,n]+4096*n)
plt.grid()
plt.show()
