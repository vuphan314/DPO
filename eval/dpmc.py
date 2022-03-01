#!/usr/bin/env python3

import click
import math
import os
import subprocess

PLANNER = 'planner'
DMC = 'dmc'
DPMC = 'dpmc'
DPMC_SOLVERS = (PLANNER, DMC, DPMC)

HTD = 'htd'
FLOW = 'flow'
DECOMPOSERS = {
    HTD: '/solvers/htd-master/bin/htd_main --opt width --iterations 0 --strategy challenge --print-progress --preprocessing full',
    FLOW: '/solvers/flow-cutter-pace17/flow_cutter_pace17 -p 100'
}

CUDD = 'c'
SYLVAN = 's'
DD_PACKAGES = (CUDD, SYLVAN)

def getBinPath(fileName):
    return os.path.join(os.path.dirname(os.path.realpath(__file__)), 'bin', fileName)

@click.command(context_settings={'max_content_width': 90, 'help_option_names': ['-h', '--help']})
@click.option('--cf', help='CNF file', required=True)
@click.option('--solver', help='planner, executor, or counter', type=click.Choice(DPMC_SOLVERS), default=DPMC, show_default=True)
@click.option('--decomposer', help='planning technique', type=click.Choice(DECOMPOSERS), default=FLOW, show_default=True)
@click.option('--jf', help='JT file', default='', show_default=True)
@click.option('--sif', help='dmc.sif instead of dmc', default=0, show_default=True)
@click.option('--softmemcap', help='for solver, in GB', default=0., show_default=True)
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
def main(cf, solver, decomposer, jf, sif, softmemcap, tc, wc, pc, er, lb, tm, ep, mf, mv, sm, pw, dp, ts, sv, vj, vp, vs):
    lgCmd = [getBinPath('lg.sif'), DECOMPOSERS[decomposer]]

    dmcCmd = [
        'singularity',
        'run',
        '--bind="/:/host"',
        getBinPath('dmc.sif'),
        f'--cf=/host{os.path.realpath(cf)}'
    ] if sif else [getBinPath('dmc'), f'--cf={cf}'] # link=-static is safe iff tc == 1
    dmcCmd += [
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
        f'--tc={tc}',
        f'--ts={ts}',
        f'--sv={sv}',
        f'--mm={int(softmemcap * 1e3)}',
        f'--vj={vj}',
        f'--vp={vp}',
        f'--vs={vs}'
    ]
    if dp == CUDD:
        dmcCmd.append('--lc=1')

    if solver == PLANNER:
        subprocess.run(lgCmd, stdin=open(cf))
    elif solver == DMC:
        assert jf, 'must specify --jf'
        subprocess.run(dmcCmd, stdin=open(jf))
    else:
        assert solver == DPMC, solver

        plannerArgs = {'stdout': subprocess.PIPE}
        plannerArgs['stdin'] = open(cf)
        plannerArgs['stderr'] = subprocess.DEVNULL # LG's stderr would mess up postprocessor.py

        p = subprocess.Popen(lgCmd, **plannerArgs)
        subprocess.run(dmcCmd, stdin=p.stdout)

if __name__ == '__main__':
    main()
