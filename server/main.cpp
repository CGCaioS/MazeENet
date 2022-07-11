#include "ENetServer.h"

#include <chrono>
#include <sstream>

#include <iostream>
#include <string>
#include <thread>

#include <conio.h>

#include <future>
#include <atomic>

const uint32_t PORT = 7000;

bool quit = false;

ENetServer* g_server = nullptr;

namespace Net {
    enum Types {
        CLIENT_INFO
    };
}

int main(int argc, char** argv)
{
    g_server = new ENetServer();
    if (g_server->Start(PORT)) 
    {
        return 1;
    }

    auto frameCount = 0;

    int anim = 0;

    auto getInput = []()->int {
        return _getch();
    };

    auto future = std::async(std::launch::async, getInput);

    while (true)
    {
        std::system("cls");

        std::cout << "\nServer running";

        for (int i = 0; i <= anim; i++)
        {
            std::cout << ".";
        }

        if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            auto input = future.get();

            future = std::async(std::launch::async, getInput);

            std::cout << " Input: " << input << std::endl;

            std::stringstream stringStream;

            stringStream << "0-0,0";

            // broadcast to all clients
            g_server->Broadcast(DeliveryType::RELIABLE, stringStream.str());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        anim++;
        anim = anim % 3;

        // poll for events
        auto messages = g_server->Poll();

        // process events
        for (const auto& msg : messages) 
        {
            uint32_t id = msg.GetPeerID();

            switch (msg.GetType()) 
            {

                case Message::Type::CONNECT:
                        std::cout << "\nConnection from client_" << id << " received";
                        break;

                case Message::Type::DISCONNECT:

                        std::cout << "\nConnection from client_" << id << " lost";
                        break;

                case Message::Type::DATA:  
                    
                        std::cout << "\nData from client_" << id << " " << msg.GetData();

                        break;
            }
        }        

        // check if exit
        if (quit) 
        {
            break;
        }
    }

    // stop server and disconnect all clients
    g_server->Stop();

    delete g_server;
    g_server = nullptr;
}
