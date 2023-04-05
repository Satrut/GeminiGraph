#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "core/graph.hpp"

// 权值
typedef float Weight;

// 消息结构体
struct MSG {
  VertexId root;
  VertexId pre;
  Weight dis;
  bool sent;
  MSG *next;
};

// 消息链表
class MSGList {
public:
  MSG *begin;
  MSG *end;
  MSGList():begin(nullptr), end(nullptr) {};
  MSGList(MSG *begin, MSG *end):begin(begin), end(end) {};
  ~MSGList() {
    for (MSG *cur = begin, *next; cur != nullptr; ) {
      next = cur->next;
      delete cur;
      cur = next;
    }
  }
  // 尾插法插入
  void insert(MSG *msg) {
    if (begin == nullptr) {
      begin = end = msg;
      return;
    }
    end->next = msg;
    end = msg;
  }
  // 删除指定MSG
  bool remove(MSG *msg) {
    MSG *cur = begin;
    if (begin == msg) {
      begin = begin->next;
      if (end == msg) {
        end = nullptr;
      }
      delete msg;
      return true;
    }
    while (cur->next != nullptr) {
      if (cur->next == msg) {
        if (msg == end) {
          end = cur;
        }
        cur->next = cur->next->next;
        delete msg;
        return true;
      }
      cur = cur->next;
    }
    std::cout << "cannot find msg" << std::endl;
    return false;
  }
  // 查找源为root的消息
  MSG *find(VertexId root) {
    MSG *cur = begin;
    while (cur != nullptr) {
      if (cur->root == root) {
        return cur;
      }
      cur = cur->next;
    }
    return nullptr;
  }
  // 查找未发送的且字典序最小的MSG
  MSG *get() {
    MSG *res = nullptr;
    MSG *cur = begin;
    while (cur != nullptr) {
      if (cur->sent == false && (res == nullptr || res->root > cur->root)) {
        res = cur;
      }
      cur = cur->next;
    }
    return res;
  }
};

void compute(Graph<Weight> *graph) {
  double exec_time = 0;
  exec_time -= get_time();

  // 每个点保存自己计算出的围长
  Weight *girth = graph->alloc_vertex_array<Weight>();
  VertexSubset *active_in = graph->alloc_vertex_subset();
  VertexSubset *active_out = graph->alloc_vertex_subset();
  // 源节点集
  active_in->clear();
  active_in->fill();
  VertexId active_vertices = graph->vertices;

  // 初始围长赋值
  graph->fill_vertex_array(girth, (Weight) 1e9);

  // 节点集消息链表数组
  MSGList msglist[graph->vertices];
  // 初始化消息链表
  MSG *init_msg = nullptr;
  for (int i = 0;i < graph->vertices;i++) {
    init_msg = new MSG();
    init_msg->root = i;
    init_msg->pre = graph->vertices;
    init_msg->dis = 0;
    init_msg->sent = false;
    init_msg->next = nullptr;
    msglist[i].insert(init_msg);
  }

  for (int i_i = 0;active_vertices > 0;i_i++) {
    if (graph->partition_id == 0) {
      printf("active(%d)>=%u\n", i_i, active_vertices);
    }
    active_out->clear();
    active_vertices = graph->process_edges<VertexId, MSG>(
      [&](VertexId src) {
        // 求最小字典序的可发送消息
        MSG *msg = msglist[src].get();
        if (msg != nullptr) {
          // std::cout << msg->root << " " << msg->dis << " " << std::endl;
          graph->emit(src, *msg);
          msg->sent = true;
        }
      },
      [&](VertexId src, MSG msg, VertexAdjList<Weight> outgoing_adj) {
        VertexId activated = 0;
        VertexId root = msg.root;
        for (AdjUnit<Weight> *ptr = outgoing_adj.begin;ptr != outgoing_adj.end;ptr++) {
          VertexId dst = ptr->neighbour;
          if (src == 23) {
            std::cout << dst << std::endl;
          }
          // 不要把消息往回发
          if (dst == msg.pre) {
            continue;
          }
          // 查找是否已存在到root的最短路径
          MSG *res = msglist[dst].find(root);
          // 求解围长
          if (res != nullptr) {
            Weight new_cicle = res->dis + msg.dis + ptr->edge_data;
            // 比较保存的围长大小
            if (new_cicle < girth[dst]) {
              write_min(&girth[dst], new_cicle);
            }
          }
          // 更新最短路径
          if (res == nullptr) {
            // 不存在root的路径
            MSG *new_msg = new MSG();
            new_msg->root = msg.root;
            new_msg->pre = src;
            new_msg->dis = msg.dis + ptr->edge_data;
            new_msg->sent = false;
            new_msg->next = nullptr;
            msglist[dst].insert(new_msg);
            active_out->set_bit(dst);
            activated += 1;
          }
          else if (res->dis > msg.dis + ptr->edge_data) {
            // 新路径更短
            write_min(&res->dis, msg.dis + ptr->edge_data);
            res->pre = src;
            res->sent = false;
            active_out->set_bit(dst);
            activated += 1;
          }
        }
        return activated;
      },
        [&](VertexId dst, VertexAdjList<Weight> incoming_adj) {
        // Weight msg = 1e9;
        // for (AdjUnit<Weight> *ptr = incoming_adj.begin;ptr != incoming_adj.end;ptr++) {
        //   VertexId src = ptr->neighbour;
        //   // if (active_in->get_bit(src)) {
        //   Weight relax_dist = distance[src] + ptr->edge_data;
        //   if (relax_dist < msg) {
        //     // printf("src:%d, dst:%d, distance[src]:%f, ptr->edge_data:%f\n", src, dst, distance[src], ptr->edge_data);
        //     msg = relax_dist;
        //   }
        //   // }
        // }
        // if (msg < 1e9) graph->emit(dst, msg);
      },
      [&](VertexId dst, MSG msg) {
        // if (msg < distance[dst]) {
        //   write_min(&distance[dst], msg);
        //   active_out->set_bit(dst);
        //   return 1;
        // }
        return 0;
      },
        active_in
        );
    std::swap(active_in, active_out);
  }

  exec_time += get_time();
  if (graph->partition_id == 0) {
    printf("exec_time=%lf(s)\n", exec_time);
  }

  graph->gather_vertex_array(girth, 0);
  if (graph->partition_id == 0) {
    VertexId max_v_i = 0;
    for (VertexId v_i = 0;v_i < graph->vertices;v_i++) {
      // std::cout << v_i << " " << girth[v_i] << std::endl;
      if (girth[v_i] < 1e9 && girth[v_i] < girth[max_v_i]) {
        max_v_i = v_i;
      }
    }
    if (girth[max_v_i] < 1e9) {
      printf("girth[%u]=%f\n", max_v_i, girth[max_v_i]);
    }
    else {
      std::cout << "cannot find circle" << std::endl;
    }
  }

  graph->dealloc_vertex_array(girth);
  // delete[]msglist;
  delete active_in;
  delete active_out;
}

int main(int argc, char **argv) {
  MPI_Instance mpi(&argc, &argv);

  if (argc != 3) {
    printf("girth [file] [vertices]\n");
    exit(-1);
  }

  Graph<Weight> *graph;
  graph = new Graph<Weight>();
  graph->load_undirected_from_directed(argv[1], std::atoi(argv[2]));
  // graph->load_directed(argv[1], std::atoi(argv[2]));
  // VertexId root = std::atoi(argv[3]);

  compute(graph);
  // for (int run = 0;run < 5;run++) {
  //   compute(graph, root);
  // }

  delete graph;
  return 0;
}
