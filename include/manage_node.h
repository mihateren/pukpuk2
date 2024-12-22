#pragma once
#include <zmq.hpp>
#include <string>
#include <vector>
#include <unistd.h>

#include "msg.h"
#include "tree.hpp"

// Класс, управляющий отправкой/получением сообщений (PUB/SUB)
class Manage_node
{
private:
    zmq::context_t context;
    zmq::socket_t publisher; // PUB
    zmq::socket_t sub;       // SUB

public:
    Manage_node();
    ~Manage_node();

    // Читаем ответы (например, при pingall)
    void receive_msg(Message_type msg_type, tree &tree);

    // Отправляем Message с типом (create, exec, pingall, etc.)
    bool send_msg(Message msg);
};
