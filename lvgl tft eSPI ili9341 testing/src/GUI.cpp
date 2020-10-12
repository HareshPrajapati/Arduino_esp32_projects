
#include "GUI.h"
#include <SD.h>
#include <FS.h>

TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];


/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint16_t c;
  tft.startWrite();                                    /* Start new TFT transaction */
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

bool my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  uint16_t touchX, touchY;
  bool touched = tft.getTouch(&touchX, &touchY, 600);
  if (!touched){
    return false;
  }
  if (touchX > SCREEN_WIDTH || touchY > SCREEN_HIGHT) {
    Serial.println("Y or y outside of expected parameters..");
    Serial.print("y:");
    Serial.print(touchX);
    Serial.print(" x:");
    Serial.print(touchY);
  }else {
    data->state = touched ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    /*Set the coordinates (if released use the last pressed coordinates)*/
    data->point.x = touchX;
    data->point.y = touchY;
  }
  return false; /*Return `false` because we are not buffering and no more data to read*/
}

void guiInIt() {

    lv_init();
    tft.begin(); /* TFT init */
    tft.setRotation(1);

    //uint16_t calData[5] = { 275, 3620, 264, 3532, 1 };
    uint16_t calData[5] = {378, 3539, 743, 2724, 7};
    tft.setTouch(calData);
    lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);
    /*Initialize the display*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);          /*Descriptor of a input device driver*/
    indev_drv.type = LV_INDEV_TYPE_POINTER; /*Touch pad is a pointer-like device*/
    indev_drv.read_cb = my_touchpad_read;   /*Set your driver function*/
    lv_indev_drv_register(&indev_drv);      /*Finally register the driver*/
}

bool TFT_Gui::initSD() {
  if (!SD.begin(SS)) {
    return false;
  }
  return true;
}

String TFT_Gui::openFile(String filename) {
  File file = SD.open(filename);
  if (!file) {
    file.close();
    return "";
  }
  String textData;
  while (file.available()) {
    textData += char(file.read());
  }
  Serial.println(textData);
  file.close();
  return textData;
}


void TFT_Gui::lvMain() {
  lv_theme_t *th = lv_theme_material_init(LV_THEME_DEFAULT_COLOR_PRIMARY, LV_THEME_DEFAULT_COLOR_SECONDARY, LV_THEME_DEFAULT_FLAG, LV_THEME_DEFAULT_FONT_SMALL, LV_THEME_DEFAULT_FONT_NORMAL, LV_THEME_DEFAULT_FONT_SUBTITLE, LV_THEME_DEFAULT_FONT_TITLE);
  lv_theme_set_act(th);

  lv_obj_t *scr = lv_cont_create(NULL, NULL);
  lv_disp_load_scr(scr);

  tv = lv_tabview_create(scr, NULL);
  lv_obj_set_pos(tv, 0, 0);
  lv_obj_set_size(tv, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
  lv_tabview_set_anim_time(tv, 50);

  lv_obj_t *tab1  = lv_tabview_add_tab(tv,"My Sd Card");
  createTab1(tab1);
}

void TFT_Gui::lvErrorPage() {
  lv_obj_t *label = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_text(label, "Please check your SD Card");
  lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
}

void TFT_Gui::refreshList() {
  if (list != NULL) {
    lv_list_clean(list);
  }

  File root = SD.open("/");
  File file = root.openNextFile();
  while (file) {
    // lv_obj_t *btn = lv_list_add_btn(list, LV_SYMBOL_FILE, file.name());   //when impliment callback please comment out it
    lv_list_add_btn(list, LV_SYMBOL_FILE, file.name());
    file = root.openNextFile();
  }
}

void TFT_Gui::createTab1(lv_obj_t *parent)
{
  //List Tab
  lv_page_set_scrl_layout(parent, LV_LAYOUT_PRETTY_TOP);
  list = lv_list_create(parent, NULL);
  lv_obj_set_size(list, lv_obj_get_width(parent) - 20, lv_obj_get_height(parent) - 20);
  lv_obj_align(list, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 10);
  refreshList();
}

void TFT_Gui::fileListEvent(lv_obj_t *btn, lv_event_t e) {
  if (e == LV_EVENT_CLICKED) {
    currentFileName = lv_list_get_btn_text(btn);
    Serial.println(currentFileName);

    headerSetText(currentFileName);
    lv_tabview_set_tab_act(tv, 1, LV_ANIM_ON);

    // lv_textarea_set_text(ta, openFile(currentFileName).c_str());

  //   lv_obj_t *parent = lv_obj_get_parent(lv_obj_get_parent(ta));
  //   lv_obj_set_size(ta, lv_obj_get_width(parent) - 10, lv_obj_get_height(parent) - 10);
  //   lv_obj_move_background(kb);
  }
}



