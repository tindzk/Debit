#import "HttpConnection.h"

#define self HttpConnection

class {
	bool incomplete;

	HTTP_Method  method;
	HTTP_Server  server;
	HTTP_Version version;

	ResponseSender respSender;

	Logger            *logger;
	Connection_Client *client;
};

static def(void, OnRequest);
static def(void, OnHeader, RdString name, RdString value);
static def(void, OnVersion, HTTP_Version version);
static def(void, OnMethod, HTTP_Method method);
static def(void, OnPath, RdString path);
static def(String *, OnQueryParameter, RdString name);
static def(String *, OnBodyParameter, RdString name);
static def(void, OnRespond, bool persistent);

def(void, Init, Connection_Client *client, Logger *logger) {
	this->method     = HTTP_Method_Get;
	this->version    = HTTP_Version_1_0;
	this->server     = HTTP_Server_New(client->conn, 2048, 4096);
	this->client     = client;
	this->incomplete = true;

	HTTP_Server_BindRequest(&this->server, HTTP_Server_OnRequest_For(this, ref(OnRequest)));
	HTTP_Server_BindHeader(&this->server, HTTP_OnHeader_For(this, ref(OnHeader)));
	HTTP_Server_BindVersion(&this->server, HTTP_OnVersion_For(this, ref(OnVersion)));
	HTTP_Server_BindMethod(&this->server, HTTP_OnMethod_For(this, ref(OnMethod)));
	HTTP_Server_BindPath(&this->server, HTTP_OnPath_For(this, ref(OnPath)));
	HTTP_Server_BindQueryParameter(&this->server, HTTP_OnParameter_For(this, ref(OnQueryParameter)));
	HTTP_Server_BindBodyParameter(&this->server, HTTP_OnParameter_For(this, ref(OnBodyParameter)));
	HTTP_Server_BindRespond(&this->server, HTTP_Server_OnRespond_For(this, ref(OnRespond)));

	this->logger = logger;

	Logger_Debug(this->logger, $("Connection initialized"));

	ResponseSender_Init(&this->respSender, client, logger);
}

def(void, Destroy) {
	Logger_Debug(this->logger, $("Connection destroyed"));

	ResponseSender_Destroy(&this->respSender);
	HTTP_Server_Destroy(&this->server);
}

static def(void, Error, HTTP_Status status, RdString msg) {
	RequestPacket *packet = ResponseSender_GetPacket(&this->respSender);

	FrontController_Error(&packet->controller, status, msg, &packet->response);
}

def(void, Process) {
	this->incomplete = true;

	String fmt = String_New(0);

	try {
		this->incomplete = HTTP_Server_Process(&this->server);
	} catch(HTTP_Query, ExceedsPermittedLength) {
		call(Error, HTTP_Status_ClientError_RequestEntityTooLarge,
			$("A parameter exceeds its permitted length."));
	} catch(HTTP_Header, UnknownVersion) {
		call(Error, HTTP_Status_ServerError_VersionNotSupported,
			$("Unknown HTTP version."));
	} catch(HTTP_Header, UnknownMethod) {
		call(Error, HTTP_Status_ServerError_NotImplemented,
			$("Method is not implemented."));
	} catch(HTTP_Server, BodyUnexpected) {
		fmt = String_Format(
			$("Body not expected with method '%'."),
			HTTP_Method_ToString(this->method));
		call(Error, HTTP_Status_ClientError_ExpectationFailed, fmt.rd);
	} catch(HTTP_Header, RequestMalformed) {
		call(Error, HTTP_Status_ClientError_BadRequest,
			$("Request malformed."));
	} catch(HTTP_Server, HeaderTooLarge) {
		call(Error, HTTP_Status_ClientError_RequestEntityTooLarge,
			$("The header exceeds its permitted length."));
	} catch(HTTP_Server, BodyTooLarge) {
		call(Error, HTTP_Status_ClientError_RequestEntityTooLarge,
			$("The body exceeds its permitted length."));
	} catch(HTTP_Server, UnknownContentType) {
		call(Error, HTTP_Status_ClientError_NotAcceptable,
			$("Content-Type not recognized."));
	} catch(HTTP_Header, EmptyRequestUri) {
		call(Error, HTTP_Status_ClientError_BadRequest,
			$("Empty request URI."));
	} catchAny {
		Logger_Error(this->logger, $("Uncaught exception in HTTP server"));
		__exc_rethrow = true;
	} finally {
		if (e != 0) {
			this->incomplete = false;
		}

		String_Destroy(&fmt);
	} tryEnd;
}

static def(void, OnRequest) {
	Logger_Debug(this->logger, $("Receiving request..."));
	ResponseSender_NewPacket(&this->respSender);
}

static def(void, OnVersion, HTTP_Version version) {
	RequestPacket *packet = ResponseSender_GetPacket(&this->respSender);
	RequestPacket_SetVersion(packet, version);

	this->version = version;
}

static def(void, OnMethod, HTTP_Method method) {
	RequestPacket *packet = ResponseSender_GetPacket(&this->respSender);
	RequestPacket_SetMethod(packet, method);

	this->method = method;
}

/* Parameters to extract from URL. */
static def(void, OnPath, RdString path) {
	Logger_Info(this->logger, $("% % %"),
		HTTP_Method_ToString(this->method), path,
		HTTP_Version_ToString(this->version));

	Router *router = Router_GetInstance();

	MatchingRoute match = Router_FindRoute(router, path);

	if (match.route == NULL) {
		match = Router_GetDefaultRoute(router);
	}

	RequestPacket *packet = ResponseSender_GetPacket(&this->respSender);

	if (match.route != NULL) {
		FrontController_CreateResource(&packet->controller,
			match.route, match.resource);

		/* `routeElems' and `pathElems' aren't used for the default route. */
		if (match.routeElems != NULL) {
			Router_ExtractParts(router,
				match.routeElems, match.pathElems,
				Router_OnPart_For(&packet->controller, FrontController_StoreEx));
		}

		Router_DestroyMatch(router, match);
	}
}

static def(void, OnHeader, RdString name, RdString value) {
	Logger_Debug(this->logger, $("Header: % = %"), name, value);

	RequestPacket *packet = ResponseSender_GetPacket(&this->respSender);

	if (String_Equals(name, $("Cookie"))) {
		RdStringArray *items = String_Split(value, '=');

		if (items->len > 1) {
			RequestPacket_SetCookie(packet,
				items->buf[0],
				items->buf[1]);
		}

		RdStringArray_Free(items);
	} else {
		RequestPacket_SetHeader(packet, name, value);
	}
}

static def(String *, OnQueryParameter, RdString name) {
	Logger_Debug(this->logger, $("Received GET parameter '%'"), name);

	RequestPacket *packet = ResponseSender_GetPacket(&this->respSender);

	return FrontController_GetMemberAddr(&packet->controller, name);
}

static def(String *, OnBodyParameter, RdString name) {
	Logger_Debug(this->logger, $("Received POST parameter '%'"), name);

	RequestPacket *packet = ResponseSender_GetPacket(&this->respSender);

	return FrontController_GetMemberAddr(&packet->controller, name);
}

static def(void, OnRespond, bool persistent) {
	RequestPacket *packet = ResponseSender_GetPacket(&this->respSender);
	RequestPacket_Dispatch(packet, persistent);
}

static def(Connection_Status, Pull) {
	Logger_Debug(this->logger, $("Got pull"));

	call(Process);

	return !this->incomplete && ResponseSender_Close(&this->respSender)
		? Connection_Status_Close
		: Connection_Status_Open;
}

static def(Connection_Status, Push) {
	Logger_Debug(this->logger, $("Got push"));

	ResponseSender_Continue(&this->respSender);

	return !this->incomplete && ResponseSender_Close(&this->respSender)
		? Connection_Status_Close
		: Connection_Status_Open;
}

Impl(Connection) = {
	.size    = sizeof(self),
	.init    = ref(Init),
	.destroy = ref(Destroy),
	.pull    = ref(Pull),
	.push    = ref(Push)
};
