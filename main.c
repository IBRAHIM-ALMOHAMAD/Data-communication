#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <pthread.h>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 8000
#define BUFFER_SIZE 1024

char client_name[BUFFER_SIZE] = "";
const char *key = "\xAA";

void xor_encrypt_decrypt(char *data, const char *key) {
    size_t data_len = strlen(data);
    size_t key_len = strlen(key);
    for (size_t i = 0; i < data_len; i++) {
        data[i] ^= key[i % key_len];
    }
}

void *listen_to_server(void *socket_desc) {
    SOCKET server_socket = *(SOCKET *)socket_desc;
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_received = recv(server_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Disconnected from server.\n");
            break;
        }
        buffer[bytes_received] = '\0';

        xor_encrypt_decrypt(buffer, key);
        if (strstr(buffer, "Connected clients:") != NULL) {
            char *pos = strstr(buffer, client_name);
            if (pos) {
                size_t name_length = strlen(client_name);
                if (pos[name_length] == '\n') {
                    memmove(pos, pos + name_length + 1, strlen(pos + name_length + 1) + 1);
                } else {
                    memmove(pos, pos + name_length, strlen(pos + name_length) + 1);
                }
            }
            if (strcmp(buffer, "Connected clients: ") != 0) {
                printf("%s\n", buffer);
            }
        } else {
            printf("%s\n", buffer);
        }
    }
    return NULL;
}

int main() {
    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        return EXIT_FAILURE;
    }

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation error.\n");
        WSACleanup();
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Connection to server failed.\n");
        closesocket(client_socket);
        WSACleanup();
        return EXIT_FAILURE;
    }

    printf("Enter your name: ");
    fgets(client_name, BUFFER_SIZE, stdin);
    strtok(client_name, "\n");

    send(client_socket, client_name, strlen(client_name), 0);

    pthread_t listen_thread;
    pthread_create(&listen_thread, NULL, listen_to_server, (void *)&client_socket);

    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
        strtok(buffer, "\n");

        if (strcasecmp(buffer, "exit") == 0) {
            break;
        }

        xor_encrypt_decrypt(buffer, key);
        send(client_socket, buffer, strlen(buffer), 0);
    }

    closesocket(client_socket);
    pthread_join(listen_thread, NULL);
    WSACleanup();

    return 0;
}
