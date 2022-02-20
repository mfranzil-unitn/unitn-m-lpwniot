import os
import sys
import re
import statistics

# Firefly addresses
addr_id_map = {
    "f7:9c":  1,
    "d9:76":  2,
    "f3:84":  3,
    "f3:ee":  4,
    "f7:92":  5,
    "f3:9a":  6,
    "de:21":  7,
    "f2:a1":  8,
    "d8:b5":  9,
    "f2:1e": 10,
    "d9:5f": 11,
    "f2:33": 12,
    "de:0c": 13,
    "f2:0e": 14,
    "d9:49": 15,
    "f3:dc": 16,
    "d9:23": 17,
    "f3:8b": 18,
    "f3:c2": 19,
    "f3:b7": 20,
    "de:e4": 21,
    "f3:88": 22,
    "f7:9a": 23,
    "f7:e7": 24,
    "f2:85": 25,
    "f2:27": 26,
    "f2:64": 27,
    "f3:d3": 28,
    "f3:8d": 29,
    "f7:e1": 30,
    "de:af": 31,
    "f2:91": 32,
    "f2:d7": 33,
    "f3:a3": 34,
    "f2:d9": 35,
    "d9:9f": 36,
    "f3:90": 50,
    "f2:3d": 51,
    "f7:ab": 52,
    "f7:c9": 53,
    "f2:6c": 54,
    "f2:fc": 56,
    "f1:f6": 57,
    "f3:cf": 62,
    "f3:c3": 63,
    "f7:d6": 64,
    "f7:b6": 65,
    "f7:b7": 70,
    "f3:f3": 71,
    "f1:f3": 72,
    "f2:48": 73,
    "f3:db": 74,
    "f3:fa": 75,
    "f3:83": 76,
    "f2:b4": 77
}


def parse_file(log_file):
    tx = {}
    rx = {}
    rssi = {}

    # regular expressions to match
    regex_tx = re.compile(r".*TX (?P<addr>\w\w:\w\w).*\n")
    regex_rx = re.compile(r".*RX (?P<addr_from>\w\w:\w\w)->(?P<addr_to>\w\w:\w\w), RSSI = (?P<rssi>-?\d+)dBm.*\n")

    # open log and read line by line
    with open(log_file, 'r') as f:
        print(f"Parsing {log_file}")
        for line in f:
        
            # match transmissions strings
            m = regex_tx.match(line)
            if m:

                # get dictionary of matched groups
                d = m.groupdict()

                # increase the count of transmissions from this address
                tx_id = addr_id_map[d['addr']]
                tx[tx_id] = tx.get(tx_id, 0) + 1
                continue

            # match reception strings
            m = regex_rx.match(line)
            if m:

                # get dictionary of matched groups
                d = m.groupdict()

                # increase the count of receptions for the given link
                from_id = addr_id_map[d['addr_from']]
                to_id = addr_id_map[d['addr_to']]

                if (from_id, to_id) in rssi:
                    rssi[(from_id, to_id)].append(int(d['rssi']))
                elif (to_id, from_id) in rssi:
                    rssi[(to_id, from_id)].append(int(d['rssi']))
                else:
                    rssi[(from_id, to_id)] = [int(d['rssi'])]

                rx_update = rx.get(from_id, dict())
                rx_update[to_id] = rx_update.get(to_id, 0) + 1
                rx[from_id] = rx_update
                continue

            # no match
            # print(f"Line \"{line}\" not matched.")

    # diplay results
    for tx_id in tx:
        avg = []
        print(f"FROM {tx_id}\t\t{tx[tx_id]}")
        print(sum(rx[tx_id].values())/(len(rx[tx_id])*tx[tx_id]))
        for rx_id in rx[tx_id]:
            avg.append((rx[tx_id][rx_id])/tx[tx_id])
            print(f"\tTO {rx_id}\t{rx[tx_id][rx_id]}")
        print(f"AVG: {statistics.mean(avg)}")
        print("\n")

    # display RSSI results

    for rssi_res in rssi:
        print(f"{rssi_res}: {statistics.mean(rssi[rssi_res])}")



if __name__ == '__main__':

    if len(sys.argv) < 2:
        print("Error: Missing log file.")
        sys.exit(1)

    # get the log path and check that it exists
    log_file = sys.argv[1]
    if not os.path.isfile(log_file) or not os.path.exists(log_file):
        print("Error: Log file not found.")
        sys.exit(1)

    # parse file
    parse_file(log_file)
