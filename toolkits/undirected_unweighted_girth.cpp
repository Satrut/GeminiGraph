#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <list>
#include "core/graph.hpp"

// 权值
typedef float Weight;
// 自定义单向链表，用于存储边和环的节点顺序
struct MyList {
  VertexId vertex;
  MyList *next;
  MyList() {}
  MyList(VertexId vertex) {
    this->vertex = vertex;
    this->next = nullptr;
  }
};

// 消息结构体
struct MSG {
  // 用begin开头的单链表记录路径节点
  MyList *begin;
  // pre记录消息经过的上一节点
  VertexId pre;
  // 距离
  Weight dis;
  // 节点数
  int count;
  // 是否发送
  bool sent;
  // 下一条消息
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
      if (cur->begin->vertex == root) {
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
      if (cur->sent == false && (res == nullptr || res->begin->vertex > cur->begin->vertex)) {
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

  // 每个点保存自己计算出的围长及环的数据
  Weight *girth = graph->alloc_vertex_array<Weight>();
  std::list<VertexId> circle[graph->vertices];
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
  for (VertexId i = 0;i < graph->vertices;i++) {
    init_msg = new MSG();
    init_msg->begin = new MyList(i);
    init_msg->pre = graph->vertices;
    init_msg->dis = 0;
    init_msg->count = 1;
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
          graph->emit(src, *msg);
          msg->sent = true;
        }
      },
      [&](VertexId src, MSG msg, VertexAdjList<Weight> outgoing_adj) {
        VertexId activated = 0;
        VertexId root = msg.begin->vertex;

        for (AdjUnit<Weight> *ptr = outgoing_adj.begin;ptr != outgoing_adj.end;ptr++) {
          VertexId dst = ptr->neighbour;
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
              // 更新围长大小及环的信息          
              write_min(&girth[dst], new_cicle);
              circle[dst].clear();
              // 插入p(s,u)
              for (MyList *node = msg.begin;node != nullptr;node = node->next) {
                circle[dst].push_back(node->vertex);
              }
              // 倒序插入p(s,v)，且不插入最后一个s
              VertexId ids[res->count];
              VertexId id = 0;
              for (MyList *node = res->begin;node != nullptr;node = node->next, id++) {
                ids[id] = node->vertex;
              }
              for (id = res->count - 1;id > 0;id--) {
                circle[dst].push_back(ids[id]);
              }
            }
          }
          // 更新最短路径
          if (res == nullptr) {
            // 不存在root的路径
            MSG *new_msg = new MSG();
            new_msg->dis = msg.dis + ptr->edge_data;
            new_msg->count = msg.count + 1;
            new_msg->pre = src;
            new_msg->next = nullptr;

            // 复制路径
            MyList *node = msg.begin;
            MyList *cur = new MyList(node->vertex);
            new_msg->begin = cur;
            node = node->next;
            while (node != nullptr) {
              cur->next = new MyList(node->vertex);
              cur = cur->next;
              node = node->next;
            }
            cur->next = new MyList(dst);

            new_msg->sent = false;
            msglist[dst].insert(new_msg);
            // 避免重复设置下轮发送端节点
            if (!active_out->get_bit(dst)) {
              active_out->set_bit(dst);
              activated += 1;
            }
          }
          else if (res->dis > msg.dis + ptr->edge_data) {
            // 新路径更短
            // 更长的路径没必要转发，减小带宽压力
            // 有res说明找到了环，但是只在dst处求出了该环的权值和，若继续转发该消息，环上的其他节点也能更新该环的权值，但耗费了更多无意义的资源
            write_min(&res->dis, msg.dis + ptr->edge_data);
            res->count = msg.count + 1;
            res->pre = src;

            // 清空路径
            MyList *cur = res->begin, *next = res->begin->next;
            res->begin = nullptr;
            while (cur != nullptr) {
              next = cur->next;
              delete cur;
              cur = next;
            }

            // 复制路径
            MyList *node = msg.begin;
            cur = new MyList(node->vertex);
            res->begin = cur;
            node = node->next;
            while (node != nullptr) {
              cur->next = new MyList(node->vertex);
              cur = cur->next;
              node = node->next;
            }
            cur->next = new MyList(dst);

            res->sent = false;
            // 避免重复设置下轮发送端节点
            if (!active_out->get_bit(dst)) {
              active_out->set_bit(dst);
              activated += 1;
            }
          }
        }
        return activated;
      },
        [&](VertexId dst, VertexAdjList<Weight> incoming_adj) {
      },
      [&](VertexId dst, MSG msg) {
        return 0;
      },
        active_in
        );
    // 更新发送端和接收端节点集
    std::swap(active_in, active_out);
  }

  exec_time += get_time();
  if (graph->partition_id == 0) {
    printf("exec_time=%lf(s)\n", exec_time);
  }

  // 汇聚围长信息
  graph->gather_vertex_array(girth, 0);
  if (graph->partition_id == 0) {
    VertexId max_v_i = 0;
    for (VertexId v_i = 0;v_i < graph->vertices;v_i++) {
      if (girth[v_i] < 1e9 && girth[v_i] < girth[max_v_i]) {
        max_v_i = v_i;
      }
    }
    if (girth[max_v_i] < 1e9) {
      printf("girth=%f\n", girth[max_v_i]);
      std::cout << "circle: ";
      for (auto it = circle[max_v_i].begin();it != circle[max_v_i].end();it++) {
        std::cout << *it << " ";
      }
      std::cout << std::endl;
    }
    else {
      std::cout << "cannot find circle" << std::endl;
    }
  }

  // 释放malloc资源
  graph->dealloc_vertex_array(girth);
  delete active_in;
  delete active_out;
}

int main(int argc, char **argv) {
  MPI_Instance mpi(&argc, &argv);

  if (argc != 3) {
    printf("undirected_unweighted_girth [file] [vertices]\n");
    exit(-1);
  }
  int vertices = std::atoi(argv[2]);
  if (vertices <= 0) {
    std::cout << "vertex numbers must be bigger than 0" << std::endl;
    exit(-1);
  }

  Graph<Weight> *graph;
  graph = new Graph<Weight>();
  graph->load_undirected_from_directed(argv[1], vertices);

  compute(graph);
  for (int run = 0;run < 5;run++) {
    compute(graph);
  }

  delete graph;
  return 0;
}
