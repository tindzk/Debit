#import "FileResponse.h"

void FileResponse(Response *resp, RdString path, DateTime lastModified) {
	try {
		File file = File_New(path, FileStatus_ReadOnly);

		Stat64 attr = File_GetStat(&file);

		if (!BitMask_Has(attr.mode, FileMode_Regular)) {
			Response_SetStatus(resp, HTTP_Status_ClientError_NotFound);
			BufferResponse(resp, $$("Not a file."));
			// Logger_Error(&logger, $("Not a file."));
		} else {
			DateTime fileTime = DateTime_FromUnixEpoch(attr.mtime.sec);

			if (DateTime_Compare(lastModified, fileTime) >= 0) {
				// Logger_Debug(&logger, $("No need to send file."));
				Response_SetStatus(resp, HTTP_Status_Redirection_NotModified);
			} else {
				// Logger_Debug(&logger, $("Sending file..."));

				RdString ext = Path_GetExtension(path);

				Response_SetContentType(resp,
					String_ToCarrier($$("application/octet-stream")));

				fwd(i, MimeTypes_Length) {
					if (String_Equals(ext, MimeTypes[i].extension)) {
						Response_SetContentType(resp,
							String_ToCarrier(
								RdString_Exalt(
									MimeTypes[i].mimeType)));

						break;
					}
				}

				Response_SetFileBody    (resp, file, attr.size);
				Response_SetLastModified(resp, fileTime);
			}

			Response_Flush(resp);
		}
	} catch(File, NotFound) {
		Response_SetStatus(resp, HTTP_Status_ClientError_NotFound);
		BufferResponse(resp, $$("File not found."));
		// Logger_Error(&logger, $("File not found."));
	} catch(File, AccessDenied) {
		Response_SetStatus(resp, HTTP_Status_ClientError_Forbidden);
		BufferResponse(resp, $$("Access denied."));
		// Logger_Error(&logger, $("Access denied."));
	} finally {

	} tryEnd;
}
