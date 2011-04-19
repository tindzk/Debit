#import "RequestPacket.h"
#import "ResponseSender.h"

#define self RequestPacket

def(void, Init, ResponseSender *sender, Logger *logger) {
	this->sess   = NULL;
	this->state  = ref(State_Processing);
	this->logger = logger;
	this->sender = sender;

	this->response = Response_New(this);

	this->request.priv.referer       = String_New(0);
	this->request.priv.sessionId     = String_New(0);
	this->request.priv.lastModified  = Date_RFC822_New();
	this->request.priv.referer.len   = 0;
	this->request.priv.sessionId.len = 0;

	this->controller = FrontController_New(logger);
}

def(void, Destroy) {
	FrontController_Destroy(&this->controller);

	String_Destroy(&this->request.priv.referer);
	String_Destroy(&this->request.priv.sessionId);

	Response_Destroy(&this->response);
}

def(void, SetVersion, HTTP_Version version) {
	Response_SetVersion(&this->response, version);
	this->request.priv.version = version;
}

def(void, SetMethod, HTTP_Method method) {
	this->request.priv.method = method;
}

def(void, SetCookie, RdString name, RdString value) {
	if (String_Equals(name, $("Session-ID"))) {
		String_Copy(&this->request.priv.sessionId, value);
	}
}

def(void, SetHeader, RdString name, RdString value) {
	String_ToLower((String *) &name);

	if (String_Equals(name, $("if-modified-since"))) {
		this->request.priv.lastModified = Date_RFC822_Parse(value);
	} else if (String_Equals(name, $("referer"))) {
		String_Copy(&this->request.priv.referer, value);
	}
}

static def(void, ResolveSession) {
	SessionManager *sessMgr = SessionManager_GetInstance();

	if (this->request.priv.sessionId.len == 0) {
		/* Initialize the session but don't map it to an ID yet. */
		this->sess = SessionManager_CreateSession(sessMgr);
	} else {
		/* If the ID is found, just use its session object. Otherwise create an
		 * empty session object.
		 */
		Session *res = SessionManager_Resolve(sessMgr,
			this->request.priv.sessionId.rd);

		if (Session_IsNull(res)) {
			this->sess = SessionManager_CreateSession(sessMgr);
		} else {
			this->sess = res;
		}
	}

	assert(this->sess != NULL);

	/* The user was logged in but his last activity is too long ago. */
	if (Session_IsExpired(this->sess)) {
		Logger_Debug(this->logger, $("Session is expired"));
		SessionManager_Unlink(sessMgr, this->request.priv.sessionId.rd);

		/* This resets the whole object, including its ID. Additionally, this
		 * sets `changed' to true so that the ID in the HTTP cookie will be
		 * updated accordingly.
		 */
		Session_Reset(this->sess);
	}
}

def(void, Dispatch, bool persistent) {
	Response_SetPersistent(&this->response, persistent);

	if (this->request.priv.sessionId.len > 0) {
		Logger_Debug(this->logger, $("Client has session ID is '%'"),
			this->request.priv.sessionId.rd);
	}

	call(ResolveSession);

	FrontController_Dispatch(&this->controller,
		this->sess, &this->request, &this->response);
}

def(void, Error, HTTP_Status status, RdString msg) {
	FrontController_Error(&this->controller,
		status, msg, &this->response);
}

def(void, Flush) {
	assert(this->state != ref(State_Done));

	/* sess is not initialized when an error occurred. */
	if (this->sess != NULL) {
		FrontController_PostDispatch(&this->controller,
			this->sess, &this->request, &this->response);

		SessionManager *sessMgr = SessionManager_GetInstance();

		if (Session_HasChanged(this->sess) && !Session_IsReferenced(this->sess)) {
			/* Map the session to an ID... */
			RdString id = SessionManager_Register(sessMgr, this->sess);

			/* ...and update the cookie accordingly. */
			Response_SetCookie(&this->response,
				String_ToCarrier($$("Session-ID")),
				String_ToCarrier(RdString_Exalt(id)));
		}

		if (Session_IsReferenced(this->sess)) {
			/* Update the last activity. */
			Session_Touch(this->sess);
		} else {
			/* Session was not used. It can be safely destroyed. */
			SessionManager_DestroySession(sessMgr, this->sess);
		}
	}

	this->state = ref(State_Done);
	ResponseSender_SendResponse(this->sender, this);
}

def(bool, IsReady) {
	return this->state == ref(State_Done);
}
