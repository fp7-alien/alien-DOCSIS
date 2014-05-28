/* 
 * File:   ALHINP.h
 * Author: victor
 *
 * 
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

#include <list>

using namespace rofl;

class ALHINP : public crofbase {
    
    friend orchestrator;
    
private:
    
    discovery network;
    orchestrator manager; 
    cofctl* controller;
    
public:
    ALHINP();
    ALHINP(const ALHINP& orig);
    virtual ~ALHINP();
    
private:

    
//    virtual void handle_timeout(int opaque);
    virtual void handle_dpath_open(cofdpt *dpt);
    virtual void handle_dpath_close(cofdpt *dpt);
    virtual void handle_ctrl_open(cofctl *ctl);
    virtual void handle_ctrl_close(cofctl *ctl);
//  
//    //SOUTHBOUND INTERFACE
//    virtual void handle_aggregate_stats_reply(cofdpt* dpt, cofmsg_aggr_stats_reply* msg);
//    virtual void handle_barrier_reply (cofdpt *dpt, cofmsg_barrier_request *msg);
//    virtual void handle_barrier_reply_timeout(cofdpt *dpt, uint32_t xid);
//
//    virtual void handle_desc_stats_reply (cofdpt *dpt, cofmsg_desc_stats_reply *msg);
//    virtual void handle_error (cofdpt *dpt, cofmsg_error *msg);
//    virtual void handle_features_reply(cofdpt *dpt, cofmsg_features_reply *msg);
//    virtual void handle_flow_removed (cofdpt *dpt, cofmsg_flow_removed *msg);
//
//    virtual void handle_flow_stats_reply (cofdpt *dpt, cofmsg_flow_stats_reply *msg);
//    virtual void handle_get_config_reply (cofdpt *dpt, cofmsg_get_config_reply *msg);
    virtual void handle_packet_in(cofdpt *dpt, cofmsg_packet_in *msg);
    void send_packet_in_ctrl(uint32_t buffer_id, uint16_t total_len,uint8_t reason,uint8_t table_id,uint64_t cookie,uint16_t in_port, cofmatch &match,uint8_t *data,size_t datalen);
//    virtual void handle_port_stats_reply (cofdpt *dpt, cofmsg_port_stats_reply *msg);
//    virtual void handle_port_status (cofdpt *dpt, cofmsg_port_status *msg);
//    virtual void handle_queue_get_config_reply (cofdpt *dpt, cofmsg_queue_get_config_reply *msg);
//    virtual void handle_stats_reply (cofdpt *dpt, cofmsg_stats_reply *msg);
//    virtual void handle_table_stats_reply (cofdpt *dpt, cofmsg_table_stats_reply *msg);
//
//    //NORTHBOUND INTERFACE: INTERFACING A CONTROLLER
//    virtual void handle_features_request (cofctl *ctl, cofmsg_features_request *msg);
//    virtual void handle_set_config (cofctl *ctl, cofmsg_set_config *msg);
//    virtual void handle_aggregate_stats_request (cofctl *ctl, cofmsg_aggr_stats_request *msg);
//    virtual void handle_barrier_request (cofctl *ctl, cofmsg_barrier_request *msg);
//    virtual void handle_flow_mod (cofctl *ctl, cofmsg_flow_mod *msg);
//    virtual void handle_flow_stats_request (cofctl *ctl, cofmsg_flow_stats_request *msg);
//    virtual void handle_get_config_request (cofctl *ctl, cofmsg_get_config_request *msg);
//    virtual void handle_packet_out (cofctl *ctl, cofmsg_packet_out *msg);
//    virtual void handle_port_mod (cofctl *ctl, cofmsg_port_mod *msg);
//    virtual void handle_queue_get_config_request (cofctl *ctl, cofmsg_queue_get_config_request *msg);
//    virtual void handle_role_request (cofctl *ctl, cofmsg_role_request *msg);
//    virtual void handle_table_mod (cofctl *ctl, cofmsg_table_mod *msg);
//    virtual void handle_table_stats_request (cofctl *ctl, cofmsg_table_stats_request *msg);
//    virtual void handle_group_mod (cofctl *ctl, cofmsg_group_mod *msg);
//    //virtual void handle_stats_request(cofctl* ctl, cofmsg_stats_request *msg);
//    virtual void handle_port_stats_request(cofctl* ctl, cofmsg_port_stats_request* msg);

};

#endif	/* ALHINP_H */
