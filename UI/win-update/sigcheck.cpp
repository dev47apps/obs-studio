#include <windows.h>
#include <wincrypt.h>
#include <wintrust.h>
#include "obs-app.hpp"

// TODO: Use the new Windows APIs
// https://docs.microsoft.com/en-us/windows/win32/seccng/cng-portal

#pragma comment(lib, "crypt32.lib")
#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

const char *DEV47APPS_SUBJECT_NAME = "DEV47 APPS LTD.";

int VerifySignature(const char *filename, const BYTE *serial) {
	HCERTSTORE hStore = NULL;
	HCRYPTMSG hMsg = NULL;
	PCCERT_CONTEXT pCertContext = NULL;
	PCMSG_SIGNER_INFO pSignerInfo = NULL;
	BOOL rc = FALSE;
	DWORD dwSignerInfo;
	DWORD dwEncoding, dwContentType, dwFormatType;
	WCHAR szFileName[MAX_PATH];

	if (filename) {
		mbstowcs(szFileName, filename, MAX_PATH);
	}
	else {
		GetModuleFileNameW(NULL, szFileName, MAX_PATH);
	}

	if (!CryptQueryObject(CERT_QUERY_OBJECT_FILE, szFileName,
			CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED, CERT_QUERY_FORMAT_FLAG_BINARY,
			0, &dwEncoding, &dwContentType, &dwFormatType, &hStore, &hMsg, NULL))
		goto exit;

	// Check and Load signer info
	if (CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &dwSignerInfo))
		pSignerInfo = (PCMSG_SIGNER_INFO) LocalAlloc(LPTR, dwSignerInfo);

	if (pSignerInfo
		&& CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, (PVOID)pSignerInfo, &dwSignerInfo))
	{
		CERT_INFO CertInfo = { 0 };
		CertInfo.Issuer = pSignerInfo->Issuer;
		CertInfo.SerialNumber = pSignerInfo->SerialNumber;
		pCertContext = CertFindCertificateInStore(hStore, ENCODING, 0,
			CERT_FIND_SUBJECT_CERT, (PVOID)&CertInfo, NULL);
	}

	if (pCertContext) {
		char subject[512];
		DWORD dwLen = CertNameToStrA(ENCODING, &pCertContext->pCertInfo->Subject,
			CERT_SIMPLE_NAME_STR, subject, sizeof(subject));

		DWORD sLen = (DWORD) strlen(DEV47APPS_SUBJECT_NAME);
		if (!(dwLen > sLen
				&& strncmp(&subject[dwLen-sLen-1], DEV47APPS_SUBJECT_NAME, sLen) == 0))
			goto exit;

		if (serial) {
			/*const char alphabet[] = "0123456789abcdef";
			for (int i = 0; i < 16; i++) {
				subject[2 * i] = alphabet[serial[i] / 16];
				subject[2 * i + 1] = alphabet[serial[i] % 16];
			}
			subject[16] = 0;
			blog(LOG_INFO, "VerifySignature Serial: %s", subject);*/

			dwLen = pCertContext->pCertInfo->SerialNumber.cbData;
			for (DWORD n = 0; n < dwLen; n++) {
				const BYTE octet = (BYTE) pCertContext->pCertInfo->SerialNumber.pbData[dwLen - (n + 1)];
				if (serial[n] == 0 || serial[n] != octet)
					goto exit;
			}

		}

		rc = TRUE;
	}

exit:
	if (pSignerInfo) LocalFree(pSignerInfo);
	if (pCertContext) CertFreeCertificateContext(pCertContext);
	if (hStore) CertCloseStore(hStore, 0);
	if (hMsg) CryptMsgClose(hMsg);
	return rc;
}
