import argparse
import numpy as np
import os

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

print(f'{"an un" if opt.undirected else "a "}directed graph with '
      f'{opt.v} {"vertices" if opt.v > 1 else "vertex"} and {opt.e} {"edges" if opt.e > 1 else "edge"} will be stored at {opt.o}')

edges_list = []
for i in range(opt.v):
    for j in range(opt.v):
        if i == j:
            continue
        if opt.undirected and j >= i:
            break
        edges_list.append(f"{i} {j}")
edges_list = np.asarray(edges_list)

assert len(edges_list) == max_edges

selection = sorted(np.random.choice(len(edges_list), opt.e, replace=False))
selected_edges = edges_list[selection]

with open(opt.o, 'wt') as f:
    for i in range(opt.e):
        f.write(selected_edges[i])
        if i != opt.e-1:
            f.write('\n')
