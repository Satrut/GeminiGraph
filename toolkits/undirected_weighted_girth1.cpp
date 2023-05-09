#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <list>
#include <math.h>
#include <float.h>
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

Weight compute(Graph<Weight> *graph, Weight t, bool *conditionMark) {

  // 保存现有最小围长及节点ID
  Weight girth = 1e9;
  VertexId girthId = graph->partitions;
  std::list<VertexId> circle[graph->vertices];
  VertexSubset *active_in = graph->alloc_vertex_subset();
  VertexSubset *active_out = graph->alloc_vertex_subset();
  // 源节点集
  active_in->clear();
  active_in->fill();
  VertexId active_vertices = graph->vertices;

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

  // 至多n轮循环
  for (VertexId i_i = 0;(i_i < graph->vertices) && (active_vertices > 0);i_i++) {
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

          // 超过距离限制t则放弃，既不能计算围长也不能更新信息
          if ((msg.dis + ptr->edge_data > t) && (fabs(msg.dis + ptr->edge_data - t) > FLT_EPSILON)) {
            continue;
          }

          // 查找是否已存在到root的最短路径
          MSG *res = msglist[dst].find(root);
          // 已存在root的路径，求解围长
          if (res != nullptr) {
            Weight new_circle = res->dis + msg.dis + ptr->edge_data;
            // 条件1触发
            *conditionMark = true;
            // 比较保存的围长大小
            if ((girth > new_circle) && (fabs(girth - new_circle) > FLT_EPSILON)) {
              // 新围长更小，更新围长大小及环的信息     

              // 多线程下原子修改
              bool done1 = false, done2 = false;
              while (!(done1 = cas(&girth, girth, new_circle)) || !(done2 = cas(&girthId, girthId, dst))) {
                // 若有围长更小的线程完成围长的更新,则放弃该信息的更新
                if ((girth < new_circle) && (fabs(girth - new_circle) > FLT_EPSILON)) {
                  break;
                }
              }
              // 被更小围长抢占则本信息作废
              if ((girth < new_circle) && (fabs(girth - new_circle) > FLT_EPSILON)) {
                continue;
              }

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
          // 不存在root的路径则更新，前面已验证满足距离限制t
          else {
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
    // 每轮结束时v0检查是否触发条件1 其他节点发送conditionMark
    if (graph->partition_id == 0) {
      // v0接受所有节点的conditionMark状态，判断是否有条件1触发，若有则更新conditionMark和girth（汇聚围长）
      bool tmpMark = false;
      Weight tmpGirth = 1e9;
      // 用partitionId表示最小围长对应的partition
      VertexId partitionId = graph->vertices;
      if (*conditionMark) {
        partitionId = 0;
      }
      if (graph->partitions > 1) {
        for (VertexId v_i = 1;v_i < graph->partitions;v_i++) {
          MPI_Recv(&tmpMark, 1, MPI_CHAR, v_i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          MPI_Recv(&tmpGirth, 1, MPI_FLOAT, v_i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          if (tmpMark) {
            // 其他节点条件1触发 更新conditionMark、girth及ID
            *conditionMark = true;
            if ((girth > tmpGirth) && (fabs(girth - tmpGirth) > FLT_EPSILON)) {
              girth = tmpGirth;
              partitionId = v_i;
            }
          }
        }
      }

      // 判断条件1是否触发及触发的partition_id是否为0，若为0则v0输出环的信息
      if (partitionId == 0) {
        printf("girth=%f\t", girth);
        std::cout << "circle: ";
        for (auto it = circle[girthId].begin();it != circle[girthId].end();it++) {
          std::cout << *it << " ";
        }
        std::cout << std::endl;
      }

      // 向所有节点发送是否有条件1触发的partition_id
      if (graph->partitions > 1) {
        for (VertexId v_i = 1;v_i < graph->partitions;v_i++) {
          MPI_Send(&partitionId, 1, MPI_UNSIGNED, v_i, 0, MPI_COMM_WORLD);
        }
      }

      // break退出
      if (*conditionMark) {
        break;
      }
    }
    else {
      // 其他节点向v0发送conditionMark状态和girth
      MPI_Send(conditionMark, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
      MPI_Send(&girth, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);

      // 其他节点接收v0信息，判断触发条件1的partition_id是不是自己，若是则输出环的信息
      VertexId partitionId = 0;
      MPI_Recv(&partitionId, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      if (partitionId == graph->partition_id) {
        printf("girth=%f\t", girth);
        std::cout << "circle: ";
        for (auto it = circle[girthId].begin();it != circle[girthId].end();it++) {
          std::cout << *it << " ";
        }
        std::cout << std::endl;
      }

      // 条件1触发则break退出（partitionId为正确的值）
      if (0 <= partitionId && partitionId < graph->partitions) {
        break;
      }
    }
    // 更新发送端和接收端节点集
    std::swap(active_in, active_out);
  }

  // 释放malloc资源
  delete active_in;
  delete active_out;
  return girth;
}

int main(int argc, char **argv) {
  MPI_Instance mpi(&argc, &argv);

  if (argc != 4) {
    printf("undirected_unweighted_girth [file] [vertices] [max Weight]\n");
    exit(-1);
  }
  VertexId vertices = std::atoi(argv[2]);
  Weight maxWeight = std::atof(argv[3]);
  if (vertices <= 0 || maxWeight <= 0) {
    std::cout << "Parameters Error" << std::endl;
    exit(-1);
  }

  Graph<Weight> *graph;
  graph = new Graph<Weight>();
  graph->load_undirected_from_directed(argv[1], vertices);

  // i表示阶段，idx表示2的i次方，用于更新t
  int i = 0, idx = 1;
  // alpha为下界，beta为上界，t为距离限制
  Weight alpha = 1, beta = vertices * maxWeight, t = 1;
  // conditionMark标记运行多源有限距离的宽度优先搜索算法后条件1/2发生，false为2发生，true为1发生
  bool conditionMark = false;
  // mingirth保存最小围长
  Weight minGirth = 1e9;
  double exec_time = 0;
  exec_time -= get_time();

  while ((beta - alpha > 2) && (fabs(beta - alpha - 2) > FLT_EPSILON)) {
    if (graph->partition_id == 0) {
      // 协调点v0计算t
      if ((vertices * maxWeight > beta) && (fabs(vertices * maxWeight - beta) > FLT_EPSILON)) {
        int tmp = floor((alpha + beta) / 4);
        if (fabs(t - tmp) <= FLT_EPSILON) {
          t++;
        }
        else {
          t = tmp;
        }
      }
      else {
        t = idx;
        idx *= 2;
      }
      // v0传递t
      if (graph->partitions > 1) {
        for (VertexId v_i = 1;v_i < graph->partitions;v_i++) {
          MPI_Send(&t, 1, MPI_FLOAT, v_i, 0, MPI_COMM_WORLD);
        }
      }
      printf("t:%f\n", t);
    }
    else {
      // 其他节点接收t
      MPI_Recv(&t, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // 初始化conditionMark
    conditionMark = false;

    // 运行多源有限距离的宽度优先搜索算法
    Weight girth = compute(graph, t, &conditionMark);    

    // 更新 α和 β
    if (graph->partition_id == 0) {
      if (conditionMark) {
        // 根据条件1更新alpha和beta:beta取2t和girth中的较小值
        // 更新最小围长值
        write_min(&minGirth, girth);
        if ((2 * t > minGirth) && (fabs(2 * t - minGirth) > FLT_EPSILON)) {
          beta = minGirth;
        }
        else {
          beta = 2 * t;
        }
      }
      else {
        // 根据条件2更新alpha和beta
        if ((fabs(minGirth - 1e9) <= FLT_EPSILON) || ((minGirth > 2 * t) && (fabs(minGirth - 2 * t) > FLT_EPSILON))) {
          alpha = 2 * t;
        }
        else {
          if ((beta > minGirth) && (fabs(beta - minGirth) > FLT_EPSILON)) {
            beta = minGirth;
          }
        }
      }
      // 将更新后的alpha和beta传给其他节点
      if (graph->partitions > 1) {
        for (VertexId v_i = 1;v_i < graph->partitions;v_i++) {
          MPI_Send(&alpha, 1, MPI_FLOAT, v_i, 0, MPI_COMM_WORLD);
          MPI_Send(&beta, 1, MPI_FLOAT, v_i, 0, MPI_COMM_WORLD);
        }
      }
      printf("alpha:%f\tbeta:%f\n", alpha, beta);
    }
    else {
      // 其他节点接收更新后的alpha和beta
      MPI_Recv(&alpha, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv(&beta, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    i++;
  }

  if (graph->partition_id == 0) {
    if (fabs(minGirth - 1e9) <= FLT_EPSILON) {
      printf("no circle\n");
    }
    else {
      printf("mingirth:%f\n", beta);
    }
  }

  exec_time += get_time();
  if (graph->partition_id == 0) {
    printf("exec_time=%lf(s)\n", exec_time);
  }

  delete graph;
  return 0;
}