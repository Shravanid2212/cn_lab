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
    int sock;
    struct sockaddr_in server;

    int bits, window_size, packets, lost_count;
    int msg, status, seq;
    int retransmissions = 0;

    int lost[MAX_PACKETS + 1] = {0};
    int acked[MAX_PACKETS + 1] = {0};

    printf("=====================================\n");
    printf("       SELECTIVE REPEAT CLIENT\n");
    printf("=====================================\n");

    printf("Enter number of bits: ");
    if (scanf("%d", &bits) != 1 || bits < 1 || bits > 7)
    {
        printf("Invalid number of bits.\n");
        return 1;
    }

    window_size = 1 << (bits - 1);

    printf("Window size = %d\n", window_size);

    printf("Enter number of packets: ");
    if (scanf("%d", &packets) != 1 ||
        packets < 1 || packets > MAX_PACKETS)
    {
        printf("Invalid number of packets.\n");
        return 1;
    }

    printf("Enter number of lost packets: ");
    if (scanf("%d", &lost_count) != 1 ||
        lost_count < 0 || lost_count > packets)
    {
        printf("Invalid number of lost packets.\n");
        return 1;
    }

    printf("Enter lost packet numbers (1-based):\n");

    for (int i = 0; i < lost_count; i++)
    {
        int x;

        if (scanf("%d", &x) != 1 ||
            x < 1 || x > packets || lost[x])
        {
            printf("Invalid or repeated packet number.\n");
            return 1;
        }

        lost[x] = 1;
    }

    /* Create socket */

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket error");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* Connect to server */

    if (connect(sock,
                (struct sockaddr *)&server,
                sizeof(server)) < 0)
    {
        perror("Connection error");
        close(sock);
        return 1;
    }

    printf("\nConnected to server.\n");

    /* Send total number of packets */

    if (send_int(sock, packets) < 0)
    {
        printf("Failed to send packet count.\n");
        close(sock);
        return 1;
    }

    int base = 1;

    /* Process each window */

    while (base <= packets)
    {
        int end = base + window_size - 1;

        if (end > packets)
            end = packets;

        printf("\n-------------------------------------\n");
        printf("Window: %d to %d\n", base, end);
        printf("-------------------------------------\n");

        /* Initial transmission */

        for (int i = base; i <= end; i++)
        {
            if (lost[i])
            {
                printf("Packet %d is LOST.\n", i);
                continue;
            }

            if (send_int(sock, i) < 0)
            {
                printf("Failed to send packet.\n");
                close(sock);
                return 1;
            }

            printf("Packet %d sent.\n", i);
        }

        /* Mark end of initial batch */

        if (send_int(sock, END_BATCH) < 0)
        {
            close(sock);
            return 1;
        }

        /* Receive responses to initial transmission */

        while (1)
        {
            if (recv_int(sock, &status) < 0)
            {
                printf("Server disconnected.\n");
                close(sock);
                return 1;
            }

            if (status == END_BATCH)
                break;

            if (status != ACK && status != NACK)
            {
                printf("Invalid response received.\n");
                close(sock);
                return 1;
            }

            if (recv_int(sock, &seq) < 0)
            {
                printf("Failed to receive packet number.\n");
                close(sock);
                return 1;
            }

            if (seq < base || seq > end)
                continue;

            if (status == ACK)
            {
                printf("ACK received for packet %d\n", seq);
                acked[seq] = 1;
            }
            else
            {
                printf("-ve ACK received for packet %d\n", seq);
            }
        }

        /* Repeat until every packet in this window is ACKed */

        while (1)
        {
            int pending = 0;

            for (int i = base; i <= end; i++)
            {
                if (!acked[i])
                    pending++;
            }

            if (pending == 0)
                break;

            printf("\nRetransmission round\n");

            /* Retransmit only unacknowledged packets */

            for (int i = base; i <= end; i++)
            {
                if (!acked[i])
                {
                    if (send_int(sock, i) < 0)
                    {
                        close(sock);
                        return 1;
                    }

                    retransmissions++;

                    printf("Retransmitting packet %d\n", i);
                }
            }

            /* Mark end of retransmission batch */

            if (send_int(sock, END_BATCH) < 0)
            {
                close(sock);
                return 1;
            }

            /* Receive ACKs for retransmitted packets */

            while (1)
            {
                if (recv_int(sock, &status) < 0)
                {
                    printf("Server disconnected.\n");
                    close(sock);
                    return 1;
                }

                if (status == END_BATCH)
                    break;

                if (status != ACK && status != NACK)
                {
                    printf("Invalid response received.\n");
                    close(sock);
                    return 1;
                }

                if (recv_int(sock, &seq) < 0)
                {
                    close(sock);
                    return 1;
                }

                if (seq < base || seq > end)
                    continue;

                if (status == ACK)
                {
                    printf("ACK received for packet %d\n", seq);
                    acked[seq] = 1;
                }
                else
                {
                    printf("-ve ACK received for packet %d\n", seq);
                }
            }
        }

        printf("\nWindow %d to %d completed.\n", base, end);

        base = end + 1;
    }

    printf("\nAll packets acknowledged.\n");

    printf("\n=====================================\n");
    printf("       TRANSMISSION SUMMARY\n");
    printf("=====================================\n");

    printf("Number of bits       : %d\n", bits);
    printf("Window size          : %d\n", window_size);
    printf("Total packets        : %d\n", packets);
    printf("Total retransmissions: %d\n", retransmissions);

    printf("=====================================\n");

    close(sock);

    return 0;
}