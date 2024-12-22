#include "../include/tree.hpp"
#include <iostream>
#include <queue>
using namespace std;

tree::tree()
{
    root = nullptr;
    node_cnt = 0;
}

tree::~tree()
{
    // При желании можно рекурсивно удалить все узлы.
    // В вашем примере этот деструктор пуст.
}

// Поиск узла с данным ID (простой обход в ширину)
node *tree::find_node(node *current, int ID)
{
    if (!current)
        return nullptr;

    // BFS (очередь)
    queue<node *> q;
    q.push(current);
    while (!q.empty())
    {
        node *front = q.front();
        q.pop();
        if (front->ID == ID)
        {
            return front;
        }
        // Добавляем всех детей (child + sibling-цепочку)
        node *c = front->child;
        while (c)
        {
            q.push(c);
            c = c->sibling;
        }
    }
    // Не нашли
    return nullptr;
}

// Проверка, есть ли узел в дереве
bool tree::is_in_tree(int ID)
{
    if (!root)
        return false;
    return (find_node(root, ID) != nullptr);
}

// --- Вставка нового узла ---
bool tree::insert(int ID)
{
    // Если уже есть
    if (is_in_tree(ID))
    {
        return false;
    }
    // Если нет узлов, создаём root
    if (!root)
    {
        node *newNode = new node;
        newNode->ID = ID;
        // available = false по умолчанию
        root = newNode;
        node_cnt = 1;
        cout << "Ok: " << ID << "\n";
        return true;
    }
    // Иначе вставляем «в ширину»
    bool res = insert_bfs(ID);
    if (res)
    {
        cout << "Ok: " << ID << "\n";
    }
    return res;
}

// Реализация вставки по BFS:
// ищем первую вершину, у которой нет child => делаем child = newNode
// Если у неё уже есть child, всё равно идём дальше по BFS (т.е. спускаемся глубже).
bool tree::insert_bfs(int ID)
{
    node *newNode = new node;
    newNode->ID = ID;

    queue<node *> q;
    q.push(root);
    while (!q.empty())
    {
        node *front = q.front();
        q.pop();

        // Если у front ещё нет child — подвесим нового
        if (!front->child)
        {
            front->child = newNode;
            node_cnt++;
            return true;
        }
        else
        {
            // Иначе у него есть child => посмотрим siblings + уйдём глубже
            // Сначала пушим child, и все child->sibling
            node *c = front->child;
            while (c)
            {
                q.push(c);
                c = c->sibling;
            }
        }
    }

    // Теоретически мы всегда должны найти вершину без child,
    // но если вдруг нет, вернём false.
    delete newNode;
    return false;
}

// Вернуть указатель на узел (или nullptr)
node *tree::get(int ID)
{
    if (!root)
        return nullptr;
    return find_node(root, ID);
}

// Получить корень дерева
node *tree::get_root()
{
    return root;
}

// Поиск родителя (child_id) обходом в ширину
int tree::find_parent_id(node *current, int child_id)
{
    if (!current)
        return -1;

    queue<node *> q;
    q.push(current);
    while (!q.empty())
    {
        node *front = q.front();
        q.pop();

        // Пробежимся по всем "детям" front (child + siblings)
        node *c = front->child;
        while (c)
        {
            if (c->ID == child_id)
            {
                // Нашли ребенка => возвращаем front->ID
                return front->ID;
            }
            q.push(c);
            c = c->sibling;
        }
    }
    return -1;
}

// parent_id: если узел сам root, или не найден — вернёт -1
int tree::parent_id(int child_id)
{
    // Если дерево пустое или узла нет
    if (!root || !is_in_tree(child_id))
    {
        return -1;
    }
    // Если это root
    if (root->ID == child_id)
    {
        return -1; // у root нет родителя
    }
    // Иначе ищем
    return find_parent_id(root, child_id);
}

// Простой вывод дерева (child-sibling) в «повёрнутом виде»
void tree::draw_subtree(node *current, int level)
{
    if (!current)
        return;
    // Сперва рисуем sibling правее
    draw_subtree(current->sibling, level);

    // Выводим отступы
    for (int i = 0; i < level; ++i)
    {
        cout << "   ";
    }
    // Печатаем ID (+ признак доступности)
    cout << current->ID << (current->available ? "(+)" : "(-)") << "\n";

    // Затем рисуем детей глубже
    draw_subtree(current->child, level + 1);
}

void tree::draw_tree()
{
    draw_subtree(root, 0);
}

// Проверяем «available»
bool tree::is_available(int ID)
{
    node *n = get(ID);
    if (!n)
        return false;
    return n->available;
}

// Меняем доступность
void tree::change_availability(int ID, bool status)
{
    node *n = get(ID);
    if (n)
    {
        n->available = status;
    }
}

// Сброс флага available во всём поддереве
void tree::bypass_reset_subtree(node *current)
{
    if (!current)
        return;
    current->available = false;
    // Идём по sibling
    bypass_reset_subtree(current->sibling);
    // Идём по child
    bypass_reset_subtree(current->child);
}

// Обёртка
void tree::bypass_reset(node *current)
{
    bypass_reset_subtree(current);
}

// Собираем все недоступные (available = false)
void tree::bypass_collect_unavailable(node *current, vector<int> &unavailable)
{
    if (!current)
        return;
    if (!current->available)
    {
        unavailable.push_back(current->ID);
    }
    // Перебираем sibling
    bypass_collect_unavailable(current->sibling, unavailable);
    // Уходим к child
    bypass_collect_unavailable(current->child, unavailable);
}

// Обёртка
void tree::bypass(node *current, vector<int> &unavailable)
{
    bypass_collect_unavailable(current, unavailable);
}

// Количество узлов
int tree::cnt()
{
    return node_cnt;
}
