#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main()
{
    int sock;
    struct sockaddr_in server;

    char ip[50];
    char searchIP[50];

    char result[5000];
    char searchResult[1000];

    char ready[20];

    int subnets;
    int required[20];

    /* ==============================
       CREATE SOCKET
       ============================== */

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    /* ==============================
       SERVER DETAILS
       ============================== */

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr =
        inet_addr("127.0.0.1");

    /* ==============================
       CONNECT
       ============================== */

    if (connect(sock,
                (struct sockaddr *)&server,
                sizeof(server)) < 0)
    {
        perror("Connection failed");
        close(sock);
        return 1;
    }

    printf("===============================================\n");
    printf("             VLSM SUBNETTING CLIENT\n");
    printf("===============================================\n");

    /* ==============================
       INPUT NETWORK IP
       ============================== */

    printf("\nEnter IP address: ");
    scanf("%49s", ip);

    /* ==============================
       INPUT NUMBER OF SUBNETS
       ============================== */

    printf("Enter number of subnets: ");
    scanf("%d", &subnets);

    if (subnets <= 0 || subnets > 20)
    {
        printf("Invalid number of subnets.\n");
        close(sock);
        return 1;
    }

    /* ==============================
       INPUT REQUIREMENTS
       ============================== */

    printf("\nEnter required number of IP addresses for each subnet:\n");

    for (int i = 0; i < subnets; i++)
    {
        printf("Subnet %d: ", i + 1);
        scanf("%d", &required[i]);

        if (required[i] <= 0)
        {
            printf("Invalid number of IP addresses.\n");
            close(sock);
            return 1;
        }
    }

    /* ==============================
       SEND NETWORK IP
       ============================== */

    send(sock,
         ip,
         sizeof(ip),
         0);

    /* ==============================
       SEND NUMBER OF SUBNETS
       ============================== */

    send(sock,
         &subnets,
         sizeof(subnets),
         0);

    /* ==============================
       SEND REQUIREMENTS
       ============================== */

    send(sock,
         required,
         sizeof(required),
         0);

    /* ==============================
       RECEIVE VLSM RESULT
       ============================== */

    memset(result, 0, sizeof(result));

    recv(sock,
         result,
         sizeof(result),
         0);

    printf("\n%s\n", result);

    /* ==============================
       TELL SERVER THAT CLIENT
       RECEIVED THE RESULT
       ============================== */

    strcpy(ready, "READY");

    send(sock,
         ready,
         sizeof(ready),
         0);

    /* ==============================
       INPUT IP TO SEARCH
       ============================== */

    printf("\nEnter IP address to find its subnet: ");
    scanf("%49s", searchIP);

    /* ==============================
       SEND SEARCH IP
       ============================== */

    send(sock,
         searchIP,
         sizeof(searchIP),
         0);

    /* ==============================
       RECEIVE SEARCH RESULT
       ============================== */

    memset(searchResult, 0, sizeof(searchResult));

    recv(sock,
         searchResult,
         sizeof(searchResult),
         0);

    printf("\n%s\n", searchResult);

    close(sock);

    return 0;
}