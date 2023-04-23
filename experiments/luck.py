from random import random
from matplotlib import pyplot as plt

population = 1000
trials = 1000
bins = 20

results = [0] * population


for t in range(trials):
	for p in range(population):
		results[p] += random()


results.sort()

#fig, axs = plt.subplots(1, 2, sharey=True, tight_layout=True)


#axs[0].hist(results, bins=bins)
#axs[1].hist(range(100), bins=bins)
plt.hist(results, bins=bins);
plt.show()

"""
plt.plot(results)
plt.show()
"""
