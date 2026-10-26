#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8082
#define BYTE_SIZE 8
#define MAX_FRAMES 20
#define MAX_DATA 1024

#define FLAG "01111110"
#define ESC  "11111110"

void stuff(char data[], char stuffed[]) {
    int i, j = 0;
    int len = strlen(data);

    for (i = 0; i < len; i += BYTE_SIZE) {
        char byte[BYTE_SIZE + 1];

        strncpy(byte, data + i, BYTE_SIZE);
        byte[BYTE_SIZE] = '\0';

        if (strcmp(byte, FLAG) == 0 || strcmp(byte, ESC) == 0) {
            strcpy(stuffed + j, ESC);
            j += BYTE_SIZE;
        }

        strcpy(stuffed + j, byte);
        j += BYTE_SIZE;
    }

    stuffed[j] = '\0';
}

int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;

    char data[MAX_DATA];
    char stuffed[2 * MAX_DATA + 1];
    char frame[2 * MAX_DATA + 3];

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientSocket, (struct sockaddr *)&serverAddr,
                sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        close(clientSocket);
        return 1;
    }

    int numFrames;

    printf("Enter number of frames: ");
    if (scanf("%d", &numFrames) != 1 ||
        numFrames < 1 || numFrames > MAX_FRAMES) {
        printf("Invalid number of frames.\n");
        close(clientSocket);
        return 1;
    }

    /* Send the number of frames first. */
    if (send(clientSocket, &numFrames, sizeof(numFrames), 0)
        != sizeof(numFrames)) {
        perror("Failed to send frame count");
        close(clientSocket);
        return 1;
    }

    for (int f = 0; f < numFrames; f++) {
        printf("\nEnter binary data for Frame %d: ", f + 1);

        if (scanf("%1023s", data) != 1) {
            printf("Input error.\n");
            close(clientSocket);
            return 1;
        }

        int len = strlen(data);

        if (len == 0 || len % BYTE_SIZE != 0) {
            printf("Each frame must contain complete 8-bit groups.\n");
            close(clientSocket);
            return 1;
        }

        int valid = 1;

        for (int i = 0; i < len; i++) {
            if (data[i] != '0' && data[i] != '1') {
                valid = 0;
                break;
            }
        }

        if (!valid) {
            printf("Enter only 0s and 1s.\n");
            close(clientSocket);
            return 1;
        }

        stuff(data, stuffed);

        /* Add the starting and ending FLAG. */
        strcpy(frame, FLAG);
        strcat(frame, stuffed);
        strcat(frame, FLAG);

        int frameLen = strlen(frame);

        printf("\n--- Sender: Frame %d ---\n", f + 1);
        printf("Original data : %s\n", data);
        printf("Stuffed data  : %s\n", stuffed);
        printf("Transmitted   : %s\n", frame);

        /*
         * Send the frame length followed by the frame.
         * The receiver uses the length to identify the
         * exact frame boundary over TCP.
         */
        if (send(clientSocket, &frameLen, sizeof(frameLen), 0)
            != sizeof(frameLen)) {
            perror("Failed to send frame length");
            close(clientSocket);
            return 1;
        }

        int totalSent = 0;

        while (totalSent < frameLen) {
            int n = send(clientSocket, frame + totalSent,
                         frameLen - totalSent, 0);

            if (n <= 0) {
                perror("Failed to send frame");
                close(clientSocket);
                return 1;
            }

            totalSent += n;
        }
    }

    close(clientSocket);
    return 0;
}
