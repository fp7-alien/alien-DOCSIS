/* 
 * File:   cof_queue_prop_src_port.cpp
 * Author: victor
 * 
 * Created on 9 de octubre de 2014, 15:47
 */

#include "cof_queue_prop_src_port.h"

cofqueue_prop_src_port::~cofqueue_prop_src_port() {
}

cofqueue_prop_src_port::cofqueue_prop_src_port(
	uint8_t of_version,
	uint32_t src_port) :
		cofqueue_prop(of_version)
{
	switch (of_version) {
	case OFP10_VERSION: {
		resize(sizeof(struct ofp10_queue_prop_src_port));
	} break;
	case OFP12_VERSION: {
		resize(sizeof(struct ofp12_queue_prop_src_port));
	} break;
	case OFP13_VERSION: {
		throw eNotImplemented();
	} break;
	default: {
		throw eBadVersion();
	}
	}
	set_property(OFPQT_SRC_PORT);
	set_length(memlen());
	set_src_port(src_port);
}


cofqueue_prop_src_port::cofqueue_prop_src_port(
	cofqueue_prop_src_port const& qp) :
		cofqueue_prop(qp.of_version)
{
	*this = qp;
}



cofqueue_prop_src_port&
cofqueue_prop_src_port::operator= (
		cofqueue_prop const& qp)
{
	if (this == &qp)
		return *this;

	if (OFPQT_SRC_PORT != qp.get_property())
		throw eInval();

	unpack(qp.somem(), qp.memlen());

	return *this;
}



uint8_t*
cofqueue_prop_src_port::resize(size_t len)
{
	ofq_src_port = cofqueue_prop::resize(len);
	return somem();
}



void
cofqueue_prop_src_port::pack(uint8_t *buf, size_t buflen) const
{
	switch (of_version) {
	case OFP10_VERSION: {
		if (buflen < sizeof(struct ofp10_queue_prop_src_port))
			throw eInval();
	} break;
	case OFP12_VERSION: {
		if (buflen < sizeof(struct ofp12_queue_prop_src_port))
			throw eInval();
	} break;
	case OFP13_VERSION: {
		throw eNotImplemented();
	} break;
	default: {
		throw eBadVersion();
	}
	}

	cofqueue_prop::pack(buf, buflen);
}



void
cofqueue_prop_src_port::unpack(uint8_t *buf, size_t buflen)
{
	switch (of_version) {
	case OFP10_VERSION: {
		if (buflen < sizeof(struct ofp10_queue_prop_src_port))
			throw eInval();
	} break;
	case OFP12_VERSION: {
		if (buflen < sizeof(struct ofp12_queue_prop_src_port))
			throw eInval();
	} break;
	case OFP13_VERSION: {
		throw eNotImplemented();
	} break;
	default: {
		throw eBadVersion();
	}
	}

	cofqueue_prop::unpack(buf, buflen);
	ofq_src_port = somem();
}



uint32_t
cofqueue_prop_src_port::get_src_port() const
{
	switch (of_version) {
	case OFP10_VERSION: {
		return be16toh(ofq10_src_port->src_port);
	} break;
	case OFP12_VERSION: {
		return be32toh(ofq12_src_port->src_port);
	} break;
	case OFP13_VERSION: {
		throw eNotImplemented();
	} break;
	default: {
		throw eBadVersion();
	}
	}
	return 0;
}



void
cofqueue_prop_src_port::set_src_port(uint32_t src_port)
{
	switch (of_version) {
	case OFP10_VERSION: {
		ofq10_src_port->src_port = htobe16(src_port);
	} break;
	case OFP12_VERSION: {
		ofq12_src_port->src_port = htobe32(src_port);
	} break;
	case OFP13_VERSION: {
		throw eNotImplemented();
	} break;
	default: {
		throw eBadVersion();
	}
	}
}