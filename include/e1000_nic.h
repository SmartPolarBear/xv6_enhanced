//
// Created by bear on 10/23/2022.
//

#pragma once

#include "types.h"

#define E1000_CTRL 0x00000 // Device Control - RW
#define E1000_CTRL_SLU 0x40 // Set link up
#define E1000_CTRL_ASDE 0x20 // Auto speed detect enable
#define E1000_CTRL_SWDPIO 0x10 // SW Defineable Power Indication
#define E1000_CTRL_RST 0x04000000 // Global reset
#define E1000_CTRL_RFCE 0x08000000 // Receive Flow Control enable
#define E1000_CTRL_TFCE 0x10000000 // Transmit flow control enable
#define E1000_CTRL_VME 0x40000000 // IEEE VLAN mode enable
#define E1000_CTRL_PHY_RST 0x80000000 // PHY reset

#define E1000_STATUS 0x00008 // Device Status - RO

#define E1000_EECD 0x00010 // EEPROM/Flash Control - RW

#define E1000_EERD 0x00014 // EEPROM Read - RW
#define E1000_EERD_START 0x1 // Start Read
#define E1000_EERD_DONE 0x10 // Read Done

#define E1000_EEPROM_ETHERNET_ADDR_2_1 0x00
#define E1000_EEPROM_ETHERNET_ADDR_4_3 0x01
#define E1000_EEPROM_ETHERNET_ADDR_6_5 0x02

#define E1000_CTRL_EXT 0x00018 // Extended Device Control - RW
#define E1000_FLA 0x0001C // Flash Access - RW
#define E1000_MDIC 0x00020 // MDI Control - RW
#define E1000_SCTL 0x00024 // SerDes Control - RW
#define E1000_FCAL 0x00028 // Flow Control Address Low - RW
#define E1000_FCAH 0x0002C // Flow Control Address High -RW

#define E1000_TCTL 0x00400/4
#define TCTL_EN 0x2
#define TCTL_PSP 0x8
#define TCTL_CT 0xFF0
#define TCTL_COLD 0x3FF000
#define E1000_TIPG 0x00410/4

#define E1000_TDBAL 0x03800/4
#define E1000_TDBAH 0x03804/4
#define E1000_TDLEN 0x03808/4
#define E1000_TDH   0x03810/4
#define E1000_TDT   0x03818/4

// command
#define TXD_STAT_DD 0x01
#define TXD_CMD_RS  0x08
#define TXD_CMD_EOP 0x01

// recv
#define RXD_STAT_DD       0x01
#define RXD_STAT_EOP      0x02

#define E1000_RCTL     0x00100/4
#define RCTL_EN             0x00000002
#define RCTL_LPE            0x00000020
#define RCTL_LBM_NO         0x00000000
#define RCTL_BAM            0x00008000
#define RCTL_SZ_2048     	0x00030000
#define RCTL_SECRC          0x04000000
#define E1000_RDBAL    0x02800/4
#define E1000_RDBAH    0x02804/4
#define E1000_RDLEN    0x02808/4
#define E1000_RDH      0x02810/4
#define E1000_RDT      0x02818/4
#define E1000_MTA      0x05200/4
#define E1000_RA       0x05400/4
#define E1000_RAV      0x80000000
#define E1000_IMS      0x000D0/4

struct TD //tx_desc
{
	uint64 addr;
	uint16 length;
	uint8 cso;
	uint8 cmd;
	uint8 status;
	uint8 css;
	uint16 special;
}__attribute__((packed));

struct RD
{
	uint64 addr;
	uint16 length;
	uint16 checksum;
	uint8 status;
	uint8 errors;
	uint16 special;
}__attribute__((packed));

#define PKTSIZE 2048
struct packet
{
	char buf[PKTSIZE];
};

#define NE1000CARDS 8

