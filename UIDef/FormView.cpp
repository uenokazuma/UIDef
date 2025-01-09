#include "FormView.h"

LRESULT CALLBACK MainProc(HWND, UINT, WPARAM, LPARAM);

void FormView::show(HINSTANCE hInstance, HWND hWnd) {
    HWND hWndMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_FORMVIEW), hWnd, (DLGPROC)MainProc);
}

void listScannedFile(HWND hWnd, std::vector<std::filesystem::path> listFile) {
    HWND hWndList = GetDlgItem(hWnd, IDC_LIST_SCAN);

    for (const auto& file : listFile) {
        SendMessageA(hWndList, LB_ADDSTRING, 0, (LPARAM)file.string().c_str());
    }
}

void buttonBrowse(HWND hWnd) {
    
    HWND hWndEditPath = GetDlgItem(hWnd, IDC_EDIT_PATH);
    BROWSEINFO browseInfo = { 0 };
    browseInfo.lpszTitle = L"Select Folder";
    browseInfo.ulFlags = BIF_USENEWUI | BIF_NONEWFOLDERBUTTON;

    LPITEMIDLIST lpItemIdList = SHBrowseForFolder(&browseInfo);
    if (lpItemIdList != NULL) {
        wchar_t folderPath[MAX_PATH];
        if (SHGetPathFromIDList(lpItemIdList, folderPath)) {
            SetWindowTextW(hWndEditPath, folderPath);
        }

        CoTaskMemFree(lpItemIdList);
    }
}

void buttonScan(HWND hWnd) {
    //Connection connect;
    //if (connect.checkInternetConnection()) {
    //    SetDlgItemText(hWnd, IDC_STATIC, L"connected");
    //}
    //else {
    //    SetDlgItemText(hWnd, IDC_STATIC, L"not connected");
    //}
    HWND hWndEditPath = GetDlgItem(hWnd, IDC_EDIT_PATH);
    int hWndEditPathLength = GetWindowTextLength(hWndEditPath) + 1;

    char* textEditPath = new char[hWndEditPathLength];
    GetWindowTextA(hWndEditPath, textEditPath, hWndEditPathLength);
    std::string pathScan(textEditPath);
    std::vector<std::filesystem::path> file;

    File::scan(pathScan, file);
    listScannedFile(hWnd, file);
}

LRESULT CALLBACK MainProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        SetDlgItemText(hWnd, IDC_STATIC, L"Ransomware Defender");
        SetDlgItemText(hWnd, IDC_EDIT_PATH, L"D:\\AMP");
        break;
    case WM_COMMAND:
        int wmcID = LOWORD(wParam);
        switch(wmcID) {
            case IDC_BTN_BROWSE:
                buttonBrowse(hWnd);
                break;
            case IDC_BTN_SCAN:
                buttonScan(hWnd);
                break;
        }
    }

    return 0;
}