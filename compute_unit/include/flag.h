#ifndef BITS_H
#define BITS_H

class Flag{
    std::uint8_t bit;
    public:
    void initFlag(bool);
    void setFlag();
    void resetFlag();
    bool checkFlag();
};
void Flag::initFlag(bool state = false){
    if(state)
        bit = 1;
    else 
        bit = 0;
}
void Flag::setFlag(){
    bit = 1;
}

void Flag::resetFlag(){
    bit = 0;
}

bool Flag::checkFlag(){
    if(bit)
        return true;
    return false;
}

#endif