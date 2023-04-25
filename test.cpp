#include<bits/stdc++.h>
#include<mpi.h>
using namespace std;
int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);    //初始化MPI环境
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);    //获取进程组中进程编号rank
    MPI_Comm_size(MPI_COMM_WORLD, &size);    //获取进程组中进程数size

    float number;
    if (rank == 0) {        
        number = 2.5;
        MPI_Send(
            /* data         = */ &number,
            /* count        = */ 1,
            /* datatype     = */ MPI_FLOAT,
            /* destination  = */ 1,
            /* tag          = */ 0,
            /* communicator = */ MPI_COMM_WORLD);
    }
    else {
        MPI_Recv(
            /* data         = */ &number,
            /* count        = */ 1,
            /* datatype     = */ MPI_FLOAT,
            /* source       = */ 0,
            /* tag          = */ 0,
            /* communicator = */ MPI_COMM_WORLD,
            /* status       = */ MPI_STATUS_IGNORE);
        printf("Process %d received number %f from process 0\n", rank, number);
    }

    MPI_Finalize();    //退出MPI环境
    return 0;
}