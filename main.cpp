#include <iostream>
#include <openmpi/mpi.h>
#include <vector>
#include <sstream>

using namespace std;

void getStationNames(vector<string> &stationNames) {
    string rawStationNames;
    cin >> rawStationNames;

    char delimiter = ',';
    stringstream ss(rawStationNames);
    string stationName;
    while (getline(ss, stationName, delimiter)) {
        stationNames.push_back(stationName);
    }
}


int main() {
    const unsigned int NUM_LINES = 3;

    // Read inputs
    unsigned int numStations;
    cin >> numStations;

    vector<string> stationNames;
    getStationNames(stationNames);
    map<string, unsigned int> stationNamesToStationId;
    for (unsigned int i = 0; i < numStations; i++) {
        stationNamesToStationId.insert(pair<string, unsigned int>(stationNames[i], i));
        // cout << stationNames[i] << endl;
    }

    std::vector<pair<unsigned int, unsigned int>> links;
    vector<vector<unsigned int>> linkCosts;
    linkCosts.resize(numStations);
    for (unsigned int i = 0; i < numStations; i++) {
        linkCosts[i].resize(numStations);
        for (unsigned int j = 0; j < numStations; j++) {
            cin >> linkCosts[i][j];
            if (linkCosts[i][j] > 0) {
                links.push_back(pair<unsigned int, unsigned int>(i,j));
            }
        }
    }

    vector<float> stationPopularities(numStations);
    for (unsigned int i = 0; i < numStations; i++) {
        cin >> stationPopularities[i];
    }

    vector<vector<unsigned int>> line_stationIds;
    line_stationIds.resize(NUM_LINES);
    for (unsigned int i = 0; i < NUM_LINES; i++) {
        vector<string> stationsInLine;
        getStationNames(stationsInLine);
        line_stationIds[i].resize(stationsInLine.size());
        for (unsigned int j = 0; j < stationsInLine.size(); j++) {
            unsigned int stationId = stationNamesToStationId[stationsInLine[j]];
            line_stationIds[i][j] = stationId;
        }
    }

    unsigned int numTicks;
    cin >> numTicks;

    unsigned int numTrainsPerLine[NUM_LINES];
    for (unsigned int i = 0; i < NUM_LINES; i++) {
        cin >> numTrainsPerLine[i];
    }

    // Initialize the MPI environment

    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Get the name of the processor
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    unsigned int numChildProcesses = links.size();
    int* errcodes = (int *) malloc(sizeof(int) * numChildProcesses);

    MPI_Comm intercomm;
    // Spawn link processes
    MPI_Comm_spawn("./simulation_process", MPI_ARGV_NULL, numChildProcesses, MPI_INFO_NULL, 0,
            MPI_COMM_WORLD, &intercomm, errcodes);

    int intercommRank;
    MPI_Comm_rank(intercomm, &intercommRank);

    // Initialize link processes
    unsigned int** linkCostTuples = (unsigned int **) malloc(sizeof(unsigned int*) * links.size());
    for (unsigned int i = 0; i < links.size(); i++) {
        linkCostTuples[i] = (unsigned int*) malloc(sizeof(unsigned int) * 3);
        linkCostTuples[i][0] = links[i].first;
        linkCostTuples[i][1] = links[i].second;
        linkCostTuples[i][2] = linkCosts[links[i].first][links[i].second];
        MPI_Send(linkCostTuples[i], 3, MPI_UINT32_T, i, 0, intercomm);
    }


    for (unsigned int t = 0; t < numTicks; t++) {

    }


    MPI_Finalize();

    return 0;
}