#pragma once

#include <memory>
#include <string>
#include <vector>

class Message {

public:

    enum Type {
        CONNECT,
        DISCONNECT,
        DATA
    };
    
    Message(uint32_t peerId, Type type, const std::string& data);

    uint32_t GetPeerID() const;
    Type GetType() const;

    const std::string& GetData() const;

private:  
    
    uint32_t m_peerID;
    Type m_type;

    std::string m_data;
    
};
