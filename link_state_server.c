#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <limits.h>

#define PORT 8080
#define MAX 10
#define BUFFER_SIZE 4096
#define INF 9999


/*
    Find vertex with minimum distance.
*/
int findMin(
    int distance[],
    int visited[],
    int n
)
{
    int min = INF;
    int index = -1;

    for (int i = 0; i < n; i++)
    {
        if (!visited[i] &&
            distance[i] < min)
        {
            min = distance[i];
            index = i;
        }
    }

    return index;
}


/*
    Print shortest path.
*/
void printPath(
    int parent[],
    int vertex,
    char *result
)
{
    if (parent[vertex] == -1)
    {
        char temp[10];

        sprintf(
            temp,
            "%c",
            'A' + vertex
        );

        strcat(result, temp);

        return;
    }

    printPath(
        parent,
        parent[vertex],
        result
    );

    char temp[10];

    sprintf(
        temp,
        " -> %c",
        'A' + vertex
    );

    strcat(result, temp);
}


/*
    Dijkstra Algorithm
*/
void dijkstra(
    int graph[MAX][MAX],
    int n,
    int source,
    char *result
)
{
    int distance[MAX];
    int visited[MAX];
    int parent[MAX];


    /*
        Initialize
    */
    for (int i = 0; i < n; i++)
    {
        distance[i] = INF;
        visited[i] = 0;
        parent[i] = -1;
    }


    distance[source] = 0;


    /*
        Dijkstra
    */
    for (int count = 0;
         count < n - 1;
         count++)
    {
        int u = findMin(
            distance,
            visited,
            n
        );

        if (u == -1)
            break;

        visited[u] = 1;


        for (int v = 0; v < n; v++)
        {
            if (!visited[v] &&
                graph[u][v] != 0 &&
                distance[u] != INF &&
                distance[u] +
                graph[u][v] <
                distance[v])
            {
                distance[v] =
                    distance[u] +
                    graph[u][v];

                parent[v] = u;
            }
        }
    }


    /*
        Create result
    */
    sprintf(
        result,
        "\nLINK STATE ROUTING\n"
        "DIJKSTRA ALGORITHM\n"
        "Source Router: %c\n\n"
        "%-12s %-10s %-30s\n"
        "----------------------------------------------\n",
        'A' + source,
        "Router",
        "Cost",
        "Shortest Path"
    );


    /*
        Add routes
    */
    for (int i = 0; i < n; i++)
    {
        if (i == source)
            continue;

        char path[200] = "";

        if (distance[i] == INF)
        {
            char line[100];

            sprintf(
                line,
                "%-12c %-10s No path\n",
                'A' + i,
                "INF"
            );

            strcat(result, line);
        }
        else
        {
            printPath(
                parent,
                i,
                path
            );

            char line[300];

            sprintf(
                line,
                "%-12c %-10d %-30s\n",
                'A' + i,
                distance[i],
                path
            );

            strcat(result, line);
        }
    }
}


int main()
{
    int serverSocket;
    int clientSocket;

    struct sockaddr_in serverAddress;
    struct sockaddr_in clientAddress;

    socklen_t clientLength;

    int n;
    int source;

    int graph[MAX][MAX];

    char result[BUFFER_SIZE];


    /*
        Create socket
    */
    serverSocket = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (serverSocket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }


    /*
        Server address
    */
    serverAddress.sin_family = AF_INET;

    serverAddress.sin_addr.s_addr =
        INADDR_ANY;

    serverAddress.sin_port =
        htons(PORT);


    /*
        Bind
    */
    if (bind(
            serverSocket,
            (struct sockaddr *)&serverAddress,
            sizeof(serverAddress)
        ) < 0)
    {
        perror("Bind failed");
        close(serverSocket);
        return 1;
    }


    /*
        Listen
    */
    listen(
        serverSocket,
        5
    );


    printf("============================================\n");
    printf("       LINK STATE ROUTING SERVER\n");
    printf("============================================\n");

    printf("Waiting for client on port %d...\n",
           PORT);


    /*
        Accept
    */
    clientLength =
        sizeof(clientAddress);

    clientSocket = accept(
        serverSocket,
        (struct sockaddr *)&clientAddress,
        &clientLength
    );


    if (clientSocket < 0)
    {
        perror("Accept failed");
        close(serverSocket);
        return 1;
    }


    printf("Client connected.\n");


    /*
        Receive number of routers
    */
    recv(
        clientSocket,
        &n,
        sizeof(n),
        0
    );


    /*
        Receive graph
    */
    recv(
        clientSocket,
        graph,
        sizeof(graph),
        0
    );


    /*
        Receive source
    */
    recv(
        clientSocket,
        &source,
        sizeof(source),
        0
    );


    printf("Network topology received.\n");

    printf("Running Dijkstra algorithm...\n");


    /*
        Run Dijkstra
    */
    dijkstra(
        graph,
        n,
        source,
        result
    );


    /*
        Print on server
    */
    printf("\n%s\n", result);


    /*
        Send result to client
    */
    send(
        clientSocket,
        result,
        strlen(result),
        0
    );


    printf("Result sent to client.\n");


    close(clientSocket);
    close(serverSocket);

    return 0;
}