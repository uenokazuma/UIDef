#include "FormView.h"
#include "Time.h"
#include "Connection.h"


LRESULT CALLBACK MainProc(HWND, UINT, WPARAM, LPARAM);

void FormView::show(HINSTANCE hInstance, HWND hWnd) {
    HWND hWndMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_FORMVIEW), hWnd, (DLGPROC)MainProc);
}

void buttonScan(HWND hWnd) {
    Connection connect;
    if (connect.checkInternetConnection()) {
        SetDlgItemText(hWnd, IDC_STATIC, L"connected");
    }
    else {
        SetDlgItemText(hWnd, IDC_STATIC, L"not connected");
    }
}

LRESULT CALLBACK MainProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        SetDlgItemText(hWnd, IDC_STATIC, L"change text static");
        break;
    case WM_COMMAND:
        int wmcID = LOWORD(wParam);
        switch(wmcID) {
            case IDC_SCAN:
                buttonScan(hWnd);
        }
    }

    return 0;
}