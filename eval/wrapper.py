#!/usr/bin/env python3

import click
import functools
import math
import os
import subprocess
import sys

print = functools.partial(print, flush=True)

PLANNER = 'planner'
DMC = 'dmc'
DPMC = 'dpmc'
DPMC_SOLVERS = (PLANNER, DMC, DPMC)

GAUSS = 'gauss'
MAXHS = 'maxhs'
UWR = 'uwr'
SOLVERS = DPMC_SOLVERS + (GAUSS, MAXHS, UWR)

HTD = 'htd'
FLOW = 'flow'
DECOMPOSERS = (HTD, FLOW)

CUDD = 'c'
SYLVAN = 's'
DD_PACKAGES = (CUDD, SYLVAN)

def getAbsPath(*args):
    return os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), *args))

def getBinPath(fileName):
    return getAbsPath('bin', fileName)

def cat(cmd):
    assert isinstance(cmd, list), cmd
    return ' '.join(cmd)

def sh(cmd, **kwargs):
    subprocess.run(cat(cmd), shell=True, **kwargs)

def printPair(key, value): # for postprocessor.py
    print(f'c {key} {value}')

@click.command(context_settings={'max_content_width': 90, 'help_option_names': ['-h', '--help']})
@click.option('--cf', help='CNF file', required=True)
@click.option('--solver', help='planner, executor, or counter', type=click.Choice(SOLVERS), default=DPMC, show_default=True)
@click.option('--decomposer', help='planning technique', type=click.Choice(DECOMPOSERS), default=FLOW, show_default=True)
@click.option('--jf', help='JT file', default='', show_default=True)
@click.option('--sif', help='dmc.sif instead of dmc', default=0, show_default=True)
@click.option('--softmemcap', help='for solver, in GB', default=0., show_default=True)
@click.option('--memcap', help='for runsolver, in GB', default=0., show_default=True)
@click.option('--timecap', help='for runsolver, in seconds', default=0., show_default=True)
@click.option('--runner', help='runsolver', default=0, show_default=True)
@click.option('--tc', help='thread count', default=1, show_default=True)
@click.option('--wc', help='weighted counting', default=0, show_default=True)
@click.option('--pc', help='projected counting', default=0, show_default=True)
@click.option('--er', help='existential-randomized stochastic SAT', default=0, show_default=True)
@click.option('--lb', help='log10 of bound', default=-math.inf, show_default=True)
@click.option('--tm', help='threshold model', default='', show_default=True)
@click.option('--ep', help='existential pruning', default=0, show_default=True)
@click.option('--mf', help='maximizer format', default=0, show_default=True)
@click.option('--mv', help='maximizer verification', default=0, show_default=True)
@click.option('--sm', help='substitution-based maximization', default=0, show_default=True)
@click.option('--pw', help='planner wait duration', default=0., show_default=True)
@click.option('--dp', help='diagram package', type=click.Choice(DD_PACKAGES), default=CUDD, show_default=True)
@click.option('--ts', help='thread slice count', default=1, show_default=True)
@click.option('--sv', help='slice var order', default=7, show_default=True)
@click.option('--vj', help='verbose join-tree processing', default=0, show_default=True)
@click.option('--vp', help='verbose profiling', default=0, show_default=True)
@click.option('--vs', help='verbose solving', default=1, show_default=True)
@click.option('--extra', help='other arguments', default='', show_default=True)
def main(cf, solver, decomposer, jf, sif, softmemcap, memcap, timecap, runner, tc, wc, pc, er, lb, tm, ep, mf, mv, sm, pw, dp, ts, sv, vj, vp, vs, extra):
    def getRunnerCmd():
        cmd = [getBinPath('runsolver'), '-w /dev/null', '-v /dev/stdout']
        if memcap:
            cmd.append(f'-R {int(memcap * 1e3)}') # in MB (must not use -M or -V)
        if timecap:
            cmd.append(f'-W {int(timecap)}')
        return cmd

    printPair('cf', cf)
    printPair('solver', solver)
    print()

    cmd = getRunnerCmd() if runner else []

    if solver in {PLANNER, DMC, DPMC}:
        cmd += [
            getAbsPath('dpmc.py'),
            f'--cf={cf}',
            f'--solver={solver}',
            f'--decomposer={decomposer}',
            f'--jf={jf}',
            f'--sif={sif}',
            f'--softmemcap={softmemcap}',
            f'--tc={tc}',
            f'--wc={wc}',
            f'--pc={pc}',
            f'--er={er}',
            f'--lb={lb}',
            f'--tm={tm}',
            f'--ep={ep}',
            f'--mf={mf}',
            f'--mv={mv}',
            f'--sm={sm}',
            f'--pw={pw}',
            f'--dp={dp}',
            f'--ts={ts}',
            f'--sv={sv}',
            f'--vj={vj}',
            f'--vp={vp}',
            f'--vs={vs}'
        ]
        sh(cmd, stderr=(sys.stderr if solver == PLANNER else sys.stdout))
    elif solver == GAUSS:
        cmd += [
            getBinPath('gaussmaxhs'),
            f'-verb={vs}',
            cf
        ]
        sh(cmd, stderr=sys.stdout)
    elif solver == MAXHS:
        cmd += [
            getBinPath('maxhs'),
            '-no-printOptions',
            '-printSoln',
            f'-verb={vs}',
            cf
        ]
        sh(cmd, stderr=sys.stdout)
    else:
        assert solver == UWR, solver
        cmd += [
            getBinPath('uwrmaxsat'),
            '-bm',
            f'-v{vs}',
            cf
        ]
        sh(cmd, stderr=sys.stdout)

if __name__ == '__main__':
    main()
