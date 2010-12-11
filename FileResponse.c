#import "FileResponse.h"

extern Logger logger;

void FileResponse(ResponseInstance resp, String path, DateTime lastModified) {
	try {
		File file;
		File_Open(&file, path, FileStatus_ReadOnly);

		Stat64 attr = File_GetStat(&file);

		if (!BitMask_Has(attr.mode, FileMode_Regular)) {
			Response_SetStatus(resp, HTTP_Status_ClientError_NotFound);
			BufferResponse(resp, $("Not a file."));
		} else {
			DateTime fileTime = DateTime_FromUnixEpoch(attr.mtime.sec);

			if (DateTime_Compare(lastModified, fileTime) >= 0) {
				Logger_Debug(&logger, $("No need to send file."));
				Response_SetStatus(resp, HTTP_Status_Redirection_NotModified);
			} else {
				Logger_Debug(&logger, $("Sending file..."));

				String contentType = $("application/octet-stream");

				ssize_t pos = String_ReverseFind(path, '.');

				if (pos != String_NotFound) {
					String ext = String_Slice(path, pos + 1);

					forward (i, MimeTypes_Length) {
						if (String_Equals(ext, MimeTypes[i].extension)) {
							contentType = MimeTypes[i].mimeType;
							break;
						}
					}
				}

				Response_SetFileBody    (resp, file, attr.size);
				Response_SetContentType (resp, contentType);
				Response_SetLastModified(resp, fileTime);
			}
		}
	} clean catch(File, NotFound) {
		Response_SetStatus(resp, HTTP_Status_ClientError_NotFound);
		BufferResponse(resp, $("File not found."));
	} catch(File, AccessDenied) {
		Response_SetStatus(resp, HTTP_Status_ClientError_Forbidden);
		BufferResponse(resp, $("Access denied."));
	} finally {

	} tryEnd;
}
