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
    char result[2500];

    /* Create socket */

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    /* Server details */

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* Connect */

    if (connect(sock,
                (struct sockaddr *)&server,
                sizeof(server)) < 0)
    {
        perror("Connection failed");
        close(sock);
        exit(1);
    }

    printf("=====================================\n");
    printf("        IP CLASSIFICATION CLIENT\n");
    printf("=====================================\n");

    printf("Connected to server.\n\n");

    /* Input IP */

    printf("Enter IP address: ");
    scanf("%49s", ip);

    /* Send IP */

    send(sock,
         ip,
         sizeof(ip),
         0);

    printf("\nIP sent to server.\n");

    /* Receive result */

    memset(result, 0, sizeof(result));

    recv(sock,
         result,
         sizeof(result),
         0);

    /* Display result */

    printf("%s\n", result);

    close(sock);

    return 0;
}