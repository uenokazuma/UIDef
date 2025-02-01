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
    lvColumn.cx = 500;
    ListView_InsertColumn(hWndList, 1, &lvColumn);

    lvColumn.pszText = (WCHAR*)L"Result";
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndList, 2, &lvColumn);

    lvColumn.pszText = (WCHAR*)L"Stopped at";
    lvColumn.cx = 100;
    ListView_InsertColumn(hWndList, 3, &lvColumn);

}

int getDlgItemTextAsInt(HWND hWnd, int idDlgItem) {

    int length = GetWindowTextLength(GetDlgItem(hWnd, idDlgItem)) + 1;
    char* text = new char[length];

    GetDlgItemTextA(hWnd, idDlgItem, text, length);
    std::string textStr(text);

    delete[] text;

    try {
        return std::stoi(text);
    }
    catch (const std::invalid_argument& e) {
        return 0;
    }
    catch (const std::out_of_range& e) {
        return 0;
    }
}

void visualization(HWND hWnd, HWND hWndList, std::string filepath, size_t rowIndex, TFModel model) {

    std::string visualization = Visualization::scan(filepath, model);
    float visNum = std::stof(visualization);

    if (visNum > 0.5) {
        countTrueVis++;
        ListView_SetItemText(hWndList, rowIndex, 2, const_cast<LPWSTR>(L"yes"));
        ListView_SetItemText(hWndList, rowIndex, 3, const_cast<LPWSTR>(L"Visualization"));
    }
    else {
        countTrueBenign++;
        ListView_SetItemText(hWndList, rowIndex, 2, const_cast<LPWSTR>(L"no"));
    }
}

void yaraRules(YaraRules yara, HWND hWnd, HWND hWndList, std::string filepath, size_t rowIndex, TFModel model) {
   
    const wchar_t* result;
    if (yara.scan(filepath)) {
        result = L"yes";
        countTrueYara++;
        ListView_SetItemText(hWndList, rowIndex, 2, const_cast<LPWSTR>(L"yes"));
        ListView_SetItemText(hWndList, rowIndex, 3, const_cast<LPWSTR>(L"Yara Rules"));
    }
    else {
        result = L"no";
        visualization(hWnd, hWndList, filepath, rowIndex, model);
    }
}

void scanSerial(HWND hWnd, HWND hWndList, std::shared_ptr<std::vector<std::filesystem::path>> files) {
    //auto start = std::chrono::high_resolution_clock::now();

    Connection connect;
    auto yara = YaraRules();
    yara.compileRulesRecursive(File::getPathDir() + "\\data\\yru");

    std::string pathModel = File::getPathDir() + "\\data\\vism\\seq_binary_model";
    const char* argModel = pathModel.c_str();

    TFModel model(argModel);

    countTrueHash = 0;
    countTrueYara = 0;
    countTrueVis = 0;
    countTrueBenign = 0;

    SetDlgItemText(hWnd, IDC_SCAN_TOTAL_COUNT, Convert::IntToWstr(0).c_str());
    SetDlgItemText(hWnd, IDC_SCAN_HASH_TRUE_COUNT, Convert::IntToWstr(countTrueHash).c_str());
    SetDlgItemText(hWnd, IDC_SCAN_YARA_TRUE_COUNT, Convert::IntToWstr(countTrueYara).c_str());
    SetDlgItemText(hWnd, IDC_SCAN_VIS_TRUE_COUNT, Convert::IntToWstr(countTrueVis).c_str());
    SetDlgItemText(hWnd, IDC_SCAN_UNDETECTED_TRUE_COUNT, Convert::IntToWstr(countTrueBenign).c_str());

    if (connect.checkInternetConnection()) {
        //std::string url = "http://10.5.101.199:9000/hash";
        std::string url = "http://srv513883.hstgr.cloud:9000/hash";
        SetDlgItemText(hWnd, IDC_CONNECT, L"connected");

        //int listCount = ListView_GetItemCount(hWndList);
        int listCount = files->size();
        std::atomic<int> count = 0;
        //int countTrue = 0;
        //for (int i = 0; i < listCount; i++) {


        auto postHashSignatureBatch = [&yara, &model, &connect, url, hWnd, hWndList, &files, &count](size_t startIndex, size_t endIndex) {

            const auto size = endIndex - startIndex;

            std::vector<std::string> hashes(size * 3);

            std::vector<std::future<void>> futures;
            futures.reserve(size);

            for (size_t i = 0; i < size; i++) {
                count++;

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
                    [&yara, &model, &files, &response, hWnd, hWndList, startIndex](size_t index) {
                        bool found = false;
                        size_t rowIndex = startIndex + index;
                        for (size_t i = 0; i < 3; i++) {
                            bool isFound = response["response"][index * 3 + i]["is_found"];
                            std::string classType = response["response"][index * 3 + i]["class"];
                            if (isFound && (classType == "ransomware" || classType == "malware")) {
                                found = true;
                                countTrueHash++;
                                break;
                            }
                        }
                        //auto result = found ? L"yes" : L"no";
                        if (found) {
                            ListView_SetItemText(hWndList, rowIndex, 2, const_cast<LPWSTR>(L"yes"));
                            ListView_SetItemText(hWndList, rowIndex, 3, const_cast<LPWSTR>(L"Hash Signature"));
                        }
                        else {
                            yaraRules(yara, hWnd, hWndList, files->at(rowIndex).string(), rowIndex, model);
                        }
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
                    std::min(listCount, index + BATCH_SIZE)
                )
            );
            index += BATCH_SIZE;
        }

        while (!futures.empty()) {
            auto& first_future = futures.front();
            first_future.get();
            
            SetDlgItemText(hWnd, IDC_SCAN_TOTAL_COUNT, Convert::IntToWstr(count).c_str());
            SetDlgItemText(hWnd, IDC_SCAN_HASH_TRUE_COUNT, Convert::IntToWstr(countTrueHash).c_str());
            SetDlgItemText(hWnd, IDC_SCAN_YARA_TRUE_COUNT, Convert::IntToWstr(countTrueYara).c_str());
            SetDlgItemText(hWnd, IDC_SCAN_VIS_TRUE_COUNT, Convert::IntToWstr(countTrueVis).c_str());
            SetDlgItemText(hWnd, IDC_SCAN_UNDETECTED_TRUE_COUNT, Convert::IntToWstr(countTrueBenign).c_str());

            futures.pop_front();

            if (index < listCount) {
                futures.push_back(
                    std::async(std::launch::async,
                        postHashSignatureBatch,
                        index,
                        std::min(listCount, index + BATCH_SIZE)
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

void listScannedFile(HWND hWnd, std::shared_ptr<std::vector<std::filesystem::path>> listFile) {
    LVITEM lvItem;
    ZeroMemory(&lvItem, sizeof(LVITEM));
    lvItem.mask = LVIF_TEXT;

    HWND hWndList = GetDlgItem(hWnd, IDC_LIST_FILE);
    ListView_DeleteAllItems(hWndList);
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

    scanSerial(hWnd, hWndList, listFile);
    //std::thread threadScanSerial(scanSerial, hWnd, hWndList, listFile);
    //threadScanSerial.detach();
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

    HWND hWndEditPath = GetDlgItem(hWnd, IDC_EDIT_PATH);
    int hWndEditPathLength = GetWindowTextLength(hWndEditPath) + 1;

    char* textEditPath = new char[hWndEditPathLength];
    GetWindowTextA(hWndEditPath, textEditPath, hWndEditPathLength);
    std::string pathScan(textEditPath);
    auto file = std::make_shared<std::vector<std::filesystem::path>>();

    std::thread scan(
        [pathScan, hWnd, file] {
            File::scan(pathScan, *file);
            listScannedFile(hWnd, file);
        }
    );
    scan.detach();
}

LRESULT CALLBACK MainProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    Connection connect;

    switch (message) {
        /*case WM_NOTIFY: {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            if (pnmh->idFrom == IDC_LIST_FILE && pnmh->code == NM_CUSTOMDRAW) {
                LPNMLVCUSTOMDRAW pLvcd = (LPNMLVCUSTOMDRAW)lParam;

                switch (pLvcd->nmcd.dwDrawStage) {
                    case CDDS_ITEMPREPAINT: {
                        wchar_t text[5];
                        int row = pLvcd->nmcd.dwItemSpec;
                        HWND hWndList = GetDlgItem(hWnd, IDC_LIST_FILE);
                        ListView_GetItemText(hWndList, row, 2, text, 5);

                        if (wcscmp(text, L"yes") == 0) {
                            pLvcd->clrTextBk = RGB(255, 0, 0);
                        }
                        else if (wcscmp(text, L"no") == 0) {
                            pLvcd->clrTextBk = RGB(0, 255, 0);
                        }
                        break;
                    }
                        return CDRF_DODEFAULT;
                        break;
                    case CDDS_ITEMPOSTPAINT:
                        break;
                }
            }
            break;
        }*/
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
            SetDlgItemText(hWnd, IDC_EDIT_PATH, L"D:\\AMP\\documents");
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
            break;
    }

    return 0;
}