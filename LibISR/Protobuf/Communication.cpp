#include "Communication.h"
#include <thread>

int LibISR::Protobuf::Communication::initServer(){

    server_socketfd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_socketfd == -1){
        printf("server socket fd creation error\n");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(2333);
    if(bind(server_socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        printf("server socket fd bind error\n");
        return -1;
    }

    if(listen(server_socketfd, 5) == -1){
        printf("server socket fd listen error\n");
        return -1;
    }

    std::thread commu = std::thread(&LibISR::Protobuf::Communication::communicate, this);
    commu.detach();
}

LibISR::Protobuf::Communication::~Communication(){
    sending = false;
    close(client_socketfd);
    close(server_socketfd);
}

void LibISR::Protobuf::Communication::communicate(){
    client_addr_size = sizeof(client_addr);
    client_socketfd = accept(server_socketfd, (struct sockaddr*)&client_addr, &client_addr_size);
    if(client_socketfd == -1){
        printf("client fd accept error\n");
        return ;
    }

    int formerCmd = 0;
    while(true){
        int len;
        char recvContent[4096];
        protocol::Command recvCmd;

        len = recv(client_socketfd, recvContent, 4096, 0);
        recvCmd.ParseFromArray(recvContent, len);

        switch(recvCmd.commandid()){
        case -1:
            break;
        case 1:
            if(formerCmd == 0){
                LibISRUtils::UIEngine::setMainloopState(1);
                formerCmd = 1;
            }
            printf("start recording\n");
            break;
        case 2:
            if(formerCmd == 1){
                LibISRUtils::UIEngine::setMainloopState(2);
                formerCmd = 2;
            }
            printf("start tracing\n");
            break;
        case 3:
            LibISRUtils::UIEngine::setMainloopState(3);
            break;
        default:
            break;
        }
        if(formerCmd == 2)
            break;
    }

    while(true){
        int len;
        protocol::TracingResult result;
        result.add_result(0);
        result.add_result(1);
        std::string buf = "";
        result.SerializeToString(&buf);
        len = send(client_socketfd, buf.c_str(), buf.size(), 0);
        char empty[1024];
        len = recv(client_socketfd, empty, 1024, 0);
    }
}

