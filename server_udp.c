#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>

int main()
{
    int serversocket, bindstatus;
    char servermessage[256] = "Hello Client and we are connected now";

    struct sockaddr_in serveraddress, clientaddress;
    socklen_t clientlen = sizeof(clientaddress);

    // Create socket
    serversocket = socket(AF_INET, SOCK_DGRAM, 0);

    if(serversocket < 0)
        printf("Socket creation failed\n");
    else
        printf("Socket created successfully\n");

    serveraddress.sin_family = AF_INET;
    serveraddress.sin_port = htons(9000);
    serveraddress.sin_addr.s_addr = INADDR_ANY;

    // Bind
    bindstatus = bind(serversocket,
                     (struct sockaddr *)&serveraddress,
                     sizeof(serveraddress));

    if(bindstatus < 0)
        printf("Binding failed\n");
    else
        printf("Binding successful\n");

    printf("Waiting for client...\n");

    char buffer[256];

    // Receive message from client
    recvfrom(serversocket,
             buffer,
             sizeof(buffer),
             0,
             (struct sockaddr *)&clientaddress,
             &clientlen);

    printf("Message from client: %s\n", buffer);

    // Send reply
    sendto(serversocket,
           servermessage,
           sizeof(servermessage),
           0,
           (struct sockaddr *)&clientaddress,
           clientlen);

    close(serversocket);

    return 0;
}