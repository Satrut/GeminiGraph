import argparse
import numpy as np
import os
import networkx as nx

parser = argparse.ArgumentParser(description='generate a random graph')
parser.add_argument('-v', required=True, type=int,
                    help='number of vertices in the graph')
parser.add_argument('-e', type=int, default=-1,
                    help='number of edges in the graph, randomized if not given')
parser.add_argument('--undirected', action='store_true',
                    help='generate an undirected graph instead of the default directed one')
parser.add_argument('-o', required=True, type=str,
                    help='name of the generated file')

opt = parser.parse_args()

max_edges = opt.v * \
    (opt.v - 1) if not opt.undirected else opt.v * (opt.v - 1) // 2
opt.e = min(opt.e, max_edges)

if opt.e == -1:
    # the number of edges is not given
    opt.e = np.random.randint(low=0, high=max_edges + 1)

opt.o = os.path.basename(opt.o)

print(f'{"an un" if opt.undirected else "a "}directed graph with {opt.v} {"vertices" if opt.v > 1 else "vertex"} and {opt.e} {"edges" if opt.e > 1 else "edge"} will be stored at {opt.o}')

G = nx.gnm_random_graph(opt.v, opt.e)

with open(opt.o, 'wt') as f:
    for e in G.edges():
        f.write(str(e[0])+' '+str(e[1])+'\n')
