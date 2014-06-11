:: /**********************************************************************************************
::   Filename:       cc253x_sbl_pre.bat
::   Revised:        $Date:$
::   Revision:       $Revision:$
:: 
::   Description:
::
::   This is a Pre-build processor to build the specific SBL that the Post-build will rely upon
::   in the "ProdHex" builds.
::
::   There are 4 required arguments when invoking this batch file:
::
::   %1 must be the IAR program installation path: "$EW_DIR$"
::   
::   %2 must be the relative path of the IAR project for the SBL build, like this:
::   "$PROJ_DIR$\..\..\SerialBoot\CC253x"
::
::   %3 must be the name of the IAR project for the SBL build, like this:
::   "sbl_cc253x.ewp"
::
::   %4 must be the name of the IAR project build target for the SBL, like this:
::   "CC2533F96_I2C_HEX"
:: 
:: 
::   Copyright 2011 Texas Instruments Incorporated. All rights reserved.
:: 
::   IMPORTANT: Your use of this Software is limited to those specific rights
::   granted under the terms of a software license agreement between the user
::   who downloaded the software, his/her employer (which must be your employer)
::   and Texas Instruments Incorporated (the "License").  You may not use this
::   Software unless you agree to abide by the terms of the License. The License
::   limits your use, and you acknowledge, that the Software may not be modified,
::   copied or distributed unless embedded on a Texas Instruments microcontroller
::   or used solely and exclusively in conjunction with a Texas Instruments radio
::   frequency transceiver, which is integrated into your product.  Other than for
::   the foregoing purpose, you may not use, reproduce, copy, prepare derivative
::   works of, modify, distribute, perform, display or sell this Software and/or
::   its documentation for any purpose.
:: 
::   YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
::   PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
::   INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE, 
::   NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
::   TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
::   NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
::   LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
::   INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
::   OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCU::
::   OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
::   (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
:: 
::   Should you have any questions regarding your right to use this Software,
::   contact Texas Instruments Incorporated at www.TI.com.
:: **********************************************************************************************/

@echo off

SET bOPT=-make
SET bLOG=-log info

SET bCMD=%1"\\common\\bin\\iarbuild.exe"

chdir %2

%bCMD% %3 %bOPT% %4 %bLOG%

