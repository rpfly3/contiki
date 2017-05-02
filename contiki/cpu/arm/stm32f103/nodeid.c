/*----------------------------------------------------------------------------
 * Name:    Hello.c
 * Purpose: Hello World Example
 * Note(s):
 *----------------------------------------------------------------------------
 * This file is part of the uVision/ARM development tools.
 * This software may only be used under the terms of a valid, current,
 * end user licence from KEIL for a compatible version of KEIL software
 * development tools. Nothing else gives you the right to use this software.
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 * Copyright (c) 2012 Keil - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/

#include <stdio.h> 
#include "eeprom.h"
#include "debug.h"

uint16_t VirtAddVarTab[NumbOfVar]={0x5555};

uint8_t getNodeId()
{ 
	uint16_t moteid;

	FLASH_Unlock();
	EE_Init();
	
	uint16_t status= EE_ReadVariable(VirtAddVarTab[0], &moteid);
	if( status != 0)
	{
		log_error("Read moteid failed");
	}
	return (uint8_t)moteid;
}
