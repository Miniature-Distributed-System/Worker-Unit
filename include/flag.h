#ifndef FLAG_H
#define FLAG_H
#include <atomic>

class Flag{
    bool bit = {0};
    public:
    inline void initFlag(bool);
    inline void setFlag();
    inline void resetFlag();
    inline bool isFlagSet();
};

inline void Flag::initFlag(bool state = false){
    if(state)
        bit = 1;
    else 
        bit = 0;
}
inline void Flag::setFlag(){
    bit = 1;
}

inline void Flag::resetFlag(){
    bit = 0;
}

inline bool Flag::isFlagSet(){
    if(bit)
        return true;
    return false;
}

#endif