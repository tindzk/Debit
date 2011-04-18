#import "RedirectResponse.h"

overload void RedirectResponse(Response *resp, String location) {
	Response_SetStatus(resp, HTTP_Status_Redirection_Found);
	Response_SetLocation(resp, String_ToCarrier(location));

	Response_Flush(resp);
}

overload void RedirectResponse(Response *resp, OmniString location) {
	Response_SetStatus(resp, HTTP_Status_Redirection_Found);
	Response_SetLocation(resp, String_ToCarrier(location));

	Response_Flush(resp);
}
