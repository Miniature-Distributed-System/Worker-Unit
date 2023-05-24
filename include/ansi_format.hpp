#ifndef _ANSI_FORMAT_H
#define _ANSI_FORMAT_H

enum Foreground {
    FG_RED      = 31,
    FG_GREEN    = 32,
    FG_GOLD     = 33,
    FG_BLUE     = 34,
    FG_MAGNETA  = 35,
    FG_CYAN     = 36,
    FG_GREY     = 90,
    FG_YELLLOW  = 93,
    FG_LMAGNETA = 95,
    FG_DEFAULT  = 39
};
enum Background {
    BG_RED      = 41,
    BG_GREEN    = 42,
    BG_BLUE     = 44,
    BG_DARKGREY = 100,
    BG_DEFAULT  = 49
};
enum Format {
    ALL = 0,
    BOLD = 1,
    DIM = 2,
    UNDERLINED = 4
};

#endif