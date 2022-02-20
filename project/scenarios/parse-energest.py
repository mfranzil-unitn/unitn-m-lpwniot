import sys

# Get filename from command line argument
filename = sys.argv[1]

with open(filename.replace('.csv', '-merge.csv'), 'w') as o:
    with open(filename, 'r') as f:
        lines = f.readlines()[1:]
        # This file is a csv, each line is in the following fields:
        # time, node, cnt, cpu, lpm, tx, rx

        data = {}
        for line in lines:
            # Split the line into fields
            fields = line.split(',')
            node = int(fields[1])

            if node not in data:
                data[node] = {'cnt': 0, 'cpu': 0, 'lpm': 0, 'tx': 0, 'rx': 0}

            data[node]['cnt'] += 1
            data[node]['cpu'] += float(fields[3])
            data[node]['lpm'] += float(fields[4])
            data[node]['tx'] += float(fields[5])
            data[node]['rx'] += float(fields[6])

        o.write("node,cnt,cpu,lpm,tx,rx\n")

        for node in data:
            # Obtain averages and print them
            data[node]['cpu'] /= data[node]['cnt']
            data[node]['lpm'] /= data[node]['cnt']
            data[node]['tx'] /= data[node]['cnt']
            data[node]['rx'] /= data[node]['cnt']

            o.write(f'{node},{data[node]["cnt"]},{data[node]["cpu"]},'
                    f'{data[node]["lpm"]},{data[node]["tx"]},{data[node]["rx"]}\n')
