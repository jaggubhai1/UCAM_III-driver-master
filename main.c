#include <stdio.h>
#include "camera/ucam.h"
#include "pinmux.h"
int main(void) {
  *pinmux_config_reg = 0x5;
  int brate = 115200;
  set_baud_rate(uart_instance[1],brate);
  if( init_cam() < 0) 
      return -1;
  get_pic();
  return 0;
}



/*
Details about communication( μ-CAM III ) :

Commands : ( All commands 6 bytes )

1) SYNC_command : AA 0D 00 00 00 00  
2) NAK_command : AA 0F 00 xx ee 00
      xx = counter // let’s ignore as of now
      ee = error code
3)  SET_BAUDRATE_command : AA 07 31 00 00 00  ( specific for 115200 )
4)  INITIAL_command : AA 01 00 07 xx 07 ( specific for JPEG,640x480 )
      xx = don't care
5)  RESET_command : AA 08 tt 00 00 xx
      tt = 00 ……...resets fully
      tt = 01 ……...resets only state machine
      xx = FF then responds immediately
6)  SET_PACKAGE_SIZE_command : AA 06 08 00 xx 00 
            (a)  xx = higher bits of img.size . 01 for (1<<8) size.
7)  SNAPSHOT_command : AA 05 00 00 00 00 ( jpg format, keep current frame)
8)  GET_PICTURE_command : AA 04 05 00 00 00 ( current jpg mode )
9)  DATA_command : AA 0A 05 xx yy zz ( current jpg mode )
            (a)  zzyyxx is binary rep. of image size
      Used for calculating no. of packages = Img.size / (packagesize-6)
10)  ACK_command : AA 0E cc xx pn pn 
            (a) cc = command number ( when sent from ucam as an acknowledgement )
            (b) xx = counter ( let’s ignore )
            (c) pn pn = package id when sent from host to receive the kth package from ucam
                              = 00 00 , in all other cases


What all we have to do :

Start by sending upto 60 SYNC_commands
ucam replies with an ACK_command and one SYNC_command  
Host sends one more ACK_command and this ends the starting 

For getting image, we shall always use snapshot mode

Send INITIAL_command, get ACK from ucam
Send SET_PACKAGE_SIZE_command, get ACK from ucam
Send SNAPSHOT_command, get ACK from ucam
Send GET_PICTURE_command, get ACK and DATA_command from ucam
Send an ACK_command with pnpn = 0000
Calculate no. of packages :
    For i from 1 to no. of packages :
get one package from ucam with id = i , Send ACK with pnpn = i 




*/
