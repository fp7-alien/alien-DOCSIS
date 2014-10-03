/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* 
 * File:   CMTS_handler.cpp
 * Author: victor Fuentes
 * University of the Basque country / Universidad del Pais Vasco (UPV/EHU)
 * I2T Research Group
 */

#include "CMTS_handler.h"
#include <unistd.h>
#include <iostream>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <sstream>

#define ScriptPath
#define AsignVlanScript

//using namespace std;

CMTS_handler::CMTS_handler() {
}

CMTS_handler::CMTS_handler(const CMTS_handler& orig) {
}

CMTS_handler::~CMTS_handler() {
}

void CMTS_handler::L2VPN_enable(char* mac,uint16_t vlan_vid,uint8_t interface){

    std::string string2;
    string2 = mac;
    std::string::iterator end_pos = std::remove(string2.begin(), string2.end(),':');
    string2.erase(end_pos, string2.end());
    string2.insert(4,".");
    string2.insert(9,".");

    
    
    //std::cout << pid << std::endl;
    
    std::string command;
    command = "tclsh assign_vlan.tcl";
    command.append(" ");
    command.append(string2);
    command.append(" ");
    std::ostringstream convert;
    convert << vlan_vid;
    command.append(convert.str());
    command.append(" &");

    std::cout <<"Command over CMTS: "<<command << "\n";
    
    
    //int result =system(command.c_str());

}

void CMTS_handler::reboot_CM(char* mac) {
    
    
    std::string formated_mac;
    formated_mac = mac;
    std::string::iterator end_pos = std::remove(formated_mac.begin(), formated_mac.end(),':');
    formated_mac.erase(end_pos, formated_mac.end());
    formated_mac.insert(4,".");
    formated_mac.insert(9,".");
    
    std::string command;
    command = "tclsh reboot_CM.tcl";
    command.append(" ");
    command.append(formated_mac);
    std::cout <<"Command REBOOT sent over CMTS: "<<command << "\n";
    
    
    //int result =system(command.c_str());
}
