#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "foothread.h"

#define MAX_NODES 25

// Structure to store parent-child relationships
typedef struct {
    int parent;
    int child;
} NodeRelationship;

// Global variables
int leaf_nodes[MAX_NODES] = {0}; // Array to store leaf nodes
int sums[MAX_NODES] = {0};       // Array to store partial sums
int num_leaf_nodes = 0;           // Number of leaf nodes
foothread_barrier_t barrier[MAX_NODES]; // Array of barriers
int num_children[MAX_NODES] = {0}; // Array to store number of children for each node

// Function to identify leaf nodes
void identify_leaf_nodes(NodeRelationship relationships[], int num_nodes) {
    for (int i = 0; i < num_nodes; i++) {
        int is_leaf = 1;
        for (int j = 0; j < num_nodes; j++) {
            if (relationships[j].parent == relationships[i].child) {
                is_leaf = 0;
                break;
            }
        }
        if (is_leaf) {
            leaf_nodes[num_leaf_nodes++] = relationships[i].child;
            printf("no.ofleafnodes:%d\n",num_leaf_nodes);
        }
    }
}

// Function to compute sum recursively
void compute_sum(int node, NodeRelationship relationships[], int num_nodes) {
    int sum = 0;
    printf("comp sum %d\n",node);

    // Check if the current node is a leaf node
    int is_leaf = 1;
    int num_children = 0;
    for (int i = 0; i < num_nodes; i++) {
        if (relationships[i].parent == node) {
            is_leaf = 0;
            num_children++;
        }
    }
    if (is_leaf) {
        foothread_barrier_wait(&barrier[node]);

        return; // Break out of the recursion if it's a leaf node
    }

    // Compute sum for child nodes
    for (int i = 0; i < num_nodes; i++) {
        if (relationships[i].parent == node) {
            if(relationships[i].child==node)continue;
            int child = relationships[i].child;
            compute_sum(child, relationships, num_nodes);
            sum += sums[child];
            foothread_barrier_wait(&barrier[child]);
            break;
        }
    }

    // Update sum for the current node
    sums[node] = sum;
    printf("Internal node %d gets the partial sum %d from its children\n", node, sum);

    // Wait for other threads to reach this barrier
    if (num_children > 0) { // Only wait if there are children
        foothread_barrier_wait(&barrier[node]);
    }
}

int identify_root_node(NodeRelationship relationships[], int num_nodes) {
    for (int i = 0; i < num_nodes; i++) {
        if (relationships[i].parent == relationships[i].child) {
            return relationships[i].child; // Return the index of the root node
        }
    }
    return -1; // No root node found
}

// Main function
int main() {
    NodeRelationship relationships[MAX_NODES];
    int num_nodes = 0;

    // Open and read the tree file
    FILE *file = fopen("tree.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }
    if (fscanf(file, "%d", &num_nodes) != 1) {
        perror("Error reading number of nodes");
        fclose(file);
        return 1;
    }
    while (fscanf(file, "%d %d", &relationships[num_nodes].child, &relationships[num_nodes].parent) == 2) {
        num_nodes++;
    }
    fclose(file);

    // Identify leaf nodes and count number of children for each node
    for (int i = 0; i < num_nodes; i++) {
        num_children[relationships[i].parent]++;
    }

    // Initialize barriers with number of children for each node
    for (int i = 0; i < num_nodes; i++) {
        foothread_barrier_init(&barrier[i], num_children[i]);
    }

    // Identify leaf nodes
    identify_leaf_nodes(relationships, num_nodes);

    // Manually input values for each leaf node
    for (int i = 0; i < num_leaf_nodes; i++) {
        printf("Leaf node %d :: Enter a positive integer: ", leaf_nodes[i]);
        scanf("%d", &sums[leaf_nodes[i]]);
    }
    int root=identify_root_node(relationships,num_nodes);
    // Compute sum recursively
    compute_sum(root, relationships, num_nodes);

    // Print sum at root
    printf("Sum at root (node 0) = %d\n", sums[0]);

    // Destroy barriers
    for (int i = 0; i < num_nodes; i++) {
        foothread_barrier_destroy(&barrier[i]);
    }

    // Exit
    foothread_exit();

    return 0;
}
