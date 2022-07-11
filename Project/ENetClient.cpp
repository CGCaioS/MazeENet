#include "ENetClient.h"
#include <chrono>
#include <ctime>
#include <thread>

const uint8_t SERVER_ID = 0;
const std::time_t REQUEST_INTERVAL = 16666; // 60fps

char ENetClient::MESSAGE_BUFFER[] = {};

ENetClient::ENetClient()
    : m_host(nullptr)
    , m_peerID(-1)
{
    // initialize enet
    // TODO: prevent this from being called multiple times
    if (enet_initialize() != 0) 
    {
        return;
    }
    // create a host
    m_host = enet_host_create(
        nullptr, // create a client host
        1, // only allow 1 outgoing connection
        NUM_CHANNELS, // allow up to N channels to be used
        0, // assume any amount of incoming bandwidth
        0); // assume any amount of outgoing bandwidth
    // TODO check if creation was successful
    
}

ENetClient::~ENetClient()
{
    // disconnect if haven't already
    Disconnect();
    // destroy the host
    // NOTE: safe to call when host is nullptr
    enet_host_destroy(m_host);
    m_host = nullptr;
    // TODO: prevent this from being called multiple times
    enet_deinitialize();
}

bool ENetClient::Connect(const std::string& host, uint32_t port)
{
    if (IsConnected()) 
    {
        //LOG_DEBUG("ENetClient is already connected to a server");
        return 0;
    }
    // set address to connect to
    ENetAddress address;
    enet_address_set_host(&address, host.c_str());
    address.port = port;
    // initiate the connection, allocating the two channels 0 and 1.
    m_server = enet_host_connect(m_host, &address, NUM_CHANNELS, 0);
    if (m_server == nullptr) 
    {
        return 1;
    }
    // attempt to connect to the peer (server)
    ENetEvent event;
    // wait N for connection to succeed
    // NOTE: we don't need to check / destroy packets because the server will be
    // unable to send the packets without first establishing a connection
    if (enet_host_service(m_host, &event, TIMEOUT_MS) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) 
    {
        if (event.peer != nullptr)
        {
            m_peerID = event.peer->outgoingPeerID;
        }

        // Connection successful

        return 0;
    }
    // failure to connect
     enet_peer_reset(m_server);
    m_server = nullptr;
    
    return 1;
}

bool ENetClient::Disconnect()
{
    if (!IsConnected()) 
    {
        return 0;
    }
    
    // attempt to gracefully disconnect
    enet_peer_disconnect(m_server, 0);
    // wait for the disconnect to be acknowledged
    ENetEvent event;
    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch())
        .count();;
    bool success = false;
    while (true) 
    {
        int32_t res = enet_host_service(m_host, &event, 0);
        if (res > 0) 
        {
            // event occured
            if (event.type == ENET_EVENT_TYPE_RECEIVE) 
            {
                // throw away any received packets during disconnect
                enet_packet_destroy(event.packet);
            } 
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT) 
            {
                // disconnect successful              
                success = true;
                break;
            }
        } 
        else if (res < 0) 
        {
            // error occured
            break;
        }
        // check for timeout
        auto now = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
        if (now - timestamp > TIMEOUT_MS * 1000) 
        {
            break;
        }
    }

    if (!success) 
    {
        // disconnect attempt didn't succeed yet, force close the connection
        enet_peer_reset(m_server);
    }
    m_server = nullptr;
    return !success;
}

bool ENetClient::IsConnected() const
{
    return m_host->connectedPeers > 0;
}

void ENetClient::Send(DeliveryType type, const std::string& messageStr) const
{
    if (!IsConnected())
    {
        return;
    }

    uint32_t channel = 0;
    uint32_t flags = 0;
    if (type == DeliveryType::RELIABLE) 
    {
        channel = RELIABLE_CHANNEL;
        flags = ENET_PACKET_FLAG_RELIABLE;
    } 
    else 
    {
        channel = UNRELIABLE_CHANNEL;
        flags = ENET_PACKET_FLAG_UNSEQUENCED;
    }

    // get bytes

    // create the packet
    ENetPacket* p = enet_packet_create(
        messageStr.c_str(),
        strlen(messageStr.c_str()) + 1,
        flags);

    // send the packet to the peer
    enet_peer_send(m_server, channel, p);
    // flush / send the packet queue
    enet_host_flush(m_host);
}

std::vector<Message> ENetClient::Poll()
{
    std::vector<Message> msgs;
    if (!IsConnected()) 
    {
        return msgs;
    }
    ENetEvent event;
    while (true) 
    {
        // poll with a zero timeout
        int32_t res = enet_host_service(m_host, &event, 0);
        if (res > 0) 
        {
            // event occured
            if (event.type == ENET_EVENT_TYPE_RECEIVE) 
            {
                // received a packet

                memcpy(MESSAGE_BUFFER, event.packet->data, event.packet->dataLength);

                Message msg = Message(SERVER_ID, Message::Type::DATA, MESSAGE_BUFFER);

                msgs.push_back(msg);

                // destroy packet payload
                enet_packet_destroy(event.packet);

            } 
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT) 
            {
                Message msg = Message(SERVER_ID, Message::Type::DISCONNECT, "");
                msgs.push_back(msg);
                m_server = nullptr;
            }
        } 
        else if (res < 0) 
        {
            // error occured
            
            break;
        } 
        else 
        {
            // no event
            break;
        }
    }
    
    return msgs;
}
