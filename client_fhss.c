#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9999
#define N 8
#define END -1

int freqBands[8] = {200, 300, 400, 500, 600, 700, 800, 900};

typedef struct {
    int seqNo;
    int frequency;
    char data[4];
} Packet;


// Convert binary chunk in decimal
int chunkToFreqIndex(const char *chunk) {
    int value = 0;
    for (int i = 0; i < 3; i++)
        value = value * 2 + (chunk[i] - '0');
    return value;
}
int main() {
char bits[100], spread[300] = "";
    printf("Enter binary string");
    scanf("%s", bits);

    // Spread ,rotates 100 or 010 or 001 by place value
const char *oneCode[3] = {"100", "010", "001"};
int onesCount = 0;

for (int i = 0; bits[i]; i++) {
        if (bits[i] == '1') {
            strcat(spread, oneCode[onesCount % 3]);
            onesCount++;
        } else {
             strcat(spread, "000");
        }
    }
printf("Spread data: %s\n", spread);

    // socket udp
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    // make packets of 3 , assign frequency and send
    int numPackets = strlen(spread) / 3;
    for (int i = 0; i < numPackets; i++) {
        Packet p;
        p.seqNo = i;
        strncpy(p.data, spread + i*3, 3);
        p.data[3] = '\0';

        int freqIndex = chunkToFreqIndex(p.data);   //assign frequency 
        p.frequency = freqBands[freqIndex];

        sendto(sock, &p, sizeof(p), 0, (struct sockaddr*)&server, sizeof(server));
        printf("Packet %d | Freq: %d Hz | Data: %s\n", i+1, p.frequency, p.data);
        usleep(500000); // 500ms delay 
    }
    Packet end = {END, 0, ""};
    sendto(sock, &end, sizeof(end), 0, (struct sockaddr*)&server, sizeof(server));
    close(sock);
    return 0;
}