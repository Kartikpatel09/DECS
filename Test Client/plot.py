import matplotlib.pyplot as plt

i=0
arr = []

file = open("results.txt", "r")

for line in file:
    arr.append(float(line.strip()))

start=5
end=76
skip=5
plt.figure()
plt.plot(range(start, end, skip), arr, color="r")
plt.xlabel("Concurrent clients")
plt.ylabel("Average Response Time")

plt.savefig("plot.jpg")