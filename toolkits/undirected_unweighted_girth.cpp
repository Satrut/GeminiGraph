#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <list>
#include "core/graph.hpp"

// 权值
typedef uint32_t Weight;
// 自定义单向链表，用于存储边和环的节点顺序
struct MyList {
  VertexId vertex;
  MyList *next;
  MyList() {}
  MyList(VertexId vertex) {
    this->vertex = vertex;
    this->next = nullptr;
  }
} __attribute__((packed));

struct BFSMSG {
  MyList *path;
  VertexId root;
  VertexId pre;
  Weight dis;
  int count;
  bool sent;
  BFSMSG *next;
} __attribute__((packed));

class BFSMSGList {
public:
  BFSMSG *begin;
  BFSMSG *end;
  BFSMSGList() :begin(nullptr), end(nullptr) {};
  BFSMSGList(BFSMSG *begin, BFSMSG *end) :begin(begin), end(end) {};
  ~BFSMSGList() {
    BFSMSG *cur = begin, *next = nullptr;
    begin = end = nullptr;
    while (cur != nullptr) {
      next = cur->next;
      delete cur;
      cur = next;
    }
  }
  // 尾插法插入
  void insert(BFSMSG *msg) {
    if (begin == nullptr) {
      begin = end = msg;
      return;
    }
    end->next = msg;
    end = msg;
  }
  // 查找源为root的消息
  BFSMSG *find(VertexId root) {
    BFSMSG *cur = begin;
    while (cur != nullptr) {
      if (cur->root == root) {
        return cur;
      }
      cur = cur->next;
    }
    return nullptr;
  }
  // 查找未发送的且字典序最小的MSG
  BFSMSG *get() {
    BFSMSG *res = nullptr;
    BFSMSG *cur = begin;
    while (cur != nullptr) {
      if (cur->sent == false && (res == nullptr || res->root > cur->root)) {
        res = cur;
      }
      cur = cur->next;
    }
    return res;
  }
};

struct MSG {
  VertexId root;
  VertexId pre;
  Weight dis;
  bool sent;
  MSG *next;
} __attribute__((packed));

// 消息链表
class MSGList {
public:
  MSG *begin;
  MSG *end;
  MSGList() :begin(nullptr), end(nullptr) {};
  MSGList(MSG *begin, MSG *end) :begin(begin), end(end) {};
  ~MSGList() {
    MSG *cur = begin, *next = nullptr;
    begin = end = nullptr;
    while (cur != nullptr) {
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

void compute(Graph<Empty> *graph) {
  double exec_time = 0;
  exec_time -= get_time();

  // 每个点保存自己计算出的围长及环的数据
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
  for (VertexId i = 0;i < graph->vertices;i++) {
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
          graph->emit(src, *msg);
          msg->sent = true;
        }
      },
      [&](VertexId src, MSG msg, VertexAdjList<Empty> outgoing_adj) {
        VertexId activated = 0;
        VertexId root = msg.root;

        for (AdjUnit<Empty> *ptr = outgoing_adj.begin;ptr != outgoing_adj.end;ptr++) {
          VertexId dst = ptr->neighbour;
          // 不要把消息往回发          
          if (dst == msg.pre) {
            continue;
          }
          // 查找是否已存在到root的最短路径
          MSG *res = msglist[dst].find(root);
          // 求解围长
          if (res != nullptr) {
            Weight new_circle = res->dis + msg.dis + 1;
            // 比较保存的围长大小
            if (new_circle < girth[dst]) {
              // 更新围长大小及环的信息          
              write_min(&girth[dst], new_circle);
            }
          }
          // 更新最短路径
          if (res == nullptr) {
            // 不存在root的路径
            MSG *new_msg = new MSG();
            new_msg->root = root;
            new_msg->dis = msg.dis + 1;
            new_msg->pre = src;
            new_msg->next = nullptr;
            new_msg->sent = false;
            msglist[dst].insert(new_msg);
            // 避免重复设置下轮发送端节点
            if (!active_out->get_bit(dst)) {
              active_out->set_bit(dst);
              activated += 1;
            }
          }
          else if (res->dis > msg.dis + 1) {
            // 新路径更短            
            bool wt_res = write_min(&res->dis, msg.dis + 1);
            if (wt_res) {
              res->pre = src;
              res->sent = false;
              // 避免重复设置下轮发送端节点
              if (!active_out->get_bit(dst)) {
                active_out->set_bit(dst);
                activated += 1;
              }
            }
          }
        }
        return activated;
      },
      [&](VertexId dst, VertexAdjList<Empty> incoming_adj) {
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
      printf("girth=%d\n", girth[max_v_i]);
      active_in->clear();
      active_in->set_bit(max_v_i);
      BFSMSGList bfslist[graph->vertices];
      // 初始化消息链表
      BFSMSG *bfsmsg = new BFSMSG();
      bfsmsg->path = new MyList(max_v_i);
      bfsmsg->root = max_v_i;
      bfsmsg->pre = graph->vertices;
      bfsmsg->dis = 0;
      bfsmsg->sent = false;
      bfsmsg->next = nullptr;
      bfslist[max_v_i].insert(bfsmsg);
      bool mark = false;
      active_vertices = 1;
      int i_i = 0;
      while (!mark && active_vertices > 0) {
        active_out->clear();
        active_vertices = graph->process_edges<VertexId, BFSMSG>(
          [&](VertexId src) {
            // 求最小字典序的可发送消息
            BFSMSG *msg = bfslist[src].get();
            if (msg != nullptr) {
              graph->emit(src, *msg);
              msg->sent = true;
            }
          },
          [&](VertexId src, BFSMSG msg, VertexAdjList<Empty> outgoing_adj) {
            VertexId activated = 0;
            VertexId root = msg.root;

            for (AdjUnit<Empty> *ptr = outgoing_adj.begin;ptr != outgoing_adj.end;ptr++) {
              VertexId dst = ptr->neighbour;
              // 不要把消息往回发          
              if (dst == msg.pre) {
                continue;
              }
              // 查找是否已存在到root的路径，存在则找到环
              BFSMSG *res = bfslist[dst].find(root);
              if (res == nullptr) {
                BFSMSG *new_msg = new BFSMSG();
                new_msg->path = new MyList(root);
                new_msg->root = root;
                new_msg->dis = msg.dis + 1;
                new_msg->pre = src;
                new_msg->next = nullptr;
                new_msg->sent = false;

                // 记录path
                MyList *cur = new_msg->path;
                for (auto it = msg.path->next;it != nullptr;it = it->next) {
                  cur->next = new MyList(it->vertex);
                  cur = cur->next;
                }
                cur->next = new MyList(dst);

                bfslist[dst].insert(new_msg);
                // 避免重复设置下轮发送端节点
                if (!active_out->get_bit(dst)) {
                  active_out->set_bit(dst);
                  activated += 1;
                }
              }
              else {
                if (mark) {
                  continue;
                }
                mark = true;
                printf("circle:");
                for (auto it = msg.path;it != nullptr;it = it->next) {
                  printf("%d ", it->vertex);
                }
                std::list<VertexId> list;
                for (auto it = res->path;it != nullptr;it = it->next) {
                  list.push_front(it->vertex);
                }
                list.pop_back();
                for (auto it = list.begin();it != list.end();it++) {
                  printf("%d ", *it);
                }
                printf("\n");
              }
            }
            return activated;
          },
          [&](VertexId dst, VertexAdjList<Empty> incoming_adj) {
          },
          [&](VertexId dst, BFSMSG msg) {
            return 0;
          },
          active_in
        );
        // 更新发送端和接收端节点集
        std::swap(active_in, active_out);
      }
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

  Graph<Empty> *graph;
  graph = new Graph<Empty>();
  graph->load_undirected_from_directed(argv[1], vertices);

  compute(graph);

  delete graph;
  return 0;
}