import argparse
import numpy as np
import os

parser = argparse.ArgumentParser(description='generate a random graph')
parser.add_argument('-v', required=True, type=int,
                    help='number of vertices in the graph')
parser.add_argument('-e', type=int, default=-1,
                    help='number of edges in the graph, randomized if not given')
parser.add_argument('-w', required=True, type=int,
                    help='max weight of edges')
parser.add_argument('-o', required=True, type=str,
                    help='name of the generated file')

opt = parser.parse_args()

opt.o = os.path.basename(opt.o)

print(f'an undirected graph with '
      f'{opt.v} vertices and {opt.e} edges and max weight {opt.w} will be stored at {opt.o}')

edges_list = []
for i in range(opt.v):
    for j in range(opt.v):
        if j >= i:
            break
        for w in range(1, opt.w):
            edges_list.append(f"{i} {j} {w}")
edges_list = np.asarray(edges_list)

selection = sorted(np.random.choice(len(edges_list), opt.e, replace=False))
selected_edges = edges_list[selection]

with open(opt.o, 'wt') as f:
    for i in range(opt.e):
        f.write(selected_edges[i])
        if i != opt.e-1:
            f.write('\n')
