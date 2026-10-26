#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#define PORT 9020
#define CHUNK_SIZE 1024
#define DEST_DIR "received_files"
#define MAX_FILENAME 255

static int recv_all(int sock, void *buffer, size_t len)
{
    size_t done = 0;
    char *p = buffer;

    while (done < len) {
        ssize_t n = recv(sock, p + done, len - done, 0);
        if (n <= 0) return -1;
        done += (size_t)n;
    }
    return 0;
}

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
    int server_fd = -1, client_fd = -1;
    FILE *out = NULL;
    char output_path[PATH_MAX];
    char header[64];
    char filename[MAX_FILENAME + 1];
    char buffer[CHUNK_SIZE];
    int failed = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    if (mkdir(DEST_DIR, 0755) < 0 && errno != EEXIST) {
        perror("mkdir " DEST_DIR);
        close(server_fd);
        return 1;
    }

    printf("TCP receiver listening on port %d\n", PORT);
    printf("Saving files to: %s\n", DEST_DIR);
    printf("Waiting for sender...\n");
    fflush(stdout);

    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        close(server_fd);
        return 1;
    }

    /* Protocol: filename length + newline, filename bytes,
       file size + newline, then exactly file_size data bytes. */
    if (recv_line(client_fd, header, sizeof(header)) < 0) {
        fprintf(stderr, "Could not receive filename length\n");
        goto cleanup;
    }

    char *end = NULL;
    errno = 0;
    unsigned long name_len = strtoul(header, &end, 10);
    if (errno || header[0] == '\0' || *end != '\0' ||
        name_len == 0 || name_len > MAX_FILENAME) {
        fprintf(stderr, "Invalid filename length\n");
        goto cleanup;
    }

    if (recv_all(client_fd, filename, (size_t)name_len) < 0) {
        fprintf(stderr, "Could not receive filename\n");
        goto cleanup;
    }
    filename[name_len] = '\0';

    /* Only accept a plain filename, not a directory path. */
    if (strchr(filename, '/') || strchr(filename, '\\') ||
        strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
        fprintf(stderr, "Unsafe filename rejected\n");
        goto cleanup;
    }

    if (recv_line(client_fd, header, sizeof(header)) < 0) {
        fprintf(stderr, "Could not receive file size\n");
        goto cleanup;
    }

    errno = 0;
    unsigned long long total_size = strtoull(header, &end, 10);
    if (errno || header[0] == '\0' || *end != '\0' || header[0] == '-') {
        fprintf(stderr, "Invalid file size\n");
        goto cleanup;
    }

    int path_len = snprintf(output_path, sizeof(output_path),
                            "%s/%s", DEST_DIR, filename);
    if (path_len < 0 || (size_t)path_len >= sizeof(output_path)) {
        fprintf(stderr, "Output path too long\n");
        goto cleanup;
    }

    out = fopen(output_path, "wb");
    if (!out) {
        perror("fopen output file");
        goto cleanup;
    }

    printf("Receiving file: %s\n", filename);
    printf("Expected size: %llu bytes\n", total_size);

    unsigned long long received_size = 0;
    while (received_size < total_size) {
        unsigned long long remaining = total_size - received_size;
        size_t wanted = remaining < CHUNK_SIZE
                      ? (size_t)remaining : CHUNK_SIZE;

        ssize_t n = recv(client_fd, buffer, wanted, 0);
        if (n <= 0) {
            fprintf(stderr, "\nConnection ended before the whole file arrived\n");
            goto cleanup;
        }

        size_t written = fwrite(buffer, 1, (size_t)n, out);
        if (written != (size_t)n) {
            perror("fwrite");
            goto cleanup;
        }

        received_size += (unsigned long long)n;
        printf("\rReceived: %llu / %llu bytes",
               received_size, total_size);
        fflush(stdout);
    }
    printf("\n");

    if (fflush(out) != 0) {
        perror("fflush");
        goto cleanup;
    }
    if (fclose(out) != 0) {
        out = NULL;
        perror("fclose");
        goto cleanup;
    }
    out = NULL;

    if (send_all(client_fd, "OK\n", 3) < 0)
        fprintf(stderr, "File saved, but acknowledgment could not be sent\n");
    else
        printf("File saved successfully: %s\n", output_path);

    failed = 0;

cleanup:
    if (out) fclose(out);
    if (failed && client_fd >= 0)
        (void)send_all(client_fd, "ERR\n", 4);
    if (client_fd >= 0) close(client_fd);
    if (server_fd >= 0) close(server_fd);
    return failed ? 1 : 0;
}
