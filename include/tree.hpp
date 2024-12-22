#pragma once
#include <iostream>
#include <vector>

// Узел «общего вида»: child / sibling
struct node
{
    int ID;
    bool available = false;  // для pingall
    node *child = nullptr;   // указатель на первого потомка
    node *sibling = nullptr; // указатель на «брата»
};

// Класс дерева
class tree
{
private:
    node *root = nullptr; // корень
    int node_cnt = 0;     // количество узлов

    // --- Вспомогательные методы ---
    // Поиск узла с данным ID (возвращает указатель или nullptr)
    node *find_node(node *current, int ID);

    // Вставка нового узла (по алгоритму «в ширину»):
    //   находим первый узел, у которого ещё нет child => подвешиваем туда.
    // Если у узла уже есть child, идём BFS глубже.
    bool insert_bfs(int ID);

    // Поиск родителя через обход
    int find_parent_id(node *current, int child_id);

    // Обход для draw, bypass, и т.п.
    void draw_subtree(node *current, int level);
    void bypass_reset_subtree(node *current);
    void bypass_collect_unavailable(node *current, std::vector<int> &unavailable);

public:
    tree();
    ~tree();

    // Проверка, есть ли узел в дереве
    bool is_in_tree(int ID);

    // Вставка
    bool insert(int ID);

    // Возвращает узел (указатель) - может быть nullptr
    node *get(int ID);

    // Корень дерева
    node *get_root();

    // «Родитель» данного узла (поиск в ширину/глубину)
    // Если узел — root, или не найден, вернёт -1
    int parent_id(int child_id);

    // Вывод (визуализация дерева)
    void draw_tree();

    // Доступность узла (для pingall)
    bool is_available(int ID);
    void change_availability(int ID, bool status);

    // Сброс доступности всех узлов (ставим available = false)
    void bypass_reset(node *current);

    // Сбор всех недоступных узлов
    void bypass(node *current, std::vector<int> &unavailable);

    // Количество узлов
    int cnt();
};
