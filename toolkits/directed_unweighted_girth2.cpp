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
};

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
  BFSMSGList() :begin(nullptr) {};
  BFSMSGList(BFSMSG *begin, BFSMSG *end) :begin(begin) {};
  ~BFSMSGList() {
    BFSMSG *cur = begin, *next = nullptr;
    begin = nullptr;
    while (cur != nullptr) {
      next = cur->next;
      delete cur;
      cur = next;
    }
  }
  // 尾插法插入
  void insert(BFSMSG *msg) {
    msg->next = begin;
    begin = msg;
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

// 消息结构体
struct MSG {
  // 根结点
  VertexId root;
  // pre记录消息经过的上一节点
  VertexId pre;
  // 距离
  Weight dis;
  // 是否发送
  bool sent;
  // 下一条消息
  MSG *next;
};

// 消息链表
class MSGList {
public:
  MSG *begin;
  MSGList() :begin(nullptr) {};
  MSGList(MSG *begin, MSG *end) :begin(begin) {};
  ~MSGList() {
    for (MSG *cur = begin, *next; cur != nullptr; ) {
      next = cur->next;
      delete cur;
      cur = next;
    }
  }
  // 头插法插入
  void insert(MSG *msg) {
    msg->next = begin;
    begin = msg;
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
          // 求解围长
          if (root == dst) {
            // 存在环
            Weight new_circle = msg.dis + 1;
            // 比较保存的围长大小
            if (new_circle < girth[dst]) {
              // 更新围长大小及环的信息          
              write_min(&girth[dst], new_circle);
            }
            continue;
          }
          // 查找是否已存在到root的最短路径
          MSG *res = msglist[dst].find(root);
          // 更新最短路径
          if (res == nullptr) {
            // 不存在root的路径
            MSG *new_msg = new MSG();
            new_msg->root = msg.root;
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

  // 汇聚围长信息
  graph->gather_vertex_array(girth, 0);
  // 围长对应的结点ID
  VertexId max_v_i = 0;
  // 围长
  Weight minGirth = 0;
  if (graph->partition_id == 0) {
    for (VertexId v_i = 0;v_i < graph->vertices;v_i++) {
      if (girth[v_i] < 1e9 && girth[v_i] < girth[max_v_i]) {
        max_v_i = v_i;
      }
    }
    minGirth = girth[max_v_i];
    for (VertexId v_i = 1;v_i < graph->partitions;v_i++) {
      // 向其他节点发送相关结点ID及围长
      MPI_Send(&max_v_i, 1, MPI_UNSIGNED, v_i, 0, MPI_COMM_WORLD);
      MPI_Send(&minGirth, 1, MPI_UNSIGNED, v_i, 0, MPI_COMM_WORLD);
    }
    if (girth[max_v_i] < 1e9) {
      printf("girth=%d\n", girth[max_v_i]);
    }
    else {
      std::cout << "cannot find circle" << std::endl;
    }
  }
  else {
    // 其他节点接收关键结点ID及围长
    MPI_Recv(&max_v_i, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&minGirth, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }

  if (minGirth < 1e9) {
    // 求解环
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
      if (graph->partition_id == 0) {
        printf("active(%d)>=%u\n", i_i++, active_vertices);
      }
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

            if (root == dst) {
              if (mark) {
                continue;
              }
              mark = true;
              printf("circle:");
              for (auto it = msg.path;it != nullptr;it = it->next) {
                printf("%d ", it->vertex);
              }
              printf("\n");
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
      // 各节点进行通信
      if (graph->partition_id == 0) {
        bool tmpMark = false;
        // 0号节点收集信息
        for (VertexId v_i = 1;v_i < graph->partitions;v_i++) {
          MPI_Recv(&tmpMark, 1, MPI_CHAR, v_i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          if (tmpMark) {
            mark = true;
          }
        }
        // 整合后发送信息
        for (VertexId v_i = 1;v_i < graph->partitions;v_i++) {
          MPI_Send(&mark, 1, MPI_CHAR, v_i, 0, MPI_COMM_WORLD);
        }
      }
      else {
        // 其他节点发送后接收信息
        MPI_Send(&mark, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        MPI_Recv(&mark, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
    }
  }

  exec_time += get_time();
  if (graph->partition_id == 0) {
    printf("exec_time=%lf(s)\n", exec_time);
  }

  // 释放malloc资源
  graph->dealloc_vertex_array(girth);
  delete active_in;
  delete active_out;
}

int main(int argc, char **argv) {
  MPI_Instance mpi(&argc, &argv);

  if (argc != 3) {
    printf("directed_unweighted_girth [file] [vertices]\n");
    exit(-1);
  }
  int vertices = std::atoi(argv[2]);
  if (vertices <= 0) {
    std::cout << "vertex numbers must be bigger than 0" << std::endl;
    exit(-1);
  }

  Graph<Empty> *graph;
  graph = new Graph<Empty>();
  graph->load_directed(argv[1], vertices);

  compute(graph);

  delete graph;
  return 0;
}