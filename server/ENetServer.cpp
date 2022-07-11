#include "ENetServer.h"


#include <chrono>
#include <ctime>

const uint32_t MAX_CONNECTIONS = 64;

char ENetServer::MESSAGE_BUFFER[] = {};

ENetServer::ENetServer()
    : m_host(nullptr)
{
    // initialize enet
    // TODO: prevent this from being called multiple times
    if (enet_initialize() != 0) 
    {        
        return;
    }
}

ENetServer::~ENetServer()
{
    // stop the server if we haven't already
    Stop();
    // TODO: prevent this from being called multiple times
    enet_deinitialize();
}

bool ENetServer::Start(uint32_t port)
{
    // create address
    ENetAddress address;
    enet_address_set_host(&address, "localhost");
    address.port = port;
    // create host
    m_host = enet_host_create(
        &address, // the address to bind the server host to
        MAX_CONNECTIONS, // allow up to N clients and/or outgoing connections
        NUM_CHANNELS, // allow up to N channels to be used
        0, // assume any amount of incoming bandwidth
        0); // assume any amount of outgoing bandwidth
    // check if creation was successful
    // NOTE: this only fails if malloc fails inside `enet_host_create`
    if (m_host == nullptr) 
    {        
        return 1;
    }
    return 0;
}

bool ENetServer::Stop()
{
    if (!IsRunning()) 
    {
        // no clients to disconnect from
        return 0;
    }
    // attempt to gracefully disconnect all clients
    
    for (auto iter : m_clients) 
    {
        auto client = iter.second;
        
        enet_peer_disconnect(client, 0);
    }
    // wait for the disconnections to be acknowledged
    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch())
        .count();;
    bool success = false;
    ENetEvent event;
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
               
                // remove from remaining
                m_clients.erase(event.peer->incomingPeerID);
            } 
            else if (event.type == ENET_EVENT_TYPE_CONNECT) 
            {
                // client connected
                
                   
                // add and remove client
                enet_peer_disconnect(event.peer, 0);
                // add to remaining
                m_clients[event.peer->incomingPeerID] = event.peer;
            }
        } 
        else if (res < 0) 
        {
            // error occured
            
            break;
        } 
        else 
        {
            // no event, check if finished
            if (m_clients.empty()) 
            {
                // all clients successfully disconnected
                
                success = true;
                break;
            }
        }
        // check timeout
        auto timestampNow = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
        if (timestampNow - timestamp > TIMEOUT_MS * 1000) 
        {
            
            break;
        }
    }
    // force disconnect the remaining clients
    for (auto iter : m_clients) 
    {
        auto id = iter.first;
        auto client = iter.second;
        
        enet_peer_reset(client);
    }
    // clear clients
    m_clients = std::map<uint32_t, ENetPeer*>();
    // destroy the host
    enet_host_destroy(m_host);
    m_host = nullptr;
    return !success;
}

bool ENetServer::IsRunning() const
{
    return m_host != nullptr;
}

uint32_t ENetServer::NumClients() const
{
    return static_cast<uint32_t>(m_host->connectedPeers);
}

ENetPeer* ENetServer::GetClient(uint32_t id) const
{
    auto iter = m_clients.find(id);
    if (iter == m_clients.end()) 
    {
        // no client to send to
        
        return nullptr;
    }
    return iter->second;
}


void ENetServer::Send(uint32_t id, DeliveryType type, const std::string& messageStr) const
{
    auto client = GetClient(id);
    if (!client) 
    {
        // no client to send to
        
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

    ENetPacket* p = enet_packet_create(
        messageStr.c_str(),
        strlen(messageStr.c_str()) + 1,
        flags);

    // send the packet to the peer
    enet_peer_send(client, channel, p);
    // flush / send the packet queue
    enet_host_flush(m_host);
}

void ENetServer::Broadcast(DeliveryType type, const std::string& messageStr) const
{
    if (NumClients() == 0) 
    {
        // no clients to broadcast to
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

    ENetPacket* p = enet_packet_create(
        messageStr.c_str(),
        strlen(messageStr.c_str()) + 1,
        flags);

    // send the packet to the peer
    enet_host_broadcast(m_host, channel, p);
    // flush / send the packet queue
    enet_host_flush(m_host);
}

std::vector<Message> ENetServer::Poll()
{
    std::vector<Message> msgs;
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
                
                Message msg = Message(event.peer->incomingPeerID, Message::Type::DATA, MESSAGE_BUFFER);

                msgs.push_back(msg);                

                // destroy packet payload
                enet_packet_destroy(event.packet);

            } 
            else if (event.type == ENET_EVENT_TYPE_CONNECT) 
            {
                // client connected
                
                // add msg
                Message msg = Message( event.peer->incomingPeerID, Message::Type::CONNECT, "");
                msgs.push_back(msg);
                m_clients[event.peer->incomingPeerID] = event.peer;

            } 
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT) 
            {
                // client disconnected
                
                // add msg
                Message msg = Message(event.peer->incomingPeerID, Message::Type::DISCONNECT, "");
                msgs.push_back(msg);
                m_clients.erase(event.peer->incomingPeerID);
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
