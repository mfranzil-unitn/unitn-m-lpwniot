#!/usr/bin/env python3
import os
import sys
import re
from deploymentMap import addrById, idByAddr, coordById

def distance(a, b):
    return (
        ((a[0] - b[0]) ** 2) +
        ((a[1] - b[1]) ** 2)
    ) ** 0.5


def parse_file(log_file):

    # regular expressions to match
    # Ex.: RANGING OK [11:0c->19:15] 169 bias 199
    regex_rng = re.compile(r".*RANGING OK \[(?P<init>\w\w:\w\w)->(?P<resp>\w\w:\w\w)\] (?P<dist>\d+) mm.*\n")

    # open log and read line by line
    with open(log_file, 'r') as f:
        for line in f:

            # match transmissions strings
            m = regex_rng.match(line)
            if m:

                # get dictionary of matched groups
                d = m.groupdict()

                # retrieve IDs of nodes and the measured distance
                init = idByAddr[d['init']]
                resp = idByAddr[d['resp']]
                dist = int(d['dist'])

                # retrieve coordinates
                init_coords = coordById[init]
                resp_coords = coordById[resp]

                # compute the error
                true_dist = int(distance(init_coords, resp_coords) * 1000) # mm
                err = int(dist - true_dist)
                print(f"Ranging error [{init}->{resp}] {err} (dist {dist} true dist {true_dist})")

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
