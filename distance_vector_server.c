#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX 10
#define BUFFER_SIZE 8192
#define INF 9999


/*
    Distance Vector Algorithm
*/
void distanceVector(
    int graph[MAX][MAX],
    int n,
    char *result
)
{
    int distance[MAX][MAX];

    int nextHop[MAX][MAX];


    /*
        Initialize routing table
    */
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            if (i == j)
            {
                distance[i][j] = 0;
                nextHop[i][j] = i;
            }
            else if (graph[i][j] != 0)
            {
                distance[i][j] =
                    graph[i][j];

                nextHop[i][j] = j;
            }
            else
            {
                distance[i][j] = INF;
                nextHop[i][j] = -1;
            }
        }
    }


    /*
        Repeatedly update
        distance vectors.
    */
    int changed = 1;

    while (changed)
    {
        changed = 0;


        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                for (int k = 0; k < n; k++)
                {
                    if (distance[i][k] != INF &&
                        distance[k][j] != INF)
                    {
                        int newDistance =
                            distance[i][k] +
                            distance[k][j];


                        if (newDistance <
                            distance[i][j])
                        {
                            distance[i][j] =
                                newDistance;

                            nextHop[i][j] =
                                nextHop[i][k];

                            changed = 1;
                        }
                    }
                }
            }
        }
    }


    /*
        Create result
    */
    strcpy(
        result,
        "\nDISTANCE VECTOR ROUTING\n"
        "============================================\n"
    );


    /*
        Generate routing table
        for every router.
    */
    for (int i = 0; i < n; i++)
    {
        char header[200];

        sprintf(
            header,
            "\nRouting Table for Router %c\n"
            "--------------------------------------------\n"
            "%-15s %-10s %-10s\n",
            'A' + i,
            "Destination",
            "Cost",
            "Next Hop"
        );

        strcat(
            result,
            header
        );


        for (int j = 0; j < n; j++)
        {
            char line[200];


            if (distance[i][j] == INF)
            {
                sprintf(
                    line,
                    "%-15c %-10s %-10s\n",
                    'A' + j,
                    "INF",
                    "-"
                );
            }
            else
            {
                sprintf(
                    line,
                    "%-15c %-10d %-10c\n",
                    'A' + j,
                    distance[i][j],
                    'A' + nextHop[i][j]
                );
            }


            strcat(
                result,
                line
            );
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
    serverAddress.sin_family =
        AF_INET;

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
    printf("       DISTANCE VECTOR SERVER\n");
    printf("============================================\n");

    printf(
        "Waiting for client on port %d...\n",
        PORT
    );


    /*
        Accept client
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


    printf("Network topology received.\n");

    printf(
        "Running Distance Vector algorithm...\n"
    );


    /*
        Run algorithm
    */
    distanceVector(
        graph,
        n,
        result
    );


    /*
        Display on server
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


    printf(
        "Routing tables sent to client.\n"
    );


    close(clientSocket);

    close(serverSocket);


    return 0;
}