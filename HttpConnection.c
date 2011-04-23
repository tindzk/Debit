#import "HttpConnection.h"

#define self HttpConnection

class {
	ResponseSender respSender;
	HTTP_Server    server;
	Logger         *logger;
	Server_Client  *client;
};

static def(void, OnRequest);
static def(void, OnRequestInfo, HTTP_RequestInfo info);
static def(void, OnHeader, RdString name, RdString value);
static def(String *, OnQueryParameter, RdString name);
static def(String *, OnBodyParameter, RdString name);
static def(void, OnRespond, bool persistent);

def(void, Init, Server_Client *client, Logger *logger) {
	this->client = client;
	this->server = HTTP_Server_New(client->socket.conn, 2048, 4096);

	HTTP_Server_BindRequest(&this->server,
		HTTP_Server_OnRequest_For(this, ref(OnRequest)));

	HTTP_Server_BindRequestInfo(&this->server,
		HTTP_OnRequestInfo_For(this, ref(OnRequestInfo)));

	HTTP_Server_BindHeader(&this->server,
		HTTP_OnHeader_For(this, ref(OnHeader)));

	HTTP_Server_BindQueryParameter(&this->server,
		HTTP_OnParameter_For(this, ref(OnQueryParameter)));

	HTTP_Server_BindBodyParameter(&this->server,
		HTTP_OnParameter_For(this, ref(OnBodyParameter)));

	HTTP_Server_BindRespond(&this->server,
		HTTP_Server_OnRespond_For(this, ref(OnRespond)));

	this->logger = logger;

	Logger_Debug(this->logger, $("Connection initialized"));

	ResponseSender_Init(&this->respSender, client, logger);
}

def(void, Destroy) {
	Logger_Debug(this->logger, $("Connection destroyed"));

	ResponseSender_Destroy(&this->respSender);
	HTTP_Server_Destroy(&this->server);
}

static def(void, OnRequest) {
	Logger_Debug(this->logger, $("Receiving request..."));
	ResponseSender_NewPacket(&this->respSender);
}

static def(void, OnRequestInfo, HTTP_RequestInfo info) {
	RequestPacket *packet = ResponseSender_GetPacket(&this->respSender);

	RequestPacket_SetMethod(packet,  info.method);
	RequestPacket_SetVersion(packet, info.version);

	Logger_Info(this->logger, $("% % %"),
		HTTP_Method_ToString(info.method), info.path,
		HTTP_Version_ToString(info.version));

	Router *router = Router_GetInstance();

	MatchingRoute match = Router_FindRoute(router, info.path);

	if (match.route == NULL) {
		match = Router_GetDefaultRoute(router);
	}

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
		RdString cookieName, cookieValue;
		if (String_Parse($("%=%"), value, &cookieName, &cookieValue)) {
			RequestPacket_SetCookie(packet, cookieName, cookieValue);
		}
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

static inline def(void, Error, HTTP_Status status, RdString msg) {
	RequestPacket *packet = ResponseSender_GetPacket(&this->respSender);
	RequestPacket_Error(packet, status, msg);
}

static def(void, Pull) {
	Logger_Debug(this->logger, $("Got pull"));

	ResponseSender_SetComplete(&this->respSender, false);

	try {
		bool incomplete = HTTP_Server_Process(&this->server);
		ResponseSender_SetComplete(&this->respSender, !incomplete);
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
		call(Error, HTTP_Status_ClientError_ExpectationFailed,
			$("Body not expected for given method."));
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
		if (e != 0 || ResponseSender_Close(&this->respSender)) {
			Server_Client_Close(this->client);
		}
	} tryEnd;
}

static def(void, Push) {
	Logger_Debug(this->logger, $("Got push"));
	ResponseSender_Continue(&this->respSender);
}

Impl(Connection) = {
	.size    = sizeof(self),
	.init    = ref(Init),
	.destroy = ref(Destroy),
	.pull    = ref(Pull),
	.push    = ref(Push)
};
