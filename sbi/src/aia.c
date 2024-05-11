/**
 * @file aia.c
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Advanced Interrupt Architecture (AIA) routines.
 * @version 0.1
 * @date 2023-03-10
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <aia.h>

// APLIC is connected to 0x0c00_0000 on 'virt' machine for M-mode.
#define APLIC_M_BASE            0x0c000000UL

// APLIC is connected to 0x0d00_0000 on 'virt' machine for S-mode.
#define APLIC_S_BASE            0x0d000000UL

// IMSIC is connected to 0x2400_0000 on 'virt' machine for M-mode.
#define IMSIC_M_BASE            0x24000000UL

// IMSIC is connected to 0x2400_0000 on 'virt' machine for S-mode.
#define IMSIC_S_BASE            0x28000000UL

#pragma pack(push, 1)
typedef struct AplicRegs {
    uint32_t domaincfg;
    uint32_t sourcecfg[1023];
    uint8_t _reserved1[0xBC0];

    uint32_t mmsiaddrcfg;
    uint32_t mmsiaddrcfgh;
    uint32_t smsiaddrcfg;
    uint32_t smsiaddrcfgh;
    uint8_t _reserved2[0x30];

    uint32_t setip[32];
    uint8_t _reserved3[92];

    uint32_t setipnum;
    uint8_t _reserved4[0x20];

    uint32_t in_clrip[32];
    uint8_t _reserved5[92];

    uint32_t clripnum;
    uint8_t _reserved6[32];

    uint32_t setie[32];
    uint8_t _reserved7[92];

    uint32_t setienum;
    uint8_t _reserved8[32];

    uint32_t clrie[32];
    uint8_t _reserved9[92];

    uint32_t clrienum;
    uint8_t _reserved10[32];

    uint32_t setipnum_le;
    uint32_t setipnum_be;
    uint8_t _reserved11[4088];

    uint32_t genmsi;
    uint32_t target[1023];
} AplicRegs;
#pragma pack(pop)

#define XAPLIC(xa)  ((volatile AplicRegs *)(xa))

Aplic *aplic_get(AplicModes mode) {
    return (Aplic *)mode;
}

void aplic_set_domaincfg(Aplic *aplic, uint32_t domaincfg) {
    XAPLIC(aplic)->domaincfg = domaincfg;
}


