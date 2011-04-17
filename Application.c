#import "Application.h"

#define self Application

override def(void, OnInit)    { }
override def(void, OnDestroy) { }

def(void, ListRoutes) {
	Logger_Debug(&this->logger, $("Registered resources:"));

	Router *router = Router_GetInstance();

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
	Server server = Server_New(HttpConnection_GetImpl(), &this->logger);

	try {
		Server_Listen(&server, 8080);
		Logger_Info(&this->logger, $("Server started."));
	} catch(Socket, AddressInUse) {
		Logger_Error(&this->logger, $("The address is already in use!"));
		excReturn false;
	} finally {

	} tryEnd;

	call(ListRoutes);

	call(OnInit);

	try {
		EventLoop_Run(EventLoop_GetInstance());
	} catch(Signal, SigInt) {
		Logger_Info(&this->logger, $("Server shutdown."));
	} finally {
		Server_Destroy(&server);
		call(OnDestroy);
	} tryEnd;

	return true;
}
