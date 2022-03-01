#!/usr/bin/env python3

import click
import functools
import os
import signal
import sys

print = functools.partial(print, flush=True)

PLANNER = 'planner'
DMC = 'dmc'
DPMC = 'dpmc'
DPMC_SOLVERS = (PLANNER, DMC, DPMC)

MAXHS = 'maxhs'
UWR = 'uwr'
SOLVERS = DPMC_SOLVERS + (MAXHS, UWR)

HTD = 'htd'
FLOW = 'flow'
DECOMPOSERS = (HTD, FLOW)

cnfFilePath = ''

treeWidths = []
treeTimes = []

def signalPids(pids, sig=signal.SIGTERM):
    for i in range(len(pids)):
        pid = pids[i]
        # printOut(f'pid{i + 1}', pid)
        try:
            os.kill(pid, sig)
        except ProcessLookupError:
            pass

def printOut(key, value): # for .out file
    print(f'{key}:{value}')

def printBaseName(benchmarkPath):
    printOut('base', os.path.splitext(os.path.basename(benchmarkPath))[0])

def printLogSol(exponent):
    printOut('logsol', exponent)

def printSol(s):
    printOut('sol', s)

def printTime(t): # runner
    printOut('time', t)

def printMem(m): # runner, in GB
    printOut('mem', m)

def printRunnerLine(line, solver):
    (k, v) = line.split('=')
    if k == 'WCTIME':
        time = float(v)
        if solver == DMC:
            printOut('exetime', time)
            if treeTimes:
                time += treeTimes[-1]
        printTime(time)
    elif k == 'MAXMM': # in KB; determines TIMEOUT (must not use MAXVM or MAXRSS)
        printMem(float(v) / 1e6)
    elif k == 'TIMEOUT':
        printOut('timeout', int(v == 'true'))
    elif k == 'MEMOUT':
        printOut('memout', int(v == 'true'))
    elif k == 'EXITSTATUS':
        printOut('exit', v)

def printTreeTimeLine(line):
    t = float(line.split()[-1])
    treeTimes.append(t)
    printOut(f'treetime{len(treeWidths)}', t)

def printDpmcLine(line, solver, pids):
    words = line.split()

    if line.startswith('c seconds') and solver == PLANNER:
        printTreeTimeLine(line)
        # signalPids(pids, signal.SIGKILL) # SIGTERM would result in duplicated tree

    elif line.startswith('c joinTreeWidth'): # planner or DMC
        width = int(words[-1])
        treeWidths.append(width)
        printOut(f'width{len(treeWidths)}', width)

    elif line.startswith('c plannerSeconds'): # DMC
        printTreeTimeLine(line)
    elif line.startswith('c diagramVarSeconds'):
        printOut('dvtime', words[-1])
    elif line.startswith('c sliceVarSeconds'):
        printOut('svtime', words[-1])
    elif line.startswith('c sliceAssignmentsSeconds'):
        printOut('satime', words[-1])
    elif line.startswith('c sliceWidth'):
        printOut('swidth', words[-1])
    elif line.startswith('c s log10-estimate'):
        printLogSol(words[-1])
    elif line.startswith('c s exact'):
        printSol(words[-1])

    elif line.startswith('c solutionMatch'):
        printOut('match', words[-1])
    elif line.startswith('c maximizerVerificationSeconds'):
        printOut('mvtime', words[-1])

    elif line.startswith('c apparentSolution'):
        printOut('applogsol', words[-1])
    elif line.startswith('c logBound'):
        printOut('logbound', words[-1])
    elif line.startswith('c prunedDdCount'):
        printOut('prunecount', words[-1])
    elif line.startswith('c pruningSeconds'):
        printOut('prunetime', words[-1])

def printTrees():
    if treeWidths:
        printOut('treecount', len(treeTimes))
        printOut('width', treeWidths[-1])
        printOut('treetime', treeTimes[-1])

def convertMaxSatOptimum(optimum):
    for line in open(os.path.join(os.path.dirname(__file__), cnfFilePath)):
        words = line.split()
        if line.startswith('p wcnf'):
            weightedVarCount = int(words[2])
        elif line.startswith('c weightedVarCount'): # only in new wcnf files
            weightedVarCount = int(words[-1])
        elif line.startswith('c softWeightSum'):
            softWeightSum = float(words[-1])
        elif line.startswith('c softWeightOffset'):
            softWeightOffset = float(words[-1])
        elif line.startswith('c logBase'):
            logBase = int(words[-1])
            assert logBase == 10
            break
    exponent = softWeightSum - optimum - weightedVarCount * softWeightOffset
    if exponent.is_integer():
        exponent = int(exponent)
    printLogSol(exponent)
    printSol(logBase**exponent)

@click.command(context_settings={'max_content_width': 90, 'help_option_names': ['-h', '--help']})
@click.option('--verbose', help='copies stdin to stderr', default=1, show_default=True)
def main(verbose):
    solver = '' # from wrapper.py
    pids = [] # from wrapper.py or LG
    solverEnded = False

    for line in sys.stdin:
        line = line.rstrip()
        words = line.split()

        if line.startswith('#'):
            if not solverEnded:
                solverEnded = True
                printTrees()
        elif solverEnded:
            printRunnerLine(line, solver)

        elif line.startswith('c cf'): # wrapper.py
            global cnfFilePath
            cnfFilePath = words[-1]
            printBaseName(cnfFilePath)
        elif line.startswith('c solver'): # wrapper.py
            solver = words[-1]
        elif line.startswith('c pid'): # wrapper.py or LG
            pids.append(int(words[-1]))

        elif solver in DPMC_SOLVERS:
            printDpmcLine(line, solver, pids)

        elif solver in {MAXHS, UWR}:
            if line.startswith('o '):
                optimum = float(words[1])
                convertMaxSatOptimum(optimum)

        if line.startswith('v ') and len(words) == 2:
            maximizer = words[1]
            varCount = len(maximizer)
            if varCount > 2e5:
                maximizer = -1
            printOut('model', maximizer)
            printOut('varcount', varCount)

        if verbose:
            print(line, file=sys.stderr)

    if not solverEnded: # in case runsolver fails
        printTrees()

if __name__ == '__main__':
    main()
