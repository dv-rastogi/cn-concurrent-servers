import os
import matplotlib.pyplot as plt

response_time = {}
for filename in os.listdir("response_time"):
    with open(f'response_time/{filename}', 'r') as f:
        data = float(f.readline())
        num = int(filename[:filename.find('.')])
        print(f'num processes {num}, response time {data}s')
        response_time[num] = data

plt.scatter(response_time.keys(), response_time.values())
plt.xlabel('num processes')
plt.ylabel('avg. response time (s)')
plt.title('num processes vs response time')
plt.show()
