#ifndef _SmartDimmer_hpp_
#define _SmartDimmer_hpp_

#define PACKET_OK                       1
#define PACKET_FAIL                     0
#define PACKET_NOT_OWN                  -1
#define PACKET_INVALID_ID               -2
#define PACKET_NOT_ENOUGH_LENGTH        -3
#define FLASH_ERROR                     -4

#include <Arduino.h>
#include <EEPROM.h>

#define BUFF_SIZE 32

#ifdef LONG_ID
#define ID_LENGTH       4
extern const unsigned char BROADCAST_ID[ID_LENGTH];
extern const unsigned char CONNECT_BOX_ID[ID_LENGTH];
extern const unsigned char DEFAULT_ID[ID_LENGTH];
#else
#define ID_LENGTH       1
extern const unsigned char BROADCAST_ID[ID_LENGTH];
extern const unsigned char CONNECT_BOX_ID[ID_LENGTH];
extern const unsigned char DEFAULT_ID[ID_LENGTH];
#endif

class SmartDimmer
{
  public :
    unsigned char trans_buff[BUFF_SIZE];
    unsigned char trans_buff_len;
    unsigned char need_to_send;

    char err_msg[];
    int errn;

    int ProcessPacket(char * packet, unsigned char len);
    int compareTime(int currHour, int curr_Minute, int compHour, int comp_Minute);
    int SearchProgress(void);

    SmartDimmer();
    ~SmartDimmer();
};

#endif

