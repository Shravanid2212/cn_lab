
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>

#define PORT 8080

int main()
{
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;

    socklen_t client_len = sizeof(client_addr);

    int window_size;
    int total_packets;
    int loss_n;

    int expected = 1;
    int next_loss;

    int *dropped;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock < 0)
    {
        perror("Socket error");
        return 1;
    }

    int opt = 1;

    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("Bind error");
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, 1) < 0)
    {
        perror("Listen error");
        close(server_sock);
        return 1;
    }

    printf("GBN Server waiting for client...\n");

    client_sock = accept(server_sock,
                         (struct sockaddr *)&client_addr,
                         &client_len);

    if (client_sock < 0)
    {
        perror("Accept error");
        close(server_sock);
        return 1;
    }

    recv(client_sock, &window_size, sizeof(window_size), 0);
    recv(client_sock, &total_packets, sizeof(total_packets), 0);
    recv(client_sock, &loss_n, sizeof(loss_n), 0);

    dropped = calloc(total_packets + 1, sizeof(int));

    if (dropped == NULL)
    {
        printf("Memory allocation failed.\n");
        close(client_sock);
        close(server_sock);
        return 1;
    }

    next_loss = loss_n;

    printf("\n====================================\n");
    printf("             GBN SERVER\n");
    printf("====================================\n");

    printf("Window size   : %d\n", window_size);
    printf("Total packets : %d\n", total_packets);
    printf("Loss parameter: %d\n", loss_n);

    while (1)
    {
        int packet;
        int type;

        if (recv(client_sock, &packet, sizeof(packet), 0) <= 0)
            break;

        if (packet == -1)
        {
            printf("\nTransmission completed.\n");
            break;
        }

        if (recv(client_sock, &type, sizeof(type), 0) <= 0)
            break;

        printf("\nReceived packet %d (%s)\n",
               packet,
               type == 1 ? "new transmission" : "retransmission");

        /*
           Simulate the designated loss only once.
        */

        if (packet == next_loss && !dropped[packet])
        {
            dropped[packet] = 1;

            printf("SIMULATED LOSS: Packet %d dropped.\n", packet);

            next_loss += loss_n - 1;

            continue;
        }

        /*
           Process packets in order.
        */

        if (packet == expected)
        {
            printf("Packet %d accepted.\n", packet);

            expected++;

            printf("ACK %d generated.\n", packet);
        }
        else if (packet > expected)
        {
            printf("Packet %d discarded; waiting for %d.\n",
                   packet, expected);

            printf("Duplicate ACK %d generated.\n", expected - 1);
        }
        else
        {
            printf("Duplicate packet %d received.\n", packet);

            printf("ACK %d generated.\n", expected - 1);
        }
    }

    free(dropped);

    close(client_sock);
    close(server_sock);

    return 0;
}

