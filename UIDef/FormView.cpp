#include "FormView.h"
#include "Time.h"


LRESULT CALLBACK MainProc(HWND, UINT, WPARAM, LPARAM);

void FormView::show(HINSTANCE hInstance, HWND hWnd) {
    HWND hWndMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_FORMVIEW), hWnd, (DLGPROC)MainProc);
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
                SetDlgItemText(hWnd, IDC_STATIC, L"changed");
        }
    }

    return 0;
}