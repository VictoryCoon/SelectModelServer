#include "PreCompliedHeader.h"
#include "GameServer.h"

using namespace std;

int main()
{
	GameServer server;
	server.Initialize("127.0.0.1", 16000);

	server.Run();

	server.Release();
	return 0;
}