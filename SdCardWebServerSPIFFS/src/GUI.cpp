
#include "GUI.h"
#include <SD.h>
#include <FS.h>



String nextFile, deleteFile = "", shareFile = "";
TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];
static lv_color_t buf2[LV_HOR_RES_MAX * 10];
lv_obj_t *main_list, *win ;
lv_obj_t *bar;
bool LedShareFileOpen = false;
File myFile;
extern TFT_Gui TFTGUI;





String TFT_Gui::openFile(String filename)
{
  File file = SD.open(filename);
  if (!file)
  {
    file.close();
    return "";
  }
  String textData;
  while (file.available())
  {
    textData += char(file.read());
  }
  Serial.println(textData);
  file.close();
  return textData;
}

void TFT_Gui::lvErrorPage()
{
  lv_obj_t *label = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_text(label, "Please check your SD Card");
  lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
}

void TFT_Gui::refreshList(String fileName)
{
  if (list != NULL)
  {
    lv_list_clean(list);
  }
  File root = SD.open(fileName);
  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      nextFile = file.name();
      int n = nextFile.lastIndexOf('/');
      int m = nextFile.length();
      String tempName = nextFile.substring(n, m);
      lv_obj_t *list_btn = lv_list_add_btn(list, LV_SYMBOL_DIRECTORY, (const char *)tempName.c_str());
      lv_obj_set_event_cb(list_btn, eventHandler);
    }
    else
    {
      String nextFile1 = file.name();
      int n = nextFile1.lastIndexOf('/');
      int m = nextFile1.length();
      String tempName = nextFile1.substring(n, m);
      lv_obj_t *list_btn = lv_list_add_btn(list, LV_SYMBOL_FILE, (const char *)tempName.c_str());
      lv_obj_set_event_cb(list_btn, fileHandler);
    }
    file = root.openNextFile();
  }
}

void TFT_Gui::createTab1(lv_obj_t *parent, String fileName)
{
  //List Tab
  lv_page_set_scrl_layout(parent, LV_LAYOUT_PRETTY_TOP);
  list = lv_list_create(parent, NULL);
  lv_obj_set_size(list, lv_obj_get_width(parent) - 20, lv_obj_get_height(parent) - 20);
  lv_obj_align(list, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 10);
  refreshList(fileName);
}

/************************************************************************************************************/


/* Display flushing */
void myDispFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint16_t c;
  tft.startWrite();                                                                            /* Start new TFT transaction */
  tft.setAddrWindow(area->x1, area->y1, (area->x2 - area->x1 + 1), (area->y2 - area->y1 + 1)); /* set the working window */
  for (int y = area->y1; y <= area->y2; y++)
  {
    for (int x = area->x1; x <= area->x2; x++)
    {
      c = color_p->full;
      tft.writeColor(c, 1);
      color_p++;
    }
  }
  tft.endWrite();            /* terminate TFT transaction */
  lv_disp_flush_ready(disp); /* tell lvgl that flushing is done */
}

bool myTouchpadRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  uint16_t touchX, touchY;
  bool touched = tft.getTouch(&touchX, &touchY, 600U);
  if (!touched)
  {
    return false;
  }
  if (touchX > SCREEN_WIDTH || touchY > SCREEN_HIGHT)
  {
    Serial.println("Y or y outside of expected parameters..");
    Serial.print("y:");
    Serial.print(touchX);
    Serial.print(" x:");
    Serial.print(touchY);
  }
  else
  {
    data->state = touched ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    /*Set the coordinates (if released use the last pressed coordinates)*/
    data->point.x = touchX;
    data->point.y = touchY;
    // Serial.printf(" data->point.x = %u \r\n",touchX);
    // Serial.printf(" data->point.y = %u \r\n",touchY);
  }
  return false; /*Return `false` because we are not buffering and no more data to read*/
}

void guiInIt()
{

  lv_init();
  tft.begin(); /* TFT init */
  tft.setRotation(1);
  // uint16_t calData[5] = { 275, 3620, 264, 3532, 1 };
  uint16_t calData[5] = {378U, 3539U, 743U, 2724U, 7U};
  tft.setTouch(calData);
  lv_disp_buf_init(&disp_buf, buf, buf2, LV_HOR_RES_MAX * 10);
  /*Initialize the display*/
  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = SCREEN_WIDTH;
  disp_drv.ver_res = SCREEN_HIGHT;
  disp_drv.flush_cb = myDispFlush;
  disp_drv.buffer = &disp_buf;
  lv_disp_drv_register(&disp_drv);
  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);          /*Descriptor of a input device driver*/
  indev_drv.type = LV_INDEV_TYPE_POINTER; /*Touch pad is a pointer-like device*/
  indev_drv.read_cb = myTouchpadRead;   /*Set your driver function*/
  lv_indev_drv_register(&indev_drv);      /*Finally register the driver*/
}

void TFT_Gui::lvFileBrowser(String fileName)
{
  /* Create a window to use as the action bar */
  win = lv_win_create(lv_scr_act(), NULL);
  // lv_obj_clean(win);
  lv_obj_set_size(win, LV_HOR_RES, LV_VER_RES);
  lv_win_set_title(win, "File Manager");
  lv_obj_t *up_btn = lv_win_add_btn(win, LV_SYMBOL_UP);
  lv_obj_set_event_cb(up_btn, goUp);
  lv_obj_t *home = lv_win_add_btn(win, LV_SYMBOL_HOME);
  lv_obj_set_event_cb(home, homeDir);
  lv_obj_t *win_content = lv_win_get_content(win);
  lv_cont_set_fit(lv_page_get_scrl(win_content), _LV_FIT_LAST);
  /* Create the list */
  main_list = lv_list_create(win, NULL);
  /* Fit the list inside the page, taking into account any borders. */
  lv_area_t page_area;
  lv_obj_get_coords(win_content, &page_area);
  lv_obj_get_inner_coords(win_content, &page_area);
  lv_obj_set_size(main_list, lv_area_get_width(&page_area), lv_area_get_height(&page_area));
  createTab1(main_list, fileName);
}

void eventHandler(lv_obj_t *obj, lv_event_t event)
{
  if (event == LV_EVENT_CLICKED)
  {
    Serial.printf("Clicked: %s\n", lv_list_get_btn_text(obj));
    shareFile += lv_list_get_btn_text(obj);
    deleteFile += lv_list_get_btn_text(obj);
    lv_obj_del(win);
    TFTGUI.lvFileBrowser(nextFile);
  }
}

void fileHandler(lv_obj_t *obj, lv_event_t event)
{
  if (event == LV_EVENT_CLICKED)
  {
    Serial.printf("Clicked: %s\n", lv_list_get_btn_text(obj));
    shareFile += lv_list_get_btn_text(obj);
    deleteFile += lv_list_get_btn_text(obj);
    win = lv_win_create(lv_scr_act(), NULL);
    lv_obj_set_size(win, LV_HOR_RES, LV_VER_RES);
    lv_win_set_title(win, "File Manager");
    lv_obj_t *up_btn = lv_win_add_btn(win, LV_SYMBOL_UP);
    lv_obj_set_event_cb(up_btn, goUp);
    lv_obj_t *home = lv_win_add_btn(win, LV_SYMBOL_HOME);
    lv_obj_set_event_cb(home, homeDir);
    lv_obj_t *win_content = lv_win_get_content(win);
    lv_cont_set_fit(lv_page_get_scrl(win_content), _LV_FIT_LAST);
    lv_obj_t *btn = lv_btn_create(win, NULL);
    lv_obj_set_pos(btn, 40, 60);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_set_event_cb(btn, deleteBtnCb);
    lv_obj_t *label = lv_label_create(btn, NULL); /*Add a label to the button*/
    lv_label_set_text(label, "Delete");
    lv_obj_t *shareBtn = lv_btn_create(win, btn);
    lv_obj_set_pos(shareBtn, 180, 60);
    lv_obj_set_size(shareBtn, 120, 50);
    lv_obj_t *shareLabel = lv_label_create(shareBtn, NULL); /*Add a label to the button*/
    lv_label_set_text(shareLabel, "Share");
    lv_obj_set_event_cb(shareBtn, shareBtnCb);
  }
}

void homeDir(lv_obj_t *obj, lv_event_t event)
{
  if (event == LV_EVENT_CLICKED)
  {
    Serial.printf("Clicked: %s\n", lv_list_get_btn_text(obj));
    shareFile = "";
    deleteFile = "";
    lv_obj_del(win);
    TFTGUI.lvFileBrowser("/");
  }
}

void deleteBtnCb(lv_obj_t *obj, lv_event_t event)
{
  if (event == LV_EVENT_CLICKED)
  {
    Serial.printf("Clicked: %s\n", lv_list_get_btn_text(obj));
    Serial.printf("deleted File : %s\n", deleteFile.c_str());
    SD.remove(deleteFile);
    deleteFile = "";
    int n = nextFile.lastIndexOf('/');
    String backDir = nextFile.substring(0, n);
    if (n == 0)
    {
      lv_obj_del(win);
      TFTGUI.lvFileBrowser();
    }
    else
    {
      lv_obj_del(win);
      TFTGUI.lvFileBrowser(backDir);
    }
  }
}

void goUp(lv_obj_t *obj, lv_event_t event)
{
  if (event == LV_EVENT_CLICKED)
  {
    int n = nextFile.lastIndexOf('/');
    String backDir = nextFile.substring(0, n);
    shareFile = backDir;
    deleteFile = backDir;
    Serial.printf("len is %d and  backDir is %s \r\n", n, backDir.c_str());
    if (n == 0)
    {

      lv_obj_del(win);
      TFTGUI.lvFileBrowser();
    }
    else
    {
      lv_obj_del(win);
      TFTGUI.lvFileBrowser(backDir);
      nextFile = backDir;
    }
  }
}

void shareBtnCb(lv_obj_t *obj, lv_event_t event)
{
  if (event == LV_EVENT_CLICKED)
  {
    Serial.printf("Clicked: %s\n", lv_list_get_btn_text(obj));
    win = lv_win_create(lv_scr_act(), NULL);
    lv_obj_set_size(win, LV_HOR_RES, LV_VER_RES);
    lv_win_set_title(win, "File Manager");
    lv_obj_t *up_btn = lv_win_add_btn(win, LV_SYMBOL_UP);
    lv_obj_set_event_cb(up_btn, goUp);
    lv_obj_t *home = lv_win_add_btn(win, LV_SYMBOL_HOME);
    lv_obj_set_event_cb(home, homeDir);
    lv_obj_t *win_content = lv_win_get_content(win);
    lv_cont_set_fit(lv_page_get_scrl(win_content), _LV_FIT_LAST);
    Serial.printf("shared File : %s\n", shareFile.c_str());
    bar = lv_bar_create(win, NULL);
    lv_obj_set_size(bar, 250, 30);
    lv_obj_set_pos(bar, 40, 60);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_start_value(bar, 0, LV_ANIM_ON);
    myFile = SD.open(shareFile);
    if (myFile.available())
    {
      LedShareFileOpen = true;
    }
  }
}
