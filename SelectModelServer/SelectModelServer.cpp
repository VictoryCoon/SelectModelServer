#include "PreCompliedHeader.h"
#include "SelectModelServer.h"

using namespace std;

int uniqueNum = 0;

ErrCommon ec;

SelectModelServer::SelectModelServer()
{
}

SelectModelServer::~SelectModelServer()
{
}

void SelectModelServer::Initialize(string ipAddress, unsigned short port)
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		ec.ErrorQuit("WSAStartup()");
	}

	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(listenSocket == INVALID_SOCKET)
	{
		ec.ErrorQuit("ListenSocket()");
	}

	int returnValue;
	unsigned long on = 1;
	sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = ToNetworkLong(ipAddress);
	serverAddr.sin_port = htons(port);
	returnValue = bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
	if(returnValue == SOCKET_ERROR)
	{
		ec.ErrorQuit("Bind()");
	}

	returnValue = listen(listenSocket, SOMAXCONN);
	if (returnValue == SOCKET_ERROR)
	{
		ec.ErrorQuit("Listen()");
	}

	returnValue = ioctlsocket(listenSocket, FIONBIO, &on);
	if (returnValue == SOCKET_ERROR)
	{
		ec.ErrorDisplay("IoctlSocket()");
	}
}

void SelectModelServer::Run()
{
	int returnValue;
	int addrLen;
	while (!isShutdown)
	{
		FD_ZERO(&rSet);
		FD_ZERO(&wSet);
		FD_SET(listenSocket, &rSet);

		auto iter = sessions.begin();
		for (; iter != sessions.end(); ++iter)
		{
			Session* session = iter->second;
			FD_SET(session->socket, &rSet);
			if (session->sendFlag)
			{
				session->sendFlag = false;
				FD_SET(session->socket, &wSet);
			}
		}

		returnValue = select(0, &rSet, &wSet, NULL, NULL);
		if (returnValue == SOCKET_ERROR)
		{
			ec.ErrorDisplay("Select()");
		}

		if (FD_ISSET(listenSocket, &rSet))
		{
			SOCKET socket;
			sockaddr_in address;
			addrLen = sizeof(address);
			socket = accept(listenSocket, (sockaddr*)&address, &addrLen);
			if (socket == INVALID_SOCKET)
			{
				ec.ErrorDisplay("Accept()");
				break;
			}
			char addr[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &address.sin_addr, addr, INET_ADDRSTRLEN);
			printf("[System] Client is connected : [IP = %s][PORT = %d]\n", addr, ntohs(address.sin_port));
			int sId = AllocateSessionId();
			CreateSession(sId, socket, address);
			OnConnected(sId);
		}

		iter = sessions.begin();
		for (; iter != sessions.end(); ++iter)
		{
			Session* session = iter->second;
			if (!ProcessRecv(session->id))
			{
				DeleteSession(session->id);
			}

			if (!ProcessSend(session->id))
			{
				DeleteSession(session->id);
			}
		}
		ProcessDeleteSessionAll();
	}
}

void SelectModelServer::Release()
{
	closesocket(listenSocket);
	WSACleanup();
}

void SelectModelServer::Shutdown()
{
	isShutdown = true;
}

unsigned long SelectModelServer::ToNetworkLong(string ipAddress)
{
	unsigned long networkAddress;
	inet_pton(AF_INET, ipAddress.c_str(), &networkAddress);
	return networkAddress;
}

void SelectModelServer::ToPeer(char* dst, const void* pAddress)
{
	inet_ntop(AF_INET, pAddress, dst, sizeof(dst));
}

void SelectModelServer::CreateSession(int sId, SOCKET socket, sockaddr_in address)
{
	Session* session = new Session(sId, socket, address);
	sessions[sId] = session;
}

void SelectModelServer::DeleteSession(int sId)
{
	removeSessions.push_back(sId);
}

bool SelectModelServer::ProcessRecv(int sId)
{
	auto find = sessions.find(sId);
	if (find == sessions.end())
	{
		return false;
	}

	Session* session = find->second;
	if (FD_ISSET(session->socket, &rSet))
	{
		char* recvBuffer = session->recvBuffer.GetWriteBuffer();
		int length = session->recvBuffer.GetWritableSizeAtOnce();
		int bytesTransferred = recv(session->socket, recvBuffer, length, 0);
		if (bytesTransferred == SOCKET_ERROR)
		{
			ec.ErrorDisplay("Recv()");
			return false;
		}
		else if (bytesTransferred == 0)
		{
			printf("[System] Client is disconnected : [IP = %s][PORT = %d]\n", session->ipAddress, session->port);
			return false;
		}
		session->recvBuffer.MoveWPos(bytesTransferred);

		ProcessPacket(session);
	}
	return true;
}

bool SelectModelServer::ProcessSend(int sId)
{
	auto find = sessions.find(sId);
	if (find == sessions.end())
	{
		return false;
	}

	Session* session = find->second;

	if (FD_ISSET(session->socket, &wSet))
	{
		char* sendBuffer = session->sendBuffer.GetReadBuffer();
		int length = session->sendBuffer.GetReadableSizeAtOnce();
		int bytesTransferred = send(session->socket, sendBuffer, length, 0);
		if (bytesTransferred == SOCKET_ERROR)
		{
			ec.ErrorDisplay("Send()");
			return false;
		}

		session->sendBuffer.MoveRPos(bytesTransferred);
		OnSent(session->id);
	}

	return true;
}

void SelectModelServer::ProcessPacket(Session* session)
{
	Header header;
	int headerSize = sizeof(header);

	while (1)
	{
		int bufferSize = session->recvBuffer.GetReadableSize();
		if (bufferSize < headerSize)
		{
			break;
		}

		session->recvBuffer.Peek((char*)&header, headerSize);
		if (header.magicNumber != MagicNumber)
		{
			DeleteSession(session->id);
			break;
		}

		if (bufferSize < headerSize + header.payloadLength)
		{
			break;
		}

		Packet* packet = new Packet();
		session->recvBuffer.MoveRPos(headerSize);
		session->recvBuffer.Read(packet->GetBuffer(), header.payloadLength);
		packet->MoveWPos(header.payloadLength);
		OnReceived(session->id, packet);
	}
}

void SelectModelServer::ProcessDeleteSessionAll()
{
	int count = removeSessions.size();
	for (int i = 0; i < count; i++)
	{
		int sId = removeSessions[i];
		auto find = sessions.find(sId);
		if (find == sessions.end())
		{
			return;
		}
		delete find->second;
		sessions.erase(find);
	}
	removeSessions.clear();
}

void SelectModelServer::SendUnicast(int sId, Packet* packet)
{
	auto find = sessions.find(sId);
	if (find == sessions.end())
	{
		return;
	}
	
	find->second->sendBuffer.Write(packet->GetBuffer(), packet->Count());
	find->second->sendFlag = true;
}

void SelectModelServer::SendBroadcast(Packet* packet)
{
	auto iter = sessions.begin();
	for (; iter != sessions.end(); ++iter)
	{
		iter->second->sendBuffer.Write(packet->GetBuffer(), packet->Count());
		iter->second->sendFlag = true;
	}
}

int SelectModelServer::AllocateSessionId()
{
	return uniqueNum++;
}
