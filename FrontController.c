#import "FrontController.h"

#define self FrontController

static def(void, Defaults) {
	this->route    = NULL;
	this->resource = NULL;
	this->instance = Generic_Null();

	this->request.priv.lastModified  = Date_RFC822_Empty();
	this->request.priv.referer.len   = 0;
	this->request.priv.sessionId.len = 0;
}

rsdef(self, New) {
	self res;

	res.request.priv.referer   = String_New(0);
	res.request.priv.sessionId = String_New(0);

	scall(Defaults, &res);

	return res;
}

static def(void, DestroyResource) {
	if (this->resource->destroy != NULL) {
		this->resource->destroy(this->instance);
	}

	fwd(i, ResourceInterface_MaxMembers) {
		ResourceMember *member = &this->resource->members[i];

		if (member->name.buf == NULL) {
			break;
		}

		if (member->array) {
			StringArray **s = Generic_GetObject(this->instance) + member->offset;
			StringArray_Destroy(*s);
			StringArray_Free(*s);
		} else {
			String *s = Generic_GetObject(this->instance) + member->offset;
			String_Destroy(s);
		}
	}

	if (!Generic_IsNull(this->instance)) {
		Generic_Free(this->instance);
	}
}

def(void, Destroy) {
	if (this->resource != NULL) {
		call(DestroyResource);
	}

	String_Destroy(&this->request.priv.referer);
	String_Destroy(&this->request.priv.sessionId);
}

def(bool, HasResource) {
	return this->resource != NULL;
}

def(void, Reset) {
	if (this->resource != NULL) {
		call(DestroyResource);
	}

	call(Defaults);
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

static def(ResourceMember *, ResolveMember, RdString name) {
	fwd(i, ResourceInterface_MaxMembers) {
		ResourceMember *member = &this->resource->members[i];

		if (member->name.buf == NULL) {
			break;
		}

		if (String_Equals(member->name, name)) {
			return member;
		}
	}

	return NULL;
}

def(String *, GetMemberAddr, RdString name) {
	if (call(HasResource)) {
		ResourceMember *member = call(ResolveMember, name);

		if (member != NULL) {
			if (!member->array) {
				return Generic_GetObject(this->instance) + member->offset;
			} else {
				StringArray **ptr = Generic_GetObject(this->instance) + member->offset;
				StringArray_Push(ptr, String_New(0));
				return &(*ptr)->buf[(*ptr)->len - 1];
			}
		}
	}

	return NULL;
}

def(bool, Store, RdString name, RdString value) {
	String *s = call(GetMemberAddr, name);

	if (s != NULL) {
		String_Copy(s, value);
		return true;
	}

	return false;
}

def(void, SetMethod, HTTP_Method method) {
	this->request.priv.method = method;
}

def(void, SetRoute, ResourceRoute *route) {
	this->route = route;
}

def(void, SetResource, ResourceInterface *resource) {
	this->resource = resource;
}

def(void, CreateResource) {
	this->instance = (this->resource->size > 0)
		? Generic_New(this->resource->size)
		: Generic_Null();

	fwd(i, ResourceInterface_MaxMembers) {
		ResourceMember *member = &this->resource->members[i];

		if (member->name.buf == NULL) {
			break;
		}

		if (member->array) {
			StringArray **ptr = Generic_GetObject(this->instance) + member->offset;
			*ptr = StringArray_New(16);
		} else {
			String *ptr = Generic_GetObject(this->instance) + member->offset;
			*ptr = String_New(0);
		}
	}

	if (this->resource->init != NULL) {
		this->resource->init(this->instance);
	}
}

def(void, HandleRequest, Logger *logger, ResponseInstance resp) {
	if (this->route->role == Role_Unspecified) {
		if (this->resource->role == Role_Unspecified) {
			/* Default role. Page is accessible for anyone. */
			this->route->role = Role_Guest;
		} else {
			/* Inherit role. */
			this->route->role = this->resource->role;
		}
	}

	SessionManagerInstance sessMgr = SessionManager_GetInstance();

	if (this->request.priv.sessionId.len > 0) {
		Logger_Debug(logger, $("Client has session ID is '%'"),
			this->request.priv.sessionId.rd);
	}

	SessionInstance sess;

	if (this->request.priv.sessionId.len == 0) {
		/* Initialize the session but don't map it to an ID, yet. */
		sess = SessionManager_CreateSession(sessMgr);
	} else {
		/* If the ID is found, just use its session object. Otherwise create an
		 * empty session object.
		 */
		SessionInstance res = SessionManager_Resolve(sessMgr,
			this->request.priv.sessionId.rd);

		if (Session_IsNull(res)) {
			sess = SessionManager_CreateSession(sessMgr);
		} else {
			sess = res;
		}
	}

	/* The user was logged in but his last activity is too long ago. */
	if (Session_IsExpired(sess)) {
		Logger_Debug(logger, $("Session is expired"));
		SessionManager_Unlink(sessMgr, this->request.priv.sessionId.rd);

		/* This resets the whole object, including its ID. But this
		 * sets hasChanged to true so that we the ID in the HTTP
		 * cookie will be updated too.
		 */
		Session_Reset(sess);
	}

	if (this->route->role == Role_Guest /* || Session_IsUser(sess) */) {
		/* Rule doesn't require user role or client is already
		 * authorized.
		 */

		#undef action

		if (this->route->setUp != NULL) {
			this->route->setUp(this->instance,
				sess, this->request, resp);
		}

		try {
			this->route->action(this->instance,
				sess, this->request, resp);
		} catchAny {
			String fmt = Exception_Format(e);
			Logger_Debug(logger, fmt.rd);
			BufferResponse(resp, fmt);

#if Exception_SaveTrace
			Backtrace_PrintTrace(
				Exception_GetTraceBuffer(),
				Exception_GetTraceLength());
#endif
		} finally {

		} tryEnd;

		if (this->route->tearDown != NULL) {
			this->route->tearDown(this->instance,
				sess, this->request, resp);
		}
	} else {
		/* Authorization required. */
		Logger_Debug(logger, $("Authorization required"));
	}

	if (Session_HasChanged(sess) && !Session_IsReferenced(sess)) {
		/* Map the session to an ID... */
		RdString id = SessionManager_Register(sessMgr, sess);

		/* ...and update the cookie accordingly. */
		Response_SetCookie(resp,
			String_ToCarrier($$("Session-ID")),
			String_ToCarrier(RdString_Exalt(id)));
	}

	if (Session_IsReferenced(sess)) {
		/* Update the last activity. */
		Session_Touch(sess);
	} else {
		/* Session was not used. It can be safely destroyed. */
		SessionManager_DestroySession(sessMgr, sess);
	}
}
