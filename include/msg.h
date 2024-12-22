#pragma once
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// Типы сообщений
enum Message_type
{
    create,
    create_asw,
    exec,
    exec_asw,
    pingall,
    pingall_asw,
    error,
    die
};

// Структура, описывающая отправляемое сообщение
struct Message
{
    Message_type type; // Тип сообщения (create, exec, pingall, …)
    vector<int> data;  // Числовые данные (ID родителя, ID узла, числа для exec и т.д.)
};
