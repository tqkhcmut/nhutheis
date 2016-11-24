#include "SmartDimmer.h"

#include <string.h>
#include <stdio.h>

unsigned char flash_data_need_to_save = 0;

// command
#define TEMPERATURE             0x01
#define HUMIDITY                0x02
#define POWER                   0x03
#define DIMMING                 0x04
#define MOTOR                   0x05
#define TIME_SETTING            0x06
#define CALENDER_SETTING        0x07
#define ADDRESS_SETTING         0x08
#define MANUAL_MODE             0x09
#define BLUETOOTH_NAME          0x0a
#define BLUETOOTH_PIN           0x0b
#define DEVICE_PAIR             0x10
#define DEVICE_SEARCH           0x11

#define IS_SUPPORT_CMD(x) (x == TEMPERATURE || x == HUMIDITY || x == POWER || \
                           x == DIMMING || x == MOTOR || x == TIME_SETTING || \
                           x == CALENDER_SETTING || x == ADDRESS_SETTING || x == MANUAL_MODE || \
                           x == BLUETOOTH_NAME || x == BLUETOOTH_PIN || x == DEVICE_PAIR || \
                           x == DEVICE_SEARCH)

#define MANUAL_MODE_ON          0x01
#define MANUAL_MODE_OFF         0x02

#define CONTROL                 0x01
#define QUERY                   0x02
#define ANSWER                  0x03

#define SOURCE_BT               0x01
#define SOURCE_RF_REMOTE        0x02
#define SOURCE_RF_CONNECT_BOX   0x03

#define DEVICE_PAIRED           0x01
#define DEVICE_UNPAIRED         0x02

#define SEARCH_LEVEL_1          0x01
#define SEARCH_LEVEL_2          0x02
#define SEARCH_LEVEL_3          0x03

#ifdef LONG_ID
#define DEV_TYPE_VM_LIFT                0x01
#define DEV_TYPE_VM_SMART_DIMMER        0x02
#define DEV_TYPE_VM_MOTION_BOX          0x03

#define OWN_DEV_TYPE            DEV_TYPE_VM_SMART_DIMMER
#endif

#ifdef LONG_ID
const unsigned char BROADCAST_ID[] = {0xff, 0xff, 0xff, 0xff};
const unsigned char CONNECT_BOX_ID[] = {0x00, 0x00, 0x00, 0x00};
const unsigned char DEFAULT_ID[] = {0x00, 0x00, 0x00, 0x01};
#else
const unsigned char BROADCAST_ID[] = {0xff};
const unsigned char CONNECT_BOX_ID[] = {0x00};
const unsigned char DEFAULT_ID[] = {0x01};
#endif

struct Packet
{
  unsigned char start;
  unsigned char state;
#ifdef LONG_ID
  unsigned char dev_type;
#endif
  unsigned char id[ID_LENGTH];
  unsigned char source;
  unsigned char data[];
};

struct addr_s
{
  unsigned char year;
  unsigned char month;
  unsigned char date;
  unsigned char hour;
  unsigned char minute;
  unsigned char second;
};

struct FlashData
{
  unsigned char power_value;
  unsigned char id[ID_LENGTH];
  unsigned char dev_pair;
};

FlashData flash_data;

struct addr_s addr_struct;

union int_s
{
  uint32_t n;
  uint8_t b[4];
};

union int_s addr_num;

void addr_struct_to_u32(struct addr_s s, uint32_t * addr)
{
  *addr = s.year << 27 | s.month << 23 | s.date << 17 | s.hour << 12 | s.minute << 6 | s.second;
}
void addr_u32_to_struct(uint32_t addr, struct addr_s * s)
{
  s->year = (addr >> 27) & 0x3f; // 6
  s->month = (addr >> 23) & 0x0f; // 4
  s->date = (addr >> 17) & 0x1f; // 5
  s->hour = (addr >> 12) & 0x1f; // 5
  s->minute = (addr >> 6) & 0x3f; // 6
  s->second = (addr >> 0) & 0x3f; // 6
}

int flash_read_buffer(char * buff, int len)
{
  int i = 0;
  for (i = 0; i < len; i++)
  {
    buff[i] = EEPROM.read(i);
  }
  return 0;
}
int flash_write_buffer(char * buff, int len)
{
  int i = 0;
  for (i = 0; i < len; i++)
  {
    EEPROM.write(i, buff[i]);
  }
  return 0;
}

int IsValidString(char * str, int len)
{
  int i = 0;
  for (i = 0; i < len; i++)
  {
    if (str[i] > 'z' || str[i] < '!')
      return 0;
  }
  return i;
}

SmartDimmer::SmartDimmer()
{
  int i;

  //DrvCurr_SetPWM(MAX_PWM);

  flash_read_buffer((char *)&flash_data, sizeof(struct FlashData));
  if (memcmp(flash_data.id, BROADCAST_ID, ID_LENGTH) == 0 )
  {
    memcpy(flash_data.id, DEFAULT_ID, ID_LENGTH);
    flash_data.dev_pair = DEVICE_PAIRED;
    if (flash_write_buffer((char*)&flash_data, sizeof(flash_data)) != 0)
    {
      //      return FLASH_ERROR;
    }
  }

  analogWrite(13, 0);

  for (i = 0; i < ID_LENGTH; i++)
  {
    addr_num.b[i] = flash_data.id[ID_LENGTH - 1 - i];
  }
  addr_u32_to_struct(addr_num.n, &addr_struct);

}

#define SEARCH_DISABLE  -1
#define SEARCH_IDLE     0
int SearchDelay = SEARCH_DISABLE, under100ms = 0;
unsigned char search_level = 0;


int SmartDimmer::ProcessPacket(char * packet, unsigned char len)
{
  struct Packet * recv_packet = (struct Packet *)packet;
  struct Packet * send_packet = (struct Packet *)trans_buff;
  int i;

  errn = PACKET_OK;
  memset(err_msg, 0, 100);

  if (IS_SUPPORT_CMD(recv_packet->start))
  {
  }
  else
  {
    strcpy(err_msg, "SmartDimmer: Command not supported.");
    errn = PACKET_FAIL;
    return PACKET_FAIL;
  }

#ifdef LONG_ID
  if (len < 5 + ID_LENGTH)
#else
  if (len < 4 + ID_LENGTH)
#endif
  {
    strcpy(err_msg, "SmartDimmer: Not enough length.");
    errn = PACKET_NOT_ENOUGH_LENGTH;
    return PACKET_NOT_ENOUGH_LENGTH;
  }
  if (recv_packet == (void*)0)
  {
    strcpy(err_msg, "SmartDimmer: Void packet.");
    errn = PACKET_NOT_ENOUGH_LENGTH;
    return PACKET_NOT_ENOUGH_LENGTH;
  }

#ifdef LONG_ID
  if (recv_packet->dev_type != OWN_DEV_TYPE && recv_packet->dev_type != 0xff)
  {
    strcpy(err_msg, "SmartDimmer: Not own device.");
    errn = PACKET_INVALID_ID;
    return PACKET_INVALID_ID;
  }
#endif

  if (memcmp((const void *)recv_packet->id, (const void *)flash_data.id, ID_LENGTH) != 0 && memcmp((const void *)recv_packet->id, (const void *)BROADCAST_ID, ID_LENGTH) != 0)
  {
    strcpy(err_msg, "SmartDimmer: Not own ID.");
    errn = PACKET_INVALID_ID;
    return PACKET_INVALID_ID;
  }

  switch (recv_packet->start)
  {
    case DIMMING:
      switch (recv_packet->state)
      {
        case CONTROL:
          {
#ifdef LONG_ID
            if (len < 6 + ID_LENGTH)
#else
            if (len < 5 + ID_LENGTH)
#endif
            {
              strcpy(err_msg, "SmartDimmer: Not enough length.");
              errn = PACKET_NOT_ENOUGH_LENGTH;
              return PACKET_NOT_ENOUGH_LENGTH;
            }
            else
            {
              //              DrvCurr_SetPWM(convert_pow(recv_packet->data[0]));
              analogWrite(13, recv_packet->data[0]);
            }
          }
          break;
        case QUERY:
          break;
        case ANSWER:
          break;
        default: break;
      }
      break;
    case ADDRESS_SETTING:
      switch (recv_packet->state)
      {
        case 0x55:
          //    case CONTROL:
          strcpy(err_msg, "SmartDimmer: Setup ID.");

#ifdef LONG_ID
          if (len < 5 + ID_LENGTH + ID_LENGTH)
#else
          if (len < 4 + ID_LENGTH + ID_LENGTH)
#endif
          {
            strcpy(err_msg, "SmartDimmer: Not enough length.");
            errn = PACKET_NOT_ENOUGH_LENGTH;
            return PACKET_NOT_ENOUGH_LENGTH;
          }
#ifdef LONG_ID
          if (recv_packet->data[0] != OWN_DEV_TYPE)
          {
            strcpy(err_msg, "SmartDimmer: Not own device.");
            errn = PACKET_INVALID_ID;
            return PACKET_INVALID_ID;
          }
          memcpy(flash_data.id, &recv_packet->data[1], ID_LENGTH);
#else
          memcpy(flash_data.id, recv_packet->data, ID_LENGTH);
#endif

          flash_data_need_to_save = 1;

          for (i = 0; i < ID_LENGTH; i++)
          {
            addr_num.b[i] = flash_data.id[ID_LENGTH - 1 - i];
          }
          addr_u32_to_struct(addr_num.n, &addr_struct);
          break;
        case QUERY:
          send_packet->start = recv_packet->start;
          send_packet->state = ANSWER;
#ifdef LONG_ID
          send_packet->dev_type = OWN_DEV_TYPE;
#endif
          memcpy(send_packet->id, flash_data.id, ID_LENGTH);
          send_packet->source = recv_packet->source;
#ifdef LONG_ID
          send_packet->data[0] = 0xfe;
          trans_buff_len = 5 + ID_LENGTH;
#else
          memcpy(send_packet->data, flash_data.id, ID_LENGTH);
          send_packet->data[ID_LENGTH] = 0xfe;
          trans_buff_len = 4 + ID_LENGTH + ID_LENGTH;
#endif
          need_to_send = 1;
          break;
        case ANSWER:
          break;
        default: break;
      }
      break;
    case DEVICE_PAIR:
      switch (recv_packet->state)
      {
        case CONTROL:
          //      strcpy(err_msg, "SmartDimmer: Manual Mode Change.");
#ifdef LONG_ID
          if (len < 6 + ID_LENGTH)
#else
          if (len < 5 + ID_LENGTH)
#endif
          {
            strcpy(err_msg, "SmartDimmer: Not enough length.");
            errn = PACKET_NOT_ENOUGH_LENGTH;
            return PACKET_NOT_ENOUGH_LENGTH;
          }
          if (recv_packet->data[0] == DEVICE_PAIRED)
          {
            strcpy(err_msg, "SmartDimmer: Pair device.");
            flash_data.dev_pair = DEVICE_PAIRED;
            flash_data_need_to_save = 1;
          }
          if (recv_packet->data[0] == DEVICE_UNPAIRED)
          {
            strcpy(err_msg, "SmartDimmer: Unpair device.");
            flash_data.dev_pair = DEVICE_UNPAIRED;
            flash_data_need_to_save = 1;
          }
          break;
        case QUERY:
          strcpy(err_msg, "SmartDimmer: Query Device Pair.");

          send_packet->start = recv_packet->start;
          send_packet->state = ANSWER;
#ifdef LONG_ID
          send_packet->dev_type = OWN_DEV_TYPE;
#endif
          memcpy(send_packet->id, flash_data.id, ID_LENGTH);
          send_packet->source = recv_packet->source;
          send_packet->data[0] = flash_data.dev_pair;
          send_packet->data[1] = 0xfe;
          trans_buff_len = 5 + ID_LENGTH;
#ifdef LONG_ID
          trans_buff_len++;
#endif
          need_to_send = 1;
          break;
        case ANSWER:
          break;
        default: break;
      }
      break;

    case DEVICE_SEARCH:
      switch (recv_packet->state)
      {
        case CONTROL:
          strcpy(err_msg, "SmartDimmer: Search cannot controlled.");
          break;
        case QUERY:
          if (flash_data.dev_pair == DEVICE_PAIRED)
          {
            strcpy(err_msg, "SmartDimmer: Paired, search unavailable.");
            break;
          }
          if (SearchDelay != SEARCH_DISABLE)
          {
            strcpy(err_msg, "SmartDimmer: Search in progress.");
            break;
          }
          if (recv_packet->source != SOURCE_BT)
          {
            switch (recv_packet->data[0])
            {
              case SEARCH_LEVEL_1:
                under100ms = ((addr_struct.minute * 60 + addr_struct.second) * 2) % 100;
                SearchDelay = ((addr_struct.minute * 60 + addr_struct.second) * 2) / 100;
                search_level = 1;
                sprintf(err_msg, "SmartDimmer: Search Level 1. 100 * %d + %d ms.", SearchDelay, under100ms);
                break;
              case SEARCH_LEVEL_2:
                under100ms = ((addr_struct.date * 24 + addr_struct.hour) * 2) % 100;
                SearchDelay = ((addr_struct.date * 24 + addr_struct.hour) * 2) / 100;
                search_level = 2;
                sprintf(err_msg, "SmartDimmer: Search Level 2. 100 * %d + %d ms.", SearchDelay, under100ms);
                break;
              case SEARCH_LEVEL_3:
                under100ms = ((addr_struct.year * 12 + addr_struct.month) * 2) % 100;
                SearchDelay = ((addr_struct.year * 12 + addr_struct.month) * 2) / 100;
                search_level = 3;
                sprintf(err_msg, "SmartDimmer: Search Level 3. 100 * %d + %d ms.", SearchDelay, under100ms);
                break;
              default:
                strcpy(err_msg, "SmartDimmer: Unknown Search Level.");
                break;
            }
          }
          else
          {
            strcpy(err_msg, "SmartDimmer: Search unavailable for bluetooth.");
            break;
          }
          break;
        case ANSWER:
          break;
        default: break;
      }
      break;

    default:
      strcpy(err_msg, "SmartDimmer: Command not supported.\n");
      errn = PACKET_FAIL;
      return PACKET_FAIL;
      break;
  }

  if (flash_data_need_to_save)
  {
    if (flash_write_buffer((char*)&flash_data, sizeof(struct FlashData)) != 0)
    {
      strcpy(err_msg, "SmartDimmer: Flash write error.\n");
      errn = FLASH_ERROR;
      return FLASH_ERROR;
    }
    flash_data_need_to_save = 0;
  }
  errn = PACKET_OK;
  return PACKET_OK;
}

int SmartDimmer::SearchProgress(void)
{
  struct Packet * send_packet = (struct Packet *)trans_buff;
  if (SearchDelay <= SEARCH_DISABLE)
  {
  }
  else if (SearchDelay == SEARCH_IDLE)
  {
    delay(under100ms);
    // send search result
    send_packet->start = DEVICE_SEARCH;
    send_packet->state = ANSWER;
#ifdef LONG_ID
    send_packet->dev_type = OWN_DEV_TYPE;
#endif
    memcpy(send_packet->id, flash_data.id, ID_LENGTH);
    send_packet->source = 0x03;
    send_packet->data[0] = search_level;
    send_packet->data[1] = 0xfe;
    trans_buff_len = 5 + ID_LENGTH;
#ifdef LONG_ID
    trans_buff_len++;
#endif
    SearchDelay = SEARCH_DISABLE;
    return 1;
  }
  else
  {
    SearchDelay--;
  }
  return 0;
}

int SmartDimmer::compareTime(int currHour, int curr_Minute, int compHour, int comp_Minute)
{
  int time_c = 0, time_w = 0;
  time_c = currHour * 60 + curr_Minute;
  time_w = compHour * 60 + comp_Minute;
  if (time_c > time_w)
  {
    return 1;
  }
  else if (time_c == time_w)
  {
    return 0;
  }
  else
  {
    return -1;
  }
}

SmartDimmer::~SmartDimmer()
{
}

