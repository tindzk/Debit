#import "FrontController.h"

extern Logger logger;

static def(void, Defaults) {
	this->route    = NULL;
	this->resource = NULL;
	this->instance = Generic_Null();

	this->request.lastModified  = Date_RFC822_Empty();
	this->request.sessionId.len = 0;
}

def(void, Init) {
	this->request.sessionId = HeapString(0);
	call(Defaults);
}

static def(void, DestroyResource) {
	if (this->resource->destroy != NULL) {
		this->resource->destroy(this->instance);
	}

	forward (i, ResourceInterface_MaxMembers) {
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

	String_Destroy(&this->request.sessionId);
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

def(void, SetCookie, String name, String value) {
	if (String_Equals(name, $("Session-ID"))) {
		String_Copy(&this->request.sessionId, value);
	}
}

def(void, SetHeader, String name, String value) {
	name.mutable = true;
	String_ToLower(&name);

	if (String_Equals(name, String("if-modified-since"))) {
		this->request.lastModified = Date_RFC822_Parse(value);
	}
}

static def(ResourceMember *, ResolveMember, String name) {
	forward (i, ResourceInterface_MaxMembers) {
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

def(String *, GetMemberAddr, String name) {
	if (call(HasResource)) {
		ResourceMember *member = call(ResolveMember, name);

		if (member != NULL) {
			if (!member->array) {
				return Generic_GetObject(this->instance) + member->offset;
			} else {
				StringArray **ptr = Generic_GetObject(this->instance) + member->offset;
				StringArray_Push(ptr, HeapString(0));
				return &(*ptr)->buf[(*ptr)->len - 1];
			}
		}
	}

	return NULL;
}

def(bool, Store, String name, String value) {
	String *s = call(GetMemberAddr, name);

	if (s != NULL) {
		String_Copy(s, value);
		return true;
	}

	return false;
}

def(void, SetMethod, HTTP_Method method) {
	this->request.method = method;
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

	forward (i, ResourceInterface_MaxMembers) {
		ResourceMember *member = &this->resource->members[i];

		if (member->name.buf == NULL) {
			break;
		}

		if (member->array) {
			StringArray **ptr = Generic_GetObject(this->instance) + member->offset;
			*ptr = StringArray_New(16);
		} else {
			String *ptr = Generic_GetObject(this->instance) + member->offset;
			*ptr = HeapString(0);
		}
	}

	if (this->resource->init != NULL) {
		this->resource->init(this->instance);
	}
}

def(void, HandleRequest, ResponseInstance resp) {
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

	if (this->request.sessionId.len > 0) {
		Logger_Debug(&logger, $("Client has session ID is '%'"),
			this->request.sessionId);
	}

	SessionInstance sess;

	if (this->request.sessionId.len == 0) {
		/* Initialize the session but don't map it to an ID, yet. */
		sess = Session_New();
		Session_Init(sess);
	} else {
		/* If the ID is found, just return its session object.
		 * Otherwise return an empty object.
		 */
		SessionInstance res = SessionManager_Resolve(sessMgr,
			this->request.sessionId);

		if (Session_IsNull(res)) {
			sess = Session_New();
			Session_Init(sess);
		} else {
			sess = res;
		}
	}

	/* The user was logged in but his last activity is too long ago. */
	if (Session_IsExpired(sess)) {
		Logger_Debug(&logger, $("Session is expired"));
		SessionManager_Unlink(sessMgr, this->request.sessionId);

		/* This resets the whole object, including its ID. But this
		 * sets hasChanged to true so that we the ID in the HTTP
		 * cookie will be updated too.
		 */
		Session_Reset(sess);
	}

	if (this->route->role == Role_Guest || Session_IsUser(sess)) {
		/* Rule doesn't require user role or client is already
		 * authorized.
		 */

		#undef action
		this->route->action(this->instance,
			sess, this->request, resp);
	} else {
		/* Authorization required. */
		Logger_Debug(&logger, $("Authorization required"));
	}

	if (Session_HasChanged(sess) && !Session_IsReferenced(sess)) {
		/* Map the session to an ID... */
		String id = SessionManager_Register(sessMgr, sess);

		/* ...and update the cookie accordingly. */
		Response_SetCookie(resp, $("Session-ID"), id);
	}

	if (Session_IsReferenced(sess)) {
		/* Update the last activity. */
		Session_Touch(sess);
	} else {
		/* Session was not used. It can be safely destroyed. */
		Session_Destroy(sess);
		Session_Free(sess);
	}
}
