#define _ENUM_TO_STR(name)      #name
#define ENUM_TO_STR(name)       _ENUM_TO_STR(name)

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

#define MAP_VALUE(x, in_min, in_max, out_min, out_max) \
    ((x) - (in_min)) * ((out_max) - (out_min)) / ((in_max) - (in_min)) + (out_min)