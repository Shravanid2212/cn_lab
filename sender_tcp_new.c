#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 9020
#define CHUNK_SIZE 1024
#define MAX_FILENAME 255

static int send_all(int sock, const void *buffer, size_t len)
{
    size_t done = 0;
    const char *p = buffer;

    while (done < len) {
        ssize_t n = send(sock, p + done, len - done, 0);
        if (n <= 0) return -1;
        done += (size_t)n;
    }
    return 0;
}

static int recv_line(int sock, char *buffer, size_t capacity)
{
    size_t i = 0;
    char ch;

    while (i + 1 < capacity) {
        ssize_t n = recv(sock, &ch, 1, 0);
        if (n <= 0) return -1;
        if (ch == '\n') {
            buffer[i] = '\0';
            return 0;
        }
        buffer[i++] = ch;
    }
    return -1;
}

int main(void)
{
    char filepath[1024];
    char header[64];
    char buffer[CHUNK_SIZE];

    printf("Enter full file path: ");
    fflush(stdout);

    if (!fgets(filepath, sizeof(filepath), stdin)) {
        fprintf(stderr, "Could not read file path\n");
        return 1;
    }
    filepath[strcspn(filepath, "\r\n")] = '\0';

    FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("fopen input file");
        return 1;
    }

    if (fseeko(file, 0, SEEK_END) != 0) {
        perror("fseeko");
        fclose(file);
        return 1;
    }
    off_t size = ftello(file);
    if (size < 0 || fseeko(file, 0, SEEK_SET) != 0) {
        perror("file size / rewind");
        fclose(file);
        return 1;
    }

    char *filename = strrchr(filepath, '/');
    filename = filename ? filename + 1 : filepath;

    size_t filename_len = strlen(filename);
    if (filename_len == 0 || filename_len > MAX_FILENAME) {
        fprintf(stderr, "Invalid filename length\n");
        fclose(file);
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        fclose(file);
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid receiver IP address\n");
        close(sock);
        fclose(file);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        fclose(file);
        return 1;
    }

    /* Send filename length + newline, filename bytes, file size + newline. */
    int header_len = snprintf(header, sizeof(header), "%zu\n", filename_len);
    if (header_len < 0 || (size_t)header_len >= sizeof(header) ||
        send_all(sock, header, (size_t)header_len) < 0 ||
        send_all(sock, filename, filename_len) < 0) {
        fprintf(stderr, "Could not send filename\n");
        close(sock);
        fclose(file);
        return 1;
    }

    header_len = snprintf(header, sizeof(header), "%llu\n",
                          (unsigned long long)size);
    if (header_len < 0 || (size_t)header_len >= sizeof(header) ||
        send_all(sock, header, (size_t)header_len) < 0) {
        fprintf(stderr, "Could not send file size\n");
        close(sock);
        fclose(file);
        return 1;
    }

    printf("Sending: %s (%llu bytes)\n",
           filename, (unsigned long long)size);

    unsigned long long total_sent = 0;
    int failed = 0;

    while (1) {
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);

        if (bytes_read > 0) {
            if (send_all(sock, buffer, bytes_read) < 0) {
                perror("send file data");
                failed = 1;
                break;
            }
            total_sent += bytes_read;
            printf("\rSent: %llu / %llu bytes",
                   total_sent, (unsigned long long)size);
            fflush(stdout);
        }

        if (bytes_read < sizeof(buffer)) {
            if (ferror(file)) {
                perror("fread");
                failed = 1;
            }
            break;
        }
    }
    printf("\n");
    fclose(file);

    if (failed || total_sent != (unsigned long long)size) {
        fprintf(stderr, "Sending failed or file size changed during transfer\n");
        close(sock);
        return 1;
    }

    shutdown(sock, SHUT_WR);

    char response[32];
    if (recv_line(sock, response, sizeof(response)) == 0 &&
        strcmp(response, "OK") == 0) {
        printf("Receiver confirmed successful file saving.\n");
    } else {
        fprintf(stderr, "Transfer was not confirmed by receiver.\n");
        close(sock);
        return 1;
    }

    close(sock);
    return 0;
}
