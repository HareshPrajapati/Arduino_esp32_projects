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

#endif