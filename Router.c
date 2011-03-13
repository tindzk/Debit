#import "Router.h"

#define self Router

Singleton(self);
SingletonDestructor(self);

rsdef(self, New) {
	return (self) {
		.resources = Resources_New(75)
	};
}

def(void, Destroy) {
	Resources_Free(this->resources);
}

def(void, DestroyMatch, MatchingRoute match) {
	if (match.pathElems != NULL) {
		RdStringArray_Free(match.pathElems);
	}

	if (match.routeElems != NULL) {
		RdStringArray_Free(match.routeElems);
	}
}

def(void, AddResource, ResourceInterface *resource) {
	Resources_Push(&this->resources, resource);
}

static sdef(bool, IsRoot, RdStringArray *elems) {
	return elems->len == 2
		&& elems->buf[0].len == 0
		&& elems->buf[1].len == 0;
}

static def(bool, ParseSub, RdString route, RdString path, ref(OnPart) onPart) {
	ssize_t j = -1;
	size_t offset = 0;
	bool bracket = false;

	RdString name  = $("");
	RdString value = $("");

	fwd(i, route.len) {
		if (bracket) {
			if (route.buf[i] == '{') {
				throw(NestedBrackets);
			} else if (route.buf[i] == '}') {
				bracket = false;

				name  = String_Slice(route, offset, i - offset);
				value = String_Slice(path, j + 1);
			}
		} else {
			if (route.buf[i] == '{') {
				bracket = true;
				offset = i + 1;
			} else {
				RdString find = String_Slice(route, i);
				fwd(u, find.len) {
					if (find.buf[u] == '{') {
						find.len = u;
					}
				}

				ssize_t pos = String_Find(path, j + 1, find);

				if (pos == String_NotFound) {
					return false;
				}

				value = String_Slice(path, j + 1, pos - j - 1);
				callback(onPart, name, value);
				name.len = 0;

				i += find.len - 1;
				j = pos + find.len - 1;
			}
		}
	}

	if (name.len > 0) {
		callback(onPart, name, value);
	}

	return true;
}

def(bool, IsRouteMatching, RdStringArray *route, RdStringArray *path) {
	if (route->len > path->len) {
		return false;
	}

	/* '/' should not act as a fall-back route. */
	if (scall(IsRoot, route) && !scall(IsRoot, path)) {
		return false;
	}

	fwd(i, route->len) {
		if (String_BeginsWith(route->buf[i], $(":"))) {
			continue;
		}

		if (!call(ParseSub, route->buf[i], path->buf[i], EmptyCallback())) {
			return false;
		}
	}

	return true;
}

def(void, ExtractParts, RdStringArray *route, RdStringArray *path, ref(OnPart) onPart) {
	fwd(i, route->len) {
		if (String_BeginsWith(route->buf[i], $(":"))) {
			if (path->buf[i].len > 0) {
				RdString name  = String_Slice(route->buf[i], 1);
				RdString value = path->buf[i];

				callback(onPart, name, value);
			}
		} else {
			call(ParseSub, route->buf[i], path->buf[i], onPart);
		}
	}
}

/* Finds a matching route. */
def(MatchingRoute, FindRoute, RdString path) {
	RdStringArray *arrPath = String_Split(path, '/');

	fwd(i, this->resources->len) {
		ResourceInterface *resource = this->resources->buf[i];

		fwd(j, ResourceInterface_MaxRoutes) {
			ResourceRoute *route = &resource->routes[j];

			if (route->path.len == 0) {
				break;
			}

			RdStringArray *arrRoute = String_Split(route->path, '/');

			if (call(IsRouteMatching, arrRoute, arrPath)) {
				MatchingRoute match = {
					.route      = route,
					.resource   = resource,
					.pathElems  = arrPath,
					.routeElems = arrRoute
				};

				return match;
			}

			RdStringArray_Free(arrRoute);
		}
	}

	RdStringArray_Free(arrPath);

	return (MatchingRoute) { .route = NULL };
}

def(MatchingRoute, GetDefaultRoute) {
	fwd(i, this->resources->len) {
		ResourceInterface *resource = this->resources->buf[i];

		fwd(j, ResourceInterface_MaxRoutes) {
			ResourceRoute *route = &resource->routes[j];

			if (route->path.len == 0) {
				break;
			}

			if (String_Equals(route->path, $("*"))) {
				return (MatchingRoute) {
					.route    = route,
					.resource = resource
				};
			}
		}
	}

	return (MatchingRoute) { .route = NULL };
}
