/* 
 * File:   cofpacket_queue_extended.h
 * Author: victor
 *
 * Created on 1 de octubre de 2014, 17:17
 */

#ifndef COFPACKET_QUEUE_EXTENDED_H
#define	COFPACKET_QUEUE_EXTENDED_H

#include <rofl/common/openflow/cofpacketqueue.h>
//#include <rofl/common/openflow/cofpacketqueuelist.h>

class cofpacket_queue_extended :
            public rofl::cofpacket_queue
{
public:
    cofpacket_queue_extended();
    cofpacket_queue_extended(const cofpacket_queue_extended& orig);
    virtual ~cofpacket_queue_extended();
    cofpacket_queue_extended::set_queue_list(rofl::cofpacket_queue_list list);
private:

};

#endif	/* COFPACKET_QUEUE_EXTENDED_H */

