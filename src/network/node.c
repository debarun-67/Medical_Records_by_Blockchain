#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <time.h>

#include "protocol.h"
#include "node.h"

Peer peers[MAX_PEERS];
int peer_count = 0;
pthread_mutex_t peer_lock = PTHREAD_MUTEX_INITIALIZER;

/* =========================
   Find Peer Index
========================= */
int find_peer_by_socket(int socket)
{
    for (int i = 0; i < MAX_PEERS; i++)
    {
        if (peers[i].active && peers[i].socket == socket)
            return i;
    }
    return -1;
}

/* =========================
   Remove Peer Safely
========================= */
void remove_peer(int socket)
{
    pthread_mutex_lock(&peer_lock);

    int index = find_peer_by_socket(socket);
    if (index != -1)
    {
        peers[index].active = 0;
        close(peers[index].socket);
        peer_count--;
        printf("Peer removed.\n");
    }

    pthread_mutex_unlock(&peer_lock);
}

/* =========================
   Update Last Seen
========================= */
void update_peer_last_seen(int socket)
{
    pthread_mutex_lock(&peer_lock);

    int index = find_peer_by_socket(socket);
    if (index != -1)
    {
        peers[index].last_seen = time(NULL);
    }

    pthread_mutex_unlock(&peer_lock);
}

/* =========================
   MESSAGE HANDLER
========================= */
void handle_message(int client_socket, const char *message)
{
    update_peer_last_seen(client_socket);
    protocol_dispatch(client_socket, message);
}

/* =========================
   CLIENT THREAD
========================= */
void *client_thread(void *arg)
{
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    char message_buffer[BUFFER_SIZE];
    int message_len = 0;

    while (1)
    {
        int bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0)
        {
            break;
        }

        buffer[bytes] = '\0';

        for (int i = 0; i < bytes; i++)
        {
            if (buffer[i] == '\n')
            {
                message_buffer[message_len] = '\0';
                handle_message(client_socket, message_buffer);
                message_len = 0;
            }
            else
            {
                if (message_len < BUFFER_SIZE - 1)
                    message_buffer[message_len++] = buffer[i];
            }
        }
    }

    remove_peer(client_socket);
    return NULL;
}

/* =========================
   START SERVER
========================= */
void start_server(int port)
{
    int server_fd;
    struct sockaddr_in address, client_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    while (1)
    {
        int *client_socket = malloc(sizeof(int));
        socklen_t addrlen = sizeof(client_addr);

        *client_socket = accept(server_fd,
                                (struct sockaddr *)&client_addr,
                                &addrlen);

        if (*client_socket < 0)
        {
            free(client_socket);
            continue;
        }

        pthread_mutex_lock(&peer_lock);

        for (int i = 0; i < MAX_PEERS; i++)
        {
            if (!peers[i].active)
            {
                peers[i].socket = *client_socket;
                peers[i].port = ntohs(client_addr.sin_port);
                peers[i].last_seen = time(NULL);
                peers[i].active = 1;
                peer_count++;
                break;
            }
        }

        pthread_mutex_unlock(&peer_lock);

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_thread, client_socket);
        pthread_detach(thread_id);
    }
}

/* =========================
   CONNECT TO PEER
========================= */
void connect_to_peer(const char *ip, int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return;

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)
    {
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        close(sock);
        return;
    }

    pthread_mutex_lock(&peer_lock);

    for (int i = 0; i < MAX_PEERS; i++)
    {
        if (!peers[i].active)
        {
            peers[i].socket = sock;
            peers[i].port = port;
            peers[i].last_seen = time(NULL);
            peers[i].active = 1;
            peer_count++;
            break;
        }
    }

    pthread_mutex_unlock(&peer_lock);

    printf("Connected to peer %d\n", port);

    pthread_t thread_id;
    int *socket_ptr = malloc(sizeof(int));
    *socket_ptr = sock;
    pthread_create(&thread_id, NULL, client_thread, socket_ptr);
    pthread_detach(thread_id);
}

/* =========================
   BROADCAST
========================= */
void broadcast_message(const char *message)
{
    pthread_mutex_lock(&peer_lock);

    for (int i = 0; i < MAX_PEERS; i++)
    {
        if (peers[i].active)
        {
            send(peers[i].socket, message, strlen(message), 0);
        }
    }

    pthread_mutex_unlock(&peer_lock);
}

/* =========================
   PEER COUNT (THREAD SAFE)
========================= */
int get_peer_count()
{
    pthread_mutex_lock(&peer_lock);
    int count = peer_count;
    pthread_mutex_unlock(&peer_lock);
    return count;
}
