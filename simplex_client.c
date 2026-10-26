#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main()
{
    int sock;
    struct sockaddr_in server_addr;

    float tp, tt;
    int packets;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket error");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("Connection error");
        exit(1);
    }

    printf("enter propogation time: ");
    scanf("%f", &tp);

    printf("enter transmission time: ");
    scanf("%f", &tt);

    printf("enter number of packets:");
    scanf("%d", &packets);

    send(sock, &packets, sizeof(packets), 0);

    float rtt = 2*(tp + tt);

    /*start at t=1*/
    float current_time = 1;

   /*printf("\n STOP AND WAIT SIMPLEX \n");*/ 

    printf("rtt= 2*(tp + tt)\n");
    printf("rtt= 2*(%.2f + %.2f) = %.2f \n\n",
           tp, tt, rtt);

    for (int i = 1; i <= packets; i++)
    {
        int packet = i;
        int ack;

        float start = current_time;

        printf("packet %d\n", i);

        printf("  sending packet %d at %.2f ms\n",
               i, start);

        send(sock, &packet, sizeof(packet), 0);

        recv(sock, &ack, sizeof(ack), 0);


        current_time += rtt;

        printf(" ack %d received at %.2f \n",
               ack, current_time);

        printf("  time taken = %.2f \n\n", rtt);
    }

   /*printf("===========================================\n");*/ 
    printf("start:1 ms\n");
    printf("final time: %.2f\n",
           current_time);
    printf("total time : %.2f\n",
           current_time - 1);
    /*printf("===========================================\n");*/

    close(sock);

    return 0;
}