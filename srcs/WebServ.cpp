

void WebServ::eventLoop()
{
    struct epoll_event event, events[10]; // TODO: replace 10 with configurable value

    // Init a epoll instance
    int epollFd = epoll_create(0);
    if (epollFd == -1)
    {
        std::cerr << "Failed to create epoll instance." << std::endl;
        return;
    }

    // Listen to the ports
    auto it = serverMap_.begin();
    while (it != serverMap_.end())
    {
        int socketFd = socket(AF_INET, SOCK_STREAM, 0);
        if (socketFd < 0)
        {
            std::cerr << "Error creating socket" << std::endl;
            return;
        }

        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(8080); //  TODO: replace with it->first
        serverAddress.sin_addr.s_addr = INADDR_ANY;

        if (bind(socketFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
        {
            std::cerr << "Failed to bind socket." << std::endl;
            close(socketFd);
            return;
        }

        if (listen(socketFd, 10) == -1) //  TODO: replace it with value from config.
        {
            std::cerr << "Failed to listen." << std::endl;
            close(socketFd);
            return;
        }

        event.events = EPOLLIN;
        event.data.fd = socketFd;
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, socketFd, &event) == -1)
        {
            std::cerr << "Failed to add socket to epoll instance." << std::endl;
            close(socketFd); // TODO: close all sockets.
            close(epollFd);
            return 1;
        }
    }

    // event loop
    while (true)
    {
        //TODO: Timeout killer.

        while (true)
        {
            //TODO: Timeout killer.

            int numEvents = epoll_wait(epollFd, events, 10, 1000); // TODO:replace 10 and 1000 with configurable value
            if (numEvents == -1)
            {
                std::cerr << "Failed to wait for events." << std::endl; // TODO: handle error
                break;
            }

            for (int i = 0; i < numEvents; ++i)
            {
                // Check if the event is for a server socket
                if (serverMap_.find(events[i].data.fd) != serverMap_.end()) // TODO: replace with serverMap. fd
                {
                    struct sockaddr_in clientAddress;
                    socklen_t clientAddressLength = sizeof(clientAddress);
                    int clientFd = accept(serverFd, (struct sockaddr *)&clientAddress, &clientAddressLength);
                    if (clientFd == -1)
                    {
                        std::cerr << "Failed to accept client connection." << std::endl;
                        continue;
                    }

                    // Add client socket to epoll
                    event.events = EPOLLIN;
                    event.data.fd = clientFd;
                    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &event) == -1)
                    {
                        std::cerr << "Failed to add client socket to epoll instance." << std::endl;
                        close(clientFd);
                        continue;
                    }
                }
                else if (events[i].events & EPOLLIN) // Check if the event is for a client socket
                {
                    // incoming data from client

                    // incoming data from response
                }
                else if (events[i].events & EPOLLOUT) // Check if the event is for a client socket
                {
                    // outgoing data to client

                    // outgoing data to handler
                }
                else if (events[i].events & EPOLLERR) // Check if the event is an error
                {
                    std::cerr << "Error on socket: " << events[i].data.fd << std::endl;
                    close(events[i].data.fd); // Close the socket on error
                }
                else if (events[i].events & EPOLLHUP) // Check if the event is a hang-up
                {
                    std::cerr << "Hang-up on socket: " << events[i].data.fd << std::endl;
                    close(events[i].data.fd); // Close the socket on hang-up
                }
                else
                {
                    std::cerr << "Unknown event on socket: " << events[i].data.fd << std::endl;
                    close(events[i].data.fd); // Close the socket on unknown event
                }
            }
        }
    }