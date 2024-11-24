import matplotlib.pyplot as plt

i=0
arr = []

file = open("results1.txt", "r")

for line in file:
    arr.append(float(line.strip()))

start=5
end=51
skip=5
plt.figure()
plt.plot(range(start, end, skip), arr, color="r")
plt.xlabel("Concurrent clients")
plt.ylabel("Average throughput")

plt.savefig("plot1.jpg")