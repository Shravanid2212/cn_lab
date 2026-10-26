#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define TIMEOUT_SEC 3

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

int send_packet(int sock, int packet, int type)
{
    return send_int(sock, packet) &&
           send_int(sock, type);
}

int main()
{
    int sock;
    struct sockaddr_in server_addr;

    int window_size, total_packets, loss_interval;
    int base = 1;

    int total_transmissions = 0;
    int total_retransmissions = 0;

    printf("Enter window size: ");
    scanf("%d", &window_size);

    printf("Enter total packets: ");
    scanf("%d", &total_packets);

    printf("Enter loss interval: ");
    scanf("%d", &loss_interval);

    if (window_size <= 0 || total_packets <= 0 ||
        loss_interval <= 0)
    {
        printf("Invalid input.\n");
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("Connection");
        close(sock);
        return 1;
    }

    if (!send_int(sock, window_size) ||
        !send_int(sock, total_packets) ||
        !send_int(sock, loss_interval))
    {
        printf("Failed to send configuration.\n");
        close(sock);
        return 1;
    }

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
               &timeout, sizeof(timeout));

    printf("\n--- GO-BACK-N ARQ ---\n");

    while (base <= total_packets)
    {
        int end = base + window_size - 1;

        if (end > total_packets)
            end = total_packets;

        printf("\nSending window: %d to %d\n", base, end);

        /*
         * Send the complete current batch.
         * Type 1 = initial transmission.
         * Type 2 = retransmission.
         */
        int retransmission = (base != 1);

        for (int p = base; p <= end; p++)
        {
            printf("%s packet %d\n",
                   retransmission ? "Retransmitting" : "Sending",
                   p);

            if (!send_packet(sock, p, retransmission ? 2 : 1))
            {
                printf("Send failed.\n");
                close(sock);
                return 1;
            }

            total_transmissions++;

            if (retransmission)
                total_retransmissions++;
        }

        /*
         * Receive one response for each packet sent.
         * The server sends duplicate cumulative ACKs
         * for packets that were dropped or arrived out of order.
         */
        int highest_ack = base - 1;
        int failed = 0;

        for (int p = base; p <= end; p++)
        {
            int ack;

            if (!recv_int(sock, &ack))
            {
                printf("Timeout or connection failure.\n");
                failed = 1;
                break;
            }

            printf("ACK received: %d\n", ack);

            if (ack > highest_ack)
                highest_ack = ack;
        }

        if (failed)
        {
            printf("Connection did not respond. "
                   "Stopping to avoid an infinite loop.\n");
            break;
        }

        /*
         * Cumulative ACKs advance the sender's base.
         * If the batch contains a loss, base stays at
         * the first unacknowledged packet.
         */
        base = highest_ack + 1;
    }

    send_int(sock, -1);

    printf("\n--- SUMMARY ---\n");
    printf("Total packets: %d\n", total_packets);
    printf("Total transmissions: %d\n", total_transmissions);
    printf("Total retransmissions: %d\n", total_retransmissions);

    close(sock);

    return 0;
}