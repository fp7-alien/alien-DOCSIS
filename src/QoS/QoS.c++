/* 
 * File:   QoS.c++
 * Author: victor
 * 
 * Created on 30 de septiembre de 2014, 13:04
 */

#include <map>
#include <rofl/common/openflow/openflow12.h>
#include "QoS.h"
#include <rofl/common/crofbase.h>
#include "../proxy/ALHINP.h"


QoS::QoS(ALHINP* proxye) {
    proxy=proxye;
}

QoS::QoS(const QoS& orig) {
}

QoS::~QoS() {
}

bool QoS::add_new_qos(uint32_t port, uint32_t min_bw, uint32_t max_bw) {
    //check first if empty
    if (port==0)
        return false;
    //then get the latest Identifier created for that port
    QueueList temp;
    temp.max_BW=(uint16_t) max_bw;
    temp.min_sustained_BW=(uint16_t) max_bw;
    return true;
    
}
void QoS::add_new_qos(uint32_t port){
    
    
}

void QoS::add_new_qos_batch(uint32_t port_id, std::vector<QueueList> queuelist) {
    QoSMap[port_id]= queuelist;
    std::map< uint32_t , std::vector<QueueList> >::iterator qos_iter;
    std::vector<QueueList>::iterator prop_iter;
    qos_iter = QoSMap.find(port_id);
    std::cout <<"For port: "<< port_id <<"\n";
    for(prop_iter= qos_iter->second.begin();prop_iter != qos_iter->second.end();prop_iter++){
        std::cout << "queue added: " <<  std::dec << prop_iter->description <<" min_BW: " << prop_iter->min_sustained_BW << " max_BW:" << prop_iter->max_BW << "\n";
    }
}


std::vector<QueueList> QoS::get_queues(uint32_t port_id) {
    
    std::vector<QueueList> temp;
    std::map< uint32_t , std::vector<QueueList> >::iterator qos_iter;
    qos_iter = QoSMap.find(port_id);
    if(qos_iter == QoSMap.end()){
        std::cout<<"no queues defined for"<< port_id <<"\n";
        return temp;
    }else {
        std::cout<<"Queues found for "<< port_id <<"!\n";
        temp = qos_iter->second;
        return temp;
    }
    
}

void QoS::remove_qos(uint32_t port_id,uint32_t queue_id) {
    std::vector<QueueList>::iterator queue_iter;
//    queue_iter=QoSMap[port_id].begin();
    queue_iter=queue_iter+queue_id-1;
//    QoSMap[port_id].erase(queue_iter);
}

std::vector<QueueList> QoS::get_default_queues() {
    return get_queues(OFPP12_ANY);
}

void QoS::allocate_identifiers(std::vector<QueueList> &queues, uint16_t vid) {
    if(vid!=0){
        int counter = 1;
        std::vector<QueueList>::iterator queue_iter;
        for(queue_iter= queues.begin();queue_iter != queues.end();queue_iter++){
             if(queue_iter->description=="openflow"){
                 queue_iter->vlanID = 0;

            }else{
                queue_iter->vlanID = vid*10+counter;
                counter++;
        }

    }
    QoSMap [(uint32_t)(vid*10)] = queues;
    check_queues((uint32_t)(vid*10));
    }

    return;
}
void QoS::check_queues(uint32_t port){
    std::map< uint32_t , std::vector<QueueList> >::iterator qos_iter;
    std::vector<QueueList>::iterator prop_iter;
    qos_iter = QoSMap.find(port);
    std::cout <<"For port: "<< port << "\n";
    for(prop_iter= qos_iter->second.begin();prop_iter != qos_iter->second.end();prop_iter++){
        std::cout << " queue added: " <<  std::dec << prop_iter->description <<" min_BW: " << prop_iter->min_sustained_BW << " max_BW:" << prop_iter->max_BW << " vlanID:" << prop_iter->vlanID << " dst_port:" << prop_iter->dst_port <<"\n";
    }
}

void QoS::enable_queues_for_port(uint32_t port_id){
    uint32_t vlan;
    vlan = (uint32_t) port_id/10;
    uint32_t cmid;
    cmid = vlan*10;
    //std::cout << cmid << "\n";
    std::cout<<"enabling queues for port "<< port_id <<"\n";
    std::vector<QueueList> templist;
    std::vector<QueueList> templist_oposite;
    std::vector<QueueList>::iterator queue_iter;
    for (queue_iter=QoSMap [cmid].begin();queue_iter!=QoSMap [cmid].end();queue_iter++){
        if(queue_iter->description!="openflow"){
            QueueList queue1;
            queue1.description = (*queue_iter).description;
            queue1.direction = "upstream";
            queue1.dst_port = (uint32_t) proxy->portconfig.data_port;
            queue1.max_BW = (*queue_iter).max_BW;
            queue1.min_sustained_BW = (*queue_iter).min_sustained_BW;
            queue1.src_port = port_id;
            queue1.vlanID=(*queue_iter).vlanID;            
            templist.push_back(queue1);
            
            if((*queue_iter).direction=="bidirectional"){
                QueueList queue;
                queue.description = (*queue_iter).description;
                queue.direction = "downstream";
                queue.dst_port = port_id;
                queue.max_BW = (*queue_iter).max_BW;
                queue.min_sustained_BW = (*queue_iter).min_sustained_BW;
                queue.src_port = proxy->virtualizer->get_virtual_port_id(proxy->config.AGS_dpid,proxy->portconfig.data_port);
                queue.vlanID=(*queue_iter).vlanID;
                //templist_oposite.push_back(queue);
                QoSMap [proxy->virtualizer->get_virtual_port_id(proxy->config.AGS_dpid,proxy->portconfig.data_port)].push_back(queue);
            }
        }
    }
    QoSMap [ port_id] = templist;
    //add, not copy
   // QoSMap [proxy->virtualizer->get_virtual_port_id(proxy->config.AGS_dpid,proxy->portconfig.data_port)] = templist_oposite;
    check_queues(port_id);
    check_queues(proxy->virtualizer->get_virtual_port_id(proxy->config.AGS_dpid,proxy->portconfig.data_port));
}

uint16_t QoS::get_default_queue_vlan(uint32_t inport, uint32_t outport) {
    std::vector<QueueList>::iterator queue_iter;
    for(queue_iter = QoSMap[inport].begin(); queue_iter!=QoSMap[inport].end();queue_iter++){
        if(queue_iter->dst_port==outport && queue_iter->description=="default"){
            std::cout<<"applying QosVLAN = "<< queue_iter->vlanID<<"\n";
            return queue_iter->vlanID;
            
        }
            
    }
}

bool QoS::qos_id_is_valid(uint16_t qos_id, uint32_t inport, uint32_t outport) {
    std::vector<QueueList>::iterator queue_iter;
    for(queue_iter = QoSMap[inport].begin(); queue_iter!=QoSMap[inport].end();queue_iter++){
        if(queue_iter->dst_port==outport && queue_iter->vlanID==qos_id){
            std::cout<<"applying QosVLAN = "<< queue_iter->vlanID<<"\n";
            return true;
        }
         
    }
    return false;
}

