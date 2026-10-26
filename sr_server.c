#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define MAX_PACKETS 100

#define ACK 0
#define NACK -1
#define END_BATCH -2

/* Send one complete integer */
int send_int(int sock, int value)
{
    char *ptr = (char *)&value;
    int sent = 0;

    while (sent < sizeof(value))
    {
        int n = send(sock, ptr + sent,
                     sizeof(value) - sent, 0);

        if (n <= 0)
            return -1;

        sent += n;
    }

    return 0;
}

/* Receive one complete integer */
int recv_int(int sock, int *value)
{
    char *ptr = (char *)value;
    int received = 0;

    while (received < sizeof(*value))
    {
        int n = recv(sock, ptr + received,
                     sizeof(*value) - received, 0);

        if (n <= 0)
            return -1;

        received += n;
    }

    return 0;
}

int main()
{
    int server_fd, client_fd;

    struct sockaddr_in server, client;
    socklen_t addrlen = sizeof(client);

    int packets, msg, seq;

    int received[MAX_PACKETS + 1] = {0};
    int nack_sent[MAX_PACKETS + 1] = {0};

    int expected = 1;

    /* Create socket */

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("Socket error");
        return 1;
    }

    int opt = 1;

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    /* Bind */

    if (bind(server_fd,
             (struct sockaddr *)&server,
             sizeof(server)) < 0)
    {
        perror("Bind error");
        close(server_fd);
        return 1;
    }

    /* Listen */

    if (listen(server_fd, 5) < 0)
    {
        perror("Listen error");
        close(server_fd);
        return 1;
    }

    printf("=====================================\n");
    printf("       SELECTIVE REPEAT SERVER\n");
    printf("=====================================\n");

    printf("Waiting for client...\n");

    /* Accept client */

    client_fd = accept(server_fd,
                       (struct sockaddr *)&client,
                       &addrlen);

    if (client_fd < 0)
    {
        perror("Accept error");
        close(server_fd);
        return 1;
    }

    printf("Client connected.\n");

    /* Receive packet count */

    if (recv_int(client_fd, &packets) < 0 ||
        packets < 1 || packets > MAX_PACKETS)
    {
        printf("Invalid packet count or connection error.\n");
        close(client_fd);
        close(server_fd);
        return 1;
    }

    /* Process all batches using ONE main loop */

    while (expected <= packets)
    {
        if (recv_int(client_fd, &msg) < 0)
            break;

        /* End of current batch */

        if (msg == END_BATCH)
        {
            if (send_int(client_fd, END_BATCH) < 0)
                break;

            continue;
        }

        /* Validate packet number */

        if (msg < 1 || msg > packets)
        {
            printf("Invalid packet number received.\n");
            continue;
        }

        seq = msg;

        printf("\nPacket %d received.\n", seq);

        /* Accept packet or detect duplicate */

        if (!received[seq])
        {
            received[seq] = 1;
            nack_sent[seq] = 0;

            printf("Packet %d accepted.\n", seq);
        }
        else
        {
            printf("Duplicate packet %d received.\n", seq);
        }

        /*
         * If a later packet arrives while expected
         * is missing, request that missing packet.
         */

        if (seq > expected &&
            !received[expected] &&
            !nack_sent[expected])
        {
            printf("-ve ACK sent for packet %d\n", expected);

            if (send_int(client_fd, NACK) < 0 ||
                send_int(client_fd, expected) < 0)
            {
                break;
            }

            nack_sent[expected] = 1;
        }

        /* Send positive ACK for the received packet */

        if (send_int(client_fd, ACK) < 0 ||
            send_int(client_fd, seq) < 0)
        {
            break;
        }

        printf("ACK sent for packet %d\n", seq);

        /* Advance expected across consecutive received packets */

        while (expected <= packets && received[expected])
        {
            expected++;
        }
    }

    if (expected > packets)
        printf("\nAll packets received successfully.\n");
    else
        printf("\nConnection ended before all packets arrived.\n");

    close(client_fd);
    close(server_fd);

    return 0;
}