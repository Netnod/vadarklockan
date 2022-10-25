#include "clusteringAlgorithm.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct Edge{
    struct Edge *next_edge;
    struct Edge *prev_edge;

    int amount_edges;
    int chime;
    double value;
}Edge;

typedef struct EdgeList{
    Edge *root;
}EdgeList;

struct clusteringData{
    EdgeList *edge_list;
    
    double lo;
    double hi;
    double adjustment;
    double uncertainty;

    int n;
    int bad;
    int wanted;
    int amount_responses;
};

Edge* createEdge(double value, int chime){
    Edge *edge = calloc(1, sizeof(Edge));
    if(!edge) return NULL;

    edge->value = value;
    edge->chime = chime;

    edge->next_edge = NULL;
    edge->prev_edge = NULL;

    return edge;
}

EdgeList* createEdgeList(){
    EdgeList *edge_list = calloc(1, sizeof(EdgeList));
    if(!edge_list) return NULL;

    edge_list->root = NULL;

    return edge_list;
}

clusteringData* createClusterData(int n){
    clusteringData *new_edge_list = calloc(1, sizeof(clusteringData));

    new_edge_list->edge_list = createEdgeList();
    if(!new_edge_list->edge_list) return NULL;

    new_edge_list->amount_responses = 0;
    new_edge_list->n = n;
    new_edge_list->bad = 0;
    new_edge_list->wanted = 0;
    new_edge_list->adjustment = 0;
    new_edge_list->uncertainty = 0;
    new_edge_list->hi = 0;
    new_edge_list->lo = 0;

    return new_edge_list;
}

void sortedInsert(Edge** root, Edge* new_edge)
{
    Edge* current;
 
    // if list is empty
    if (*root == NULL)
        *root = new_edge;
 
    // if the node is to be inserted at the beginning
    // of the doubly linked list
    else if ((*root)->value >= new_edge->value) {
        new_edge->next_edge = *root;
        new_edge->next_edge->prev_edge = new_edge;
        *root = new_edge;
    }
 
    else {
        current = *root;
 
        // locate the node after which the new node
        // is to be inserted
        while (current->next_edge != NULL &&
               current->next_edge->value < new_edge->value){
            current = current->next_edge;
        }
 
        /*Make the appropriate links */
 
        new_edge->next_edge = current->next_edge;
 
        // if the new node is not inserted
        // at the end of the list
        if (current->next_edge != NULL)
            new_edge->next_edge->prev_edge = new_edge;
 
        current->next_edge = new_edge;
        new_edge->prev_edge = current;
    }
}

int insertEdge(EdgeList *edge_list, double value, int chime){
    Edge* new_edge = createEdge(value, chime);

    // Checks if memory is allocated correctly
    if(new_edge == NULL) return -1;

    sortedInsert(&(edge_list->root), new_edge);
}

void print_tree(clusteringData *server_cluster_data)
{
    Edge *temp = server_cluster_data->edge_list->root;
    while(temp->next_edge != NULL){
        printf("%f ", temp->value);
        temp = temp->next_edge;
    }
    printf("\n\n");
}

int find_overlap(clusteringData *server_cluster_data, double adjust, double uncert){

    server_cluster_data->amount_responses++;
    double lo = 0;
    double hi = 0;

    if(insertEdge(server_cluster_data->edge_list, adjust-uncert, -1) == -1){
        printf("Left edge fail\n");
        return -1;
    }

    if(!insertEdge(server_cluster_data->edge_list, adjust+uncert, +1) == -1){
        printf("Right edge fail\n");
        return -1;
    }
    
    int max_allow = server_cluster_data->amount_responses - server_cluster_data->n;

    // Too few edges currently added
    if (max_allow < 0){
        return 0;
    }    

    for(int i = 0; i<=max_allow; i++){

        server_cluster_data->wanted = server_cluster_data->amount_responses - i;

        int chime = 0;

        // Make a copy of the root
        Edge *current_edge = server_cluster_data->edge_list->root;

        // Find lo
        while(current_edge->next_edge != NULL)
        {
            chime -= current_edge->chime;
            if(chime >= server_cluster_data->wanted)
            {
                lo = current_edge->value;
                break;
            }
            current_edge = current_edge->next_edge;
        }

        chime = 0;
        // Reverse edge list
        while(current_edge->next_edge != NULL){
            current_edge = current_edge->next_edge;
        }

        // Find hi
        while(current_edge->prev_edge != NULL){
            chime += current_edge->chime;
            if (chime >= server_cluster_data->wanted){
                hi = current_edge->value;
                break;
            }
            current_edge = current_edge->prev_edge;
        }

        if(lo != 0 && hi != 0 && lo <= hi){
            break;
        }     
    }
      
    server_cluster_data->lo = lo;
    server_cluster_data->hi = hi;
    server_cluster_data->adjustment = (lo + hi) / 2;
    server_cluster_data->uncertainty = (hi - lo) / 2;
    return 0;
}

int is_overlap(clusteringData *server_cluster_data, int server_lo, int server_hi){
    if(server_lo > server_cluster_data->hi || server_hi < server_cluster_data->lo){
        return 0;
    }
}

double get_adjustment(clusteringData *server_cluster_data)
{
    return server_cluster_data->adjustment;
}

int free_tree(clusteringData *server_cluster_data)
{
    Edge *current;
    while(server_cluster_data->edge_list->root != NULL){
        current = server_cluster_data->edge_list->root;
        server_cluster_data->edge_list->root = server_cluster_data->edge_list->root->next_edge;
        free(current);
    }
    free(server_cluster_data->edge_list);
    free(server_cluster_data);
    return 0;
}