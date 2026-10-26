#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>

#define PORT 8080

/* ==========================================
   Convert IP into 32-bit number
   ========================================== */

uint32_t ipToNumber(int a, int b, int c, int d)
{
    return ((uint32_t)a << 24) |
           ((uint32_t)b << 16) |
           ((uint32_t)c << 8) |
           (uint32_t)d;
}

/* ==========================================
   Convert 32-bit number into IP
   ========================================== */

void numberToIP(uint32_t ip, char *str)
{
    int a, b, c, d;

    a = (ip >> 24) & 255;
    b = (ip >> 16) & 255;
    c = (ip >> 8) & 255;
    d = ip & 255;

    sprintf(str,
            "%d.%d.%d.%d",
            a, b, c, d);
}

int main()
{
    int server_fd, client_fd;

    struct sockaddr_in server;
    struct sockaddr_in client;

    socklen_t addrlen =
        sizeof(client);

    char ip[50];
    char searchIP[50];

    char result[5000];
    char searchResult[1000];

    char ready[20];

    int subnets;

    /* ==========================================
       CREATE SOCKET
       ========================================== */

    server_fd = socket(AF_INET,
                       SOCK_STREAM,
                       0);

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

    /* ==========================================
       SERVER DETAILS
       ========================================== */

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    /* ==========================================
       BIND
       ========================================== */

    if (bind(server_fd,
             (struct sockaddr *)&server,
             sizeof(server)) < 0)
    {
        perror("Bind failed");
        close(server_fd);
        exit(1);
    }

    /* ==========================================
       LISTEN
       ========================================== */

    listen(server_fd, 5);

    printf("=====================================\n");
    printf("         SUBNETTING SERVER\n");
    printf("=====================================\n");

    printf("Waiting for client...\n");

    /* ==========================================
       ACCEPT
       ========================================== */

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

    /* ==========================================
       RECEIVE IP
       ========================================== */

    memset(ip, 0, sizeof(ip));

    recv(client_fd,
         ip,
         sizeof(ip),
         0);

    /* ==========================================
       RECEIVE NUMBER OF SUBNETS
       ========================================== */

    recv(client_fd,
         &subnets,
         sizeof(subnets),
         0);

    printf("IP received: %s\n", ip);
    printf("Number of subnets: %d\n", subnets);

    /* ==========================================
       VALIDATE SUBNET COUNT
       ========================================== */

    if (subnets <= 0)
    {
        strcpy(result,
               "Invalid number of subnets.");

        send(client_fd,
             result,
             sizeof(result),
             0);

        close(client_fd);
        close(server_fd);

        return 0;
    }

    /* ==========================================
       EXTRACT IP OCTETS
       ========================================== */

    int a, b, c, d;

    if (sscanf(ip,
               "%d.%d.%d.%d",
               &a, &b, &c, &d) != 4)
    {
        strcpy(result,
               "Invalid IP address.");

        send(client_fd,
             result,
             sizeof(result),
             0);

        close(client_fd);
        close(server_fd);

        return 0;
    }

    /* ==========================================
       VALIDATE IP
       ========================================== */

    if (a < 0 || a > 255 ||
        b < 0 || b > 255 ||
        c < 0 || c > 255 ||
        d < 0 || d > 255)
    {
        strcpy(result,
               "Invalid IP address.");

        send(client_fd,
             result,
             sizeof(result),
             0);

        close(client_fd);
        close(server_fd);

        return 0;
    }

    /* ==========================================
       DETERMINE CLASS
       ========================================== */

    char className;
    int networkBits;
    int hostBits;

    if (a >= 1 && a <= 126)
    {
        className = 'A';
        networkBits = 8;
        hostBits = 24;
    }
    else if (a >= 128 && a <= 191)
    {
        className = 'B';
        networkBits = 16;
        hostBits = 16;
    }
    else if (a >= 192 && a <= 223)
    {
        className = 'C';
        networkBits = 24;
        hostBits = 8;
    }
    else
    {
        strcpy(result,
               "Only Class A, B and C addresses are supported.");

        send(client_fd,
             result,
             sizeof(result),
             0);

        close(client_fd);
        close(server_fd);

        return 0;
    }

    /* ==========================================
       FIND BORROWED BITS
       ========================================== */

    int borrowedBits = 0;
    int totalSubnets = 1;

    while (totalSubnets < subnets)
    {
        totalSubnets =
            totalSubnets * 2;

        borrowedBits++;
    }

    /* ==========================================
       CHECK HOST BITS
       ========================================== */

    if (borrowedBits >= hostBits)
    {
        strcpy(result,
               "Number of subnets is too large.");

        send(client_fd,
             result,
             sizeof(result),
             0);

        close(client_fd);
        close(server_fd);

        return 0;
    }

    /* ==========================================
       NEW PREFIX
       ========================================== */

    int newPrefix =
        networkBits + borrowedBits;

    /* ==========================================
       REMAINING HOST BITS
       ========================================== */

    int remainingHostBits =
        hostBits - borrowedBits;

    /* ==========================================
       SUBNET SIZE
       ========================================== */

    uint32_t subnetSize = 1;

    for (int i = 0;
         i < remainingHostBits;
         i++)
    {
        subnetSize =
            subnetSize * 2;
    }

    /* ==========================================
       SUBNET MASK
       ========================================== */

    uint32_t mask;

    if (newPrefix == 32)
    {
        mask = 0xFFFFFFFF;
    }
    else
    {
        mask =
            0xFFFFFFFF <<
            (32 - newPrefix);
    }

    /* ==========================================
       BUILD RESULT HEADER
       ========================================== */

    char maskString[50];

    numberToIP(mask,
               maskString);

    sprintf(result,

            "\n===============================================\n"
            "              SUBNETTING RESULT\n"
            "===============================================\n"
            "IP Address          : %s\n"
            "Class               : %c\n"
            "Number of Subnets   : %d\n"
            "Borrowed Bits       : %d\n"
            "New Prefix          : /%d\n"
            "Subnet Mask         : %s\n"
            "Subnet Size         : %u addresses\n"
            "===============================================\n"
            "%-10s %-22s %-22s\n"
            "===============================================\n",

            ip,
            className,
            totalSubnets,
            borrowedBits,
            newPrefix,
            maskString,
            subnetSize,

            "Subnet",
            "Subnet ID",
            "Broadcast ID");

    /* ==========================================
       GENERATE SUBNETS
       ========================================== */

    for (int i = 0;
         i < totalSubnets;
         i++)
    {
        /*
            Calculate subnet ID.

            Start with original network IP
            and move by subnet size.
        */

        uint32_t network;

        if (className == 'A')
        {
            uint32_t hostPart =
                (uint32_t)i * subnetSize;

            network =
                ipToNumber(
                    a,
                    (hostPart >> 16) & 255,
                    (hostPart >> 8) & 255,
                    hostPart & 255);
        }

        else if (className == 'B')
        {
            uint32_t hostPart =
                (uint32_t)i * subnetSize;

            network =
                ipToNumber(
                    a,
                    b,
                    (hostPart >> 8) & 255,
                    hostPart & 255);
        }

        else
        {
            uint32_t hostPart =
                (uint32_t)i * subnetSize;

            network =
                ipToNumber(
                    a,
                    b,
                    c,
                    hostPart & 255);
        }

        /*
            Broadcast ID
        */

        uint32_t broadcast =
            network + subnetSize - 1;

        /* ======================================
           CONVERT TO STRINGS
           ====================================== */

        char subnetString[50];
        char broadcastString[50];

        numberToIP(network,
                   subnetString);

        numberToIP(broadcast,
                   broadcastString);

        /* ======================================
           ADD TO RESULT
           ====================================== */

        char line[200];

        sprintf(line,
                "%-10d %-22s %-22s\n",
                i + 1,
                subnetString,
                broadcastString);

        strcat(result, line);
    }

    strcat(result,
           "===============================================\n");

    /* ==========================================
       SEND RESULT
       ========================================== */

    send(client_fd,
         result,
         sizeof(result),
         0);

    printf("\nSubnet table sent to client.\n");

    /* ==========================================
       WAIT FOR CLIENT
       ========================================== */

    memset(ready, 0, sizeof(ready));

    recv(client_fd,
         ready,
         sizeof(ready),
         0);

    /* ==========================================
       RECEIVE SEARCH IP
       ========================================== */

    memset(searchIP, 0, sizeof(searchIP));

    recv(client_fd,
         searchIP,
         sizeof(searchIP),
         0);

    printf("\nSearch IP received: %s\n",
           searchIP);

    /* ==========================================
       PARSE SEARCH IP
       ========================================== */

    int sa, sb, sc, sd;

    if (sscanf(searchIP,
               "%d.%d.%d.%d",
               &sa, &sb, &sc, &sd) != 4)
    {
        strcpy(searchResult,
               "Invalid search IP address.");

        send(client_fd,
             searchResult,
             sizeof(searchResult),
             0);

        close(client_fd);
        close(server_fd);

        return 0;
    }

    if (sa < 0 || sa > 255 ||
        sb < 0 || sb > 255 ||
        sc < 0 || sc > 255 ||
        sd < 0 || sd > 255)
    {
        strcpy(searchResult,
               "Invalid search IP address.");

        send(client_fd,
             searchResult,
             sizeof(searchResult),
             0);

        close(client_fd);
        close(server_fd);

        return 0;
    }

    /* ==========================================
       CONVERT SEARCH IP TO NUMBER
       ========================================== */

    uint32_t searchValue =
        ipToNumber(sa, sb, sc, sd);

    int found = 0;

    /* ==========================================
       FIND SUBNET USING:

       SEARCH IP & SUBNET MASK
       ========================================== */

    for (int i = 0;
         i < totalSubnets;
         i++)
    {
        /*
            Recreate subnet ID
            for this subnet.
        */

        uint32_t network;

        if (className == 'A')
        {
            uint32_t hostPart =
                (uint32_t)i * subnetSize;

            network =
                ipToNumber(
                    a,
                    (hostPart >> 16) & 255,
                    (hostPart >> 8) & 255,
                    hostPart & 255);
        }

        else if (className == 'B')
        {
            uint32_t hostPart =
                (uint32_t)i * subnetSize;

            network =
                ipToNumber(
                    a,
                    b,
                    (hostPart >> 8) & 255,
                    hostPart & 255);
        }

        else
        {
            uint32_t hostPart =
                (uint32_t)i * subnetSize;

            network =
                ipToNumber(
                    a,
                    b,
                    c,
                    hostPart & 255);
        }

        /*
            =====================================
            IMPORTANT LOGIC

            IP AND MASK = NETWORK ADDRESS
            =====================================
        */

        uint32_t calculatedNetwork =
            searchValue & mask;

        /*
            Compare calculated network
            with current subnet ID.
        */

        if (calculatedNetwork == network)
        {
            char calculatedString[50];
            char subnetString[50];
            char broadcastString[50];

            numberToIP(calculatedNetwork,
                       calculatedString);

            numberToIP(network,
                       subnetString);

            numberToIP(network + subnetSize - 1,
                       broadcastString);

            sprintf(searchResult,

                    "\n===============================================\n"
                    "              SUBNET FOUND\n"
                    "===============================================\n"
                    "IP Address          : %s\n"
                    "Subnet Number       : %d\n"
                    "Subnet ID           : %s\n"
                    "Broadcast ID        : %s\n"
                    "Subnet Mask         : %s\n"
                    "IP AND Mask Result  : %s\n"
                    "===============================================\n",

                    searchIP,
                    i + 1,
                    subnetString,
                    broadcastString,
                    maskString,
                    calculatedString);

            found = 1;

            break;
        }
    }

    /* ==========================================
       IF NOT FOUND
       ========================================== */

    if (!found)
    {
        sprintf(searchResult,

                "\n===============================================\n"
                "             SUBNET NOT FOUND\n"
                "===============================================\n"
                "IP Address : %s\n"
                "The IP does not belong to any generated subnet.\n"
                "===============================================\n",

                searchIP);
    }

    /* ==========================================
       SEND SEARCH RESULT
       ========================================== */

    send(client_fd,
         searchResult,
         sizeof(searchResult),
         0);

    printf("Search result sent to client.\n");

    close(client_fd);
    close(server_fd);

    return 0;
}