#import "RedirectResponse.h"

overload void RedirectResponse(ResponseInstance resp, String location) {
	Response_SetStatus(resp, HTTP_Status_Redirection_Found);
	Response_SetLocation(resp, String_ToCarrier(location));
}

overload void RedirectResponse(ResponseInstance resp, ProtString location) {
	Response_SetStatus(resp, HTTP_Status_Redirection_Found);
	Response_SetLocation(resp, String_ToCarrier(location));
}
