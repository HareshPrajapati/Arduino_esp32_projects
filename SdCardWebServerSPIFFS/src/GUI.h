#ifndef GUI_h
#define GUI_h

#include <lvgl.h>
#include <Ticker.h>
#include <TFT_eSPI.h>
#include "SDCard.h"



#define LVGL_TICK_PERIOD  (60)
#define SCREEN_WIDTH      (320)
#define SCREEN_HIGHT      (240)


void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
bool my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
void guiInIt();


class TFT_Gui {
private:
    String currentFileName;
    lv_obj_t *list, *tv;
    lv_obj_t *slider_label;

public:
    bool initSD();
    void lvMain();
    void taskFileDownload(void *pvParameters);
    void lvErrorPage();
    void headerSetText(String _name);
    void refreshList();
    void createTab1(lv_obj_t *parent);
    void fileListEvent(lv_obj_t *btn, lv_event_t e);
    String openFile(String filename);
};

#endif