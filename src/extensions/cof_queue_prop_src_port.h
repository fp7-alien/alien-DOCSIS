/* 
 * File:   cof_queue_prop_src_port.h
 * Author: victor
 *
 * Created on 9 de octubre de 2014, 15:47
 */

#ifndef COF_QUEUE_PROP_SRC_PORT_H
#define	COF_QUEUE_PROP_SRC_PORT_H 1

#include <ostream>
#include <endian.h>
#ifndef htobe32
	#include "../endian_conversion.h"
#endif

#include <rofl/common/openflow/cofqueueprop.h>

#define OFPQT_SRC_PORT 5


using namespace rofl;

struct ofp12_queue_prop_src_port{
    struct ofp12_queue_prop_header prop_header; /* prop: OFPQT_MIN, len: 16. */
    uint32_t src_port;        /* Qos can be applied from certain inputs */
    uint8_t pad[4];          /* 64-bit alignment */
};
OFP_ASSERT(sizeof(struct ofp12_queue_prop_src_port) == 16);

struct ofp10_queue_prop_src_port{
    struct ofp10_queue_prop_header prop_header; /* prop: OFPQT_MIN, len: 16. */
    uint16_t src_port;        /* Qos can be applied from certain inputs */
    uint8_t pad[6];          /* 64-bit alignment */
};
OFP_ASSERT(sizeof(struct ofp12_queue_prop_src_port) == 16);


class cofqueue_prop_src_port :
		public cofqueue_prop {
private:

	union {
		uint8_t				*ofqu_src_port;
		struct ofp10_queue_prop_src_port	*ofqu10_src_port;
		struct ofp12_queue_prop_src_port	*ofqu12_src_port;
	} ofq_ofqu;

#define ofq_src_port ofq_ofqu.ofqu_src_port
#define ofq10_src_port ofq_ofqu.ofqu10_src_port
#define ofq12_src_port ofq_ofqu.ofqu12_src_port
//#define ofqu13_min_rate ofq_ofqu.ofqu13_min_rate

public:

	/**
	 *
	 */
	cofqueue_prop_src_port(
			uint8_t of_version, uint32_t src_port = 0);


	/**
	 *
	 */
	virtual
	~cofqueue_prop_src_port();


	/**
	 *
	 */
	cofqueue_prop_src_port(
			cofqueue_prop_src_port const& qp);



	/**
	 *
	 */
	cofqueue_prop_src_port&
	operator= (
			cofqueue_prop const& qp);


public:


	/**
	 *
	 */
	virtual void
	pack(uint8_t *buf, size_t buflen) const;


	/**
	 *
	 */
	virtual void
	unpack(uint8_t *buf, size_t buflen);


public:


	/**
	 *
	 */
	uint32_t
	get_src_port() const;


	/**
	 *
	 */
	void
	set_src_port(uint32_t src_port);


private:


	/**
	 *
	 */
	virtual uint8_t*
	resize(size_t len);
};


#endif /* COF_QUEUE_PROP_SRC_PORT_H */
