#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>

int main()
{
    int clientsocket;

    struct sockaddr_in serveraddress;

    clientsocket = socket(AF_INET, SOCK_DGRAM, 0);

    if(clientsocket < 0)
        printf("Socket creation failed\n");
    else
        printf("Socket created successfully\n");

    serveraddress.sin_family = AF_INET;
    serveraddress.sin_port = htons(9000);
    serveraddress.sin_addr.s_addr = INADDR_ANY;

    char clientmessage[256] = "Hello Server";
    char serverresponse[256];

    // Send message to server
    sendto(clientsocket,
           clientmessage,
           sizeof(clientmessage),
           0,
           (struct sockaddr *)&serveraddress,
           sizeof(serveraddress));

    socklen_t len = sizeof(serveraddress);

    // Receive reply
    recvfrom(clientsocket,
             serverresponse,
             sizeof(serverresponse),
             0,
             (struct sockaddr *)&serveraddress,
             &len);

    printf("Reply from server: %s\n", serverresponse);

    close(clientsocket);

    return 0;
}