#include<bits/stdc++.h>
#include <sys/time.h>
using namespace std;
int edge[100][100];
int dis[100][100];

void split(string s, string delimiter, vector<string> &res) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    res.clear();

    while ((pos_end = s.find(delimiter, pos_start)) != string::npos) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
}

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (tv.tv_usec / 1e6);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("floyd [file] [vertices]\n");
        exit(-1);
    }

    double exec_time = 0;
    exec_time -= get_time();

    char *fin = argv[1];
    ifstream in(fin);
    int vertices = atoi(argv[2]), m;
    //  初始化，所有点之间无边
    for (int i = 0;i < vertices;i++)
        for (int j = 0;j < vertices;j++)
            if (i != j)
                edge[i][j] = dis[i][j] = 1e8;

    string line;
    vector<string> container;
    uint32_t src, dst;
    if (in.is_open()) {
        while (getline(in, line)) {
            split(line, " ", container);
            src = (uint32_t) stoi(container[0]);
            dst = (uint32_t) stoi(container[1]);
            assert(src >= 0 && dst >= 0);
            dis[src][dst] = dis[dst][src] = edge[src][dst] = edge[dst][src] = 1;
        }
    }
    else {
        cout << "cannot open input file" << endl;
    }

    int ans = 1e8;
    for (int k = 0;k < vertices;k++) {
        for (int i = 0;i < k;i++) {
            for (int j = i + 1;j < k;j++) {
                ans = min(ans, dis[i][j] + edge[j][k] + edge[k][i]);
            }
        }
        for (int i = 0;i < vertices;i++) {
            for (int j = 0;j < vertices;j++) {
                dis[i][j] = min(dis[i][j], dis[i][k] + dis[k][j]);
            }
        }
    }

    exec_time += get_time();
    if (ans == 1e8) {
        cout << "cannot find circle" << endl;
    }
    else {
        cout << "girth:" << ans << endl;
    }
    cout << "exec_time:" << fixed << exec_time << "s" << endl;
    return 0;
}