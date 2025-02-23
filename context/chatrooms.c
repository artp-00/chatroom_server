#include <context/connection.h>
#include <string.h>
#include <utils/utils.h>

struct chatroom *add_chatroom(struct chatroom *head, char *name)
{
    struct chatroom *res = xmalloc(sizeof(struct chatroom));
    res->chatroom_id = name;
    res->next = head;
    return res;
}

struct chatroom *remove_chatroom(struct chatroom *head, char *name)
{
    if (strcmp(head->chatroom_id, name) == 0)
    {
        free(head->chatroom_id);
        struct chatroom *res = head->next;
        free(head);
        return res;
    }
    return remove_chatroom(head->next, name);
}

void free_chatrooms(struct chatroom *head)
{
    if (head)
    {
        free_chatrooms(head->next);
        free(head->chatroom_id);
        free(head);
    }
}
