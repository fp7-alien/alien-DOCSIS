/* 
 * File:   orchestrator.h
 * Author: victor
 *
 * Created on 26 de mayo de 2014, 11:06
 */

#ifndef ORCHESTRATOR_H
#define	ORCHESTRATOR_H

#include <rofl/common/openflow/cofdpt.h>
#include "../translator/translator.h"
#include "structs.h"





using namespace rofl;
class ALHINP;
class orchestrator {
    ALHINP *proxy;

private:
    uint16_t                    config_flags;
    uint16_t                    miss_send_len;

    int                         feat_req_sent;
    uint32_t                    feat_req_last_xid;
    uint32_t                    stats_xid;
    
    std::map<uint32_t,std::map <uint32_t,cofaclist*> > packoutcache;

    
public:
    orchestrator(ALHINP *ofproxy);
    orchestrator(const orchestrator& orig);
    virtual ~orchestrator();

    void OUI_connected(cofdpt* dpt);
    void OUI_disconnected(cofdpt* dpt);
    void OUI_reset_flows(cofdpt* dpt);
    void AGS_connected(cofdpt* dpt);
    void AGS_disconnected(cofdpt* dpt);    
    void AGS_reset_flows(cofdpt* dpt);
    void AGS_set_port_behavior(cofdpt* dpt);
    void OUI_set_port_behavior(cofdpt* dpt);
    void CTRL_disconnected();
    void CTRL_connected();
    
    void handle_timeout(int opaque); 
    

    
    void handle_features_request (cofctl *ctl, cofmsg_features_request *msg);
    void handle_features_reply(cofdpt* dpt, cofmsg_features_reply* msg);
    
    void handle_port_status(cofdpt* dpt, cofmsg_port_status* msg);
    void handle_port_mod (cofctl *ctl, cofmsg_port_mod *msg);
    
    void handle_get_config_request(cofctl* ctl, cofmsg_get_config_request* msg);
    void handle_set_config (cofctl *ctl, cofmsg_set_config *msg);

    void flow_mod_add(cofctl *ctl, cofmsg_flow_mod *msg);
    void flow_mod_modify(cofctl *ctl, cofmsg_flow_mod *msg, bool strict);
    void flow_mod_delete(cofctl *ctl, cofmsg_flow_mod *msg, bool strict);
cofmatch* process_matching(cofmsg_flow_mod *msg, uint8_t ofversion = OFP12_VERSION);
cofmatch* process_matching(cofmatch match, uint8_t ofversion = OFP12_VERSION);
    bool process_action_list(flowpath &flows,cofmatch* common_match,cofaclist aclist, uint8_t ofversion, uint32_t inport, uint8_t nw_proto, uint8_t message);
    void fill_flowpath(flowpath &flows,cofmatch* common_match, cofaclist aclist,uint32_t inport,uint32_t outport, uint8_t flowtype);

    
    void dispath_PACKET_IN(cofdpt *dpt, cofmsg_packet_in *msg); 
    
    void handle_packet_out (cofctl *ctl, cofmsg_packet_out *msg);
    void process_packet_out(uint32_t inport,cofaclist list, uint8_t *data,size_t datalen);
    void fill_packetouts(flowpath flows,cofaclist aclist,uint32_t inport, uint32_t outport, uint8_t flowtype);
    void flow_test(cofdpt* dpt);
    uint8_t typeflow(uint64_t src_dpid,uint64_t dst_dpid);
    
    void handle_flow_stats_request (cofctl *ctl, cofmsg_flow_stats_request *msg);
    void handle_flow_stats_reply (cofdpt *dpt, cofmsg_flow_stats_reply *msg);
    
    void handle_port_stats_request (cofctl *ctl, cofmsg_port_stats_request *msg);
    void handle_port_stats_reply   (cofdpt *dpt, cofmsg_port_stats_reply *msg);
    
    void handle_table_stats_request(cofctl *ctl, cofmsg_table_stats_request *msg);
    //void flow_mod_generator(cofmatch ofmatch,cofinlist instrlist, flow_mod_constants *constants, uint32_t inport, uint32_t outport);
    void handle_flow_removed (cofdpt *dpt, cofmsg_flow_removed *msg);
};

#endif	/* ORCHESTRATOR_H */

