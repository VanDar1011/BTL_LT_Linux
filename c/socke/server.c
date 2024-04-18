#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>
#define PORT 5555
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 20 // Số lượng client tối đa có thể kết nối
typedef struct {
    int clientNumber;
    SOCKET socketDesc;
} ClientInfo;
void error(const char* msg) {
    perror(msg);
    exit(1);
}
void* receive(void** clientInfo_ptr) {
    ClientInfo* clientInfo = (ClientInfo*)clientInfo_ptr;
    int clientNumber = clientInfo->clientNumber;
    SOCKET newsockfd = clientInfo->socketDesc;
    char buffer[BUFFER_SIZE];
    int n;  

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        n = recv(newsockfd, buffer, BUFFER_SIZE - 1, 0);
        if (n < 0) {
            printf("Client %d disconnected\n", clientNumber);
            break;
        }
        else if (n == 0) {
            printf("Client %d disconnected\n", clientNumber);
            break;
        }

        printf("Client %d: %s\n", clientNumber, buffer);
    }

    return NULL;
}
void* sendMessages(void* sockets_ptr) {
    SOCKET* sockets = (SOCKET*)sockets_ptr;
    char buffer[BUFFER_SIZE];
    int n;

    while (1) {
        printf("Server: ");
        fgets(buffer, BUFFER_SIZE - 1, stdin);
        size_t length = strlen(buffer);
        if (length > INT_MAX) {
            // Xử lý trường hợp độ dài chuỗi vượt quá giới hạn của biến int
        }
        //printf("d", length);
        if( length > 0)
        {
            // Gửi tin nhắn tới tất cả các client
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (sockets[i] != INVALID_SOCKET) {
                    send(sockets[i], buffer, length, 0);
                }
            }
        }

        // Kiểm tra xem có tin nhắn nào từ client không
        Sleep(100); // Đợi 0.1 giây
    }

    return NULL;
}
int main() {
    WSADATA wsa;
    SOCKET sockfd, newsockfd; // Mảng socket mới cho mỗi client
    struct sockaddr_in server_addr, client_addr;
    int clilen;
    char buffer[BUFFER_SIZE];
    int n;

    // Khởi tạo Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code : %d", WSAGetLastError());
        return 1;
    }

    // Tạo socket
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == INVALID_SOCKET)
        error("ERROR opening socket");

    // Thiết lập địa chỉ server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        error("ERROR on binding");

    // Lắng nghe kết nối từ client
    if (listen(sockfd, MAX_CLIENTS) == SOCKET_ERROR)
        error("ERROR on listening");

    printf("Waiting for clients to connect...\n");

    clilen = sizeof(client_addr);
    int clientCount = 0; // Số lượng client hiện tại
    ClientInfo clients[MAX_CLIENTS];
    SOCKET clientSockets[MAX_CLIENTS];
    // Chấp nhận kết nối từ nhiều client
    while (1) {
        // Chấp nhận kết nối mới từ client
        newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &clilen);
        if (newsockfd == INVALID_SOCKET) {
            error("ERROR on accept");
        }
        clients[clientCount].clientNumber = clientCount + 1;
        clients[clientCount].socketDesc = newsockfd;
        clientSockets[clientCount] = newsockfd;
        printf("Client %d connected\n", clientCount + 1);
        uintptr_t threadID;
        if (_beginthreadex(NULL, 0, receive, (void*)&clients[clientCount], 0, &threadID) == NULL) {
            error("ERROR creating thread");
        }

      
        clientCount++;
        uintptr_t sendThreadID;
            if (_beginthreadex(NULL, 0, sendMessages, (void*)clientSockets, 0, &sendThreadID) == NULL) {
                error("ERROR creating send thread");
            }
        // Nếu đã đạt đến số lượng client tối đa, dừng chấp nhận kết nối mới
        if (clientCount >= MAX_CLIENTS) {
            printf("Maximum number of clients reached. No more connections will be accepted.\n");
            break;
        }
    }
    //if (clientCount > 0) {
    //    // Bắt đầu luồng để gửi tin nhắn tới tất cả các client
    //    uintptr_t sendThreadID;
    //    if (_beginthreadex(NULL, 0, sendMessages, (void*)clientSockets, 0, &sendThreadID) == NULL) {
    //        error("ERROR creating send thread");
    //    }
    //}
    
    // Đóng tất cả các socket
    for (int i = 0; i < clientCount; i++) {
        closesocket(clients[i].socketDesc);
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}
