import os
import sys

import matplotlib.pyplot as plt
import numpy as np

FIGSIZE_WIDTH = 8
FIGSIZE_HEIGHT = 6
FIG_DPI = 300

def draw(mergename):
    folder_name = 'results/' + mergename
    result_folder = '../report/drawable/' + mergename

    if not os.path.exists(result_folder):
        os.mkdir(result_folder)

    # Draw DC graph

    with open(folder_name + "/" + mergename + "-dc.csv", 'r') as f:
        lines = f.readlines()[1:]
        x = []
        names = []
        for line in lines:
            line = line.strip().split(',')
            names.append(line[0].split(".")[0])
            x.append(float(line[1]))
        plt.figure(figsize=(FIGSIZE_WIDTH, FIGSIZE_HEIGHT), dpi=FIG_DPI)
        plt.bar(names, x, align='center')
        plt.xlabel('Node ID')
        plt.ylabel('DC (%)')
        plt.title('Duty Cycle')
        plt.savefig(result_folder + '/' + mergename + '-dc.png', dpi=FIG_DPI)
        plt.close()

    # Draw Energest graphs

    with open(folder_name + "/" + mergename + "-energest-merge.csv", 'r') as f:
        lines = f.readlines()[1:]

        cpu = []
        lpm = []
        tx = []
        rx = []

        names = []
        for line in lines:
            line = line.strip().split(',')
            names.append(int(line[0].split(".")[0]))
            cpu.append(float(line[2]))
            lpm.append(float(line[3]))
            tx.append(float(line[4]))
            rx.append(float(line[5]))

        cpu = np.asarray(cpu)
        lpm = np.asarray(lpm)
        tx = np.asarray(tx)
        rx = np.asarray(rx)

        fig, ax = plt.subplots()
        fig.set_size_inches(FIGSIZE_WIDTH, FIGSIZE_HEIGHT)
        fig.set_dpi(FIG_DPI)
        ax.bar(names, lpm, label="LPM")
        ax.bar(names, cpu, label='CPU', bottom=lpm)
        ax.set_yscale('log')
        ax.set_ylabel('Energy (mW)')
        ax.set_title('Energy consumption estimate (CPU vs LPM, log scale)')
        ax.legend()
        plt.savefig(result_folder + '/' + mergename + '-energest-merged-cpu-lpm.png', dpi=FIG_DPI)
        plt.close()

        fig, ax = plt.subplots()
        fig.set_size_inches(FIGSIZE_WIDTH, FIGSIZE_HEIGHT)
        fig.set_dpi(FIG_DPI)
        ax.bar(names, tx, label="TX")
        ax.bar(names, rx, label='RX', bottom=tx)
        ax.set_ylabel('Energy (mW)')
        ax.set_title('Energy consumption estimate (TX vs RX)')
        ax.legend()
        plt.savefig(result_folder + '/' + mergename + '-energest-merged-tx-rx.png', dpi=FIG_DPI)
        plt.close()

        fig, ax = plt.subplots()
        fig.set_size_inches(FIGSIZE_WIDTH, FIGSIZE_HEIGHT)
        fig.set_dpi(FIG_DPI)
        ax.bar(names, tx, label="TX")
        ax.set_ylabel('Energy (mW)')
        ax.set_title('Energy consumption estimate (TX)')
        ax.legend()
        plt.savefig(result_folder + '/' + mergename + '-energest-tx.png', dpi=FIG_DPI)
        plt.close()

        fig, ax = plt.subplots()
        fig.set_size_inches(FIGSIZE_WIDTH, FIGSIZE_HEIGHT)
        fig.set_dpi(FIG_DPI)
        ax.bar(names, rx, label="RX")
        ax.set_ylabel('Energy (mW)')
        ax.set_title('Energy consumption estimate (RX)')
        ax.legend()
        plt.savefig(result_folder + '/' + mergename + '-energest-rx.png', dpi=FIG_DPI)
        plt.close()

        fig, ax = plt.subplots()
        fig.set_size_inches(FIGSIZE_WIDTH, FIGSIZE_HEIGHT)
        fig.set_dpi(FIG_DPI)
        ax.bar(names, lpm, label="LPM")
        ax.set_ylabel('Energy (mW)')
        ax.set_title('Energy consumption estimate (LPM)')
        # ax.set_ylim(min=np.mean(lpm)/2)
        ax.legend()
        plt.savefig(result_folder + '/' + mergename + '-energest-lpm.png', dpi=FIG_DPI)
        plt.close()

        fig, ax = plt.subplots()
        fig.set_size_inches(FIGSIZE_WIDTH, FIGSIZE_HEIGHT)
        fig.set_dpi(FIG_DPI)
        ax.bar(names, cpu, label="CPU")
        ax.set_ylabel('Energy (mW)')
        ax.set_title('Energy consumption estimate (CPU)')
        ax.legend()
        plt.savefig(result_folder + '/' + mergename + '-energest-cpu.png', dpi=FIG_DPI)
        plt.close()


if __name__ == "__main__":
    # mergename = sys.argv[1]
    mergenames = os.listdir("results")
    for name in mergenames:
        draw(name)
