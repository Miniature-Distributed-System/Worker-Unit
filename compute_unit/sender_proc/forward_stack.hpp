#ifndef FWDSTK_H
#define FWDSTK_H

#include <iostream>
#include <list>

struct fwd_stack {
    int statusCode;
    std::string tableID;
    std::string data;
};

extern std::list<fwd_stack*> senderStack;

int push_forward_stk(std::string, std::string, int);
fwd_stack* pop_forward_stk(void);
fwd_stack* show_front(void);
#endif