#!/usr/bin/env python2.7
from __future__ import division

import re
import sys
import os.path
import argparse
from datetime import datetime

sink_id = 1

# Firefly addresses
addr_id_map = {
    "f7:9c":  1, "d9:76":  2, "f3:84":  3, "f3:ee":  4, "f7:92":  5,
    "f3:9a":  6, "de:21":  7, "f2:a1":  8, "d8:b5":  9, "f2:1e": 10,
    "d9:5f": 11, "f2:33": 12, "de:0c": 13, "f2:0e": 14, "d9:49": 15,
    "f3:dc": 16, "d9:23": 17, "f3:8b": 18, "f3:c2": 19, "f3:b7": 20,
    "de:e4": 21, "f3:88": 22, "f7:9a": 23, "f7:e7": 24, "f2:85": 25,
    "f2:27": 26, "f2:64": 27, "f3:d3": 28, "f3:8d": 29, "f7:e1": 30,
    "de:af": 31, "f2:91": 32, "f2:d7": 33, "f3:a3": 34, "f2:d9": 35,
    "d9:9f": 36, "f3:90": 50, "f2:3d": 51, "f7:ab": 52, "f7:c9": 53,
    "f2:6c": 54, "f2:fc": 56, "f1:f6": 57, "f3:cf": 62, "f3:c3": 63,
    "f7:d6": 64, "f7:b6": 65, "f7:b7": 70, "f3:f3": 71, "f1:f3": 72,
    "f2:48": 73, "f3:db": 74, "f3:fa": 75, "f3:83": 76, "f2:b4": 77
}


def parse_file(log_file, testbed=False):
    # Print some basic information for the user
    print(f"Logfile: {log_file}")
    print(f"{'Cooja simulation' if not testbed else 'Testbed experiment'}")

    # Create CSV output files
    fpath = os.path.dirname(log_file)
    fname_common = os.path.splitext(os.path.basename(log_file))[0]
    frecv_name = os.path.join(fpath, f"{fname_common}-recv.csv")
    fsent_name = os.path.join(fpath, f"{fname_common}-sent.csv")
    frecv = open(frecv_name, 'w')
    fsent = open(fsent_name, 'w')

    # Write CSV headers
    frecv.write("time\tdest\tsrc\tseqn\thops\n")
    fsent.write("time\tdest\tsrc\tseqn\n")

    if testbed:
        # Regex for testbed experiments
        testbed_record_pattern = r"\[(?P<time>.{23})\] INFO:firefly\.(?P<self_id>\d+): \d+\.firefly < b"
        regex_node = re.compile(r"{}'Rime configured with address "
                                r"(?P<src1>\d+).(?P<src2>\d+)'".format(testbed_record_pattern))
        regex_recv = re.compile(r"{}'App: Recv from (?P<src1>\w+):(?P<src2>\w+) "
                                r"seqn (?P<seqn>\d+) hops (?P<hops>\d+)'".format(testbed_record_pattern))
        regex_sent = re.compile(r"{}'App: Send seqn (?P<seqn>\d+)'".format(
            testbed_record_pattern))
    else:
        # Regular expressions --- different for COOJA w/o GUI
        record_pattern = r"(?P<time>[\w:.]+)\s+ID:(?P<self_id>\d+)\s+"
        regex_node = re.compile(r"{}Rime started with address "
                                r"(?P<src1>\d+).(?P<src2>\d+)".format(record_pattern))
        regex_recv = re.compile(r"{}App: Recv from (?P<src1>\w+):(?P<src2>\w+) "
                                r"seqn (?P<seqn>\d+) hops (?P<hops>\d+)".format(record_pattern))
        regex_sent = re.compile(r"{}App: Send seqn (?P<seqn>\d+)".format(
            record_pattern))

    # Node list and dictionaries for later processing
    nodes = []
    drecv = {}
    dsent = {}

    # Parse log file and add data to CSV files
    with open(log_file, 'r') as f:
        for line in f:
            # Node boot
            m = regex_node.match(line)
            if m:
                # Get dictionary with data
                d = m.groupdict()
                node_id = int(d["self_id"])
                # Save data in the nodes list
                if node_id not in nodes:
                    nodes.append(node_id)
                # Continue with the following line
                continue

            # RECV
            m = regex_recv.match(line)
            if m:
                # Get dictionary with data
                d = m.groupdict()
                if testbed:
                    ts = datetime.strptime(d["time"], '%Y-%m-%d %H:%M:%S,%f')
                    ts = ts.timestamp()
                    src_addr = "{}:{}".format(d["src1"], d["src2"])
                    try:
                        src = addr_id_map[src_addr]
                    except KeyError as e:
                        print("KeyError Exception: key {} not found in "
                              "addr_id_map".format(src_addr))

                else:
                    ts = d["time"]
                    # Discard second byte and convert to decimal
                    src = int(d["src1"], 16)
                dest = int(d["self_id"])
                seqn = int(d["seqn"])
                hops = int(d["hops"])
                # Write to CSV file
                frecv.write("{}\t{}\t{}\t{}\t{}\n".format(
                    ts, dest, src, seqn, hops))
                # Save data in the drecv dictionary for later processing
                if dest == sink_id:
                    drecv.setdefault(src, {})[seqn] = ts
                # Continue with the following line
                continue

            # SENT
            m = regex_sent.match(line)
            if m:
                d = m.groupdict()
                if testbed:
                    ts = datetime.strptime(d["time"], '%Y-%m-%d %H:%M:%S,%f')
                    ts = ts.timestamp()
                else:
                    ts = d["time"]
                src = int(d["self_id"])
                dest = 1
                seqn = int(d["seqn"])
                # Write to CSV file
                fsent.write("{}\t{}\t{}\t{}\n".format(ts, dest, src, seqn))
                # Save data in the dsent dictionary
                dsent.setdefault(src, {})[seqn] = ts

    # Analyze dictionaries and print some stats
    # Overall number of packets sent / received
    tsent = 0
    trecv = 0

    # Stop if there are no sent or received messages
    if not dsent:
        print("No sent messages could be parsed. Exiting...")
        sys.exit(1)
    if not drecv:
        print("No received messages could be parsed. Exiting...")
        sys.exit(1)

    # Nodes that did not manage to send data
    fails = []
    for node_id in sorted(nodes):
        if node_id == sink_id:
            continue
        if node_id not in dsent.keys():
            fails.append(node_id)
    if fails:
        print("----- WARNING -----")
        for node_id in fails:
            print("Warning: node {} did not send any data.".format(node_id))
        print("")  # To separate clearly from the following set of prints

    # Print node stats
    print("\n########## Node Statistics ##########\n")
    for node in sorted(dsent.keys()):
        nsent = len(dsent[node])
        nrecv = 0
        if node in drecv.keys():
            nrecv = len(drecv[node])

        pdr = 100 * nrecv / nsent
        print("Node {}: TX Packets = {}, RX Packets = {}, PDR = {:.2f}%, PLR = {:.2f}%".format(
            node, nsent, nrecv, pdr, 100 - pdr))

        # Update overall packets sent / received
        tsent += nsent
        trecv += nrecv

    # Print overall stats
    print("\n########## Overall Statistics ##########\n")
    print("Total Number of Packets Sent: {}".format(tsent))
    print("Total Number of Packets Received: {}".format(trecv))
    opdr = 100 * trecv / tsent
    print("Overall PDR = {:.2f}%".format(opdr))
    print("Overall PLR = {:.2f}%\n".format(100 - opdr))


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('logfile', action="store", type=str,
                        help="data collection logfile to be parsed and analyzed.")
    parser.add_argument('-t', '--testbed', action='store_true',
                        help="flag for testbed experiments")
    return parser.parse_args()


if __name__ == '__main__':
    args = parse_args()
    print(args)

    if not args.logfile:
        print("Log file needs to be specified as 1st positional argument.")
    if not os.path.exists(args.logfile):
        print("The logfile argument {} does not exist.".format(args.logfile))
        sys.exit(1)
    if not os.path.isfile(args.logfile):
        print("The logfile argument {} is not a file.".format(args.logfile))
        sys.exit(1)

    # Parse log file, create CSV files, and print some stats
    parse_file(args.logfile, testbed=args.testbed)
