import matplotlib.pyplot as plt
import re


def parse_line(line):
    line = line.replace('\n', '')
    data = line.strip().split()
    out = []
    ticks = int(data[0])
    for item in data[1:]:
        process_id = int(item.split('(')[0])
        qno = int(item.split('(')[1].split(',')[0])
        out.append((process_id, qno))
    return ticks, out


input_file = '/home/amey/Academics/OSN/project2/initial-xv6/src/time'
process_data = {}


with open(input_file, 'r') as file:
    for line in file:
        ticks, out = parse_line(line)
        for item in out:
            process_id, qno = item
            process_id = int(process_id)
            qno = int(qno)
            if process_id not in process_data:
                process_data[process_id] = {'time': [], 'qno': []}
            process_data[process_id]['time'].append(ticks)
            process_data[process_id]['qno'].append(qno)
        # process_data[time] = out

minproc = min(process_data.keys())
mintime = process_data[minproc]["time"][0]-1

for i in range(5):
    del process_data[minproc + i]

for item in process_data:
    for j in range(len(process_data[item]["time"])):
        process_data[item]["time"][j] = process_data[item]["time"][j] - mintime

# Create a multiline graph
plt.figure(figsize=(10, 6))
for process_id, data in process_data.items():
    process_id = process_id - minproc
    plt.plot(data['time'], data['qno'], label=f'Proc {process_id}')

plt.xlabel('No. of ticks elapsed')
plt.ylabel('Queue number')
plt.title('MLFQ Performance graph (Aging time: 30 ticks)')
plt.yticks([0, 1, 2, 3])
plt.legend()
plt.grid(True)

# Show the plot
plt.show()
