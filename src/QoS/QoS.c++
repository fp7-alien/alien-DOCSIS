/* 
 * File:   QoS.c++
 * Author: victor
 * 
 * Created on 30 de septiembre de 2014, 13:04
 */

#include <map>
#include <rofl/common/openflow/openflow12.h>
#include "QoS.h"


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

void QoS::add_new_qos_batch(uint32_t port_id, std::vector<QueueList> queuelist) {
    QoSMap[port_id]= queuelist;
    std::map< uint32_t , std::vector<QueueList> >::iterator qos_iter;
    std::vector<QueueList>::iterator prop_iter;
    qos_iter = QoSMap.find(port_id);
    for(prop_iter= qos_iter->second.begin();prop_iter != qos_iter->second.end();prop_iter++){
        std::cout << "Queue added: " <<  std::dec << prop_iter->description <<" min_BW: " << prop_iter->min_sustained_BW << " max_BW:" << prop_iter->max_BW << "\n";
    }
}


std::vector<QueueList> QoS::get_queues(uint32_t port_id) {
    
    std::vector<QueueList> temp;
    std::map< uint32_t , std::vector<QueueList> >::iterator qos_iter;
    qos_iter = QoSMap.find(port_id);
    if(qos_iter ==QoSMap.end())
        return temp;
    else {
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
    int counter = 2;
    std::vector<QueueList>::iterator queue_iter;
   for(queue_iter= queues.begin();queue_iter != queues.end();queue_iter++){
       if(queue_iter->description=="openflow"){
            queue_iter->vlanID = 0;

       }else{
            queue_iter->vlanID = vid*10+counter;
            counter++;
       }

   }
}
