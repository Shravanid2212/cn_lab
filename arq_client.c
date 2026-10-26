#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define max_pkt 100

int main()
{
    int sock;
    struct sockaddr_in server_addr;

    float tp, tt;
    int packets;
    int lost_count;
    int lost[max_pkt];

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket error");
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

    printf("propogation time: ");
    scanf("%f", &tp);

    
    printf("transmission time: ");
    scanf("%f", &tt);

    printf("number of packets:");
    scanf("%d", &packets);

    printf("number of lost packets: ");
    scanf("%d", &lost_count);

    printf("lost packet numbers:\n");

    for (int i = 0; i < lost_count; i++)
    {
        scanf("%d", &lost[i]);
    }

    /* send to server*/

    send(sock,
         &packets,
         sizeof(packets),
         0);

    send(sock,
         &lost_count,
         sizeof(lost_count),
         0);

    send(sock,
         lost,
         sizeof(int) * lost_count,
         0);

    /*calculate RTT.*/

    float rtt = 2 * (tp + tt);

    float current_time = 1;

    int retransmissions = 0;

   /*printf("\n============================================\n");
    printf("       STOP AND WAIT ARQ - NOISY CHANNEL\n");
    printf("============================================\n");*/ 

    printf("tp = %.2f \n", tp);
    printf("tt= %.2f \n", tt);
    printf("rtt = 2 * (tp + tt)\n");
    printf("rtt = %.2f\n", rtt);

    for (int i = 1; i <= packets; i++)
    {
        int is_lost = 0;
        for (int j = 0; j < lost_count; j++)
        {
            if (lost[j] == i)
            {
                is_lost = 1;
                break;
            }
        }

        int packet = i;
        int ack;

        float start = current_time;

        printf("\npacket %d\n", i);

        printf("  sending acket %d at %.2f ms\n",
               i, current_time);

        send(sock,
             &packet,
             sizeof(packet),
             0);

        if (!is_lost)
        {
       
            recv(sock,
                 &ack,
                 sizeof(ack),
                 0);

            current_time += rtt;

            printf(" ack %d received at %.2f\n",
                   ack, current_time);

            printf("  packet time = %.2f ms\n",
                   rtt);
        }
        else
        {
            /*
               LOST PACKET
               
               Timer starts from the instant
               the lost packet is sent.

               Wait 3 RTT.
            */

            float timeout = 3 * rtt;

            current_time += timeout;

            printf("packet %d lost\n", i);

            printf("waiting 3 RTT = %.2f ms\n",
                   timeout);

            printf("timeout at %.2f ms\n",
                   current_time);

            printf("  retransmitting packet %d at %.2f ms\n",
                   i, current_time);

            send(sock,
                 &packet,
                 sizeof(packet),
                 0);

            recv(sock,
                 &ack,
                 sizeof(ack),
                 0);

            current_time += rtt;

            retransmissions++;

            printf("ack %d received at %.2f\n",
                   ack, current_time);

        }

    }

    /*printf("\n============================================\n");
    printf("                 RESULT\n");
    printf("============================================\n");*/

    printf("number of packets: %d\n", packets);
    printf("lost: %d\n", lost_count);
    printf("retransmissions : %d\n", retransmissions);
    printf("rtt  : %.2f ms\n", rtt);
    printf("start time : 1 ms\n");
    printf("final time : %.2f ms\n",
           current_time);
    printf("total time : %.2f ms\n",
           current_time - 1);


    close(sock);

    return 0;
}