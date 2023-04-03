import networkx as nx
from matplotlib import pyplot as plt
from networkx import k_core

p = nx.path_graph(28)
G = nx.Graph()
G.add_nodes_from(p)
edges = nx.read_edgelist("test.txt", create_using=nx.Graph(), nodetype=int)  # 读取txt文件
G.add_edges_from(nx.edges(edges))
print(k_core(G, 0))
print(k_core(G, 1))
print(k_core(G, 2))
print(k_core(G, 3))
print(k_core(G, 4))

plt.show()