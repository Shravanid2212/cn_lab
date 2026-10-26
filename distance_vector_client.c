#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX 10
#define BUFFER_SIZE 4096

int main()
{
    int sock;

    struct sockaddr_in serverAddress;

    int n;

    int graph[MAX][MAX];

    char buffer[BUFFER_SIZE];


    /*
        Create socket
    */
    sock = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (sock < 0)
    {
        perror("Socket creation failed");
        return 1;
    }


    /*
        Server address
    */
    serverAddress.sin_family =
        AF_INET;

    serverAddress.sin_port =
        htons(PORT);


    inet_pton(
        AF_INET,
        "127.0.0.1",
        &serverAddress.sin_addr
    );


    /*
        Connect
    */
    printf("Connecting to server...\n");


    if (connect(
            sock,
            (struct sockaddr *)&serverAddress,
            sizeof(serverAddress)
        ) < 0)
    {
        perror("Connection failed");
        close(sock);
        return 1;
    }


    printf("Connected to server.\n");


    /*
        Number of routers
    */
    printf("\nEnter number of routers: ");

    scanf("%d", &n);


    /*
        Cost matrix
    */
    printf("\nEnter cost matrix:\n");

    printf(
        "Enter 0 if there is no direct connection.\n\n"
    );


    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            scanf(
                "%d",
                &graph[i][j]
            );
        }
    }


    /*
        Send number of routers
    */
    send(
        sock,
        &n,
        sizeof(n),
        0
    );


    /*
        Send graph
    */
    send(
        sock,
        graph,
        sizeof(graph),
        0
    );


    printf("\nTopology sent to server.\n");

    printf("Waiting for routing tables...\n");


    /*
        Receive result
    */
    memset(
        buffer,
        0,
        sizeof(buffer)
    );


    int bytesReceived;

    bytesReceived = recv(
        sock,
        buffer,
        sizeof(buffer) - 1,
        0
    );


    if (bytesReceived <= 0)
    {
        printf("Failed to receive result.\n");

        close(sock);

        return 1;
    }


    buffer[bytesReceived] = '\0';


    /*
        Display result
    */
    printf("\n");
    printf("============================================\n");
    printf("        RESULT FROM SERVER\n");
    printf("============================================\n");

    printf("%s", buffer);


    close(sock);

    return 0;
}