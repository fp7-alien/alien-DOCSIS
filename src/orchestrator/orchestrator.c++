/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* 
 * File:   orchestrator.c++
 * Author: victor Fuentes
 * University of the Basque country / Universidad del Pais Vasco (UPV/EHU)
 * I2T Research Group
 */

#include <rofl/common/crofbase.h>
#include "orchestrator.h"
#include "../proxy/ALHINP.h"
#include "../discovery/discovery.h"
#include <rofl/datapath/pipeline/openflow/openflow1x/pipeline/of1x_pipeline.h>
#include <rofl/common/openflow/cofmatch.h>


#define __STDC_FORMAT_MACROS

#define C_TAG 0x8100
#define S_TAG 0x88A8

#define takefromoutput 0xAAAAAAAA
#define qos_default 4090


orchestrator::orchestrator(ALHINP *ofproxy) {
    proxy=ofproxy;
    config_flags=0;
    miss_send_len=OF1X_DEFAULT_MISS_SEND_LEN;
    feat_req_sent=0;
    feat_req_last_xid=0;
    stats_xid=0;
    

}

orchestrator::orchestrator(const orchestrator& orig) {
}

orchestrator::~orchestrator() {
}
/*************************  USER OPENFLOW EVENTS *************************************/
void orchestrator::OUI_connected(cofdpt* dpt){
    OUI_reset_flows(dpt);
    OUI_set_port_behavior(dpt);
    //OUI_enable_qos(dpt);
}

void orchestrator::OUI_disconnected(cofdpt* dpt){ //1.0 & 1.2 compliant
    std::cout<<"[Warning] Cablemodem "<< dpt->get_dpid_s() <<"disconnected. Maybe rebooting?";
    std::map<uint32_t, cofport*>::iterator port_it;
    std::map<uint32_t, cofport*> portlist= dpt->get_ports();
    for (port_it = portlist.begin(); port_it != portlist.end(); ++port_it) {
        
        if(!proxy->discover->OUI_is_hidden_port(port_it->first)){
            if(proxy->controller->get_version()==OFP10_VERSION){
                cofport port10 (OFP10_VERSION);
                port10.set_port_no((uint16_t)proxy->virtualizer->get_virtual_port_id(dpt->get_dpid(),port_it->first));
                port10.set_hwaddr(port_it->second->get_hwaddr());
                port10.set_config(port_it->second->get_config());
                port10.set_state(port_it->second->get_state()); 
                port10.set_curr(port_it->second->get_curr());
                port10.set_advertised(port_it->second->get_advertised()); 
                port10.set_supported(port_it->second->get_supported());
                port10.set_peer(port_it->second->get_peer());  
                try{    
                    proxy->send_port_status_message(proxy->controller,OFPPR_DELETE,port10);
                }catch(...){}
            }else{
                try{    
                    proxy->send_port_status_message(proxy->controller,OFPPR_DELETE,*(port_it->second));
                }catch(...){}
            }
        }
    }
    
} /////////////////////////////////////////////////
void orchestrator::OUI_reset_flows(cofdpt* dpt){
    //RESET any existing flow
    cflowentry fe_NO_PACKET_IN(0x03); 
        fe_NO_PACKET_IN.set_command(OFPFC_DELETE);
        fe_NO_PACKET_IN.set_idle_timeout(0);
        fe_NO_PACKET_IN.set_hard_timeout(0);
        fe_NO_PACKET_IN.set_table_id(255);
        fe_NO_PACKET_IN.set_cookie(cookiemask+1);
        fe_NO_PACKET_IN.set_cookie_mask(cookiemask);
        fe_NO_PACKET_IN.set_flags(0);
        fe_NO_PACKET_IN.set_priority(1);
        //fe_NO_PACKET_IN.match.set_in_port(proxy->portconfig.oui_netport);
        try{
            proxy->send_flow_mod_message(dpt,fe_NO_PACKET_IN);
        }catch(...){
        
        }
}
void orchestrator::OUI_set_port_behavior(cofdpt* dpt){
    std::map<uint32_t, cofport*>::iterator port_it;
    std::map<uint32_t, cofport*> portlist= dpt->get_ports();
    
    cflowentry fe_NO_PACKET_IN(OFP12_VERSION); 
    fe_NO_PACKET_IN.set_command(OFPFC_ADD);
    fe_NO_PACKET_IN.set_idle_timeout(0);
    fe_NO_PACKET_IN.set_hard_timeout(0);
    fe_NO_PACKET_IN.set_table_id(1);
    fe_NO_PACKET_IN.set_cookie(cookiemask+1);
    fe_NO_PACKET_IN.set_cookie_mask(cookiemask);
    fe_NO_PACKET_IN.set_flags(0);
    fe_NO_PACKET_IN.set_priority(1);
    fe_NO_PACKET_IN.match.set_in_port(proxy->portconfig.oui_netport);
    proxy->send_flow_mod_message(dpt,fe_NO_PACKET_IN);
    
    for (port_it = portlist.begin(); port_it != portlist.end(); ++port_it) {
        
        if(proxy->discover->OUI_is_hidden_port(port_it->first)){
            cflowentry rem_vlan_and_goto_table(OFP12_VERSION); 
                rem_vlan_and_goto_table.set_command(OFPFC_ADD);
                rem_vlan_and_goto_table.set_idle_timeout(0);
                rem_vlan_and_goto_table.set_hard_timeout(0);
                rem_vlan_and_goto_table.set_table_id(0);
                rem_vlan_and_goto_table.set_cookie(cookiemask+1);
                rem_vlan_and_goto_table.set_cookie_mask(cookiemask);
                rem_vlan_and_goto_table.set_flags(0);
                rem_vlan_and_goto_table.set_priority(1);
                rem_vlan_and_goto_table.match.set_in_port(port_it->first);
                rem_vlan_and_goto_table.instructions.next() = cofinst_apply_actions(OFP12_VERSION);
                rem_vlan_and_goto_table.instructions.back().actions.next()=cofaction_pop_vlan(OFP12_VERSION); //remove QoS VLAN tag
                rem_vlan_and_goto_table.instructions.next() = cofinst_goto_table(OFP12_VERSION,1);
            try{    
                proxy->send_flow_mod_message(dpt,rem_vlan_and_goto_table);
            }catch(...){}
        }else{
            cflowentry fe_DATA_INPORT(OFP12_VERSION); 
                fe_DATA_INPORT.set_command(OFPFC_ADD);
                fe_DATA_INPORT.set_idle_timeout(0);
                fe_DATA_INPORT.set_hard_timeout(0);
                fe_DATA_INPORT.set_table_id(0);
                fe_DATA_INPORT.set_cookie(cookiemask+1);
                fe_DATA_INPORT.set_cookie_mask(cookiemask);
                fe_DATA_INPORT.set_flags(0);
                fe_DATA_INPORT.set_priority(1);
                fe_DATA_INPORT.match.set_in_port(port_it->first);
                fe_DATA_INPORT.instructions.next()=cofinst_goto_table(OFP12_VERSION,1);
            try{    
                proxy->send_flow_mod_message(dpt,fe_DATA_INPORT);
            }catch(...){}            
        }
     
    }
   //OK

}
void orchestrator::OUI_enable_qos(cofdpt* dpt){
    std::map<uint32_t, cofport*>::iterator port_it;
    std::map<uint32_t, cofport*> portlist= dpt->get_ports();
    for (port_it = portlist.begin(); port_it != portlist.end(); ++port_it) {
        cflowentry qos(OFP12_VERSION); 
            qos.set_command(OFPFC_ADD);
            qos.set_idle_timeout(0);
            qos.set_hard_timeout(0);
            qos.set_table_id(0);
            qos.set_cookie(cookiemask+1);
            qos.set_cookie_mask(cookiemask);
            qos.set_flags(0);
            qos.set_priority(1);
            qos.match.set_in_port(port_it->first);       
        if(proxy->discover->OUI_is_hidden_port(port_it->first)){ //for netport
            qos.instructions.next() = cofinst_apply_actions(OFP12_VERSION);
            qos.instructions.back().actions.next()=cofaction_pop_vlan(OFP12_VERSION); //remove QoS VLAN tag
            
            qos.instructions.next() = cofinst_goto_table(OFP12_VERSION,1);
        }else{
            qos.instructions.next() = cofinst_goto_table(OFP12_VERSION,1);
        }
        try{    
            proxy->send_flow_mod_message(dpt,qos);
        }catch(...){}
     
    }
}
/***********************  AGREGATION SWITCH EVENTS ***********************************/
void orchestrator::AGS_connected(cofdpt* dpt){
    
    AGS_reset_flows(dpt);
    AGS_set_port_behavior(dpt);
    proxy->discover->detect_CM(dpt);
    proxy->discover->detect_DPS_traffic(dpt);
    proxy->discover->detect_OUI_control_traffic(dpt);
    std::cout<<"AGS succesfully configured!\n";
    fflush(stdout);
    //
//    cofmatch match (OFP12_VERSION,OFPMT_OXM);
//    match.set_in_port(1);
//    cofflow_stats_request stats_request (OFP12_VERSION,match,OFPTT_ALL,OFPP12_ANY,OFPG12_ANY,0,0);
//    proxy->send_flow_stats_request(dpt,0,stats_request);
    
    
    //flow_test(dpt);
}
void orchestrator::AGS_reset_flows(cofdpt* dpt){
    //RESET any existing flow
    cflowentry reset(OFP12_VERSION);
        reset.set_command(OFPFC_DELETE);
        reset.set_table_id(1);
        reset.set_cookie(cookiemask);
        reset.set_cookie_mask(cookiemask);
    proxy->send_flow_mod_message(dpt,reset);
    std::cout<<"Flow RESET over AGS\n";
    fflush(stdout);
}
void orchestrator::AGS_set_port_behavior(cofdpt* dpt){
    std::map<uint32_t, cofport*>::iterator port_it;
    std::map<uint32_t, cofport*> portlist= dpt->get_ports();
    for (port_it = portlist.begin(); port_it != portlist.end(); ++port_it) {
        
        if(proxy->discover->is_hidden_port(dpt->get_dpid(),port_it->first)){
            cflowentry fe_NO_PACKET_IN(OFP12_VERSION); 
                fe_NO_PACKET_IN.set_command(OFPFC_ADD);
                fe_NO_PACKET_IN.set_idle_timeout(0);
                fe_NO_PACKET_IN.set_hard_timeout(0);
                fe_NO_PACKET_IN.set_table_id(0);
                fe_NO_PACKET_IN.set_cookie(cookiemask+1);
                fe_NO_PACKET_IN.set_cookie_mask(cookiemask);
                fe_NO_PACKET_IN.set_flags(0);
                fe_NO_PACKET_IN.set_priority(1);
                fe_NO_PACKET_IN.match.set_in_port(port_it->first);
            proxy->send_flow_mod_message(dpt,fe_NO_PACKET_IN);
        }else{
            cflowentry default1(OFP12_VERSION);
                default1.set_command(OFPFC_ADD);
                default1.set_idle_timeout(0);
                default1.set_hard_timeout(0);
                default1.set_table_id(0);
                default1.set_cookie(cookiemask);
                default1.set_cookie_mask(cookiemask);
                default1.match.set_in_port(port_it->first);
                default1.instructions.next() = cofinst_goto_table(OFP12_VERSION,1);
            proxy->send_flow_mod_message(dpt,default1);
        }
     
    }

}
void orchestrator::AGS_disconnected(cofdpt* dpt){             //////////////////////////////////////////////////
    std::cout<<"[ERROR] AGS disconnected !!...this not should be happening\n";
} /////////////////////////////////////////////////
/***************************  CONTROLLER EVENTS **************************************/
void orchestrator::CTRL_connected(){
    
}
void orchestrator::CTRL_disconnected(){
    //proxy->controller=0;
}     

/********************************  PACKET_IN *****************************************/
void      orchestrator::dispath_PACKET_IN(cofdpt *dpt, cofmsg_packet_in *msg){
    if(msg->get_reason()==OFPR_NO_MATCH){
        if(proxy->discover->is_hidden_port(dpt->get_dpid(),msg->get_match().get_in_port())){
            std::cout<<"Discarding Packet_IN\n";
            delete msg;
            return;
        }else{ //not hidden!
            uint32_t translated_port;
            if(proxy->config.of_version=="1.0"){
                //1.0 version
                translated_port=(uint16_t)proxy->virtualizer->get_virtual_port_id(dpt->get_dpid(),msg->get_match().get_in_port());
                cofmatch match(OFP10_VERSION);
                match = msg->get_match();
                match.set_in_port((uint16_t)translated_port);
                try{
                    proxy->send_packet_in_message(proxy->controller, 
                            msg->get_buffer_id(), 
                            msg->get_total_len(), 
                            msg->get_reason(), 
                            0, /*table_id =0*/
                            0, 
                            (uint16_t)translated_port,
                            match,
                            msg->get_packet().soframe(),
                            msg->get_packet().framelen());
            
 

                }catch (eRofBaseNotConnected e){
                       std::cout<<"WARNING: No Controller connected (discarding PACKEt_IN message)\n";
                }
            }else{
                //1.2 section
                translated_port=proxy->virtualizer->get_virtual_port_id(dpt->get_dpid(),msg->get_match().get_in_port());
                cofmatch match(OFP12_VERSION);
                match = msg->get_match();
                match.set_in_port(translated_port);
                try{
                    proxy->send_packet_in_message(proxy->controller, 
                            msg->get_buffer_id(), 
                            msg->get_total_len(), 
                            msg->get_reason(), 
                            0, /*table_id =0*/
                            0, 
                            translated_port,
                            match,
                            msg->get_packet().soframe(),
                            msg->get_packet().framelen());
                }catch (eRofBaseNotConnected e){
                       std::cout<<"WARNING: No Controller connected (discarding PACKEt_IN message)\n";
                }                
            }
        }
    }else if(msg->get_reason()==OFPR_ACTION){
        
        //PACKET_OUT temp
        std::cout << "PACKET_OUT_TEMP\n";
        fflush(stdout);
        std::map<uint32_t,std::map <uint32_t, cofaclist*> >::iterator it;
        std::map<uint32_t,cofaclist*>::iterator it2;
        uint32_t Vinport= proxy->virtualizer->get_virtual_port_id(dpt->get_dpid(),msg->get_match().get_in_port());
        it=packoutcache.find(Vinport);

        if(it!=packoutcache.end()){
            std::cout<<"Finding... Port: "<< Vinport <<"Buffer-id: "<< msg->get_buffer_id()-1 <<"\n";
            it2=it->second.find(msg->get_buffer_id()-1); //added -1
            if(it2!=it->second.end()){
                std::cout<<"FOUND!\n";
                if(msg->get_match().get_eth_type()==0x0806){
                    std::cout<<"Soy el ARP\n";
                }
                process_packet_out(Vinport,(*it2->second),msg->get_packet().soframe(),msg->get_packet().framelen());
                packoutcache.erase(msg->get_buffer_id()-1);
                fflush(stdout);
                
                delete msg;
                return;
            }

        }


        
        if(msg->get_table_id()==1 || dpt->get_dpid()!=proxy->config.AGS_dpid){ //OFPP_CONTROLLER //needs to be checked
            std::cout<<"Estoy aqui\n";
            uint32_t translated_port;
            translated_port=proxy->virtualizer->get_virtual_port_id(dpt->get_dpid(),msg->get_match().get_in_port());
            cofmatch match(OFP12_VERSION);
            match = msg->get_match();
            match.set_in_port(translated_port);
            try{
                proxy->send_packet_in_message(proxy->controller, 
                        msg->get_buffer_id(), 
                        msg->get_total_len(), 
                        msg->get_reason(), 
                        0, /*table_id =0*/
                        msg->get_cookie(), 
                        (uint16_t)translated_port,
                        match,
                        msg->get_packet().soframe(),
                        msg->get_packet().framelen()); 

            }catch (eRofBaseNotConnected e){
                   std::cout<<"WARNING: No Controller connected (discarding PACKEt_IN message)\n";
            }
            delete msg;
            return;
        }else{//FOR INTERNAL USAGE
            try{
                if(msg->get_match().get_udp_dst()==67 && msg->get_match().get_ipv4_src_value().get_ipv4_addr()==caddress(AF_INET,"10.10.20.1").get_ipv4_addr()){ // CM_traffic BOOTP
                    std::string string;
                    string = msg->get_packet().payload()->c_str();
                    std::string::iterator end_pos = std::remove(string.begin(), string.end(), ' ');
                    string.erase(end_pos, string.end());
                    //Depending on CM MAC
                    unsigned pos = string.find("a4a24a");
                    //unsigned pos = string.find("001e6b");
                    std::string bootp_mac;

                    bootp_mac = string.substr(pos,12);
                    std::string bootp_mac2 =bootp_mac;
                    bootp_mac.insert(2,":");
                    bootp_mac.insert(5,":");
                    bootp_mac.insert(8,":");
                    bootp_mac.insert(11,":");
                    bootp_mac.insert(14,":");

                    cmacaddr mac(bootp_mac.c_str());

                    fflush(stdout);
                    uint16_t vlan=proxy->discover->register_CM(mac.get_mac());

                    if(vlan!=0){
                        std::cout << "New CableModem registered with MAC: " <<mac.c_str() <<"\n";
                        std::cout << "VLAN assigned:"<< vlan<< "\n";
                    fflush(stdout);
                    //docsis.CMTS.assign_vlan((char*)mac.c_str(),assigned_vlan,1);
                    }else{
                        std::cout << "CableModem RETRY with MAC: " <<mac.c_str() <<"\n";
                    }
                    std::cout << "Trying to enforce QoS to CableModem : " <<mac.c_str() <<"\n";
                    cofaclist aclist(OFP12_VERSION);
                    aclist.next() = cofaction_output(OFP12_VERSION,proxy->portconfig.dps_port);
                    
                    //Load default PROFILE to the CableModem before sending the request
                    std::vector<QueueList> profile = proxy->qosmap->get_default_queues();
                    //set IDENTIFIERS AND VLAN_ID for each QUEUE
                    proxy->qosmap->allocate_identifiers(profile,vlan);
                    proxy->docsis.push_config_file(mac.c_str(),profile);
                    proxy->send_packet_out_message(dpt,msg->get_buffer_id(),proxy->portconfig.cmts_port,aclist);
                    delete msg; 
                    return;            
                }
            }catch(...){
                //delete msg; 
                //return; 
            }
            cmacaddr OUI(proxy->config.oui_mac);
            if(msg->get_match().get_in_port()==proxy->portconfig.cmts_port && msg->get_match().get_eth_src_addr().get_mac()-OUI.get_mac()<OUI.get_mac()){
                std::cout << "OUI detected: "<< msg->get_match().get_eth_src_addr().c_str() <<" with VLAN "<< msg->get_match().get_vlan_vid() <<"\n";
                fflush(stdout);
                proxy->discover->AGS_enable_OUI_traffic(dpt , msg->get_match().get_eth_src_addr().get_mac(), msg->get_match().get_vlan_vid());
                delete msg; 
                return;
            }
//            //PACKET_OUT temp
//            std::cout << "PACKET_OUT_TEMP\n";
//            fflush(stdout);
//            std::map<uint32_t,std::map <uint32_t, cofaclist*> >::iterator it;
//            std::map<uint32_t,cofaclist*>::iterator it2;
//            uint32_t Vinport= proxy->virtualizer->get_virtual_port_id(dpt->get_dpid(),msg->get_match().get_in_port());
//            it=packoutcache.find(Vinport);
//
//            if(it!=packoutcache.end()){
//                std::cout<<"Finding... Port: "<< Vinport <<"Buffer-id: "<< msg->get_buffer_id()-1 <<"\n";
//                it2=it->second.find(msg->get_buffer_id()-1); //added -1
//                if(it2!=it->second.end()){
//                    std::cout<<"FOUND!\n";
//                    if(msg->get_match().get_eth_type()==0x0800){
//                        std::cout<<"Soy el ARP\n";
//                    }
//                    process_packet_out(msg->get_match().get_in_port(),(*it2->second),msg->get_packet().soframe(),msg->get_packet().framelen());
//                }
//                //proxy->send_packet_out_message(dpt,it->first,)
//            }else{
//                std::cout<<"not found! :(\n";
//            }
//            packoutcache.erase(msg->get_buffer_id());
//            fflush(stdout);
            std::cout<<"PACKET_IN 6\n";
            delete msg; 
            return;


        }
        
    }


    delete msg; 
    return;
}//ready for 1.0 // 1.2

/************************  FEATURES REQUEST / REPLY ***********************************/
void      orchestrator::handle_features_request (cofctl *ctl, cofmsg_features_request *msg){
    //std::cout<<"features_request received\n";
    //fflush(stdout);
    uint32_t xid = msg->get_xid();
    std::set<cofdpt*>::iterator dpt_it2;
    for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
        //std::cout<<"sending FEATURES REQUEST to"<< (*dpt_it2)->get_dpid_s()<<"\n";
        proxy->send_features_request((*dpt_it2));
        ++feat_req_sent;
    }
    feat_req_last_xid=xid;
    //std::cout<<"reg timer\n";
    proxy->register_timer(FEATURES_REQ,FEATURES_REQ_TIMER);

    delete msg;
    return;
} // ready for 1.0 // 1.2
void      orchestrator::handle_features_reply(cofdpt* dpt, cofmsg_features_reply* msg){

    if(feat_req_sent==0&&feat_req_last_xid==0){ //discovering new ports

        proxy->virtualizer->enable_device(dpt->get_dpid()%0x0001000000000000,dpt->get_dpid(),dpt->get_dpid_s()); 
        std::vector<cofport>::iterator port_it;
        for (port_it = msg->get_ports().begin(); port_it != msg->get_ports().end(); ++port_it) {
            cofport port = *(port_it);
            if(!proxy->discover->is_hidden_port(dpt->get_dpid(),port.get_port_no())){ //No announce Emulator ports
                proxy->virtualizer->enable_port(dpt->get_dpid(),port.get_port_no());
                uint32_t portnum= proxy->virtualizer->get_virtual_port_id(dpt->get_dpid(),port.get_port_no());
                /////// QOS enable
                if(dpt->get_dpid()!=proxy->config.AGS_dpid){
                    proxy->qosmap->enable_queues_for_port(portnum);
                }
                ///////
                port.set_port_no(portnum);

                try{
                    if(proxy->ctl_find(proxy->controller)->get_version()==OFP12_VERSION){
                            proxy->send_port_status_message(proxy->controller,OFPPR_MODIFY,port) ; 
                    }
                    if(proxy->ctl_find(proxy->controller)->get_version()==OFP10_VERSION){
                       cofport port10 (OFP10_VERSION);
                       port10.set_port_no((uint16_t)portnum);
                       port10.set_hwaddr((*port_it).get_hwaddr());
                       port10.set_name((*port_it).get_name());
                       port10.set_config((*port_it).get_config());
                       port10.set_state((*port_it).get_state());
                       port10.set_curr((*port_it).get_curr());
                       port10.set_advertised((*port_it).get_advertised());
                       port10.set_supported((*port_it).get_supported());
                       port10.set_peer((*port_it).get_peer());
                       proxy->send_port_status_message(proxy->controller,OFPPR_MODIFY,port10) ; 

                    }
                }catch(eRofBaseNotConnected& e){
                    std::cout<<"WARNING: No Controller connected \n";   
                    fflush(stdout);
                }catch (eRofBaseNotFound& t){
                    std::cout<<"WARNING: No Controller connected \n";                        
                    fflush(stdout);
                }
            }
        } 
            
    }else{
        --feat_req_sent;
        if(feat_req_sent==0){//last of the features reply requested
            proxy->cancel_timer(FEATURES_REQ);
            uint64_t dpid = proxy->config.ALHINP_dpid;
            uint32_t n_buffers = 2048;
            uint8_t n_tables =1;
            uint32_t capabilities = OFPC_PORT_STATS;
            uint8_t of13_auxiliary_id=0;
            uint32_t of10_actions_bitmap =0;
            cofportlist global_portlist;
            std::set<cofdpt*>::iterator dpt_it;
                for (dpt_it = proxy->ofdpt_set.begin(); dpt_it != proxy->ofdpt_set.end(); ++dpt_it) {
            
                    std::map<uint32_t, cofport*>::iterator port_it;
                    std::map<uint32_t, cofport*> portlist= (*dpt_it)->get_ports();
                    for (port_it = portlist.begin(); port_it != portlist.end(); ++port_it) {

                        uint32_t portnum = port_it->first;
                        cofport port;
                        if(!proxy->discover->is_hidden_port((*dpt_it)->get_dpid(),portnum)){
                            if(proxy->controller->get_version()==OFP12_VERSION){
                                cofport port;
                                port.set_port_no(proxy->virtualizer->get_virtual_port_id((*dpt_it)->get_dpid(),port_it->first));
                                port.set_hwaddr(port_it->second->get_hwaddr());
                                port.set_config(port_it->second->get_config());
                                port.set_state(port_it->second->get_state()); 
                                port.set_curr(port_it->second->get_curr());
                                port.set_advertised(port_it->second->get_advertised()); 
                                port.set_supported(port_it->second->get_supported());
                                port.set_peer(port_it->second->get_peer());                
                                port.set_curr_speed(port_it->second->get_curr_speed());
                                port.set_max_speed(port_it->second->get_max_speed());               
                                global_portlist.next()=port;
                            }
                            if(proxy->controller->get_version()==OFP10_VERSION){
                                cofport port10 (OFP10_VERSION);
                                port10.set_port_no((uint16_t)proxy->virtualizer->get_virtual_port_id((*dpt_it)->get_dpid(),port_it->first));
                                port10.set_hwaddr(port_it->second->get_hwaddr());
                                port10.set_config(port_it->second->get_config());
                                port10.set_state(port_it->second->get_state()); 
                                port10.set_curr(port_it->second->get_curr());
                                port10.set_advertised(port_it->second->get_advertised()); 
                                port10.set_supported(port_it->second->get_supported());
                                port10.set_peer(port_it->second->get_peer());                
                                //port10.set_curr_speed(port_it->second->get_curr_speed());
                                //port10.set_max_speed(port_it->second->get_max_speed()); 
                                global_portlist.next()=port10;
                            }
                    
                        }
                    } 
            
                }    
            try{
            proxy->send_features_reply(proxy->controller, 
                    feat_req_last_xid, 
                    dpid, 
                    n_buffers, 
                    n_tables, 
                    capabilities, 
                    of13_auxiliary_id, 
                    of10_actions_bitmap, 
                    global_portlist);
            feat_req_last_xid=0;
            }catch(...){
                std::cout<<"UOPS\n";
            }
            
        }
        
    }   
    delete msg;
    return; 
} //ready for 1.0 // 1.2

/****************************  PORT MOD / STATUS *************************************/
void      orchestrator::handle_port_status(cofdpt* dpt, cofmsg_port_status* msg){

//OF 1.0 compatible
    cofport port=msg->get_port();
    if(!proxy->discover->is_hidden_port(dpt->get_dpid(),port.get_port_no())){ //No announce Emulator ports
        port.set_port_no(proxy->virtualizer->get_virtual_port_id(dpt->get_dpid(),msg->get_port().get_port_no()));
        if(proxy->ctl_find(proxy->controller)->get_version()==OFP12_VERSION)
                proxy->send_port_status_message(proxy->controller,msg->get_reason(),port) ; 
        if(proxy->ctl_find(proxy->controller)->get_version()==OFP10_VERSION){
            cofport port10(OFP10_VERSION);
                port10.set_port_no((uint16_t)proxy->virtualizer->get_virtual_port_id(dpt->get_dpid(),msg->get_port().get_port_no()));
                port10.set_hwaddr(port.get_hwaddr());
                port10.set_config(port.get_config());
                port10.set_state(port.get_state()); 
                port10.set_curr(port.get_curr());
                port10.set_advertised(port.get_advertised()); 
                port10.set_supported(port.get_supported());
                port10.set_peer(port.get_peer()); 
                try{
                    proxy->send_port_status_message(proxy->controller,msg->get_reason(),port10) ;
                }catch(...){
                    std::cout<<"[WARNING:] Port Status not sent to controller";
                }
        }
    }
    delete msg;
    return;
} //ready for 1.0 // 1.2
void      orchestrator::handle_port_mod (cofctl *ctl, cofmsg_port_mod *msg){
        
        //check port id
    uint32_t realport = proxy->virtualizer->get_real_port_id(msg->get_port_no());
    if (realport == 0){
        proxy->send_error_message(proxy->controller,msg->get_xid(),OFPET_PORT_MOD_FAILED,OFPPMFC_BAD_PORT,msg->soframe(),msg->framelen());
        //Send ERROR
    }else{
        uint32_t config = msg->get_config();
        //config &= OFP10PC_PORT_DOWN;
    proxy->send_port_mod_message(proxy->dpt_find(proxy->virtualizer->get_own_dpid(msg->get_port_no())),
                proxy->virtualizer->get_real_port_id(msg->get_port_no()),
                msg->get_hwaddr(),
                msg->get_config(),
                0,
                msg->get_advertise());
    }
	delete msg;
        return;
}//ready for 1.0 // 1.2

/******************************  cONFIGURATION ***************************************/
void      orchestrator::handle_get_config_request(cofctl* ctl, cofmsg_get_config_request* msg){ //ready for 1.0 // 1.2
    proxy->send_get_config_reply(proxy->controller,msg->get_xid(),config_flags,miss_send_len);
    delete msg;
}
void      orchestrator::handle_set_config (cofctl *ctl, cofmsg_set_config *msg){//ready for 1.0 // 1.2
    config_flags=msg->get_flags();
    miss_send_len=msg->get_miss_send_len();
    
    std::set<cofdpt*>::iterator dpt_it;
    for (dpt_it = proxy->ofdpt_set.begin(); dpt_it != proxy->ofdpt_set.end(); ++dpt_it) {
        proxy->send_set_config_message(*dpt_it,config_flags,miss_send_len);
    }
    delete msg;
    return;
}

/***********************************  TIMERS *****************************************/
void      orchestrator::handle_timeout(int opaque){
    switch(opaque){
        case FEATURES_REQ:
            proxy->cancel_timer(FEATURES_REQ);
            uint64_t dpid = ALHINP_DPID;
            uint32_t n_buffers = 2048;
            uint8_t n_tables =1;
            uint32_t capabilities = OFPC_PORT_STATS;
            uint8_t of13_auxiliary_id=0;
            uint32_t of12_actions_bitmap =0;
            cofportlist global_portlist;
            std::set<cofdpt*>::iterator dpt_it;
                for (dpt_it = proxy->ofdpt_set.begin(); dpt_it != proxy->ofdpt_set.end(); ++dpt_it) {
            
                    std::map<uint32_t, cofport*>::iterator port_it;
                    std::map<uint32_t, cofport*> portlist= (*dpt_it)->get_ports();
                    for (port_it = portlist.begin(); port_it != portlist.end(); ++port_it) {

                        uint32_t portnum = port_it->first;
                        cofport port;
                        if(!proxy->discover->is_hidden_port((*dpt_it)->get_dpid(),portnum)){
                            uint32_t portno;
                            portno = proxy->virtualizer->get_virtual_port_id((*dpt_it)->get_dpid(),port_it->first);
                            port.set_port_no(portno);
                            port.set_hwaddr(port_it->second->get_hwaddr());
                            port.set_config(port_it->second->get_config());
                            port.set_state(port_it->second->get_state()); 
                            port.set_curr(port_it->second->get_curr());
                            port.set_advertised(port_it->second->get_advertised()); 
                            port.set_supported(port_it->second->get_supported());
                            port.set_peer(port_it->second->get_peer());                
                            port.set_curr_speed(port_it->second->get_curr_speed());
                            port.set_max_speed(port_it->second->get_max_speed());               
                            global_portlist.next()=port;
//                            if((*dpt_it)->get_dpid()!=proxy->config.AGS_dpid){
//                                proxy->qosmap->enable_queues_for_port(portno);
//                            }
                                
                        }
                    } 
            
                }           
            proxy->send_features_reply(proxy->controller, feat_req_last_xid, dpid, n_buffers, n_tables, capabilities, of13_auxiliary_id, of12_actions_bitmap, global_portlist);
            feat_req_last_xid=0;
            break;
    }
}

/*******************************  PACKET_OUT *****************************************/
void      orchestrator::handle_packet_out (cofctl *ctl, cofmsg_packet_out *msg){
    if(msg->get_buffer_id()==OFP_NO_BUFFER){
        //packet attached to message
        std::cout<<"packeout received with NO BUFFER\n";
        uint8_t *data  = msg->get_packet().payload()->soframe();
        size_t datalen = msg->get_packet().payload()->framelen();
        
        //can be directly processed
        process_packet_out(OFPP10_NONE,msg->get_actions(),msg->get_packet().soframe(),msg->get_packet().framelen());
        
    }else{
        cofaclist* temp = new cofaclist (OFP10_VERSION);
        (*temp)=msg->get_actions();
        packoutcache [msg->get_in_port()][msg->get_buffer_id()] =temp;
        std::cout<<"packeout cached with BUFFER-ID "<< msg->get_buffer_id()<<"\n";
        cofaclist list (OFP12_VERSION);
        list.next() = cofaction_output(OFP12_VERSION,OFPP12_CONTROLLER);
        uint32_t realport = proxy->virtualizer->get_real_port_id(msg->get_in_port());
        uint64_t dpid = proxy->virtualizer->get_own_dpid(msg->get_in_port());
        proxy->send_packet_out_message(proxy->dpt_find(dpid),msg->get_buffer_id(),realport,list);
        
    }
    delete msg;
    return;
}
void      orchestrator::process_packet_out(uint32_t inport,cofaclist list, uint8_t *data,size_t datalen){
    
    flowpath packetouts;
    std::cout<<"packeout Processing\n";
    //cofmatch common_match (OFP12_VERSION);
    std::cout<<"inport was: "<< inport << "\n";
    //process_action_list(&packetouts,common_match,list, OFP10_VERSION, inport,0xFF,OFPT10_PACKET_OUT);
    process_output_actions(packetouts,list, OFP10_VERSION, inport, 0xFF,data,datalen);
    std::cout<<"packeout Processing completed\n";
    std::map<uint64_t , cflowentry*>::iterator it;
    for(it=packetouts.flowmodlist.begin();it!=packetouts.flowmodlist.end();++it){

    proxy->send_packet_out_message(proxy->dpt_find(it->first),
            (uint32_t)OFP12_NO_BUFFER,
            inport,
            it->second->instructions.back().actions,
            data,
            datalen);
    }
    
    
}

void      orchestrator::process_output_actions(flowpath &flows,cofaclist aclist, uint8_t ofversion, uint32_t inport, uint8_t nw_proto,uint8_t *data,size_t datalen){
    flowpath flowlist;
    flowlist.longest=0;
    cofinlist instrlist;
    cofaclist::iterator it;
    bool anyoutput=false;
    uint8_t flowtype;
    if (ofversion==OFP10_VERSION){   
        instrlist.next()=cofinst_apply_actions(OFP12_VERSION);
        for (it = aclist.begin(); it != aclist.end(); ++it){
            try{
                std::cout<< (int)htobe16((*it).oac_oacu.oacu_header->type)<<"\n";
                switch (htobe16((*it).oac_oacu.oacu_header->type)) {
                    case OFP10AT_OUTPUT:{
                        anyoutput=true;
                        fill_packetouts2(flowlist,instrlist.back().actions,inport, htobe16((*it).oac_oacu.oacu_10output->port),data, datalen);
                        break;
                        
                    }
                    case OFP10AT_SET_VLAN_VID:{
                        //std::cout<<"SET_VLAN_VID: "<< htobe16((*it).oac_oacu.oacu_10vlanvid->vlan_vid) <<"\n";
                        fflush(stdout);
                        instrlist.back().actions.next() = cofaction_push_vlan(OFP12_VERSION,C_TAG); 
                        instrlist.back().actions.next() = cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_vid (OFPVID_PRESENT|htobe16((*it).oac_oacu.oacu_10vlanvid->vlan_vid)));
                        break;
                    }
                    case OFP10AT_SET_VLAN_PCP:{
                        //std::cout<<"SET_VLAN_PCP: "<< (*it).oac_oacu.oacu_10vlanpcp->vlan_pcp <<"\n";
                        instrlist.back().actions.next() = cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_pcp ((*it).oac_oacu.oacu_10vlanpcp->vlan_pcp));
                        break;
                    }
                    case OFP10AT_STRIP_VLAN:{
                        //std::cout<<"POP VLAN\n";
                        instrlist.back().actions.next()= cofaction_pop_vlan(OFP12_VERSION);
                        break;
                    }
                    case OFP10AT_SET_DL_SRC:{
                        cmacaddr mac_dl_src((*it).oac_oacu.oacu_10dladdr->dl_addr,6);
                        //std::cout<<"SET_MAC_SRC: "<< mac_dl_src.c_str()<<"\n";
                        instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_eth_src(mac_dl_src));
                        break;
                    }
                    case OFP10AT_SET_DL_DST:{
                        cmacaddr mac_dl_dst((*it).oac_oacu.oacu_10dladdr->dl_addr,6);
                        //std::cout<<"SET_MAC_DST: "<< mac_dl_dst.c_str()<<"\n";
                        instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_eth_dst(mac_dl_dst));
                        break;
                    }
                    case OFP10AT_SET_NW_SRC:{
                        caddress ip_src(AF_INET);
                        ip_src.set_ipv4_addr(htobe32((*it).oac_oacu.oacu_10nwaddr->nw_addr));
                        //std::cout<<"SET_IP_SRC "<<ip_src.addr_c_str() <<"\n";
                        instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_ipv4_src(ip_src));                        
                        break;
                    }
                    case OFP10AT_SET_NW_DST:{
                        caddress ip_dst(AF_INET);
                        ip_dst.set_ipv4_addr(htobe32((*it).oac_oacu.oacu_10nwaddr->nw_addr));
                        //std::cout<<"SET_IP_DST "<< ip_dst.addr_c_str() <<"\n";
                        instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_ipv4_dst(ip_dst));
                        break;
                    }                                         
                    case OFP10AT_SET_NW_TOS:{
                        //std::cout<< "SET_NW_TOS: "<<(uint32_t)(*it).oac_oacu.oacu_10nwtos->nw_tos<<"\n";
                        instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_ip_ecn((*it).oac_oacu.oacu_10nwtos->nw_tos));
                        instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_ip_dscp((*it).oac_oacu.oacu_10nwtos->nw_tos));
                        break;
                    }
                    case OFP10AT_SET_TP_SRC:{
                        //std::cout<< "SET_TP_SRC "<<htobe16((*it).oac_oacu.oacu_10tpport->tp_port)<<"\n";
                        if(nw_proto==0x06 || nw_proto == 0xFF)//TCP
                            instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_tcp_src(htobe16((*it).oac_oacu.oacu_10tpport->tp_port)));
                        else
                            instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_udp_src(htobe16((*it).oac_oacu.oacu_10tpport->tp_port)));
                        break;
                    }
                    case OFP10AT_SET_TP_DST:{
                        //std::cout<< "SET_TP_DST "<<htobe16((*it).oac_oacu.oacu_10tpport->tp_port)<<"\n";
                        if(nw_proto==0x06 || nw_proto == 0xFF)//TCP
                            instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_tcp_dst(htobe16((*it).oac_oacu.oacu_10tpport->tp_port)));
                        else
                            instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_udp_dst(htobe16((*it).oac_oacu.oacu_10tpport->tp_port)));
                        break;
                    }
//                    case OFP10AT_SET_BANDWIDTH:{
//                        //std::cout<< "ACTIONS: SET_BANDWIDTH\n"<<(*it).oac_oacu.oacu_10setBW->bandwidth <<"\n";
//                        break;
//                    }
                    default:{
                       break;
                    }
                }
            }catch(...){
                
            }
        }//END FOR
        if(anyoutput==false){
            std::cout<<"OUTPORT=NONE (DROP ACTION) sending drop\n";
            fflush(stdout);
            flowtype = flow_direction(inport,0);
            //fill_flowpath2(flowlist,common_match,instrlist.back().actions, inport,0,flowtype);
            
        }
    }
    flows=flowlist;
    return;    
}
void      orchestrator::fill_packetouts(flowpath &flows,cofaclist aclist,uint32_t inport, uint32_t outport, uint8_t flowtype){
    

    uint64_t outdpid;
    proxy->virtualizer->get_own_dpid(outport);  
    if(outport==OFPP10_ALL){
        return;
        //send error;
    }
    if(inport==OFPP10_NONE){ //for DROP rule
        //std::cout<<"taking DPID to send from OUTPUT\n";

        uint64_t indpid =proxy->virtualizer->get_own_dpid(outport); 
        cflowentry* temp;
        temp = new cflowentry (OFP12_VERSION);
        flows.flowmodlist.insert(std::make_pair(indpid,temp)); //INGRESS 
        flows.flowmodlist[indpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
        flows.flowmodlist[indpid]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->virtualizer->get_real_port_id(outport)); 
        return;
        
    }
    uint64_t indpid =proxy->virtualizer->get_own_dpid(inport); 
    switch(flowtype){
        case LOCAL:{
            std::map<uint64_t , cflowentry*>::iterator it;
            outdpid=indpid;
            it=flows.flowmodlist.find(outdpid);
            if(it->first!=indpid){
                //doesn't exist!!
                cflowentry* temp;
                temp = new cflowentry (OFP12_VERSION);
                flows.flowmodlist.insert(std::make_pair(indpid,temp)); //INGRESS 
                flows.flowmodlist[indpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
                flows.flowmodlist[indpid]->instructions.back().actions= aclist;
            }
            cofaclist::iterator iter;
            for(iter=aclist.begin();iter!=aclist.end();++iter){
                flows.flowmodlist[indpid]->instructions.back().actions.next()=(*iter); //append actions;
            }
            flows.flowmodlist[indpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->virtualizer->get_real_port_id(outport));  
            break;
            
        }
        default:{
            std::map<uint64_t , cflowentry*>::iterator it;
            outdpid=proxy->virtualizer->get_own_dpid(outport);
            it=flows.flowmodlist.find(outdpid);
            if(it->first!=outdpid){
                //doesn't exist!!
                cflowentry* temp2;
                temp2 = new cflowentry (OFP12_VERSION);
                flows.flowmodlist.insert(std::make_pair(outdpid,temp2)); //INGRESS 
                flows.flowmodlist[outdpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
                flows.flowmodlist[outdpid]->instructions.back().actions= aclist;
            }
            //copy new actions
            cofaclist::iterator iter;
            for(iter=aclist.begin();iter!=aclist.end();++iter){
                flows.flowmodlist[outdpid]->instructions.back().actions.next()=(*iter); //append actions;
            }
            flows.flowmodlist[indpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->virtualizer->get_real_port_id(outport)); 
            break;
        }
        
    }
    
}
void      orchestrator::fill_packetouts2(flowpath &flows,cofaclist aclist,uint32_t inport, uint32_t outport,uint8_t *data,size_t datalen){
    std::cout<< "entering fillpktout2\n";
    std::cout<<"inport: "<<inport<<"\n";
    std::cout<<"outport: "<<outport <<"\n";
    uint64_t outdpid;
    uint32_t real_inport=0;
    uint64_t real_indpid=0;
    if(inport<OFPP10_MAX){
        real_inport = proxy->virtualizer->get_real_port_id(inport);
        if(real_inport!=0)
            real_indpid = proxy->virtualizer->get_own_dpid(inport);
    }
    switch(outport){
        case OFPP10_ALL:
        case OFPP10_FLOOD:{
            if(inport==OFPP10_NONE){
                std::set<cofdpt*>::iterator dpt_it2;
                std::cout<< "entering dpt_iter\n";
                for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
                    outdpid = (*dpt_it2)->get_dpid();
                    std::cout<<(*dpt_it2)->get_dpid_s() <<" ";
                    std::map<uint32_t, cofport*>::iterator port_it;
                    std::map<uint32_t, cofport*> portlist= (*dpt_it2)->get_ports();
                    for (port_it = portlist.begin(); port_it != portlist.end(); ++port_it) {
                        uint32_t portnum = (*port_it).first;
                        std::cout<< portnum<<"\n";
                        cofport port;
                        if(!proxy->discover->is_hidden_port((*dpt_it2)->get_dpid(),portnum)){
                            if(proxy->controller->get_version()==OFP12_VERSION){
                                //TO be done
                            }
                            if(proxy->controller->get_version()==OFP10_VERSION){
                                std::cout<< "entering sending\n";
                                uint32_t outport = portnum;
                                cofaclist aclist2 = aclist;
                                aclist2.next()=cofaction_output(OFP12_VERSION,outport); 
                                proxy->send_packet_out_message(proxy->dpt_find(outdpid),OFP_NO_BUFFER,inport,aclist2,data,datalen);
                            }
                    
                        }
                    } 
                }
            }else{
                std::cout<< "entering dpt_iter2\n";
                std::set<cofdpt*>::iterator dpt_it2;
                for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
                    outdpid = (*dpt_it2)->get_dpid();
                    
                    std::map<uint32_t, cofport*>::iterator port_it;
                    std::map<uint32_t, cofport*> portlist= (*dpt_it2)->get_ports();
                    for (port_it = portlist.begin(); port_it != portlist.end(); ++port_it) {
                        uint32_t portnum = port_it->first;
                        std::cout<<(*dpt_it2)->get_dpid_s() <<" ";
                        std::cout<< portnum<<"\n";
                        cofport port;
                        if(!proxy->discover->is_hidden_port((*dpt_it2)->get_dpid(),portnum)){
                            if(proxy->controller->get_version()==OFP12_VERSION){
                                //TO be done
                            }
                            if(proxy->controller->get_version()==OFP10_VERSION){
                                uint32_t outport = portnum;
                                std::cout<<"real_dpid: "<<real_indpid <<" real_port"<<real_inport<<"\n";
                                if(!(real_indpid== (*dpt_it2)->get_dpid() && real_inport == portnum)){
                                    std::cout<< "entering sending for "<< portnum <<"\n";
                                    cofaclist aclist2 (OFP12_VERSION);
                                    try{
                                        aclist2 = aclist;
                                    }catch(...){}
                                    aclist2.next()=cofaction_output(OFP12_VERSION,outport); 
                                    std::cout<< "Sending...\n";
                                    proxy->send_packet_out_message(proxy->dpt_find(outdpid),OFP_NO_BUFFER,inport,aclist2,data,datalen);
                                }
                            }
                    
                        }
                    } 
                }                
                
            }
        }break;
        case OFPP10_LOCAL:{
            
        }break;
        case OFPP10_IN_PORT:{
            
            
        }break;
        default:{
            outdpid = proxy->virtualizer->get_own_dpid(outport);
            uint32_t outport = proxy->virtualizer->get_real_port_id(outport);
            cofaclist aclist2 = aclist;
            aclist2.next()=cofaction_output(OFP12_VERSION,proxy->virtualizer->get_real_port_id(outport)); 
            proxy->send_packet_out_message(proxy->dpt_find(outdpid),OFP_NO_BUFFER,inport,aclist2,data,datalen);
            
        }break;
        
    }
    
}
/*******************************  FLOW_MODS  *****************************************/
void      orchestrator::flow_mod_add(cofctl *ctl, cofmsg_flow_mod *msg){
    uint8_t ofversion=ctl->get_version();
    //getting IN_PORT
    std::cout<<"\nNew Flow_Mod... \n";
    uint32_t virtual_inport=0;
    bool input_wildcarded = false;
    try{
        virtual_inport = msg->get_match().get_in_port();
    }catch(...){}
    if(virtual_inport==0){
        input_wildcarded = true;
    }
    //Processing match
    cofmatch common_match (OFP12_VERSION);
    if(ofversion==OFP10_VERSION){
        common_match=process_matching_from_msg(msg);
    }else{
        common_match=msg->get_match();
    }
    std::cout<<"Exiting... "<< common_match.c_str()<<"\n";
    
    flowpath flows;
    flows.longest=0;
    std::cout<<"Entering to processing action list... \n";
    
    //proccessing 1.0 version
    if(ofversion==OFP10_VERSION){
        cofaclist aclist (OFP10_VERSION);
        uint8_t proto=0;
        try{
            aclist=msg->get_actions(); //In case of Drop action
        }catch(...){}
        try{
            proto=msg->get_match().get_nw_proto();
        }catch(...){}
        process_action_list_10(flows,common_match, aclist, OFP10_VERSION, virtual_inport,proto,OFPT10_FLOW_MOD); //FOR OF 1.0
    }else{
        
        cofinlist inlist (OFP12_VERSION);
        uint8_t proto=0;
        try{
            inlist=msg->get_instructions(); //In case of Drop action
        }catch(...){}
        try{
            proto=msg->get_match().get_ip_proto();
        }catch(...){}
        process_action_list_12(flows,common_match, inlist, OFP12_VERSION, virtual_inport,proto,OFPT12_FLOW_MOD); //FOR OF 1.2        
    }
    
    std::map<uint64_t , cflowentry*>::iterator it;
    if(flows.flowmodlist.empty()){
        std::cout<<"Something went wrong\n";
        //Sending error message;
        //proxy->send_error_meesage
        delete msg;
        return;
    }
    std::cout<<"Storing Flow into flowcache... \n";
    uint16_t flow_id = proxy->flowcache->store_flow(msg,(uint64_t) 0); //needs to be completed
    std::cout<<"Stored Flow with ID "<< (int)flow_id <<"\n";
    
    //std::cout<<"Iteration over dpids... \n";    
    for(it=flows.flowmodlist.begin();it!=flows.flowmodlist.end();++it){
        //std::cout<<"Conforming FLOW_MOD for "<<(*it).first <<"\n";
        if(input_wildcarded==false){
            std::cout<<"Registering partial Flow... \n";
            proxy->flowcache->reg_partial_flow(flow_id,it->first);
        }

        ////////////////
        //std::cout<<"Sending Flow... \n";
        it->second->set_cookie(msg->get_cookie());
        it->second->set_command(OFPFC_ADD);
        it->second->set_buffer_id(OFP_NO_BUFFER);
        //it->second.partial_fe.set_table_id(msg->get_table_id());
        it->second->set_priority(msg->get_priority());
        it->second->set_hard_timeout(msg->get_hard_timeout());
        it->second->set_idle_timeout(msg->get_idle_timeout());
        //print content
        std::cout<< it->second->c_str();
        proxy->send_flow_mod_message(proxy->dpt_find(it->first),(*it->second));
        delete(it->second);
    }
    if(input_wildcarded==true){
        proxy->flowcache->reg_partial_flow(flow_id,ALL_DPID);
        std::cout<<"Registering WIldcard partial Flow... \n";
    }

    delete msg;
    
}
void      orchestrator::flow_mod_modify(cofctl *ctl, cofmsg_flow_mod *msg, bool strict){
    
}
void      orchestrator::flow_mod_delete(cofctl *ctl, cofmsg_flow_mod *msg, bool strict){
    std::cout<<"Deleting Flow entries\n";
    cflowentry fe_DELETE(OFP12_VERSION); 
    fe_DELETE.set_command(OFPFC_DELETE);
    fe_DELETE.set_table_id(msg->get_table_id());
    //fe_DELETE.set_cookie(msg->get_cookie()&~cookiemask);            
    fe_DELETE.set_cookie_mask(0);
    fe_DELETE.set_flags(msg->get_flags());

    std::set<cofdpt*>::iterator dpt_it2;
    for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
        if((*dpt_it2)->get_dpid()==proxy->config.AGS_dpid)
            fe_DELETE.set_table_id(1);
        proxy->send_flow_mod_message((*dpt_it2),fe_DELETE);

    }
    delete msg;
}
void      orchestrator::handle_flow_removed (cofdpt *dpt, cofmsg_flow_removed *msg){
    //firstly exists??
    uint64_t virtualcookie = msg->get_cookie();
    bool exist = false/*result*/;
    exist=proxy->flowcache->flow_exists((uint16_t)virtualcookie);
    if(exist==false){
        delete msg;
        return;
        //message has already been sent 
    }else{
        cflow flow;
        flow = proxy->flowcache->get_flow(virtualcookie);
        proxy->send_flow_removed_message(proxy->controller,
                flow.fe->match,
                flow.fe->get_cookie(),
                msg->get_priority(),
                msg->get_reason(),
                0,
                msg->get_duration_sec(),
                msg->get_duration_nsec(),
                msg->get_idle_timeout(),
                msg->get_hard_timeout(),
                msg->get_packet_count(),
                msg->get_byte_count());
        proxy->flowcache->deleteflow(flow.fe->get_cookie());
    }
    delete msg;
    return;
}

/**************************  COMMON AUX FUNCTIONS  ************************************/
/**
 * \brief for an incoming OF1.0 match structure, a common_match is processed
 * @param match
 * @param ofversion
 * @return 
 */
cofmatch  orchestrator::process_matching(cofmatch match, uint8_t ofversion){
    cofmatch common_match (OFP12_VERSION);
    
    if(ofversion==OFP12_VERSION){
        common_match=match;
        //virtual_inport=msg->get_match().get_in_port();
    }
    if(ofversion==OFP10_VERSION){
        //Openflow 1.0 Match parsing
//        try{
//            =(uint32_t)msg->get_match().get_in_port();
//            std::cout<<"in_port_not_wildcarded: " << msg->get_match().get_in_port()<<"\n";
//        }catch(eOFmatchNotFound& e){}   
        
        try{ //OF 1.0
            common_match.set_eth_type(match.get_eth_type());
            //std::cout<<"Ethtype_not_wildcarded: "<<msg->get_match().get_eth_type()<<"\n";
        }catch(eOFmatchNotFound& e){}

        try{ //OF 1.0
        if(match.get_eth_src_addr().get_mac()!=0){ //ETH_SRC
            common_match.set_eth_src(match.get_eth_src_addr());
            //std::cout<<"dl_src_not_wildcarded: "<<msg->get_match().get_eth_src_addr().c_str()<<"\n";
        }
        }catch(eOFmatchNotFound& e){}

        try{ //OF 1.0
        if(match.get_eth_dst_addr().get_mac()!=0){ //ETH_SRC
            common_match.set_eth_dst(match.get_eth_dst_addr());
            //std::cout<<"dl_dst_not_wildcarded: "<<match.get_eth_dst_addr().c_str()<<"\n";
        }            
        }catch(eOFmatchNotFound& e){}
        
        try{ //OF 1.0
            common_match.set_vlan_vid(match.get_vlan_vid());
            //std::cout<<"vid_not_wildcarded: "<<match.get_vlan_vid()<<"\n";

        }catch(eOFmatchNotFound& e){}

        try{ //OF 1.0
            common_match.set_vlan_pcp(match.get_vlan_pcp());
            //std::cout<<"pcp_not_wildcarded: "<<(uint32_t)match.get_vlan_pcp()<<"\n";
        }catch(eOFmatchNotFound& e){}

        try{ //OF 1.0

            if(match.get_nw_dst_value().get_ipv4_addr()!=0){
            common_match.set_ipv4_dst(match.get_nw_dst_value());
            //std::cout<<"nw_dst_not_wildcarded: "<<match.get_nw_dst_value().addr_c_str()<<"\n";
            }

        }catch(eOFmatchNotFound& e){}
        
        try{//check !=0
            if(match.get_nw_dst_value().get_ipv4_addr()!=0){
                //caddr ip_src();
            common_match.set_ipv4_src(match.get_nw_src_value());
            //std::cout<<"nw_src_not_wildcarded: "<<match.get_nw_src_value().addr_c_str()<<"\n";
            }
        }catch(eOFmatchNotFound& e){}
        
        try{
            //common_match.set_ip_proto(match.get_nw_proto());
            //std::cout<<"ip_proto_not_wildcarded: "<<match.get_ip_proto()<<"\n";
            if(match.get_nw_proto()==6){
            try{
                    try{
                        //std::cout<<"TCP_SRC_not_wildcarded: "<<match.get_tp_src()<<"\n";
                        common_match.set_tcp_src(match.get_tp_src());


                    }catch(eOFmatchNotFound& e){}
                    try{
                        common_match.set_tcp_dst(match.get_tp_dst());
                        //std::cout<<"TCP_DST_not_wildcarded: "<<match.get_tp_dst()<<"\n";

                    }catch(eOFmatchNotFound& e){}
            }catch(eOFmatchNotFound& e){}
            }
            if(match.get_nw_proto()==17){
            try{
                try{
                    common_match.set_udp_src(match.get_tp_src());
                    //std::cout<<"UDP_SRC_not_wildcarded: "<<match.get_tp_src()<<"\n";

                }catch(eOFmatchNotFound& e){}
                try{
                    common_match.set_udp_dst(match.get_tp_dst());
                    //std::cout<<"UDP_src_not_wildcarded: "<<match.get_tp_dst()<<"\n";

                }catch(eOFmatchNotFound& e){}
            }catch(eOFmatchNotFound& e){} 
            }
        }catch(eOFmatchNotFound& e){} 
    } 
    return common_match;
}
cofmatch  orchestrator::process_matching_from_msg(cofmsg_flow_mod *msg, uint8_t ofversion){
    cofmatch common_match (OFP12_VERSION);
    
    if(ofversion==OFP12_VERSION){
        common_match=msg->get_match();
        //virtual_inport=msg->get_match().get_in_port();
    }
    if(ofversion==OFP10_VERSION){
        //Openflow 1.0 Match parsing
//        try{
//            =(uint32_t)msg->get_match().get_in_port();
//            std::cout<<"in_port_not_wildcarded: " << msg->get_match().get_in_port()<<"\n";
//        }catch(eOFmatchNotFound& e){}   
        
        try{ //OF 1.0
            common_match.set_eth_type(msg->get_match().get_eth_type());
            //std::cout<<"Ethtype_not_wildcarded: "<<msg->get_match().get_eth_type()<<"\n";
        }catch(eOFmatchNotFound& e){}

        try{ //OF 1.0
        if(msg->get_match().get_eth_src_addr().get_mac()!=0){ //ETH_SRC
            common_match.set_eth_src(msg->get_match().get_eth_src_addr());
            //std::cout<<"dl_src_not_wildcarded: "<<msg->get_match().get_eth_src_addr().c_str()<<"\n";
        }
        }catch(eOFmatchNotFound& e){}

        try{ //OF 1.0
        if(msg->get_match().get_eth_dst_addr().get_mac()!=0){ //ETH_SRC
            common_match.set_eth_dst(msg->get_match().get_eth_dst_addr());
            //std::cout<<"dl_dst_not_wildcarded: "<<msg->get_match().get_eth_dst_addr().c_str()<<"\n";
        }            
        }catch(eOFmatchNotFound& e){}
        
        try{ //OF 1.0
            //common_match.set_vlan_vid(msg->get_match().get_vlan_vid());
            //std::cout<<"vid_not_wildcarded: "<<msg->get_match().get_vlan_vid()<<"\n";

        }catch(eOFmatchNotFound& e){}

        try{ //OF 1.0
            //common_match.set_vlan_pcp(msg->get_match().get_vlan_pcp());
            //std::cout<<"pcp_not_wildcarded: "<<(uint32_t)msg->get_match().get_vlan_pcp()<<"\n";
        }catch(eOFmatchNotFound& e){}
        if(msg->get_match().get_eth_type()==0x0806){//ARP
            try{ //OF 1.0
                if(msg->get_match().get_nw_dst_value().get_ipv4_addr()!=0){
                common_match.set_arp_tpa(msg->get_match().get_nw_dst_value());
                //std::cout<<"nw_dst_not_wildcarded: "<<msg->get_match().get_nw_dst_value().addr_c_str()<<"\n";
                }

            }catch(eOFmatchNotFound& e){}

            try{//check !=0
                if(msg->get_match().get_nw_dst_value().get_ipv4_addr()!=0){
                    //caddr ip_src();
                common_match.set_arp_spa(msg->get_match().get_nw_src_value());
                //std::cout<<"nw_src_not_wildcarded: "<<msg->get_match().get_nw_src_value().addr_c_str()<<"\n";
                }
            }catch(eOFmatchNotFound& e){}
        }
        if(msg->get_match().get_eth_type()==0x0800){
            try{ //OF 1.0
                if(msg->get_match().get_nw_dst_value().get_ipv4_addr()!=0){
                common_match.set_ipv4_dst(msg->get_match().get_nw_dst_value());
                //std::cout<<"nw_dst_not_wildcarded: "<<msg->get_match().get_nw_dst_value().addr_c_str()<<"\n";
                }

            }catch(eOFmatchNotFound& e){}

            try{//check !=0
                if(msg->get_match().get_nw_dst_value().get_ipv4_addr()!=0){
                    //caddr ip_src();
                common_match.set_ipv4_src(msg->get_match().get_nw_src_value());
                //std::cout<<"nw_src_not_wildcarded: "<<msg->get_match().get_nw_src_value().addr_c_str()<<"\n";
                }
            }catch(eOFmatchNotFound& e){}            
        }
        try{
            //common_match.set_ip_proto(msg->get_match().get_nw_proto());
            std::cout<<"ip_proto_not_wildcarded: "<<msg->get_match().get_ip_proto()<<"\n";
            if(msg->get_match().get_nw_proto()==6){
            try{
                    try{
                        //std::cout<<"TCP_SRC_not_wildcarded: "<<msg->get_match().get_tp_src()<<"\n";
                        common_match.set_tcp_src(msg->get_match().get_tp_src());


                    }catch(eOFmatchNotFound& e){}
                    try{
                        common_match.set_tcp_dst(msg->get_match().get_tp_dst());
                        //std::cout<<"TCP_DST_not_wildcarded: "<<msg->get_match().get_tp_dst()<<"\n";

                    }catch(eOFmatchNotFound& e){}
            }catch(eOFmatchNotFound& e){}
            }
            if(msg->get_match().get_nw_proto()==17){
            try{
                try{
                    common_match.set_udp_src(msg->get_match().get_tp_src());
                    //std::cout<<"UDP_SRC_not_wildcarded: "<<msg->get_match().get_tp_src()<<"\n";

                }catch(eOFmatchNotFound& e){}
                try{
                    common_match.set_udp_dst(msg->get_match().get_tp_dst());
                    //std::cout<<"UDP_src_not_wildcarded: "<<msg->get_match().get_tp_dst()<<"\n";

                }catch(eOFmatchNotFound& e){}
            }catch(eOFmatchNotFound& e){} 
            }
        }catch(eOFmatchNotFound& e){} 
    } 
    return common_match;
}

bool      orchestrator::process_action_list_12(flowpath &flows,cofmatch common_match,cofinlist inlist, uint8_t ofversion, uint32_t inport,uint8_t nw_proto, uint8_t message){
    flowpath flowlist;
    flowlist.longest=0;
    cofinlist instrlist;
    cofmatch filling_match (OFP12_VERSION,OFPMT_OXM);
    cofinlist::iterator it;
    cofaclist::iterator it2;
    bool anyoutput=false;
    uint8_t flowtype;
    uint32_t qos_id=0;
    if (ofversion==OFP12_VERSION){   
        //instrlist.next()=cofinst_apply_actions(OFP12_VERSION);
        for (it = inlist.begin(); it != inlist.end(); ++it){
            switch(it->get_type()){
                case OFPIT_APPLY_ACTIONS:
                    instrlist.next()=cofinst_apply_actions(OFP12_VERSION);
                    break;
                case OFPIT_WRITE_ACTIONS:
                    instrlist.next()=cofinst_write_actions(OFP12_VERSION);
                    break;                    
                default:
                    //send_error_message
                    return false;
            }
            for (it2 = it->actions.begin(); it2 != it->actions.end(); ++it2){
                try{
                    std::cout<< (int)htobe16((*it2).oac_oacu.oacu_header->type)<<"\n";
                    switch (htobe16((*it2).oac_oacu.oacu_header->type)) {
                        case OFP12AT_OUTPUT:{
                            anyoutput=true;
                            flowtype = flow_direction(inport,htobe32((*it2).oac_oacu.oacu_12output->port));
                            std::cout<<"Flowtype "<< (int)flowtype<<" processing...\n";
                            fill_flowpath_with_qos(flowlist,common_match,instrlist.back().actions, inport,htobe32((*it2).oac_oacu.oacu_12output->port),flowtype,qos_id);
                            instrlist.back().actions.clear();
                            break;
                        }
                        case OFP12AT_SET_FIELD:{
                            instrlist.back().actions.next()=(*it2);
                            break;
                        }
                        case OFP12AT_SET_QUEUE:{
                            //QOS :) // implementing :

                            qos_id=htobe32((*it2).oac_oacu.oacu_12set_queue->queue_id);
                            std::cout<<"Service Flow selected: "<< qos_id <<"\n";
                            break;
                        }
                        case OFP12AT_PUSH_VLAN:{
                            instrlist.back().actions.next() = cofaction_push_vlan(OFP12_VERSION,C_TAG); 
                            break;
                        }
                        case OFP12AT_POP_VLAN:{
                            instrlist.back().actions.next() = cofaction_pop_vlan(OFP12_VERSION);
                            break;
                        }
                        default:{
                            //unsupported
                            //send error message
                            return false;
                        }
                        
                    }
                }catch(...){}                
            }

        }
    }
    flows=flowlist;
    return true;
}
bool      orchestrator::process_action_list_10(flowpath &flows,cofmatch common_match,cofaclist aclist, uint8_t ofversion, uint32_t inport,uint8_t nw_proto, uint8_t message){
    flowpath flowlist;
    flowlist.longest=0;
    cofinlist instrlist;
    cofmatch filling_match (OFP12_VERSION,OFPMT_OXM);
    cofaclist::iterator it;
    bool anyoutput=false;
    uint8_t flowtype;
    if (ofversion==OFP10_VERSION){   
        instrlist.next()=cofinst_apply_actions(OFP12_VERSION);
        for (it = aclist.begin(); it != aclist.end(); ++it){
            try{
                std::cout<< (int)htobe16((*it).oac_oacu.oacu_header->type)<<"\n";
                switch (htobe16((*it).oac_oacu.oacu_header->type)) {
                    case OFP10AT_OUTPUT:{
                        anyoutput=true;
                        
                        flowtype = flow_direction(inport,htobe16((*it).oac_oacu.oacu_10output->port));
                        std::cout<<"Flowtype "<< (int)flowtype<<" processing...\n";
                        fill_flowpath_from_10(flowlist,common_match,instrlist.back().actions, inport,htobe16((*it).oac_oacu.oacu_10output->port),flowtype);
                        instrlist.back().actions.clear();
                        break;
                        
                    }
                    case OFP10AT_SET_VLAN_VID:{
                        //std::cout<<"SET_VLAN_VID: "<< htobe16((*it).oac_oacu.oacu_10vlanvid->vlan_vid) <<"\n";
                        fflush(stdout);
                        instrlist.back().actions.next() = cofaction_push_vlan(OFP12_VERSION,C_TAG); 
                        instrlist.back().actions.next() = cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_vid (OFPVID_PRESENT|htobe16((*it).oac_oacu.oacu_10vlanvid->vlan_vid)));
                        break;
                    }
                    case OFP10AT_SET_VLAN_PCP:{
                        //std::cout<<"SET_VLAN_PCP: "<< (*it).oac_oacu.oacu_10vlanpcp->vlan_pcp <<"\n";
                        instrlist.back().actions.next() = cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_pcp ((*it).oac_oacu.oacu_10vlanpcp->vlan_pcp));
                        break;
                    }
                    case OFP10AT_STRIP_VLAN:{
                        //std::cout<<"POP VLAN\n";
                        instrlist.back().actions.next()= cofaction_pop_vlan(OFP12_VERSION);
                        break;
                    }
                    case OFP10AT_SET_DL_SRC:{
                        cmacaddr mac_dl_src((*it).oac_oacu.oacu_10dladdr->dl_addr,6);
                        //std::cout<<"SET_MAC_SRC: "<< mac_dl_src.c_str()<<"\n";
                        instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_eth_src(mac_dl_src));
                        break;
                    }
                    case OFP10AT_SET_DL_DST:{
                        cmacaddr mac_dl_dst((*it).oac_oacu.oacu_10dladdr->dl_addr,6);
                        //std::cout<<"SET_MAC_DST: "<< mac_dl_dst.c_str()<<"\n";
                        instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_eth_dst(mac_dl_dst));
                        break;
                    }
                    case OFP10AT_SET_NW_SRC:{
                        caddress ip_src(AF_INET);
                        ip_src.set_ipv4_addr(htobe32((*it).oac_oacu.oacu_10nwaddr->nw_addr));
                        //std::cout<<"SET_IP_SRC "<<ip_src.addr_c_str() <<"\n";
                        instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_ipv4_src(ip_src));                        
                        break;
                    }
                    case OFP10AT_SET_NW_DST:{
                        caddress ip_dst(AF_INET);
                        ip_dst.set_ipv4_addr(htobe32((*it).oac_oacu.oacu_10nwaddr->nw_addr));
                        //std::cout<<"SET_IP_DST "<< ip_dst.addr_c_str() <<"\n";
                        instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_ipv4_dst(ip_dst));
                        break;
                    }                                         
                    case OFP10AT_SET_NW_TOS:{
                        //std::cout<< "SET_NW_TOS: "<<(uint32_t)(*it).oac_oacu.oacu_10nwtos->nw_tos<<"\n";
                        instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_ip_ecn((*it).oac_oacu.oacu_10nwtos->nw_tos));
                        instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_ip_dscp((*it).oac_oacu.oacu_10nwtos->nw_tos));
                        break;
                    }
                    case OFP10AT_SET_TP_SRC:{
                        //std::cout<< "SET_TP_SRC "<<htobe16((*it).oac_oacu.oacu_10tpport->tp_port)<<"\n";
                        if(nw_proto==0x06 || nw_proto == 0xFF)//TCP
                            instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_tcp_src(htobe16((*it).oac_oacu.oacu_10tpport->tp_port)));
                        else
                            instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_udp_src(htobe16((*it).oac_oacu.oacu_10tpport->tp_port)));
                        break;
                    }
                    case OFP10AT_SET_TP_DST:{
                        //std::cout<< "SET_TP_DST "<<htobe16((*it).oac_oacu.oacu_10tpport->tp_port)<<"\n";
                        if(nw_proto==0x06 || nw_proto == 0xFF)//TCP
                            instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_tcp_dst(htobe16((*it).oac_oacu.oacu_10tpport->tp_port)));
                        else
                            instrlist.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_udp_dst(htobe16((*it).oac_oacu.oacu_10tpport->tp_port)));
                        break;
                    }
//                    case OFP10AT_SET_BANDWIDTH:{
//                        //std::cout<< "ACTIONS: SET_BANDWIDTH\n"<<(*it).oac_oacu.oacu_10setBW->bandwidth <<"\n";
//                        break;
//                    }
                    default:{
                       break;
                    }
                }
            }catch(...){
                return false; //ACTION LIST CANNOT BE PROCESSED
            }
        }//END FOR
        if(anyoutput==false){
            std::cout<<"OUTPORT=NONE (DROP ACTION) sending drop\n";
            fflush(stdout);
            flowtype = flow_direction(inport,0);
            fill_flowpath_from_10(flowlist,common_match,instrlist.back().actions, inport,0,flowtype);
            
        }
    }
    flows=flowlist;
    return true;
}

void      orchestrator::fill_flowpath_with_qos(flowpath &flows,cofmatch common_match,cofaclist aclist,uint32_t inport, uint32_t outport, uint8_t flowtype, uint32_t qos_id){
    std::cout<<"debugging actions entering at fill flowpath QOS "<<aclist.c_str()<<"\n";
    switch(flowtype){ //CHANGED
        case WCtoALL:{
            //not implemented yet
            break;
        }
        case WCtoSPECIAL:{ //BUG table 1 para AGS
//            std::set<cofdpt*>::iterator dpt_it2;
//            for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
//                cflowentry* temp;
//                temp = new cflowentry (OFP12_VERSION);     
//                flows.flowmodlist.insert(std::make_pair((*dpt_it2)->get_dpid(),temp));
//                flows.flowmodlist[(*dpt_it2)->get_dpid()]->match = common_match;
//                flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
//                flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,outport);
//                
//            }
            break;            
        }
        case WCtoAGS:{
//            uint64_t outdpid = proxy->virtualizer->get_own_dpid(outport);
//            
//            cflowentry* tempup2;
//            tempup2 = new cflowentry (OFP12_VERSION);
//            flows.flowmodlist.insert(std::make_pair(proxy->virtualizer->get_own_dpid(outport),tempup2));
//            flows.flowmodlist[outdpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);                
//            flows.flowmodlist[outdpid]->match = common_match;
//            flows.flowmodlist[outdpid]->set_table_id(1); //AGS table
//            
//            flows.flowmodlist[outdpid]->match.set_in_port(proxy->portconfig.cmts_port);            
//            flows.flowmodlist[outdpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION); 
//            flows.flowmodlist[outdpid]->instructions.back().actions=aclist;              
//            flows.flowmodlist[outdpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->virtualizer->get_real_port_id(outport)); 
//            
//            std::set<cofdpt*>::iterator dpt_it2;
//            for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
//                if((*dpt_it2)->get_dpid()!=proxy->config.AGS_dpid){
//                    cflowentry* temp;
//                    temp = new cflowentry (OFP12_VERSION);   
//                    flows.flowmodlist.insert(std::make_pair((*dpt_it2)->get_dpid(),temp));
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match = common_match;
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match.set_in_port(proxy->portconfig.oui_userport);
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->portconfig.oui_netport);
//                    
//                }
//
//            }
            break;
        }
        case WCtoCLIENT:{
//           uint64_t outdpid = proxy->virtualizer->get_own_dpid(outport); 
//           // uint64_t outdpid = proxy->virtualizer->get_own_dpid(12); 
//            //OUTPUT DPID
//            std::set<cofdpt*>::iterator dpt_it2;
//            for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
//                if((*dpt_it2)->get_dpid()!=proxy->config.AGS_dpid){
//                    if((*dpt_it2)->get_dpid()==outdpid){
//                        cflowentry* tempup2;
//                        tempup2 = new cflowentry (OFP12_VERSION);
//                        flows.flowmodlist.insert(std::make_pair(proxy->virtualizer->get_own_dpid(outport),tempup2));
//                        flows.flowmodlist[outdpid]->match = common_match;
//                        flows.flowmodlist[outdpid]->set_table_id(0); //AGS table
//                        flows.flowmodlist[outdpid]->match.set_in_port(proxy->portconfig.oui_netport);   
//                        
//                        flows.flowmodlist[outdpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION); 
//                        flows.flowmodlist[outdpid]->instructions.back().actions=aclist;  
//                        flows.flowmodlist[outdpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->virtualizer->get_real_port_id(outport)); 
//                    }else{
//                        cflowentry* temp;
//                        temp = new cflowentry (OFP12_VERSION);   
//                        flows.flowmodlist.insert(std::make_pair((*dpt_it2)->get_dpid(),temp));
//                        flows.flowmodlist[(*dpt_it2)->get_dpid()]->match = common_match;
//                        flows.flowmodlist[(*dpt_it2)->get_dpid()]->set_table_id(0); //AGS table
//                        flows.flowmodlist[(*dpt_it2)->get_dpid()]->match.set_in_port(proxy->portconfig.oui_userport);
//                        flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
//                        flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->portconfig.oui_netport);
//                    } 
//                }else{
//                    cflowentry* temp;
//                    temp = new cflowentry (OFP12_VERSION);   
//                    flows.flowmodlist.insert(std::make_pair((*dpt_it2)->get_dpid(),temp));
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match = common_match;
//                    
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->set_table_id(1); //AGS table
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.back().actions.next()= cofaction_push_vlan(OFP12_VERSION,C_TAG); 
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_vid (proxy->virtualizer->get_vlan_tag(proxy->config.AGS_dpid,proxy->virtualizer->get_own_dpid(outport))));  
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->portconfig.cmts_port);                    
//                }
//
//            }
            break;
        }
        case error:{
            break;
        }
        case AGStoCLI:{ //DOWNSTREAM
            uint64_t indpid = proxy->virtualizer->get_own_dpid(inport);
            uint64_t outdpid = proxy->virtualizer->get_own_dpid(outport); 
            uint16_t qos_vlan;
            if(proxy->qosmap->qos_id_is_valid(qos_id,inport,outport)){
                qos_vlan = (uint16_t)qos_id;
                std::cout<<"valid"<< (uint16_t)qos_vlan <<"\n";
                
            }else{
                qos_vlan = proxy->qosmap->get_default_queue_vlan(inport,outport); 
                std::cout<<"not valid"<< (uint16_t)qos_vlan <<"\n";
            }

            cflowentry* temp1; //for AGS
            temp1= new cflowentry (OFP12_VERSION);
            flows.flowmodlist.insert(std::make_pair(proxy->config.AGS_dpid,temp1)); //INGRESS DPID
            flows.flowmodlist[proxy->config.AGS_dpid]->match = common_match;
            flows.flowmodlist[proxy->config.AGS_dpid]->set_table_id(1); //table AGS
            flows.flowmodlist[proxy->config.AGS_dpid]->match.set_in_port(proxy->virtualizer->get_real_port_id(inport));
            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()= cofaction_push_vlan(OFP12_VERSION,C_TAG);
            std::cout<<"writing VLAN for QoS: "<< qos_vlan<<"\n";
            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_vid (qos_vlan));  
            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()= cofaction_push_vlan(OFP12_VERSION,C_TAG); 
            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_vid (proxy->virtualizer->get_vlan_tag(proxy->config.AGS_dpid,proxy->virtualizer->get_own_dpid(outport))));              
            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->portconfig.cmts_port); 

            std::cout<<"setting DOWNSTREAM flow CLIENT\n";
            cflowentry* temp2;
            temp2 = new cflowentry (OFP12_VERSION);
            flows.flowmodlist.insert(std::make_pair(proxy->virtualizer->get_own_dpid(outport),temp2)); //INGRESS DPID
            flows.flowmodlist[outdpid]->set_table_id(1); //table
            flows.flowmodlist[outdpid]->match = common_match;
            flows.flowmodlist[outdpid]->match.set_in_port(proxy->portconfig.oui_netport);
            flows.flowmodlist[outdpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
            flows.flowmodlist[outdpid]->instructions.back().actions=aclist;   
            flows.flowmodlist[outdpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->virtualizer->get_real_port_id(outport));                 
            flows.whereask=indpid;
            flows.longest=2;
            break;
        }
        case CLItoAGS:{
            uint64_t indpid = proxy->virtualizer->get_own_dpid(inport);
            uint64_t outdpid = proxy->virtualizer->get_own_dpid(outport);
            uint16_t qos_vlan;
            if(proxy->qosmap->qos_id_is_valid(qos_id,inport,outport))
                qos_vlan = (uint16_t)qos_id;
            else
                qos_vlan = proxy->qosmap->get_default_queue_vlan(inport,outport);
            
            
            std::cout<<"setting UPSTREAM flow\n";
            cflowentry* tempup1;
            tempup1 = new cflowentry (OFP12_VERSION);
            flows.flowmodlist.insert(std::make_pair(proxy->virtualizer->get_own_dpid(inport),tempup1));
            flows.flowmodlist[indpid]->match = common_match;
            flows.flowmodlist[indpid]->set_table_id(1); //table CLI
            flows.flowmodlist[indpid]->match.set_in_port(proxy->virtualizer->get_real_port_id(inport));
            flows.flowmodlist[indpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
            //signalling QoS

            flows.flowmodlist[indpid]->instructions.back().actions.next()= cofaction_push_vlan(OFP12_VERSION,C_TAG);
            std::cout<<"writing VLAN for QoS: "<< qos_vlan<<"\n";
            flows.flowmodlist[indpid]->instructions.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_vid (qos_vlan));
            ///////////
            flows.flowmodlist[indpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->portconfig.oui_netport); 

            cflowentry* tempup2;
            tempup2 = new cflowentry (OFP12_VERSION);
            flows.flowmodlist.insert(std::make_pair(proxy->virtualizer->get_own_dpid(outport),tempup2));
            flows.flowmodlist[outdpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);                
            flows.flowmodlist[outdpid]->match = common_match;
            flows.flowmodlist[outdpid]->set_table_id(1); //AGS table
            flows.flowmodlist[outdpid]->match.set_in_port(proxy->portconfig.cmts_port);
            //flows.flowmodlist[outdpid]->match.set_metadata((uint64_t)proxy->virtualizer->get_vlan_tag(indpid,outdpid));   0.4VERSION
            flows.flowmodlist[outdpid]->match.set_metadata(0x0200000000000000);                                           //0.5VERSION
            flows.flowmodlist[outdpid]->instructions.back().actions=aclist;  
         
            flows.flowmodlist[outdpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->virtualizer->get_real_port_id(outport));                    
            flows.whereask=indpid;
            flows.longest=2;

            break;            

        }
        case CLItoCLI:{
//            uint64_t indpid = proxy->virtualizer->get_own_dpid(inport);
//            uint64_t outdpid = proxy->virtualizer->get_own_dpid(outport);
//            cflowentry* tempb1;
//            tempb1 = new cflowentry (OFP12_VERSION);                    
//            flows.flowmodlist.insert(std::make_pair(indpid,tempb1)); //INGRESS DPID
//            flows.flowmodlist[indpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);   
//            flows.flowmodlist[indpid]->match = common_match;
//            flows.flowmodlist[indpid]->set_table_id(0);; //table CLIENT
//            flows.flowmodlist[indpid]->match.set_in_port(proxy->virtualizer->get_real_port_id(inport));
//            flows.flowmodlist[indpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->portconfig.oui_netport); 
//                    
//            cflowentry* tempb2;
//            tempb2 = new cflowentry (OFP12_VERSION);
//            flows.flowmodlist.insert(std::make_pair(proxy->config.AGS_dpid,tempb2)); //INGRESS DPID
//            flows.flowmodlist[proxy->config.AGS_dpid]->set_table_id(1); //table AGS
//
//            flows.flowmodlist[proxy->config.AGS_dpid]->match = common_match;
//            flows.flowmodlist[proxy->config.AGS_dpid]->match.set_in_port(proxy->portconfig.cmts_port);
//            flows.flowmodlist[proxy->config.AGS_dpid]->match.set_metadata(proxy->virtualizer->get_vlan_tag(indpid,outdpid));
//            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);  
//
//            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next() = cofaction_push_vlan(OFP12_VERSION,C_TAG); 
//            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next() = cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_vid (OFPVID_PRESENT|proxy->virtualizer->get_vlan_tag(proxy->config.AGS_dpid,outdpid)));                
//            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next() = cofaction_output(OFP12_VERSION,OFPP12_IN_PORT); 
//
//            cflowentry* tempb3;
//            tempb3 = new cflowentry (OFP12_VERSION);     
//            flows.flowmodlist.insert(std::make_pair(proxy->virtualizer->get_own_dpid(outport),tempb3)); //INGRESS DPID
//            flows.flowmodlist[outdpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
//            flows.flowmodlist[outdpid]->set_table_id(0); //table client
//            flows.flowmodlist[outdpid]->match = common_match;
//            flows.flowmodlist[outdpid]->match.set_in_port(proxy->portconfig.oui_netport);
//            flows.flowmodlist[outdpid]->instructions.back().actions=aclist;              
//            flows.flowmodlist[outdpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->virtualizer->get_real_port_id(outport));             
            break;
        }
        case AGStoALL:{
//            cflowentry* temp1;
//            temp1= new cflowentry (OFP12_VERSION);
//            flows.flowmodlist.insert(std::make_pair(proxy->config.AGS_dpid,temp1)); //INGRESS DPID
//            flows.flowmodlist[proxy->config.AGS_dpid]->match = common_match;
//            flows.flowmodlist[proxy->config.AGS_dpid]->set_table_id(1); //table AGS
//            flows.flowmodlist[proxy->config.AGS_dpid]->match.set_in_port(proxy->virtualizer->get_real_port_id(inport));
//            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
//            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()= cofaction_push_vlan(OFP12_VERSION,C_TAG); 
//            std::set<cofdpt*>::iterator dpt_it2;            
//            for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
//                if((*dpt_it2)->get_dpid()!=proxy->config.AGS_dpid){
//                    cflowentry* temp;
//                    temp = new cflowentry (OFP12_VERSION);   
//                    flows.flowmodlist.insert(std::make_pair((*dpt_it2)->get_dpid(),temp));
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match = common_match;
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match.set_in_port(proxy->portconfig.oui_netport);
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.next().actions=aclist;
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->portconfig.cmts_port);
//                    
//                    flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next() = cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_vid(proxy->virtualizer->get_vlan_tag(proxy->config.AGS_dpid,(*dpt_it2)->get_dpid())));  
//                    flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->portconfig.cmts_port);
//                }
//
//            }
            break;
        }
        case CLItoALL:{
//            uint64_t indpid = proxy->virtualizer->get_own_dpid(inport);
//
//            cflowentry* tempb1;
//            tempb1 = new cflowentry (OFP12_VERSION);                    
//            flows.flowmodlist.insert(std::make_pair(indpid,tempb1)); //INGRESS DPID
//            flows.flowmodlist[indpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);   
//            flows.flowmodlist[indpid]->match = common_match;
//            flows.flowmodlist[indpid]->set_table_id(0);; //table CLIENT
//            flows.flowmodlist[indpid]->match.set_in_port(proxy->virtualizer->get_real_port_id(inport));
//            flows.flowmodlist[indpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->portconfig.oui_netport); 
//                    
//            cflowentry* tempb2;
//            tempb2 = new cflowentry (OFP12_VERSION);
//            flows.flowmodlist.insert(std::make_pair(proxy->config.AGS_dpid,tempb2)); //INGRESS DPID
//            flows.flowmodlist[proxy->config.AGS_dpid]->set_table_id(1); //table AGS
//
//            flows.flowmodlist[proxy->config.AGS_dpid]->match = common_match;
//            flows.flowmodlist[proxy->config.AGS_dpid]->match.set_in_port(proxy->portconfig.cmts_port);
//            flows.flowmodlist[proxy->config.AGS_dpid]->match.set_metadata(proxy->virtualizer->get_vlan_tag(indpid,proxy->config.AGS_dpid));
//            
//            std::set<cofdpt*>::iterator dpt_it2;
//            bool tagged;
//            for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
//                if((*dpt_it2)->get_dpid()!=proxy->config.AGS_dpid && (*dpt_it2)->get_dpid()!=indpid){
//                    cflowentry* temp;
//                    temp = new cflowentry (OFP12_VERSION);   
//                    flows.flowmodlist.insert(std::make_pair((*dpt_it2)->get_dpid(),temp));
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match = common_match;
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match.set_in_port(proxy->portconfig.oui_netport);
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.next().actions=aclist;
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->portconfig.oui_userport);
//                    
//                    flows.flowmodlist[proxy->config.AGS_dpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
//                    flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()= cofaction_push_vlan(OFP12_VERSION,C_TAG); 
//                    flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next() = cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_vid(proxy->virtualizer->get_vlan_tag(proxy->config.AGS_dpid,(*dpt_it2)->get_dpid())));  
//                    tagged=true;
//                }
//
//            }
//            if(tagged==true){
//                flows.flowmodlist[proxy->config.AGS_dpid]->instructions.next()=cofinst_write_actions(OFP12_VERSION);
//                flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()= cofaction_pop_vlan(OFP12_VERSION); 
//                flows.flowmodlist[proxy->config.AGS_dpid]->instructions.next().actions=aclist;
//                flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->portconfig.data_port);  
//                
//            }else{
//                flows.flowmodlist[proxy->config.AGS_dpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
//                flows.flowmodlist[proxy->config.AGS_dpid]->instructions.next().actions=aclist;
//                flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->portconfig.data_port);                
//            }
            break;          
        }
        case VirtPorttoSPECIAL:{
//            uint64_t indpid = proxy->virtualizer->get_own_dpid(inport);
//            cflowentry* templ;
//            templ = new cflowentry (OFP12_VERSION);
//            flows.flowmodlist.insert(std::make_pair(indpid,templ)); //INGRESS DPID
//            flows.flowmodlist[indpid]->match = common_match;
//            
//            if(indpid==proxy->config.AGS_dpid){
//                flows.flowmodlist[indpid]->set_table_id(1); //table AGS
//            }else{
//                flows.flowmodlist[indpid]->set_table_id(0); //table CLIENT                       
//            }
//            flows.flowmodlist[indpid]->match.set_in_port(proxy->virtualizer->get_real_port_id(inport));
//            flows.flowmodlist[indpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
//            flows.flowmodlist[indpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,outport); 
            break;  
        }
        case DROP:{
//            uint64_t indpid = proxy->virtualizer->get_own_dpid(inport);
//            cflowentry* templ;
//            templ = new cflowentry (OFP12_VERSION);
//            flows.flowmodlist.insert(std::make_pair(indpid,templ)); //INGRESS DPID
//            flows.flowmodlist[indpid]->match = common_match;
//            
//            if(indpid==proxy->config.AGS_dpid){
//                flows.flowmodlist[indpid]->set_table_id(1); //table AGS
//            }else{
//                flows.flowmodlist[indpid]->set_table_id(0); //table CLIENT                       
//            }
//            flows.flowmodlist[indpid]->match.set_in_port(proxy->virtualizer->get_real_port_id(inport));
            break;
        }
        case WDROP:{
//            std::set<cofdpt*>::iterator dpt_it2;
//            for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
//                cflowentry* temp;
//                temp = new cflowentry (OFP12_VERSION);   
//                flows.flowmodlist.insert(std::make_pair((*dpt_it2)->get_dpid(),temp));
//                flows.flowmodlist[(*dpt_it2)->get_dpid()]->match = common_match;
//                if((*dpt_it2)->get_dpid()==proxy->config.AGS_dpid){
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match.set_in_port(proxy->portconfig.data_port);
//                }else{
//                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match.set_in_port(proxy->portconfig.oui_userport);
//                }
//
//            }
            break;
        }
        
    }   
    
}
void      orchestrator::fill_flowpath_from_10(flowpath &flows,cofmatch common_match,cofaclist aclist,uint32_t inport, uint32_t outport, uint8_t flowtype){
    std::cout<<"debugging actions entering at fill flowpath "<<aclist.c_str()<<"\n";
    switch(flowtype){ //CHANGED
        case WCtoALL:{
            //not implemented yet
            break;
        }
        case WCtoSPECIAL:{ //BUG table 1 para AGS
            std::set<cofdpt*>::iterator dpt_it2;
            for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
                cflowentry* temp;
                temp = new cflowentry (OFP12_VERSION);     
                flows.flowmodlist.insert(std::make_pair((*dpt_it2)->get_dpid(),temp));
                flows.flowmodlist[(*dpt_it2)->get_dpid()]->match = common_match;
                flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
                flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,outport);
                
            }
            break;            
        }
        case WCtoAGS:{
            uint64_t outdpid = proxy->virtualizer->get_own_dpid(outport);
            
            cflowentry* tempup2;
            tempup2 = new cflowentry (OFP12_VERSION);
            flows.flowmodlist.insert(std::make_pair(proxy->virtualizer->get_own_dpid(outport),tempup2));
            flows.flowmodlist[outdpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);                
            flows.flowmodlist[outdpid]->match = common_match;
            flows.flowmodlist[outdpid]->set_table_id(1); //AGS table
            
            flows.flowmodlist[outdpid]->match.set_in_port(proxy->portconfig.cmts_port);            
            flows.flowmodlist[outdpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION); 
            flows.flowmodlist[outdpid]->instructions.back().actions=aclist;              
            flows.flowmodlist[outdpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->virtualizer->get_real_port_id(outport)); 
            
            std::set<cofdpt*>::iterator dpt_it2;
            for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
                if((*dpt_it2)->get_dpid()!=proxy->config.AGS_dpid){
                    cflowentry* temp;
                    temp = new cflowentry (OFP12_VERSION);   
                    flows.flowmodlist.insert(std::make_pair((*dpt_it2)->get_dpid(),temp));
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match = common_match;
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match.set_in_port(proxy->portconfig.oui_userport);
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->portconfig.oui_netport);
                    
                }

            }
            break;
        }
        case WCtoCLIENT:{
           uint64_t outdpid = proxy->virtualizer->get_own_dpid(outport); 
           // uint64_t outdpid = proxy->virtualizer->get_own_dpid(12); 
            //OUTPUT DPID
            std::set<cofdpt*>::iterator dpt_it2;
            for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
                if((*dpt_it2)->get_dpid()!=proxy->config.AGS_dpid){
                    if((*dpt_it2)->get_dpid()==outdpid){
                        cflowentry* tempup2;
                        tempup2 = new cflowentry (OFP12_VERSION);
                        flows.flowmodlist.insert(std::make_pair(proxy->virtualizer->get_own_dpid(outport),tempup2));
                        flows.flowmodlist[outdpid]->match = common_match;
                        flows.flowmodlist[outdpid]->set_table_id(0); //AGS table
                        flows.flowmodlist[outdpid]->match.set_in_port(proxy->portconfig.oui_netport);   
                        
                        flows.flowmodlist[outdpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION); 
                        flows.flowmodlist[outdpid]->instructions.back().actions=aclist;  
                        flows.flowmodlist[outdpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->virtualizer->get_real_port_id(outport)); 
                    }else{
                        cflowentry* temp;
                        temp = new cflowentry (OFP12_VERSION);   
                        flows.flowmodlist.insert(std::make_pair((*dpt_it2)->get_dpid(),temp));
                        flows.flowmodlist[(*dpt_it2)->get_dpid()]->match = common_match;
                        flows.flowmodlist[(*dpt_it2)->get_dpid()]->set_table_id(0); //AGS table
                        flows.flowmodlist[(*dpt_it2)->get_dpid()]->match.set_in_port(proxy->portconfig.oui_userport);
                        flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
                        flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->portconfig.oui_netport);
                    } 
                }else{
                    cflowentry* temp;
                    temp = new cflowentry (OFP12_VERSION);   
                    flows.flowmodlist.insert(std::make_pair((*dpt_it2)->get_dpid(),temp));
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match = common_match;
                    
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->set_table_id(1); //AGS table
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.back().actions.next()= cofaction_push_vlan(OFP12_VERSION,C_TAG); 
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_vid (proxy->virtualizer->get_vlan_tag(proxy->config.AGS_dpid,proxy->virtualizer->get_own_dpid(outport))));  
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->portconfig.cmts_port);                    
                }

            }
            break;
        }
        case error:{
            break;
        }
        case AGStoCLI:{
            uint64_t indpid = proxy->virtualizer->get_own_dpid(inport);
            uint64_t outdpid = proxy->virtualizer->get_own_dpid(outport); 
            
            cflowentry* temp1;
            temp1= new cflowentry (OFP12_VERSION);
            flows.flowmodlist.insert(std::make_pair(proxy->config.AGS_dpid,temp1)); //INGRESS DPID
            flows.flowmodlist[proxy->config.AGS_dpid]->match = common_match;
            flows.flowmodlist[proxy->config.AGS_dpid]->set_table_id(1); //table AGS
            flows.flowmodlist[proxy->config.AGS_dpid]->match.set_in_port(proxy->virtualizer->get_real_port_id(inport));
            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()= cofaction_push_vlan(OFP12_VERSION,C_TAG); 
            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()= cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_vid (OFPVID_PRESENT|proxy->virtualizer->get_vlan_tag(proxy->config.AGS_dpid,proxy->virtualizer->get_own_dpid(outport))));                                                         
            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->portconfig.cmts_port); 

            std::cout<<"setting DOWNSTREAM flow CLIENT\n";
            cflowentry* temp2;
            temp2 = new cflowentry (OFP12_VERSION);
            flows.flowmodlist.insert(std::make_pair(proxy->virtualizer->get_own_dpid(outport),temp2)); //INGRESS DPID
            flows.flowmodlist[outdpid]->set_table_id(0); //table
            flows.flowmodlist[outdpid]->match = common_match;
            flows.flowmodlist[outdpid]->match.set_in_port(proxy->portconfig.oui_netport);
            flows.flowmodlist[outdpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
            flows.flowmodlist[outdpid]->instructions.back().actions=aclist;   
            flows.flowmodlist[outdpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->virtualizer->get_real_port_id(outport));                 
            flows.whereask=indpid;
            flows.longest=2;
            break;
        }
        case CLItoAGS:{
            uint64_t indpid = proxy->virtualizer->get_own_dpid(inport);
            uint64_t outdpid = proxy->virtualizer->get_own_dpid(outport);
            std::cout<<"setting UPSTREAM flow\n";
            cflowentry* tempup1;
            tempup1 = new cflowentry (OFP12_VERSION);
            flows.flowmodlist.insert(std::make_pair(proxy->virtualizer->get_own_dpid(inport),tempup1));
            flows.flowmodlist[indpid]->match = common_match;
            flows.flowmodlist[indpid]->set_table_id(0); //table CLI
            flows.flowmodlist[indpid]->match.set_in_port(proxy->virtualizer->get_real_port_id(inport));
            flows.flowmodlist[indpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
            flows.flowmodlist[indpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->portconfig.oui_netport); 

            cflowentry* tempup2;
            tempup2 = new cflowentry (OFP12_VERSION);
            flows.flowmodlist.insert(std::make_pair(proxy->virtualizer->get_own_dpid(outport),tempup2));
            flows.flowmodlist[outdpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);                
            flows.flowmodlist[outdpid]->match = common_match;
            flows.flowmodlist[outdpid]->set_table_id(1); //AGS table
            flows.flowmodlist[outdpid]->match.set_in_port(proxy->portconfig.cmts_port);
            flows.flowmodlist[outdpid]->match.set_metadata((uint64_t)proxy->virtualizer->get_vlan_tag(indpid,outdpid));
            flows.flowmodlist[outdpid]->instructions.back().actions=aclist;              
            flows.flowmodlist[outdpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->virtualizer->get_real_port_id(outport));                    
            flows.whereask=indpid;
            flows.longest=2;
            break;            

        }
        case CLItoCLI:{
            uint64_t indpid = proxy->virtualizer->get_own_dpid(inport);
            uint64_t outdpid = proxy->virtualizer->get_own_dpid(outport);
            cflowentry* tempb1;
            tempb1 = new cflowentry (OFP12_VERSION);                    
            flows.flowmodlist.insert(std::make_pair(indpid,tempb1)); //INGRESS DPID
            flows.flowmodlist[indpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);   
            flows.flowmodlist[indpid]->match = common_match;
            flows.flowmodlist[indpid]->set_table_id(0);; //table CLIENT
            flows.flowmodlist[indpid]->match.set_in_port(proxy->virtualizer->get_real_port_id(inport));
            flows.flowmodlist[indpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->portconfig.oui_netport); 
                    
            cflowentry* tempb2;
            tempb2 = new cflowentry (OFP12_VERSION);
            flows.flowmodlist.insert(std::make_pair(proxy->config.AGS_dpid,tempb2)); //INGRESS DPID
            flows.flowmodlist[proxy->config.AGS_dpid]->set_table_id(1); //table AGS

            flows.flowmodlist[proxy->config.AGS_dpid]->match = common_match;
            flows.flowmodlist[proxy->config.AGS_dpid]->match.set_in_port(proxy->portconfig.cmts_port);
            flows.flowmodlist[proxy->config.AGS_dpid]->match.set_metadata(proxy->virtualizer->get_vlan_tag(indpid,outdpid));
            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);  

            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next() = cofaction_push_vlan(OFP12_VERSION,C_TAG); 
            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next() = cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_vid (OFPVID_PRESENT|proxy->virtualizer->get_vlan_tag(proxy->config.AGS_dpid,outdpid)));                
            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next() = cofaction_output(OFP12_VERSION,OFPP12_IN_PORT); 

            cflowentry* tempb3;
            tempb3 = new cflowentry (OFP12_VERSION);     
            flows.flowmodlist.insert(std::make_pair(proxy->virtualizer->get_own_dpid(outport),tempb3)); //INGRESS DPID
            flows.flowmodlist[outdpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
            flows.flowmodlist[outdpid]->set_table_id(0); //table client
            flows.flowmodlist[outdpid]->match = common_match;
            flows.flowmodlist[outdpid]->match.set_in_port(proxy->portconfig.oui_netport);
            flows.flowmodlist[outdpid]->instructions.back().actions=aclist;              
            flows.flowmodlist[outdpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->virtualizer->get_real_port_id(outport));             
            break;
        }
        case AGStoALL:{
            cflowentry* temp1;
            temp1= new cflowentry (OFP12_VERSION);
            flows.flowmodlist.insert(std::make_pair(proxy->config.AGS_dpid,temp1)); //INGRESS DPID
            flows.flowmodlist[proxy->config.AGS_dpid]->match = common_match;
            flows.flowmodlist[proxy->config.AGS_dpid]->set_table_id(1); //table AGS
            flows.flowmodlist[proxy->config.AGS_dpid]->match.set_in_port(proxy->virtualizer->get_real_port_id(inport));
            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
            flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()= cofaction_push_vlan(OFP12_VERSION,C_TAG); 
            std::set<cofdpt*>::iterator dpt_it2;            
            for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
                if((*dpt_it2)->get_dpid()!=proxy->config.AGS_dpid){
                    cflowentry* temp;
                    temp = new cflowentry (OFP12_VERSION);   
                    flows.flowmodlist.insert(std::make_pair((*dpt_it2)->get_dpid(),temp));
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match = common_match;
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match.set_in_port(proxy->portconfig.oui_netport);
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.next().actions=aclist;
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->portconfig.cmts_port);
                    
                    flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next() = cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_vid(proxy->virtualizer->get_vlan_tag(proxy->config.AGS_dpid,(*dpt_it2)->get_dpid())));  
                    flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->portconfig.cmts_port);
                }

            }
            break;
        }
        case CLItoALL:{
            uint64_t indpid = proxy->virtualizer->get_own_dpid(inport);

            cflowentry* tempb1;
            tempb1 = new cflowentry (OFP12_VERSION);                    
            flows.flowmodlist.insert(std::make_pair(indpid,tempb1)); //INGRESS DPID
            flows.flowmodlist[indpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);   
            flows.flowmodlist[indpid]->match = common_match;
            flows.flowmodlist[indpid]->set_table_id(0);; //table CLIENT
            flows.flowmodlist[indpid]->match.set_in_port(proxy->virtualizer->get_real_port_id(inport));
            flows.flowmodlist[indpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,proxy->portconfig.oui_netport); 
                    
            cflowentry* tempb2;
            tempb2 = new cflowentry (OFP12_VERSION);
            flows.flowmodlist.insert(std::make_pair(proxy->config.AGS_dpid,tempb2)); //INGRESS DPID
            flows.flowmodlist[proxy->config.AGS_dpid]->set_table_id(1); //table AGS

            flows.flowmodlist[proxy->config.AGS_dpid]->match = common_match;
            flows.flowmodlist[proxy->config.AGS_dpid]->match.set_in_port(proxy->portconfig.cmts_port);
            flows.flowmodlist[proxy->config.AGS_dpid]->match.set_metadata(proxy->virtualizer->get_vlan_tag(indpid,proxy->config.AGS_dpid));
            
            std::set<cofdpt*>::iterator dpt_it2;
            bool tagged;
            for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
                if((*dpt_it2)->get_dpid()!=proxy->config.AGS_dpid && (*dpt_it2)->get_dpid()!=indpid){
                    cflowentry* temp;
                    temp = new cflowentry (OFP12_VERSION);   
                    flows.flowmodlist.insert(std::make_pair((*dpt_it2)->get_dpid(),temp));
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match = common_match;
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match.set_in_port(proxy->portconfig.oui_netport);
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.next().actions=aclist;
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->portconfig.oui_userport);
                    
                    flows.flowmodlist[proxy->config.AGS_dpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
                    flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()= cofaction_push_vlan(OFP12_VERSION,C_TAG); 
                    flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next() = cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_vid(proxy->virtualizer->get_vlan_tag(proxy->config.AGS_dpid,(*dpt_it2)->get_dpid())));  
                    tagged=true;
                }

            }
            if(tagged==true){
                flows.flowmodlist[proxy->config.AGS_dpid]->instructions.next()=cofinst_write_actions(OFP12_VERSION);
                flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()= cofaction_pop_vlan(OFP12_VERSION); 
                flows.flowmodlist[proxy->config.AGS_dpid]->instructions.next().actions=aclist;
                flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->portconfig.data_port);  
                
            }else{
                flows.flowmodlist[proxy->config.AGS_dpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
                flows.flowmodlist[proxy->config.AGS_dpid]->instructions.next().actions=aclist;
                flows.flowmodlist[proxy->config.AGS_dpid]->instructions.back().actions.next()=cofaction_output(OFP12_VERSION,proxy->portconfig.data_port);                
            }
            break;          
        }
        case VirtPorttoSPECIAL:{
            uint64_t indpid = proxy->virtualizer->get_own_dpid(inport);
            cflowentry* templ;
            templ = new cflowentry (OFP12_VERSION);
            flows.flowmodlist.insert(std::make_pair(indpid,templ)); //INGRESS DPID
            flows.flowmodlist[indpid]->match = common_match;
            
            if(indpid==proxy->config.AGS_dpid){
                flows.flowmodlist[indpid]->set_table_id(1); //table AGS
            }else{
                flows.flowmodlist[indpid]->set_table_id(0); //table CLIENT                       
            }
            flows.flowmodlist[indpid]->match.set_in_port(proxy->virtualizer->get_real_port_id(inport));
            flows.flowmodlist[indpid]->instructions.next()=cofinst_apply_actions(OFP12_VERSION);
            flows.flowmodlist[indpid]->instructions.back().actions.next()= cofaction_output(OFP12_VERSION,outport); 
            break;
        }
        case DROP:{
            uint64_t indpid = proxy->virtualizer->get_own_dpid(inport);
            cflowentry* templ;
            templ = new cflowentry (OFP12_VERSION);
            flows.flowmodlist.insert(std::make_pair(indpid,templ)); //INGRESS DPID
            flows.flowmodlist[indpid]->match = common_match;
            
            if(indpid==proxy->config.AGS_dpid){
                flows.flowmodlist[indpid]->set_table_id(1); //table AGS
            }else{
                flows.flowmodlist[indpid]->set_table_id(0); //table CLIENT                       
            }
            flows.flowmodlist[indpid]->match.set_in_port(proxy->virtualizer->get_real_port_id(inport));
            break;
        }
        case WDROP:{
            std::set<cofdpt*>::iterator dpt_it2;
            for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
                cflowentry* temp;
                temp = new cflowentry (OFP12_VERSION);   
                flows.flowmodlist.insert(std::make_pair((*dpt_it2)->get_dpid(),temp));
                flows.flowmodlist[(*dpt_it2)->get_dpid()]->match = common_match;
                if((*dpt_it2)->get_dpid()==proxy->config.AGS_dpid){
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match.set_in_port(proxy->portconfig.data_port);
                }else{
                    flows.flowmodlist[(*dpt_it2)->get_dpid()]->match.set_in_port(proxy->portconfig.oui_userport);
                }

            }
            break;
        }
        
    }
    
}

uint8_t   orchestrator::typeflow_old(uint64_t src_dpid,uint64_t dst_dpid){
    if(src_dpid==0)
        return ALL;
    if(src_dpid==takefromoutput)
        return LOCAL;
    if(src_dpid==proxy->config.AGS_dpid){
        if(src_dpid==dst_dpid)
            return LOCAL;
        else
            return DOWNSTREAM;

    }else{
            if(dst_dpid==proxy->config.AGS_dpid){
               //UPSTREAM Flow
                return UPSTREAM;
            }else{
                if(dst_dpid==src_dpid){
                    //local Flow
                    return LOCAL;
                }else{
                    //Flow between clients
                    return BCLIENTS;
                }
            }
        }
}
uint8_t   orchestrator::flow_direction(uint32_t inport,uint32_t outport){
    if(inport==0){
        //inport wildcarded
        if(outport<OFPP10_MAX){
            //Wildcard->Virtualport
            if(outport==0){
                return WDROP;
            }
            uint64_t outdpid = proxy->virtualizer->get_own_dpid(outport);
            if(outdpid==proxy->config.AGS_dpid){
                return WCtoAGS;
            }else{
                return WCtoCLIENT;
            }
        }else{

            if(outport==OFPP10_ALL){
                return WCtoALL;
            }else{
                //controller or IN_PORT
                return WCtoSPECIAL;                
            }
        }
    }else{
        //inport NOT wildcarded
        if(outport<OFPP10_MAX){
            //VirtPort->Virtualport
            if(outport==0){
                return DROP;
            }
            uint64_t indpid = proxy->virtualizer->get_own_dpid(inport);
            uint64_t outdpid = proxy->virtualizer->get_own_dpid(outport);
            if(indpid==0||outdpid==0){
                return error;                
            }else{
                if(indpid==proxy->config.AGS_dpid){
                    return AGStoCLI;
                }else{
                    if(outdpid==proxy->config.AGS_dpid){
                        return CLItoAGS;
                    }else{
                        return CLItoCLI;
                    }
                }
            }
            
        }else{
            if(outport==OFPP10_ALL){
                //VirtPorttoALL;
                uint64_t indpid = proxy->virtualizer->get_own_dpid(inport);
                if(indpid==proxy->config.AGS_dpid){
                    return AGStoALL;
                }else{
                    return CLItoALL;
                }                
            }else{
                //controller or IN_PORT
                return VirtPorttoSPECIAL;                
            }
        }
    }
}
/********************************  STATISTICS *****************************************/
void      orchestrator::handle_flow_stats_request (cofctl *ctl, cofmsg_flow_stats_request *msg){
    std::cout<<"flow_stats_REQUEST received\n";
    uint64_t realcookie;
    uint16_t virtualcookie;
    uint32_t port;
    try{
        realcookie=msg->get_flow_stats().get_cookie();
        virtualcookie= proxy->flowcache->get_virtual_cookie(realcookie);
        
    }catch(...){}
    try{
        port = msg->get_flow_stats().get_match().get_in_port();
    }catch(...){}
    cofmatch common_match (OFP12_VERSION);
    common_match = process_matching(msg->get_flow_stats().get_match());
    
    if(port!=0){
        //inport is not WILDCARDED
        std::cout<<"inport is NOT wildcarded\n";
        uint64_t dpid = proxy->virtualizer->get_own_dpid(port);
        cofflow_stats_request stats_request = msg->get_flow_stats();
        common_match.set_in_port(proxy->virtualizer->get_real_port_id(port));
        stats_request.set_match(common_match);
        if(realcookie!=0)
            stats_request.set_cookie(proxy->flowcache->get_virtual_cookie(virtualcookie));
        proxy->send_flow_stats_request(proxy->dpt_find(dpid),msg->get_stats_flags(),stats_request);
    }else{
        std::set<cofdpt*>::iterator dpt_it2;
        for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
            std::cout<<"Inport WILDCARDED\n";
            uint8_t tableid=0;
            if ((*dpt_it2)->get_dpid()==proxy->config.AGS_dpid){
                tableid=1;
            }
            cofflow_stats_request stats_request (OFP12_VERSION,common_match,tableid,msg->get_flow_stats().get_out_port());
            if(realcookie!=0){
                stats_request.set_cookie(proxy->flowcache->get_virtual_cookie(virtualcookie));
            }
            std::cout<<"sending FLOW_STATS_REQUEST to"<< (*dpt_it2)->get_dpid_s()<<"\n";
            proxy->send_flow_stats_request((*dpt_it2),msg->get_stats_flags(),stats_request);

        }
    }
    delete msg;
    return;
}
void      orchestrator::handle_flow_stats_reply (cofdpt *dpt, cofmsg_flow_stats_reply *msg){
    std::cout<<"Flow_stats_received\n";
    std::vector<cofflow_stats_reply> templist;
    std::vector<cofflow_stats_reply>::iterator it;
        
    for(it=msg->get_flow_stats().begin();it!=msg->get_flow_stats().end();++it){
        if(proxy->flowcache->get_flow(it->get_cookie()).ask_dpid==dpt->get_dpid()){
            cofflow_stats_reply temp(
			OFP10_VERSION,
			0,
			it->get_duration_sec(),
			it->get_duration_nsec(),
			it->get_priority(),
			it->get_idle_timeout(),
			it->get_hard_timeout(),
			proxy->flowcache->get_flow(it->get_cookie()).fe->get_cookie(),
			it->get_packet_count(),
			it->get_byte_count(),
			proxy->flowcache->get_flow(it->get_cookie()).fe->match,
			proxy->flowcache->get_flow(it->get_cookie()).fe->actions);
            templist.push_back(temp);
        }
        //else ignore stat :)
    }
    proxy->send_flow_stats_reply(proxy->controller,0x5621,templist,true);
}
void      orchestrator::handle_table_stats_request (cofctl *ctl, cofmsg_table_stats_request *msg){
    //ignore
    if (ctl->get_version()==OFP12_VERSION){
        std::vector<coftable_stats_reply> table_stats;
        
        coftable_stats_reply stat (OFP12_VERSION);
        stat.set_apply_actions(0);
        stat.set_instructions(0);
        stat.set_match(0);
        stat.set_metadata_match(0);
        stat.set_table_id(0);
        stat.set_write_actions(0);
        table_stats.push_back(stat);
        proxy->send_table_stats_reply(proxy->controller,msg->get_xid(),table_stats);

    }else{
        proxy->send_error_message(proxy->controller,msg->get_xid(),OFPET_BAD_REQUEST,OFPBRC_BAD_STAT,msg->soframe(),msg->framelen());  

    }
    delete msg;
    return;


} //IGNORING
void      orchestrator::handle_port_stats_request (cofctl *ctl, cofmsg_port_stats_request *msg){
    try{
        if(msg->get_port_stats().get_portno()==OFPP10_NONE){
            std::set<cofdpt*>::iterator dpt_it2;
            for (dpt_it2 = proxy->ofdpt_set.begin(); dpt_it2 != proxy->ofdpt_set.end(); ++dpt_it2) {
                std::cout<<"sending PORT_STATS_REQUEST to"<< (*dpt_it2)->get_dpid_s()<<"\n";
                cofport_stats_request request(OFP12_VERSION,OFPP12_ANY);
                proxy->send_port_stats_request((*dpt_it2),msg->get_stats_flags(),request);   
            }
        }else{
            cofport_stats_request request(OFP12_VERSION,proxy->virtualizer->get_real_port_id(msg->get_port_stats().get_portno()));
            proxy->send_port_stats_request(proxy->dpt_find(proxy->virtualizer->get_own_dpid(msg->get_port_stats().get_portno())),msg->get_stats_flags(),request);            
        }

    }catch(...){
        //something happened;
    }
    delete msg;
    return;
}
void      orchestrator::handle_port_stats_reply (cofdpt *dpt, cofmsg_port_stats_reply *msg){
    std::vector<cofport_stats_reply> templist;
    std::vector<cofport_stats_reply>::iterator it;
    for(it=msg->get_port_stats().begin();it!=msg->get_port_stats().end();++it){
        if(proxy->discover->is_hidden_port(dpt->get_dpid(),it->get_portno())==false){
            cofport_stats_reply temp(
			OFP10_VERSION,
			proxy->virtualizer->get_virtual_port_id(dpt->get_dpid(),it->get_portno()),
			it->get_rx_packets(),
			it->get_tx_packets(),
			it->get_rx_bytes(),
			it->get_tx_bytes(),
			it->get_rx_dropped(),
			it->get_tx_dropped(),
			it->get_rx_errors(),
			it->get_tx_errors(),
			it->get_rx_frame_err(),
			it->get_rx_over_err(),
			it->get_rx_crc_err(),
			it->get_collisions());
            templist.push_back(temp);
        }
        //else ignore stat :)
    }
    proxy->send_port_stats_reply(proxy->controller,0x5621,templist,true);
}

void      orchestrator::handle_queue_get_config_request(cofctl* ctl, cofmsg_queue_get_config_request* msg) {
    //std::cout <<"Queue request received for port:"<< msg->get_port_no() << "\n";
    std::vector<QueueList> templist;
    templist = proxy->qosmap->get_queues(msg->get_port_no());
    std::vector<QueueList>::iterator iter;
    
    cofpacket_queue_list list(OFP12_VERSION);
    
    
    for(iter=templist.begin();iter!=templist.end();++iter){
        cofpacket_queue queue (OFP12_VERSION);
        queue.set_port(msg->get_port_no());
        //std::cout << "Port: "<< msg->get_port_no() <<"\n";
        queue.set_queue_id(iter->vlanID);
        //std::cout << "ID: "<< int(iter->vlanID) <<"\n";
        queue.get_queue_prop_list().next()=cofqueue_prop_max_rate(OFP12_VERSION,iter->max_BW);
        //std::cout << "Max BW: "<< iter->max_BW <<"\n";
        queue.get_queue_prop_list().next()=cofqueue_prop_min_rate(OFP12_VERSION,iter->min_sustained_BW);
        //std::cout << "Min BW: "<< iter->min_sustained_BW <<"\n";
        queue.get_queue_prop_list().next() = cofqueue_prop_src_port(OFP12_VERSION,iter->dst_port); /// ojo con los puertos
        list.next()=queue;
        //proxy->send_queue_get_config_reply(proxy->controller,msg->get_xid(),msg->get_port_no(),list);
    }
    
    proxy->send_queue_get_config_reply(proxy->controller,msg->get_xid(),msg->get_port_no(),list);
    delete msg;
}

void      orchestrator::handle_experimenter_message(cofctl* ctl, cofmsg_experimenter* msg){
    if(msg->get_experimenter_id()==0x00FF00FF){
        std::cout << "QoS extension RECEIVED\n";
        //cmemory *messagemem=&(msg->get_body()); 
        //cofmsg_queue_get_config_reply reply (messagemem);
        std::cout<< msg->get_body().c_str() << "\n";

        
    }else{
        std::cout << "unknown extension...DISCARDING\n";
        
    }
    delete msg;
}

/******************************  TESTING FUNCTIONS ************************************/
void      orchestrator::flow_test(cofdpt* dpt){

    cflowentry test(OFP12_VERSION);
    cmacaddr test_mac("03:52:20:23:22:11");
    caddress test_ip(AF_INET,"192.168.5.1");

    test.set_command(OFPFC_ADD);
    test.set_table_id(1);
    test.set_priority(32768);
    test.match.set_eth_src(test_mac);
    test.match.set_eth_dst(test_mac);
    test.match.set_eth_type(0x0800);
    test.match.set_vlan_vid(5);
    test.match.set_vlan_pcp(1);
    test.match.set_ipv4_src(test_ip);
    test.match.set_ipv4_dst(test_ip);
    test.match.set_ip_proto(0x06);
    test.match.set_tcp_src(5555);
    test.match.set_tcp_dst(6666);
    test.match.set_metadata(0x65214);
    
    test.instructions.next()=cofinst_apply_actions(OFP12_VERSION);
    test.instructions.back().actions.next() = cofaction_push_vlan(OFP12_VERSION,C_TAG); 
    test.instructions.back().actions.next() = cofaction_set_field(OFP12_VERSION,coxmatch_ofb_vlan_vid (66));   
    test.instructions.back().actions.next() = cofaction_set_field(OFP12_VERSION,coxmatch_ofb_eth_dst (test_mac));
    test.instructions.back().actions.next() = cofaction_set_field(OFP12_VERSION,coxmatch_ofb_eth_src (test_mac));
    test.instructions.back().actions.next() = cofaction_set_field(OFP12_VERSION,coxmatch_ofb_ipv4_src (test_ip));
    test.instructions.back().actions.next() = cofaction_set_field(OFP12_VERSION,coxmatch_ofb_ipv4_src (test_ip));   
    test.instructions.back().actions.next() = cofaction_output(OFP12_VERSION,1);
    proxy->send_flow_mod_message(dpt,test);
    std::cout<<"testing2\n";
}