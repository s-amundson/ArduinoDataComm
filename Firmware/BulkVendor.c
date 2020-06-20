/*
             LUFA Library
     Copyright (C) Dean Camera, 2017.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2017  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the Bulk Vendor demo. This file contains the main tasks of the demo and
 *  is responsible for the initial application hardware configuration.
 */

#define  INCLUDE_FROM_BULKVENDOR_C
#include "BulkVendor.h"


/** Circular buffer to hold data from the host before it is sent to the device via the serial port. */
static RingBuffer_t USBtoUSART_Buffer;

/** Underlying data buffer for \ref USBtoUSART_Buffer, where the stored bytes are located. */
static uint8_t      USBtoUSART_Buffer_Data[128];

/** Circular buffer to hold data from the serial port before it is sent to the host. */
static RingBuffer_t USARTtoUSB_Buffer;

/** Underlying data buffer for \ref USARTtoUSB_Buffer, where the stored bytes are located. */
static uint8_t      USARTtoUSB_Buffer_Data[128];
//static uint8_t      Send_Buffer_Data[128];
/** length of data to be sent to host **/
//uint16_t Incoming_Data_Size = 0;
static uint8_t      Size_Buffer[2];
static uint8_t      ReceivedData[VENDOR_IO_EPSIZE];
/** Main program entry point. This routine configures the hardware required by the application, then
 *  enters a loop to run the application tasks in sequence.
 */
int main(void)
{
	SetupHardware();

	RingBuffer_InitBuffer(&USBtoUSART_Buffer, USBtoUSART_Buffer_Data, sizeof(USBtoUSART_Buffer_Data));
	RingBuffer_InitBuffer(&USARTtoUSB_Buffer, USARTtoUSB_Buffer_Data, sizeof(USARTtoUSB_Buffer_Data));

//	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	LEDs_TurnOffLEDs(LEDS_ALL_LEDS);
	GlobalInterruptEnable();
	Serial_Init(9600, false);

	for (;;)
	{
		USB_USBTask();

//		uint8_t ReceivedData[VENDOR_IO_EPSIZE];
		memset(ReceivedData, 0x00, sizeof(ReceivedData));

		Endpoint_SelectEndpoint(VENDOR_OUT_EPADDR);
		if (Endpoint_IsOUTReceived())
		{
//			LEDs_SetAllLEDs(LEDMASK_RX);
			Endpoint_Read_Stream_LE(ReceivedData, VENDOR_IO_EPSIZE, NULL);
			Endpoint_ClearOUT();

            USART_Package();


		}
		USART_Tasks();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
#if (ARCH == ARCH_AVR8)
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
#elif (ARCH == ARCH_XMEGA)
	/* Start the PLL to multiply the 2MHz RC oscillator to 32MHz and switch the CPU core to run from it */
	XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
	XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);

	/* Start the 32MHz internal RC oscillator and start the DFLL to increase it to 48MHz using the USB SOF as a reference */
	XMEGACLK_StartInternalOscillator(CLOCK_SRC_INT_RC32MHZ);
	XMEGACLK_StartDFLL(CLOCK_SRC_INT_RC32MHZ, DFLL_REF_INT_USBSOF, F_USB);

	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
#endif

	/* Hardware Initialization */
	LEDs_Init();
	USB_Init();
}

/** Event handler for the USB_Connect event. This indicates that the device is enumerating via the status LEDs. */
void EVENT_USB_Device_Connect(void)
{
	/* Indicate USB enumerating */
//	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
	LEDs_TurnOffLEDs(LEDS_ALL_LEDS);
}

/** Event handler for the USB_Disconnect event. This indicates that the device is no longer connected to a host via
 *  the status LEDs.
 */
void EVENT_USB_Device_Disconnect(void)
{
	/* Indicate USB not ready */
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the USB_ConfigurationChanged event. This is fired when the host set the current configuration
 *  of the USB device after enumeration - the device endpoints are configured.
 */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	/* Setup Vendor Data Endpoints */
	ConfigSuccess &= Endpoint_ConfigureEndpoint(VENDOR_IN_EPADDR,  EP_TYPE_BULK, VENDOR_IO_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(VENDOR_OUT_EPADDR, EP_TYPE_BULK, VENDOR_IO_EPSIZE, 1);

	/* Indicate endpoint configuration success or failure */
//	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
	LEDs_TurnOffLEDs(LEDS_ALL_LEDS);
}

/** Event handler for the USB_ControlRequest event. This is used to catch and process control requests sent to
 *  the device from the USB host before passing along unhandled control requests to the library for processing
 *  internally.
 */
void EVENT_USB_Device_ControlRequest(void)
{
	// Process vendor specific control requests here
	{
		if(((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_TYPE)== REQTYPE_VENDOR)
				&& ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_RECIPIENT)== REQREC_DEVICE))
		{
			if ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_DIRECTION)== REQDIR_HOSTTODEVICE)
			{
				switch(USB_ControlRequest.bRequest)
				{
				    case CONTROL_SEND_SIZE:
				        // report to the host how much data will be sent.
				        Endpoint_ClearSETUP();
				        /*read data from endpoint*/
				        Endpoint_Read_Control_Stream_LE(Size_Buffer, 2);
				        /*and mark the whole request as successful:*/
				        Endpoint_ClearStatusStage();
				        break;

				    case CONTROL_SEND_USART:
//				        uint16_t ReceivedData[8];
				        Endpoint_ClearSETUP();
				        /*read data from endpoint*/
				        Endpoint_Read_Control_Stream_LE(ReceivedData, 64);
				        /*and mark the whole request as successful:*/
				        Endpoint_ClearStatusStage();
				        USART_Package();
				        break;

				}
////					case CMD_REQUEST_DATA:
////                        Endpoint_ClearSETUP();
////                        Endpoint_ClearOUT();
////
////                        /*wait for the final IN token:*/
////                        while (!(Endpoint_IsINReady()));
////
////                        /*and mark the whole request as successful:*/
////                        Endpoint_ClearIN();
////					    for (int i = 0; i < USB_ControlRequest.wLength; i++)
////                        {
////                            if (!(RingBuffer_IsEmpty(&USBtoUSART_Buffer)))
////                            {
////                                Send_Buffer_Data[i] = RingBuffer_Remove(&USARTtoUSB_Buffer);
////                            }
////                        }
//////                        TODO add for loop to for if buffer bigger then EP size
//////            			LEDs_SetAllLEDs(LEDMASK_TX);
////                        Endpoint_SelectEndpoint(VENDOR_IN_EPADDR);
////                        Endpoint_Write_Stream_LE(Send_Buffer_Data, VENDOR_IO_EPSIZE, NULL);
////                        Endpoint_ClearIN();
////                        LEDs_TurnOffLEDs(LEDS_ALL_LEDS);
////						break;
////					case SERVO_CMD_SETALL:
////						process_SERVO_CMD_SETALL();
////						break;
//				}
			} else
			{
				switch(USB_ControlRequest.bRequest)
				{
				case CMD_DATA_AVAILABLE:
					LEDs_SetAllLEDs(LEDMASK_TX);

					Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);

					Endpoint_ClearSETUP();
					Size_Buffer[0] = RingBuffer_GetCount(&USARTtoUSB_Buffer);
					//, USARTtoUSB_Buffer_Data_Size};
					Endpoint_Write_Control_Stream_LE(&Size_Buffer, 2);

					Endpoint_ClearStatusStage();
					Delay_MS(100);
					LEDs_TurnOffLEDs(LEDS_ALL_LEDS);
					break;

                case CMD_REQUEST_DATA:
                    Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);
                    Endpoint_ClearSETUP();
                    uint8_t Send_Buffer_Data[USB_ControlRequest.wLength];

                    for (int i = 0; i < USB_ControlRequest.wLength; i++)
                    {
                        if (!(RingBuffer_IsEmpty(&USBtoUSART_Buffer)))
                        {
                            Send_Buffer_Data[i] = RingBuffer_Remove(&USARTtoUSB_Buffer);
                        }
                    }

                    Endpoint_Write_Control_Stream_LE(&Send_Buffer_Data, USB_ControlRequest.wLength);
//                        LEDs_TurnOffLEDs(LEDS_ALL_LEDS);
//
////                        Endpoint_ClearIN();
//                        /*and mark the whole request as successful:*/
                    Endpoint_ClearStatusStage();
                    break;
                }
			}
		}
	}
}
void USART_Package()
{
    // put package reeieved into USART buffer to be sent.
    int package_len = ReceivedData[0]; // << 4 + ReceivedData[1];

    if (package_len > 62)
    {
        package_len = 62;
    }
    for (int i = 0; i < package_len; i++)
    {
        if (!RingBuffer_IsFull(&USBtoUSART_Buffer))
        {
            RingBuffer_Insert(&USBtoUSART_Buffer, ReceivedData[i + 2]);
        }
    }
}
void USART_Tasks()
{
    if (Serial_IsSendReady() && !(RingBuffer_IsEmpty(&USBtoUSART_Buffer)))
    {
        LEDs_SetAllLEDs(LEDMASK_TX);
        Serial_SendByte(RingBuffer_Remove(&USBtoUSART_Buffer));
        //Delay_MS(100);
        LEDs_TurnOffLEDs(LEDS_ALL_LEDS);
    } else {
//			LEDs_SetAllLEDs(LEDS_ALL_LEDS);
    }

    if (Serial_IsCharReceived() && !(RingBuffer_IsFull(&USARTtoUSB_Buffer)))
    {
//        int16_t DataByte = Serial_ReceiveByte();
        int8_t DataByte = Serial_ReceiveByte();
        LEDs_SetAllLEDs(LEDMASK_RX);
        RingBuffer_Insert(&USARTtoUSB_Buffer, DataByte);
		LEDs_TurnOffLEDs(LEDS_ALL_LEDS);
    }
}
