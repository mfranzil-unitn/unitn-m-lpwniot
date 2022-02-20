import os
import sys
import re
import csv
import argparse
import pandas as pd
import numpy as np
import deploymentMap_dwm as dwm_nodes
import deploymentMap_evb as evb_nodes


def distance(a, b):
    return (
        ((a[0] - b[0]) ** 2) +
        ((a[1] - b[1]) ** 2)
    ) ** 0.5


def parse_file(log_file, results_file, nodes, fixed_dist=None):
    columns = ['init', 'resp', 'seqn', 'time', 'success', 'dist', 'dist_bias',
               'err', 'err_bias', 'dist_ground_truth',
               'fp_pwr', 'rx_pwr', 'freq_offset']
    outfile = open(results_file, 'w')
    writer = csv.writer(outfile, dialect='excel')
    writer.writerow(columns)
    columns = ['node', 'seqn', 'time', 'temp', 'volts']
    tvbat_outfile = open('tvbat.csv', 'w')
    tvbat_writer = csv.writer(tvbat_outfile, dialect='excel')
    tvbat_writer.writerow(columns)

    idByAddr = {v:k for k, v in nodes.addrById.items()}
    if fixed_dist is None:
        coordById = nodes.coordById

    # regular expressions to match
    # RNG [%lu/%lums] %x->%x SUCCESS %d bias %d fppwr %d rxpwr %d cifo %d"

    regex_rng = re.compile(r".*RNG \[(?P<seqn>\d+)/(?P<ms>\d+)ms\] "
                           r"(?P<init>\w\w\w\w)->(?P<resp>\w\w\w\w): SUCCESS "
                           r"(?P<dist>-?\d+) bias (?P<dist_bias>-?\d+) "
                           r"fppwr (?P<fppwr>-?\d+) "
                           r"rxpwr (?P<rxpwr>-?\d+) "
                           r"cifo (?P<cifo>-?\d+).*\n")

    regex_no = re.compile(r".*RNG \[(?P<seqn>\d+)/(?P<ms>\d+)ms\] "
                          r"(?P<init>\w\w\w\w)->(?P<resp>\w\w\w\w):.*FAIL.*\n")

    regex_tvbat = re.compile(r".*TVBAT \[(?P<seqn>\d+)/(?P<ms>\d+)ms\] "
                             r"(?P<init>\w\w\w\w)->(?P<resp>\w\w\w\w) "
                             r"(?P<millivolts>-?\d+)mV "
                             r"(?P<millicelsius>-?\d+)mC.*\n")

    # open log and read line by line
    with open(log_file, 'r') as f:
        for line in f:

            # match transmissions strings
            m = regex_rng.match(line)
            if m:

                # get dictionary of matched groups
                d = m.groupdict()

                seqn = int(d['seqn'])
                time = int(d['ms']) / 1000

                # retrieve IDs of nodes and the measured distance
                init = idByAddr[d['init'][:2] + ':' + d['init'][2:]]
                resp = idByAddr[d['resp'][:2] + ':' + d['resp'][2:]]
                dist = int(d['dist']) / 100.0
                dist_bias = int(d['dist_bias']) / 100.0
                fp_pwr = float(d['fppwr']) / 1000.0
                rx_pwr = float(d['rxpwr']) / 1000.0
                cifo = float(d['cifo']) / 1000.0  # ppm

                # retrieve coordinates
                if fixed_dist is not None:
                    true_dist = float(fixed_dist)
                else:
                    init_coords = coordById[init]
                    resp_coords = coordById[resp]
                    true_dist = distance(init_coords, resp_coords)

                # compute the error
                err = abs(dist - true_dist)
                err_bias = abs(dist_bias - true_dist)
                print(f"Ranging error [{init}->{resp}] "
                      f"{err} bias {err_bias} "
                      f"(dist {dist}/{dist_bias} true dist {true_dist})")

                log_row = [init, resp, seqn, time, True, dist, dist_bias,
                           err, err_bias, true_dist,
                           fp_pwr, rx_pwr, cifo]
                writer.writerow([format(x, '.2f') for x in log_row])
                continue

            m = regex_no.match(line)
            if m:

                # get dictionary of matched groups
                d = m.groupdict()

                seqn = int(d['seqn'])
                time = int(d['ms']) / 1000

                # retrieve IDs of nodes
                init = idByAddr[d['init'][:2] + ':' + d['init'][2:]]
                resp = idByAddr[d['resp'][:2] + ':' + d['resp'][2:]]

                # retrieve coordinates
                if fixed_dist is not None:
                    true_dist = float(fixed_dist)
                else:
                    init_coords = coordById[init]
                    resp_coords = coordById[resp]
                    true_dist = distance(init_coords, resp_coords)

                # print(f"Ranging failed [{init}->{resp}] ")

                log_row = [init, resp, seqn, time, False, np.nan, np.nan,
                           np.nan, np.nan, true_dist,
                           np.nan, np.nan, np.nan]
                writer.writerow([format(x, '.2f') for x in log_row])
                continue

            m = regex_tvbat.match(line)
            if m:

                # get dictionary of matched groups
                d = m.groupdict()

                seqn = int(d['seqn'])
                time = int(d['ms']) / 1000

                # retrieve IDs of the node
                init = idByAddr[d['init'][:2] + ':' + d['init'][2:]]

                # temperature and voltage
                millicelsius = int(d['millicelsius'])
                millivolts = int(d['millivolts'])

                log_row = [init, seqn, time,
                           millicelsius / 1000.0, millivolts / 1000.0]

                tvbat_writer.writerow([format(x, '.3f') for x in log_row])

    outfile.close()
    tvbat_outfile.close()


def std(x):
    return np.std(x)


def percentile(n):
    def percentile_(x):
        return np.nanpercentile(x, n)
    percentile_.__name__ = f'{n}'
    return percentile_


def analyze(results_file, analysis_file, ignore_seqn=0,
            ignore_seconds=0, max_rangings=None):
    df = pd.read_csv(results_file)
    df = df[df.seqn >= ignore_seqn]
    df = df[df.time >= ignore_seconds]
    # df = df.dropna()
    df['count'] = 1
    print(df)

    # take only the first max_rangings
    if max_rangings is not None:
        df_pairs = df.groupby(['init', 'resp']).count().reset_index()
        df_cut = pd.DataFrame(columns=df.columns)
        for index, row in df_pairs.iterrows():
            init = row['init']
            resp = row['resp']
            df_tmp = df[df['init'] == init]
            df_tmp = df_tmp[df_tmp['resp'] == resp]
            r = df_tmp.head(max_rangings).reset_index()
            df_cut = df_cut.append(r, ignore_index=True)
        df = df_cut

    # make DataFrame numeric
    #df[df.columns] = df[df.columns].apply(pd.to_numeric, errors='coerce', axis=1)
    print(df)

    # group and aggregate
    df = df.groupby(['init', 'resp', 'dist_ground_truth'])
    df = df[['count', 'time', 'success',
             'dist', 'dist_bias', 'err', 'err_bias',
             'fp_pwr', 'rx_pwr', 'freq_offset']]
    df = df.agg({
        'count': 'sum',
        'time': ['min', 'max'],
        'success': ['sum', 'mean'],
        'dist': ['mean', 'std',
                 percentile(50),
                 percentile(75),
                 percentile(95),
                 percentile(99)],
        'dist_bias': ['mean', 'std',
                      percentile(50),
                      percentile(75),
                      percentile(95),
                      percentile(99)],
        'err': ['mean', 'std',
                percentile(50),
                percentile(75),
                percentile(95),
                percentile(99)],
        'err_bias': ['mean', 'std',
                     percentile(50),
                     percentile(75),
                     percentile(95),
                     percentile(99)],
        'fp_pwr': ['mean', 'std'],
        'rx_pwr': ['mean', 'std'],
        'freq_offset': ['mean', 'std']
    }).reset_index()
    df.columns = ['_'.join(col).strip(' _') for col in df.columns.values]

    summary_row = []
    for col in df.columns:
        summary_row.append(df[col].mean())
    summary_row[0] = None
    summary_row[1] = None
    # print(summary_row)
    df.loc[len(df)] = summary_row

    df = df.round(3)

    df.to_csv(analysis_file, index=False)


def tvbat_analyze(tvbat_file, analysis_file, ignore_seqn=0,
                  ignore_seconds=0):
    df = pd.read_csv(tvbat_file)
    df = df[df.seqn >= ignore_seqn]
    df = df[df.time >= ignore_seconds]
    df = df[df.temp >= -20]  # remove wrong readings
    # df = df.dropna()
    df['count'] = 1

    if df.shape[0] < 1:
        print("No temp./voltage data to analyze.")
        return

    df = df.groupby(['node'])
    df = df[['count', 'time', 'temp', 'volts']]
    df = df.agg({
        'count': 'sum',
        'time': ['min', 'max'],
        'temp': ['mean', 'std',
                 percentile(50),
                 percentile(75),
                 percentile(95),
                 percentile(99)],
        'volts': ['mean', 'std',
                  percentile(50),
                  percentile(75),
                  percentile(95),
                  percentile(99)]
    }).reset_index()
    df.columns = ['_'.join(col).strip(' _') for col in df.columns.values]

    summary_row = []
    for col in df.columns:
        summary_row.append(df[col].mean())
    summary_row[0] = None
    # print(summary_row)
    df.loc[len(df)] = summary_row

    df = df.round(6)

    df.to_csv(analysis_file, index=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("log_file", help="The path to ranging logs")
    parser.add_argument("--ignore_seqn", "-i",
                        help="How many initial sequence numbers to"
                             "ignore in analysis",
                        default=0)
    parser.add_argument("--ignore_seconds", "-s",
                        help="Ignore rangings done before the time"
                             "mark, in seconds",
                        default=0)
    parser.add_argument("--max", "-m",
                        help="Stop analysis after max ranging"
                             "exchanges per link",
                        default=0)
    parser.add_argument("--dwm", "-d",
                        help="If logs is obtained from DWMs",
                        action="store_true", default=False)
    parser.add_argument("--gt", "-g",
                        help="Fixed ground truth distance to be "
                             "used if node position cannot be found",
                        default=None)
    args = parser.parse_args(sys.argv[1:])
    print(args.log_file)

    # parse and analyze
    results_file = 'ranging_results.csv'
    analysis_file = 'ranging_analysis.csv'
    nodes = dwm_nodes if args.dwm else evb_nodes
    max_rangings = int(args.max) if args.max else None
    parse_file(args.log_file, results_file, nodes, fixed_dist=args.gt)
    analyze(results_file, analysis_file,
            ignore_seqn=int(args.ignore_seqn),
            ignore_seconds=int(args.ignore_seconds),
            max_rangings=max_rangings)
    tvbat_analyze('tvbat.csv', 'tvbat_analysis.csv',
                  ignore_seqn=int(args.ignore_seqn),
                  ignore_seconds=int(args.ignore_seconds))
