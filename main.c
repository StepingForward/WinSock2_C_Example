#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024
#define NAME_SIZE 64
#define MAX_CLIENTS 10

typedef struct {
    SOCKET socket;
    struct sockaddr_in server_addr;
    struct timeval timeout;
    char name[NAME_SIZE];
} Client;

typedef struct {
    SOCKET server_socket, client_socket;
    Client clients[MAX_CLIENTS];
    struct sockaddr_in server_addr, client_addr;
    int addr_len;
} Server;

void handleError(const char* msg);

Server createEmptyServer();

Client createEmptyClient(char* ip);
void connectClient(Client* client);

void runServer();
void runClient();

int main() {
    WSADATA wsaData;
    char choice;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        handleError("WSAStartup failed");
    }

    printf("Run as (s)erver or (c)lient? ");
    scanf_s(" %c", &choice, 1);
    int ign = getchar();

    if (choice == 's') {
        runServer();
    }
    else if (choice == 'c') {
        runClient();
    }
    else {
        printf("Invalid option.\n");
    }

    WSACleanup();
    return 0;
}

void handleError(const char* msg) {
    printf("%s: %d\n", msg, WSAGetLastError());
    exit(1);
}

Server createEmptyServer() {
    SOCKET server_socket, client_socket, clients[MAX_CLIENTS] = { 0 };
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(struct sockaddr_in);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        handleError("Socket creation failed");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        handleError("Bind failed");
    }

    if (listen(server_socket, MAX_CLIENTS) == SOCKET_ERROR) {
        handleError("Listen failed");
    }

    return (Server) {
        .addr_len = addr_len,
            .server_socket = server_socket,
            .client_socket = { 0 },
            .clients = clients,
    };
}

Client createEmptyClient(char* ip) {
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        handleError("Socket creation failed");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(PORT);

    return (Client) {
        .socket = client_socket,
            .timeout = timeout,
            .server_addr = server_addr,
    };
}

void connectClient(Client* client) {
    if (connect(client->socket, (struct sockaddr*)&client->server_addr, sizeof(client->server_addr)) == SOCKET_ERROR) {
        closesocket(client->socket);
        handleError("Connection to server failed.");
    }
}

void runServer() {
    Server server;
    fd_set masterSet, readSet;
    char buffer[BUFFER_SIZE];

    server = createEmptyServer();

    printf("Server started on port %d. Waiting for connections...\n", PORT);

    FD_ZERO(&masterSet);
    FD_SET(server.server_socket, &masterSet);

    while (1) {
        readSet = masterSet;

        // Wait for activity
        int activity = select(0, &readSet, NULL, NULL, NULL);
        if (activity == SOCKET_ERROR) {
            handleError("select() failed");
        }

        if (FD_ISSET(server.server_socket, &readSet)) {
            server.client_socket = accept(server.server_socket, (struct sockaddr*)&server.client_addr, &server.addr_len);
            if (server.client_socket == INVALID_SOCKET) {
                printf("Accept failed\n");
                continue;
            }

            char clientName[50];
            int nameLength = recv(server.client_socket, clientName, sizeof(clientName) - 1, 0);
            if (nameLength <= 0) {
                closesocket(server.client_socket);
                continue;
            }
            clientName[nameLength] = '\0';

            int assigned = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (server.clients[i].socket == 0) {
                    server.clients[i].socket = server.client_socket;
                    strcpy_s(server.clients[i].name, sizeof(server.clients[i].name), clientName);
                    FD_SET(server.client_socket, &masterSet);
                    printf("New client connected: %s (Socket: %d)\n", clientName, server.client_socket);
                    assigned = 1;
                    break;
                }
            }

            if (!assigned) {
                printf("Server full. Rejecting client.\n");
                closesocket(server.client_socket);
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            SOCKET sock = server.clients[i].socket;
            if (sock > 0 && FD_ISSET(sock, &readSet)) {
                int bytesReceived = recv(sock, buffer, BUFFER_SIZE - 1, 0);

                if (bytesReceived <= 0) {
                    printf("Client disconnected: %d\n", sock);
                    closesocket(sock);
                    FD_CLR(sock, &masterSet);
                    server.clients[i].socket = 0;
                }
                else {
                    buffer[bytesReceived] = '\0';
                    

                    char message[BUFFER_SIZE + NAME_SIZE];
                    sprintf_s(message, sizeof(message), "%s: %s", server.clients[i].name, buffer);

                    printf("%s\n", message);

                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (server.clients[j].socket > 0 && server.clients[j].socket != sock) {
                            send(server.clients[j].socket, message, strlen(message) + 2, 0);
                        }
                    }
                }
            }
        }
    }

    closesocket(server.server_socket);
}

void runClient() {
    Client client;
    fd_set readSet;
    char buffer[BUFFER_SIZE];

    client = createEmptyClient("127.0.0.1");
    connectClient(&client);

    printf("Enter your name: ");
    fgets(client.name, sizeof(client.name), stdin);
    client.name[strcspn(client.name, "\n")] = 0;
    send(client.socket, client.name, strlen(client.name), 0);

    printf("Connected to server as %s!\nType messages and press Enter to send.\n", client.name);

    while (1) {
        FD_ZERO(&readSet);
        FD_SET(client.socket, &readSet);

        client.timeout.tv_sec = 0;
        client.timeout.tv_usec = 100000;

        int activity = select(0, &readSet, NULL, NULL, &client.timeout);

        if (activity == SOCKET_ERROR) {
            printf("select() failed: %d\n", WSAGetLastError());
            break;
        }

        if (activity > 0 && FD_ISSET(client.socket, &readSet)) {
            char message[BUFFER_SIZE + NAME_SIZE];
            int bytesReceived = recv(client.socket, message, BUFFER_SIZE + NAME_SIZE, 0);
            printf("%s\n", message);
            if (bytesReceived <= 0) {
                printf("Disconnected from server.\n");
                break;
            }
            message[bytesReceived] = '\0';
        }

        if (_kbhit()) {
            printf("Sending: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            buffer[strcspn(buffer, "\n")] = 0;
            send(client.socket, buffer, strlen(buffer), 0);
            if (strcmp(buffer, "exit") == 0) break;
        }

        Sleep(10);
    }

    closesocket(client.socket);
}
