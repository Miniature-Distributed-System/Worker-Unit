#ifndef GLOBAL_OBJS_H
#define GLOBAL_OBJS_H

#include "../include/logger.hpp"
#include <unordered_map>

class Base 
{
        static std::string classID;
    public:
        virtual ~Base() = default;
        const std::string& getId() const {
            return classID;
        }
};

class GlobalObjectsManager 
{
		std::unordered_map <std::string, Base *> map;
	public:
        template<typename T>
    	void add(T &obj){
        	map[T::getId()] = &obj;
       	}
       	template<typename T>
		T& get() const {
        	return dynamic_cast<T&>(*map.at(T::getId()));
        }
};

extern GlobalObjectsManager globalObjectsManager;

#endif
