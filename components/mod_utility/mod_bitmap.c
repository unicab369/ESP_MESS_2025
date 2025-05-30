#include "mod_bitmap.h"


const uint8_t FONT_7x5[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x00, 0x08, 0x14, 0x22, 0x41}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x41, 0x22, 0x14, 0x08, 0x00}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x01, 0x01}, // F
    {0x3E, 0x41, 0x41, 0x51, 0x32}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x00, 0x7F, 0x41, 0x41}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 
    {0x41, 0x41, 0x7F, 0x00, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x08, 0x14, 0x54, 0x54, 0x3C}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x00, 0x7F, 0x10, 0x28, 0x44}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // }
    {0x08, 0x08, 0x2A, 0x1C, 0x08}  // ~
};


const uint8_t ICONS_8x8[][8] = {
    {0xff, 0x85, 0x89, 0x91, 0x91, 0x89, 0x85, 0xff}, //0 email
    {0x3c, 0x5e, 0xa3, 0x03, 0x03, 0xe3, 0x7e, 0x3c}, //1 github
    {0x80, 0xc0, 0xe6, 0xef, 0xef, 0xe6, 0xc0, 0x80}, //2 user
    {0xf0, 0xfe, 0xf1, 0x91, 0x91, 0xf1, 0xfe, 0xf0}, //3 lock, password
    {0x18, 0x7e, 0x7e, 0xe7, 0xe7, 0x7e, 0x7e, 0x18}, //4 cog, settings
    {0x3c, 0x42, 0x81, 0x9d, 0x91, 0x91, 0x42, 0x3c}, //5 clock
    {0x1e, 0x33, 0x21, 0x21, 0x33, 0x7e, 0xe0, 0xc0}, //6 search
    {0x7e, 0x3f, 0x15, 0x15, 0x15, 0x1d, 0x1f, 0x0e}, //7 message
    {0x00, 0x42, 0x24, 0x18, 0xff, 0x99, 0x66, 0x00}, //8 bluetooth
    {0x00, 0x3c, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e},//9 battery 100%
    {0x00, 0x3c, 0x7e, 0x42, 0x7e, 0x7e, 0x7e, 0x7e},//10 battery 75%
    {0x00, 0x3c, 0x7e, 0x42, 0x42, 0x7e, 0x7e, 0x7e},//11 battery 50%
    {0x00, 0x3c, 0x7e, 0x42, 0x42, 0x42, 0x7e, 0x7e},//12 battery 25%
    {0x00, 0x3c, 0x7e, 0x42, 0x42, 0x42, 0x42, 0x7e},//13 battery 0%
    {0xe7, 0x81, 0x81, 0x00, 0x00, 0x81, 0x81, 0xe7}, //14 full screen
    {0x24, 0x24, 0xe7, 0x00, 0x00, 0xe7, 0x24, 0x24}, //15 small screen
    {0x02, 0xfa, 0x8b, 0xfb, 0xfb, 0x8b, 0xfa, 0x02}, //16 trash, remove, delete
    {0x7c, 0x82, 0x83, 0x82, 0xb2, 0xb3, 0x82, 0x7c}, //17 calendar
    {0x7e, 0x81, 0x81, 0x9d, 0x98, 0x94, 0x82, 0x71}, //18 minimize
    {0x7e, 0x81, 0x81, 0x91, 0x88, 0x85, 0x83, 0x77}, //19 maximize
    {0x7c, 0x44, 0xe7, 0xa5, 0xa5, 0xe7, 0x44, 0x7c}, //20 printer
    {0x00, 0x18, 0x18, 0x24, 0x24, 0xc3, 0xc3, 0x00}, //21 share
    {0x1c, 0x3e, 0x73, 0xe1, 0xe1, 0x73, 0x3e, 0x1c}, //22 map
    {0xd8, 0x88, 0xde, 0xda, 0xda, 0xde, 0x88, 0xd8}, //23 case
    {0xe0, 0xfc, 0xfe, 0xa3, 0xfe, 0xfc, 0xe0, 0x00}, //24 warning
    {0x3c, 0x7e, 0xe7, 0xe7, 0xe7, 0xe7, 0x7e, 0x3c}, //25 error
    {0x3c, 0x42, 0x81, 0x99, 0x99, 0x81, 0x42, 0x3c}, //26 record
    {0xff, 0xff, 0xff, 0x7e, 0x7e, 0x3c, 0x3c, 0x18}, //27 play
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, //28 stop
    {0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00}, //29 pause
    {0xff, 0x7e, 0x3c, 0x18, 0xff, 0x7e, 0x3c, 0x18}, //30 forward
    {0x18, 0x3c, 0x7e, 0xff, 0x18, 0x3c, 0x7e, 0xff}, //31 backward
    {0x07, 0x0f, 0x1f, 0xff, 0xff, 0x1f, 0x0f, 0x07}, //32 filter
    {0x07, 0x09, 0x11, 0xe1, 0xe1, 0x11, 0x09, 0x07}, //33 filter outline
    {0x3c, 0x7e, 0xff, 0x00, 0x18, 0x42, 0x3c, 0x00}, //34 sound 100%
    {0x3c, 0x7e, 0xff, 0x00, 0x18, 0x00, 0x00, 0x00}, //35 sound 50%
    {0x3c, 0x7e, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00}, //36 sound 0%
    {0x01, 0x02, 0x3c, 0x7e, 0xff, 0x20, 0x40, 0x80}, //37 sound off
    {0xe0, 0x88, 0x90, 0xbf, 0xbf, 0x90, 0x88, 0xe0}, //38 download
    {0xe0, 0x84, 0x82, 0xbf, 0xbf, 0x82, 0x84, 0xe0}, //39 upload
    {0x7e, 0x81, 0xa1, 0xa1, 0xaf, 0xa9, 0x8a, 0x7c}, //40 text file
    {0xf0, 0x88, 0xa0, 0x92, 0x49, 0x05, 0x11, 0x0f}, //41 link
    {0xe0, 0x90, 0x88, 0x44, 0x26, 0x19, 0x0a, 0x04}, //42 pencil
    {0xfe, 0x7f, 0x3f, 0x1f, 0x1f, 0x3f, 0x7f, 0xfe}, //43 bookmark
    {0x00, 0x98, 0xdc, 0xfe, 0x7f, 0x3b, 0x19, 0x18}, //44 flash, lighting
    {0x7e, 0x81, 0x99, 0xa5, 0xa5, 0xa5, 0xa1, 0x9e}, //45 attach
    {0x1e, 0x3f, 0x7f, 0xfe, 0xfe, 0x7f, 0x3f, 0x1e}, //46 heart
    {0x1e, 0x21, 0x41, 0x82, 0x82, 0x41, 0x21, 0x1e}, //47 heart outline
    {0x72, 0x77, 0x77, 0xff, 0xff, 0x77, 0x77, 0x27}, //48 direction
    {0x18, 0x24, 0x42, 0x5a, 0x5a, 0x42, 0x24, 0x18}, //49 eye, visible
    {0x06, 0x09, 0x11, 0xff, 0xff, 0x11, 0x09, 0x06}, //50 antenna
    {0x00, 0xe0, 0x00, 0xf0, 0x00, 0xfc, 0x00, 0xff}, //51 mobile network 100%
    {0x00, 0xe0, 0x00, 0xf0, 0x00, 0xfc, 0x00, 0x80}, //52 mobile network 75%
    {0x00, 0xe0, 0x00, 0xf0, 0x00, 0x80, 0x00, 0x80}, //53 mobile network 50%
    {0x00, 0xe0, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80}, //54 mobile network 25%
    {0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80}, //55 mobile network 0%
    {0x20, 0x40, 0xff, 0x00, 0x00, 0xff, 0x02, 0x04}, //56 sync
    {0x7e, 0x7e, 0x7e, 0x7e, 0x3c, 0x18, 0x3c, 0x7e}, //57 video
    {0x02, 0x09, 0x25, 0x95, 0x95, 0x25, 0x09, 0x02}, //58 wifi
    {0x70, 0x88, 0x84, 0x84, 0xa4, 0x98, 0x90, 0x60}, //59 cloud
    {0x01, 0x0a, 0xf8, 0xfb, 0xf8, 0x0a, 0x01, 0x00}, //60 flashlight on
    {0x00, 0x08, 0xf8, 0x88, 0xf8, 0x08, 0x00, 0x00}, //61 flashlight off
    {0x70, 0x4e, 0x41, 0xc1, 0xc1, 0x41, 0x4e, 0x70}, //62 bell
    {0xf0, 0xfe, 0xf1, 0x91, 0x91, 0xf1, 0xf2, 0xf0}, //63 unlock
    {0x3c, 0x00, 0xff, 0x81, 0x81, 0xff, 0x00, 0x3c}, //64 vibrate
    {0x7e, 0x81, 0x91, 0xa1, 0x91, 0x89, 0x84, 0x72}, //65 checked
    {0x7e, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x7e}, //66 unchecked
    {0x54, 0x00, 0xff, 0x81, 0x81, 0xff, 0x00, 0x54}, //67 chip
    {0x48, 0x48, 0xfd, 0x82, 0x82, 0xfd, 0x48, 0x48}, //68 bug
    {0xff, 0x89, 0xe9, 0xa9, 0xa9, 0xe9, 0x8a, 0xfc}, //69 save
    {0xfe, 0xa2, 0x92, 0x8a, 0x8a, 0xcc, 0x28, 0x18}, //70 open
    {0xc0, 0xe0, 0x70, 0x3e, 0x19, 0x11, 0x10, 0x0c}, //71 tool
    {0xff, 0xdd, 0xeb, 0xb7, 0xbf, 0xbf, 0xbf, 0xff}, //72 console
    {0xdb, 0xdb, 0x00, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb}, //73 todo, list
    {0xe7, 0xe7, 0xe7, 0x00, 0x00, 0xe2, 0xe7, 0xe2}, //74 apps
    {0xf1, 0x8a, 0xa4, 0x84, 0x84, 0xa4, 0x8a, 0xf1}, //75 android
    {0x00, 0x0e, 0x71, 0xa1, 0xa1, 0x71, 0x0e, 0x00}, //76 bulb
    {0x7e, 0x81, 0x99, 0x99, 0x18, 0x5a, 0x3c, 0x18}, //77 logout
    {0x18, 0x18, 0x5a, 0x3c, 0x99, 0x81, 0x81, 0x7e}, //78 login
    {0x3e, 0x41, 0x49, 0x5d, 0x49, 0x41, 0xfe, 0xc0}, //79 zoom in
    {0x3e, 0x41, 0x49, 0x49, 0x49, 0x41, 0xfe, 0xc0}, //80 zoom out
    {0xff, 0xff, 0xff, 0x00, 0xf7, 0xf7, 0xf7, 0xf7}, //81 dashboard
    {0xc3, 0x99, 0x3c, 0x7e, 0x7e, 0x3c, 0x99, 0xc3}, //82 all out
    {0xc0, 0xe0, 0xf1, 0xff, 0xff, 0xf1, 0xe0, 0xc0}, //83 science
    {0x3c, 0x42, 0xa1, 0x91, 0x89, 0x85, 0x42, 0x3c}, //84 block
    {0x7f, 0x82, 0xb4, 0x84, 0x84, 0xb4, 0x82, 0x7f}, //85 cat, fox
    {0x06, 0x09, 0x09, 0x06, 0x78, 0x84, 0x84, 0x48}, //86 celsius
    {0x06, 0x09, 0x09, 0x06, 0x08, 0xfc, 0x88, 0xc0}, //87 temperature
    {0x06, 0x09, 0x09, 0x06, 0xf8, 0x20, 0x50, 0x88}, //88 kelvin
    {0x24, 0x24, 0xff, 0x24, 0x24, 0xff, 0x24, 0x24}, //89 tag
    {0x00, 0x00, 0x81, 0xc3, 0x66, 0x3c, 0x18, 0x00}, //90 chevron right
    {0x00, 0x18, 0x3c, 0x66, 0xc3, 0x81, 0x00, 0x00}, //91 chevron left
    {0x30, 0x18, 0x0c, 0x06, 0x06, 0x0c, 0x18, 0x30}, //92 chevron up
    {0x0c, 0x18, 0x30, 0x60, 0x60, 0x30, 0x18, 0x0c}, //93 chevron down
    {0x00, 0x60, 0x90, 0x90, 0x7f, 0x00, 0x00, 0x00}, //94 note 1
    {0x00, 0x60, 0xf0, 0xf0, 0x7f, 0x00, 0x00, 0x00}, //95 note 2
    {0x00, 0x60, 0x90, 0x90, 0x7f, 0x01, 0x01, 0x00}, //96 note 3
    {0x00, 0x60, 0xf0, 0xf0, 0x7f, 0x01, 0x01, 0x00}, //97 note 4
    {0x00, 0x60, 0x90, 0x90, 0x7f, 0x05, 0x05, 0x00}, //98 note 5
    {0x00, 0x60, 0xf0, 0xf0, 0x7f, 0x05, 0x05, 0x00}, //99 note 6
    {0x7f, 0x81, 0x81, 0x81, 0x81, 0x7f, 0x22, 0x1e}, //100 cup empty
    {0x7f, 0xe1, 0xe1, 0xe1, 0xe1, 0x7f, 0x22, 0x1e}, //101 cup 20%
    {0x7f, 0xf1, 0xf1, 0xf1, 0xf1, 0x7f, 0x22, 0x1e}, //102 cup 40%
    {0x7f, 0xf9, 0xf9, 0xf9, 0xf9, 0x7f, 0x22, 0x1e}, //103 cup 60%
    {0x7f, 0xfd, 0xfd, 0xfd, 0xfd, 0x7f, 0x22, 0x1e}, //104 cup 80%
    {0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x22, 0x1e}, //105 cup 100%
    {0x07, 0x09, 0x91, 0xe1, 0xe1, 0x91, 0x09, 0x07}, //106 wineglass
    {0x07, 0x19, 0xe1, 0x81, 0x81, 0xe1, 0x19, 0x07}, //107 glass
    {0x04, 0x06, 0xff, 0xff, 0xff, 0x06, 0x0f, 0x0f}, //108 hammer
    {0xff, 0x81, 0x81, 0x81, 0x82, 0x82, 0x82, 0xfc}, //109 folder outline
    {0xff, 0xff, 0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xfc}, //110 folder
    {0xff, 0x81, 0x81, 0x91, 0xba, 0x92, 0x82, 0xfc}, //111 add folder
    {0x1c, 0x30, 0x20, 0xef, 0xef, 0x20, 0x30, 0x1c}, //112 microphone
    {0xe0, 0xe0, 0x00, 0xff, 0xff, 0x00, 0xfc, 0xfc}, //113 equalizer
    {0xff, 0xff, 0xff, 0x7e, 0x3c, 0x18, 0x00, 0xff}, //114 next
    {0xff, 0x00, 0x18, 0x3c, 0x7e, 0xff, 0xff, 0xff}, //115 prev
    {0x3f, 0xa1, 0xa1, 0xe1, 0xe1, 0xa1, 0xa1, 0x3f}, //116 monitor, display
    {0x7c, 0xe2, 0xe1, 0x01, 0x01, 0xe1, 0xe2, 0x7c}, //117 headset, earphones
    {0x18, 0x3c, 0x7e, 0x18, 0x18, 0x7e, 0x3c, 0x18}, //118 workout, fitness
    {0xff, 0x0a, 0x15, 0x0a, 0x15, 0x0a, 0x15, 0x0a}, //119 sport flag
    {0x18, 0x18, 0x24, 0xc3, 0xc3, 0x24, 0x18, 0x18}, //120 location
    {0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0x01, 0xbd}, //121 cellular 1
    {0xc0, 0xa0, 0x90, 0x88, 0x84, 0x82, 0x01, 0xbd}, //122 cellular 2
    {0xc0, 0xa0, 0x90, 0x88, 0x84, 0x82, 0x81, 0xff}, //123 cellular 3
    {0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff, 0xff}, //124 cellular 4
    {0x43, 0x66, 0x7c, 0x78, 0x74, 0x6e, 0xdf, 0xbf}, //125 cellular 5
    {0xc0, 0xe0, 0xf0, 0xf8, 0x0c, 0xae, 0x4f, 0xaf}, //126 cellular 6
    {0x92, 0x49, 0x49, 0x92, 0x92, 0x49, 0x49, 0x92}, //127 waves
    {0x18, 0xf4, 0x92, 0x91, 0x91, 0x92, 0xf4, 0x18}, //128 home
    {0x7e, 0x81, 0xa5, 0x81, 0x81, 0xa5, 0x81, 0x7e}, //129 dice 1
    {0x7e, 0x81, 0x85, 0x81, 0x81, 0xa1, 0x81, 0x7e}, //130 dice 2
    {0x3c, 0x44, 0x47, 0xc4, 0xc4, 0x47, 0x44, 0x3c}, //131 plug
    {0xf8, 0x84, 0x82, 0x81, 0xb1, 0xb2, 0x84, 0xf8}, //132 home 2
    {0xf0, 0x90, 0xf0, 0xf1, 0x92, 0x94, 0x98, 0xf0}, //133 radio
    {0x24, 0x7e, 0xc3, 0x5a, 0x5a, 0xc3, 0x7e, 0x24}, //134 memory
    {0x44, 0x92, 0xba, 0x92, 0x82, 0xaa, 0x82, 0x44}, //135 gamepad
    {0x70, 0x88, 0xa8, 0x8a, 0xa9, 0x8d, 0x89, 0x72}, //136 router
    {0x7e, 0x81, 0x95, 0xa1, 0xa1, 0x95, 0x81, 0x7e}, //137 smile 1
    {0x7e, 0x81, 0xa5, 0x91, 0x91, 0xa5, 0x81, 0x7e}, //138 smile 2
    {0x7e, 0x81, 0xa5, 0xa1, 0xa1, 0xa5, 0x81, 0x7e}, //139 smile 3
    {0x7e, 0x81, 0x85, 0xb1, 0xb1, 0x85, 0x81, 0x7e}, //140 smile 4
    {0x7e, 0x81, 0x8d, 0xe1, 0xe1, 0x8d, 0x81, 0x7e}, //141 smile 5
    {0x01, 0x03, 0xff, 0xfb, 0xbb, 0xab, 0xab, 0xff}, //142 sms
    {0x3c, 0x7e, 0x7e, 0x7e, 0x7e, 0x66, 0x66, 0x3c}, //143 toggle on
    {0x3c, 0x5a, 0x5a, 0x42, 0x42, 0x42, 0x42, 0x3c}, //144 toggle off
    {0x00, 0x00, 0xff, 0x7e, 0x3c, 0x18, 0x00, 0x00}, //145 arrow type 1 right
    {0x00, 0x00, 0x18, 0x3c, 0x7e, 0xff, 0x00, 0x00}, //146 arrow type 1 left
    {0x04, 0x0c, 0x1c, 0x3c, 0x3c, 0x1c, 0x0c, 0x04}, //147 arrow type 1 down
    {0x20, 0x30, 0x38, 0x3c, 0x3c, 0x38, 0x30, 0x20}, //148 arrow type 1 up
    {0x18, 0x18, 0x18, 0x18, 0xff, 0x7e, 0x3c, 0x18}, //149 arrow type 2 right
    {0x18, 0x3c, 0x7e, 0xff, 0x18, 0x18, 0x18, 0x18}, //150 arrow type 2 left
    {0x10, 0x30, 0x70, 0xff, 0xff, 0x70, 0x30, 0x10}, //151 arrow type 2 down
    {0x08, 0x0c, 0x0e, 0xff, 0xff, 0x0e, 0x0c, 0x08}, //152 arrow type 2 up
    {0x02, 0x79, 0x85, 0xb4, 0xa4, 0x85, 0x79, 0x02}, //153 alarm clock
    {0x3c, 0x42, 0x42, 0x3c, 0x08, 0x18, 0x08, 0x38}, //154 key
};
