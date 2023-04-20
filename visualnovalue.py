import networkx as nx
import matplotlib.pyplot as plt
import argparse
import os

parser = argparse.ArgumentParser(description='generate a random graph')
parser.add_argument('-v', required=True, type=int,
                    help='number of vertices in the graph')
parser.add_argument('-f', required=True, type=str,
                    help='the file name for the graph')
parser.add_argument('-o', required=True, type=str,
                    help='name of visualization image')
parser.add_argument('--undirected', action='store_true',
                    help='display an undirected graph instead of the default directed one')

opt = parser.parse_args()
opt.o = os.path.basename(opt.o)

Graph = nx.Graph() if opt.undirected else nx.DiGraph()
Graph.add_nodes_from(range(opt.v))
with open(opt.f, "rt") as f:
    for line in f.readlines():
        i, j = line.split()
        i, j = int(i), int(j)
        Graph.add_edge(i, j)
# nx.draw_circular(Graph, with_labels=True)
nx.draw(Graph, with_labels=True)

plt.savefig(opt.o)
