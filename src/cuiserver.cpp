#include "cuiserver.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
using namespace json_spirit;

boost::thread_group CUIServer::m_threadGroup;

bool CUIServer::HasConnection(){
	if(NULL == instance)
		return false;

	return instance->m_bConnect;
}

void CUIServer::StartServer() {
	m_threadGroup.create_thread(boost::bind(&CUIServer::RunThreadPorc, CUIServer::getInstance()));
}

void CUIServer::StopServer(){
	if(NULL == instance)
		return ;
	instance->m_iosev.stop();
	instance->m_bRunFlag = false;
	m_threadGroup.interrupt_all();
	m_threadGroup.join_all();
}

void CUIServer::RunThreadPorc(CUIServer* pThis) {
	if (NULL == pThis)
		return;

	pThis->RunServer();
}

CUIServer* CUIServer::getInstance() {
	if (NULL == instance)
		instance = new CUIServer();
	return instance;
}

void CUIServer::Send(const string& strData) {
	if(NULL == instance)
		return ;
//	LogPrint("TOUI","send message: %s\n", strData);
	string sendData(strData);
	PackageData(sendData);
	LogPrint("TOUI","send message: %s\n", sendData);
	instance->SendData(sendData);
}

bool CUIServer::IsInitalEnd = false;
void CUIServer::SendData(const string& strData) {
	system::error_code ignored_error;
	if(m_bConnect&&m_socket.get() ){
		m_socket->write_some(asio::buffer(strData), ignored_error);
	}
}

void CUIServer::Accept() {
	sock_pt sock(new tcp::socket(m_iosev));
	m_acceptor.async_accept(*sock, bind(&CUIServer::Accept_handler, this, sock));
}

void CUIServer::Accept_handler(sock_pt sock) {
	if(m_bConnect)
	{
		//only save one connection
		Accept();
		return;
	}
	m_socket = sock;
	m_bConnect = true;
	Object obj;
	if (CUIServer::IsInitalEnd == true) {
		obj.push_back(Pair("type", "init"));
		obj.push_back(Pair("msg", "initialize end"));
	} else {
		obj.push_back(Pair("type", "hello"));
		obj.push_back(Pair("msg", "hello asio"));
	}

	Accept();
	string sendData = write_string(Value(std::move(obj)),true);
	PackageData(sendData);
	sock->async_write_some(asio::buffer(sendData), bind(&CUIServer::write_handler, this));
	std::shared_ptr<vector<char> > str(new vector<char>(100, 0));
	memset(data_,0,max_length);
	sock->async_read_some(asio::buffer(data_,max_length),
			bind(&CUIServer::read_handler, this, asio::placeholders::error, data_, sock));
}

void CUIServer::read_handler(const system::error_code& ec, char* pstr, sock_pt sock) {
	if (ec) {
		sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both, const_cast<system::error_code&>(ec));
		sock->close(const_cast<system::error_code&>(ec));
		m_bConnect = false;
		return;
	}
}

void CUIServer::RunServer(){
	Accept();
	m_iosev.run();
	if(instance != NULL && !instance->m_bRunFlag) {
		delete instance;
			instance = NULL;
	}
}

CUIServer* CUIServer::instance = NULL;


void CUIServer::PackageData(string &strData) {
	unsigned char cDataTemp[65536] = {0};
	unsigned short nDataLen = strData.length();
	if(0 == nDataLen)
		return;
	cDataTemp[0] = '<';
	memcpy(cDataTemp+1, &nDataLen, 2);
	memcpy(cDataTemp+3, strData.c_str(), nDataLen);
	cDataTemp[nDataLen+3] = '>';
	LogPrint("TOUI","send message length: %d\n", nDataLen);
	strData.assign(cDataTemp, cDataTemp+nDataLen+4);
//	cout << "Send Data len " << nDataLen + 4 << ":";
//	for(int i=0; i< nDataLen+4; ++i)
//		printf("%02X", cDataTemp[i]);
//	cout << endl;
//	strData = strprintf("<%s%s>", cLen, strData.c_str());
}
