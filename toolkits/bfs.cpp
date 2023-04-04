/*
Copyright (c) 2014-2015 Xiaowei Zhu, Tsinghua University

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

/*从给的root开始 '发送端-接收端' 形式的bfs，通过已有的发送端集合+边的信息求出接收端集合，
  ，统计新发现节点，然后本轮的接收端集合成为下轮的发送端集合继续循环，直到无法发现新节点
*/

#include <stdio.h>
#include <stdlib.h>

#include "core/graph.hpp"

void compute(Graph<Empty> *graph, VertexId root) {
  //  执行时间
  double exec_time = 0;
  exec_time -= get_time();

  //  parent[dst]=src，记录接收端对应的发送端
  VertexId *parent = graph->alloc_vertex_array<VertexId>();
  //  已访问节点集合
  VertexSubset *visited = graph->alloc_vertex_subset();
  //  发送端集合
  VertexSubset *active_in = graph->alloc_vertex_subset();
  //  接收端集合
  VertexSubset *active_out = graph->alloc_vertex_subset();

  //  初始只有root节点被访问
  visited->clear();
  visited->set_bit(root);
  active_in->clear();
  active_in->set_bit(root);
  //  将除了root的所有节点的parent赋值为节点数，访问到相关节点时更新parent
  graph->fill_vertex_array(parent, graph->vertices);
  //  root的parent特殊赋值为root，以便第一轮循环启动
  parent[root] = root;

  //  记录每轮新发现的节点数
  VertexId active_vertices = 1;

  //  循环直到无法发现新节点
  for (int i_i = 0;active_vertices > 0;i_i++) {
    //  输出上轮新节点信息
    if (graph->partition_id == 0) {
      printf("active(%d)>=%u\n", i_i, active_vertices);
    }
    //  接收端集合每轮清零
    active_out->clear();
    // ？？？
    active_vertices = graph->process_edges<VertexId, VertexId>(
      // sparse_signal向所有备份传递信息
      [&](VertexId src) {
        graph->emit(src, src);
      },
      // 备份接受信息，开始计算
      [&](VertexId src, VertexId msg, VertexAdjList<Empty> outgoing_adj) {
        VertexId activated = 0;
        // outgoing_adj为出边链表，遍历该链表得到并更新接收端集合
        for (AdjUnit<Empty> *ptr = outgoing_adj.begin;ptr != outgoing_adj.end;ptr++) {
          VertexId dst = ptr->neighbour;
          // parent[dst] == graph->vertices代表该节点parent为初始赋值，故该节点未访问过
          // cas为原子性的将parent[dst]正确赋值为src
          if (parent[dst] == graph->vertices && cas(&parent[dst], graph->vertices, src)) {
            // 更新本轮的接收端集合和新发现节点数
            active_out->set_bit(dst);
            activated += 1;
          }
        }
        return activated;
      },
      // 备份从入边的邻接节点获取信息并进行局部计算
      [&](VertexId dst, VertexAdjList<Empty> incoming_adj) {
        // 若该节点访问过则跳过
        if (visited->get_bit(dst)) return;
        // incoming_adj为入边链表
        for (AdjUnit<Empty> *ptr = incoming_adj.begin;ptr != incoming_adj.end;ptr++) {
          VertexId src = ptr->neighbour;
          // 若src为发送端集合中的元素，则向dst发送信息
          if (active_in->get_bit(src)) {
            graph->emit(dst, src);
            break;
          }
        }
      },
      // 备份将信息发送至主备份
      [&](VertexId dst, VertexId msg) {
        // 原子性更新parent和接收端信息
        if (cas(&parent[dst], graph->vertices, msg)) {
          active_out->set_bit(dst);
          return 1;
        }
        return 0;
      },
      active_in, visited
    );
    // ？？？
    active_vertices = graph->process_vertices<VertexId>(
      [&](VertexId vtx) {
        visited->set_bit(vtx);
        return 1;
      },
      active_out
    );
    std::swap(active_in, active_out);
  }

  exec_time += get_time();
  if (graph->partition_id==0) {
    printf("exec_time=%lf(s)\n", exec_time);
  }

  graph->gather_vertex_array(parent, 0);
  if (graph->partition_id==0) {
    VertexId found_vertices = 0;
    for (VertexId v_i=0;v_i<graph->vertices;v_i++) {
      if (parent[v_i] < graph->vertices) {
        found_vertices += 1;
      }
    }
    printf("found_vertices = %u\n", found_vertices);
  }

  graph->dealloc_vertex_array(parent);
  delete active_in;
  delete active_out;
  delete visited;
}

int main(int argc, char ** argv) {
  MPI_Instance mpi(&argc, &argv);

  if (argc<4) {
    printf("bfs [file] [vertices] [root]\n");
    exit(-1);
  }

  Graph<Empty> * graph;
  graph = new Graph<Empty>();
  VertexId root = std::atoi(argv[3]);
  graph->load_directed(argv[1], std::atoi(argv[2]));

  compute(graph, root);
  for (int run=0;run<5;run++) {
    compute(graph, root);
  }

  delete graph;
  return 0;
}
