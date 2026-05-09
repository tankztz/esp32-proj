#include <M5Unified.h>

RTC_DATA_ATTR int boot_count = 0;

void drawPage(const char *mode) {
  M5.Display.clear(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextDatum(top_left);
  M5.Display.setTextSize(1);
  M5.Display.setFont(&fonts::Font2);

  M5.Display.drawString("esp32-proj", 12, 14);
  M5.Display.drawString("CoreInk Hello", 12, 38);

  char line[48];
  snprintf(line, sizeof(line), "Boot: %d", boot_count);
  M5.Display.drawString(line, 12, 70);

  snprintf(line, sizeof(line), "Mode: %s", mode);
  M5.Display.drawString(line, 12, 94);

  M5.Display.drawString("Btn: refresh", 12, 130);
  M5.Display.drawString("E-ink keeps frame", 12, 154);
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  boot_count++;

  drawPage("boot");
}

void loop() {
  M5.update();

  if (M5.BtnPWR.wasPressed() || M5.BtnEXT.wasPressed()) {
    drawPage("button");
  }

  delay(50);
}
