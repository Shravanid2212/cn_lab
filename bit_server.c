#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>

#define PORT 8080
#define MAX_FRAMES 20
#define MAX_DATA 1024

void bitDestuff(char stuffed[], char data[])
{
    int i, j = 0;
    int count = 0;

    for (i = 0; stuffed[i] != '\0'; i++)
    {
        data[j++] = stuffed[i];

        if (stuffed[i] == '1')
        {
            count++;

            if (count == 5)
            {
                i++;
                count = 0;
            }
        }
        else
        {
            count = 0;
        }
    }

    data[j] = '\0';
}

int main()
{
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientLength = sizeof(clientAddress);

    int32_t numFrames, frameLen;
    int i;

    char stuffed[2 * MAX_DATA];
    char data[2 * MAX_DATA];

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddress,
             sizeof(serverAddress)) < 0)
    {
        perror("Bind failed");
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 5) < 0)
    {
        perror("Listen failed");
        close(serverSocket);
        return 1;
    }

    printf("Server is waiting for a connection...\n");

    clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress,
                          &clientLength);

    if (clientSocket < 0)
    {
        perror("Accept failed");
        close(serverSocket);
        return 1;
    }

    printf("Client connected.\n");

    if (recv(clientSocket, &numFrames, sizeof(numFrames), 0)
        != sizeof(numFrames))
    {
        printf("Failed to receive number of frames.\n");
        close(clientSocket);
        close(serverSocket);
        return 1;
    }

    if (numFrames < 1 || numFrames > MAX_FRAMES)
    {
        printf("Invalid number of frames received.\n");
        close(clientSocket);
        close(serverSocket);
        return 1;
    }

    for (i = 0; i < numFrames; i++)
    {
        if (recv(clientSocket, &frameLen, sizeof(frameLen), 0)
            != sizeof(frameLen))
        {
            printf("Failed to receive frame length.\n");
            break;
        }

        if (frameLen < 1 || frameLen >= (int32_t)sizeof(stuffed))
        {
            printf("Invalid frame length.\n");
            break;
        }

        if (recv(clientSocket, stuffed, frameLen, 0) != frameLen)
        {
            printf("Failed to receive complete frame.\n");
            break;
        }

        stuffed[frameLen] = '\0';

        printf("\nFrame %d received.\n", i + 1);
        printf("Stuffed data   : %s\n", stuffed);

        bitDestuff(stuffed, data);

        printf("Original data  : %s\n", data);
    }

    close(clientSocket);
    close(serverSocket);

    return 0;
}