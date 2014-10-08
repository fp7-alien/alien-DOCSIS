/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* 
 * File:   ALHINP.c++
 * Author: Victor Fuentes
 * University of the Basque country / Universidad del Pais Vasco (UPV/EHU)
 * I2T Research Group
 */

#include "ALHINP.h"
#include <rofl/common/crofbase.h>
#include <rofl/common/openflow/openflow.h>
#include <rofl/common/openflow/coxmatch.h>
#include <list>





using namespace rofl;
using namespace std;
using namespace libconfig;

ALHINP::ALHINP(char* configfile): crofbase::crofbase((uint32_t)(1 << OFP10_VERSION) | (1 << OFP12_VERSION)){
    //listen for AGS
    
    if(parse_config_file(configfile)!=0){
        exit(EXIT_FAILURE);
    }
   
    
    discover= new discovery(this);
    manager= new orchestrator(this);
    flowcache= new Flowcache(this);
    virtualizer = new translator (this);
    qosmap = new QoS (this);
    
    char qos_file [4] = "QOS";
    if(parse_qos_file(qos_file)!=0){
        exit(EXIT_FAILURE);
    }
    
    //Listen for AGS
    std::cout << "Listening on "<< config.listening_IP_ags.c_str() <<":"<< std::dec <<(uint16_t) config.listening_ags_port<<" for AGS\n";
    try{
    rpc_listen_for_dpts(caddress(AF_INET, config.listening_IP_ags.c_str(),config.listening_ags_port));
    }catch(...){
        std::cout<<"Unable to Listen for AGS\n";
        exit(EXIT_FAILURE);
    }
    //listen for OUI
    std::cout << "Listening on "<< config.listening_IP_oui.c_str()<<":"<<config.listening_ags_port<<" for OUIs\n";
    try{
    rpc_listen_for_dpts(caddress(AF_INET, config.listening_IP_oui.c_str(), config.listening_oui_port));
    }catch(...){
        std::cout<<"Unable to Listen for OUIS\n";
        exit(EXIT_FAILURE);
    }
    //test();
}
ALHINP::ALHINP(const ALHINP& orig) {
}
ALHINP::~ALHINP() {
    
}

int ALHINP::parse_config_file(char* file){
    Config cfg;
    try{
        cfg.readFile(file);
    }catch(const FileIOException &fioex){
        std::cerr << "I/O error while reading file." << std::endl;
          return(EXIT_FAILURE);
    }catch(const ParseException &pex){
            std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
            << " - " << pex.getError() << std::endl;
        return(EXIT_FAILURE);
    }
    // Get the store name.
    string tempdpid;
    string desc;
    try{
        string name = cfg.lookup("name");
        //cout << "Store name: " << name << endl << endl;
        string temp1 = cfg.lookup("desc");
        string temp2 = cfg.lookup("dpid");
        tempdpid = temp2;
        desc = temp1;
    }catch(const SettingNotFoundException &nfex){
        std::cerr << "Uops! something went wrong D :S" << endl;
        return(EXIT_FAILURE);
    }
    
    std::stringstream ss;
    ss << std::hex << tempdpid.c_str();
    ss >> config.ALHINP_dpid;
    std::cout << "ALHINP DPID: "<< std::hex << config.ALHINP_dpid << "\n";

    //GET ALHINP config
    Setting &root = cfg.getRoot();
    uint32_t controllerport;
    uint32_t ouiport;
    uint32_t agsport;
    uint32_t vlanstart;
    try{
        const Setting &AHLPconfig = root["ALHINP-config"];
        if(!(AHLPconfig.lookupValue("CONTROLLER_IP",config.controller_ip) 
                &&AHLPconfig.lookupValue("CONTROLLER_OF_VERSION",config.of_version)
                &&AHLPconfig.lookupValue("CONTROLLER_PORT",controllerport)
                &&AHLPconfig.lookupValue("LISTENING_IP_AGS",config.listening_IP_ags)
                &&AHLPconfig.lookupValue("LISTENING_PORT_AGS",agsport)
                &&AHLPconfig.lookupValue("LISTENING_IP_OUIS",config.listening_IP_oui)
                &&AHLPconfig.lookupValue("VLANstart",vlanstart)
                &&AHLPconfig.lookupValue("CMTS_IP",config.CMTS_ip)
                &&AHLPconfig.lookupValue("DPS_IP",config.DPS_ip)
                &&AHLPconfig.lookupValue("OUI_MAC",config.oui_mac)
                &&AHLPconfig.lookupValue("CM_MAC",config.cm_mac)
                &&AHLPconfig.lookupValue("LISTENING_PORT_OUIS",ouiport) ));
    }catch(...){
        std::cerr << "Uops! something went wrong (ALHINP-config) :S" << endl;
        return(EXIT_FAILURE);
    }
    //std::cout << std::dec <<agsport<<"\n";
    //std::cout << (uint16_t)ouiport<<"\n";
    config.controller_port=(uint16_t)controllerport;
    config.listening_oui_port=(uint16_t)ouiport;
    config.listening_ags_port=(uint16_t)agsport;
    config.VLANstart=(uint16_t)vlanstart;
    
    uint32_t cmtsport;
    uint32_t dataport;
    uint32_t dpsport;
    uint32_t proxyport;
    uint32_t ouinetport;    
    uint32_t ouiuserport;
    try{
        const Setting &AHLPortConfig = root["Port-config"];
        if(!     (AHLPortConfig.lookupValue("CMTS_PORT",cmtsport) 
                &&AHLPortConfig.lookupValue("DATA_PORT",dataport)
                &&AHLPortConfig.lookupValue("DPS_PORT",dpsport)
                &&AHLPortConfig.lookupValue("PROXY_PORT",proxyport)                
                &&AHLPortConfig.lookupValue("OUI_NETPORT",ouinetport)
                &&AHLPortConfig.lookupValue("OUI_USERPORT",ouiuserport) ));
    }catch(...){
        std::cerr << "Uops! something went wrong (PORT-CONFIG) :S" << endl;
        return(EXIT_FAILURE);
    }
    portconfig.cmts_port=(uint16_t)cmtsport;
    portconfig.data_port=(uint16_t)dataport;
    portconfig.dps_port=(uint16_t)dpsport;
    portconfig.oui_netport=(uint16_t)ouinetport;
    portconfig.oui_userport=(uint16_t)ouiuserport;
    portconfig.proxy_port=(uint16_t)proxyport;
    config.queue_counter=2;
    
    string AGS_dpidtemp;
    
    try{
        const Setting &AGSconfig = root["AGS-config"];
        AGSconfig.lookupValue("AGSDPID",AGS_dpidtemp);

    }catch(...){
        std::cerr << "Uops! something went wrong (AGS-CONFIG):S" << endl;
        return(EXIT_FAILURE);
    }
    std::stringstream ss2;
    ss2 << std::hex << AGS_dpidtemp.c_str();
    ss2 >> config.AGS_dpid;
    //std::cout<<"AGS_DPID: "<< config.AGS_dpid<<"\n";
    
    return (EXIT_SUCCESS);
}

int ALHINP::parse_qos_file(char* file) {
    std::cout << "Trying to parse QoS file\n";
    Config cfg;
    try{
        cfg.readFile(file);
    }catch(const FileIOException &fioex){
        std::cerr << "I/O error while reading QOS file." << std::endl;
          return(EXIT_FAILURE);
    }catch(const ParseException &pex){
            std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
            << " - " << pex.getError() << std::endl;
        return(EXIT_FAILURE);
    }
    
    //GET ALHINP config
    Setting &root = cfg.getRoot();
    try
    {
      const Setting &cablemodemlist = root["CMLIST"];
      int count = cablemodemlist.getLength();

      for(int i = 0; i < count; ++i)
      {
        const Setting &cablemodem = cablemodemlist[i];

        // Only output the record if all of the expected fields are present.
        string identifier;
        int max_rate, min_rate;

        if(!(cablemodem.lookupValue("queueID", identifier)
             && cablemodem.lookupValue("max_rate", max_rate)
             && cablemodem.lookupValue("min_rate", min_rate)))
          continue;
        //then add 
        //TB implemented
      }
      cout << endl;
    }
    catch(const SettingNotFoundException &nfex)
    {
      // Ignore.
    }
    
    string identifier,direction;
    uint32_t max_rate, min_rate, source_port, dest_port;
    QueueList queue;
    std::vector<QueueList> Queues;
    int queuecounter =2;
    bool openflow_present = false;
    bool default_present = false;
    try
    {
        const Setting &cablemodemlist = root["DEFAULT_QUEUELIST"];
        int count = cablemodemlist.getLength();
        

        for(int i = 0; i < count; ++i)
        {
            const Setting &ALLcablemodem = cablemodemlist[i];

            // Only output the record if all of the expected fields are present.

                    
            if(!(ALLcablemodem.lookupValue("queueID", identifier)
                 && ALLcablemodem.lookupValue("max_rate", max_rate)
                 && ALLcablemodem.lookupValue("destination_port", dest_port)
                 && ALLcablemodem.lookupValue("source_port", source_port)
                 && ALLcablemodem.lookupValue("direction", direction)
                 && ALLcablemodem.lookupValue("min_rate", min_rate)))
                continue;
                
                queue.max_BW            = max_rate;
                queue.min_sustained_BW  = min_rate;
                queue.description       = identifier;
                queue.src_port          = source_port;
                queue.dst_port          = dest_port;
                queue.direction         = direction;
                if(identifier=="openflow"){
                    queue.vlanID            = 0     ;
                    queue.dst_port          = 0     ;
                    queue.direction         = "none";
                    Queues.push_back(queue); 
                    openflow_present = true;
                    
                }else{
                    if(identifier=="default")
                        default_present = true;
                    Queues.push_back(queue);            
                }
        }

    }
    catch(const SettingNotFoundException &nfex)
    {
      // Ignore.
    }
    if(openflow_present==false || default_present == false)
        return(EXIT_FAILURE);
    else{
        qosmap->add_new_qos_batch(0xFFFFFFFF,Queues);
        std::cout << "QoS file successfully parsed\n";
        return 0;
    }

}


void ALHINP::handle_dpath_open(cofdpt* dpt){
    if(discover->is_aggregator(dpt->get_dpid())){
        manager->AGS_connected(dpt);
    }else{
        manager->OUI_connected(dpt);
        //This not should be like this, the controller should verify that ports for Flow Mods exist
        std::cout<<"Trying to connect controller @ "<< config.controller_ip.c_str() << "\n";
        if(config.of_version=="1.0"){
            rpc_connect_to_ctl(OFP10_VERSION,5,caddress(AF_INET,config.controller_ip.c_str(),config.controller_port));
        }else{
            rpc_connect_to_ctl(OFP12_VERSION,5,caddress(AF_INET,config.controller_ip.c_str(),config.controller_port));
        }
    }

}
void ALHINP::handle_dpath_close(cofdpt* dpt){
    if(discover->is_aggregator(dpt->get_dpid())){
        //AGS connected
        manager->AGS_disconnected(dpt);
        
        
    }else{
        //An OUI is connected
        manager->OUI_disconnected(dpt);
    }    
}

void ALHINP::handle_ctrl_open(cofctl *ctl){
    controller=ctl;
    std::cout<<"Controller connected using OF 0x0"<< int(ctl->get_version())<<"\n";
    fflush(stdout);
}
void ALHINP::handle_ctrl_close(cofctl *ctl){
    controller=0;
    std::cout<<"[WARNING]: Controller disconnected\n";
    rpc_connect_to_ctl(OFP10_VERSION,5,caddress(AF_INET,config.controller_ip.c_str(),config.controller_port));
    
}

void ALHINP::handle_timeout(int opaque){
    manager->handle_timeout(opaque);
}

void ALHINP::handle_packet_in(cofdpt *dpt, cofmsg_packet_in *msg){
    
    manager->dispath_PACKET_IN(dpt,msg);
}

void ALHINP::handle_port_status(cofdpt* dpt, cofmsg_port_status* msg){
    manager->handle_port_status(dpt,msg);
}
void ALHINP::handle_port_mod (cofctl *ctl, cofmsg_port_mod *msg){
    manager->handle_port_mod (ctl, msg);
} 

void ALHINP::handle_get_config_request(cofctl* ctl, cofmsg_get_config_request* msg){
    manager->handle_get_config_request (ctl,msg);
}
void ALHINP::handle_get_config_reply (cofdpt *dpt, cofmsg_get_config_reply *msg){
        //send_set_config_message(dpt,config_flags,miss_send_len);
        delete msg;
        return;
}
void ALHINP::handle_set_config (cofctl *ctl, cofmsg_set_config *msg){
    manager->handle_set_config (ctl, msg);
}

void ALHINP::handle_features_request (cofctl *ctl, cofmsg_features_request *msg){
    manager->handle_features_request (ctl,msg);
}
void ALHINP::handle_features_reply(cofdpt *dpt, cofmsg_features_reply *msg){
    manager->handle_features_reply(dpt,msg);
}

void ALHINP::handle_flow_mod (cofctl *ctl, cofmsg_flow_mod *msg){
    switch (msg->get_command()) {
        case OFPFC_ADD: {
                manager->flow_mod_add(ctl, msg);
                } break;

        case OFPFC_MODIFY: {
                manager->flow_mod_modify(ctl, msg, false);
                } break;

        case OFPFC_MODIFY_STRICT: {
                manager->flow_mod_modify(ctl, msg, true);
                } break;

        case OFPFC_DELETE: {
                manager->flow_mod_delete(ctl, msg, true);
                } break;

        case OFPFC_DELETE_STRICT: {
                manager->flow_mod_delete(ctl, msg, true);
                } break;

        default:
                throw eFlowModBadCommand();
    }
    //delete msg;
    return;
}  
void ALHINP::handle_flow_removed (cofdpt *dpt, cofmsg_flow_removed *msg){
    //firstly exists??
    manager->handle_flow_removed(dpt,msg);
   
    
}
void ALHINP::handle_packet_out (cofctl *ctl, cofmsg_packet_out *msg){
    manager->handle_packet_out(ctl,msg);
}

void ALHINP::handle_barrier_request(cofctl *ctl, cofmsg_barrier_request *msg){
    
    send_barrier_reply(ctl,msg->get_xid());
}

void ALHINP::handle_flow_stats_request (cofctl *ctl, cofmsg_flow_stats_request *msg){
    manager->handle_flow_stats_request(ctl,msg);
    
}
void ALHINP::handle_flow_stats_reply (cofdpt *dpt, cofmsg_flow_stats_reply *msg){
    std:cout<<"stats_received\n";
    delete msg;
    return;
    //manager->handle_flow_stats_reply (dpt, msg);
}

void ALHINP::handle_desc_stats_request (cofctl *ctl, cofmsg_desc_stats_request *msg){
    
    cofdesc_stats_reply stats(
                    OFP10_VERSION,
                    "mfr desc",
                    "DOCSIS ALHINP",
                    "alpha 0.1",
                    "1234567890",
                    "DOCSIS ALHINP");
    send_desc_stats_reply(controller,msg->get_xid(),stats,false);
    //cofmsg_desc_stats_request request;
    delete msg;
    return;
}

void ALHINP::handle_table_stats_request (cofctl *ctl, cofmsg_table_stats_request *msg){
    manager->handle_table_stats_request(ctl,msg);
    
}

void ALHINP::handle_port_stats_request (cofctl *ctl, cofmsg_port_stats_request *msg){
    manager->handle_port_stats_request (ctl, msg);
}
void ALHINP::handle_port_stats_reply   (cofdpt *dpt, cofmsg_port_stats_reply *msg){
    manager->handle_port_stats_reply (dpt, msg);
}
void ALHINP::handle_stats_reply (cofdpt *dpt, cofmsg_stats_reply *msg){
    std:cout<<"stats_received____\n";
    delete msg;
    return;
}
void ALHINP::test(){
    
}
void ALHINP::handle_queue_get_config_request (cofctl *ctl, cofmsg_queue_get_config_request *msg){
   manager->handle_queue_get_config_request (ctl, msg);
}