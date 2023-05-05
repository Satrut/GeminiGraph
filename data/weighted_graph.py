import argparse
import numpy as np
import os
import random
import networkx as nx

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
G = nx.gnm_random_graph(opt.v, opt.e)

with open(opt.o, 'wt') as f:
    for e in G.edges():
        f.write(str(e[0])+' '+str(e[1])+' '+str(random.randint(1, opt.w))+'\n')
