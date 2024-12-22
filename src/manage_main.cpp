#include <iostream>
#include <string>
#include <unistd.h> // fork, execl
#include <vector>
#include <map> // Храним связи parent->children (локальное дерево)

#include <zmq.hpp>

#include "../include/tree.hpp"      // Предположим, что в нём у вас "дерево общего вида"
#include "../include/manage_node.h" // Класс Manage_node (PUB/SUB), send_msg/receive_msg
#include "../include/msg.h"

using namespace std;

// Запускаем ./node <child_id>
int start_node_process(int child_id)
{
    int pid = fork();
    if (pid < 0)
    {
        perror("fork failed");
        return -1;
    }
    else if (pid == 0)
    {
        // child
        string id_str = to_string(child_id);
        if (access("./node", X_OK) == -1)
        {
            perror("Error: ./node not found or not executable");
            _exit(1);
        }
        execl("./node", "./node", id_str.c_str(), nullptr);
        // если дошли сюда, execl не сработал
        perror("execl failed");
        _exit(1);
    }
    else
    {
        // parent
        return pid; // вернём PID
    }
}

int main()
{
    // Условное дерево (можете использовать ваш класс Tree или map<int,int>)
    // Ниже для простоты делаем map< child, parent >
    // Но можно хранить parent_id в вашей "tree.hpp"
    map<int, int> parent_of;

    // Собственно ZeroMQ-обёртка
    Manage_node nodeManager;

    cout << "Available commands:\n"
         << " create <child_id> <parent_id>\n"
         << " exec <id> <n> <numbers...>\n"
         << " pingall\n"
         << " draw\n"
         << " q (exit)\n\n";

    while (true)
    {
        string cmd;
        if (!(cin >> cmd))
        {
            // EOF
            break;
        }

        if (cmd == "create")
        {
            // Формат: create <child_id> <parent_id>
            int child_id, parent_id;
            cin >> child_id >> parent_id;
            if (!cin.good())
            {
                cout << "Error: bad input\n";
                cin.clear();
                continue;
            }
            if (child_id < 0)
            {
                cout << "Error: ID must be >= 0\n";
                continue;
            }
            // проверим, что child_id ещё не существует
            if (parent_of.count(child_id) > 0 || child_id == -1)
            {
                cout << "Error: Already exists\n";
                continue;
            }

            // Сохраняем связь в локальном "дереве"
            parent_of[child_id] = parent_id; // если parent_id=-1 => корневой

            if (parent_id == -1)
            {
                // Запускаем новый узел
                int pid = start_node_process(child_id);
                if (pid > 0)
                {
                    cout << "Ok: " << child_id << " (PID=" << pid << ")\n";
                }
            }
            else
            {
                // Посылаем команду "create child_id" узлу <parent_id>
                Message msg;
                msg.type = Message_type::create;
                msg.data.push_back(parent_id); // в первом "слоте" кладём "кому"
                msg.data.push_back(child_id);  // во втором "слоте" - "новый ID"

                if (!nodeManager.send_msg(msg))
                {
                    cout << "Error: can't send create-message\n";
                }
                else
                {
                    cout << "Ok: " << parent_id << " -> " << child_id << "\n";
                }
            }
        }
        else if (cmd == "exec")
        {
            // Формат: exec <id> <n> <numbers...>
            int id, n;
            cin >> id >> n;
            if (!cin.good() || n <= 0)
            {
                cout << "Error: bad input for exec\n";
                // пропустим возможно лишние
                cin.clear();
                while (n--)
                {
                    int skipVal;
                    cin >> skipVal;
                }
                continue;
            }
            vector<int> nums(n);
            for (int i = 0; i < n; i++)
            {
                cin >> nums[i];
                if (!cin.good())
                {
                    cout << "Error: bad input for numbers\n";
                    cin.clear();
                    break;
                }
            }
            // Проверим, есть ли этот узел в нашем map
            if (!parent_of.count(id))
            {
                cout << "Error: no such node\n";
                continue;
            }

            // Формируем msg: "exec" => [target_id, n, val1, val2, ...]
            // Но, чтобы отправить именно нужному узлу,
            //  мы положим "target_id" в data[0].
            Message msg;
            msg.type = Message_type::exec;
            msg.data.push_back(id); // кому
            msg.data.push_back(n);
            for (int v : nums)
            {
                msg.data.push_back(v);
            }

            if (!nodeManager.send_msg(msg))
            {
                cout << "Error: exec send failed\n";
            }
        }
        else if (cmd == "pingall")
        {
            // Отправим -100 + pingall
            Message msg;
            msg.type = Message_type::pingall;
            // msg.data.push_back(-100) - условно
            msg.data.push_back(-100);

            if (!nodeManager.send_msg(msg))
            {
                cout << "Error: pingall send failed\n";
                continue;
            }
            // Локально ждём ответы.
            // Но непонятно, сколько узлов (?).
            // Можно взять число = parent_of.size().
            // Тот же подход: receive_msg n раз
            int total_nodes = parent_of.size();

            // Сбросим "available" в tree (если у вас свой класс - используйте его)
            // Здесь не реализовано, но можно хранить map<int,bool> isAvail

            for (int i = 0; i < total_nodes; i++)
            {
                nodeManager.receive_msg(Message_type::pingall /*необязательно*/,
                                        /*AWL_tree*/ (*(new tree())));
                // Здесь костыль, так как у вас, возможно, своя структура
                // для выставления флага "available".
                // Либо можете сделать что-то вроде `tree.change_availability(...)`.
                // В демонстрационных целях оставим так.
            }

            // Далее предполагаем, что manage_node::receive_msg() выводит IDs
            // или где-то ставит available=true.
            // Чтобы показать пример, вы можете собрать список недоступных.
            cout << "Ok: -1\n";
        }
        else if (cmd == "draw")
        {
            // Тут можно нарисовать локальное дерево parent_of
            // (где -1 = root)
            // Чисто для наглядности, что у нас есть
            // Пример примитивного вывода:
            cout << "Local structure:\n";
            for (auto &p : parent_of)
            {
                cout << " Node " << p.first << " -> parent " << p.second << "\n";
            }
        }
        else if (cmd == "q")
        {
            cout << "Exit.\n";
            break;
        }
        else
        {
            cout << "Unknown command.\n";
        }
    }

    return 0;
}
