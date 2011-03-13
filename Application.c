#import "Application.h"

#define self Application

def(bool, StartServer, Server *server, ClientListener listener) {
	try {
		Server_Init(server, 8080, listener);
		Logger_Info(&this->logger, $("Server started."));
		excReturn true;
	} catch(Socket, AddressInUse) {
		Logger_Error(&this->logger, $("The address is already in use!"));
		excReturn false;
	} finally {

	} tryEnd;

	return false;
}

override def(void, OnInit)    { }
override def(void, OnDestroy) { }

def(bool, Run) {
	GenericClientListener listener;
	GenericClientListener_Init(&listener, HttpConnection_GetImpl(), &this->logger);

	Server server;

	if (!call(StartServer, &server,
		GenericClientListener_AsClientListener(&listener)))
	{
		return false;
	}

	call(OnInit);

	try {
		while (true) {
			Server_Process(&server);
		}
	} catch(Signal, SigInt) {
		Logger_Info(&this->logger, $("Server shutdown."));
	} finally {
		Server_Destroy(&server);
		call(OnDestroy);
	} tryEnd;

	return true;
}
