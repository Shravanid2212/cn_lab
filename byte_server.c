#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8082
#define BYTE_SIZE 8
#define MAX_FRAMES 20
#define MAX_FRAME_SIZE 2050

#define FLAG "01111110"
#define ESC  "11111110"

void destuff(char stuffed[], char data[]) {
    int i = 0, j = 0;
    int len = strlen(stuffed);

    while (i + BYTE_SIZE <= len) {
        char byte[BYTE_SIZE + 1];

        strncpy(byte, stuffed + i, BYTE_SIZE);
        byte[BYTE_SIZE] = '\0';

        if (strcmp(byte, ESC) == 0) {
            i += BYTE_SIZE;

            if (i + BYTE_SIZE > len)
                break;

            strncpy(byte, stuffed + i, BYTE_SIZE);
            byte[BYTE_SIZE] = '\0';
        }

        strcpy(data + j, byte);
        j += BYTE_SIZE;
        i += BYTE_SIZE;
    }

    data[j] = '\0';
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrSize = sizeof(clientAddr);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr,
             sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 5) < 0) {
        perror("Listen failed");
        close(serverSocket);
        return 1;
    }

    printf("Byte Stuffing Server waiting...\n");

    clientSocket = accept(serverSocket,
                          (struct sockaddr *)&clientAddr,
                          &addrSize);

    if (clientSocket < 0) {
        perror("Accept failed");
        close(serverSocket);
        return 1;
    }

    printf("Client connected.\n");

    int numFrames;

    int n = recv(clientSocket, &numFrames, sizeof(numFrames), 0);

    if (n != sizeof(numFrames) ||
        numFrames < 1 || numFrames > MAX_FRAMES) {
        printf("Invalid frame count or incomplete receive.\n");
        close(clientSocket);
        close(serverSocket);
        return 1;
    }

    printf("Number of frames: %d\n", numFrames);

    for (int f = 0; f < numFrames; f++) {
        int frameLen;

        n = recv(clientSocket, &frameLen, sizeof(frameLen), 0);

        if (n != sizeof(frameLen) ||
            frameLen < 2 * BYTE_SIZE ||
            frameLen >= MAX_FRAME_SIZE) {
            printf("Invalid frame length or incomplete receive.\n");
            break;
        }

        char frame[MAX_FRAME_SIZE];
        char payload[MAX_FRAME_SIZE];
        char data[MAX_FRAME_SIZE];

        n = recv(clientSocket, frame, frameLen, 0);

        if (n != frameLen) {
            printf("Incomplete frame received.\n");
            break;
        }

        frame[frameLen] = '\0';

        printf("\n--- Receiver: Frame %d ---\n", f + 1);
        printf("Received frame: %s\n", frame);

        if (strncmp(frame, FLAG, BYTE_SIZE) != 0 ||
            strncmp(frame + frameLen - BYTE_SIZE,
                    FLAG, BYTE_SIZE) != 0) {
            printf("Invalid frame: FLAG missing.\n");
            continue;
        }

        int payloadLen = frameLen - 2 * BYTE_SIZE;

        strncpy(payload, frame + BYTE_SIZE, payloadLen);
        payload[payloadLen] = '\0';

        printf("Stuffed payload: %s\n", payload);

        destuff(payload, data);

        printf("Destuffed data : %s\n", data);
    }

    close(clientSocket);
    close(serverSocket);

    return 0;
}
