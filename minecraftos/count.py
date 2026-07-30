from collections import Counter
c = Counter()
with open("output.ppm") as f:
    lines = f.readlines()[3:]
    data = []
    for l in lines:
        data.extend(l.split())
    colors = [int(data[i]) for i in range(0, len(data), 3)]
    for col in colors:
        c[col] += 1
print(c)
