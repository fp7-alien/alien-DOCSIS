/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* 
 * File:   structs.h
 * Author: victor Fuentes
 * University of the Basque country / Universidad del Pais Vasco (UPV/EHU)
 * I2T Research Group
 */

#ifndef STRUCTS_H
#define	STRUCTS_H

using namespace std;

struct flowpath{
    uint64_t whereask;
    uint8_t longest;
    std::map<uint64_t , cflowentry*> flowmodlist;
};

struct ALHINPconfig {
    string      controller_ip;
    string      of_version;
    uint16_t    controller_port;
    
    string      listening_IP_ags;
    uint16_t    listening_ags_port;

    string      listening_IP_oui;
    uint16_t    listening_oui_port;

    string      DPS_ip;
    string      CMTS_ip;
    
    string      oui_mac;
    string      cm_mac;
    uint16_t    VLANstart;
    
    uint64_t    AGS_dpid;
    uint64_t    ALHINP_dpid;
    
    uint16_t    queue_counter;
    
};

struct ALHINPportconfig {
    uint16_t    cmts_port;
    uint16_t    data_port;
    uint16_t    dps_port;
    uint16_t    proxy_port;
    
    uint16_t    oui_netport;
    uint16_t    oui_userport;
    

};



#endif	/* STRUCTS_H */

