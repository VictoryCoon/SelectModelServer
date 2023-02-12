#pragma once

using namespace std;

class SelectModelServer
{
public:
	SelectModelServer();
	~SelectModelServer();

	void Initialize(string ipAddress, unsigned short port);
	void Run();
	void Release();
	void Shutdown();
public:
	static unsigned long ToNetworkLong(string ipAddress);
	static void ToPeer(char* dst, const void* pAddress);

private:
	void CreateSession(int sId, SOCKET socket, sockaddr_in address);
	void DeleteSession(int sId);
	bool ProcessRecv(int sId);
	bool ProcessSend(int sId);
	void ProcessPacket(Session* session);
	void ProcessDeleteSessionAll();
	int AllocateSessionId();
protected:
	void SendUnicast(int sId, Packet* packet);
	void SendBroadcast(Packet* packet);

	virtual void OnConnected(int sId) = 0;
	virtual void OnDisconnected(int sId) = 0;
	virtual void OnReceived(int sId, Packet* packet) = 0;
	virtual void OnSent(int sId) = 0;
	
private:
	SOCKET listenSocket;
	fd_set rSet;
	fd_set wSet;

	map<int, Session*> sessions;
	vector<int> removeSessions;

	bool isShutdown = false;
};