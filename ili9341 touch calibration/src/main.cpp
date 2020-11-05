#include <Arduino.h>
#include <SPI.h>
#include <FS.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <lvgl.h>
#include <Ticker.h>
#include <SD.h>

#define LVGL_TICK_PERIOD (30)
#define SCREEN_WIDTH (320)
#define SCREEN_HIGHT (240)

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
Ticker tick;
static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];
static lv_color_t buf2[LV_HOR_RES_MAX * 10];
lv_obj_t *list, *tv;
lv_obj_t *slider_label;
// static lv_obj_t *main_list, *win;
String currentFileName;
String nextFile, deleteFile = "";
;

void touch_calibrate();
bool my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void lvErrorPage();
void refreshList(String fileName = "/");
void createTab1(lv_obj_t *parent, String fileName = "/");
void fileListEvent(lv_obj_t *btn, lv_event_t e);
static void eventHandler(lv_obj_t *obj, lv_event_t event);
static void homeDir(lv_obj_t *obj, lv_event_t event);
static void goUp(lv_obj_t *obj, lv_event_t event);
static void deleteBtnCb(lv_obj_t *obj, lv_event_t event);
static void fileHandler(lv_obj_t *obj, lv_event_t event);
void lv_file_browser(String fileName = "/");
static void LvTickHandler(void);
//------------------------------------------------------------------------------------------

void setup()
{
  // Use serial port
  Serial.begin(921600UL);
  Serial.setDebugOutput(true);
  ledcAttachPin(22, 1);      // assign TFT_BL pin to channel 1
  ledcSetup(1, 12000UL, 10); // 12 kHz PWM, 10-bit resolution
  analogReadResolution(10);
  ledcWrite(1, 768); // brightness 0 - 255
  // Initialise the TFT screen
  lv_init();
  tft.begin();

  // Set the rotation before we calibrate
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  uint16_t calData[5] = {378U, 3539U, 743U, 2724U, 7U};
  tft.setTouch(calData);
  lv_disp_buf_init(&disp_buf, buf, buf2, LV_HOR_RES_MAX * 10);
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
  tick.attach_ms(LVGL_TICK_PERIOD, LvTickHandler);
  if (SD.begin(SS))
  {
    lv_file_browser();
  }
  else
  {
    lvErrorPage();
  }
}
//------------------------------------------------------------------------------------------

void loop(void)
{

  lv_task_handler();
  vTaskDelay(5);
  Serial.printf("hello lcd. \r\n");
}

//------------------------------------------------------------------------------------------

static void LvTickHandler(void)
{
  lv_tick_inc(LVGL_TICK_PERIOD);
}

// Code to run a screen calibration, not needed when calibration values set in setup()
void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // Calibrate
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(20, 0);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.println("Touch corners as indicated");

  tft.setTextFont(1);
  tft.println();

  tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

  Serial.println();
  Serial.println();
  Serial.println("// Use this calibration code in setup():");
  Serial.print("  uint16_t calData[5] = ");
  Serial.print("{ ");

  for (uint8_t i = 0; i < 5; i++)
  {
    Serial.print(calData[i]);
    if (i < 4)
      Serial.print(", ");
  }

  Serial.println(" };");
  Serial.print("  tft.setTouch(calData);");
  Serial.println();
  Serial.println();

  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("Calibration complete!");
  tft.println("Calibration code sent to Serial port.");

  delay(4000);
}

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
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

bool my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  uint16_t touchX, touchY;
  bool touched = tft.getTouch(&touchX, &touchY, 600U);
  if (!touched)
  {
    // Serial.printf("touch false \r\n");
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

    // Serial.printf("touched \r\n");
  }
  return false; /*Return `false` because we are not buffering and no more data to read*/
}

void lvErrorPage()
{
  lv_obj_t *label = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_text(label, "Please check your SD Card");
  lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
}

void refreshList(String fileName)
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
      // Serial.printf("nextFile in file.isDirectory() = %s \r\n",nextFile.c_str());
      int n = nextFile.lastIndexOf('/');
      int m = nextFile.length();
      String tempName = nextFile.substring(n, m);
      lv_obj_t *list_btn = lv_list_add_btn(list, LV_SYMBOL_DIRECTORY, (const char *)tempName.c_str());
      // Serial.printf("next directory is %s \r\n",nextFile.c_str());
      lv_obj_set_event_cb(list_btn, eventHandler);
    }
    else
    {
      String nextFile1 = file.name();
      int n = nextFile1.lastIndexOf('/');
      int m = nextFile1.length();
      String tempName = nextFile1.substring(n, m);
      lv_obj_t *list_btn = lv_list_add_btn(list, LV_SYMBOL_FILE, (const char *)tempName.c_str());
      deleteFile = nextFile1;
      lv_obj_set_event_cb(list_btn, fileHandler);
    }
    file = root.openNextFile();
  }
}

void createTab1(lv_obj_t *parent, String fileName)
{
  //List Tab
  lv_page_set_scrl_layout(parent, LV_LAYOUT_PRETTY_TOP);
  list = lv_list_create(parent, NULL);
  lv_obj_set_size(list, lv_obj_get_width(parent) - 20, lv_obj_get_height(parent) - 20);
  lv_obj_align(list, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 10);
  refreshList(fileName);
}

/************************************************************************************************************/

void lv_file_browser(String fileName)
{
  /* Create a window to use as the action bar */
  lv_obj_t *win;
  win = lv_win_create(lv_scr_act(), NULL);
  lv_obj_set_size(win, LV_HOR_RES, LV_VER_RES);
  lv_win_set_title(win, "File Manager");
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
    lv_obj_set_event_cb(shareBtn, deleteBtnCb);
  // lv_win_set_title(win, "File Manager");
  // lv_obj_t *up_btn = lv_win_add_btn(win, LV_SYMBOL_UP);
  // lv_obj_set_event_cb(up_btn, goUp);
  // lv_obj_t *home = lv_win_add_btn(win, LV_SYMBOL_HOME);
  // lv_obj_set_event_cb(home, homeDir);
  // lv_obj_t *win_content = lv_win_get_content(win);
  // lv_cont_set_fit(lv_page_get_scrl(win_content), _LV_FIT_LAST);
  // /* Create the list */
  // main_list = lv_list_create(win, NULL);
  // /* Fit the list inside the page, taking into account any borders. */
  // lv_area_t page_area;
  // lv_obj_get_coords(win_content, &page_area);
  // lv_obj_get_inner_coords(win_content, &page_area);
  // lv_obj_set_size(main_list, lv_area_get_width(&page_area), lv_area_get_height(&page_area));
  // createTab1(main_list, fileName);
}

static void eventHandler(lv_obj_t *obj, lv_event_t event)
{
  if (event == LV_EVENT_CLICKED)
  {
    // Serial.printf("Clicked: %s\n", lv_list_get_btn_text(obj));
    lv_file_browser(nextFile);
  }
}

// static void fileHandler(lv_obj_t *obj, lv_event_t event)
// {
//   if (event == LV_EVENT_CLICKED)
//   {
//     win = lv_win_create(lv_scr_act(), NULL);
//     lv_obj_set_size(win, LV_HOR_RES, LV_VER_RES);
//     lv_win_set_title(win, "File Manager");
//     lv_obj_t *up_btn = lv_win_add_btn(win, LV_SYMBOL_UP);
//     lv_obj_set_event_cb(up_btn, goUp);
//     lv_obj_t *home = lv_win_add_btn(win, LV_SYMBOL_HOME);
//     lv_obj_set_event_cb(home, homeDir);
//     lv_obj_t *win_content = lv_win_get_content(win);
//     lv_cont_set_fit(lv_page_get_scrl(win_content), _LV_FIT_LAST);
//     lv_obj_t *btn = lv_btn_create(win, NULL);
//     lv_obj_set_pos(btn, 40, 60);
//     lv_obj_set_size(btn, 120, 50);
//     lv_obj_set_event_cb(btn, deleteBtnCb);
//     lv_obj_t *label = lv_label_create(btn, NULL); /*Add a label to the button*/
//     lv_label_set_text(label, "Delete");
//     lv_obj_t *shareBtn = lv_btn_create(win, btn);
//     lv_obj_set_pos(shareBtn, 180, 60);
//     lv_obj_set_size(shareBtn, 120, 50);
//     lv_obj_t *shareLabel = lv_label_create(shareBtn, NULL); /*Add a label to the button*/
//     lv_label_set_text(shareLabel, "Share");
//   }
// }

static void homeDir(lv_obj_t *obj, lv_event_t event)
{
  if (event == LV_EVENT_CLICKED)
  {
    // Serial.printf("Clicked: %s\n", lv_list_get_btn_text(obj));
    lv_file_browser("/");
  }
}

static void deleteBtnCb(lv_obj_t *obj, lv_event_t event)
{
  if (event == LV_EVENT_CLICKED)
  {
    Serial.printf("Clicked: %s\n", lv_list_get_btn_text(obj));
     lv_file_browser();
    // Serial.printf("deleted File : %s\n", deleteFile.c_str());
    // SD.remove(deleteFile);
    // int n = nextFile.lastIndexOf('/');
    // String backDir = nextFile.substring(0, n);
    // if (n == 0)
    // {
    //   lv_file_browser();
    // }
    // else
    // {
    //   lv_file_browser(backDir);
    // }
  }
}

static void goUp(lv_obj_t *obj, lv_event_t event)
{
  if (event == LV_EVENT_CLICKED)
  {
    int n = nextFile.lastIndexOf('/');
    String backDir = nextFile.substring(0, n);
    Serial.printf("len is %d and  backDir is %s \r\n", n, backDir.c_str());
    if (n == 0)
    {
      lv_file_browser();
    }
    else
    {
      lv_file_browser(backDir);
      nextFile = backDir;
    }
  }
}