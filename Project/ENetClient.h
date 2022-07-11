#pragma once

#include "NetCommon.h"
#include "Message.h"

#include <enet/enet.h>

#include <vector>


class ENetClient 
{
public:
    ENetClient();
    ~ENetClient();

    bool Connect(const std::string&, uint32_t);
    bool Disconnect();
    bool IsConnected() const;

    void Send(DeliveryType, const std::string& messageStr) const;
    std::vector<Message> Poll();

    static ENetClient& GetInstance()
    {
        static ENetClient instance;
        return instance;
    }

    inline int GetPeerID() const { return m_peerID;  }

private:
    
    ENetHost* m_host;
    ENetPeer* m_server;

    int m_peerID;

    static char MESSAGE_BUFFER[MAX_MESSAGE_LEN];
};
