#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>

#define PORT 8080
#define MAX_FRAMES 20
#define MAX_DATA 1024

void bitStuff(char data[], char stuffed[])
{
    int i, j = 0;
    int count = 0;

    for (i = 0; data[i] != '\0'; i++)
    {
        stuffed[j++] = data[i];

        if (data[i] == '1')
        {
            count++;

            if (count == 5)
            {
                stuffed[j++] = '0';
                count = 0;
            }
        }
        else
        {
            count = 0;
        }
    }

    stuffed[j] = '\0';
}

int main()
{
    int clientSocket;
    struct sockaddr_in serverAddress;

    char data[MAX_DATA];
    char stuffed[2 * MAX_DATA];

    int numFrames, i;
    int32_t countToSend, frameLen;

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientSocket, (struct sockaddr *)&serverAddress,
                sizeof(serverAddress)) < 0)
    {
        perror("Connection failed");
        close(clientSocket);
        return 1;
    }

    printf("Connected to server.\n");

    printf("Enter number of frames: ");
    scanf("%d", &numFrames);

    if (numFrames < 1 || numFrames > MAX_FRAMES)
    {
        printf("Invalid number of frames.\n");
        close(clientSocket);
        return 1;
    }

    countToSend = numFrames;

    if (send(clientSocket, &countToSend, sizeof(countToSend), 0) < 0)
    {
        perror("Send failed");
        close(clientSocket);
        return 1;
    }

    for (i = 0; i < numFrames; i++)
    {
        printf("\nEnter binary data for frame %d: ", i + 1);
        scanf("%1023s", data);

        int valid = 1;

        for (int j = 0; data[j] != '\0'; j++)
        {
            if (data[j] != '0' && data[j] != '1')
            {
                valid = 0;
                break;
            }
        }

        if (!valid || data[0] == '\0')
        {
            printf("Invalid input. Enter only 0s and 1s.\n");
            close(clientSocket);
            return 1;
        }

        bitStuff(data, stuffed);

        printf("Original data: %s\n", data);
        printf("Stuffed data : %s\n", stuffed);

        frameLen = strlen(stuffed);

        if (send(clientSocket, &frameLen, sizeof(frameLen), 0) < 0)
        {
            perror("Send failed");
            close(clientSocket);
            return 1;
        }

        if (send(clientSocket, stuffed, frameLen, 0) < 0)
        {
            perror("Send failed");
            close(clientSocket);
            return 1;
        }
    }

    printf("\nAll frames sent successfully.\n");

    close(clientSocket);
    return 0;
}