#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "zwl.h" 

#define SERVER_IP "127.0.0.1"
#define PORT 9011
#define BUFFER_SIZE 4096

void send_message(int socket, ZwlMessage *message) {
    char *message_str = ZwlMessage_to_String(message);
    if (!message_str) {
        fprintf(stderr, "Failed to serialize message\n");
        return;
    }

    send(socket, message_str, strlen(message_str), 0);
    free(message_str);
}

void receive_response(int socket, char r_buffer[BUFFER_SIZE]) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        perror("recv");
    }

    buffer[bytes_received] = '\0';
    printf("Server Response:\n%s\n", buffer);
    strcpy(r_buffer, buffer);
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;

    
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n");

    
    ZwlEntry payload[] = {{"client", "1.0"}};
    ZwlMessage handshake = create_zwl_message("SERVER", "HANDSHAKE", payload, 1);
    send_message(client_socket, &handshake);

    char R_BUFFER[BUFFER_SIZE];
    receive_response(client_socket, R_BUFFER);


    ZwlMessage response_message_to_check = parse_zwl_message_body(R_BUFFER);
    if (!strcmp(response_message_to_check.target, "CLIENT") == 0) {
        printf("Unexpected target: %s\n", response_message_to_check.target);
    }


    free_zwl_message(&handshake);
    bool running = true;

    while (running) {
        char input[BUFFER_SIZE];
        scanf("%s", input);


        if (strcmp(input, "q")) {
            ZwlMessage close_msg = create_zwl_message("SERVER", "CLOSE", NULL, 0);
            send_message(client_socket, &close_msg);
            free_zwl_message(&close_msg);
            running = false;
        }



    }


    close(client_socket);
    return 0;
}
