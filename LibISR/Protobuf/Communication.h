#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "result.pb.h"
#include "../UI/UIEngine.h"

namespace LibISR
{
    namespace Protobuf
    {
        class Communication
        {
        private:
            int server_socketfd;
            int client_socketfd;

            bool sending = false;

            struct sockaddr_in server_addr;
            struct sockaddr_in client_addr;

            socklen_t client_addr_size;

        public:
            int initServer();
            void communicate();

            ~Communication();
        };
    };
};