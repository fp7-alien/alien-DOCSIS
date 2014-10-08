/* 
 * File:   QoS.h
 * Author: victor
 *
 * Created on 30 de septiembre de 2014, 13:04
 */

#ifndef QOS_H
#define	QOS_H

#include <stdint.h>
#include <vector>
#include <map>
#include <iostream>
#include <iostream>
#include <iomanip>
#include <cstdlib>


struct QueueList {

    uint16_t        min_sustained_BW;
    uint16_t        max_BW;
    std::string     description;
    uint16_t        vlanID;
    uint32_t        src_port;
    uint32_t        dst_port;
    std::string     direction;
};

class ALHINP;

class QoS {
public:
    QoS(ALHINP* proxye);
    QoS(const QoS& orig);
    virtual ~QoS();
    bool add_new_qos(uint32_t port, uint32_t min_bw, uint32_t max_bw);
    void add_new_qos(uint32_t port);
    void add_new_qos_batch(uint32_t port_id, std::vector<QueueList> queuelist);
    void remove_qos(uint32_t port_id,uint32_t queue_id); //implicaria pasar todos los flujos a default?¿
    std::vector<QueueList> get_queues(uint32_t port_id);
    std::vector<QueueList> get_default_queues();
    void allocate_identifiers (std::vector<QueueList> &queues,uint16_t vid);
    void check_queues(uint32_t port);
    void enable_queues_for_port(uint32_t port_id);
    uint16_t get_default_queue_vlan(uint32_t inport,uint32_t outport);
    bool qos_id_is_valid(uint16_t qos_id, uint32_t inport, uint32_t outport); 
    
private:
    ALHINP* proxy;
    std::map< uint32_t , std::vector<QueueList> > QoSMap;
    
};

#endif	/* QOS_H */

