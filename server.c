#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "zwl.h" 

#define PORT 9011
#define BUFFER_SIZE 4096

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        perror("recv");
        close(client_socket);
        return;
    }

    buffer[bytes_received] = '\0';

    
    ZwlMessage message = parse_zwl_message_body(buffer);
    printf("Received Message:\nTarget: %s\nHeader: %s\n", message.target, message.header);

    
    if (parse_zwl_header(&message) == HANDSHAKE) {
        const char *response = "TARGET: SERVER\r\nHEADER: HANDSHAKE\r\nPAYLOAD:\r\nstatus: connected\r\n";
        send(client_socket, response, strlen(response), 0);
    } else if (parse_zwl_header(&message) == CLOSE) {
        const char *response = "TARGET: SERVER\r\nHEADER: CLOSE\r\nPAYLOAD:\r\nstatus: goodbye\r\n";
        send(client_socket, response, strlen(response), 0);
    } else {
        const char *response = "TARGET: SERVER\r\nHEADER: ERROR\r\nPAYLOAD:\r\nmessage: invalid header\r\n";
        send(client_socket, response, strlen(response), 0);
    }

    free_zwl_message(&message);
    close(client_socket);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket == -1) {
            perror("accept");
            continue;
        }

        printf("Client connected.\n");
        handle_client(client_socket);
    }

    close(server_fd);
    return 0;
}
