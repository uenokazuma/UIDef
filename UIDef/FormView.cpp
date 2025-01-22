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

void hashSignature(HWND hWnd, HWND hWndList, std::shared_ptr<std::vector<std::filesystem::path>> files) {
    //auto start = std::chrono::high_resolution_clock::now();

    Connection connect;
    if (connect.checkInternetConnection()) {
        std::string url = "http://srv513883.hstgr.cloud:9000/hash";
        SetDlgItemText(hWnd, IDC_CONNECT, L"connected");

        auto listCount = files->size();

        auto postHashSignatureBatch = [&connect, url, hWndList, files](size_t startIndex, size_t endIndex) {
            const auto size = endIndex - startIndex;

            std::vector<std::string> hashes(size * 3);

            std::vector<std::future<void>> futures;
            futures.reserve(size);

            for (size_t i = 0; i < size; i++) {
                futures.push_back(std::async(
                    std::launch::async,
                    [&hashes](std::string path, size_t index) {
                        hashes[index * 3] = File::hash(path, File::HashType::MD5);
                        hashes[index * 3 + 1] = File::hash(path, File::HashType::SHA1);
                        hashes[index * 3 + 2] = File::hash(path, File::HashType::SHA256);
                    },
                    files->at(startIndex + i).string(),
                    i
                ));
            }
            for (auto& f : futures) {
                f.get();
            }
            futures.clear();

            nlohmann::json json = {
                {"hashes", hashes }
            };

            nlohmann::json response = nlohmann::json::parse(connect.sendPostCurl(url, json));

            for (size_t i = 0; i < size; i++) {
                futures.push_back(std::async(
                    std::launch::async,
                    [&response, hWndList, startIndex](size_t index) {
                        bool found = false;
                        for (size_t i = 0; i < 3; i++) {
                            if (response["response"][index * 3 + i]["is_found"]) {
                                found = true;
                                break;
                            }
                        }
                        auto result = found ? L"yes" : L"no";
                        ListView_SetItemText(hWndList, startIndex + index, 2, const_cast<LPWSTR>(result));
                    },
                    i
                ));
            }
            for (auto& f : futures) {
                f.get();
            }

            };

        std::deque<std::future<void>> futures;
        int index = 0;

        while (index < listCount && futures.size() < 5) {
            futures.push_back(
                std::async(std::launch::async,
                    postHashSignatureBatch,
                    index,
                    min(listCount, index + BATCH_SIZE)
                )
            );
            index += BATCH_SIZE;
        }

        while (!futures.empty()) {
            auto& first_future = futures.front();
            first_future.get();
            futures.pop_front();

            if (index < listCount) {
                futures.push_back(
                    std::async(std::launch::async,
                        postHashSignatureBatch,
                        index,
                        min(listCount, index + BATCH_SIZE)
                    )
                );
                index += BATCH_SIZE;
            }
        }
    }
    else {
        SetDlgItemText(hWnd, IDC_CONNECT, L"not connected");
    }

    //auto end = std::chrono::high_resolution_clock::now();

    //std::chrono::duration<double> duration = end - start;
    //auto text = std::to_wstring(duration.count()) + L" s";
    //SetDlgItemText(hWnd, IDC_CONNECT, text.c_str());
}


void yaraRules(HWND hWnd, HWND hWndList) {
    int listCount = ListView_GetItemCount(hWndList);

    auto scanYara = [hWndList](int rowIndex) {
        wchar_t filePath[2048];
        ListView_GetItemText(hWndList, rowIndex, 1, filePath, sizeof(filePath));
        std::string file = Convert::WCharToStr(filePath);

        std::string responseYara = YaraRules::scan(file);
        std::wstring resultHash = Convert::StrToWstr(responseYara);

        ListView_SetItemText(hWndList, rowIndex, 3, const_cast<LPWSTR>(resultHash.c_str()));
        };

    std::deque<std::future<void>> futures;
    int index = 0;

    while (index < listCount && futures.size() < MAX_CONCURRENT_TASKS) {
        futures.push_back(
            std::async(std::launch::async,
                scanYara,
                index++
            )
        );
    }

    while (!futures.empty()) {
        auto& first_future = futures.front();
        first_future.get();
        futures.pop_front();

        if (index < listCount) {
            futures.push_back(
                std::async(std::launch::async,
                    scanYara,
                    index++
                )
            );
        }
    }
}

void listScannedFile(HWND hWnd, std::shared_ptr<std::vector<std::filesystem::path>> listFile) {
    LVITEM lvItem;
    ZeroMemory(&lvItem, sizeof(LVITEM));
    lvItem.mask = LVIF_TEXT;

    HWND hWndList = GetDlgItem(hWnd, IDC_LIST_FILE);
    //SendMessageA(hWndList, LB_RESETCONTENT, 0, 0);
    //SendMessageA(hWndList, LB_SETHORIZONTALEXTENT, (WPARAM)1000, 0);
    int i = 0;
    for (const auto& file : *listFile) {
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

    //yaraRules(hWnd, hWndList);
    std::thread threadYara(yaraRules, hWnd, hWndList);
    threadYara.detach();

    std::thread threadHashSignature(hashSignature, hWnd, hWndList, listFile);
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
    auto file = std::make_shared<std::vector<std::filesystem::path>>();

    File::scan(pathScan, *file);
    listScannedFile(hWnd, file);
}

LRESULT CALLBACK MainProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    Connection connect;

    switch (message) {
    case WM_INITDIALOG:
        if (connect.checkInternetConnection()) {
            std::string url = "http://srv513883.hstgr.cloud:9000/hash";
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
        switch (wmcID) {
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