#include <thread>

#include <projects/vocal/vocal_lib/resolver.h>
#include <projects/vocal/vocal_lib/utils.h>
#include <projects/vocal/vocal_lib/compress.h>
#include <projects/vocal/vocal_lib/crypto.h>
#include <projects/vocal/vocal_lib/serializer.h>
#include <projects/vocal/vocal_lib/nat_pmp.h>

#include "friend.h"
#include "client.h"

using namespace std;
using namespace std::placeholders;

namespace NVocal {

TFriend::TFriend()
    : ConnectionStatus(COS_Offline)
    , Client(nullptr)
{
}

TFriend::TFriend(const string& login, EFriendStatus status)
    : FullLogin(login)
    , Status(status)
    , ToDelete(false)
    , ConnectionStatus(COS_Offline)
    , Client(nullptr)
{
}

void TFriend::InitUdtClient() {
    NUdt::TClientConfig config;
    config.ConnectionCallback = std::bind(&TFriend::OnConnected, this, _1);
    config.DataReceivedCallback = std::bind(&TFriend::OnDataReceived, this, _1);
    config.ConnectionLostCallback = std::bind(&TFriend::OnDisconnected, this);
    UdtClient.reset(new NUdt::TClient(config));
}

bool TFriend::OnClientConnected(const TNetworkAddress& addr) {
    if (ConnectionStatus != COS_WaitingFriendConnection) {
        return false;
    }
    FriendAddress = addr;
    ConnectionStatus = COS_AcceptedConnection;
    cerr << "ESTABLISHED FRIEND CONNECTION!\n";
}

const string& TFriend::GetLogin() {
    return FullLogin;
}

const string& TFriend::GetName() {
    return Name;
}

EFriendStatus TFriend::GetStatus() {
    return Status;
}

const string& TFriend::GetStatusMessage() {
    assert(false && "unimplemented");
}

std::vector<TMessage> TFriend::GetHistory() {
    assert(false && "unimplemented");
}

void TFriend::SendMssg(const std::string& message) {
    assert(false && "unimplemented");
}

void TFriend::SendFile(const std::string& name,
              size_t size,
              TDataRequireCallback fileDataCallback)
{
    assert(false && "unimplemented");
}

void TFriend::StartCall(TDataRequireCallback videoDataRequireCallback,
               TDataRequireCallback audioDataRequireCallback,
               TDataCallback audioDataCallback,
               TDataCallback videoDataCallback,
               TBoolCallback partnerVideoStatusCallback,
               bool videoEnabled)
{
    assert(false && "unimplemented");
}

void TFriend::EnableVideo() {
    assert(false && "unimplemented");
}

void TFriend::DisableVideo() {
    assert(false && "unimplemented");
}

void TFriend::FinishCall() {
    assert(false && "unimplemented");
}

// todo: don't try to connect very often
void TFriend::Connect() {
    if (Status != FS_Offline || ConnectionStatus != COS_Offline) {
        return;
    }
    assert(!PublicKey.empty() && "no public key for friend");
    assert(!ServerPublicKey.empty() && "no server key for friend");

    if (Client->HasNatPmp()) {
        TNatPmp& natPmp = Client->GetNatPmp();
        PublicAddress = natPmp.GetPublicAddress();
        try {
            natPmp.ForwardRandomPort(LocalPort, PublicPort);
        } catch (const UException&) {
            LocalPort = 0;
            PublicPort = 0;
            cerr << "warning: failed to forward ports with natPmp\n";
        }
    }

    InitUdtClient();

    pair<string, string> loginHost = GetLoginHost(FullLogin);
    std::vector<TNetworkAddressRef> addresses = GetConnectionAddresses(loginHost.first, loginHost.second);
    if (addresses.size() == 0) {
        throw UException("no address found for host");
    }
    // todo: random address selection
    ConnectionStatus = COS_ConnectingToServer;
    AcceptingConnection = false;
    UdtClient->Connect(*addresses[0], false);
}

void TFriend::ConnectAccept() {
    if (Status != FS_Offline || ConnectionStatus != COS_Offline) {
        return;
    }

    assert(!PublicKey.empty() && "no public key for friend");
    assert(!ServerPublicKey.empty() && "no server key for friend");

    InitUdtClient();

    std::vector<TNetworkAddressRef> addresses = GetConnectionAddresses(Client->State.login(), Client->State.host());
    if (addresses.size() == 0) {
        throw UException("no address found for host");
    }
    // todo: random address selection
    ConnectionStatus = COS_ConnectingToServer;
    AcceptingConnection = true;
    UdtClient->Connect(*addresses[0]);
}

void TFriend::OnConnected(bool success) {
    if (!success) {
        ConnectionStatus = COS_Offline;
        return;
    }
    if (ConnectionStatus == COS_ConnectingToServer) {
        string data;
        data.push_back((char)RT_HelpConnect);
        UdtClient->Send(data);
    } else if (ConnectionStatus == COS_ConnectingToFriend) {
        cerr << "ESTABLISHED FRIEND CONNECTION!\n"; // todo: process friend connection
        UdtClient->Send(string("hello, words!"));
    }
}

void TFriend::OnDataReceived(const TBuffer& data) {
    Buffer += data.ToString();
    string packetStr;
    if (!Deserialize(Buffer, packetStr)) {
        return;
    }

    switch (ConnectionStatus) {
    case COS_ConnectingToServer: {
        packetStr = Decompress(packetStr);
        TServerAuthorizeRequest packet;
        if (!packet.ParseFromString(packetStr)) {
            cerr << "warning: failed to deserialize server authroize request\n";
            ForceDisconnect();
            return;
        }
        if (!CheckSignature(ServerPublicKey, packet.randomsequence(), packet.signature())) {
            cerr << "warning: failed to check server signature\n";
            ForceDisconnect();
            return;
        }
        TClientConnectHelpAuthorizeRequest request;
        assert(!FullLogin.empty() && "friend has no login");
        assert(Client != nullptr && "Client is NULL");
        request.set_login(Client->GetFullLogin());
        request.set_friendlogin(FullLogin);
        string hash = Hash(packet.randomsequence());
        request.set_randomsequencehash(hash);
        request.set_randomsequencehashsignature(Sign(Client->GetPrivateKey(), hash));
        request.set_acceptingconnection(AcceptingConnection);
        if (LocalPort != 0) {
            TNetworkAddress address = PublicAddress;
            address.Port = PublicPort;
            request.set_publicaddress(address.ToString());
        }
        string response = Compress(request.SerializeAsString());
        response = Serialize(EncryptAsymmetrical(ServerPublicKey, response));
        ConnectionStatus = COS_WaitingFriendAddress;
        UdtClient->Send(response);
    } break;
    case COS_WaitingFriendAddress: {
        assert(Client != nullptr && "Client is NULL");
        packetStr = Decompress(DecryptAsymmetrical(Client->GetPrivateKey(), packetStr));
        TServerConnectHelpRequest connectHelp;
        if (!connectHelp.ParseFromString(packetStr)) {
            cerr << "error: failed to deserialize connect help request\n";
            return;
        }
        // todo: free nat-pmp port if not used
        switch (connectHelp.connectiontype()) {
        case CTP_Offline: {
            ForceDisconnect();
            return;
        } break;
        case CTP_NatTraversal: {
            ConnectThrowNat(TNetworkAddress(connectHelp.address()), LocalPort);
        } break;
        case CTP_Listen: {
            UdtClient->Disconnect();
            NUdt::TServerConfig config;
            config.NewConnectionCallback = bind(&TFriend::OnClientConnected, this, _1);
            config.DataReceivedCallback = bind(&TFriend::OnDataReceived, this, _1);
            config.Port = LocalPort;
            ConnectionStatus = COS_WaitingFriendConnection;
            UdtServer.reset(new NUdt::TServer(config));
        } break;
        case CTP_Connect: {
            ConnectionStatus = COS_ConnectingToFriend;
            UdtClient->Connect(connectHelp.address());
        } break;
        }

    } break;
    }
}

void TFriend::OnDisconnected() {
}

void TFriend::ForceDisconnect() {
    Status = FS_Offline;
    ConnectionStatus = COS_Offline;
    UdtClient->Disconnect();
}

void TFriend::ConnectThrowNat(const TNetworkAddress& address, ui16 localPort) {
    if (Status != FS_Offline ||
            !(ConnectionStatus == COS_Offline ||
              ConnectionStatus == COS_WaitingFriendAddress))
    {
        cerr << "warning: wrong status to connect throw nat\n";
        return;
    }
    ConnectionStatus = COS_ConnectingToFriend;
    UdtClient->Connect(address, localPort, true);
}

} // NVocal
