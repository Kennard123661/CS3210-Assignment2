//
// Created by kennard on 07/10/18.
//
#include <openmpi/mpi.h>

using namespace std;

int main(void) {
    MPI_Comm parentcomm, intercomm;

    MPI_Init(NULL,NULL);
    MPI_Comm_get_parent(&parentcomm);

    cout << "I am a worker process\n";
    MPI_Finalize();
    return EXIT_SUCCESS;
}