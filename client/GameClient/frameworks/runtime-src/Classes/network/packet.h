/*******************************************************************
**		
** author : xzben 2014/12/09
** �洢�ͻ��˽��յ������ݰ�
*******************************************************************/

#ifndef __2014_12_09_PACKET_H__
#define __2014_12_09_PACKET_H__

#include <vector>
#include "Mutex.h"

class PacketBuffer
{
public:
	PacketBuffer(){};
	~PacketBuffer(){};

private:
};

class PacketQueue
{
public:
	PacketQueue(){};
	~PacketQueue(){};
	

private:
	std::vector<PacketBuffer*>	m_queue;
	Mutex						m_lock;
};
#endif//__2014_12_09_PACKET_H__