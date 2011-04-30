#import <HTTP.h>
#import <Task.h>
#import <String.h>

#import "Session.h"
#import "Request.h"
#import "Response.h"

#ifndef ResourceInterface_MaxMembers
#define ResourceInterface_MaxMembers 30
#endif

#ifndef ResourceInterface_MaxRoutes
#define ResourceInterface_MaxRoutes  15
#endif

typedef void (ResourceInit)   (Instance $this);
typedef void (ResourceDestroy)(Instance $this);
typedef void (ResourceAction) (Instance $this,
	Session  *sess,
	Request  *req,
	Response *resp,
	Tasks    *tasks);

#define action(name)             \
	static def(void, name,       \
		__unused Session  *sess, \
		__unused Request  *req,  \
		__unused Response *resp, \
		__unused Tasks    *tasks)

#define dispatch(name) \
	call(name, sess, req, resp, tasks)

#define RouterConstructor                               \
	Constructor {                                       \
		Router *router = Router_GetInstance();          \
		Router_AddResource(router, &ref(ResourceImpl)); \
	}

record(ResourceMember) {
	RdString name;
	size_t offset;
	bool array;
};

#define Member(name) \
	offsetof(self, name)

set(Role) {
	/* This is the default value when the `role' field is not set in
	 * the resource structure. The page can be accessed by anyone.
	 */

	Role_Unspecified,

	/* The user is unauthorized. */
	Role_Guest,

	/* The user is logged in. */
	Role_User
};

record(ResourceRoute) {
	RdString path;
	ResourceAction *action;

	/* If unspecified, the resource's role is inherited. */
	Role role;

	ResourceAction *setUp;
	ResourceAction *tearDown;
};

Interface(Resource) {
	Role role;

	size_t   size;
	RdString name;

	ResourceInit    *init;
	ResourceDestroy *destroy;

	ResourceRoute  routes  [ResourceInterface_MaxRoutes ];
	ResourceMember members [ResourceInterface_MaxMembers];
};
