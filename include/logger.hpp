#ifndef _LOGGER_H
#define _LOGGER_H

#include <string>
#include <sstream>
#include <iostream>
#include "ansi_format.hpp"

class Log{
        template<class ... Args>
        std::ostringstream DEBUG(Args && ... args){
            int i = 0;
            std::ostringstream outString;
            ([&]{
                ++i;
                outString << args;
            }(), ...);
            return outString;
        };
        const std::string currentDateTime() {
            time_t     now = time(0);
            struct tm  tstruct;
            char       buf[20];
            tstruct = *localtime(&now);
            strftime(buf, sizeof(buf), "[%d-%Y.%X]", &tstruct);

            return buf;
        };
        std::string colorCoder(std::string outString, Format fCode, Foreground fgCode, Background bgCode){
            return "\033[" + std::to_string(fCode) + ";" +std::to_string(fgCode) + ";" + std::to_string(bgCode) + "m" 
                + outString + "\033[0m";
        };

    public:
        template<typename... Args>
        void schedINFO(std::string fun_name, Args... args){
            std::ostringstream ostring;
            ostring << colorCoder(currentDateTime(), ALL, FG_YELLLOW, BG_DARKGREY) << 
                colorCoder(" I ", BOLD, FG_GREEN, BG_DEFAULT) <<
                colorCoder(fun_name, BOLD, FG_GOLD, BG_DEFAULT) <<
                colorCoder(": " + DEBUG(args...).str(), ALL, FG_GOLD, BG_DEFAULT) << std::endl;
            std::cout << ostring.str();
        };

        template<typename... Args>
        void schedERR(std::string fun_name, Args... args){
            std::ostringstream ostring;
            ostring << colorCoder(currentDateTime(), ALL, FG_YELLLOW, BG_DARKGREY) <<
                colorCoder(" E ", BOLD, FG_RED, BG_DARKGREY) <<
                colorCoder(fun_name, BOLD, FG_RED, BG_DARKGREY) <<
                colorCoder(": " + DEBUG(args...).str(), ALL, FG_RED, BG_DARKGREY) << std::endl;
            std::cout << ostring.str();
        };
        
        template<typename... Args>
        void dataProcInfo(std::string fun_name, Args... args){
            std::ostringstream ostring;
            ostring << colorCoder(currentDateTime(), ALL, FG_YELLLOW, BG_DARKGREY) << 
                colorCoder(" I ", BOLD, FG_GREEN, BG_DEFAULT) <<
                colorCoder(fun_name, BOLD, FG_CYAN, BG_DEFAULT) <<
                colorCoder(": " + DEBUG(args...).str(), ALL, FG_CYAN, BG_DEFAULT) << std::endl;
            std::cout << ostring.str();
        };

        template<typename... Args>
        void taskPoolInfo(std::string fun_name, Args... args){
            std::ostringstream ostring;
            ostring << colorCoder(currentDateTime(), ALL, FG_YELLLOW, BG_DARKGREY) << 
                colorCoder(" I ", BOLD, FG_GREEN, BG_DEFAULT) <<
                colorCoder(fun_name, BOLD, FG_MAGNETA, BG_DEFAULT) <<
                colorCoder(": " + DEBUG(args...).str(), ALL, FG_MAGNETA, BG_DEFAULT) << std::endl;
            std::cout << ostring.str();
        };

        template<typename... Args>
        void info(std::string fun_name, Args... args){
            std::ostringstream ostring;
            ostring << colorCoder(currentDateTime(), ALL, FG_YELLLOW, BG_DARKGREY) <<
                colorCoder(" I ", BOLD, FG_GREEN, BG_DEFAULT) << 
                colorCoder(fun_name, BOLD, FG_DEFAULT, BG_DEFAULT) << 
                ": " << DEBUG(args...).str() << std::endl;
            std::cout << ostring.str();
        };

        template<typename... Args>
        void debug(std::string fun_name, Args... args){
            #ifdef ENABLE_DEBUG
            std::ostringstream ostring;
            ostring << colorCoder(currentDateTime(), ALL, FG_YELLLOW, BG_DARKGREY) <<
                colorCoder(" D ", BOLD, FG_YELLLOW, BG_DEFAULT) << 
                colorCoder(fun_name, BOLD, FG_YELLLOW, BG_DEFAULT) << 
                colorCoder(": " + DEBUG(args...).str(), ALL, FG_YELLLOW, BG_DEFAULT) << std::endl;
            std::cout << ostring.str();
            #endif
        };

        template<typename... Args>
        void error(std::string fun_name, Args... args){
            std::ostringstream ostring;
            ostring << colorCoder(currentDateTime(), ALL, FG_YELLLOW, BG_DARKGREY) <<
                colorCoder(" E ", BOLD, FG_RED, BG_DEFAULT) <<
                colorCoder(fun_name, BOLD, FG_RED, BG_DEFAULT) <<
                colorCoder(": " + DEBUG(args...).str(), ALL, FG_RED, BG_DEFAULT) << std::endl;
            std::cout << ostring.str();
        };  
};

#endif