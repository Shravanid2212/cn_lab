
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main()
{
    int sock;
    struct sockaddr_in server_addr;

    int window_size;
    int total_packets;
    int loss_n;

    int total_transmissions = 0;
    int total_retransmissions = 0;

    int lost_packet;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket error");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("Connection error");
        close(sock);
        exit(1);
    }

    printf("Enter GBN window size: ");
    scanf("%d", &window_size);

    printf("Enter number of packets: ");
    scanf("%d", &total_packets);

    printf("Enter every nth packet to be lost: ");
    scanf("%d", &loss_n);

    if (window_size <= 0 || total_packets <= 0 || loss_n <= 0)
    {
        printf("Invalid input.\n");
        close(sock);
        return 1;
    }

    lost_packet = loss_n;

    send(sock, &window_size, sizeof(window_size), 0);
    send(sock, &total_packets, sizeof(total_packets), 0);
    send(sock, &loss_n, sizeof(loss_n), 0);

    printf("\n====================================\n");
    printf("             GBN ARQ\n");
    printf("====================================\n");

    printf("Window size       : %d\n", window_size);
    printf("Total packets     : %d\n", total_packets);
    printf("Every nth lost    : %d\n", loss_n);

    /*
       INITIAL TRANSMISSION
    */

    int last_sent = loss_n + window_size - 1;

    if (last_sent > total_packets)
        last_sent = total_packets;

    printf("\nInitial transmission:\n");

    for (int i = 1; i <= last_sent; i++)
    {
        printf("%d ", i);

        int type = 1;

        send(sock, &i, sizeof(i), 0);
        send(sock, &type, sizeof(type), 0);

        total_transmissions++;
    }

    printf("\n");

    /*
       RETRANSMISSION CYCLES
    */

    while (lost_packet <= total_packets)
    {
        if (lost_packet > last_sent)
            break;

        printf("\nPacket %d is LOST\n", lost_packet);

        printf("Retransmitting: ");

        for (int i = lost_packet; i <= last_sent; i++)
        {
            printf("%d ", i);

            int type = 2;

            send(sock, &i, sizeof(i), 0);
            send(sock, &type, sizeof(type), 0);

            total_transmissions++;
            total_retransmissions++;
        }

        printf("\n");

        /*
           Calculate the next designated loss.
           Example: 3 -> 5 -> 7 -> 9 ...
        */

        int next_loss = lost_packet + (loss_n - 1);

        /*
           Calculate the new transmission limit.
        */

        int new_last = next_loss + window_size - 1;

        if (new_last > total_packets)
            new_last = total_packets;

        /*
           Send only new packets.
        */

        if (last_sent < new_last)
        {
            printf("New packets sent: ");

            for (int i = last_sent + 1; i <= new_last; i++)
            {
                printf("%d ", i);

                int type = 1;

                send(sock, &i, sizeof(i), 0);
                send(sock, &type, sizeof(type), 0);

                total_transmissions++;
            }

            printf("\n");
        }

        last_sent = new_last;
        lost_packet = next_loss;
    }

    /*
       FINAL RETRANSMISSION
    */

    if (lost_packet <= total_packets)
    {
        printf("\nPacket %d is LOST\n", lost_packet);

        printf("Final retransmission: ");

        for (int i = lost_packet; i <= total_packets; i++)
        {
            printf("%d ", i);

            int type = 2;

            send(sock, &i, sizeof(i), 0);
            send(sock, &type, sizeof(type), 0);

            total_transmissions++;
            total_retransmissions++;
        }

        printf("\n");
    }

    int end_signal = -1;
    send(sock, &end_signal, sizeof(end_signal), 0);

    printf("\n====================================\n");
    printf("              RESULT\n");
    printf("====================================\n");

    printf("Window size                  : %d\n", window_size);
    printf("Total packets                : %d\n", total_packets);
    printf("Every nth packet lost        : %d\n", loss_n);
    printf("Total transmissions required : %d\n", total_transmissions);
    printf("Total retransmissions        : %d\n", total_retransmissions);

    printf("====================================\n");

    close(sock);

    return 0;
}

