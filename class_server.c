#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in server, client;
    socklen_t addrlen = sizeof(client);

    char ip[50];
    char result[2500];

    /* Create socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    int opt = 1;

    setsockopt(server_fd,
               SOL_SOCKET,
               SO_REUSEADDR,
               &opt,
               sizeof(opt));

    /* Server details */
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    /* Bind */
    if (bind(server_fd,
             (struct sockaddr *)&server,
             sizeof(server)) < 0)
    {
        perror("Bind failed");
        close(server_fd);
        exit(1);
    }

    /* Listen */
    listen(server_fd, 5);

    printf("=====================================\n");
    printf("       IP CLASSIFICATION SERVER\n");
    printf("=====================================\n");

    printf("Waiting for client...\n");

    /* Accept */
    client_fd = accept(server_fd,
                       (struct sockaddr *)&client,
                       &addrlen);

    if (client_fd < 0)
    {
        perror("Accept failed");
        close(server_fd);
        exit(1);
    }

    printf("Client connected.\n");

    /* Receive IP */
    memset(ip, 0, sizeof(ip));

    recv(client_fd,
         ip,
         sizeof(ip),
         0);

    printf("IP received: %s\n", ip);

    /* Extract octets */

    int a, b, c, d;

    if (sscanf(ip, "%d.%d.%d.%d",
               &a, &b, &c, &d) != 4)
    {
        strcpy(result, "Invalid IP address.");

        send(client_fd,
             result,
             sizeof(result),
             0);

        close(client_fd);
        close(server_fd);

        return 0;
    }

    /* Validate octets */

    if (a < 0 || a > 255 ||
        b < 0 || b > 255 ||
        c < 0 || c > 255 ||
        d < 0 || d > 255)
    {
        strcpy(result, "Invalid IP address.");

        send(client_fd,
             result,
             sizeof(result),
             0);

        close(client_fd);
        close(server_fd);

        return 0;
    }

    /* =========================================
       CLASS A
       ========================================= */

    if (a >= 1 && a <= 126)
    {
        long long networks = 1LL << 7;
        long long hosts = (1LL << 24) - 2;

        sprintf(result,

                "\n=====================================\n"
                "          IP INFORMATION\n"
                "=====================================\n"
                "IP Address           : %s\n"
                "Class                : A\n"
                "Network ID Bits      : 8\n"
                "Host ID Bits         : 24\n"
                "Number of Networks   : %lld\n"
                "Hosts per Network    : %lld\n"
                "IP Range             : 1.0.0.0 - 126.255.255.255\n"
                "Network ID           : %d.0.0.0\n"
                "Directed Broadcast   : %d.255.255.255\n"
                "Limited Broadcast    : 255.255.255.255\n"
                "=====================================\n",

                ip,
                networks,
                hosts,
                a,
                a);
    }

    /* =========================================
       CLASS B
       ========================================= */

    else if (a >= 128 && a <= 191)
    {
        long long networks = 1LL << 16;
        long long hosts = (1LL << 16) - 2;

        sprintf(result,

                "\n=====================================\n"
                "          IP INFORMATION\n"
                "=====================================\n"
                "IP Address           : %s\n"
                "Class                : B\n"
                "Network ID Bits      : 16\n"
                "Host ID Bits         : 16\n"
                "Number of Networks   : %lld\n"
                "Hosts per Network    : %lld\n"
                "IP Range             : 128.0.0.0 - 191.255.255.255\n"
                "Network ID           : %d.%d.0.0\n"
                "Directed Broadcast   : %d.%d.255.255\n"
                "Limited Broadcast    : 255.255.255.255\n"
                "=====================================\n",

                ip,
                networks,
                hosts,
                a, b,
                a, b);
    }

    /* =========================================
       CLASS C
       ========================================= */

    else if (a >= 192 && a <= 223)
    {
        long long networks = 1LL << 24;
        long long hosts = (1LL << 8) - 2;

        sprintf(result,

                "\n=====================================\n"
                "          IP INFORMATION\n"
                "=====================================\n"
                "IP Address           : %s\n"
                "Class                : C\n"
                "Network ID Bits      : 24\n"
                "Host ID Bits         : 8\n"
                "Number of Networks   : %lld\n"
                "Hosts per Network    : %lld\n"
                "IP Range             : 192.0.0.0 - 223.255.255.255\n"
                "Network ID           : %d.%d.%d.0\n"
                "Directed Broadcast   : %d.%d.%d.255\n"
                "Limited Broadcast    : 255.255.255.255\n"
                "=====================================\n",

                ip,
                networks,
                hosts,
                a, b, c,
                a, b, c);
    }

    /* =========================================
       CLASS D
       ========================================= */

    else if (a >= 224 && a <= 239)
    {
        sprintf(result,

                "\n=====================================\n"
                "          IP INFORMATION\n"
                "=====================================\n"
                "IP Address           : %s\n"
                "Class                : D\n"
                "Purpose              : Multicast\n"
                "IP Range             : 224.0.0.0 - 239.255.255.255\n"
                "Network ID Bits      : Not applicable\n"
                "Host ID Bits         : Not applicable\n"
                "Network ID           : Not applicable\n"
                "Directed Broadcast   : Not applicable\n"
                "Limited Broadcast    : 255.255.255.255\n"
                "=====================================\n",

                ip);
    }

    /* =========================================
       CLASS E
       ========================================= */

    else
    {
        sprintf(result,

                "\n=====================================\n"
                "          IP INFORMATION\n"
                "=====================================\n"
                "IP Address           : %s\n"
                "Class                : E\n"
                "Purpose              : Experimental\n"
                "IP Range             : 240.0.0.0 - 255.255.255.255\n"
                "Network ID Bits      : Not applicable\n"
                "Host ID Bits         : Not applicable\n"
                "Network ID           : Not applicable\n"
                "Directed Broadcast   : Not applicable\n"
                "Limited Broadcast    : 255.255.255.255\n"
                "=====================================\n",

                ip);
    }

    /* Send result */
    send(client_fd,
         result,
         sizeof(result),
         0);

    printf("\nResult sent to client.\n");

    close(client_fd);
    close(server_fd);

    return 0;
}