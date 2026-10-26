#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9999
#define END -1
#define PN_LEN 11

const char *PN_CODE = "11010010111";

int freqBands[8] = {200, 300, 400, 500, 600, 700, 800, 900};

typedef struct {
    int seqNo;
    int frequency;
    char data[PN_LEN + 1];
} Packet;

// ---- Despread: XOR received chunk with PN_CODE again to recover the bit ----
char despreadBit(const char *chunk) {
    char result[PN_LEN + 1];
    for (int i = 0; i < PN_LEN; i++) {
        int c = chunk[i] - '0';
        int n = PN_CODE[i] - '0';
        result[i] = (c ^ n) + '0';   // XOR cancels PN_CODE, leaves original bit repeated
    }
    result[PN_LEN] = '\0';
    return result[0];   // every position holds the same recovered bit
}

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    printf("Server listening on port %d...\n", PORT);
    printf("Using PN code: %s\n\n", PN_CODE);

    char chunks[100][PN_LEN + 1];
    int count = 0;

    Packet p;
    while (1) {
        recvfrom(sock, &p, sizeof(p), 0, NULL, NULL);
        if (p.seqNo == END) break;

        printf("Received | Seq: %d | Freq: %d Hz | Chunk: %s\n", p.seqNo, p.frequency, p.data);
        strcpy(chunks[p.seqNo], p.data);
        if (p.seqNo + 1 > count) count = p.seqNo + 1;
    }

    // ---- Despread each chunk back into the original bit ----
    char original[100] = "";
    for (int i = 0; i < count; i++) {
        char bit = despreadBit(chunks[i]);
        strncat(original, &bit, 1);
    }

    printf("\nRecovered data: %s\n", original);

    close(sock);
    return 0;
}