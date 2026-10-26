#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main()
{
    int server_fd, client_fd;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    socklen_t addr_len;

    float tp, tt;
    float a;
    float max_packets;
    float end_time;

    int n;
    int power;
    int window_size;

    int total_packets;
    int total_transmissions;
    int total_acks;


    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("Socket error");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("Bind error");
        exit(1);
    }

    listen(server_fd, 5);

    printf("Waiting for client...\n");

    addr_len = sizeof(client_addr);

    client_fd = accept(server_fd,
                       (struct sockaddr *)&client_addr,
                       &addr_len);

    if (client_fd < 0)
    {
        perror("Accept error");
        exit(1);
    }

    printf("client connected.\n\n");


    recv(client_fd, &tp,
         sizeof(tp), 0);

    recv(client_fd, &tt,
         sizeof(tt), 0);

    recv(client_fd, &a,
         sizeof(a), 0);

    recv(client_fd, &max_packets,
         sizeof(max_packets), 0);

    recv(client_fd, &n,
         sizeof(n), 0);

    recv(client_fd, &power,
         sizeof(power), 0);

    recv(client_fd, &window_size,
         sizeof(window_size), 0);

    recv(client_fd, &total_packets,
         sizeof(total_packets), 0);

    recv(client_fd, &total_transmissions,
         sizeof(total_transmissions), 0);

    recv(client_fd, &total_acks,
         sizeof(total_acks), 0);

    recv(client_fd, &end_time,
         sizeof(end_time), 0);

    /* ---------------- OUTPUT ---------------- */

   /*printf("====================================\n");
    printf("       SLIDING WINDOW PROTOCOL\n");
    printf("====================================\n");*/ 

    printf("propagation time: %.2f ms\n",
           tp);

    printf("transmission time: %.2f ms\n",
           tt);

    printf("a = tp / tt: %.2f\n",
           a);

    printf("maximum packets (1 + 2a)    : %.2f\n",
           max_packets);

    printf("number of bits (n) : %d\n",
           n);

    printf("2^n                         : %d\n",
           power);

    printf("window size                 : %d\n",
           window_size);

    printf("pkts               : %d\n",
           total_packets);

    printf("total transmissions         : %d\n",
           total_transmissions);

    printf("acks received         : %d\n",
           total_acks);

    printf("Process ends at             : %.2f ms\n",
           end_time);

    close(client_fd);
    close(server_fd);

    return 0;
}