/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* 
 * File:   CMTS_handler.h
 * Author: victor Fuentes
 * University of the Basque country / Universidad del Pais Vasco (UPV/EHU)
 * I2T Research Group
 */

#ifndef CMTS_HANDLER_H
#define	CMTS_HANDLER_H

#include <inttypes.h>

class CMTS_handler {
public:
    CMTS_handler();
    CMTS_handler(const CMTS_handler& orig);
    void L2VPN_enable(char* mac,uint16_t vlan_vid,uint8_t interface);
    void reboot_CM(char* mac);
    virtual ~CMTS_handler();
private:

};

#endif	/* CMTS_HANDLER_H */

