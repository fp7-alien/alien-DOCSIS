/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* 
 * File:   DOCSISdriver.h
 * Author: victor
 *
 * Created on 26 de mayo de 2014, 11:08
 */

#ifndef DOCSISDRIVER_H
#define	DOCSISDRIVER_H

#include "CMTS/CMTS_handler.h"
#include <vector>
#include "../QoS/QoS.h"

class DOCSISdriver {
    CMTS_handler cisco;
public:
    DOCSISdriver();
    DOCSISdriver(const DOCSISdriver& orig);
    virtual ~DOCSISdriver();
    int enable_L2VPN(char* mac,uint16_t vlan);
    void push_config_file(const char* mac, std::vector<QueueList>queues);
    void push_default_config_file(const char* mac);
    void create_SF(uint16_t vlan_vid,std::string direction);
};

#endif	/* DOCSISDRIVER_H */

