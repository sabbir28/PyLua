/*
** WinHTTP based Request library for Lua
** Provides functionality similar to Python's requests library
*/

#define lrequestlib_c
#define LUA_LIB

#include "../../include/lprefix.h"

#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <string.h>

#include "../../include/lua.h"
#include "../../include/lauxlib.h"
#include "lualib.h"

/* Helper to convert narrowed string to wide string for WinHTTP */
static LPWSTR charToWChar(const char* text) {
    int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    LPWSTR wideText = (LPWSTR)malloc(length * sizeof(WCHAR));
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wideText, length);
    return wideText;
}

/* Helper to convert wide string to narrowed string for Lua */
static char* wCharToChar(LPCWSTR wideText) {
    int length = WideCharToMultiByte(CP_UTF8, 0, wideText, -1, NULL, 0, NULL, NULL);
    char* text = (char*)malloc(length);
    WideCharToMultiByte(CP_UTF8, 0, wideText, -1, text, length, NULL, NULL);
    return text;
}

static int request_do(lua_State *L, LPCWSTR method) {
    const char *url_str = luaL_checkstring(L, 1);
    
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    URL_COMPONENTS urlComp = {0};
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwSchemeLength = (DWORD)-1;
    urlComp.dwHostNameLength = (DWORD)-1;
    urlComp.dwUrlPathLength = (DWORD)-1;
    urlComp.dwExtraInfoLength = (DWORD)-1;

    LPWSTR wUrl = charToWChar(url_str);
    if (!WinHttpCrackUrl(wUrl, 0, 0, &urlComp)) {
        free(wUrl);
        return luaL_error(L, "Invalid URL");
    }

    hSession = WinHttpOpen(L"PyLua Request/1.0", 
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME, 
                           WINHTTP_NO_PROXY_BYPASS, 0);

    if (hSession) {
        LPWSTR wHost = (LPWSTR)malloc((urlComp.dwHostNameLength + 1) * sizeof(WCHAR));
        wcsncpy(wHost, urlComp.lpszHostName, urlComp.dwHostNameLength);
        wHost[urlComp.dwHostNameLength] = L'\0';

        hConnect = WinHttpConnect(hSession, wHost, urlComp.nPort, 0);
        free(wHost);
    }

    if (hConnect) {
        hRequest = WinHttpOpenRequest(hConnect, method, urlComp.lpszUrlPath,
                                     NULL, WINHTTP_NO_REFERER, 
                                     WINHTTP_DEFAULT_ACCEPT_TYPES, 
                                     (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0);
    }

    if (hRequest) {
        BOOL bResults = WinHttpSendRequest(hRequest,
                                          WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                          WINHTTP_NO_REQUEST_DATA, 0,
                                          0, 0);

        if (bResults) bResults = WinHttpReceiveResponse(hRequest, NULL);

        if (bResults) {
            DWORD dwSize = 0;
            DWORD dwDownloaded = 0;
            LPSTR pszOutBuffer;

            /* Get Status Code first (before touching the buffer) */
            DWORD dwStatusCode = 0;
            DWORD dwSizeStatus = sizeof(dwStatusCode);
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSizeStatus,
                                WINHTTP_NO_HEADER_INDEX);

            /* Collect body into a luaL_Buffer, then push result string FIRST */
            luaL_Buffer b;
            luaL_buffinit(L, &b);
            do {
                dwSize = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
                if (dwSize == 0) break;
                pszOutBuffer = (LPSTR)malloc(dwSize + 1);
                if (!pszOutBuffer) break;
                if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded))
                    luaL_addlstring(&b, pszOutBuffer, (size_t)dwDownloaded);
                free(pszOutBuffer);
            } while (dwSize > 0);
            luaL_pushresult(&b);  /* stack: [body_string] */

            /* Get raw headers string */
            DWORD dwHeaderSize = 0;
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwHeaderSize,
                                WINHTTP_NO_HEADER_INDEX);
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                LPWSTR lpHeaderBuffer = (LPWSTR)malloc(dwHeaderSize);
                if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                        WINHTTP_HEADER_NAME_BY_INDEX, lpHeaderBuffer,
                                        &dwHeaderSize, WINHTTP_NO_HEADER_INDEX)) {
                    char *headers = wCharToChar(lpHeaderBuffer);
                    lua_pushstring(L, headers);  /* stack: [body_string, header_string] */
                    free(headers);
                } else {
                    lua_pushstring(L, "");  /* stack: [body_string, ""] */
                }
                free(lpHeaderBuffer);
            } else {
                lua_pushstring(L, "");  /* stack: [body_string, ""] */
            }

            /* Now build the result table safely, everything is already on the stack */
            lua_newtable(L);  /* stack: [body_string, header_string, table] */
            lua_pushinteger(L, dwStatusCode);
            lua_setfield(L, -2, "status_code");
            lua_pushvalue(L, -2);  /* copy header_string */
            lua_setfield(L, -2, "headers_raw");
            lua_pushvalue(L, -3);  /* copy body_string */
            lua_setfield(L, -2, "text");
            /* stack: [body_string, header_string, table] */
            lua_replace(L, -3);   /* replace body_string with table */
            lua_pop(L, 1);        /* pop header_string */
            /* stack: [table] */

            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            free(wUrl);
            return 1;
        }
    }

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    free(wUrl);
    
    return luaL_error(L, "Request failed (Error: %d)", GetLastError());
}

static int request_get(lua_State *L) {
    return request_do(L, L"GET");
}

static int request_post(lua_State *L) {
    /* For POST, we'd handle the body here. For now, basic GET-like POST. */
    return request_do(L, L"POST");
}

static const luaL_Reg requestlib[] = {
    {"get", request_get},
    {"post", request_post},
    {NULL, NULL}
};

LUAMOD_API int luaopen_request (lua_State *L) {
    luaL_newlib(L, requestlib);
    return 1;
}
