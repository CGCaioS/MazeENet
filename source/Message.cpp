#include "Message.h"

Message::Message(uint32_t peerId, Type type, const std::string& data)
    : m_peerID(peerId)
    , m_type(type)
    , m_data(data)
{
}


uint32_t Message::GetPeerID() const
{
    return m_peerID;
}

Message::Type Message::GetType() const
{
    return m_type;
}

const std::string& Message::GetData() const
{
    return m_data;
}
