#include "forward_stack.hpp"
#include "../include/packet.hpp"
#include "../include/debug_rp.hpp"

ForwardStack fwdStack;
AwaitStack awaitStack;

/* pushToForwardStack(): maps the data passed to an fwd_stack_bundle() struct and then pushes the item
 * to the rear or end of stack. This item wil be popped out last. This is used for all normal packets.
*/
int ForwardStack::pushToForwardStack(std::string data, std::string tableID, 
                int statusCode, int priority)
{
    struct fwd_stack_bundle *item = new fwd_stack_bundle;
    item->statusCode = statusCode;
    item->priority = priority;
    item->data = data;
    item->tableID = tableID;
    senderStack.push_back(item);
    DEBUG_MSG(__func__, "fwd stack current count:", senderStack.size());
    return 0;
}

/* pushFrontForwardStack(): maps the data passed to an fwd_stack_bundle() struct and then pushes the item
 * to the forward or top of stack. This item wil be popped out first. This is used mostly for the seize opertaion
 * where the server needs to be told that the compute node will not take any more packets. It can also be used for
 * sending highest priority items to server.
*/
int ForwardStack::pushFrontForwardStack(std::string data, std::string tableID,
                int statusCode, int priority)
{
    struct fwd_stack_bundle *item = new fwd_stack_bundle;
    item->statusCode = statusCode;
    item->priority = priority;
    item->data = data;
    item->tableID = tableID;
    senderStack.push_front(item);
    DEBUG_MSG(__func__, "fwd stack current count:", senderStack.size());
    return 0;
}

/* popForwardStack(): pops the latest item from sender stack.
 * The items popped are checked if they are acknoledgeable packets or not, if they are acknowledgable packets
 * then the packets are sent to the await stack (refer RDT models). The packets are copied to object and sent as value.
 * The objects are allocated and now sent as stack items the reson behing this is stack needs to be used judiciously
 * compared to the heap, the data feilds hold large data and needs more space so only use it when required.
*/
fwd_stack_bundle ForwardStack::popForwardStack(void)
{
    struct fwd_stack_bundle *item, exportItem;

    if(senderStack.empty()){
        return exportItem;
    }

    item = senderStack.front();
    if(item->statusCode == INTR_SEND || item->statusCode == FRES_SEND)
    {
        //Check if await is free if not then dont send ackable packets
        if(awaitStack.isAwaitStackFree()){
            //Send ackable packet and keep track of its acks in await stack
            senderStack.pop_front();
            awaitStack.pushToAwaitStack(item);
            exportItem.copyPointerObject(item);
        } else {
            /* if above can't be achived then just send non ackable 
                packets to server */
            item = fwdStack.getNonackableItem();
            exportItem.copyPointerObject(item);
            delete item;
        }
    } else {

        senderStack.pop_front();
    }
    DEBUG_MSG(__func__, "Forward stack current count:", senderStack.size());
    
    return exportItem;
}

// getForwardStackSize(): gets the size of forward stack(aka sender stack).
int ForwardStack::getForwardStackSize(void)
{
    return senderStack.size();
}

// isForwardStackEmpty(): returns bool true if forward stack is empty, else false.
int ForwardStack::isForwardStackEmpty()
{
    struct fwd_stack_bundle *item;
    item = senderStack.front();
    if(item)
        return 0;
    return 1;
}

/* getNonackableItem(): returns packets that dont need acknowledgement from server.
 * These packets are used when the await stack is full and the packets in the await stack is not yet acknowledged.
 * Until all items in the await stack is acknowledged this method is called, in place of popForwardStack().
 * Therefore in every send we are still emptying the sender stack and is non is present we just give out null.
*/
fwd_stack_bundle* ForwardStack::getNonackableItem()
{
    for(auto i = senderStack.begin(); i != senderStack.end(); i++){
        struct fwd_stack_bundle* item = *i;
        if(item->statusCode != INTR_SEND || item->statusCode != FRES_SEND){
            senderStack.remove(item);
            return item;
        }
    }
    return NULL;
}

/* pushToAwaitStack(): this method pushes the acknowledgable items to the await stack.
 * The packets are pushed if await stack still has space for more packets. It returns -1 if it cant push any more
 * items to stack. 
*/
int AwaitStack::pushToAwaitStack(struct fwd_stack_bundle *item)
{
    struct await_stack_bundle *awtItem;
    if(index >= TOTAL_DIFFRED_PACKETS)
        return -1;
    
    awtItem = new await_stack_bundle;
    awtItem->fwd_element = item;
    awaitStack[index] = awtItem;
    index++;

    return 0;
}
/* matchItemWithAwaitStack(): This matches packet id and status with items in its await stack to check if the packet it
 * recevied by the server is an acknowlgement to the packet the compute node sent ot the server.
 * It matches the packets depeneding on status codes. If it finds all packets in its stack are acknowledged then
 * it calls cleanupAwaitStack() method.
*/
int AwaitStack::matchItemWithAwaitStack(int statusCode, std::string tableID)
{
    int i;
    for(i = 0; i < TOTAL_DIFFRED_PACKETS; i++){
        if(awaitStack[i]){
            if(awaitStack[i]->fwd_element->tableID == tableID)
            {
                switch(statusCode)
                {
                    case SP_INTR_ACK:
                        if(awaitStack[i]->fwd_element->statusCode == INTR_SEND && !awaitStack[i]->itemAcked)
                            goto ack_packet;
                        break;
                    case SP_FRES_ACK:
                        if(awaitStack[i]->fwd_element->statusCode == FRES_SEND && !awaitStack[i]->itemAcked)
                            goto ack_packet;
                        break;
                    default:
                        DEBUG_ERR(__func__,"unknown statuscode:", statusCode);
                        return -1;
                }
            }
        }
    }
    DEBUG_MSG(__func__, "duplicate packet received");
    return -1;

ack_packet:
    awaitStack[i]->itemAcked = true;
    ackedPackets++;
    if(ackedPackets == TOTAL_DIFFRED_PACKETS)
        cleanupAwaitStack();
    return 0;
}

// cleanupAwaitStack(): Internal method which cleanups the await stack and resets the flags 
void AwaitStack::cleanupAwaitStack()
{
    for(int i = 0; i <= TOTAL_DIFFRED_PACKETS; i++)
    {
        delete awaitStack[i]->fwd_element;
        awaitStack[i]->itemAcked = false;
        ackedPackets = 0;
        index = 0;
    }
}

// isAwaitStackFree(): checks if await stack is free or not and returns bool true if it still has space.
bool AwaitStack::isAwaitStackFree()
{
    if(index <= TOTAL_DIFFRED_PACKETS){
        DEBUG_MSG(__func__, "Await stack is free");
        return true;
    }
    return false;
}