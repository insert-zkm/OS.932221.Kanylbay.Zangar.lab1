#include <vector>
#include <signal.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

volatile sig_atomic_t wasSigHup = 0;

void handleSigHup(int signal)
{
    wasSigHup = 1;
}

int main() {
    struct sigaction action;
    sigaction(SIGHUP, NULL, &action);
    action.sa_handler = handleSigHup;
    action.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &action, NULL);

    sigset_t sigMask, originalMask;
    sigemptyset(&sigMask);
    sigaddset(&sigMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &sigMask, &originalMask);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Ошибка создания сокета");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(12345);

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Ошибка привязки сокета");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, SOMAXCONN) < 0) {
        perror("Ошибка при попытке прослушивания");
        exit(EXIT_FAILURE);
    }

    printf("Сервер успешно запущен на порту 12345. Ожидание клиентов...\n");

    std::vector<int> clientSockets;
    int highestFd = serverSocket;
    fd_set descriptorSet;

    while (true)
    {
        FD_ZERO(&descriptorSet);
        FD_SET(serverSocket, &descriptorSet);
        highestFd = serverSocket;

        for (int socketFd : clientSockets) {
            FD_SET(socketFd, &descriptorSet);
            if (socketFd > highestFd) highestFd = socketFd;
        }

        if (pselect(highestFd + 1, &descriptorSet, NULL, NULL, NULL, &originalMask) == -1)
        {
            if (errno == EINTR)
            {
                printf("Поступил сигнал SIGHUP\n");
                if (wasSigHup) {
                    wasSigHup = 0;
                    printf("Сигнал SIGHUP обработан успешно\n");
                }
                continue;
            }
            else 
            {
                perror("Ошибка pselect");
                break;
            }
        }

        if (FD_ISSET(serverSocket, &descriptorSet)) {
            int newClientSocket = accept(serverSocket, NULL, NULL);
            if (newClientSocket != -1) {
                printf("Подключен новый клиент: %d\n", newClientSocket);

                for (int oldSocket : clientSockets) {
                    printf("Закрытие старого соединения: %d\n", oldSocket);
                    close(oldSocket);
                }
                clientSockets.clear();
                clientSockets.push_back(newClientSocket);
            }
        }

        for (auto iter = clientSockets.begin(); i != clientSockets.end();) {
            int clientFd = *iter;
            if (FD_ISSET(clientFd, &descriptorSet)) {
                char buffer[1024];
                ssize_t bytesRead = read(clientFd, buffer, sizeof(buffer));
                if (bytesRead <= 0) {
                    printf("Клиент %d отключился. Закрываем соединение.\n", clientFd);
                    close(clientFd);
                    iter = clientSockets.erase(iter);
                }
                else {
                    printf("Получено %zd байт от клиента %d\n", bytesRead, clientFd);
                    ++iter;
                }
            }
            else {
                ++iter;
            }
        }
    }

    close(serverSocket);
    printf("Сервер остановлен.\n");
    return 0;
}