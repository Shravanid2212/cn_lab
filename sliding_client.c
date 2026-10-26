#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>

#define PORT 8080
#define max_pkt 10000

int main()
{
    int sock;
    struct sockaddr_in server_addr;

    float tp, tt;
    float a;
    float max_packets;

    int n;
    int power;
    int win_size;
    int total;

    int total_transmissions;
    int acks;

    float end_time;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("socket error");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr =
        inet_addr("127.0.0.1");

    if (connect(sock,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connection error");
        exit(1);
    }

    printf("propagation time: ");
    scanf("%f", &tp);

    printf("transmission time: ");
    scanf("%f", &tt);

    if (tt <= 0)
    {
        printf("transmission time must be greater than 0.\n");
        close(sock);
        return 0;
    }


    a = tp / tt;

    max_packets = 1 + 2 * a;

    n = (int)floor(log2(max_packets));

    power = 1 << n;

    window_size = (int)floor(
        fmin(max_packets, power)
    );
    total_packets = (int)floor(max_packets);

    total_transmissions = total_packets;

   acks = total_packets;


    float send_time[max_pkt];
    float ack_time[max_pkt];

int base = 1;
    int next_packet =1;

    float sender_free = 1.0;

    float ack_delay = 2 * tp;

    while (base <= total_packets)
    {

        while (next_packet <= total_packets &&
               next_packet < base + window_size)
        {
            send_time[next_packet] = sender_free;

            ack_time[next_packet] =
                sender_free + ack_delay;

            printf("packet %d sent at %.2f ms\n",
                   next_packet,
                   sender_free);

            sender_free += tt;

            next_packet++;
        }

        /*
           oldest unacknowledged packet gets its ACK.
        */

        if (ack_time[base] > sender_free)
        {

            sender_free = ack_time[base];
        }

        printf("ACK %d received at %.2f ms\n",
               base,
               ack_time[base]);

        base++;
    }

    end_time = ack_time[total_packets];


    send(sock, &tp, sizeof(tp), 0);
    send(sock, &tt, sizeof(tt), 0);
    send(sock, &a, sizeof(a), 0);
    send(sock, &max_packets,
         sizeof(max_packets), 0);

    send(sock, &n, sizeof(n), 0);
    send(sock, &power, sizeof(power), 0);
    send(sock, &window_size,
         sizeof(window_size), 0);

    send(sock, &total_packets,
         sizeof(total_packets), 0);

    send(sock, &total_transmissions,
         sizeof(total_transmissions), 0);

    send(sock, &total_acks,
         sizeof(total_acks), 0);

    send(sock, &end_time,
         sizeof(end_time), 0);

    /* ---------------- OUTPUT ---------------- */
/* printf("\n====================================\n");
    printf("       SLIDING WINDOW PROTOCOL\n");
    printf("====================================\n");*/
   

    printf("propagation time : %.2f ms\n",
           tp);

    printf("transmission time: %.2f ms\n",
           tt);

    printf("a = tp / tt : %.2f\n",
           a);

    printf("maximum packets (1 + 2a): %.2f\n",
           max_packets);

    printf("number of bits (n) : %d\n",
           n);

    printf("2^n                         : %d\n",
           power);

    printf("window size                 : %d\n",
           window_size);

    printf("total packets               : %d\n",
           total_packets);

    printf("total transmissions         : %d\n",
           total_transmissions);

    printf("Total ACKs received         : %d\n",
           acks);

    printf("Process ends at             : %.2f ms\n",
           end_time);


    close(sock);

    return 0;
}