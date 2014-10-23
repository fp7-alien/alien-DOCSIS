///* 
// * File:   cofmsg_set_queues.h
// * Author: victor
// *
// * Created on 9 de octubre de 2014, 19:22
// */
//
//#ifndef COFMSG_SET_QUEUES_H
//#define	COFMSG_SET_QUEUES_H
//
//#include <rofl/common/openflow/cofqueueprop.h>
//#include <rofl/common/openflow/messages/cofmsg.h>
//using namespace rofl;
//
//class cofmsg_set_queues:
//        public cofmsg
//{
//public:
//    cofmsg_set_queues();
//    cofmsg_set_queues(const cofmsg_set_queues& orig);
//    virtual ~cofmsg_set_queues();
//private:
//
//};
/////////////////////////////////////////////////////
//class cofmsg_queue_get_config_reply :
//	public cofmsg
//{
//private:
//
//	// TODO: cofqueuelist
//	//cmemory				queues;
//	cofpacket_queue_list	pql;
//
//	union {
//		uint8_t*								ofhu_queue_get_config_reply;
//		struct ofp10_queue_get_config_reply*	ofhu10_queue_get_config_reply;
//		struct ofp12_queue_get_config_reply*	ofhu12_queue_get_config_reply;
//		struct ofp13_queue_get_config_reply*	ofhu13_queue_get_config_reply;
//	} ofhu;
//
//#define ofh_queue_get_config_reply   ofhu.ofhu_queue_get_config_reply
//#define ofh10_queue_get_config_reply ofhu.ofhu10_queue_get_config_reply
//#define ofh12_queue_get_config_reply ofhu.ofhu12_queue_get_config_reply
//#define ofh13_queue_get_config_reply ofhu.ofhu13_queue_get_config_reply
//
//public:
//
//
//	/** constructor
//	 *
//	 */
//	cofmsg_set_queues(
//			uint8_t of_version = 0,
//			uint32_t xid = 0,
//			uint32_t port_no = 0,
//			cofpacket_queue_list const &pql = cofpacket_queue_list(OFP12_VERSION));
//#if 0
//			uint8_t *data = (uint8_t*)0,
//			size_t datalen = 0);
//#endif
//
//	/**
//	 *
//	 */
//	cofmsg_set_queues(
//			cofmsg_set_queues const& queue_get_config);
//
//
//	/**
//	 *
//	 */
//	cofmsg_set_queues&
//	operator= (
//			cofmsg_set_queues const& queue_get_config);
//
//
//	/** destructor
//	 *
//	 */
//	virtual
//	~cofmsg_set_queues();
//
//
//	/**
//	 *
//	 */
//	cofmsg_set_queues(cmemory *memarea);
//
//
//	/** reset packet content
//	 *
//	 */
//	virtual void
//	reset();
//
//
//	/**
//	 *
//	 */
//	virtual void
//	resize(size_t len);
//
//
//	/** returns length of packet in packed state
//	 *
//	 */
//	virtual size_t
//	length() const;
//
//
//	/**
//	 *
//	 */
//	virtual void
//	pack(uint8_t *buf = (uint8_t*)0, size_t buflen = 0);
//
//
//	/**
//	 *
//	 */
//	virtual void
//	unpack(uint8_t *buf, size_t buflen);
//
//
//	/** parse packet and validate it
//	 */
//	virtual void
//	validate();
//
//
//public:
//
//
//	/**
//	 *
//	 */
//	uint32_t
//	get_port_no() const;
//
//	/**
//	 *
//	 */
//	void
//	set_port_no(uint32_t port_no);
//
//	/**
//	 *
//	 */
//	cofpacket_queue_list&
//	get_queues();
//
//
//	/**
//	 *
//	 */
//	friend std::ostream&
//	operator<< (std::ostream& os, cofmsg_set_queues const& msg)
//	{
//		os << "cofmsg_queue_get_config_reply[";
//		os << "portno: " << msg.get_port_no() << " ";
//		os << "queues: " << msg.pql << "]";
//		return os;
//	};
//};
////////////////////////////////////////////////////////////////
//
//#endif	/* COFMSG_SET_QUEUES_H */
//
