#include "SmartDimmer.h"

SmartDimmer process;

int bt_len;
char buff_bt[BUFF_SIZE];
char led_state = 0;

unsigned int run_time = 0, processing_time = 100;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin 13 as an output.
  pinMode(13, OUTPUT);
  Serial.begin(9600);
  Serial.flush();
}

// the loop function runs over and over again forever
void loop() {
  bt_len = Serial.available();
  if (bt_len > 7)
  {
    Serial.readBytes(buff_bt, bt_len);
    if (process.ProcessPacket(buff_bt, bt_len) == PACKET_OK)
    {
      memset(buff_bt, 0, bt_len);
      Serial.flush();
      if (process.need_to_send)
      {
        int i;
        for (i = 0; i < process.trans_buff_len; i++)
        {
          Serial.write(process.trans_buff[i]);
        }

        process.trans_buff_len = 0;
        process.need_to_send = 0;
      }
    }
    else if (process.errn == FLASH_ERROR)
    {
      //LEDs_TurnOn(LED_RUN);
      for (;;);
    }
    else if (process.errn != PACKET_NOT_ENOUGH_LENGTH)
    {
      memset(buff_bt, 0, bt_len);
      Serial.flush();
    }

  }

  if (millis() - run_time > processing_time)
  {
    run_time = millis();

    //    LEDs_Toggle(LED_RUN);
    if (led_state == 0)
    {
      digitalWrite(13, HIGH);
      led_state = 1;
    }
    else
    {
      digitalWrite(13, LOW);
      led_state = 0;
    }

    if (process.SearchProgress())
    {
      int i;
      for (i = 0; i < process.trans_buff_len; i++)
      {
#if USE_RELAYS
#else
        Serial.write(process.trans_buff[i]);
#endif
      }
    }
  }

  delay(10);
}
