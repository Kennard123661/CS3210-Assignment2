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
/*

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

    vector<vector<unsigned int>> linkCosts;
    linkCosts.resize(numStations);
    for (unsigned int i = 0; i < numStations; i++) {
        linkCosts[i].resize(numStations);
        for (unsigned int j = 0; j < numStations; j++) {
            cin >> linkCosts[i][j];
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
*/

    // Initialize the MPI environment

    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    std::cout << "Hello, World!" << std::endl;

    // Get the name of the processor
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    unsigned int NUM_SPAWNS = 3;
    unsigned int np = NUM_SPAWNS;
    int errcodes[NUM_SPAWNS];
    MPI_Comm parentcomm, intercomm;

    MPI_Comm_spawn("./simulation_process", MPI_ARGV_NULL, np, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm, errcodes );
    printf("I'm the parent.\n");
    // Print off a hello world message
    printf("Hello world from processor %s, rank %d out of %d processors\n",
           processor_name, world_rank, world_size);

    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}