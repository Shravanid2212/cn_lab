#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>

#define PORT 8080

/* ==========================================
   Convert IP address into one 32-bit number
   ========================================== */

uint32_t ipToNumber(int a, int b, int c, int d)
{
    return ((uint32_t)a << 24) |
           ((uint32_t)b << 16) |
           ((uint32_t)c << 8) |
           (uint32_t)d;
}

/* ==========================================
   Convert 32-bit number into IP string
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

    socklen_t client_len =
        sizeof(client);

    char ip[50];
    char searchIP[50];

    char result[5000];
    char searchResult[1000];

    char ready[20];

    int subnets;
    int required[20];

    /* ==========================================
       ARRAYS TO STORE GENERATED VLSM SUBNETS
       ========================================== */

    uint32_t subnetID[20];
    uint32_t broadcastID[20];
    uint32_t subnetMask[20];

    /* ==========================================
       CREATE SOCKET
       ========================================== */

    server_fd = socket(AF_INET,
                       SOCK_STREAM,
                       0);

    if (server_fd < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    /* Allow port reuse */

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
        return 1;
    }

    /* ==========================================
       LISTEN
       ========================================== */

    listen(server_fd, 5);

    printf("===============================================\n");
    printf("             VLSM SUBNETTING SERVER\n");
    printf("===============================================\n");

    printf("Waiting for client...\n");

    /* ==========================================
       ACCEPT
       ========================================== */

    client_fd = accept(server_fd,
                       (struct sockaddr *)&client,
                       &client_len);

    if (client_fd < 0)
    {
        perror("Accept failed");
        close(server_fd);
        return 1;
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

    /* ==========================================
       RECEIVE REQUIRED IPs
       ========================================== */

    recv(client_fd,
         required,
         sizeof(required),
         0);

    printf("IP received: %s\n", ip);
    printf("Number of subnets: %d\n", subnets);

    /* ==========================================
       VALIDATE SUBNET COUNT
       ========================================== */

    if (subnets <= 0 || subnets > 20)
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
    int hostBits;

    if (a >= 1 && a <= 126)
    {
        className = 'A';
        hostBits = 24;
    }
    else if (a >= 128 && a <= 191)
    {
        className = 'B';
        hostBits = 16;
    }
    else if (a >= 192 && a <= 223)
    {
        className = 'C';
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
       SORT REQUIREMENTS
       FROM LARGEST TO SMALLEST
       ========================================== */

    for (int i = 0; i < subnets - 1; i++)
    {
        for (int j = i + 1; j < subnets; j++)
        {
            if (required[i] < required[j])
            {
                int temp = required[i];

                required[i] = required[j];
                required[j] = temp;
            }
        }
    }

    /* ==========================================
       INITIAL HOST PORTION
       ========================================== */

    uint32_t current;

    if (className == 'A')
    {
        current =
            ((uint32_t)b << 16) |
            ((uint32_t)c << 8) |
            d;
    }
    else if (className == 'B')
    {
        current =
            ((uint32_t)c << 8) |
            d;
    }
    else
    {
        current = d;
    }

    /* ==========================================
       BUILD RESULT
       ========================================== */

    sprintf(result,

            "\n===============================================\n"
            "              VLSM SUBNETTING\n"
            "===============================================\n"
            "IP Address        : %s\n"
            "Class             : %c\n"
            "Number of Subnets : %d\n"
            "===============================================\n"
            "%-8s %-15s %-23s %-23s %-18s\n"
            "===============================================\n",

            ip,
            className,
            subnets,

            "Subnet",
            "Required IPs",
            "Subnet ID",
            "Broadcast ID",
            "Subnet Mask");

    /* ==========================================
       GENERATE VLSM SUBNETS
       ========================================== */

    for (int i = 0; i < subnets; i++)
    {
        /*
            Find smallest power of 2
            that can accommodate required IPs.
        */

        uint32_t subnetSize = 1;

        while (subnetSize < (uint32_t)required[i])
        {
            subnetSize = subnetSize * 2;
        }

        /*
            Current subnet ID
        */

        uint32_t network;

        if (className == 'A')
        {
            network =
                ipToNumber(
                    a,
                    (current >> 16) & 255,
                    (current >> 8) & 255,
                    current & 255);
        }
        else if (className == 'B')
        {
            network =
                ipToNumber(
                    a,
                    b,
                    (current >> 8) & 255,
                    current & 255);
        }
        else
        {
            network =
                ipToNumber(
                    a,
                    b,
                    c,
                    current & 255);
        }

        /*
            Broadcast ID
        */

        uint32_t broadcast =
            network + subnetSize - 1;

        /* ======================================
           CALCULATE HOST BITS
           ====================================== */

        int hostBitsForSubnet = 0;

        uint32_t temp = subnetSize;

        while (temp > 1)
        {
            temp = temp / 2;
            hostBitsForSubnet++;
        }

        /*
            Prefix length
        */

        int prefix =
            32 - hostBitsForSubnet;

        /* ======================================
           CREATE SUBNET MASK
           ====================================== */

        uint32_t mask;

        if (prefix == 32)
        {
            mask = 0xFFFFFFFF;
        }
        else
        {
            mask =
                0xFFFFFFFF <<
                (32 - prefix);
        }

        /*
            Store subnet information
        */

        subnetID[i] = network;
        broadcastID[i] = broadcast;
        subnetMask[i] = mask;

        /* ======================================
           CONVERT IPs TO STRINGS
           ====================================== */

        char subnetString[50];
        char broadcastString[50];
        char maskString[50];

        numberToIP(network,
                   subnetString);

        numberToIP(broadcast,
                   broadcastString);

        numberToIP(mask,
                   maskString);

        /* ======================================
           ADD ROW TO RESULT
           ====================================== */

        char line[300];

        sprintf(line,
                "%-8d %-15d %-23s %-23s %-18s\n",
                i + 1,
                required[i],
                subnetString,
                broadcastString,
                maskString);

        strcat(result, line);

        /*
            Move to next subnet
        */

        current =
            current + subnetSize;
    }

    strcat(result,
           "===============================================\n");

    /* ==========================================
       SEND VLSM TABLE TO CLIENT
       ========================================== */

    send(client_fd,
         result,
         sizeof(result),
         0);

    printf("\nVLSM table sent to client.\n");

    /* ==========================================
       WAIT FOR CLIENT READY
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
       CONVERT SEARCH IP TO 32-BIT NUMBER
       ========================================== */

    uint32_t searchValue =
        ipToNumber(sa, sb, sc, sd);

    int found = 0;

    /* ==========================================
       SEARCH USING:

       SEARCH IP AND SUBNET MASK
       ==========================
    */

    for (int i = 0; i < subnets; i++)
    {
        /*
            THIS IS THE IMPORTANT LOGIC

            Network =
            Search IP AND Subnet Mask
        */

        uint32_t calculatedNetwork =
            searchValue & subnetMask[i];

        /*
            Compare calculated network
            with the subnet ID.
        */

        if (calculatedNetwork == subnetID[i])
        {
            char calculatedNetworkString[50];
            char subnetString[50];
            char broadcastString[50];
            char maskString[50];

            numberToIP(calculatedNetwork,
                       calculatedNetworkString);

            numberToIP(subnetID[i],
                       subnetString);

            numberToIP(broadcastID[i],
                       broadcastString);

            numberToIP(subnetMask[i],
                       maskString);

            sprintf(searchResult,

                    "\n===============================================\n"
                    "              SUBNET FOUND\n"
                    "===============================================\n"
                    "IP Address          : %s\n"
                    "Subnet Number       : %d\n"
                    "Required IPs        : %d\n"
                    "Subnet ID           : %s\n"
                    "Broadcast ID        : %s\n"
                    "Subnet Mask         : %s\n"
                    "IP AND Mask Result  : %s\n"
                    "===============================================\n",

                    searchIP,
                    i + 1,
                    required[i],
                    subnetString,
                    broadcastString,
                    maskString,
                    calculatedNetworkString);

            found = 1;

            break;
        }
    }

    /* ==========================================
       IP NOT FOUND
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