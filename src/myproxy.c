// myproxy.c - HTTP proxy server (manual argument parsing, standard libraries only)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <ctype.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MAXLINE 8192
#define FORBIDDEN_LIMIT 1000
#define LOGFILE_LINE 1024

char *forbidden_sites[FORBIDDEN_LIMIT];
int forbidden_count = 0;
char *log_file_path;

// Logs each client request to the log file
void log_request(const char *client_ip, const char *request_line, int status, int bytes) {
    FILE *logf = fopen(log_file_path, "a");
    if (!logf) return;
    time_t now = time(NULL);
    struct tm *tm_utc = gmtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S.000Z", tm_utc);
    fprintf(logf, "%s %s \"%s\" %d %d\n", timestamp, client_ip, request_line, status, bytes);
    fclose(logf);
}

// Checks if the given host is in the forbidden list
int is_forbidden(const char *host) {
    for (int i = 0; i < forbidden_count; i++) {
        const char *forb = forbidden_sites[i];

        // Exact match
        if (strcmp(forb, host) == 0) return 1;

        // Normalize www prefix
        if (strncmp(host, "www.", 4) == 0 && strcmp(host + 4, forb) == 0) return 1;
        if (strncmp(forb, "www.", 4) == 0 && strcmp(forb + 4, host) == 0) return 1;
    }
    return 0;
}
    return 0;
}

// Loads forbidden sites from file into memory
void load_forbidden(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) { perror("Forbidden file"); exit(1); }
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || isspace(line[0])) continue;
        line[strcspn(line, "\n")] = 0;  // remove newline
        forbidden_sites[forbidden_count++] = strdup(line);
    }
    fclose(file);
}

// Sends a simple HTTP error response to client
void send_error(int client_fd, const char *client_ip, const char *request_line, int status, const char *msg) {
    char buf[MAXLINE];
    snprintf(buf, sizeof(buf),
             "HTTP/1.1 %d %s\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n%s\n",
             status, msg, strlen(msg) + 1, msg);
    write(client_fd, buf, strlen(buf));
    log_request(client_ip, request_line, status, 0);
}

// Thread handler for each incoming client connection
void *handle_client(void *arg) {
    int client_fd = ((int *)arg)[0];
    struct sockaddr_in cliaddr = *((struct sockaddr_in *)(&((int *)arg)[1]));
    free(arg);

    char client_ip[INET_ADDRSTRLEN] = "unknown";
    inet_ntop(AF_INET, &cliaddr.sin_addr, client_ip, sizeof(client_ip));

    char buf[MAXLINE], method[16], url[1024], version[16];
    char host[256] = "", path[1024] = "/";
    char original_request_line[MAXLINE] = "";

    FILE *client_fp = fdopen(client_fd, "r+");
    if (!client_fp) { close(client_fd); return NULL; }

    if (!fgets(buf, MAXLINE, client_fp)) { fclose(client_fp); return NULL; }
    strncpy(original_request_line, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, url, version);

    // Only support GET and HEAD methods
    if (strcmp(method, "GET") && strcmp(method, "HEAD")) {
        send_error(client_fd, client_ip, original_request_line, 501, "Not Implemented");
        fclose(client_fp); return NULL;
    }

    // Extract host and path from URL
    char *host_start = strstr(url, "//");
    if (host_start) {
        host_start += 2;
        char *slash = strchr(host_start, '/');
        if (slash) {
            strncpy(host, host_start, slash - host_start);
            host[slash - host_start] = '\0';
            strcpy(path, slash);
        } else {
            strcpy(host, host_start);
        }
    } else {
        send_error(client_fd, client_ip, original_request_line, 400, "Bad Request");
        fclose(client_fp); return NULL;
    }

    // Block access to forbidden hosts
    if (is_forbidden(host)) {
        send_error(client_fd, client_ip, original_request_line, 403, "Forbidden URL");
        fclose(client_fp); return NULL;
    }

    // Connect to destination web server using HTTP (port 80)
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, "80", &hints, &res) != 0) {
        send_error(client_fd, client_ip, original_request_line, 502, "Bad Gateway");
        fclose(client_fp); return NULL;
    }

    int server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (connect(server_fd, res->ai_addr, res->ai_addrlen) != 0) {
        freeaddrinfo(res);
        send_error(client_fd, client_ip, original_request_line, 504, "Gateway Timeout");
        fclose(client_fp); return NULL;
    }

    freeaddrinfo(res);

    // Build and send request to destination server
    char request[MAXLINE];
    snprintf(request, sizeof(request), "%s %s %s\r\nHost: %s\r\nX-Forwarded-For: %s\r\nConnection: close\r\n\r\n", method, path, version, host, client_ip);
    write(server_fd, request, strlen(request));

    // Relay response from server back to client
    int total_bytes = 0;
    while (1) {
        int n = read(server_fd, buf, MAXLINE);
        if (n <= 0) break;
        total_bytes += n;
        write(client_fd, buf, n);
    }

    log_request(client_ip, original_request_line, 200, total_bytes);

    close(server_fd);
    fclose(client_fp);
    return NULL;
}

int main(int argc, char **argv) {
    int port = 0;
    char *forbidden_path = NULL;
    char *log_file_path_local = NULL;

    // Manual argument parsing
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            forbidden_path = argv[++i];
        } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            log_file_path_local = argv[++i];
        } else {
            fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]);
            exit(1);
        }
    }

    if (!port || !forbidden_path || !log_file_path_local) {
        fprintf(stderr, "Usage: %s -p <port> -a <forbidden.txt> -l <access.log>\n", argv[0]);
        exit(1);
    }

    log_file_path = log_file_path_local;
    load_forbidden(forbidden_path);

    // Setup TCP listener
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr = {0};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    listen(listen_fd, 10);
    printf("Proxy listening on port %d...\n", port);

    // Accept incoming client connections
    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);
        int connfd = accept(listen_fd, (struct sockaddr *)&cliaddr, &len);
        int *arg = malloc(sizeof(int) + sizeof(struct sockaddr_in));
        arg[0] = connfd;
        memcpy(&arg[1], &cliaddr, sizeof(struct sockaddr_in));
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, arg);
        pthread_detach(tid);
    }

    return 0;
}
