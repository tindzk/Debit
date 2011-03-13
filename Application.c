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

def(void, ListRoutes) {
	Logger_Debug(&this->logger, $("Registered resources:"));

	RouterInstance router = Router_GetInstance();

	Resources *resources = Router_GetResources(router);

	fwd(i, resources->len) {
		ResourceInterface *resource = resources->buf[i];

		Logger_Debug(&this->logger, $("* %"),
			(resource->name.len == 0)
				? $("<unnamed>")
				: resource->name);

		fwd(j, ResourceInterface_MaxRoutes) {
			ResourceRoute *route = &resource->routes[j];

			if (route->path.len == 0) {
				break;
			}

			Logger_Debug(&this->logger, $("  - %"), route->path);
		}
	}
}

def(bool, Run) {
	call(ListRoutes);

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
