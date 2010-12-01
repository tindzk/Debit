#import "Router.h"

#define self Router

Singleton(self);
SingletonDestructor(self);

def(void, Init) {
	this->resources = Resources_New(75);
}

def(void, Destroy) {
	Resources_Free(this->resources);
}

def(void, DestroyMatch, MatchingRoute match) {
	Array_Destroy(match.pathElems);
	Array_Destroy(match.routeElems);
}

def(void, AddResource, ResourceInterface *resource) {
	Resources_Push(&this->resources, resource);
}

def(bool, IsRouteMatching, StringArray *route, StringArray *path) {
	if (route->len > path->len) {
		return false;
	}

	forward (i, route->len) {
		if (String_BeginsWith(route->buf[i], $("$"))) {
			continue;
		} else if (!String_Equals(route->buf[i], path->buf[i])) {
			return false;
		}
	}

	return true;
}

def(void, ExtractParts, StringArray *route, StringArray *path, ref(OnPart) onPart) {
	forward (i, route->len) {
		if (String_BeginsWith(route->buf[i], $("$"))) {
			if (path->buf[i].len > 0) {
				String name  = String_Slice(route->buf[i], 1);
				String value = path->buf[i];

				callback(onPart, name, value);
			}
		}
	}
}

/* Finds a matching route. */
def(MatchingRoute, FindRoute, String path) {
	StringArray *arrPath = String_Split(path, '/');

	forward (i, this->resources->len) {
		ResourceInterface *resource = this->resources->buf[i];

		forward (j, ResourceInterface_MaxRoutes) {
			ResourceRoute *route = &resource->routes[j];

			if (route->path.buf == NULL) {
				break;
			}

			StringArray *arrRoute = String_Split(route->path, '/');

			if (call(IsRouteMatching, arrRoute, arrPath)) {
				MatchingRoute match;

				match.route      = route;
				match.resource   = resource;
				match.pathElems  = arrPath;
				match.routeElems = arrRoute;

				return match;
			}

			Array_Destroy(arrRoute);
		}
	}

	Array_Destroy(arrPath);

	return (MatchingRoute) { NULL, NULL, NULL, NULL };
}
