/* 
 * File:   QoS.c++
 * Author: victor
 * 
 * Created on 30 de septiembre de 2014, 13:04
 */

#include <map>
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
    
    QueueProperties temp;
    temp.max_BW=(uint16_t) max_bw;
    temp.min_sustained_BW=(uint16_t) max_bw;
    QoSMap[port].back() = temp;

    add_new_qos_batch(port,QoSMap[port]);
    
}

void QoS::add_new_qos_batch(uint32_t port_id, std::vector<QueueProperties>) {
    
    //proxy->docsis.push_config_file()
}


std::vector<QueueProperties> QoS::get_queues(uint32_t port_id) {
    std::vector<QueueProperties> temp;
    temp = QoSMap[port_id];
    return temp;
}

void QoS::remove_qos(uint32_t port_id,uint32_t queue_id) {
    std::vector<QueueProperties>::iterator queue_iter;
    queue_iter=QoSMap[port_id].begin();
    queue_iter=queue_iter+queue_id-1;
    QoSMap[port_id].erase(queue_iter);
}
