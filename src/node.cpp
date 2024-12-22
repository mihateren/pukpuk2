#include <iostream>
#include <string>
#include <zmq.hpp>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <cstdlib> // atoi, exit
#include "../include/msg.h"

using namespace std;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " <node_id>\n";
        return 1;
    }
    // Наш собственный ID (строка)
    string my_id_str = argv[1];

    zmq::context_t context;

    // PUB-сокет => отправляем в tcp://127.0.0.1:5556 (manage_node там bind)
    zmq::socket_t publisher(context, zmq::socket_type::pub);
    publisher.connect("tcp://127.0.0.1:5556");

    // SUB-сокет => получаем из tcp://127.0.0.1:5555 (manage_node там bind)
    zmq::socket_t sub(context, zmq::socket_type::sub);
    sub.connect("tcp://127.0.0.1:5555");

    // Подписываемся на СВОЙ ID (чтобы получать команды "my_id create <child>", "my_id exec ...")
    sub.set(zmq::sockopt::subscribe, my_id_str);

    // Подписываемся на "-100" для pingall
    sub.set(zmq::sockopt::subscribe, "-100");

    // Отправим «сигнал», что узел запустился (опционально)
    {
        zmq::message_t ready_msg(my_id_str);
        publisher.send(ready_msg, zmq::send_flags::none);
    }

    // Если родитель погибнет — убить и этот процесс
    prctl(PR_SET_PDEATHSIG, SIGKILL);

    while (true)
    {
        // 1) Получаем первую часть (ID, кому адресовано, либо -100)
        zmq::message_t first_part;
        auto res = sub.recv(first_part, zmq::recv_flags::none);
        if (!res.has_value())
        {
            // Ничего не пришло
            continue;
        }
        string target_id = first_part.to_string();

        // Если это "-100", значит pingall
        // Если это наш ID, значит команда лично нам
        // Иначе пропускаем
        if (target_id == "-100")
        {
            // Ждём вторую часть (тип)
            zmq::message_t second_part;
            res = sub.recv(second_part, zmq::recv_flags::none);
            if (!res.has_value())
            {
                continue;
            }
            string cmd_str = second_part.to_string();
            if (cmd_str == to_string(Message_type::pingall))
            {
                // Отправим свой ID
                zmq::message_t resp(my_id_str);
                publisher.send(resp, zmq::send_flags::none);
            }
            // иначе игнор
        }
        else if (target_id == my_id_str)
        {
            // Команда для нас
            // Читаем вторую часть (тип команды)
            zmq::message_t second_part;
            res = sub.recv(second_part, zmq::recv_flags::none);
            if (!res.has_value())
            {
                continue;
            }
            string cmd_str = second_part.to_string();

            // CREATE
            if (cmd_str == to_string(Message_type::create))
            {
                // Третья часть: new_child_id
                zmq::message_t new_child_msg;
                res = sub.recv(new_child_msg, zmq::recv_flags::none);
                if (!res.has_value())
                    continue;

                string new_id_str = new_child_msg.to_string();

                // Порождаем узел
                int pid = fork();
                if (pid == 0)
                {
                    // child
                    execl("./node", "./node", new_id_str.c_str(), NULL);
                    perror("execl failed");
                    _exit(1);
                }
                else if (pid > 0)
                {
                    // Выводим (в саму консоль manage_main это может тоже попасть
                    //   если оно перенаправлено)
                    cout << "Ok:" << my_id_str << ": " << pid << "\n";
                }
                else
                {
                    perror("fork failed");
                }
            }
            // EXEC
            else if (cmd_str == to_string(Message_type::exec))
            {
                // Следующая часть: n
                zmq::message_t n_msg;
                res = sub.recv(n_msg, zmq::recv_flags::none);
                if (!res.has_value())
                    continue;
                int n = stoi(n_msg.to_string());

                long long sum = 0;
                for (int i = 0; i < n; i++)
                {
                    zmq::message_t num_msg;
                    res = sub.recv(num_msg, zmq::recv_flags::none);
                    if (!res.has_value())
                        break;
                    long long val = stoll(num_msg.to_string());
                    sum += val;
                }
                // Выведем ровно один раз
                cout << "Ok:" << my_id_str << ": " << sum << "\n";
            }
            // Иначе — игнорируем
        }
        else
        {
            // target_id != my_id_str и != "-100" => эта команда не для нас
            // Просто «считываем» вторую часть (тип), но игнорируем
            zmq::message_t discard;
            res = sub.recv(discard, zmq::recv_flags::none);
            // Если тип — create/exec/pingall, вероятно будет ещё части?
            // Но у нас нет универсального способа прочитать их все,
            // можно сделать "drain" цикл. В любом случае, раз сообщение не для нас —
            // мы его игнорируем.
        }
    }

    return 0;
}
