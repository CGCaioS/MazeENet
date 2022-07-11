#pragma once

#define TIMEOUT_MS 5000
#define RELIABLE_CHANNEL 0
#define UNRELIABLE_CHANNEL 1
#define NUM_CHANNELS 2

enum class DeliveryType {
    RELIABLE,
    UNRELIABLE
};

#define MAX_MESSAGE_LEN 100