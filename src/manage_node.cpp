#include "../include/manage_node.h"

// Конструктор: создаём два сокета PUB/SUB, биндим их на 5555 и 5556
Manage_node::Manage_node()
    : publisher(context, zmq::socket_type::pub),
      sub(context, zmq::socket_type::sub)
{
    // PUB-сокет связывается с портом 5555
    publisher.bind("tcp://127.0.0.1:5555");
    // SUB-сокет — 5556
    sub.bind("tcp://127.0.0.1:5556");

    // Подписываемся на все сообщения (фильтр = "")
    sub.set(zmq::sockopt::subscribe, "");
}

// Деструктор: отключаем сокеты
Manage_node::~Manage_node()
{
    sub.disconnect("tcp://127.0.0.1:5556");
    publisher.disconnect("tcp://127.0.0.1:5555");
}

// Получаем ответы от всех нужных процессов (для pingall)
void Manage_node::receive_msg(Message_type msg_type, tree &tree)
{
    int max_attempts = 5; // Максимальное число попыток опроса
    int attempt = 0;

    while (attempt < max_attempts)
    {
        zmq::message_t reply;
        // Читаем non-blocking (dontwait)
        zmq::recv_result_t res = sub.recv(reply, zmq::recv_flags::dontwait);
        if (res.has_value())
        {
            // reply.to_string() содержит ID узла, от которого пришёл «пинг»
            string id_str = reply.to_string();
            int id = stoi(id_str);
            // Ставим у узла "available = true"
            tree.change_availability(id, true);
        }
        else
        {
            ++attempt;
            // Небольшая пауза (10 мс), чтоб не крутиться слишком быстро
            usleep(10000);
        }
    }
}

// Отправка сообщения
bool Manage_node::send_msg(Message msg)
{
    // msg.type => переводим в строку (например, "0", "2" и т.д.)
    string type_str = to_string(msg.type);

    switch (msg.type)
    {
    case Message_type::create:
    {
        // data = [parent_id, new_id]
        string parent_id_str = to_string(msg.data[0]);
        string new_id_str = to_string(msg.data[1]);

        // Отправим три части подряд:
        //   1) parent_id
        //   2) type (в виде строки)
        //   3) new_id
        zmq::message_t request1(parent_id_str);
        publisher.send(request1, zmq::send_flags::sndmore);

        zmq::message_t request2(type_str);
        publisher.send(request2, zmq::send_flags::sndmore);

        zmq::message_t request3(new_id_str);
        publisher.send(request3, zmq::send_flags::none);

        return true;
    }
    case Message_type::exec:
    {
        // Формат data: [id, n, k1, k2, ..., kn]
        string id_str = to_string(msg.data[0]);
        string n_str = to_string(msg.data[1]);

        // (1) id
        zmq::message_t request1(id_str);
        publisher.send(request1, zmq::send_flags::sndmore);

        // (2) type
        zmq::message_t request2(type_str);
        publisher.send(request2, zmq::send_flags::sndmore);

        // (3) n
        zmq::message_t request3(n_str);
        // если есть ещё данные, шлём "sndmore"
        publisher.send(request3, zmq::send_flags::sndmore);

        // (4) k1..kn
        for (int i = 0; i < msg.data[1]; i++)
        {
            string num_str = to_string(msg.data[2 + i]);
            zmq::message_t num_msg(num_str);
            // последний элемент — без sndmore
            if (i == msg.data[1] - 1)
                publisher.send(num_msg, zmq::send_flags::none);
            else
                publisher.send(num_msg, zmq::send_flags::sndmore);
        }
        return true;
    }
    case Message_type::pingall:
    {
        // data = [-100] (просто условное значение)
        string all_id = to_string(msg.data[0]);
        zmq::message_t request(all_id);
        publisher.send(request, zmq::send_flags::sndmore);

        zmq::message_t type_msg(to_string(msg.type));
        publisher.send(type_msg, zmq::send_flags::none);

        return true;
    }
    default:
        return false;
    }
    return false;
}
