import networkx as nx
import matplotlib.pyplot as plt
import argparse
import os

parser = argparse.ArgumentParser(description='generate a random graph')
parser.add_argument('-v', required=True, type=int, help='number of vertices in the graph')
parser.add_argument('-f', required=True, type=str, help='the file name for the graph')
parser.add_argument('-o', required=True, type=str, help='name of visualization image')

opt = parser.parse_args()
opt.o = os.path.basename(opt.o)

Graph = nx.Graph()
Graph.add_nodes_from(range(opt.v))
with open(opt.f, "rt") as f:
    for line in f.readlines():
        i, j, w = line.split()
        i, j, w = int(i), int(j), int(w)
        Graph.add_edge(i, j, weight=w)
nx.draw(Graph, with_labels=True)
plt.savefig(opt.o)