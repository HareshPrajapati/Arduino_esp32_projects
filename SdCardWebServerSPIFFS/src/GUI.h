<<<<<<< HEAD
#ifndef GUI_h
#define GUI_h

#include <lvgl.h>
#include <Ticker.h>
#include <TFT_eSPI.h>
#include "SDCard.h"

#define LVGL_TICK_PERIOD (30)
#define SCREEN_WIDTH (320)
#define SCREEN_HIGHT (240)



void myDispFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
bool myTouchpadRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
void guiInIt();
void eventHandler(lv_obj_t *obj, lv_event_t event);
void homeDir(lv_obj_t *obj, lv_event_t event);
void goUp(lv_obj_t *obj, lv_event_t event);
void deleteBtnCb(lv_obj_t *obj, lv_event_t event);
void shareBtnCb(lv_obj_t *obj, lv_event_t event);
void fileHandler(lv_obj_t *obj, lv_event_t event);
extern TFT_eSPI tft;
extern String nextFile, deleteFile, shareFile ;
extern lv_obj_t *main_list, *win ;
extern lv_obj_t *bar;
extern bool LedShareFileOpen;
extern File myFile;


class TFT_Gui
{
private:
    String currentFileName;
    lv_obj_t *list, *tv;
    lv_obj_t *slider_label;

public:
    void lvMain();
    void taskFileDownload(void *pvParameters);
    void lvErrorPage();
    void headerSetText(String _name);
    void refreshList(String fileName = "/");
    void createTab1(lv_obj_t *parent, String fileName = "/");
    void fileListEvent(lv_obj_t *btn, lv_event_t e);
    String openFile(String filename);
    void lvFileBrowser(String fileName = "/");
};

=======
#ifndef GUI_h
#define GUI_h
/***********************************  Includes *********************************************/
#include <lvgl.h>
#include <Ticker.h>
#include <TFT_eSPI.h>
#include <SDCard.h>
#include <debug.h>

/*********************************** Defines ***********************************************/
#define LVGL_TICK_PERIOD (30)
#define SCREEN_WIDTH (320)
#define SCREEN_HIGHT (240)

/********************************** Extern variables and Objects ***************************/
extern TFT_eSPI tft;
extern String nextFile, deleteFile, shareFile;
extern lv_obj_t *main_list, *win;
extern lv_obj_t *bar;
extern bool LedShareFileOpen;
extern bool fileNameChecker;

/********************************* Function Declaration ***************************************/
void myDispFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
bool myTouchpadRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
void guiInIt();
void directoryHandler(lv_obj_t *obj, lv_event_t event);
void homeDir(lv_obj_t *obj, lv_event_t event);
void goUp(lv_obj_t *obj, lv_event_t event);
void deleteBtnCb(lv_obj_t *obj, lv_event_t event);
void shareBtnCb(lv_obj_t *obj, lv_event_t event);
void fileHandler(lv_obj_t *obj, lv_event_t event);


class TFT_Gui
{
private:
  lv_obj_t *list;
  void refreshList(String fileName = "/");
  void createTab(lv_obj_t *parent, String fileName = "/");

public:
  void lvErrorPage();
  void lvFileBrowser(String fileName = "/");
};

>>>>>>> 36b003facc6f6b0250deccaa20cc6805e5ccb6b7
#endif