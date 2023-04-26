#include<bits/stdc++.h>
#include<mpi.h>
using namespace std;
int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);    //初始化MPI环境
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);    //获取进程组中进程编号rank
    MPI_Comm_size(MPI_COMM_WORLD, &size);    //获取进程组中进程数size

    bool number;
    if (rank == 0) {        
        number = false;
        MPI_Send(
            /* data         = */ &number,
            /* count        = */ 1,
            /* datatype     = */ MPI_CHAR,
            /* destination  = */ 1,
            /* tag          = */ 0,
            /* communicator = */ MPI_COMM_WORLD);
    }
    else {
        MPI_Recv(
            /* data         = */ &number,
            /* count        = */ 1,
            /* datatype     = */ MPI_CHAR,
            /* source       = */ 0,
            /* tag          = */ 0,
            /* communicator = */ MPI_COMM_WORLD,
            /* status       = */ MPI_STATUS_IGNORE);
        cout << number << endl;
    }

    MPI_Finalize();    //退出MPI环境

    return 0;
}