#include <windows.h>

BOOL iathook(HINSTANCE hInstance, LPCSTR lpModuleName, LPCSTR lpFunctionName,
             LPVOID newFunction, LPVOID lpOldFunction) {
  if (NULL == hInstance) {
    hInstance = GetModuleHandle(NULL);
  }
  PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hInstance;
  PIMAGE_NT_HEADERS peHeader =
      (PIMAGE_NT_HEADERS)((BYTE *)dosHeader + dosHeader->e_lfanew);
  PIMAGE_IMPORT_DESCRIPTOR importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(
      (DWORD_PTR)hInstance +
      peHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
          .VirtualAddress);
  PIMAGE_THUNK_DATA originalThunkData, thunkData;
  for (; importDescriptor->Name; ++importDescriptor) {
    if (NULL != lpModuleName &&
        0 != lstrcmpiA(lpModuleName, (LPCSTR)((DWORD_PTR)hInstance +
                                              importDescriptor->Name))) {
      continue;
    }
    originalThunkData = (PIMAGE_THUNK_DATA)(
        (DWORD_PTR)hInstance + importDescriptor->OriginalFirstThunk);
    thunkData = (PIMAGE_THUNK_DATA)((DWORD_PTR)hInstance +
                                    importDescriptor->FirstThunk);
    for (; originalThunkData->u1.AddressOfData;
         ++originalThunkData, ++thunkData) {
      if (IMAGE_SNAP_BY_ORDINAL(originalThunkData->u1.Ordinal)) {
        continue;
      }
      PIMAGE_IMPORT_BY_NAME importByName = (PIMAGE_IMPORT_BY_NAME)(
          (DWORD_PTR)hInstance + originalThunkData->u1.AddressOfData);
      if (0 != lstrcmpA(lpFunctionName, importByName->Name)) {
        continue;
      }
      FARPROC *lpFunction = (FARPROC *)&thunkData->u1.Function;
      // DWORD flProtect;
      // VirtualProtect(lpFunction, sizeof(lpFunction), PAGE_EXECUTE_READWRITE,
      //                &flProtect);
      *(FARPROC *)lpOldFunction = *lpFunction;
      *lpFunction = newFunction;
      // VirtualProtect(lpFunction, sizeof(lpFunction), flProtect, NULL);
      return FALSE;
    }
  }
  return TRUE;
}

FARPROC _message_box_w;

int message_box_w(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) {
  if (NULL == lpCaption) {
    lpCaption = L"";
  }
  return _message_box_w(hWnd, lpText, lpCaption, uType);
}

int wmain(int argc, wchar_t *argv[], wchar_t *envp[]) {
  iathook(NULL, "USER32.dll", "MessageBoxW", message_box_w, &_message_box_w);
  MessageBoxW(NULL, L"Hello World!", NULL, MB_OK);
  return 0;
}
