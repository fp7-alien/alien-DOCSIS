/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* 
 * File:   ALHINP.h
 * Author: victor Fuentes
 * University of the Basque country / Universidad del Pais Vasco (UPV/EHU)
 * I2T Research Group
 */

#ifndef ALHINP_H
#define	ALHINP_H

#define __STDC_FORMAT_MACROS

#include <map>
#include <rofl/common/cmacaddr.h>
#include <rofl/common/caddress.h>
#include <rofl/common/crofbase.h>
#include "../orchestrator/orchestrator.h"
#include "../discovery/discovery.h"
#include "../DOCSIS/DOCSISdriver.h"
#include "../orchestrator/Flowcache.h"
#include "../QoS/QoS.h"

#include <libconfig.h++>
#include <iostream>
#include <iomanip>
#include <cstdlib>

#include <list>

#define TESTING 1;

using namespace rofl;
using namespace libconfig;

static bool exec_mode;

class ALHINP : public crofbase {
    
    friend orchestrator;
    friend discovery;
    friend Flowcache;
    friend translator;
    friend QoS;
    
private:
    
    discovery* discover;
    orchestrator* manager; 
    cofctl* controller;
    DOCSISdriver docsis;
    translator* virtualizer;
    Flowcache* flowcache;
    ALHINPconfig config;
    ALHINPportconfig portconfig;
    QoS* qosmap;
    
    static ALHINP *singleton;
    static bool instanceFlag;
 
public:
    ALHINP();
    ALHINP(char* configfile);
    ALHINP(const ALHINP& orig);
    virtual ~ALHINP();
    //void install_flowentry();
    static void testing();
    void test();
    static ALHINP* getInstance();

private:
    /**
 * @brief Config File parsing function with format as described in
 * @param file
 * @return 
 */
    
    int parse_config_file(char* file);
    int parse_qos_file(char* file);    
    virtual void handle_timeout(int opaque);
    virtual void handle_dpath_open(cofdpt *dpt);
    virtual void handle_dpath_close(cofdpt *dpt);
    virtual void handle_ctrl_open(cofctl *ctl);
    virtual void handle_ctrl_close(cofctl *ctl);

//  
//    //SOUTHBOUND INTERFACE
    virtual void handle_features_request (cofctl *ctl, cofmsg_features_request *msg);
    virtual void handle_features_reply(cofdpt *dpt, cofmsg_features_reply *msg);   
    
    virtual void handle_get_config_request (cofctl *ctl, cofmsg_get_config_request *msg);    
    virtual void handle_get_config_reply (cofdpt *dpt, cofmsg_get_config_reply *msg);
    virtual void handle_set_config (cofctl *ctl, cofmsg_set_config *msg);
    
    virtual void handle_port_mod (cofctl *ctl, cofmsg_port_mod *msg);
    virtual void handle_port_status (cofdpt *dpt, cofmsg_port_status *msg);
    
    virtual void handle_packet_in(cofdpt *dpt, cofmsg_packet_in *msg);
    virtual void handle_flow_mod (cofctl *ctl, cofmsg_flow_mod *msg);
    virtual void handle_flow_removed (cofdpt *dpt, cofmsg_flow_removed *msg);
    virtual void handle_packet_out (cofctl *ctl, cofmsg_packet_out *msg);
    
    virtual void handle_barrier_request (cofctl *ctl, cofmsg_barrier_request *msg); 
//  virtual void handle_barrier_reply (cofdpt *dpt, cofmsg_barrier_request *msg);
//  virtual void handle_barrier_reply_timeout(cofdpt *dpt, uint32_t xid);    
    
    virtual void handle_flow_stats_request (cofctl *ctl, cofmsg_flow_stats_request *msg);    
    virtual void handle_flow_stats_reply (cofdpt *dpt, cofmsg_flow_stats_reply *msg);
    
    virtual void handle_port_stats_request(cofctl* ctl, cofmsg_port_stats_request* msg);
    virtual void handle_port_stats_reply (cofdpt *dpt, cofmsg_port_stats_reply *msg);
    
    virtual void handle_table_stats_request (cofctl *ctl, cofmsg_table_stats_request *msg);
    virtual void handle_desc_stats_request (cofctl *ctl, cofmsg_desc_stats_request *msg);
    
//  virtual void handle_error (cofdpt *dpt, cofmsg_error *msg);

  virtual void handle_queue_get_config_request (cofctl *ctl, cofmsg_queue_get_config_request *msg);    
  //virtual void handle_queue_get_config_reply (cofdpt *dpt, cofmsg_queue_get_config_reply *msg);

//  virtual void handle_aggregate_stats_reply(cofdpt* dpt, cofmsg_aggr_stats_reply* msg);
//  virtual void handle_aggregate_stats_request (cofctl *ctl, cofmsg_aggr_stats_request *msg);

//  virtual void handle_table_stats_reply (cofdpt *dpt, cofmsg_table_stats_reply *msg);

//  virtual void handle_stats_request(cofctl* ctl, cofmsg_stats_request *msg);    
    virtual void handle_stats_reply (cofdpt *dpt, cofmsg_stats_reply *msg); 
    virtual void handle_experimenter_message(cofctl* ctl, cofmsg_experimenter* msg);



    
};




#endif	/* ALHINP_H */

