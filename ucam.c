#include "ucam.h"

void send(const uint8_t* str,int len){
  int i;
  flush_uart(CAM);
  for(i =0;i<len;i++){
    write_uart_character(CAM,*str);
    str++;
  }
}

void recieve_ack(int cmdno,int pid)
{
  uint8_t str[6]; 
  int i = 0;
  for(i=0;i<6;i++){
    read_uart_character(CAM,(str+i));
  }
  uint8_t cc = (uint8_t)cmdno; 
  uint8_t pn[] = {(uint8_t)(pid>>8),(uint8_t)(pid%256)};
  uint8_t ack_ex[] = {(uint8_t)(0xAA),(uint8_t)(0x0E),cc,str[3],pn[1],pn[0]};
  assert(strcmp(ack_ex,str)==0);
}

void get_ack(int cmdno,int pid, uint8_t str[])
{
  str[0] = (uint8_t)(0xAA);
  str[1] = (uint8_t)(0x0E);
  str[2] = (uint8_t)cmdno;
  str[3] = 0;
  str[4] = (uint8_t)(pid%256);
  str[5] = (uint8_t) (pid>>8);
}


void recieve_gen(uint8_t str[],int len)
{
  int i;
  for(i = 0;i < len; i++)
  {
    read_uart_character(CAM,str+i);
  }
}

void recieve_img(int count,int limit){
  uint8_t ptr[256];
  int i=0;
  uint8_t ack_temp[6];
  get_ack(0,count,ack_temp);
  send(ack_temp, 6);
  while(1){
    read_uart_character(CAM,(ptr+i));
    i++;
    if(i==limit) break;
  }
  for(int j=4;j<i-2;j++){
    printf("%d ",(int)ptr[j]);
  }
}


int init_cam()
{
  int delay_time = 4;
  int no=0;
  const uint8_t  sync_command[] = {(uint8_t)(0xAA),(uint8_t)(0x0D), (uint8_t)(0x0),(uint8_t)(0x0),(uint8_t)(0x0),(uint8_t)(0x0)};
  const uint8_t  ack_command[] = {(uint8_t)(0xAA),(uint8_t)(0x0E), (uint8_t)(0x0D),(uint8_t)(0x0),(uint8_t)(0x0),(uint8_t)(0x0)};
  const uint8_t  ack_command_reply[] = {(uint8_t)(0xAA),(uint8_t)(0x0E), (uint8_t)(0x0),(uint8_t)(0x0),(uint8_t)(0x0),(uint8_t)(0x0)};
  start:
  {
    no++;
    printf("Round : %d\n",no);
    send(sync_command,6);
    delay_milli(delay_time++); 
    if(CAM->status & STS_RX_NOT_EMPTY)
    {
        // code to read 12 characters from the buffer and if it matches with the ACK and sync commands go to succes
        int i;
        for(i = 0;i<6;i++) 
        {
           uint8_t ch;
           read_uart_character(CAM,&ch);
           if(i == 3)continue; // don't care xx thing
           if(ch != ack_command[i]) goto fail;
        }
        for(i = 0;i<6;i++)
        {
            uint8_t ch;
            read_uart_character(CAM,&ch);
            if(ch != sync_command[i]) goto fail;
        }
        goto success;
    }
  }
  fail:
  if(delay_time>15000) {
    printf("Can't wake up camera! Is camera connected?\n");
    return -1;
  }
  goto start;
  success:
  flush_uart(CAM);
  send(ack_command_reply,6);
  printf("\n synchronisation process done waiting for 2 seconds for stabilisation \n");
  delay(2);
  printf("\n you are all set \n");
  uint8_t reset_command[] = {(uint8_t)(0xAA),(uint8_t)(0x08),(uint8_t)(0x1),(uint8_t)(0x0),(uint8_t)(0x0),(uint8_t)(0x0)};
  send(reset_command,6);
  recieve_ack(8,0);
  return 0;
}

void get_pic()
{

  //INITIAL_command : AA 01 00 07 xx 07 ( specific for JPEG,640x480 xx=don't care )
  uint8_t initial_command[] = {(uint8_t)(0xAA),(uint8_t)(0x1),(uint8_t)(0x0),(uint8_t)(0x7),(uint8_t)(0x3),(uint8_t)(0x7)};
  send(initial_command,6);
  recieve_ack(1,0);

    //SET_PACKAGE_SIZE_command : AA 06 08 00 01 00 (size=256)
  uint8_t set_package_command[] = {(uint8_t)(0xAA),(uint8_t)(0x6),(uint8_t)(0x8),(uint8_t)(0x0),(uint8_t)(0x1),(uint8_t)(0x0)};
  send(set_package_command,6);
  recieve_ack(6,0);

  //GET_PICTURE_command : AA 04 05 00 00 00 ( Current JPEG  )
  uint8_t get_pic_command[] = {(uint8_t)(0xAA),(uint8_t)(0x4),(uint8_t)(0x5),(uint8_t)(0x0),(uint8_t)(0x0),(uint8_t)(0x0)};
  send(get_pic_command,6);
  recieve_ack(4,0);
  uint8_t data[6];
  recieve_gen(data,6);
  //DATA_command : AA 0A 05 xx yy zz ( current JPEG mode )
  assert(data[0]==0xAA);
  assert(data[1]==0xA);
  assert(data[2]==0x5);
  long no_of_pg = (int)data[3] + (((long)data[4])<<8) + (((long)data[5])<<16) ;
  printf("Image size in bytes : %d\n",no_of_pg);
  int counter = no_of_pg/250 ;
  int i = 0;
  printf("\n\n");
  for(i=0;i<counter;i++){
    recieve_img(i,256);
  }
  recieve_img(counter,6+(no_of_pg%250));
}

