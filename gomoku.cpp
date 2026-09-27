/* =========================================================================
 *  ������ (Gomoku / Five-in-a-Row)
 *  C��C++����ʵ�� -- Win32 API + GDI
 *
 *  ����:
 *    1) �˵�ҳ��: ˫�˶�ս / �˻���ս ģʽѡ��
 *    2) ��ͳľ������ 19x19������λ
 *    3) �ڰ׽������ӣ�����������ʤ
 *    4) AI ��������ʽ����
 *    5) ��ť: ���䡢���塢�ٿ�һ�֡����ز˵�
 *    6) ˮī������ͼ��˫������Ⱦ
 *    7) �������� + ������Ч
 *
 *  ����:
 *    ���������ӣ�R ���ؿ���Esc ���ز˵�
 * ========================================================================= */


/* =========================================================================
 * һ��ͷ�ļ��������  
 * ========================================================================= */
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>                 
#include <windowsx.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <olectl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * ���� ȫ�ֶ�������
 * ========================================================================= */
 
/* ���ڼ���ͼƬ�� GDI+ */
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

/* ���� IID_IPicture����������δ�ṩ�� */
static const IID MY_IID_IPicture = {
    0x7BF80980, 0xBF32, 0x101A,
    {0x8B, 0xBB, 0x00, 0xAA, 0x00, 0x30, 0x0C, 0xAB}
};

/* GDI+ ȫ�ֱ��� */
static ULONG_PTR g_gdiplusToken = 0;
static Gdiplus::Image *g_pBgImage = NULL;
static Gdiplus::Image *g_pTitleImage = NULL;  /* ����ͼƬ bt.png */
static int g_titleSrcX = 0, g_titleSrcY = 0, g_titleSrcW = 0, g_titleSrcH = 0;

/* ---- �������� ---- */
#define BOARD_SIZE      19
#define WINDOW_W        820
#define WINDOW_H        880
#define MIN_CLIENT_W    540
#define MIN_CLIENT_H    580
#define BTN_AREA_H      38

/* ---- ö�� ---- */
enum GameState { STATE_MENU, STATE_GAME };
enum GameMode  { MODE_PVP, MODE_PVA };
enum Player    { PLAYER_NONE = 0, PLAYER_BLACK = 1, PLAYER_WHITE = 2 };

/* ---- ȫ�ֱ��� ---- */
static int g_state = STATE_MENU;
static int g_mode  = MODE_PVP;
static int g_board[BOARD_SIZE][BOARD_SIZE];
static int g_currentPlayer = PLAYER_BLACK;
static int g_winner = PLAYER_NONE;
static int g_moveCount = 0;
static int g_lastX = -1, g_lastY = -1;
static int g_history[BOARD_SIZE * BOARD_SIZE][3];
static int g_cellSize = 36;
static int g_boardPx  = 0;
static int g_boardOrgX = 0, g_boardOrgY = 0;
static int g_btnTop = 0, g_btnAreaH = BTN_AREA_H;

static RECT g_btnPvP, g_btnPvA;
static RECT g_btnSurrender, g_btnUndo, g_btnRestart, g_btnBack;

static HFONT g_titleFont = NULL, g_btnFont = NULL, g_subFont = NULL, g_gameBtnFont = NULL;
static HBRUSH g_btnBrush = NULL;
static HBRUSH g_woodBrush = NULL;
static HBRUSH g_bgBrush = NULL;

static int g_hoverBtn = 0;       /* �˵�: 1=˫�˶�ս, 2=�˻���ս */
static int g_hoverGameBtn = 0;  /* ��Ϸ: 1-4 �Ű�ť */

static HBITMAP g_bgBmp = NULL;   /* ����ı���λͼ */
static int g_bgBmpW = 0, g_bgBmpH = 0;
static int g_hasBgImage = 0;

/* 19x19 ���̵���λ���꣨�� 0 ��ʼ������ */
static const int g_stars[9][2] = {
    {3,3}, {3,9}, {3,15},
    {9,3}, {9,9}, {9,15},
    {15,3}, {15,9}, {15,15}
};


/* =========================================================================
 * ��������ǰ������ 
 * ========================================================================= */

/* ---- ����ǰ������ ---- */
static void StartBGM(void);
static void StopBGM(void);
static void PlayClickSound(void);
static void InitGame(int mode);
static int  CheckWin(int x, int y, int player);
static void AITurn(void);
static void DrawMenu(HDC hdc, int w, int h);
static void DrawGame(HDC hdc, int w, int h);
static void DrawBoard(HDC hdc);
static void DrawStone(HDC hdc, int x, int y, int player);
static void DrawCenteredText(HDC hdc, const wchar_t *text, RECT *rc,
                             HFONT font, COLORREF color, UINT flags);
static void LayoutBoard(int clientW, int clientH);
static void LayoutButtons(int clientW, int clientH);
static void RebuildBgBmp(int w, int h, HDC hdc);
static void DrawBackground(HDC hdc, int w, int h);
static void LoadBgImage(void);
static void FreeBgImage(void);

/* =========================================================================
 * �ġ�ҵ���� 
 * ========================================================================= */

//4.1��Ϸ�߼� 
//��ʼ��һ����Ϸ 
static void InitGame(int mode) {
    g_mode = mode;
    g_state = STATE_GAME;
    g_currentPlayer = PLAYER_BLACK;
    g_winner = PLAYER_NONE;
    g_moveCount = 0;
    g_lastX = -1; g_lastY = -1;
    memset(g_board, 0, sizeof(g_board));
    memset(g_history, 0, sizeof(g_history));
    g_hoverBtn = 0;
    g_hoverGameBtn = 0;
}

//����ʤ���ж�
static int CheckWin(int x, int y, int player) {
    int dirs[4][2] = {{1,0},{0,1},{1,1},{1,-1}};
    for (int d = 0; d < 4; d++) {
        int count = 1;
        for (int s = 1; s < 5; s++) {
            int nx = x + dirs[d][0]*s, ny = y + dirs[d][1]*s;
            if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) break;
            if (g_board[ny][nx] != player) break;
            count++;
        }
        for (int s = 1; s < 5; s++) {
            int nx = x - dirs[d][0]*s, ny = y - dirs[d][1]*s;
            if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) break;
            if (g_board[ny][nx] != player) break;
            count++;
        }
        if (count >= 5) return 1;
    }
    return 0;
}

/* AI ����ʽ������������ (x,y) ���Ӷ� player �ļ�ֵ */
static int EvaluatePos(int x, int y, int player) {//̰���㷨����ֵ���� 
    if (g_board[y][x] != PLAYER_NONE) return -1;
    int dirs[4][2] = {{1,0},{0,1},{1,1},{1,-1}};
    int score = 0;
    for (int d = 0; d < 4; d++) {
        int cnt = 1, openA = 0, openB = 0;
        int nx = x + dirs[d][0], ny = y + dirs[d][1];
        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && g_board[ny][nx] == player) {
            cnt++; nx += dirs[d][0]; ny += dirs[d][1];
        }
        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && g_board[ny][nx] == PLAYER_NONE)
            openA = 1;
        nx = x - dirs[d][0]; ny = y - dirs[d][1];
        while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && g_board[ny][nx] == player) {
            cnt++; nx -= dirs[d][0]; ny -= dirs[d][1];
        }
        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && g_board[ny][nx] == PLAYER_NONE)
            openB = 1;
        int opens = openA + openB;
        if (cnt >= 5) score += 100000;
        else if (cnt == 4) score += (opens == 2 ? 10000 : (opens == 1 ? 1000 : 0));
        else if (cnt == 3) score += (opens == 2 ? 500 : (opens == 1 ? 50 : 0));
        else if (cnt == 2) score += (opens == 2 ? 50 : (opens == 1 ? 5 : 0));
        else if (cnt == 1) score += (opens == 2 ? 5 : 1);
    }
    return score;
}

static void AITurn(void) {//�������غ���
    /* AI ִ���� (PLAYER_WHITE) */
    int bestX = -1, bestY = -1, bestScore = -1;
    int hasStone = 0;
    for (int y = 0; y < BOARD_SIZE; y++)
        for (int x = 0; x < BOARD_SIZE; x++)
            if (g_board[y][x] != PLAYER_NONE) hasStone = 1;
    if (!hasStone) {
        /* �ײ���������Ԫ�����ģ� */
        int cx = BOARD_SIZE/2, cy = BOARD_SIZE/2;
        g_board[cy][cx] = PLAYER_WHITE;
        g_history[g_moveCount][0] = cx;
        g_history[g_moveCount][1] = cy;
        g_history[g_moveCount][2] = PLAYER_WHITE;
        g_moveCount++;
        g_lastX = cx; g_lastY = cy;
        if (CheckWin(cx, cy, PLAYER_WHITE)) g_winner = PLAYER_WHITE;
        return;
    }
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (g_board[y][x] != PLAYER_NONE) continue;
            /* ֻ��������������Χ��2 ��Χ�ڣ��Ŀ�λ */
            int isNear = 0;
            for (int dy = -2; dy <= 2 && !isNear; dy++)
                for (int dx = -2; dx <= 2 && !isNear; dx++) {
                    int nx = x+dx, ny = y+dy;
                    if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && g_board[ny][nx] != PLAYER_NONE)
                        isNear = 1;
                }
            if (!isNear) continue;
            int off = EvaluatePos(x, y, PLAYER_WHITE);
            int def = EvaluatePos(x, y, PLAYER_BLACK);
            int total = off * 12 / 10 + def;
            if (total > bestScore) {
                bestScore = total;
                bestX = x; bestY = y;
            }
        }
    }
    if (bestX < 0) return;
    g_board[bestY][bestX] = PLAYER_WHITE;
    g_history[g_moveCount][0] = bestX;
    g_history[g_moveCount][1] = bestY;
    g_history[g_moveCount][2] = PLAYER_WHITE;
    g_moveCount++;
    g_lastX = bestX; g_lastY = bestY;
    if (CheckWin(bestX, bestY, PLAYER_WHITE)) g_winner = PLAYER_WHITE;
}

// 4.2��Ƶ
static void StartBGM(void) {
    /* ���ȳ��� MP3����� WAV */
    if (GetFileAttributesW(L"bgm.mp3") != INVALID_FILE_ATTRIBUTES) {
        mciSendStringW(L"close bgm", NULL, 0, NULL);
        mciSendStringW(L"open \"bgm.mp3\" type mpegvideo alias bgm", NULL, 0, NULL);
        mciSendStringW(L"play bgm repeat", NULL, 0, NULL);
    } else if (GetFileAttributesW(L"bgm.wav") != INVALID_FILE_ATTRIBUTES) {
        mciSendStringW(L"close bgm", NULL, 0, NULL);
        mciSendStringW(L"open \"bgm.wav\" type waveaudio alias bgm", NULL, 0, NULL);
        mciSendStringW(L"play bgm repeat", NULL, 0, NULL);
    }
}

static void StopBGM(void) {
    mciSendStringW(L"stop bgm", NULL, 0, NULL);
    mciSendStringW(L"close bgm", NULL, 0, NULL);
}

static void PlayClickSound(void) {
    if (GetFileAttributesW(L"click.wav") != INVALID_FILE_ATTRIBUTES) {
        mciSendStringW(L"close clicksnd", NULL, 0, NULL);
        mciSendStringW(L"open \"click.wav\" type waveaudio alias clicksnd", NULL, 0, NULL);
        mciSendStringW(L"setaudio clicksnd volume to 600", NULL, 0, NULL);
        mciSendStringW(L"play clicksnd from 0", NULL, 0, NULL);
    }
}

//4.3���� ���ô��ڷŴ���С 
static void LayoutButtons(int clientW, int clientH) {
    int bh = clientH / 8;
    if (bh < 48) bh = 48;
    if (bh > 80) bh = 80;
    int bw = clientW * 60 / 100;
    if (bw < 200) bw = 200;
    if (bw > 400) bw = 400;
    int gap = 20;
    int totalH = bh * 2 + gap;
    int top0 = (clientH - totalH) / 2 + clientH / 8;
    if (top0 < clientH / 4) top0 = clientH / 4;
    int left0 = (clientW - bw) / 2;

    g_btnPvP.left = left0;   g_btnPvP.right = left0 + bw;
    g_btnPvP.top  = top0;    g_btnPvP.bottom = top0 + bh;
    g_btnPvA.left = left0;   g_btnPvA.right = left0 + bw;
    g_btnPvA.top  = top0 + bh + gap;  g_btnPvA.bottom = top0 + 2*bh + gap;
}

static void LayoutBoard(int clientW, int clientH) {
    /* ����ڱ�׼���ڳߴ� (WINDOW_W x WINDOW_H) �����ű��� */
    float scaleW = (float)clientW / (float)WINDOW_W;
    float scaleH = (float)clientH / (float)WINDOW_H;
    float scale = (scaleW < scaleH) ? scaleW : scaleH;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    /* ��ť����߶ȣ��Ի�׼ֵ 38 ���� */
    g_btnAreaH = (int)(38 * scale);
    if (g_btnAreaH < 28) g_btnAreaH = 28;
    if (g_btnAreaH > 80) g_btnAreaH = 80;
    g_btnTop = clientH - g_btnAreaH - 30;
    if (g_btnTop < 10) g_btnTop = 10;

    int availW = clientW - 20;
    int availH = g_btnTop - 5;
    int boardPx = (availW < availH) ? availW : availH;
    if (boardPx < 100) boardPx = 100;
    /* ����ռ���ÿռ�� 90% */
    boardPx = boardPx * 9 / 10;

    g_cellSize = boardPx / (BOARD_SIZE + 1);
    if (g_cellSize < 8) g_cellSize = 8;
    g_boardPx  = g_cellSize * (BOARD_SIZE + 1);

    g_boardOrgX = (clientW - g_boardPx) / 2;
    if (g_boardOrgX < 0) g_boardOrgX = 0;
    g_boardOrgY = 5 + (g_btnTop - 5 - g_boardPx) / 2;
    if (g_boardOrgY < 5) g_boardOrgY = 5;

    /* 4 ����Ϸ��ťˮƽ���У�����׼�������� */
    int gap = (int)(10 * scale);
    if (gap < 6) gap = 6;
    int totalGap = gap * 3;
    int bw = (clientW - totalGap - 40) / 4;
    int bwMax = (int)(180 * scale);
    if (bw > bwMax) bw = bwMax;
    if (bw < 60) bw = 60;
    int bh = g_btnAreaH - 6;
    if (bh < 22) bh = 22;
    int totalW = bw * 4 + totalGap;
    int left0 = (clientW - totalW) / 2;
    if (left0 < 0) left0 = 0;
    int top = g_btnTop + (g_btnAreaH - bh) / 2;

    g_btnSurrender.left = left0;                   g_btnSurrender.right = left0 + bw;
    g_btnSurrender.top  = top;                     g_btnSurrender.bottom = top + bh;
    g_btnUndo.left      = left0 + (bw + gap);       g_btnUndo.right      = left0 + (bw + gap) + bw;
    g_btnUndo.top       = top;                     g_btnUndo.bottom     = top + bh;
    g_btnRestart.left   = left0 + (bw + gap)*2;     g_btnRestart.right   = left0 + (bw + gap)*2 + bw;
    g_btnRestart.top    = top;                     g_btnRestart.bottom  = top + bh;
    g_btnBack.left      = left0 + (bw + gap)*3;     g_btnBack.right      = left0 + (bw + gap)*3 + bw;
    g_btnBack.top       = top;                     g_btnBack.bottom     = top + bh;
}

//4.4����ͼƬ 
static IPicture *g_pic = NULL;
static int g_picW = 0, g_picH = 0;

static void LoadBgImage(void) {
    const wchar_t *files[] = {L"bg.jpg", L"bg.jpeg", L"bg.png", L"bg.bmp"};
    const wchar_t *bgFile = NULL;
    for (int i = 0; i < 4; i++) {
        if (GetFileAttributesW(files[i]) != INVALID_FILE_ATTRIBUTES) {
            bgFile = files[i];
            break;
        }
    }
    if (!bgFile) { g_hasBgImage = 0; return; }

    /* ��ʼ�� GDI+ */
    Gdiplus::GdiplusStartupInput si;
    GdiplusStartup(&g_gdiplusToken, &si, NULL);

    /* ʹ�� GDI+ ����ͼƬ */
    wchar_t fullpath[MAX_PATH];
    GetFullPathNameW(bgFile, MAX_PATH, fullpath, NULL);
    if (g_pBgImage) { delete g_pBgImage; g_pBgImage = NULL; }
    g_pBgImage = new Gdiplus::Image(fullpath);
    if (g_pBgImage->GetLastStatus() != Gdiplus::Ok) {
        delete g_pBgImage;
        g_pBgImage = NULL;
        g_hasBgImage = 0;
        return;
    }
    g_picW = g_pBgImage->GetWidth();
    g_picH = g_pBgImage->GetHeight();
    if (g_picW <= 0 || g_picH <= 0) {
        delete g_pBgImage;
        g_pBgImage = NULL;
        g_hasBgImage = 0;
        return;
    }
    g_hasBgImage = 1;

    /* ���ر���ͼƬ bt_title.png���Ѳü�Ϊ�����֣��� bt.png */
    const wchar_t *titleFiles[] = {L"bt_title.png", L"bt.png"};
    const wchar_t *titleFile = NULL;
    for (int i = 0; i < 2; i++) {
        if (GetFileAttributesW(titleFiles[i]) != INVALID_FILE_ATTRIBUTES) {
            titleFile = titleFiles[i];
            break;
        }
    }
    if (titleFile) {
        wchar_t titlePath[MAX_PATH];
        GetFullPathNameW(titleFile, MAX_PATH, titlePath, NULL);
        g_pTitleImage = new Gdiplus::Image(titlePath);
        if (g_pTitleImage->GetLastStatus() != Gdiplus::Ok) {
            delete g_pTitleImage;
            g_pTitleImage = NULL;
        }
    }
}

static void FreeBgImage(void) {
    if (g_pBgImage) { delete g_pBgImage; g_pBgImage = NULL; }
    if (g_pTitleImage) { delete g_pTitleImage; g_pTitleImage = NULL; }
    if (g_gdiplusToken) { Gdiplus::GdiplusShutdown(g_gdiplusToken); g_gdiplusToken = 0; }
    g_hasBgImage = 0;
}

static void RebuildBgBmp(int w, int h, HDC hdc) {
    if (g_bgBmp) { DeleteObject(g_bgBmp); g_bgBmp = NULL; }
    g_bgBmpW = 0;
    g_bgBmpH = 0;

    if (!g_hasBgImage || !g_pBgImage) return;

    /* �������ݵ��ڴ� DC ��λͼ */
    HDC memDC = CreateCompatibleDC(hdc);
    g_bgBmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, g_bgBmp);

    /* ��ľɫ�����Ϊ���ñ��� */
    HBRUSH wood = CreateSolidBrush(RGB(220, 179, 92));
    RECT rcAll = {0, 0, w, h};
    FillRect(memDC, &rcAll, wood);
    DeleteObject(wood);

    /* ʹ�� GDI+ �� "����" ģʽ����ͼƬ�����вü��� */
    if (g_picW > 0 && g_picH > 0) {
        int drawW, drawH, dx, dy;
        float scaleW = (float)w / g_picW;
        float scaleH = (float)h / g_picH;
        float scale = (scaleW > scaleH) ? scaleW : scaleH;
        drawW = (int)(g_picW * scale);
        drawH = (int)(g_picH * scale);
        dx = (w - drawW) / 2;
        dy = (h - drawH) / 2;

        Gdiplus::Graphics graphics(memDC);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.DrawImage(g_pBgImage, dx, dy, drawW, drawH);
    }

    SelectObject(memDC, oldBmp);
    DeleteDC(memDC);

    g_bgBmpW = w;
    g_bgBmpH = h;
}

static void DrawBackground(HDC hdc, int w, int h) {
    if (g_hasBgImage && g_bgBmp && g_bgBmpW == w && g_bgBmpH == h) {
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, g_bgBmp);
        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBmp);
        DeleteDC(memDC);
    } else {
        /* ���÷�������ɫ��� */
        RECT rc = {0, 0, w, h};
        FillRect(hdc, &rc, g_bgBrush);
    }
}

//4.5��ͼ���������������־���
static void DrawCenteredText(HDC hdc, const wchar_t *text, RECT *rc,
                             HFONT font, COLORREF color, UINT flags) {
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, text, -1, rc, flags);
    SelectObject(hdc, oldFont);
}

// 4.6�˵�ҳ�����
static void DrawMenu(HDC hdc, int w, int h) {
    /* ���� */
    DrawBackground(hdc, w, h);

    /* ���⣺����ʹ�� bt.png ͼƬ������������ */
    int titleH = h / 6;
    if (titleH < 80) titleH = 80;
    if (titleH > 150) titleH = 150;
    int titleY = h / 7;

    if (g_pTitleImage) {
        /* ���Ʊ���ͼƬ���Ѳü�Ϊ�����֣������ֿ��߱� */
        int imgW = g_pTitleImage->GetWidth();
        int imgH = g_pTitleImage->GetHeight();
        if (imgW > 0 && imgH > 0) {
            int drawH = titleH;
            int drawW = drawH * imgW / imgH;
            int dx = (w - drawW) / 2;
            int dy = titleY;
            Gdiplus::Graphics graphics(hdc);
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            graphics.DrawImage(g_pTitleImage, dx, dy, drawW, drawH);
        }
    } else {
        const wchar_t *title = L"\x4E94\x5B50\x68CB";  /* ������ */
        RECT rcTitle = {0, titleY, w, titleY + titleH};
        DrawCenteredText(hdc, title, &rcTitle, g_titleFont, RGB(40, 25, 10),
                         DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    /* ��ť */
    RECT btns[2] = {g_btnPvP, g_btnPvA};
    const wchar_t *labels[2] = {L"\x53CC\x4EBA\x5BF9\x6218",  /* ˫�˶�ս */
                                 L"\x4EBA\x673A\x5BF9\x6218"};  /* �˻���ս */
    for (int i = 0; i < 2; i++) {
        HBRUSH fill;
        if (g_hoverBtn == i+1)
            fill = CreateSolidBrush(RGB(245, 200, 120));
        else
            fill = g_btnBrush;
        FillRect(hdc, &btns[i], fill);
        if (g_hoverBtn == i+1) DeleteObject(fill);
        FrameRect(hdc, &btns[i], (HBRUSH)GetStockObject(BLACK_BRUSH));
        DrawCenteredText(hdc, labels[i], &btns[i], g_btnFont,
                         RGB(40, 25, 10), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

//4.7���̻���
static void DrawBoard(HDC hdc) {
    int x0 = g_boardOrgX, y0 = g_boardOrgY;
    int cs = g_cellSize;
    int bp = g_boardPx;

    /* ľ�ʱ��� */
    RECT rcBoard = {x0, y0, x0 + bp, y0 + bp};
    FillRect(hdc, &rcBoard, g_woodBrush);

    /* ������ */
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HPEN oldPen = (HPEN)SelectObject(hdc, gridPen);

    for (int i = 0; i < BOARD_SIZE; i++) {
        int x = x0 + cs + i * cs;
        int y1 = y0 + cs;
        int y2 = y0 + cs + (BOARD_SIZE - 1) * cs;
        MoveToEx(hdc, x, y1, NULL);
        LineTo(hdc, x, y2);

        int y = y0 + cs + i * cs;
        int x1 = x0 + cs;
        int x2 = x0 + cs + (BOARD_SIZE - 1) * cs;
        MoveToEx(hdc, x1, y, NULL);
        LineTo(hdc, x2, y);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);

    /* ��λ */
    HBRUSH starBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
    int starR = cs / 6;
    if (starR < 2) starR = 2;
    for (int i = 0; i < 9; i++) {
        int sx = x0 + cs + g_stars[i][0] * cs;
        int sy = y0 + cs + g_stars[i][1] * cs;
        RECT star = {sx - starR, sy - starR, sx + starR, sy + starR};
        FillRect(hdc, &star, starBrush);
    }

    /* ������ӱ������ DrawGame �л��ƣ�������֮�� */
}

static void DrawStone(HDC hdc, int x, int y, int player) {
    int cs = g_cellSize;
    int cx = g_boardOrgX + cs + x * cs;
    int cy = g_boardOrgY + cs + y * cs;
    int r = cs / 2 - 2;
    if (r < 4) r = 4;

    /* �����Ӱ */
    HBRUSH shadowBrush = CreateSolidBrush(RGB(100, 75, 40));
    HBRUSH oldS = (HBRUSH)SelectObject(hdc, shadowBrush);
    HPEN shPen = (HPEN)GetStockObject(NULL_PEN);
    HPEN oldShP = (HPEN)SelectObject(hdc, shPen);
    Ellipse(hdc, cx - r + 1, cy - r + 2, cx + r + 1, cy + r + 2);
    SelectObject(hdc, oldShP);
    SelectObject(hdc, oldS);
    DeleteObject(shadowBrush);

    /* �߹�λ�ã����Ϸ���ƫ��Լ 0.3*r */
    int hox = -r * 3 / 10;
    int hoy = -r * 3 / 10;

    if (player == PLAYER_BLACK) {
        /* ���壺�������� */
        HPEN np = (HPEN)GetStockObject(NULL_PEN);
        HPEN op = (HPEN)SelectObject(hdc, np);
        HBRUSH bb = CreateSolidBrush(RGB(15, 15, 18));
        HBRUSH obb = (HBRUSH)SelectObject(hdc, bb);
        Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);
        SelectObject(hdc, obb);
        DeleteObject(bb);
        /* ����С�߹⣬���ɾ� */
        int hr = r / 3; if (hr < 3) hr = 3;
        HBRUSH hl = CreateSolidBrush(RGB(95, 95, 100));
        HBRUSH oh = (HBRUSH)SelectObject(hdc, hl);
        Ellipse(hdc, cx + hox - hr, cy + hoy - hr, cx + hox + hr, cy + hoy + hr);
        SelectObject(hdc, oh);
        DeleteObject(hl);
        SelectObject(hdc, op);
    } else {
        /* ���壨�����ʸУ���ů���װף��������Ǵ��� */
        HPEN np = (HPEN)GetStockObject(NULL_PEN);
        HPEN op = (HPEN)SelectObject(hdc, np);
        for (int i = r; i >= 0; i--) {
            int t = i * 100 / (r > 0 ? r : 1);
            /* ��Ե��ůɫ (225,220,205)�����ģ���ɫ (245,243,232) */
            int R = 225 + (245 - 225) * (100 - t) / 100;
            int G = 220 + (243 - 220) * (100 - t) / 100;
            int B = 205 + (232 - 205) * (100 - t) / 100;
            HBRUSH b = CreateSolidBrush(RGB(R, G, B));
            HBRUSH ob = (HBRUSH)SelectObject(hdc, b);
            Ellipse(hdc, cx - r + i, cy - r + i, cx + r - i, cy + r - i);
            SelectObject(hdc, ob);
            DeleteObject(b);
        }
        /* �������ǿ���� */
        HPEN outP = CreatePen(PS_SOLID, 1, RGB(155, 145, 125));
        HPEN op2 = (HPEN)SelectObject(hdc, outP);
        HBRUSH nb = (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH ob2 = (HBRUSH)SelectObject(hdc, nb);
        Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);
        SelectObject(hdc, ob2);
        SelectObject(hdc, op2);
        DeleteObject(outP);
        /* ��߹⣺ů���ϵĴ��׹�ߣ������ɼ� */
        int hr = r * 2 / 5; if (hr < 3) hr = 3;
        HPEN hp = (HPEN)GetStockObject(NULL_PEN);
        HPEN ohp = (HPEN)SelectObject(hdc, hp);
        /* ������ - ���������� */
        HBRUSH glow = CreateSolidBrush(RGB(250, 248, 240));
        HBRUSH og = (HBRUSH)SelectObject(hdc, glow);
        Ellipse(hdc, cx + hox - hr - 1, cy + hoy - hr - 1, cx + hox + hr + 1, cy + hoy + hr + 1);
        SelectObject(hdc, og);
        DeleteObject(glow);
        /* �ڲ����� - ���� */
        HBRUSH hl = CreateSolidBrush(RGB(255, 255, 255));
        HBRUSH oh = (HBRUSH)SelectObject(hdc, hl);
        Ellipse(hdc, cx + hox - hr, cy + hoy - hr, cx + hox + hr, cy + hoy + hr);
        SelectObject(hdc, oh);
        DeleteObject(hl);
        SelectObject(hdc, ohp);
    }
}

//4.8��Ϸҳ�����
static void DrawGame(HDC hdc, int w, int h) {
    /* ���� */
    DrawBackground(hdc, w, h);

    /* ���� */
    DrawBoard(hdc);

    /* ���� */
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (g_board[y][x] != PLAYER_NONE) {
                DrawStone(hdc, x, y, g_board[y][x]);
            }
        }
    }

    /* ������ӱ�� - �����Ϸ��ĺ�� */
    if (g_lastX >= 0 && g_lastY >= 0 && g_board[g_lastY][g_lastX] != PLAYER_NONE) {
        int cs = g_cellSize;
        int x0 = g_boardOrgX;
        int y0 = g_boardOrgY;
        int lx = x0 + cs + g_lastX * cs;
        int ly = y0 + cs + g_lastY * cs;
        int sr = cs / 8;
        if (sr < 2) sr = 2;
        HPEN dotPen = (HPEN)GetStockObject(NULL_PEN);
        HPEN oldDotPen = (HPEN)SelectObject(hdc, dotPen);
        HBRUSH dotBrush = CreateSolidBrush(RGB(220, 40, 40));
        HBRUSH oldDotBrush = (HBRUSH)SelectObject(hdc, dotBrush);
        Ellipse(hdc, lx - sr, ly - sr, lx + sr, ly + sr);
        SelectObject(hdc, oldDotBrush);
        DeleteObject(dotBrush);
        SelectObject(hdc, oldDotPen);
    }

    /* ��Ϸ��ť */
    {
        RECT btns[4] = {g_btnSurrender, g_btnUndo, g_btnRestart, g_btnBack};
        const wchar_t *labels[4] = {L"\x8BA4\x8F93",           /* ���� */
                                     L"\x6094\x68CB",          /* ���� */
                                     L"\x518D\x5F00\x4E00\x5C40",  /* �ٿ�һ�� */
                                     L"\x8FD4\x56DE\x83DC\x5355"};  /* ���ز˵� */
        int canUndo = (g_moveCount > 0 && g_winner == PLAYER_NONE) ? 1 : 0;
        /* �����С�水ť�߶����ţ���׼ 24 �� */
        int btnH = btns[0].bottom - btns[0].top;
        int fontSize = btnH * 24 / 32;
        if (fontSize < 14) fontSize = 14;
        if (fontSize > 48) fontSize = 48;
        HFONT dynFont = CreateFontW(fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, 0, L"\x5FAE\x8F6F\x96C5\x9ED1");
        for (int i = 0; i < 4; i++) {
            HBRUSH fill;
            if (g_hoverGameBtn == i+1)
                fill = CreateSolidBrush(RGB(245, 200, 120));
            else
                fill = g_btnBrush;
            FillRect(hdc, &btns[i], fill);
            if (g_hoverGameBtn == i+1) DeleteObject(fill);
            FrameRect(hdc, &btns[i], (HBRUSH)GetStockObject(BLACK_BRUSH));
            COLORREF tcol = (i == 1 && !canUndo) ? RGB(160,160,160) : RGB(40, 25, 10);
            DrawCenteredText(hdc, labels[i], &btns[i], dynFont, tcol,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        DeleteObject(dynFont);
    }
}

/* =========================================================================
 *  �塢���ڹ���  
 * ========================================================================= */
static HWND g_hwnd = NULL;

static void DoPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rc;
    GetClientRect(hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    /* ˫���� */
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    /* ��鱳�������Ƿ���Ҫ�ؽ� */
    if (g_hasBgImage && (!g_bgBmp || g_bgBmpW != w || g_bgBmpH != h)) {
        RebuildBgBmp(w, h, hdc);
    }

    if (g_state == STATE_MENU) {
        DrawMenu(memDC, w, h);
    } else {
        DrawGame(memDC, w, h);
    }

    BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);

    EndPaint(hwnd, &ps);
}

//���ڻص�����
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        /* ���� DPI ��֪ */
        {
            HMODULE user32 = GetModuleHandleW(L"user32.dll");
            if (user32) {
                typedef BOOL (WINAPI *pSetProcessDpiAwarenessContext)(HANDLE);
                pSetProcessDpiAwarenessContext p =
                    (pSetProcessDpiAwarenessContext)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
                if (p) p((HANDLE)-4);  /* PER_MONITOR_AWARE_V2 */
            }
        }
        /* ���� */
        g_titleFont = CreateFontW(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, 0, L"\x5FAE\x8F6F\x96C5\x9ED1");  /* ΢���ź� */
        g_btnFont = CreateFontW(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, 0, L"\x5FAE\x8F6F\x96C5\x9ED1");
        g_subFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, TRUE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, 0, L"\x5FAE\x8F6F\x96C5\x9ED1");
        g_gameBtnFont = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, 0, L"\x5FAE\x8F6F\x96C5\x9ED1");
        /* ��ˢ */
        g_btnBrush = CreateSolidBrush(RGB(235, 200, 140));
        g_woodBrush = CreateSolidBrush(RGB(220, 179, 92));
        g_bgBrush = CreateSolidBrush(RGB(235, 215, 170));
        /* ���� */
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            LayoutButtons(rc.right, rc.bottom);
            LayoutBoard(rc.right, rc.bottom);
        }
        /* ���ر���ͼƬ */
        LoadBgImage();
        /* ������������ */
        StartBGM();
        return 0;

    case WM_SIZE: {
        int w = LOWORD(lParam), h = HIWORD(lParam);
        if (w > 0 && h > 0) {
            LayoutButtons(w, h);
            LayoutBoard(w, h);
            /* ʹ��������ʧЧ */
            if (g_bgBmp) { DeleteObject(g_bgBmp); g_bgBmp = NULL; }
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_GETMINMAXINFO: {
        MINMAXINFO *mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = MIN_CLIENT_W;
        mmi->ptMinTrackSize.y = MIN_CLIENT_H;
        return 0;
    }

    case WM_LBUTTONDOWN: {
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);

        if (g_state == STATE_MENU) {
            if (PtInRect(&g_btnPvP, pt)) {
                InitGame(MODE_PVP);
                InvalidateRect(hwnd, NULL, TRUE);
            } else if (PtInRect(&g_btnPvA, pt)) {
                InitGame(MODE_PVA);
                InvalidateRect(hwnd, NULL, TRUE);
            }
        } else if (g_state == STATE_GAME && g_winner == PLAYER_NONE) {
            /* ��ⰴť��� */
            if (PtInRect(&g_btnSurrender, pt)) {
                if (g_mode == MODE_PVA && g_currentPlayer == PLAYER_WHITE) return 0;
                g_winner = (g_currentPlayer == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK;
            } else if (PtInRect(&g_btnUndo, pt)) {
                if (g_moveCount > 0) {
                    if (g_mode == MODE_PVA && g_moveCount >= 2) {
                        /* ͬʱ���� AI ����ҵ������� */
                        for (int i = 0; i < 2; i++) {
                            g_moveCount--;
                            g_board[g_history[g_moveCount][1]][g_history[g_moveCount][0]] = PLAYER_NONE;
                        }
                        g_currentPlayer = PLAYER_BLACK;
                    } else {
                        g_moveCount--;
                        g_board[g_history[g_moveCount][1]][g_history[g_moveCount][0]] = PLAYER_NONE;
                        g_currentPlayer = g_history[g_moveCount][2];
                    }
                    g_lastX = (g_moveCount > 0) ? g_history[g_moveCount-1][0] : -1;
                    g_lastY = (g_moveCount > 0) ? g_history[g_moveCount-1][1] : -1;
                    g_winner = PLAYER_NONE;
                    InvalidateRect(hwnd, NULL, TRUE);
                }
                return 0;
            } else if (PtInRect(&g_btnRestart, pt)) {
                InitGame(g_mode);
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            } else if (PtInRect(&g_btnBack, pt)) {
                g_state = STATE_MENU;
                g_hoverBtn = 0;
                g_hoverGameBtn = 0;
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            } else {
                /* ���̵�� - �ҵ�����������Ľ���� */
                int cs = g_cellSize;
                /* ����ڵ�һ������� (0,0) ���������� */
                int px = pt.x - (g_boardOrgX + cs);
                int py = pt.y - (g_boardOrgY + cs);
                /* �������뵽����Ľ���� */
                int gx = (px >= 0) ? (px + cs/2) / cs : -(((-px) + cs/2) / cs);
                int gy = (py >= 0) ? (py + cs/2) / cs : -(((-py) + cs/2) / cs);
                /* ����㵽��������ĵľ��루���أ� */
                int dx = px - gx * cs;
                int dy = py - gy * cs;
                if (dx < 0) dx = -dx;
                if (dy < 0) dy = -dy;
                /* �����ݲ������ھ������ӣ������������� */
                int tol = cs / 2;
                if (gx >= 0 && gx < BOARD_SIZE && gy >= 0 && gy < BOARD_SIZE &&
                    dx <= tol && dy <= tol &&
                    g_board[gy][gx] == PLAYER_NONE) {
                    g_board[gy][gx] = g_currentPlayer;
                    g_history[g_moveCount][0] = gx;
                    g_history[g_moveCount][1] = gy;
                    g_history[g_moveCount][2] = g_currentPlayer;
                    g_moveCount++;
                    g_lastX = gx; g_lastY = gy;
                    PlayClickSound();
                    if (CheckWin(gx, gy, g_currentPlayer)) {
                        g_winner = g_currentPlayer;
                    } else if (g_moveCount >= BOARD_SIZE * BOARD_SIZE) {
                        g_winner = -1;  /* ���� */
                    } else {
                        g_currentPlayer = (g_currentPlayer == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK;
                    }
                    /* ����ˢ�£���֤��Ӧ���� */
                    InvalidateRect(hwnd, NULL, FALSE);
                    UpdateWindow(hwnd);
                    /* AI �غ� - �����ӳ�����ҿ����Լ������� */
                    if (g_mode == MODE_PVA && g_currentPlayer == PLAYER_WHITE &&
                        g_winner == PLAYER_NONE) {
                        SetTimer(hwnd, 1, 50, NULL);
                    }
                }
            }

            /* ���ʤ��/���岢�����Ի��� */
            if (g_winner != PLAYER_NONE) {
                InvalidateRect(hwnd, NULL, FALSE);
                UpdateWindow(hwnd);
                wchar_t msg[64];
                if (g_winner == PLAYER_BLACK)
                    wsprintfW(msg, L"\x9ED1\x68CB\x80DC\x5229!");
                else if (g_winner == PLAYER_WHITE)
                    wsprintfW(msg, L"\x767D\x68CB\x80DC\x5229!");
                else
                    wsprintfW(msg, L"\x548C\x68CB!");
                int ret = MessageBoxW(hwnd, msg,
                    L"\x6E38\x620F\x7ED3\x675F",
                    MB_YESNO | MB_ICONINFORMATION);
                if (ret == IDYES) {
                    InitGame(g_mode);
                    InvalidateRect(hwnd, NULL, TRUE);
                } else {
                    g_state = STATE_MENU;
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }
        }
        return 0;
    }

    case WM_TIMER:
        if (wParam == 1) {
            KillTimer(hwnd, 1);
            if (g_state == STATE_GAME && g_mode == MODE_PVA &&
                g_currentPlayer == PLAYER_WHITE && g_winner == PLAYER_NONE) {
                AITurn();
                PlayClickSound();
                g_currentPlayer = PLAYER_BLACK;
                InvalidateRect(hwnd, NULL, FALSE);
                UpdateWindow(hwnd);
                /* ��� AI �Ƿ��ʤ */
                if (g_winner != PLAYER_NONE) {
                    wchar_t msg[64];
                    if (g_winner == PLAYER_BLACK)
                        wsprintfW(msg, L"\x9ED1\x68CB\x80DC\x5229!");
                    else if (g_winner == PLAYER_WHITE)
                        wsprintfW(msg, L"\x767D\x68CB\x80DC\x5229!");
                    else
                        wsprintfW(msg, L"\x548C\x68CB!");
                    int ret = MessageBoxW(hwnd, msg,
                        L"\x6E38\x620F\x7ED3\x675F",
                        MB_YESNO | MB_ICONINFORMATION);
                    if (ret == IDYES) {
                        InitGame(g_mode);
                        InvalidateRect(hwnd, NULL, TRUE);
                    } else {
                        g_state = STATE_MENU;
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                }
            }
        }
        return 0;

    case WM_RBUTTONDOWN:
        if (g_state == STATE_GAME) {
            InitGame(g_mode);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;

    case WM_KEYDOWN:
        if (wParam == 'R' && g_state == STATE_GAME) {
            InitGame(g_mode);
            InvalidateRect(hwnd, NULL, TRUE);
        } else if (wParam == VK_ESCAPE) {
            if (g_state == STATE_GAME) {
                g_state = STATE_MENU;
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        return 0;

    case WM_MOUSEMOVE: {
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);

        if (g_state == STATE_MENU) {
            int newHover = 0;
            if (PtInRect(&g_btnPvP, pt))      newHover = 1;
            else if (PtInRect(&g_btnPvA, pt)) newHover = 2;
            if (newHover != g_hoverBtn) {
                g_hoverBtn = newHover;
                InvalidateRect(hwnd, NULL, FALSE);
            }
        } else if (g_state == STATE_GAME) {
            int newHover = 0;
            if (PtInRect(&g_btnSurrender, pt))  newHover = 1;
            else if (PtInRect(&g_btnUndo, pt))   newHover = 2;
            else if (PtInRect(&g_btnRestart, pt)) newHover = 3;
            else if (PtInRect(&g_btnBack, pt))   newHover = 4;
            if (newHover != g_hoverGameBtn) {
                g_hoverGameBtn = newHover;
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0;
    }

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            if (g_state == STATE_MENU) {
                if (PtInRect(&g_btnPvP, pt) || PtInRect(&g_btnPvA, pt)) {
                    SetCursor(LoadCursor(NULL, IDC_HAND));
                    return TRUE;
                }
            } else if (g_state == STATE_GAME) {
                if (PtInRect(&g_btnSurrender, pt) ||
                    PtInRect(&g_btnUndo, pt) ||
                    PtInRect(&g_btnRestart, pt) ||
                    PtInRect(&g_btnBack, pt)) {
                    SetCursor(LoadCursor(NULL, IDC_HAND));
                    return TRUE;
                }
            }
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);

    case WM_ERASEBKGND:
        return 1;  /* ��ֹ��˸ */

    case WM_PAINT:
        DoPaint(hwnd);
        return 0;

    case WM_DESTROY:
        StopBGM();
        FreeBgImage();
        if (g_bgBmp) DeleteObject(g_bgBmp);
        if (g_titleFont) DeleteObject(g_titleFont);
        if (g_btnFont) DeleteObject(g_btnFont);
        if (g_subFont) DeleteObject(g_subFont);
        if (g_gameBtnFont) DeleteObject(g_gameBtnFont);
        if (g_btnBrush) DeleteObject(g_btnBrush);
        if (g_woodBrush) DeleteObject(g_woodBrush);
        if (g_bgBrush) DeleteObject(g_bgBrush);
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

/* =========================================================================
 *  ����������� WinMain�������촰��  
 * ========================================================================= */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, PWSTR lpCmdLine, int nCmdShow) {
    const wchar_t *className = L"GomokuWnd";

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = className;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, className, L"\x4E94\x5B50\x68CB",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_W, WINDOW_H, NULL, NULL, hInstance, NULL);
    g_hwnd = hwnd;

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
