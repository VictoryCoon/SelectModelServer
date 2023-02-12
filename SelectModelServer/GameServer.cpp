#include "PreCompliedHeader.h"
#include "GameServer.h"

void GameServer::OnConnected(int sId)
{
}

void GameServer::OnDisconnected(int sId)
{
}

void GameServer::OnReceived(int sId, Packet* packet)
{
	MsgType type;
	packet->Read(&type);

	switch (type)
	{
	case RequestChat:
		ProcessRequestChat(sId, packet);
		break;
	default:
		break;
	}
}

void GameServer::OnSent(int sId)
{
}

void GameServer::ProcessRequestChat(int sId, Packet* packet)
{
	MsgRequestChat msg;
	*packet >> msg.chatLength;
	packet->Read(msg.chatLength, &msg.chatMsg);

	delete packet;

	MsgResponseChat res;
	res.chatMsg = to_string(sId) + " : " + msg.chatMsg;
	res.chatLength = msg.chatMsg.length();
	
	cout << res.chatMsg << endl;

	Packet* sendPacket = new Packet();
	sendPacket->Write(res.type);
	sendPacket->Write(res.chatLength);
	sendPacket->Write(res.chatMsg);
	
	Header header;
	header.magicNumber = MagicNumber;
	header.payloadLength = sendPacket->Count();

	sendPacket->SetHeader(header);

	SendBroadcast(sendPacket);
}