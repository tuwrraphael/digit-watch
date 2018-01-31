#ifndef APP_DISPLAY_H
#define APP_DISPLAY_H

#define DISPLAY_SCS (6)
#define DISPLAY_EXTCOMIN (8)
#define DISPLAY_DISP (11)
#define DISPLAY_MOSI (4)
#define DISPLAY_SCK (3)

#define DISPLAY_WIDTH (128)
#define DISPLAY_HEIGHT (128)

void init_display();

void transfer_buffer_to_display();

#endif // !APP_DISPLAY_H
