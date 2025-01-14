#include "FormView.h"

LRESULT CALLBACK MainProc(HWND, UINT, WPARAM, LPARAM);

void FormView::show(HINSTANCE hInstance, HWND hWnd) {
    HWND hWndMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_FORMVIEW), hWnd, (DLGPROC)MainProc);
}

void viewColumns(HWND hWnd) {
    HWND hWndList = GetDlgItem(hWnd, IDC_LIST_FILE);

    LVCOLUMN lvColumn;
    ZeroMemory(&lvColumn, sizeof(LVCOLUMN));
    lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;

    lvColumn.pszText = (WCHAR*)L"DateTime";
    lvColumn.cx = 80;
    ListView_InsertColumn(hWndList, 0, &lvColumn);

    lvColumn.pszText = (WCHAR*)L"File Path";
    lvColumn.cx = 400;
    ListView_InsertColumn(hWndList, 1, &lvColumn);

    lvColumn.pszText = (WCHAR*)L"Hash Signature";
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndList, 2, &lvColumn);

    lvColumn.pszText = (WCHAR*)L"Yara Rules";
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndList, 3, &lvColumn);

    lvColumn.pszText = (WCHAR*)L"Virtualization";
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndList, 4, &lvColumn);

}

void hashSignature(HWND hWnd, HWND hWndList) {
    
    Connection connect;
    if (connect.checkInternetConnection()) {
        std::string url = "http://10.5.101.199:9000/hash";
        SetDlgItemText(hWnd, IDC_CONNECT, L"connected");

        int listCount = ListView_GetItemCount(hWndList);
        for (int i = 0; i < listCount; i++) {
            wchar_t filePath[2048];
            ListView_GetItemText(hWndList, i, 1, filePath, sizeof(filePath));
            std::string file = Convert::WCharToStr(filePath);

            std::string md5 = File::hash(file, File::HashType::MD5);
            std::string sha1 = File::hash(file, File::HashType::SHA1);
            std::string sha256 = File::hash(file, File::HashType::SHA256);

            std::vector<std::string> hashes = { md5, sha1, sha256 };
            nlohmann::json json = {
                {"hashes", hashes }
            };

            std::string response = connect.sendPostCurl(url, json);

            nlohmann::json responseJson = nlohmann::json::parse(response);
            std::wstring resultHash = L"no";
            for (const auto& item : responseJson["response"]) {
                BOOL is_found = item["is_found"];
                if (is_found) {
                    resultHash = L"yes";
                }
            }

            ListView_SetItemText(hWndList, i, 2, const_cast<LPWSTR>(resultHash.c_str()));
        }
    }
    else {
        SetDlgItemText(hWnd, IDC_CONNECT, L"not connected");
    }


}

void listScannedFile(HWND hWnd, std::vector<std::filesystem::path> listFile) {
    LVITEM lvItem;
    ZeroMemory(&lvItem, sizeof(LVITEM));
    lvItem.mask = LVIF_TEXT;

    HWND hWndList = GetDlgItem(hWnd, IDC_LIST_FILE);
     //SendMessageA(hWndList, LB_RESETCONTENT, 0, 0);
     //SendMessageA(hWndList, LB_SETHORIZONTALEXTENT, (WPARAM)1000, 0);
    int i = 0;
    for (const auto& file : listFile) {
        //std::string filePath = "datetime\t" + file.string() + "\thashsignature\tyararules\tvisualization";
        std::wstring filePath = file.wstring();
        LPWSTR lpwstrFile = &filePath[0];
        //SendMessageA(hWndList, LB_ADDSTRING, 0, (LPARAM)filePath.c_str());
        lvItem.iItem = i;
        lvItem.pszText = (WCHAR*)L"datetime";
        ListView_InsertItem(hWndList, &lvItem);
        ListView_SetItemText(hWndList, i, 1, lpwstrFile);
        i++;
    }

    std::thread threadHashSignature(hashSignature, hWnd, hWndList);
    threadHashSignature.detach();
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
    
    Connection connect;
    
    switch (message) {
    case WM_INITDIALOG:
        if (connect.checkInternetConnection()) {
            std::string url = "http://10.5.101.199:9000/hash";
            SetDlgItemText(hWnd, IDC_CONNECT, L"connected");
        }
        else {
            SetDlgItemText(hWnd, IDC_CONNECT, L"not connected");
        }

        viewColumns(hWnd);
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