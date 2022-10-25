typedef unsigned char uint8_t;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clusteringData clusteringData;

// Creates a struct to store info
clusteringData* createClusterData(int n);

// Calculates the overlap
int find_overlap(clusteringData *server_cluster_data, double adjust, double uncert);

// Function to see if a respons is whitin the overlap boundaries
int is_overlap(clusteringData *server_cluster_data, int server_lo, int server_hi);

// Returns the adjustment
double get_adjustment(clusteringData *server_cluster_data);

// Prints all the edges
void print_tree(clusteringData *server_cluster_data);

// Free all the memory allocated by the linked list
int free_tree(clusteringData *server_cluster_data);

#ifdef __cplusplus
}
#endif
