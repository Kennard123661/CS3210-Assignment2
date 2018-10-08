//
// Created by kennard on 07/10/18.
//
#include <openmpi/mpi.h>

using namespace std;

int main(void) {
    const unsigned int STATUS_NOT_PROCESSING = 0;
    const unsigned int STATUS_FINISHED_PROCESSING = 1;
    const unsigned int STATUS_IS_PROCESSING = 2;

    MPI_Comm parentcomm;
    unsigned int linkInfo[3] = {0};

    MPI_Init(NULL,NULL);
    MPI_Comm_get_parent(&parentcomm);
    int world_rank, intercommRank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_rank(parentcomm, &intercommRank);

    MPI_Recv(linkInfo, 3, MPI_UINT32_T, 0, 0, parentcomm, NULL);
    // cout << "child number " << world_rank << " and I received (" << linkInfo[0] << "," << linkInfo[1] << "," << linkInfo[2] << ")" << endl;
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(parentcomm);

    // cout << linkInfo[0] << "," << linkInfo[1] << "," << linkInfo[2] << endl;

    unsigned int numTicks;
    MPI_Bcast(&numTicks, 1, MPI_UINT32_T, 0, parentcomm);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(parentcomm);

    unsigned int countDown(0);
    unsigned int status = STATUS_NOT_PROCESSING;
    for (unsigned int t = 0; t < numTicks; t++) {
        // cout << status << "c ";
        MPI_Send(&status, 1, MPI_UINT32_T, 0, 0, parentcomm);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Barrier(parentcomm);
        if (status == STATUS_IS_PROCESSING) {
            countDown -= 1;
        } else {
            MPI_Recv(&status, 1, MPI_UINT32_T, 0, 0, parentcomm, NULL);
            if (status == STATUS_IS_PROCESSING) {
                countDown = linkInfo[2] - 1;
            }
        }

        if ((status == STATUS_IS_PROCESSING) && (countDown <= 0)) {
            status = STATUS_FINISHED_PROCESSING;
        }
        MPI_Barrier(parentcomm);
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}