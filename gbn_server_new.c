#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int send_int(int sock, int value)
{
    char *p = (char *)&value;
    int sent = 0;

    while (sent < sizeof(value))
    {
        int n = send(sock, p + sent, sizeof(value) - sent, 0);

        if (n <= 0)
            return 0;

        sent += n;
    }

    return 1;
}

int recv_int(int sock, int *value)
{
    char *p = (char *)value;
    int received = 0;

    while (received < sizeof(*value))
    {
        int n = recv(sock, p + received,
                     sizeof(*value) - received, 0);

        if (n <= 0)
            return 0;

        received += n;
    }

    return 1;
}

int main()
{
    int server_sock, client_sock;

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    int window_size, total_packets, loss_interval;
    int expected = 1;

    int *dropped;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock < 0)
    {
        perror("Socket");
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
        perror("Bind");
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, 1) < 0)
    {
        perror("Listen");
        close(server_sock);
        return 1;
    }

    printf("Server waiting for client...\n");

    client_sock = accept(server_sock,
                         (struct sockaddr *)&client_addr,
                         &client_len);

    if (client_sock < 0)
    {
        perror("Accept");
        close(server_sock);
        return 1;
    }

    if (!recv_int(client_sock, &window_size) ||
        !recv_int(client_sock, &total_packets) ||
        !recv_int(client_sock, &loss_interval))
    {
        printf("Failed to receive configuration.\n");
        close(client_sock);
        close(server_sock);
        return 1;
    }

    dropped = calloc(total_packets + 1, sizeof(int));

    if (dropped == NULL)
    {
        printf("Memory allocation failed.\n");
        close(client_sock);
        close(server_sock);
        return 1;
    }

    printf("\nWindow size: %d\n", window_size);
    printf("Total packets: %d\n", total_packets);
    printf("Loss interval: %d\n", loss_interval);
    printf("\n--- RECEIVER ---\n");

    while (1)
    {
        int packet, type;

        if (!recv_int(client_sock, &packet))
            break;

        if (packet == -1)
        {
            printf("\nTransmission completed.\n");
            break;
        }

        if (!recv_int(client_sock, &type))
            break;

        printf("\nReceived packet %d (%s)\n",
               packet,
               type == 1 ? "initial" : "retransmission");

        /*
         * Designated losses:
         * loss_interval = 3 gives packet numbers
         * 3, 5, 7, 9, 11, 13, 15.
         *
         * Each designated packet is dropped only
         * on its first arrival.
         */
        int designated_loss =
            packet >= loss_interval &&
            (packet - loss_interval) % 2 == 0;

        if (designated_loss && !dropped[packet])
        {
            dropped[packet] = 1;

            printf("SIMULATED LOSS: Packet %d dropped.\n",
                   packet);

            /*
             * Send duplicate cumulative ACK instead of
             * remaining silent, so the client can process
             * the batch without becoming stuck.
             */
            if (!send_int(client_sock, expected - 1))
                break;

            printf("Duplicate ACK %d sent.\n", expected - 1);
            continue;
        }

        if (packet == expected)
        {
            printf("Packet %d accepted.\n", packet);

            expected++;

            if (!send_int(client_sock, packet))
                break;

            printf("ACK %d sent.\n", packet);
        }
        else if (packet > expected)
        {
            printf("Packet %d discarded; waiting for %d.\n",
                   packet, expected);

            if (!send_int(client_sock, expected - 1))
                break;

            printf("Duplicate ACK %d sent.\n", expected - 1);
        }
        else
        {
            printf("Duplicate packet %d received.\n", packet);

            if (!send_int(client_sock, expected - 1))
                break;

            printf("ACK %d sent.\n", expected - 1);
        }
    }

    free(dropped);
    close(client_sock);
    close(server_sock);

    return 0;
}