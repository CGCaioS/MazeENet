#pragma once

#include "NetCommon.h"
#include "Message.h"

#include <enet/enet.h>

#include <iostream>
#include <map>
#include <memory>
#include <vector>

class ENetServer
{

public:
    ENetServer();
    ~ENetServer();

    bool Start(uint32_t);
    bool Stop();
    bool IsRunning() const;

    uint32_t NumClients() const;

    void Send(uint32_t, DeliveryType, const std::string& messageStr) const;
    void Broadcast(DeliveryType, const std::string& messageStr) const;
    std::vector<Message> Poll();

private:
    ENetPeer* GetClient(uint32_t) const;

    ENetHost* m_host;
    // NOTE: ENet allocates all peers at once and doesn't shuffle them,
    // which leads to non-contiguous connected peers. This map
    // will make it easier to manage them by id
    std::map<uint32_t, ENetPeer*> m_clients;

    static char MESSAGE_BUFFER[MAX_MESSAGE_LEN];
};
