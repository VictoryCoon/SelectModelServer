#pragma once
#include "SelectModelServer.h"
class GameServer :
    public SelectModelServer
{
protected:
    virtual void OnConnected(int sId) override;
    virtual void OnDisconnected(int sId) override;
    virtual void OnReceived(int sId, Packet* packet) override;
    virtual void OnSent(int sId) override;

private:
    void ProcessRequestChat(int sId, Packet* packet);
};

