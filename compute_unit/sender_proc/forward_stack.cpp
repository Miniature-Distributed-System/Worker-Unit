#include "forward_stack.hpp"
#include "../include/debug_rp.hpp"

std::list<fwd_stack*> senderStack;

int push_forward_stk(std::string data, std::string tableID, int statusCode)
{
    struct fwd_stack *item = new fwd_stack;
    item->statusCode = statusCode;
    item->data = data;
    item->tableID = tableID;
    senderStack.push_back(item);
    DEBUG_MSG(__func__, "fwd stack current count:", senderStack.size());
    return 0;
}

fwd_stack* pop_forward_stk(void)
{
    struct fwd_stack *item;
    if(senderStack.empty())
        return NULL;
    item = senderStack.front();
    senderStack.pop_front();
    DEBUG_MSG(__func__, "fwd stack current count:", senderStack.size());
    
    return item;
}

fwd_stack* show_front(void)
{
    struct fwd_stack *item;
    item = senderStack.front();

    return item;
}