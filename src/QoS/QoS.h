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


struct QueueProperties {
    uint16_t    vlan_id;
    uint16_t    min_sustained_BW;
    uint16_t    max_BW;
};

class ALHINP;
class QoS {
public:
    QoS(ALHINP* proxye);
    QoS(const QoS& orig);
    virtual ~QoS();
    bool add_new_qos(uint32_t port, uint32_t min_bw, uint32_t max_bw);
    void add_new_qos_batch(uint32_t port_id, std::vector<QueueProperties> properties);
    void remove_qos(uint32_t port_id,uint32_t queue_id); //implicaria pasar todos los flujos a default?¿
    std::vector<QueueProperties> get_queues(uint32_t port_id);
    
private:
    ALHINP *proxy;
    std::map< uint32_t , std::vector<QueueProperties> > QoSMap;
    
};

#endif	/* QOS_H */

